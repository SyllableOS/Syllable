/*
 *  The AtheOS kernel
 *  Copyright (C) 1999 - 2000 Kurt Skauen
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of version 2 of the GNU Library
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
 */

#ifndef __F_KERNEL_PCI_H_
#define __F_KERNEL_PCI_H_

#include <kernel/types.h>
#include <kernel/device.h>
#include <syllable/pci.h>
#include <syllable/pci_vendors.h>

typedef struct
{
	int		nBus;
	int		nDevice;
	int		nFunction;
	
	uint16	nVendorID;
	uint16	nDeviceID;
	uint16	nCommand;
	uint16	nStatus;
	uint8	nRevisionID;
	uint8 	nClassApi;
	uint8	nClassBase;
	uint8	nClassSub;
	uint8	nCacheLineSize;
	uint8	nLatencyTimer;
	uint8	nHeaderType;
	uint8	nSelfTestResult;
	uint32	nAGPMode;

	union
	{
		struct
		{
			uint32	nBase0;
			uint32	nBase1;
			uint32	nBase2;
			uint32	nBase3;
			uint32	nBase4;
			uint32	nBase5;
			uint32	nCISPointer;
			uint16	nSubSysVendorID;
			uint16	nSubSysID;
			uint32	nExpROMAddr;
			uint8	nCapabilityList;
			uint8	nInterruptLine;
			uint8	nInterruptPin;
			uint8	nMinDMATime;
			uint8	nMaxDMALatency;
			uint8	nAGPStatus;
			uint8	nAGPCommand;
		} h0;
	} u;
	int nHandle;
} PCI_Entry_s;

/* PCI bus */
#define PCI_BUS_NAME "pci"
#define PCI_BUS_VERSION 1

typedef struct
{
	status_t (*get_pci_info) ( PCI_Info_s* psInfo, int nIndex );
	uint32 (*read_pci_config)( int nBusNum, int nDevNum, int nFncNum, int nOffset, int nSize );
	status_t (*write_pci_config)( int nBusNum, int nDevNum, int nFncNum, int nOffset, 
									int nSize, uint32 nValue );
	void (*enable_pci_master)( int nBusNum, int nDevNum, int nFncNum );
	void (*set_pci_latency)( int nBusNum, int nDevNum, int nFncNum, uint8 nLatency );
	uint8 (*get_pci_capability)( int nBusNum, int nDevNum, int nFncNum, uint8 nCapID );	
	int (*read_pci_header)( PCI_Entry_s * psInfo, int nBusNum, int nDevNum, int nFncNum );
	int (*get_pci_device_type)( PCI_Info_s* psInfo );
	status_t (*get_bar_info)( PCI_Entry_s* psInfo, uintptr_t *pAddress, uintptr_t *pnSize, int nReg, int nType );
} PCI_bus_s;

#include <syllable/pci.h>

#endif /* __F_KERNEL_PCI_H_ */

