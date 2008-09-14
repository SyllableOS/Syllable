/*
 *  The Syllable kernel
 *  AGP busmanager
 *
 *  Contains some implementation ideas from Free|Net|OpenBSD
 *
 *  Copyright(C) 2008 Dee Sharpe
 *
 *	Two licenses are covered in this code.  All portions originating from
 *	BSD sources are covered by the BSD LICENSE.  Everything else is covered
 *	by the GPL v2 LICENSE.  Both licenses are included for your viewing pleasure.
 *	in no order of importance.
 *//*
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of version 2 of the GNU
 *  General Public License as published by the Free Software
 *  Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *//*
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <atheos/types.h>
#include <atheos/kernel.h>
#include <atheos/device.h>
#include <atheos/dlist.h>
#include <atheos/pci.h>
#include <atheos/agpgart.h>
#include <atheos/agp.h>
#include <atheos/string.h>
#include <atheos/bootmodules.h>
#include <posix/errno.h>
#include <macros.h>


void flush_cache(void);
void get_info(AGP_Info_s *psInfo);
int acquire(void);
int release(void);
int enable(uint32 nMode);
AGP_Memory_s *alloc_memory(AGP_Allocate_s *psAlloc);
void free_memory(int nId);
int bind_memory(AGP_Bind_s *psBind);
int unbind_memory(AGP_Unbind_s *psUnbind);

static AGP_Memory_s *find_memory(AGP_Bridge_s *psBridge, int nId);
static int acquire_helper(AGP_Acquire_State_e eState);
static int release_helper(AGP_Acquire_State_e eState);

PCI_bus_s *g_psBus;
AGP_Bridge_s *g_psBridge;

int g_nAGPHandle;
int g_nAGPNumDevices = 0;
PCI_Entry_s *g_apsAGPDevice[MAX_AGP_DEVICES];

static uint32 g_aAgpMax[][2] = {
	{0,	0},
	{32,	4},
	{64,	28},
	{128,	96},
	{256,	204},
	{512,	440},
	{1024,	942},
	{2048,	1920},
	{4096,	3932}
};
#define AGP_MAX_SIZE	(sizeof(g_aAgpMax) / sizeof(g_aAgpMax[0]))

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

status_t agp_open(void *pNode, uint32 nFlags, void **pCookie)
{
	AGP_Node_s *psNode = pNode;

	kerndbg(KERN_DEBUG, "AGP: %s opened\n", psNode->zName);
	
	if(!g_psBridge->nOpened) {
		g_psBridge->nOpened = 1;
		g_psBridge->eState = AGP_ACQUIRE_FREE;
	} else
		return -EBUSY;

	return 0;
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

status_t agp_close(void *pNode, void *pCookie)
{
	AGP_Node_s *psNode = pNode;

	kerndbg(KERN_DEBUG, "AGP: %s closed\n", psNode->zName);
	
	AGP_Memory_s *psMem;

	/*
	 * Clear the GATT and force release on last close
	 */
	if(g_psBridge->eState == AGP_ACQUIRE_USER) {
		while((psMem = DLIST_FIRST(&g_psBridge->sMemory))) {
			if(psMem->nBound) {
				kerndbg(KERN_WARNING, "AGP: agpclose: psMem %d is bound\n", psMem->nId);
				AGP_UNBIND_MEMORY(g_psBridge, psMem);
			}			
			AGP_FREE_MEMORY(g_psBridge, psMem);
		}
		g_psBridge->eState = AGP_ACQUIRE_FREE;
	}
	g_psBridge->nOpened = 0;

	return 0;
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

