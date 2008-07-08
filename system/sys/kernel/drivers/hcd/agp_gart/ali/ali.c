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
#include <macros.h>
#include "ali.h"


PCI_bus_s *g_psPCIBus;
AGP_bus_s *g_psAGPBus;
AGP_Bridge_s *g_psBridge;

static int ali_get_aperture(void);
static int ali_set_aperture(uint32 nAperture);
static int ali_bind_page(uint32 nOffset, uint32 nPhysical);
static int ali_unbind_page(uint32 nOffset);
static void ali_flush_tlb(void);

AGP_Methods_s g_sAli_Methods = {
	ali_get_aperture,
	ali_set_aperture,
	ali_bind_page,
	ali_unbind_page,
	ali_flush_tlb,
	generic_enable,
	generic_alloc_memory,
	generic_free_memory,
	generic_bind_memory,
	generic_unbind_memory
};

AGP_Device_s g_sDevices[] = {
	{0x10b9, 0x1671, "Ali", "Ali M1671 AGP->PCI Bridge"},
	{0x10b9, 0x1541, "Ali", "Ali M1541 AGP->PCI Bridge"},
	{0x10b9, 0x1621, "Ali", "Ali M1621 AGP->PCI Bridge"}
};

#define M 1024*1024

static uint32 aAliTable[] = {
	0,			/* 0 - invalid */
	1,			/* 1 - invalid */
	2,			/* 2 - invalid */
	4*M,			/* 3 - invalid */
	8*M,			/* 4 - invalid */
	0,			/* 5 - invalid */
	16*M,			/* 6 - invalid */
	32*M,			/* 7 - invalid */
	64*M,			/* 8 - invalid */
	128*M,			/* 9 - invalid */
	256*M,			/* 10 - invalid */
};
#define ALI_TABLE_SIZE (sizeof(aAliTable) / sizeof(aAliTable[0]))

static int ali_get_aperture(void)
{
	PCI_Entry_s *psDev = g_psBridge->psDev;
	AGP_Bridge_Ali_s *psAliBridge = (AGP_Bridge_Ali_s*)g_psBridge->pChipCtx;
	int nIndex;
	
	/*
	 * The aperture size is derived from the low bits of attbase.
	 * I'm not sure this is correct..
	 */
	
	kerndbg(KERN_DEBUG, "AGP: ali_get_aperture: Getting aperture\n");
	nIndex = (g_psPCIBus->read_pci_config(psDev->nBus, psDev->nDevice, psDev->nFunction,
			  AGP_ALI_ATTBASE, 4) & 0xf);
	
	if(nIndex >= ALI_TABLE_SIZE)
		return 0;
	return aAliTable[nIndex];
}

static int ali_set_aperture(uint32 nAperture)
{
	kerndbg(KERN_DEBUG, "AGP: ali_set_aperture: setting aperture to %u\n", nAperture);
	
	PCI_Entry_s *psDev = g_psBridge->psDev;
	AGP_Bridge_Ali_s *psAliBridge = (AGP_Bridge_Ali_s*)g_psBridge->pChipCtx;
	int nIndex;
	uint32 nAttBase;

	for (nIndex = 0; nIndex < ALI_TABLE_SIZE; nIndex++)
		if (aAliTable[nIndex] == nAperture)
			break;
	if (nIndex == ALI_TABLE_SIZE)
		return -EINVAL;

	nAperture = g_psPCIBus->read_pci_config(psDev->nBus, psDev->nDevice, psDev->nFunction, AGP_ALI_ATTBASE, 4);
	g_psPCIBus->write_pci_config(psDev->nBus, psDev->nDevice, psDev->nFunction,
								 AGP_ALI_ATTBASE, 4, (nAperture & ~0xf) | nIndex);	
	return 0;
}

static int ali_bind_page(uint32 nOffset, uint32 nPhysical)
{
	AGP_Bridge_Ali_s *psAliBridge = (AGP_Bridge_Ali_s*)g_psBridge->pChipCtx;
	
	kerndbg(KERN_DEBUG, "AGP: ali_bind_page: binding offset: %#lx to physical address: %#lx\n",
				(unsigned long)nOffset, (unsigned long)nPhysical);
	
	if(nOffset < 0 || nOffset >= (psAliBridge->psGatt->nEntries << AGP_PAGE_SHIFT))
		return -EINVAL;

	psAliBridge->psGatt->pVirtual[nOffset >> AGP_PAGE_SHIFT] = nPhysical;
	return 0;
}

static int ali_unbind_page(uint32 nOffset)
{
	AGP_Bridge_Ali_s *psAliBridge = (AGP_Bridge_Ali_s*)g_psBridge->pChipCtx;
	
	kerndbg(KERN_DEBUG, "AGP: ali_unbind_page: unbinding offset: %#lx\n", (unsigned long)nOffset);
	
	if(nOffset < 0 || nOffset >= (psAliBridge->psGatt->nEntries << AGP_PAGE_SHIFT))
		return -EINVAL;

	psAliBridge->psGatt->pVirtual[nOffset >> AGP_PAGE_SHIFT] = 0;
	return 0;
}

