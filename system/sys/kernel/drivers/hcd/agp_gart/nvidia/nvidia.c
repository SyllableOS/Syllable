/*-
 * Copyright (c) 2003 Matthew N. Dodd <winter@jurai.net>
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Written using information gleaned from the
 * NVIDIA nForce/nForce2 AGPGART Linux Kernel Patch.
 */

#include <posix/errno.h>
#include <atheos/kernel.h>
#include <atheos/device.h>
#include <atheos/semaphore.h>
#include <atheos/pci.h>
#include <atheos/agpgart.h>
#include <atheos/agp.h>
#include <atheos/bitops.h>
#include <atheos/udelay.h>
#include <macros.h>
#include "nvidia.h"


#define rdmsr( nReg, nLow, nHigh ) \
	__asm__ __volatile__( "rdmsr" \
			  : "=a" ( nLow ), "=d" ( nHigh ) \
			  : "c" ( nReg ) )

#define wrmsr( nReg, nLow, nHigh ) \
	__asm__ __volatile__("wrmsr" \
			  : /* no outputs */ \
			  : "c" ( nReg ), "a" ( nLow ), "d" ( nHigh ) )
			  
PCI_bus_s *g_psPCIBus;
AGP_bus_s *g_psAGPBus;
AGP_Bridge_s *g_psBridge;

static int nvidia_get_aperture(void);
static int nvidia_set_aperture(uint32 nAperture);
static int nvidia_bind_page(uint32 nOffset, uint32 nPhysical);
static int nvidia_unbind_page(uint32 nOffset);
static void nvidia_flush_tlb(void);
static int nvidia_init_iorr(uint32, uint32);

AGP_Methods_s g_snVidia_Methods = {
	nvidia_get_aperture,
	nvidia_set_aperture,
	nvidia_bind_page,
	nvidia_unbind_page,
	nvidia_flush_tlb,
	generic_enable,
	generic_alloc_memory,
	generic_free_memory,
	generic_bind_memory,
	generic_unbind_memory
};

AGP_Device_s g_sDevices[] = {
	{PCI_VENDOR_ID_NVIDIA, PCI_DEVICE_ID_NVIDIA_NFORCE, "nVidia", "NVIDIA nForce AGP Controller"},
	{PCI_VENDOR_ID_NVIDIA, PCI_DEVICE_ID_NVIDIA_NFORCE2, "nVidia", "NVIDIA nForce2 AGP Controller"}
};






static int nvidia_get_aperture(void)
{
	PCI_Entry_s *psDev = g_psBridge->psDev;
	AGP_Bridge_nVidia_s *psnVidiaBridge = (AGP_Bridge_nVidia_s*)g_psBridge->pChipCtx;
	int nApSetting, nApSize;
	
	kerndbg(KERN_DEBUG, "AGP: nvidia_get_aperture: Getting aperture\n");
	nApSetting = (g_psPCIBus->read_pci_config(psDev->nBus, psDev->nDevice,
							 	psDev->nFunction, AGP_NVIDIA_0_APSIZE, 2) & 0x0f);
	nApSize = nApSetting & 0x0f;
							 	
	switch (nApSize) {
	case 0: return (512 * 1024 * 1024); break;
	case 8: return (256 * 1024 * 1024); break;
	case 12: return (128 * 1024 * 1024); break;
	case 14: return (64 * 1024 * 1024); break;
	case 15: return (32 * 1024 * 1024); break;
	default:
		kerndbg(KERN_DEBUG, "AGP: nvidia_get_aperture: Invalid aperture setting 0x%x", nApSetting);
		return 0;
	}
}

static int nvidia_set_aperture(uint32 nAperture)
{
	kerndbg(KERN_DEBUG, "AGP: nvidia_set_aperture: setting aperture to %u\n", nAperture);
	
	PCI_Entry_s *psDev = g_psBridge->psDev;
	AGP_Bridge_nVidia_s *psnVidiaBridge = (AGP_Bridge_nVidia_s*)g_psBridge->pChipCtx;
	uint8 nVal, nKey;

	switch (nAperture) {
	case (512 * 1024 * 1024): nKey = 0; break;
	case (256 * 1024 * 1024): nKey = 8; break;
	case (128 * 1024 * 1024): nKey = 12; break;
	case (64 * 1024 * 1024): nKey = 14; break;
	case (32 * 1024 * 1024): nKey = 15; break;
	default:
		kerndbg(KERN_DEBUG, "AGP: nvidia_set_aperture: Invalid aperture nSize (%dMb)\n",
				nAperture / 1024 / 1024);
		return -EINVAL;
	}
	nVal = g_psPCIBus->read_pci_config(psDev->nBus, psDev->nDevice,	psDev->nFunction,
									   AGP_NVIDIA_0_APSIZE, 2);
	g_psPCIBus->write_pci_config(psDev->nBus, psDev->nDevice, psDev->nFunction,
								 AGP_NVIDIA_0_APSIZE, 2, ((nVal & ~0x0f) | nKey));

	return 0;
}