status_t agp_ioctl(void *pNode, void *pCookie, uint32 nCommand, void *pArgs, bool bFromKernel)
{
	AGP_Node_s *psNode = pNode;
	int nError = 0;

	switch(nCommand) {
		case AGPIOC_INFO:
			{
				kerndbg(KERN_DEBUG, "AGP: ioctl: AGPIOC_INFO\n");
				
				AGP_Info_s sInfo;
				
				memcpy_from_user(&sInfo, pArgs, sizeof(AGP_Info_s));
				get_info(&sInfo);
				memcpy_to_user(pArgs, &sInfo, sizeof(AGP_Info_s));
			} break;
		case AGPIOC_ACQUIRE:
			{
				kerndbg(KERN_DEBUG, "AGP: ioctl: AGPIOC_ACQUIRE\n");
				
				acquire_helper(AGP_ACQUIRE_USER);
			} break;
		case AGPIOC_RELEASE:
			{
				kerndbg(KERN_DEBUG, "AGP: ioctl: AGPIOC_RELEASE\n");
				
				release_helper(AGP_ACQUIRE_FREE);
			} break;
		case AGPIOC_SETUP:
			{
				kerndbg(KERN_DEBUG, "AGP: ioctl: AGPIOC_SETUP\n");
				
				AGP_Setup_s sSetup;
				
				memcpy_from_user(&sSetup, pArgs, sizeof(AGP_Setup_s));
				nError = enable(sSetup.nMode);
			} break;
		case AGPIOC_ALLOCATE:
			{
				kerndbg(KERN_DEBUG, "AGP: ioctl: AGPIOC_ALLOCATE\n");
				
				AGP_Memory_s *psMem;
				AGP_Allocate_s sAlloc;
		
				memcpy_from_user(&sAlloc, pArgs, sizeof(AGP_Allocate_s));
						
				psMem = alloc_memory(&sAlloc);
				if(psMem) {
					sAlloc.nKey = psMem->nId;
					sAlloc.nPhysical = psMem->nPhysical;
					
					memcpy_to_user(pArgs, &sAlloc, sizeof(AGP_Allocate_s));
					break;
				} else {
				nError = -ENOMEM;
				}
			} break;
		case AGPIOC_DEALLOCATE:
			{
				kerndbg(KERN_DEBUG, "AGP: ioctl: AGPIOC_DEALLOCATE\n");
				
				int nId;
				
				memcpy_from_user(&nId, pArgs, sizeof(int));
				
				free_memory(nId);
			} break;
		case AGPIOC_BIND:
			{
				kerndbg(KERN_DEBUG, "AGP: ioctl: AGPIOC_BIND\n");
				
				AGP_Bind_s sBind;
				
				memcpy_from_user(&sBind, pArgs, sizeof(AGP_Bind_s));
				
				nError = bind_memory(&sBind);
			} break;
		case AGPIOC_UNBIND:
			{
				kerndbg(KERN_DEBUG, "AGP: ioctl: AGPIOC_UNBIND\n");
				
				AGP_Unbind_s sUnbind;
				
				memcpy_from_user(&sUnbind, pArgs, sizeof(AGP_Unbind_s));
				
				nError = unbind_memory(&sUnbind);
			} break;
		case AGPIOC_MMAP:
			{
				kerndbg(KERN_DEBUG, "AGP: ioctl: AGPIOC_MMAP\n");
				
				AGP_Mmap_s sMmap;
				
				memcpy_from_user(&sMmap, pArgs, sizeof(AGP_Mmap_s));
				
				if(sMmap.nOffset > AGP_GET_APERTURE(g_psBridge))
					sMmap.pAperture = NULL;
				else { /******
						*      Horrible hack follows!!!
						*
						*      There is, currently, no way to limit the size of
						*      space that sMmap.pAperture has access to, so, 
						*      sMmap.pAperture has access to everything that comes
						*      after sMmap.nOffset.  BE VERY CAREFUL ABOUT USING THIS!!!
						******/
					sMmap.pAperture = (uintptr_t*)(g_psBridge->nApBase + sMmap.nOffset);
					
					kerndbg(KERN_DEBUG, "AGP: ioctl: AGPIOC_MMAP: mapping address %p\n", sMmap.pAperture);
				}
				
				memcpy_to_user(pArgs, &sMmap, sizeof(AGP_Mmap_s));
			} break;
		default:
			{
				kerndbg(KERN_DEBUG, "AGP: ioctl: default\n");
				
				nError = -ENOIOCTLCMD;
			}
	}
	return nError;	
}

DeviceOperations_s g_sOperations = {
	agp_open,
	agp_close,
	agp_ioctl,
	NULL,
	NULL
};

/* Helper functions for implementing user/kernel api */

static int acquire_helper(AGP_Acquire_State_e eState)
{
	int nError;
	
	if(g_psBridge->eState != AGP_ACQUIRE_FREE)
		nError = -EBUSY;
	g_psBridge->eState = eState;

	return 0;
}

static int release_helper(AGP_Acquire_State_e eState)
{
	if(g_psBridge->eState == AGP_ACQUIRE_FREE)
		return 0;	
	g_psBridge->eState = AGP_ACQUIRE_FREE;
	
	return 0;
}

