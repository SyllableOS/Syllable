/*
 *  The Syllable kernel
 *  AGP busmanager
 *
 *  Contains some implementation ideas from Free|Net|OpenBSD
 *
 *  Copyright (C) 2008 Dee Sharpe
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
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <atheos/types.h>
#include <atheos/kernel.h>
#include <atheos/device.h>
#include <atheos/semaphore.h>
#include <atheos/dlist.h>
#include <atheos/pci.h>
#include <atheos/agpgart.h>
#include <atheos/agp.h>
#include <atheos/string.h>
#include <atheos/bootmodules.h>
#include <posix/errno.h>
#include <macros.h>

extern PCI_bus_s *g_psPCIBus;
extern AGP_bus_s *g_psAGPBus;
extern AGP_Bridge_s *g_psBridge;

static int v2_enable(PCI_Entry_s *psTarget, PCI_Entry_s *psMaster, uint32 nMode);
static int v3_enable(PCI_Entry_s *psTarget, PCI_Entry_s *psMaster, uint32 nMode);

static int v2_enable(PCI_Entry_s *psTarget, PCI_Entry_s *psMaster, uint32 nMode)
{
	kerndbg(KERN_DEBUG, "AGP: generic_enable: Setting AGP v2 mode\n");
		
	uint32 nTStatus, nMStatus;
	uint32 nCommand;
	int nRQ, nSBA, nFW, nRate;

	nTStatus = g_psPCIBus->read_pci_config(psTarget->nBus, psTarget->nDevice, psTarget->nFunction,
										   psTarget->u.h0.nAGPStatus, 4);
	nMStatus = g_psPCIBus->read_pci_config(psMaster->nBus, psMaster->nDevice, psMaster->nFunction,
										   psMaster->u.h0.nAGPStatus, 4);

	/* Set RQ to the min of nMode, nTStatus and nMStatus */
	nRQ = AGP_MODE_GET_RQ(nMode);
	if(AGP_MODE_GET_RQ(nTStatus) < nRQ)
		nRQ = AGP_MODE_GET_RQ(nTStatus);
	if(AGP_MODE_GET_RQ(nMStatus) < nRQ)
		nRQ = AGP_MODE_GET_RQ(nMStatus);

	/* Set SBA if all three can deal with SBA */
	nSBA = (AGP_MODE_GET_SBA(nTStatus)
	       & AGP_MODE_GET_SBA(nMStatus)
	       & AGP_MODE_GET_SBA(nMode));

	/* Similar for FW */
	nFW = (AGP_MODE_GET_FW(nTStatus)
	       & AGP_MODE_GET_FW(nMStatus)
	       & AGP_MODE_GET_FW(nMode));

	/* Figure out the max nRate */
	nRate = (AGP_MODE_GET_RATE(nTStatus)
		& AGP_MODE_GET_RATE(nMStatus)
		& AGP_MODE_GET_RATE(nMode));
	if(nRate & AGP_MODE_V2_RATE_4x)
		nRate = AGP_MODE_V2_RATE_4x;
	else if(nRate & AGP_MODE_V2_RATE_2x)
		nRate = AGP_MODE_V2_RATE_2x;
	else
		nRate = AGP_MODE_V2_RATE_1x;
	
	/* Construct the new mode word and tell the hardware */
	nCommand = 0;
	nCommand = AGP_MODE_SET_RQ(0, nRQ);
	nCommand = AGP_MODE_SET_SBA(nCommand, nSBA);
	nCommand = AGP_MODE_SET_FW(nCommand, nFW);
	nCommand = AGP_MODE_SET_RATE(nCommand, nRate);
	nCommand = AGP_MODE_SET_AGP(nCommand, 1);
	g_psPCIBus->write_pci_config(psTarget->nBus, psTarget->nDevice, psTarget->nFunction,
										   psTarget->u.h0.nAGPCommand, 4, nCommand);
	g_psPCIBus->write_pci_config(psMaster->nBus, psMaster->nDevice, psMaster->nFunction,
										   psMaster->u.h0.nAGPStatus, 4, nCommand);
	return 0;
}

