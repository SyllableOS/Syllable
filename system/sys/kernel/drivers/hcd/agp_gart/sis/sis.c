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
 *//*-
 * Copyright (c) 2000 Doug Rabson
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

#include <posix/errno.h>
#include <atheos/kernel.h>
#include <atheos/device.h>
#include <atheos/semaphore.h>
#include <atheos/pci.h>
#include <atheos/agpgart.h>
#include <atheos/agp.h>
#include <atheos/bitops.h>
#include <macros.h>
#include "sis.h"


PCI_bus_s *g_psPCIBus;
AGP_bus_s *g_psAGPBus;
AGP_Bridge_s *g_psBridge;

static int sis_get_aperture(void);
static int sis_set_aperture(uint32 nAperture);
static int sis_bind_page(uint32 nOffset, uint32 nPhysical);
static int sis_unbind_page(uint32 nOffset);
static void sis_flush_tlb(void);

AGP_Methods_s g_sSis_Methods = {
	sis_get_aperture,
	sis_set_aperture,
	sis_bind_page,
	sis_unbind_page,
	sis_flush_tlb,
	generic_enable,
	generic_alloc_memory,
	generic_free_memory,
	generic_bind_memory,
	generic_unbind_memory
};

AGP_Device_s g_sDevices[] = {
	{0x1039,  0x0001, "SiS", "SiS 5591 AGP->PCI Bridge"},
	{0x1039,  0x0530, "SiS", "SiS 530 AGP->PCI Bridge"},
	{0x1039,  0x0540, "SiS", "SiS 540 AGP->PCI Bridge"},
	{0x1039,  0x0550, "SiS", "SiS 550 AGP->PCI Bridge"},
	{0x1039,  0x0620, "SiS", "SiS 620 AGP->PCI Bridge"},
	{0x1039,  0x0630, "SiS", "SiS 630 AGP->PCI Bridge"},
	{0x1039,  0x0645, "SiS", "SiS 645 AGP->PCI Bridge"},
	{0x1039,  0x0646, "SiS", "SiS 645DX AGP->PCI Bridge"},
	{0x1039,  0x0648, "SiS", "SiS 648 AGP->PCI Bridge"},
	{0x1039,  0x0650, "SiS", "SiS 650 AGP->PCI Bridge"},
	{0x1039,  0x0651, "SiS", "SiS 651 AGP->PCI Bridge"},
	{0x1039,  0x0655, "SiS", "SiS 655 AGP->PCI Bridge"},
	{0x1039,  0x0661, "SiS", "SiS 661 AGP->PCI Bridge"},
	{0x1039,  0x0730, "SiS", "SiS 730 AGP->PCI Bridge"},
	{0x1039,  0x0735, "SiS", "SiS 735 AGP->PCI Bridge"},
	{0x1039,  0x0740, "SiS", "SiS 740 AGP->PCI Bridge"},
	{0x1039,  0x0741, "SiS", "SiS 741 AGP->PCI Bridge"},
	{0x1039,  0x0745, "SiS", "SiS 745 AGP->PCI Bridge"},
	{0x1039,  0x0746, "SiS", "SiS 746 AGP->PCI Bridge"},
};

static int sis_get_aperture(void)
{
	PCI_Entry_s *psDev = g_psBridge->psDev;
	AGP_Bridge_Sis_s *psSisBridge = (AGP_Bridge_Sis_s*)g_psBridge->pChipCtx;
	int nGws;

	/*
	 * The aperture size is equal to 4M<<nGws.
	 */
	kerndbg(KERN_DEBUG, "AGP: sis_get_aperture: Getting aperture\n");
	nGws = (g_psPCIBus->read_pci_config(psDev->nBus, psDev->nDevice,
							 	psDev->nFunction, AGP_SIS_WINCTRL, 2) & 0x70) >> 4;
							 	
	return (4*1024*1024) << nGws;
}