static AGP_Memory_s *find_memory(AGP_Bridge_s *psBridge, int nId)
{
	AGP_Memory_s *psMem;

	kerndbg(KERN_DEBUG, "AGP: searching for memory block %d\n", nId);
	DLIST_FOR_EACH(&psBridge->sMemory, psMem, psLink) {
		kerndbg(KERN_DEBUG, "AGP: considering memory block %d\n", psMem->nId);
		if(psMem->nId == nId)
			return psMem;
	}
	return 0;
}

status_t attach_bridge(AGP_Bridge_s *psBridge)
{
	system_info* psSysInfo;
	PCI_Entry_s *psEntry;
	PCI_Info_s sInfo;
	int nMemSize, nIndex, nPCINum;
	uint8 nAGPReg;
	
	if(g_psBridge) {
		kerndbg(KERN_WARNING, "AGP: attach_bridge: cannot attach %s, %s is already attached\n",
				psBridge->psNode->zName, g_psBridge->psNode->zName);
		return -EBUSY;
	}
	
	kerndbg(KERN_DEBUG, "AGP: attach_bridge: attaching %s\n", psBridge->psNode->zName);
	
	/* Go ahead and get reference to PCI busmanager for future use */
	g_psBus = get_busmanager(PCI_BUS_NAME, PCI_BUS_VERSION);
	
	if(g_nAGPNumDevices == 0) {
		kerndbg(KERN_DEBUG, "AGP: Building list of AGP devices\n");
		for(nPCINum = 0; g_psBus->get_pci_info(&sInfo, nPCINum) == 0; ++nPCINum)
		{
			nAGPReg = g_psBus->get_pci_capability(sInfo.nBus, sInfo.nDevice, sInfo.nFunction, PCI_CAP_ID_AGP);
			if(nAGPReg > 0) {
				psEntry = kmalloc( sizeof(PCI_Entry_s), MEMF_KERNEL | MEMF_CLEAR );
				g_psBus->read_pci_header(psEntry, sInfo.nBus, sInfo.nDevice, sInfo.nFunction);
				g_apsAGPDevice[g_nAGPNumDevices++] = psEntry;
			}
		}
	}
	
	kerndbg(KERN_DEBUG, "Allocating memory for system_info structure\n");
	psSysInfo = kmalloc(sizeof(system_info), MEMF_KERNEL | MEMF_CLEAR | MEMF_NOBLOCK);
	if(!psSysInfo) {
		kerndbg(KERN_DEBUG, "Error: Out of memory\n");
		return -ENOMEM;
	}
	
	/*
	 * Work out an upper bound for agp memory allocation. This
	 * uses a heurisitc table from the Linux driver.
	 */
	kerndbg(KERN_DEBUG, "AGP: Getting memory values from system_info\n");
	get_system_info(psSysInfo, SYS_INFO_VERSION);
	nMemSize = (ptoa(psSysInfo->nMaxPages) >> 20);
	
	kerndbg(KERN_DEBUG, "AGP: Filling up AGP max memory array\n");
	for(nIndex = 0; nIndex < AGP_MAX_SIZE; nIndex++) {
		if(nMemSize <= g_aAgpMax[nIndex][0])
			break;
	}
	
	kerndbg(KERN_DEBUG, "AGP: Upper memory boundry array index is %d\n", nIndex);
	if(nIndex == AGP_MAX_SIZE)
		nIndex = AGP_MAX_SIZE - 1;
	psBridge->nMaxMem = (g_aAgpMax[nIndex][1] << 20U);
	kerndbg(KERN_DEBUG, "AGP: Upper memory boundry is %d MBs\n", psBridge->nMaxMem / (1024 ^ 2));
	
	/* Create node path */
	printk("Creating agpgart node\n");
	if((g_nAGPHandle = create_device_node(psBridge->nDeviceID, psBridge->psDev->nHandle,
	   "agpgart", &g_sOperations, psBridge->psNode)) < 0) {
		kerndbg(KERN_WARNING, "AGP: Failed to create device node agpgart\n");
	}
	
	psBridge->nAttached++;
	
	g_psBridge = psBridge;
	
	return 0;
}

status_t remove_bridge(void)
{
	kerndbg(KERN_DEBUG, "AGP: remove_bridge: removing %s\n", g_psBridge->psNode->zName);
	
	if(!g_psBridge) {
		kerndbg(KERN_WARNING, "AGP: remove_bridge: cannot remove, no bridge attached\n");
		return -EINVAL;
	} else {
		delete_device_node(g_nAGPHandle);
		flush_cache();
		g_psBridge->nAttached--;
		g_psBridge = NULL;
	}
	
	return 0;
}