static int v3_enable(PCI_Entry_s *psTarget, PCI_Entry_s *psMaster, uint32 nMode)
{
	kerndbg(KERN_DEBUG, "AGP: generic_enable: Setting AGP v3 mode\n");
	
	uint32 nTStatus, nMStatus;
	uint32 nCommand;
	int nRQ, nSBA, nFW, nRate, nARQSZ, nCal;

	nTStatus = g_psPCIBus->read_pci_config(psTarget->nBus, psTarget->nDevice, psTarget->nFunction,
										   psTarget->u.h0.nAGPStatus, 4);
	nMStatus = g_psPCIBus->read_pci_config(psMaster->nBus, psMaster->nDevice, psMaster->nFunction,
										   psMaster->u.h0.nAGPStatus, 4);

	/* Set RQ to the min of nMode, nTStatus and nMStatus */
	nRQ = AGP_MODE_GET_RQ(nMode);
	if(AGP_MODE_GET_RQ(nTStatus) < nRQ)
		nRQ = AGP_MODE_GET_RQ(nTStatus);
	if(AGP_MODE_GET_RQ(nMStatus) < nRQ)
		nRQ = AGP_MODE_GET_RQ(nMStatus);

	/*
	 * ARQSZ - Set the value to the maximum one.
	 * Don't allow the mode register to override values.
	 */
	nARQSZ = AGP_MODE_GET_ARQSZ(nMode);
	if(AGP_MODE_GET_ARQSZ(nTStatus) > nRQ)
		nRQ = AGP_MODE_GET_ARQSZ(nTStatus);
	if(AGP_MODE_GET_ARQSZ(nMStatus) > nRQ)
		nRQ = AGP_MODE_GET_ARQSZ(nMStatus);

	/* Calibration cycle - don't allow override by mode register */
	nCal = AGP_MODE_GET_CAL(nTStatus);
	if(AGP_MODE_GET_CAL(nMStatus) < nCal)
		nCal = AGP_MODE_GET_CAL(nMStatus);

	/* SBA must be supported for AGP v3. */
	nSBA = 1;

	/* Set FW if all three support it. */
	nFW = (AGP_MODE_GET_FW(nTStatus)
	       & AGP_MODE_GET_FW(nMStatus)
	       & AGP_MODE_GET_FW(nMode));
	
	/* Figure out the max nRate */
	nRate = (AGP_MODE_GET_RATE(nTStatus)
		& AGP_MODE_GET_RATE(nMStatus)
		& AGP_MODE_GET_RATE(nMode));
	if(nRate & AGP_MODE_V3_RATE_8x)
		nRate = AGP_MODE_V3_RATE_8x;
	else
		nRate = AGP_MODE_V3_RATE_4x;
		
	g_psPCIBus->write_pci_config(psTarget->nBus, psTarget->nDevice, psTarget->nFunction,
										   psTarget->u.h0.nAGPCommand, 4, 0);

	/* Construct the new mode word and tell the hardware */
	nCommand = 0;
	nCommand = AGP_MODE_SET_RQ(0, nRQ);
	nCommand = AGP_MODE_SET_ARQSZ(nCommand, nARQSZ);
	nCommand = AGP_MODE_SET_CAL(nCommand, nCal);
	nCommand = AGP_MODE_SET_SBA(nCommand, nSBA);
	nCommand = AGP_MODE_SET_FW(nCommand, nFW);
	nCommand = AGP_MODE_SET_RATE(nCommand, nRate);
	nCommand = AGP_MODE_SET_MODE_3(nCommand, 1);
	nCommand = AGP_MODE_SET_AGP(nCommand, 1);
	g_psPCIBus->write_pci_config(psTarget->nBus, psTarget->nDevice, psTarget->nFunction,
										   psTarget->u.h0.nAGPCommand, 4, nCommand);
	g_psPCIBus->write_pci_config(psMaster->nBus, psMaster->nDevice, psMaster->nFunction,
										   psMaster->u.h0.nAGPStatus, 4, nCommand);

	return 0;
}