static int nvidia_bind_page(uint32 nOffset, uint32 nPhysical)
{
	AGP_Bridge_nVidia_s *psnVidiaBridge = (AGP_Bridge_nVidia_s*)g_psBridge->pChipCtx;
	uint32 nIndex;
	
	kerndbg(KERN_DEBUG, "AGP: nvidia_bind_page: binding offset: %#lx to physical address: %#lx\n",
				(unsigned long)nOffset, (unsigned long)nPhysical);

	if(nOffset < 0 || nOffset >= (psnVidiaBridge->psGatt->nEntries << AGP_PAGE_SHIFT))
		return -EINVAL;

	nIndex = (psnVidiaBridge->nPgOffset + nOffset) >> AGP_PAGE_SHIFT;
	
	psnVidiaBridge->psGatt->pVirtual[nIndex] = nPhysical | 1;
	return 0;
}

static int nvidia_unbind_page(uint32 nOffset)
{
	AGP_Bridge_nVidia_s *psnVidiaBridge = (AGP_Bridge_nVidia_s*)g_psBridge->pChipCtx;
	uint32 nIndex;

	kerndbg(KERN_DEBUG, "AGP: nvidia_unbind_page: unbinding offset: %#lx\n", (unsigned long)nOffset);
	
	if(nOffset < 0 || nOffset >= (psnVidiaBridge->psGatt->nEntries << AGP_PAGE_SHIFT))
		return -EINVAL;

	nIndex = (psnVidiaBridge->nPgOffset + nOffset) >> AGP_PAGE_SHIFT;
	
	psnVidiaBridge->psGatt->pVirtual[nIndex] = 0;
	return 0;
}

static void nvidia_flush_tlb(void)
{
	kerndbg(KERN_DEBUG, "AGP: nvidia_flush_tlb: flushing translation look-aside buffer\n");
	
	PCI_Entry_s *psDev = g_psBridge->psDev;
	AGP_Bridge_nVidia_s *psnVidiaBridge = (AGP_Bridge_nVidia_s*)g_psBridge->pChipCtx;
	PCI_Entry_s *psMc1Dev = psnVidiaBridge->psMc1Dev;
	uint32 nWbcReg, nTemp;
	volatile uint32 *nAgVirtual;
	int nIndex;

	if(psnVidiaBridge->nWbcMask) {
		nWbcReg = g_psPCIBus->read_pci_config(psMc1Dev->nBus, psMc1Dev->nDevice, psMc1Dev->nFunction,
									   AGP_NVIDIA_1_WBC, 4);
		nWbcReg |= psnVidiaBridge->nWbcMask;
		g_psPCIBus->write_pci_config(psMc1Dev->nBus, psMc1Dev->nDevice, psMc1Dev->nFunction,
									 AGP_NVIDIA_1_WBC, 4, nWbcReg);
		
		/* Wait no more than 3 seconds. */
		for(nIndex = 0; nIndex < 3000; nIndex++) {
			nWbcReg = g_psPCIBus->read_pci_config(psMc1Dev->nBus, psMc1Dev->nDevice, psMc1Dev->nFunction,
												  AGP_NVIDIA_1_WBC, 4);
			if((psnVidiaBridge->nWbcMask & nWbcReg) == 0)
				break;
			else
				udelay(1000);
		}
		if(nIndex == 3000)
			kerndbg(KERN_DEBUG, "AGP: nvidia_flush_tlb: TLB flush took more than 3 seconds.\n");
	}

	nAgVirtual = (volatile uint32 *)psnVidiaBridge->psGatt->pVirtual;

	/* Flush TLB entries. */
	for(nIndex = 0; nIndex < 32 + 1; nIndex++)
		nTemp = nAgVirtual[nIndex * PAGE_SIZE / sizeof(uint32)];
	for(nIndex = 0; nIndex < 32 + 1; nIndex++)
		nTemp = nAgVirtual[nIndex * PAGE_SIZE / sizeof(uint32)];
}