static int sis_set_aperture(uint32 nAperture)
{
	kerndbg(KERN_DEBUG, "AGP: sis_set_aperture: setting aperture to %u\n", nAperture);
	
	PCI_Entry_s *psDev = g_psBridge->psDev;
	AGP_Bridge_Sis_s *psSisBridge = (AGP_Bridge_Sis_s*)g_psBridge->pChipCtx;
	int nGws, nReg;

	/*
	 * Check for a power of two and make sure its within the
	 * programmable range.
	 */
	if (nAperture & (nAperture - 1)
	    || nAperture < 4*1024*1024
	    || nAperture > 256*1024*1024)
		return -EINVAL;

	nGws = ffs(nAperture / 4*1024*1024) - 1;
	
	nReg = g_psPCIBus->read_pci_config(psDev->nBus, psDev->nDevice,	psDev->nFunction, AGP_SIS_WINCTRL, 2);
	nReg &= ~0x00000070;
	nReg |= nGws << 4;
	g_psPCIBus->write_pci_config(psDev->nBus, psDev->nDevice, psDev->nFunction, AGP_SIS_WINCTRL, 2, nReg);

	return 0;
}

static int sis_bind_page(uint32 nOffset, uint32 nPhysical)
{
	AGP_Bridge_Sis_s *psSisBridge = (AGP_Bridge_Sis_s*)g_psBridge->pChipCtx;
	
	kerndbg(KERN_DEBUG, "AGP: sis_bind_page: binding offset: %#lx to physical address: %#lx\n",
				(unsigned long)nOffset, (unsigned long)nPhysical);
	
	if(nOffset < 0 || nOffset >= (psSisBridge->psGatt->nEntries << AGP_PAGE_SHIFT))
		return -EINVAL;

	psSisBridge->psGatt->pVirtual[nOffset >> AGP_PAGE_SHIFT] = nPhysical;
	return 0;
}

static int sis_unbind_page(uint32 nOffset)
{
	AGP_Bridge_Sis_s *psSisBridge = (AGP_Bridge_Sis_s*)g_psBridge->pChipCtx;
	
	kerndbg(KERN_DEBUG, "AGP: sis_unbind_page: unbinding offset: %#lx\n", (unsigned long)nOffset);
	
	if(nOffset < 0 || nOffset >= (psSisBridge->psGatt->nEntries << AGP_PAGE_SHIFT))
		return -EINVAL;

	psSisBridge->psGatt->pVirtual[nOffset >> AGP_PAGE_SHIFT] = 0;
	return 0;
}

