/*
 *  Syllable Driver for VMware virtual video card
 *
 *  Based on the the xfree86 VMware driver.
 *
 *  Syllable specific code is
 *  Copyright (C) 2004, James Hayhurst (james_hayhurst@hotmail.com)
 *
 *  Other code is
 *  Copyright (C) 1998-2001 VMware, Inc.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */


#ifndef __F_VMWARE_REGS_H__
#define __F_VMWARE_REGS_H__

#include "vmware_pci_ids.h"

typedef struct ChipInfo
{
	uint32 nChipId; /* PCI Vendor ID and PCI Device ID */
	uint16 nArchRev;
	char *pzName;
} ChipInfo;

#define VMWARE_CHIP_SVGA	((PCI_VENDOR_ID_VMWARE << 16) | PCI_DEVICE_ID_VMWARE_SVGA)
#define VMWARE_CHIP_SVGA2	((PCI_VENDOR_ID_VMWARE << 16) | PCI_DEVICE_ID_VMWARE_SVGA2)


#endif