void flush_cache(void)
{
	kerndbg(KERN_INFO, "AGP: flush_cache: flushing cache (NEEDS TO BE CPU AGNOSTIC!!!)\n");
	
	/**************FIX ME!!!!!!! THIS NEEDS TO BE CPU AGNOSTIC IN ORDER FOR PORTING *****************/
	__asm __volatile("wbinvd");
}

AGP_Gatt_s *alloc_gatt(void)
{
	uint32 nApSize = AGP_GET_APERTURE(g_psBridge);
	uint32 nEntries = nApSize >> AGP_PAGE_SHIFT;
	AGP_Gatt_s *psGatt;
	
	kerndbg(KERN_DEBUG, "AGP: allocating GATT for aperture of size %dMB\n", nApSize /(1024*1024));

	if(nEntries == 0) {
		kerndbg(KERN_WARNING, "AGP: bad aperture size\n");
		return NULL;
	}

	kerndbg(KERN_DEBUG, "AGP: Allocating memory for GATT structure\n");
	psGatt = kmalloc(sizeof(AGP_Gatt_s), MEMF_KERNEL | MEMF_CLEAR);
	if(!psGatt)
		return NULL;

	psGatt->nEntries = nEntries;
	kerndbg(KERN_DEBUG, "AGP: Creating area for GATT\n");
	psGatt->nId = create_area("agp_gatt",(void **)&psGatt->pVirtual, nEntries * sizeof(uint32),
							  nEntries * sizeof(uint32), AREA_KERNEL | AREA_FULL_ACCESS, AREA_CONTIGUOUS);
	if(psGatt->nId < 0) {
		kerndbg(KERN_WARNING, "AGP: gatt area creation failed\n");
		return NULL;
	}
	
	if(!psGatt->pVirtual) {
		kerndbg(KERN_WARNING, "AGP: contiguous allocation failed\n");
		delete_area(psGatt->nId);
		kfree(psGatt);
		return NULL;
	}
	kerndbg(KERN_DEBUG, "AGP: Getting physical memory address for GATT\n");
	get_area_physical_address(psGatt->nId,(uintptr_t *)&psGatt->nPhysical);
	kerndbg(KERN_DEBUG, "AGP: Flushing cache\n");
	flush_cache();

	return psGatt;
}

void free_gatt(AGP_Gatt_s *psGatt)
{
	kerndbg(KERN_DEBUG, "AGP: free_gatt: freeing GATT - memory area - %u\n", psGatt->nId);
	
	delete_area(psGatt->nId);
	kfree(psGatt);
}

status_t map_aperture(int nReg)
{
	PCI_Entry_s *psDev = g_psBridge->psDev;
	
	kerndbg(KERN_DEBUG, "AGP: map_aperture: calling pci->get_bar_info\n");
	
	if(g_psBus->get_bar_info(psDev, (uintptr_t *)&g_psBridge->nApBase, (uintptr_t *)&g_psBridge->nApSize,
							nReg, PCI_BAR_TYPE_MEM) != 0) {
		kerndbg(KERN_WARNING, "AGP: map_aperture: could not get register info for register 0x%x\n", nReg);
		return -ENXIO;
	}
	
	kerndbg(KERN_DEBUG, "AGP: map_aperture: aperture should be create at %p\n", g_psBridge->nApBase);
	
	if(g_psBridge->nApAreaId = create_area("agp_aperture", (void **)&g_psBridge->nApBase, PAGE_ALIGN(g_psBridge->nApSize),
									  PAGE_ALIGN(g_psBridge->nApSize), AREA_BASE_ADDRESS | AREA_KERNEL | AREA_SHARED |
									  AREA_WRCOMB | AREA_FULL_ACCESS, AREA_FULL_LOCK) < 0)
	{
		kerndbg(KERN_WARNING, "AGP: map_aperture: could not create area for aperture map\n");
		return -ENXIO;
	}
	
	kerndbg(KERN_DEBUG, "AGP: map_aperture: aperture created at %p\n", g_psBridge->nApBase);
	
	return 0;
}

PCI_Entry_s *find_display(void)
{
	kerndbg(KERN_DEBUG, "AGP: find_display: finding pci entry for AGP video card\n");
	
	PCI_Entry_s *psDev;
	int nIndex;
	
	for(nIndex = 0; nIndex < g_nAGPNumDevices; nIndex++)
	{
		if(g_apsAGPDevice[nIndex]->nClassBase == PCI_DISPLAY && 
		   g_apsAGPDevice[nIndex]->nClassSub == PCI_VGA)
		{
			psDev = g_apsAGPDevice[nIndex];
			return psDev;
		}
	}
	return NULL;
}