int	generic_enable(uint32 nMode)
{
	kerndbg(KERN_DEBUG, "AGP: generic_enable: checking for available AGP mode\n");
	
	PCI_Entry_s *psMaster = g_psAGPBus->find_display();
	PCI_Entry_s *psTarget = g_psBridge->psDev;
	uint32 nTStatus, nMStatus;

	if(!psMaster) {
		kerndbg(KERN_WARNING, "AGP: can't find display\n");
		return -ENXIO;
	}

	nTStatus = g_psPCIBus->read_pci_config(psTarget->nBus, psTarget->nDevice, psTarget->nFunction,
										   psTarget->u.h0.nAGPStatus, 4);
	nMStatus = g_psPCIBus->read_pci_config(psMaster->nBus, psMaster->nDevice, psMaster->nFunction,
										   psMaster->u.h0.nAGPStatus, 4);

	/*
	 * Check display and bridge for AGP v3 support.  AGP v3 allows
	 * more variety in topology than v2, e.g. multiple AGP devices
	 * attached to one bridge, or multiple AGP bridges in one
	 * system.  This doesn't attempt to address those situations,
	 * but should work fine for a classic single AGP slot system
	 * with AGP v3.
	 */
	
	kerndbg(KERN_DEBUG, "AGP: generic_enable: AGP master supports mode 3: %s\n", (AGP_MODE_GET_MODE_3(nMStatus) ? "yes":"no"));
	kerndbg(KERN_DEBUG, "AGP: generic_enable: AGP target supports mode 3: %s\n", (AGP_MODE_GET_MODE_3(nTStatus) ? "yes":"no"));
		    	
	if(AGP_MODE_GET_MODE_3(nMode) &&
	    AGP_MODE_GET_MODE_3(nTStatus) &&
	    AGP_MODE_GET_MODE_3(nMStatus))
	    	return (v3_enable(psTarget, psMaster, nMode));
	    else
	    	return (v2_enable(psTarget, psMaster, nMode));
}

