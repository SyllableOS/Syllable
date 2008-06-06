
/*
 *  The Syllable kernel
 *  PCI busmanager
 *  Copyright (C) 2003 Arno Klenke
 *  Copyright (C) 1999 - 2001 Kurt Skauen
 *  
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

#ifndef _PCI_INTERNAL_H_
#define _PCI_INTERNAL_H_

#include <atheos/pci.h>

typedef struct
{
	int nPCIDeviceNumber;
	int nPrimaryBus;
	int nSecondaryBus;
} PCI_Bus_s;

#define MAX_PCI_BUSSES	16
#define	MAX_PCI_DEVICES	255

enum
{
	PCI_METHOD_1 = 0x01,
	PCI_METHOD_2 = 0x02,
	PCI_METHOD_BIOS = 0x04
};

extern uint32 g_nPCIMethod;
extern int g_nPCINumBusses;
extern int g_nPCINumDevices;
extern PCI_Bus_s *g_apsPCIBus[MAX_PCI_BUSSES];
extern PCI_Entry_s *g_apsPCIDevice[MAX_PCI_DEVICES];

extern void init_acpi_pci_links( void );
extern void init_acpi_pci_router( void );
extern void init_pci_irq_routing( void );
extern int lookup_irq( PCI_Entry_s* psDevice, bool bAssign );
extern uint32 read_pci_config( int nBusNum, int nDevNum, int nFncNum, int nOffset, int nSize );
extern status_t write_pci_config( int nBusNum, int nDevNum, int nFncNum, int nOffset, int nSize, uint32 nValue );

#endif