static int nvidia_init_iorr(uint32 nAddr, uint32 nSize)
{
	uint32 nBaseLow, nBaseHigh;
	uint32 nMaskLow, nMaskHigh;
	uint32 nSysLow, nSysHigh;
	uint32 nIorrAddr, nFreeIorrAddr;

	/* Find the iorr that is already used for the addr */
	/* If not found, determine the uppermost available iorr */
	nFreeIorrAddr = AMD_K7_NUM_IORR;
	for(nIorrAddr = 0; nIorrAddr < AMD_K7_NUM_IORR; nIorrAddr++) {
		rdmsr(IORR_BASE0 + 2 * nIorrAddr, nBaseLow, nBaseHigh);
		rdmsr(IORR_MASK0 + 2 * nIorrAddr, nMaskLow, nMaskHigh);

		if((nBaseLow & 0xfffff000ULL) == (nAddr & 0xfffff000))
			break;

		if((nMaskLow & 0x00000800ULL) == 0)
			nFreeIorrAddr = nIorrAddr;
	}

	if(nIorrAddr >= AMD_K7_NUM_IORR) {
		nIorrAddr = nFreeIorrAddr;
		if(nIorrAddr >= AMD_K7_NUM_IORR)
			return -EINVAL;
	}

	nBaseHigh = 0x0;
	nBaseLow = (nAddr & ~0xfff) | 0x18;
	nMaskHigh = 0xf;
	nMaskLow = ((~(nSize - 1)) & 0xfffff000) | 0x800;
	wrmsr(IORR_BASE0 + 2 * nIorrAddr, nBaseLow, nBaseHigh);
	wrmsr(IORR_MASK0 + 2 * nIorrAddr, nMaskLow, nMaskHigh);

	rdmsr(SYSCFG, nSysLow, nSysHigh);
	nSysLow |= 0x00100000ULL;
	wrmsr(SYSCFG, nSysLow, nSysHigh);

	return 0;
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

status_t device_init(int nDeviceID)
{
	AGP_Bridge_nVidia_s *psnVidiaBridge;
	PCI_Entry_s *psADev = psnVidiaBridge->psADev;
	PCI_Entry_s *psMc1Dev = psnVidiaBridge->psMc1Dev;
	PCI_Entry_s *psMc2Dev = psnVidiaBridge->psMc2Dev;
	PCI_Entry_s *psBDev = psnVidiaBridge->psBDev;
	AGP_Gatt_s *psGatt;
	PCI_Entry_s *psDev;
	AGP_Node_s *psNode;
	PCI_Info_s sInfo;
	uint32 nApBase;
	uint32 nApLimit;
	uint32 nTemp;
	int nSize;
	int nIndex;
	int nError;
	uint32 nAGPReg, nAGPRegVal;
	int nNumDevices = sizeof(g_sDevices) / sizeof(AGP_Device_s), nDeviceNum, nPCINum;
	char zTemp[255];
	char zNodePath[255];
	bool bDevFound = false;

	/* Get PCI busmanager */
	g_psPCIBus = get_busmanager(PCI_BUS_NAME, PCI_BUS_VERSION);
	if(g_psPCIBus == NULL)
		return -ENODEV;

	/* Get AGP busmanager */
	g_psAGPBus = get_busmanager(AGP_BUS_NAME, AGP_BUS_VERSION);
	if(g_psAGPBus == NULL)
		return -ENODEV;
	
	/* Look for the device */
	for(nPCINum = 0; g_psPCIBus->get_pci_info(&sInfo, nPCINum) == 0; ++nPCINum) {
		for(nDeviceNum = 0; nDeviceNum < nNumDevices; ++nDeviceNum) {
			/* Compare vendor and device id */
			if(sInfo.nVendorID == g_sDevices[nDeviceNum].nVendorID && sInfo.nDeviceID == g_sDevices[nDeviceNum].nDeviceID) {
				
				/******      ACTUAL DEVICE SETUP STARTS HERE      ******/
				
				sprintf(zTemp, "%s %s", g_sDevices[nDeviceNum].zVendorName, g_sDevices[nDeviceNum].zDeviceName);
				sInfo.nHandle = register_device( zTemp, PCI_BUS_NAME );
				
				nAGPReg = g_psPCIBus->get_pci_capability(sInfo.nBus, sInfo.nDevice, sInfo.nFunction, PCI_CAP_ID_AGP);
				if(nAGPReg <= 0) {
					continue;
				}
				
				nAGPRegVal = g_psPCIBus->read_pci_config(sInfo.nBus, sInfo.nDevice, sInfo.nFunction, nAGPReg, 4);
				
				if(claim_device(nDeviceID, sInfo.nHandle, zTemp, DEVICE_SYSTEM) != 0) {
					continue;
				}
				printk("%s found\n", zTemp);
				
				printk("Allocating memory for Generic AGP Bridge structure\n");
				g_psBridge = kmalloc(sizeof(AGP_Bridge_s), MEMF_KERNEL | MEMF_CLEAR | MEMF_NOBLOCK);
				if(!g_psBridge) {
					printk("Error: Out of memory\n");
					continue;
				}
				printk("Allocating memory for nVidia Bridge structure\n");
				psnVidiaBridge = kmalloc(sizeof(AGP_Bridge_nVidia_s), MEMF_KERNEL | MEMF_CLEAR | MEMF_NOBLOCK);
				if(!psnVidiaBridge) {
					printk("Error: Out of memory\n");
					kfree(g_psBridge);
					continue;
				}				
				psNode = kmalloc(sizeof(AGP_Node_s), MEMF_KERNEL | MEMF_CLEAR | MEMF_NOBLOCK);
				if(!psNode) {
					printk("Error: Out of memory\n");
					kfree(psnVidiaBridge);
					kfree(g_psBridge);
					continue;
				}				
				printk("Allocating memory for PCI device entry structure (AGP Controller)\n");
				psDev = kmalloc(sizeof(PCI_Entry_s), MEMF_KERNEL | MEMF_CLEAR | MEMF_NOBLOCK);
				if(!psDev) {
					printk("Error: Out of memory\n");
					kfree(psNode);
					kfree(psnVidiaBridge);
					kfree(g_psBridge);
					continue;
				}
				
				printk("Allocating memory for PCI device entry structure (Memory Controller 1)\n");
				psMc1Dev = kmalloc(sizeof(PCI_Entry_s), MEMF_KERNEL | MEMF_CLEAR | MEMF_NOBLOCK);
				if(!psMc1Dev) {
					printk("Error: Out of memory\n");
					kfree(psDev);
					kfree(psNode);
					kfree(psnVidiaBridge);
					kfree(g_psBridge);
					continue;
				}
				
				printk("Allocating memory for PCI device entry structure (Memory Controller 2)\n");
				psMc2Dev = kmalloc(sizeof(PCI_Entry_s), MEMF_KERNEL | MEMF_CLEAR | MEMF_NOBLOCK);
				if(!psMc2Dev) {
					printk("Error: Out of memory\n");
					kfree(psMc1Dev);
					kfree(psDev);
					kfree(psNode);
					kfree(psnVidiaBridge);
					kfree(g_psBridge);
					continue;
				}
				
				printk("Allocating memory for PCI device entry structure (Bridge)\n");
				psBDev = kmalloc(sizeof(PCI_Entry_s), MEMF_KERNEL | MEMF_CLEAR | MEMF_NOBLOCK);
				if(!psBDev) {
					printk("Error: Out of memory\n");
					kfree(psMc2Dev);
					kfree(psMc1Dev);
					kfree(psDev);
					kfree(psNode);
					kfree(psnVidiaBridge);
					kfree(g_psBridge);
					continue;
				}
				
				printk("Reading PCI Header Information\n");	
				g_psPCIBus->read_pci_header(psDev, psNode->sInfo.nBus, psNode->sInfo.nDevice, psNode->sInfo.nFunction);
								
				psDev->u.h0.nAGPStatus = nAGPReg + PCI_AGP_STATUS;
				psDev->u.h0.nAGPCommand = nAGPReg + PCI_AGP_COMMAND;
				
				memcpy(&psNode->sInfo, &sInfo, sizeof(PCI_Info_s));
				sprintf(psNode->zName, zTemp);
				
				g_psBridge->pChipCtx = psnVidiaBridge;
				g_psBridge->nAGPReg = nAGPReg;
				g_psBridge->psDev = psDev;
				g_psBridge->psNode = psNode;
				g_psBridge->nDeviceID = nDeviceID;
				g_psBridge->psMethods = &g_snVidia_Methods;
				g_psBridge->nMaxMem = 0;
				g_psBridge->nNextId = 0;
				
				g_psBridge->sVersion.nMajor = AGP_VERSION_MAJOR(nAGPRegVal);
				g_psBridge->sVersion.nMinor = AGP_VERSION_MINOR(nAGPRegVal);
				printk("AGP %u.%u is supported\n", g_psBridge->sVersion.nMajor, g_psBridge->sVersion.nMinor);
				
				/******      You must call attach_bridge() before atnTempting to use any other AGP busmanager function      ******/
				
				printk("Attaching %s to AGP busmanager\n", psNode->zName);
				if(g_psAGPBus->attach_bridge(g_psBridge) < 0) {
					printk("Failed to attach to AGP busmanager, shutdown & exit gracefully!\n");
					kfree(psBDev);
					kfree(psMc2Dev);
					kfree(psMc1Dev);
					kfree(psDev);
					kfree(psNode);
					kfree(psnVidiaBridge);
					kfree(g_psBridge);
					continue;
				}
				
				printk("Mapping aperture\n");
				if(g_psAGPBus->map_aperture(PCI_AGP_APBASE) != 0) {
					printk("Can't map aperture\n");
					kfree(psBDev);
					kfree(psMc2Dev);
					kfree(psMc1Dev);
					kfree(psDev);
					kfree(psNode);
					kfree(psnVidiaBridge);
					kfree(g_psBridge);
					continue;
				}
				
				/* setup nWbcMask */
				switch (sInfo.nDeviceID)
				{
					case PCI_DEVICE_ID_NVIDIA_NFORCE:
					{
						psnVidiaBridge->nWbcMask = 0x00010000;
					} break;
					case PCI_DEVICE_ID_NVIDIA_NFORCE2:
					{
						psnVidiaBridge->nWbcMask = 0x80000000;
					} break;
					default:
					{
						printk("Bad chip id\n");
						kfree(psBDev);
						kfree(psMc2Dev);
						kfree(psMc1Dev);
						kfree(psDev);
						kfree(psNode);
						kfree(psnVidiaBridge);
						kfree(g_psBridge);
						continue;
					}
				}
				
				/****** setup all AGP Controller interfaces ******/
				
				/* AGP Controller */
				psADev = psDev;
				
				/* Memory Controller 1 */
				g_psPCIBus->read_pci_header(psMc1Dev, psNode->sInfo.nBus, 0, 1);
				if(psMc1Dev == NULL) {
					printk("Unable to find NVIDIA Memory Controller 1\n");
					kfree(psBDev);
					kfree(psMc2Dev);
					kfree(psMc1Dev);
					kfree(psDev);
					kfree(psNode);
					kfree(psnVidiaBridge);
					kfree(g_psBridge);
					continue;
				}
				
				/* Memory Controller 2 */
				g_psPCIBus->read_pci_header(psMc2Dev, psNode->sInfo.nBus, 0, 2);
				if(psMc2Dev == NULL) {
					printk("Unable to find NVIDIA Memory Controller 2\n");
					kfree(psBDev);
					kfree(psMc2Dev);
					kfree(psMc1Dev);
					kfree(psDev);
					kfree(psNode);
					kfree(psnVidiaBridge);
					kfree(g_psBridge);
					continue;
				}
				
				/* AGP Host to PCI Bridge */
				g_psPCIBus->read_pci_header(psBDev, psNode->sInfo.nBus, 30, 0);
				if(psnVidiaBridge->psBDev == NULL) {
					printk("Unable to find NVIDIA AGP Host to PCI Bridge\n");
					kfree(psBDev);
					kfree(psMc2Dev);
					kfree(psMc1Dev);
					kfree(psDev);
					kfree(psNode);
					kfree(psnVidiaBridge);
					kfree(g_psBridge);
					continue;
				}

				printk("Getting initial aperture size\n");
				psnVidiaBridge->nInitAp = g_psBridge->nApSize = AGP_GET_APERTURE(g_psBridge);
				
				for(;;) {
					printk("Allocating GATT\n");
					psGatt = g_psAGPBus->alloc_gatt();
					if(psGatt)
						break;
				
					/*
					 * Probably contigmalloc failure. Try reducing the
					 * aperture so that the gatt nSize reduces.
					 */
					printk("Failed to allocate GATT, shutdown & exit gracefully!\n");
					if(AGP_SET_APERTURE(g_psBridge, AGP_GET_APERTURE(g_psBridge) / 2)) {
						kfree(psBDev);
						kfree(psMc2Dev);
						kfree(psMc1Dev);
						kfree(psDev);
						kfree(psNode);
						kfree(psnVidiaBridge);
						kfree(g_psBridge);
						continue;
					}
				}
				
				nApBase = g_psBridge->nApBase;
				nApLimit = nApBase + AGP_GET_APERTURE(g_psBridge) - 1;
				
				g_psPCIBus->write_pci_config(psMc2Dev->nBus, psMc2Dev->nDevice, psMc2Dev->nFunction,
									 AGP_NVIDIA_2_APBASE, 4, nApBase);
				g_psPCIBus->write_pci_config(psMc2Dev->nBus, psMc2Dev->nDevice, psMc2Dev->nFunction,
									 AGP_NVIDIA_2_APLIMIT, 4, nApLimit);
				
				g_psPCIBus->write_pci_config(psBDev->nBus, psBDev->nDevice, psBDev->nFunction,
									 AGP_NVIDIA_3_APBASE, 4, nApBase);
				g_psPCIBus->write_pci_config(psBDev->nBus, psBDev->nDevice, psBDev->nFunction,
									 AGP_NVIDIA_3_APLIMIT, 4, nApLimit);
				
				nError = nvidia_init_iorr(nApBase, AGP_GET_APERTURE(g_psBridge));
				if(nError) {
					printk("Failed to setup IORRs\n");
					kfree(psBDev);
					kfree(psMc2Dev);
					kfree(psMc1Dev);
					kfree(psDev);
					kfree(psNode);
					kfree(psnVidiaBridge);
					kfree(g_psBridge);
					continue;
				}
				
				/* directory nSize is 64k */
				nSize = AGP_GET_APERTURE(g_psBridge) / 1024 / 1024;
				psnVidiaBridge->nNumDirs = nSize / 64;
				psnVidiaBridge->nNumActiveEntries = (nSize == 32) ? 16384 : ((nSize * 1024) / 4);
				psnVidiaBridge->nPgOffset = 0;
				if(psnVidiaBridge->nNumDirs == 0) {
					psnVidiaBridge->nNumDirs = 1;
					psnVidiaBridge->nNumActiveEntries /= (64 / nSize);
					psnVidiaBridge->nPgOffset = (nApBase & (64 * 1024 * 1024 - 1) &
							 ~(AGP_GET_APERTURE(g_psBridge) - 1)) / PAGE_SIZE;
				}
				
				/* (G)ATT Base Address */
				for(nIndex = 0; nIndex < 8; nIndex++) {
					g_psPCIBus->write_pci_config(psMc2Dev->nBus, psMc2Dev->nDevice, psMc2Dev->nFunction,
									AGP_NVIDIA_2_ATTBASE(nIndex), 4, (psnVidiaBridge->psGatt->nPhysical +
									(nIndex % psnVidiaBridge->nNumDirs) * 64 * 1024) | 1);
				}
				
				/* GTLB Control */
				nTemp = g_psPCIBus->read_pci_config(psMc2Dev->nBus, psMc2Dev->nDevice, psMc2Dev->nFunction,
													AGP_NVIDIA_2_GARTCTRL, 4);
				g_psPCIBus->write_pci_config(psMc2Dev->nBus, psMc2Dev->nDevice, psMc2Dev->nFunction,
											 AGP_NVIDIA_2_GARTCTRL, 4, nTemp | 0x11);
				
				/* GART Control */
				nTemp = g_psPCIBus->read_pci_config(psADev->nBus, psADev->nDevice, psADev->nFunction,
													AGP_NVIDIA_0_APSIZE, 4);
				g_psPCIBus->write_pci_config(psADev->nBus, psADev->nDevice, psADev->nFunction,
											 AGP_NVIDIA_0_APSIZE, 4, nTemp | 0x100);				
				
				/*
				 * The mutex is used to prevent re-entry to
				 * agp_generic_bind_memory() since that function can sleep.
				 */
				printk("Creating mutex for AGP bridge\n");
				g_psBridge->nLock = CREATE_MUTEX("agp_bridge_mtx");
				
				DLIST_HEAD_INIT(&g_psBridge->sMemory);
				
				psnVidiaBridge->psGatt = psGatt;
				
				/******      ACTUAL DEVICE SETUP ENDS HERE      ******/
				
				bDevFound = true;
			}
		}
	}

	if(!bDevFound) {
		disable_device(nDeviceID);
		return(-ENODEV);
	}
	
	printk("%s initialized\n", zTemp);
		
	return 0;
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

status_t device_uninit(int nDeviceID)
{
	return 0;
}

