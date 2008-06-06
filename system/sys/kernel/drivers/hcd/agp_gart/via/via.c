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
 * Copyright(c) 2000 Doug Rabson
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
 * DAMAGES(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
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
#include "via.h"

status_t via_setup(uint16 nDeviceID);
status_t via_teardown(void);

PCI_bus_s *g_psPCIBus;
AGP_bus_s *g_psAGPBus;
AGP_Bridge_s *g_psBridge;

static int aVia_V2_Regs[] = { AGP_VIA_GARTCTRL, AGP_VIA_APSIZE, AGP_VIA_ATTBASE };
static int aVia_V3_Regs[] = { AGP3_VIA_GARTCTRL, AGP3_VIA_APSIZE, AGP3_VIA_ATTBASE };

static int via_get_aperture(void);
static int via_set_aperture(uint32 nAperture);
static int via_bind_page(uint32 nOffset, uint32 nPhysical);
static int via_unbind_page(uint32 nOffset);
static void via_flush_tlb(void);

AGP_Methods_s g_sVia_Methods = {
	via_get_aperture,
	via_set_aperture,
	via_bind_page,
	via_unbind_page,
	via_flush_tlb,
	generic_enable,
	generic_alloc_memory,
	generic_free_memory,
	generic_bind_memory,
	generic_unbind_memory
};

AGP_Device_s g_sDevices[] = {
	{0x1106, 0x0198, "VIA", "8763(P4X600) AGP->PCI Bridge"},
	{0x1106, 0x0259, "VIA", "PM800/PN800/PM880/PN880 AGP->PCI Bridge"},
	{0x1106, 0x0269, "VIA", "KT880 AGP->PCI Bridge"},
	{0x1106, 0x0296, "VIA", "3296(P4M800) AGP->PCI Bridge"},
	{0x1106, 0x0305, "VIA", "82C8363(Apollo KT133x/KM133) AGP->PCI Bridge"},
	{0x1106, 0x0324, "VIA", "VT3324(CX700) AGP->PCI Bridge"},
	{0x1106, 0x0391, "VIA", "8371(Apollo KX133) AGP->PCI Bridge"},
	{0x1106, 0x0501, "VIA", "8501(Apollo MVP4) AGP->PCI Bridge"},
	{0x1106, 0x0597, "VIA", "82C597(Apollo VP3) AGP->PCI Bridge"},
	{0x1106, 0x0598, "VIA", "82C598(Apollo MVP3) AGP->PCI Bridge"},
	{0x1106, 0x0601, "VIA", "8601(Apollo ProMedia/PLE133Ta) AGP->PCI Bridge"},
	{0x1106, 0x0605, "VIA", "82C694X(Apollo Pro 133A) AGP->PCI Bridge"},
	{0x1106, 0x0691, "VIA", "82C691(Apollo Pro) AGP->PCI Bridge"},
	{0x1106, 0x3091, "VIA", "8633(Pro 266) AGP->PCI Bridge"},
	{0x1106, 0x3099, "VIA", "8367(KT266/KY266x/KT333) AGP->PCI Bridge"},
	{0x1106, 0x3101, "VIA", "8653(Pro266T) AGP->PCI Bridge"},
	{0x1106, 0x3112, "VIA", "8361(KLE133) AGP->PCI Bridge"},
	{0x1106, 0x3116, "VIA", "XM266(PM266/KM266) AGP->PCI Bridge"},
	{0x1106, 0x3123, "VIA", "862x(CLE266) AGP->PCI Bridge"},
	{0x1106, 0x3128, "VIA", "8753(P4X266) AGP->PCI Bridge"},
	{0x1106, 0x3148, "VIA", "8703(P4M266x/P4N266) AGP->PCI Bridge"},
	{0x1106, 0x3156, "VIA", "XN266(Apollo Pro266) AGP->PCI Bridge"},
	{0x1106, 0x3168, "VIA", "8754(PT800) AGP->PCI Bridge"},
	{0x1106, 0x3189, "VIA", "8377(Apollo KT400/KT400A/KT600) AGP->PCI Bridge"},
	{0x1106, 0x3205, "VIA", "8235/8237(Apollo KM400/KM400A) AGP->PCI Bridge"},
	{0x1106, 0x3208, "VIA", "8783(PT890) AGP->PCI Bridge"},
	{0x1106, 0x3258, "VIA", "PT880 AGP->PCI Bridge"},
	{0x1106, 0xb198, "VIA", "VT83xx/VT87xx/KTxxx/Px8xx AGP->PCI Bridge"},
};

static int via_get_aperture(void)
{
	PCI_Entry_s *psDev = g_psBridge->psDev;
	AGP_Bridge_Via_s *psViaBridge = (AGP_Bridge_Via_s*)g_psBridge->pChipCtx;
	uint32 nApSize;

	if(psViaBridge->pRegs == aVia_V2_Regs) {
		kerndbg(KERN_DEBUG, "AGP: via_get_aperture: Get aperture using agp v2 method\n");
		nApSize = (g_psPCIBus->read_pci_config(psDev->nBus, psDev->nDevice,
							 	psDev->nFunction, psViaBridge->pRegs[REG_APSIZE], 2) & 0x1f);

		/*
		 * The size is determined by the number of low bits of
		 * register APBASE which are forced to zero. The low 20 bits
		 * are always forced to zero and each zero bit in the nApSize
		 * field just read forces the corresponding bit in the 27:20
		 * to be zero. We calculate the aperture size accordingly.
		 */
		return(((nApSize ^ 0xff) << 20) |((1 << 20) - 1)) + 1;
	} else {
		kerndbg(KERN_DEBUG, "AGP: via_get_aperture: Get aperture using agp v3 method\n");
		nApSize = (g_psPCIBus->read_pci_config(psDev->nBus, psDev->nDevice,
							 	psDev->nFunction, psViaBridge->pRegs[REG_APSIZE], 4) & 0xfff);
		switch(nApSize) {
			case 0x800:
				kerndbg(KERN_DEBUG, "AGP: via_get_aperture: Aperture size is 2GB\n");
				return 0x80000000;
			case 0xc00:
				kerndbg(KERN_DEBUG, "AGP: via_get_aperture: Aperture size is 1GB\n");
				return 0x40000000;
			case 0xe00:
				kerndbg(KERN_DEBUG, "AGP: via_get_aperture: Aperture size is 512MB\n");
				return 0x20000000;
			case 0xf00:
				kerndbg(KERN_DEBUG, "AGP: via_get_aperture: Aperture size is 256MB\n");
				return 0x10000000;
			case 0xf20:
				kerndbg(KERN_DEBUG, "AGP: via_get_aperture: Aperture size is 128MB\n");
				return 0x08000000;
			case 0xf30:
				kerndbg(KERN_DEBUG, "AGP: via_get_aperture: Aperture size is 64MB\n");
				return 0x04000000;
			case 0xf38:
				kerndbg(KERN_DEBUG, "AGP: via_get_aperture: Aperture size is 32MB\n");
				return 0x02000000;
			default:
				kerndbg(KERN_WARNING, "AGP: via_get_aperture: Invalid aperture setting 0x%x",
				    g_psPCIBus->read_pci_config(psDev->nBus, psDev->nDevice,
				    				 psDev->nFunction, psViaBridge->pRegs[REG_APSIZE], 4));
				return 0;
		}
	}
}

static int via_set_aperture(uint32 nAperture)
{
	kerndbg(KERN_DEBUG, "AGP: via_set_aperture: setting aperture to %u\n", nAperture);
	
	PCI_Entry_s *psDev = g_psBridge->psDev;
	AGP_Bridge_Via_s *psViaBridge = (AGP_Bridge_Via_s*)g_psBridge->pChipCtx;
	uint32 nApSize, nKey, nValue;

	if(psViaBridge->pRegs == aVia_V2_Regs) {
		kerndbg(KERN_DEBUG, "AGP: via_set_aperture: using AGP v2 registers\n");
		
		/*
		 * Reverse the magic from get_aperture.
		 */
		nApSize =((nAperture - 1) >> 20) ^ 0xff;

		/*
	 	 * Double check for sanity.
	 	 */
		if((((nApSize ^ 0xff) << 20) |((1 << 20) - 1)) + 1 != nAperture)
			return -EINVAL;

		g_psPCIBus->write_pci_config(psDev->nBus, psDev->nDevice, psDev->nFunction,
									 psViaBridge->pRegs[REG_APSIZE], 2, nApSize);
	} else {
		switch(nAperture) {
			case 0x80000000:
				{
					nKey = 0x800;
					kerndbg(KERN_DEBUG, "AGP: via_set_aperture: setting key to 0x%x\n", nKey);
				} break;
			case 0x40000000:
				{
					nKey = 0xc00;
					kerndbg(KERN_DEBUG, "AGP: via_set_aperture: setting key to 0x%x\n", nKey);
				} break;
			case 0x20000000:
				{
					nKey = 0xe00;
					kerndbg(KERN_DEBUG, "AGP: via_set_aperture: setting key to 0x%x\n", nKey);
				} break;
			case 0x10000000:
				{
					nKey = 0xf00;
					kerndbg(KERN_DEBUG, "AGP: via_set_aperture: setting key to 0x%x\n", nKey);
				} break;
			case 0x08000000:
				{
					nKey = 0xf20;
					kerndbg(KERN_DEBUG, "AGP: via_set_aperture: setting key to 0x%x\n", nKey);
				} break;
			case 0x04000000:
				{
					nKey = 0xf30;
					kerndbg(KERN_DEBUG, "AGP: via_set_aperture: setting key to 0x%x\n", nKey);
				} break;
			case 0x02000000:
				{
					nKey = 0xf38;
					kerndbg(KERN_DEBUG, "AGP: via_set_aperture: setting key to 0x%x\n", nKey);
				} break;
			default:
				{
					kerndbg(KERN_WARNING, "AGP: via_set_aperture: Invalid aperture size(%dMb)\n",
			 			    nAperture / 1024 / 1024);
					return -EINVAL;
				};
		}
		nValue = g_psPCIBus->read_pci_config(psDev->nBus, psDev->nDevice,
			    				 psDev->nFunction, psViaBridge->pRegs[REG_APSIZE], 4);
		g_psPCIBus->write_pci_config(psDev->nBus, psDev->nDevice, psDev->nFunction,
			    		  psViaBridge->pRegs[REG_APSIZE], 4, ((nValue & ~0xfff) | nKey));
	}
	return 0;
}

static int via_bind_page(uint32 nOffset, uint32 nPhysical)
{
	AGP_Bridge_Via_s *psViaBridge = (AGP_Bridge_Via_s*)g_psBridge->pChipCtx;
	
	kerndbg(KERN_DEBUG, "AGP: via_bind_page: binding offset: %#lx to physical address: %#lx\n",
				(unsigned long)nOffset, (unsigned long)nPhysical);
	
	if(nOffset < 0 || nOffset >= (psViaBridge->psGatt->nEntries << AGP_PAGE_SHIFT))
		return -EINVAL;

	psViaBridge->psGatt->pVirtual[nOffset >> AGP_PAGE_SHIFT] = nPhysical;
	return 0;
}

static int via_unbind_page(uint32 nOffset)
{
	AGP_Bridge_Via_s *psViaBridge = (AGP_Bridge_Via_s*)g_psBridge->pChipCtx;
	
	kerndbg(KERN_DEBUG, "AGP: via_unbind_page: unbinding offset: %#lx\n", (unsigned long)nOffset);
	
	if(nOffset < 0 || nOffset >= (psViaBridge->psGatt->nEntries << AGP_PAGE_SHIFT))
		return -EINVAL;

	psViaBridge->psGatt->pVirtual[nOffset >> AGP_PAGE_SHIFT] = 0;
	return 0;
}

static void via_flush_tlb(void)
{
	kerndbg(KERN_DEBUG, "AGP: via_flush_tlb: flushing translation look-aside buffer\n");
	
	PCI_Entry_s *psDev = g_psBridge->psDev;
	AGP_Bridge_Via_s *psViaBridge = (AGP_Bridge_Via_s*)g_psBridge->pChipCtx;
	
	uint32 nGartCtrl;

	if(psViaBridge->pRegs == aVia_V2_Regs) {
		g_psPCIBus->write_pci_config(psDev->nBus, psDev->nDevice, psDev->nFunction,
									 psViaBridge->pRegs[REG_GARTCTRL], 4, 0x8f);
		g_psPCIBus->write_pci_config(psDev->nBus, psDev->nDevice, psDev->nFunction,
									 psViaBridge->pRegs[REG_GARTCTRL], 4, 0x0f);
	} else {
		nGartCtrl = g_psPCIBus->read_pci_config(psDev->nBus, psDev->nDevice,
								 psDev->nFunction, psViaBridge->pRegs[REG_GARTCTRL], 4);
		g_psPCIBus->write_pci_config(psDev->nBus, psDev->nDevice, psDev->nFunction,
								psViaBridge->pRegs[REG_GARTCTRL], 4, nGartCtrl & ~(1 << 7));
		g_psPCIBus->write_pci_config(psDev->nBus, psDev->nDevice, psDev->nFunction,
									 psViaBridge->pRegs[REG_GARTCTRL], 4, nGartCtrl);
	}
	kerndbg(KERN_DEBUG, "AGP: via_flush_tlb: finished\n");
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

status_t device_init(int nDeviceID)
{
	AGP_Bridge_Via_s *psViaBridge;
	AGP_Gatt_s *psGatt;
	PCI_Entry_s *psDev;
	AGP_Node_s *psNode;
	PCI_Info_s sInfo;

	int nError;
	uint32 nAgpSel, nAGPReg, nAGPRegVal;
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
				printk("Allocating memory for Via Bridge structure\n");
				psViaBridge = kmalloc(sizeof(AGP_Bridge_Via_s), MEMF_KERNEL | MEMF_CLEAR | MEMF_NOBLOCK);
				if(!psViaBridge) {
					printk("Error: Out of memory\n");
					kfree(g_psBridge);
					continue;
				}				
				psNode = kmalloc(sizeof(AGP_Node_s), MEMF_KERNEL | MEMF_CLEAR | MEMF_NOBLOCK);
				if(!psNode) {
					printk("Error: Out of memory\n");
					kfree(psViaBridge);
					kfree(g_psBridge);
					continue;
				}				
				printk("Allocating memory for PCI device entry structure\n");
				psDev = kmalloc(sizeof(PCI_Entry_s), MEMF_KERNEL | MEMF_CLEAR | MEMF_NOBLOCK);
				if(!psDev) {
					printk("Error: Out of memory\n");
					kfree(psNode);
					kfree(psViaBridge);
					kfree(g_psBridge);
					continue;
				}
				
				printk("Reading PCI Header Information\n");	
				g_psPCIBus->read_pci_header(psDev, psNode->sInfo.nBus, psNode->sInfo.nDevice, psNode->sInfo.nFunction);
								
				psDev->u.h0.nAGPStatus = nAGPReg + PCI_AGP_STATUS;
				psDev->u.h0.nAGPCommand = nAGPReg + PCI_AGP_COMMAND;
				
				memcpy(&psNode->sInfo, &sInfo, sizeof(PCI_Info_s));
				sprintf(psNode->zName, zTemp);
				
				g_psBridge->pChipCtx = psViaBridge;
				g_psBridge->nAGPReg = nAGPReg;
				g_psBridge->psDev = psDev;
				g_psBridge->psNode = psNode;
				g_psBridge->nDeviceID = nDeviceID;
				g_psBridge->psMethods = &g_sVia_Methods;
				g_psBridge->nMaxMem = 0;
				g_psBridge->nNextId = 0;
				
				g_psBridge->sVersion.nMajor = AGP_VERSION_MAJOR(nAGPRegVal);
				g_psBridge->sVersion.nMinor = AGP_VERSION_MINOR(nAGPRegVal);
				printk("AGP %u.%u is supported\n", g_psBridge->sVersion.nMajor, g_psBridge->sVersion.nMinor);
				
				if(g_psBridge->sVersion.nMajor >= 3) {
					nAgpSel = g_psPCIBus->read_pci_config(psDev->nBus, psDev->nDevice, psDev->nFunction,
														  AGP_VIA_AGPSEL, 1);
					if((nAgpSel & (1 << 1)) == 0) {
						psViaBridge->pRegs = aVia_V3_Regs;
						printk("AGP version 3 registers used\n");
					} else {
						psViaBridge->pRegs = aVia_V2_Regs;
						printk("AGP version 2 registers used: compatibility mode\n");
					}	
				} else {
					psViaBridge->pRegs = aVia_V2_Regs;
					printk("AGP version 2 registers used\n");
				}
				
				/******      You must call attach_bridge() before attempting to use any other AGP busmanager function      ******/
				
				printk("Attaching %s to AGP busmanager\n", psNode->zName);
				if(g_psAGPBus->attach_bridge(g_psBridge) < 0) {
					printk("Failed to attach to AGP busmanager, shutdown & exit gracefully!\n");
					kfree(psDev);
					kfree(psNode);
					kfree(psViaBridge);
					kfree(g_psBridge);
					continue;
				}
				
				printk("Mapping aperture\n");
				if(g_psAGPBus->map_aperture(PCI_AGP_APBASE) != 0) {
					printk("Can't map aperture\n");
					kfree(psDev);
					kfree(psNode);
					kfree(psViaBridge);
					kfree(g_psBridge);
					continue;
				}
				
				printk("Getting initial aperture size\n");
				psViaBridge->nInitAp = g_psBridge->nApSize = AGP_GET_APERTURE(g_psBridge);
				
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
						kfree(psViaBridge);
						kfree(g_psBridge);
						continue;
					}
				}
				
				if(psViaBridge->pRegs == aVia_V2_Regs) {
					/* Install the gatt. */
					printk("AGP version 2 -> Installing GATT\n");
					g_psPCIBus->write_pci_config(psDev->nBus, psDev->nDevice, psDev->nFunction,
												 psViaBridge->pRegs[REG_ATTBASE], 4, psGatt->nPhysical | 3);
					
					printk("AGP version 2 -> Enabling aperture\n");
					/* Enable the aperture. */
					g_psPCIBus->write_pci_config(psDev->nBus, psDev->nDevice, psDev->nFunction,
												 psViaBridge->pRegs[REG_GARTCTRL], 4, 0x0000000f);
				} else {
					uint32 nGartCtrl;
					
					/* Install the gatt. */
					printk("AGP version 3 -> Installing GATT\n");
					g_psPCIBus->write_pci_config(psDev->nBus, psDev->nDevice, psDev->nFunction,
												 psViaBridge->pRegs[REG_ATTBASE], 4, psGatt->nPhysical);
					
					/* Enable the aperture. */
					printk("AGP version 3 -> Enabling aperture\n");
					nGartCtrl = g_psPCIBus->read_pci_config(psDev->nBus, psDev->nDevice, psDev->nFunction,
															psViaBridge->pRegs[REG_ATTBASE], 4);
					g_psPCIBus->write_pci_config(psDev->nBus, psDev->nDevice, psDev->nFunction,
												 psViaBridge->pRegs[REG_GARTCTRL], 4, nGartCtrl |(3 << 7));
				}
				
				printk("Aperture location is %p, aperture size is %dM\n", g_psBridge->nApBase, psViaBridge->nInitAp / (1024 * 1024));
				
				/*
				 * The mutex is used to prevent re-entry to
				 * agp_generic_bind_memory() since that function can sleep.
				 */
				printk("Creating mutex for AGP bridge\n");
				g_psBridge->nLock = CREATE_MUTEX("agp_bridge_mtx");
				
				DLIST_HEAD_INIT(&g_psBridge->sMemory);
				
				psViaBridge->psGatt = psGatt;
				
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

