static void ali_flush_tlb(void)
{
	kerndbg(KERN_DEBUG, "AGP: ali_flush_tlb: flushing translation look-aside buffer\n");
	
	PCI_Entry_s *psDev = g_psBridge->psDev;
	AGP_Bridge_Ali_s *psAliBridge = (AGP_Bridge_Ali_s*)g_psBridge->pChipCtx;
	
	g_psPCIBus->write_pci_config(psDev->nBus, psDev->nDevice, psDev->nFunction, AGP_ALI_TLBCTRL, 2, 0x90);
	g_psPCIBus->write_pci_config(psDev->nBus, psDev->nDevice, psDev->nFunction, AGP_ALI_TLBCTRL, 2, 0x10);
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

status_t device_init(int nDeviceID)
{
	AGP_Bridge_Ali_s *psAliBridge;
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
				printk("Allocating memory for Ali Bridge structure\n");
				psAliBridge = kmalloc(sizeof(AGP_Bridge_Ali_s), MEMF_KERNEL | MEMF_CLEAR | MEMF_NOBLOCK);
				if(!psAliBridge) {
					printk("Error: Out of memory\n");
					kfree(g_psBridge);
					continue;
				}				
				psNode = kmalloc(sizeof(AGP_Node_s), MEMF_KERNEL | MEMF_CLEAR | MEMF_NOBLOCK);
				if(!psNode) {
					printk("Error: Out of memory\n");
					kfree(psAliBridge);
					kfree(g_psBridge);
					continue;
				}				
				printk("Allocating memory for PCI device entry structure\n");
				psDev = kmalloc(sizeof(PCI_Entry_s), MEMF_KERNEL | MEMF_CLEAR | MEMF_NOBLOCK);
				if(!psDev) {
					printk("Error: Out of memory\n");
					kfree(psNode);
					kfree(psAliBridge);
					kfree(g_psBridge);
					continue;
				}
				
				printk("Reading PCI Header Information\n");	
				g_psPCIBus->read_pci_header(psDev, psNode->sInfo.nBus, psNode->sInfo.nDevice, psNode->sInfo.nFunction);
								
				psDev->u.h0.nAGPStatus = nAGPReg + PCI_AGP_STATUS;
				psDev->u.h0.nAGPCommand = nAGPReg + PCI_AGP_COMMAND;
				
				memcpy(&psNode->sInfo, &sInfo, sizeof(PCI_Info_s));
				sprintf(psNode->zName, zTemp);
				
				g_psBridge->pChipCtx = psAliBridge;
				g_psBridge->nAGPReg = nAGPReg;
				g_psBridge->psDev = psDev;
				g_psBridge->psNode = psNode;
				g_psBridge->nDeviceID = nDeviceID;
				g_psBridge->psMethods = &g_sAli_Methods;
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
					kfree(psAliBridge);
					kfree(g_psBridge);
					continue;
				}
				
				printk("Mapping aperture\n");
				if(g_psAGPBus->map_aperture(PCI_AGP_APBASE) != 0) {
					printk("Can't map aperture\n");
					kfree(psDev);
					kfree(psNode);
					kfree(psAliBridge);
					kfree(g_psBridge);
					continue;
				}
				
				printk("Getting initial aperture size\n");
				psAliBridge->nInitAp = g_psBridge->nApSize = AGP_GET_APERTURE(g_psBridge);
				
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
						kfree(psAliBridge);
						kfree(g_psBridge);
						continue;
					}
				}
				
				/* Install the gatt. */
				uint32 nGartCtrl;
				
				printk("AGP -> Installing GATT\n");
				nGartCtrl = g_psPCIBus->read_pci_config(psDev->nBus, psDev->nDevice, psDev->nFunction,
														AGP_ALI_ATTBASE, 4);
				g_psPCIBus->write_pci_config(psDev->nBus, psDev->nDevice, psDev->nFunction, AGP_ALI_ATTBASE,
											 4, psGatt->nPhysical | (nGartCtrl & 0xfff));
				
				/* Enable the aperture. */
				printk("AGP -> Enabling aperture\n");
				g_psPCIBus->write_pci_config(psDev->nBus, psDev->nDevice, psDev->nFunction,
											 AGP_ALI_TLBCTRL, 2, 0x10);
				
				printk("Aperture location is %p, aperture size is %dM\n", g_psBridge->nApBase, psAliBridge->nInitAp / (1024 * 1024));
				
				/*
				 * The mutex is used to prevent re-entry to
				 * agp_generic_bind_memory() since that function can sleep.
				 */
				printk("Creating mutex for AGP bridge\n");
				g_psBridge->nLock = CREATE_MUTEX("agp_bridge_mtx");
				
				DLIST_HEAD_INIT(&g_psBridge->sMemory);
				
				psAliBridge->psGatt = psGatt;
				
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