AGP_Memory_s *generic_alloc_memory(int nType, size_t nSize)
{
	kerndbg(KERN_DEBUG, "AGP: generic_alloc_memory: allocating memory - type: %d, size: %d bytes\n", nType, nSize);
	
	AGP_Memory_s *psMem;
	size_t *panOffsets;
	size_t *panSizes;
	char **apzNames;
	area_id	*panAreas;
	int nIndex, nAreaSizes, nAreaCount, nContigPages;

	if((nSize & (AGP_PAGE_SIZE - 1)) != 0)
		return NULL;

	if(g_psBridge->nAllocated + nSize > g_psBridge->nMaxMem) {
		kerndbg(KERN_WARNING, "AGP: agp_generic_alloc_memory: %d is too much memory to ask for\n", nSize);
		return NULL;
	}

	if(nType != 0) {
		kerndbg(KERN_WARNING, "AGP: agp_generic_alloc_memory: unsupported type %d\n", nType);
		return NULL;
	}

	psMem = kmalloc(sizeof(AGP_Memory_s), MEMF_KERNEL | MEMF_CLEAR);
	if(!psMem) {
		kerndbg(KERN_WARNING, "AGP: agp_generic_alloc_memory: not enough memory available for psMem allocation\n");
		return NULL;
	}
	
	/*
	 * XXXfvdl
	 * The memory here needs to be directly accessable from the
	 * AGP video card, so it should be allocated using bus_dma.
	 * However, it need not be contiguous, since individual pages
	 * are translated using the GATT.
	 *
	 * Using a large chunk of contiguous memory may get in the way
	 * of other subsystems that may need one, so we try to be friendly
	 * and ask for allocation in chunks of a minimum of 8 pages
	 * of contiguous memory on average, falling back to 4, 2 and 1
	 * if really needed. Larger chunks are preferred, since allocating
	 * a bus_dma_segment per page would be overkill.
	 */
	
	kerndbg(KERN_DEBUG, "AGP: agp_generic_alloc_memory: locking bridge\n");
	LOCK(g_psBridge->nLock);

	for (nContigPages = 8; nContigPages > 0; nContigPages >>= 1) {
		nAreaCount = (nSize / (nContigPages * PAGE_SIZE)) + 1;
		nAreaSizes = nSize / nAreaCount;
		
		kerndbg(KERN_DEBUG, "AGP: agp_generic_alloc_memory: allocating apzNames\n");
		apzNames = kmalloc(sizeof(uint32) * nAreaCount, MEMF_KERNEL | MEMF_CLEAR);
		if(apzNames == NULL) {
			kerndbg(KERN_WARNING, "AGP: agp_generic_alloc_memory: not enough memory available for panOffsets allocation\n");
			UNLOCK(g_psBridge->nLock);
			kfree(psMem);
			return NULL;
		}
		kerndbg(KERN_DEBUG, "AGP: agp_generic_alloc_memory: allocating panOffsets\n");
		panOffsets = kmalloc(sizeof(size_t) * nAreaCount, MEMF_KERNEL | MEMF_CLEAR);
		if(panOffsets == NULL) {
			kerndbg(KERN_WARNING, "AGP: agp_generic_alloc_memory: not enough memory available for panOffsets allocation\n");
			UNLOCK(g_psBridge->nLock);
			kfree(psMem);
			kfree(apzNames);
			return NULL;
		}
		kerndbg(KERN_DEBUG, "AGP: agp_generic_alloc_memory: allocating panSizes\n");
		panSizes = kmalloc(sizeof(size_t) * nAreaCount, MEMF_KERNEL | MEMF_CLEAR);
		if(panSizes == NULL) {
			kerndbg(KERN_WARNING, "AGP: agp_generic_alloc_memory: not enough memory available for panSizes allocation\n");
			UNLOCK(g_psBridge->nLock);
			kfree(psMem);
			kfree(apzNames);
			kfree(panOffsets);
			return NULL;
		}
		kerndbg(KERN_DEBUG, "AGP: agp_generic_alloc_memory: allocating panAreas\n");
		panAreas = kmalloc(sizeof(area_id) * nAreaCount, MEMF_KERNEL | MEMF_CLEAR);
		if(panAreas == NULL) {
			kerndbg(KERN_WARNING, "AGP: agp_generic_alloc_memory: not enough memory available for panAreas allocation\n");
			UNLOCK(g_psBridge->nLock);
			kfree(psMem);
			kfree(apzNames);
			kfree(panOffsets);
			kfree(panSizes);
			return NULL;
		}
		kerndbg(KERN_DEBUG, "AGP: agp_generic_alloc_memory: setting up memory area indices\n");
		for(nIndex = 0; nIndex < nAreaCount; nIndex++) {
			apzNames[nIndex] = kmalloc(sizeof(char) * 32, MEMF_KERNEL | MEMF_CLEAR);
			sprintf(apzNames[nIndex], "psMem_Area_%d\n", nIndex);
			panOffsets[nIndex] = nIndex * nAreaSizes;
			panSizes[nIndex] = nAreaSizes;
		}
		kerndbg(KERN_DEBUG, "AGP: agp_generic_alloc_memory: allocating memory areas\n");
		if(alloc_area_list(AREA_KERNEL |AREA_FULL_ACCESS | AREA_WRCOMB, AREA_FULL_LOCK, psMem->nVirtual, nAreaCount,
						   (const char *const*)apzNames, panOffsets, panSizes, panAreas) < 0) {
			kerndbg(KERN_WARNING, "AGP: agp_generic_alloc_memory: could not allocate memory areas for psMem\n");
			kfree(apzNames);
			kfree(panOffsets);
			kfree(panSizes);
			kfree(panAreas);
			continue;
		}
		kerndbg(KERN_DEBUG, "AGP: agp_generic_alloc_memory: filling out area data for psMem\n");
		psMem->apzNames = apzNames;
		psMem->panOffsets = panOffsets;
		psMem->panSizes = panSizes;
		psMem->panAreas = panAreas;
		psMem->nAreaCount = nAreaCount;
		break;
	}
	kerndbg(KERN_DEBUG, "AGP: agp_generic_alloc_memory: unlocking bridge\n");
	UNLOCK(g_psBridge->nLock);
	
	psMem->nId = g_psBridge->nNextId++;
	psMem->nSize = nSize;
	psMem->nType = 0;
	psMem->nBound = 0;
	kerndbg(KERN_DEBUG, "AGP: agp_generic_alloc_memory: adding psMem to memory list\n");
	DLIST_ADDTAIL(&g_psBridge->sMemory, psMem, psLink);
	g_psBridge->nAllocated += nSize;

	return psMem;
}

int	generic_free_memory(AGP_Memory_s *psMem)
{
	kerndbg(KERN_DEBUG, "AGP: generic_free_memory: freeing memory id %u\n", psMem->nId);
	
	int nIndex;
	
	if(psMem->nBound) {
		kerndbg(KERN_WARNING, "AGP: generic_free_memory: memory already bound\n");
		kerndbg(KERN_WARNING, "AGP: generic_free_memory: unbinding memory id memory %u\n", psMem->nId);
		AGP_UNBIND_MEMORY(g_psBridge, psMem);
	}

	g_psBridge->nAllocated -= psMem->nSize;
	DLIST_REMOVE(psMem, psLink);
	for(nIndex = 0; nIndex < psMem->nAreaCount; nIndex++) {
		kerndbg(KERN_DEBUG, "AGP: generic_free_memory: freeing psMem area id %d \n", psMem->panAreas[nIndex]);
		delete_area(psMem->panAreas[nIndex]);
	}
	kfree(psMem->apzNames);
	kfree(psMem->panOffsets);
	kfree(psMem->panSizes);
	kfree(psMem->panAreas);
	kfree(psMem);
	
	return 0;
}