static void sis_flush_tlb(void)
{
	kerndbg(KERN_DEBUG, "AGP: sis_flush_tlb: flushing translation look-aside buffer\n");
	
	PCI_Entry_s *psDev = g_psBridge->psDev;
	AGP_Bridge_Sis_s *psSisBridge = (AGP_Bridge_Sis_s*)g_psBridge->pChipCtx;
	
	g_psPCIBus->write_pci_config(psDev->nBus, psDev->nDevice, psDev->nFunction, AGP_SIS_TLBFLUSH, 2, 0x02);
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

status_t device_init(int nDeviceID)
{
	AGP_Bridge_Sis_s *psSisBridge;
	AGP_Gatt_s *psGatt;
	PCI_Entry_s *psDev;
	AGP_Node_s *psNode;
	PCI_Info_s sInfo;

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
				printk("Allocating memory for Sis Bridge structure\n");
				psSisBridge = kmalloc(sizeof(AGP_Bridge_Sis_s), MEMF_KERNEL | MEMF_CLEAR | MEMF_NOBLOCK);
				if(!psSisBridge) {
					printk("Error: Out of memory\n");
					kfree(g_psBridge);
					continue;
				}				
				psNode = kmalloc(sizeof(AGP_Node_s), MEMF_KERNEL | MEMF_CLEAR | MEMF_NOBLOCK);
				if(!psNode) {
					printk("Error: Out of memory\n");
					kfree(psSisBridge);
					kfree(g_psBridge);
					continue;
				}				
				printk("Allocating memory for PCI device entry structure\n");
				psDev = kmalloc(sizeof(PCI_Entry_s), MEMF_KERNEL | MEMF_CLEAR | MEMF_NOBLOCK);
				if(!psDev) {
					printk("Error: Out of memory\n");
					kfree(psNode);
					kfree(psSisBridge);
					kfree(g_psBridge);
					continue;
				}
				
				printk("Reading PCI Header Information\n");	
				g_psPCIBus->read_pci_header(psDev, psNode->sInfo.nBus, psNode->sInfo.nDevice, psNode->sInfo.nFunction);
								
				psDev->u.h0.nAGPStatus = nAGPReg + PCI_AGP_STATUS;
				psDev->u.h0.nAGPCommand = nAGPReg + PCI_AGP_COMMAND;
				
				memcpy(&psNode->sInfo, &sInfo, sizeof(PCI_Info_s));
				sprintf(psNode->zName, zTemp);
				
				g_psBridge->pChipCtx = psSisBridge;
				g_psBridge->nAGPReg = nAGPReg;
				g_psBridge->psDev = psDev;
				g_psBridge->psNode = psNode;
				g_psBridge->nDeviceID = nDeviceID;
				g_psBridge->psMethods = &g_sSis_Methods;
				g_psBridge->nMaxMem = 0;
				g_psBridge->nNextId = 0;
				
				g_psBridge->sVersion.nMajor = AGP_VERSION_MAJOR(nAGPRegVal);
				g_psBridge->sVersion.nMinor = AGP_VERSION_MINOR(nAGPRegVal);
				printk("AGP %u.%u is supported\n", g_psBridge->sVersion.nMajor, g_psBridge->sVersion.nMinor);
				
				/******      You must call attach_bridge() before attempting to use any other AGP busmanager function      ******/
				
				printk("Attaching %s to AGP busmanager\n", psNode->zName);
				if(g_psAGPBus->attach_bridge(g_psBridge) < 0) {
					printk("Failed to attach to AGP busmanager, shutdown & exit gracefully!\n");
					kfree(psDev);
					kfree(psNode);
					kfree(psSisBridge);
					kfree(g_psBridge);
					continue;
				}
				
				printk("Mapping aperture\n");
				if(g_psAGPBus->map_aperture(PCI_AGP_APBASE) != 0) {
					printk("Can't map aperture\n");
					kfree(psDev);
					kfree(psNode);
					kfree(psSisBridge);
					kfree(g_psBridge);
					continue;
				}
				
				printk("Getting initial aperture size\n");
				psSisBridge->nInitAp = g_psBridge->nApSize = AGP_GET_APERTURE(g_psBridge);
				
				for(;;) {
					printk("Allocating GATT\n");
					psGatt = g_psAGPBus->alloc_gatt();
					if(psGatt)
						break;
				
					/*
					 * Probably contigmalloc failure. Try reducing the
					 * aperture so that the gatt size reduces.
					 */
					printk("Failed to allocate GATT, shutdown & exit gracefully!\n");
					if(AGP_SET_APERTURE(g_psBridge, AGP_GET_APERTURE(g_psBridge) / 2)) {
						kfree(psDev);
						kfree(psNode);
						kfree(psSisBridge);
						kfree(g_psBridge);
						continue;
					}
				}
				
				/* Install the gatt. */
				uint32 nGartCtrl;
				
				printk("AGP -> Installing GATT\n");
				g_psPCIBus->write_pci_config(psDev->nBus, psDev->nDevice, psDev->nFunction,
											 AGP_SIS_ATTBASE, 4, psGatt->nPhysical);
				
				/* Enable the aperture. */
				printk("AGP -> Enabling aperture\n");
				nGartCtrl = g_psPCIBus->read_pci_config(psDev->nBus, psDev->nDevice, psDev->nFunction,
														AGP_SIS_WINCTRL, 4);
				g_psPCIBus->write_pci_config(psDev->nBus, psDev->nDevice, psDev->nFunction,
											 AGP_SIS_WINCTRL, 4, nGartCtrl |((0x05 << 24) | 3));
				
				printk("Aperture location is %p, aperture size is %dM\n", g_psBridge->nApBase, psSisBridge->nInitAp / (1024 * 1024));
				
				/*
				 * The mutex is used to prevent re-entry to
				 * agp_generic_bind_memory() since that function can sleep.
				 */
				printk("Creating mutex for AGP bridge\n");
				g_psBridge->nLock = CREATE_MUTEX("agp_bridge_mtx");
				
				DLIST_HEAD_INIT(&g_psBridge->sMemory);
				
				psSisBridge->psGatt = psGatt;
				
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