/* Implementation of the kernel api */

AGP_Acquire_State_e get_state(void)
{
	return g_psBridge->eState;
}

void get_info(AGP_Info_s *psInfo)
{
	psInfo->sVersion.nMajor = g_psBridge->sVersion.nMajor;
	psInfo->sVersion.nMinor = g_psBridge->sVersion.nMinor;
	psInfo->nVendor = g_psBridge->psDev->nVendorID;
	psInfo->nDevice = g_psBridge->psDev->nDeviceID;
	if(g_psBridge->nApBase)
		psInfo->nMode = g_psBus->read_pci_config(g_psBridge->psDev->nBus, g_psBridge->psDev->nDevice, g_psBridge->psDev->nFunction,
								g_psBridge->psDev->u.h0.nAGPStatus, 4);
	else
		psInfo->nMode = 0; /* i810 doesn't have real AGP */
	psInfo->nApBase = g_psBridge->nApBase;
	psInfo->nApSize = AGP_GET_APERTURE(g_psBridge) >> 20;
	psInfo->nPgTotal = psInfo->nPgSystem = g_psBridge->nMaxMem >> AGP_PAGE_SHIFT;
	psInfo->nPgUsed = g_psBridge->nAllocated >> AGP_PAGE_SHIFT;
}

int acquire(void)
{
	return acquire_helper(AGP_ACQUIRE_KERNEL);
}

int release(void)
{
	return release_helper(AGP_ACQUIRE_KERNEL);
}

int enable(uint32 nMode)
{
	return AGP_ENABLE(g_psBridge, nMode);
}

AGP_Memory_s *alloc_memory(AGP_Allocate_s *psAlloc)
{
	return AGP_ALLOC_MEMORY(g_psBridge, psAlloc->nType, psAlloc->nPgCount << AGP_PAGE_SHIFT);
}

void free_memory(int nId)
{
	AGP_Memory_s *psMem = find_memory(g_psBridge, nId);
	if(psMem)
		AGP_FREE_MEMORY(g_psBridge, psMem);
}

int bind_memory(AGP_Bind_s *psBind)
{
	AGP_Memory_s *psMem = find_memory(g_psBridge, psBind->nKey);			
	if(!psMem)
		return -ENOENT;
	
	return AGP_BIND_MEMORY(g_psBridge, psMem, psBind->nPgStart << AGP_PAGE_SHIFT);
}

int unbind_memory(AGP_Unbind_s *psUnbind)
{
	AGP_Memory_s *psMem = find_memory(g_psBridge, psUnbind->nKey);
	if(!psMem)
		return -ENOENT;
		
	return AGP_UNBIND_MEMORY(g_psBridge, psMem);
}

void memory_info(void *pHandle, AGP_Memory_Info_s *psMemInfo)
{
	AGP_Memory_s *psMem = (AGP_Memory_s *)pHandle;

	psMemInfo->nSize = psMem->nSize;
	psMemInfo->nPhysical = psMem->nPhysical;
	psMemInfo->nOffset = psMem->nOffset;
	psMemInfo->nBound = psMem->nBound;
}

AGP_bus_s sBus = {
	attach_bridge,
	remove_bridge,
	flush_cache,
	alloc_gatt,
	free_gatt,
	map_aperture,
	find_display,
	get_state,
	get_info,
	acquire,
	release,
	enable,
	alloc_memory,
	free_memory,
	bind_memory,
	unbind_memory,
	memory_info
};

void *agp_bus_get_hooks(int nVersion)
{
	if(nVersion != AGP_BUS_VERSION)
		return NULL;
	return(void *)&sBus;
}

status_t device_init(int nDeviceID)
{
	/* Check if the use of the bus is disabled */
	int i;
	int argc;
	const char *const *argv;
	bool bDisableAGP = false;

	get_kernel_arguments(&argc, &argv);

	for(i = 0; i < argc; ++i)
	{
		if(get_bool_arg(&bDisableAGP, "disable_agp=", argv[i], strlen(argv[i])))
			if(bDisableAGP)
			{
				printk("AGP bus disabled by user\n");
				return -1;
			}
	}

	register_busmanager(nDeviceID, "agp", agp_bus_get_hooks);

	printk("AGP: Busmanager initialized\n");

	return 0;
}

status_t device_uninit(int nDeviceID)
{
	return 0;
}