int	generic_bind_memory(AGP_Memory_s *psMem, uint32 nOffset)
{
	uint32 nPhyAddr1, nPhyAddr2;
	int nIndex1, nIndex2, nIndex3, nDone;
	int nError = 0;

	/* Do some sanity checks first. */
	if (nOffset < 0 || (nOffset & (AGP_PAGE_SIZE - 1)) != 0 ||
	    nOffset + psMem->nSize > AGP_GET_APERTURE(g_psBridge)) {
		kerndbg(KERN_WARNING, "AGP: generic_bind_memory: binding memory at bad offset %#x\n", (int)nOffset);
		return -EINVAL;
	}

	LOCK(g_psBridge->nLock);

	if(psMem->nBound) {
		kerndbg(KERN_WARNING, "AGP: generic_bind_memory: memory already bound\n");
		UNLOCK(g_psBridge->nLock);
		return -EINVAL;
	}
	
	/*
	 * Bind the individual pages and flush the chipset's
	 * TLB.
	 */
	nDone = 0;
	for (nIndex1 = 0; nIndex1 < psMem->nAreaCount; nIndex1++) {
		/*
		 * Install entries in the GATT, making sure that if
		 * AGP_PAGE_SIZE < PAGE_SIZE and mem->am_size is not
		 * aligned to PAGE_SIZE, we don't modify too many GATT
		 * entries.
		 */
		get_area_physical_address(psMem->panAreas[nIndex1], (uintptr_t *)&nPhyAddr1);
		
		for(nIndex2 = 0; nIndex2 < psMem->panSizes[nIndex1] && (nDone + nIndex2) < psMem->nSize;
		     nIndex2 += AGP_PAGE_SIZE) {
			nPhyAddr2 = nPhyAddr1 + nIndex2;
			kerndbg(KERN_DEBUG, "AGP: generic_bind_memory: binding offset: %#lx to physical address: %#lx\n",
				(unsigned long)(nOffset + nDone + nIndex2), (unsigned long)nPhyAddr2);
			nError = AGP_BIND_PAGE(g_psBridge, nOffset + nDone + nIndex2, nPhyAddr2);
			if(nError) {
				/*
				 * Bail out. Reverse all the mappings
				 * and unwire the pages.
				 */
				for (nIndex3 = 0; nIndex3 < nDone + nIndex2; nIndex3 += AGP_PAGE_SIZE)
					AGP_UNBIND_PAGE(g_psBridge, nOffset + nIndex3);
				UNLOCK(g_psBridge->nLock);
				return nError;
			}
		}
		nDone += psMem->panSizes[nIndex1];
	}

	/*
	 * Flush the cpu cache since we are providing a new mapping
	 * for these pages.
	 */
	g_psAGPBus->flush_cache();

	/*
	 * Make sure the chipset gets the new mappings.
	 */
	AGP_FLUSH_TLB(g_psBridge);

	psMem->nOffset = nOffset;
	psMem->nBound = 1;

	UNLOCK(g_psBridge->nLock);
	
	kerndbg(KERN_DEBUG, "AGP: generic_free_memory: finished\n");
	
	return nError;
}

int	generic_unbind_memory(AGP_Memory_s *psMem)
{
	int nIndex;

	LOCK(g_psBridge->nLock);

	if(!psMem->nBound) {
		kerndbg(KERN_WARNING, "AGP: generic_unbind_memory: memory is not bound\n");
		UNLOCK(g_psBridge->nLock);
		return -EINVAL;
	}

	/*
	 * Unbind the individual pages and flush the chipset's
	 * TLB. Unwire the pages so they can be swapped.
	 */
	for(nIndex = 0; nIndex < psMem->nSize; nIndex += AGP_PAGE_SIZE)
		AGP_UNBIND_PAGE(g_psBridge, psMem->nOffset + nIndex);

	g_psAGPBus->flush_cache();
	AGP_FLUSH_TLB(g_psBridge);

	psMem->nOffset = 0;
	psMem->nBound = 0;

	UNLOCK(g_psBridge->nLock);

	return 0;
}








