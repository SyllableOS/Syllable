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


#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <atheos/types.h>
#include <atheos/pci.h>
#include <atheos/kernel.h>
#include <atheos/isa_io.h>

#include "ChipInfo.h"
#include "svga_reg.h"
#include "vmware.h"
#include "vmware_pci_ids.h"


/* Returns true/false if successfully calculated the
 * correct values for m_IndexPort and m_ValuePort */
bool VMware::setupIoPorts(void)
{
	if(m_vmChipInfo.nChipId == VMWARE_CHIP_SVGA)
	{
		/* PCI DeviceId 0x0710 */
		dbprintf("VMware - setupIoPorts() - Does'nt support this card yet\n");
		return false;
	}
	else if(m_vmChipInfo.nChipId == VMWARE_CHIP_SVGA2)
	{
		/* PCI DeviceId 0x0405 */
		m_IndexPort = (m_cPciInfo.u.h0.nBase0 & PCI_ADDRESS_IO_MASK)
						+ SVGA_INDEX_PORT;
		m_ValuePort = (m_cPciInfo.u.h0.nBase0 & PCI_ADDRESS_IO_MASK)
						+ SVGA_VALUE_PORT;
	}

	dbprintf("VMware - Index Port = 0x%04x Value Port = 0x%04x\n",
				m_IndexPort, m_ValuePort);

	return true;
}



/*** Read / Write to IO ports ***/
/********************************/
uint32 VMware::vmwareReadIndexPort(void)
{
	return inl(m_IndexPort);
}


void VMware::vmwareWriteIndexPort(uint32 newIndex)
{
	outl(newIndex, m_IndexPort);
}


uint32 VMware::vmwareReadValuePort(void)
{
	return inl(m_ValuePort);
}

void VMware::vmwareWriteValuePort(uint32 newValue)
{
	outl(newValue, m_ValuePort);
}
/********************************/



/*** Read / Write SVGA registers ***/
/***********************************/

/* Returns the highest SVGA Id supported by the video card */
uint32 VMware::vmwareGetSvgaId(void)
{
	uint32 svgaId;

	vmwareWriteReg(SVGA_REG_ID, SVGA_ID_2);
	svgaId = vmwareReadReg(SVGA_REG_ID);
	if(svgaId == (uint32)SVGA_ID_2)
		return SVGA_ID_2;

	vmwareWriteReg(SVGA_REG_ID, SVGA_ID_1);
	svgaId = vmwareReadReg(SVGA_REG_ID);
	if(svgaId == (uint32)SVGA_ID_1)
		return SVGA_ID_1;

	vmwareWriteReg(SVGA_REG_ID, SVGA_ID_0);
	svgaId = vmwareReadReg(SVGA_REG_ID);
	if(svgaId == (uint32)SVGA_ID_0)
		return SVGA_ID_0;

	/* No supported VMware SVGA devices found */
	return SVGA_ID_INVALID;
}


void VMware::vmwareWriteReg(int32 index, uint32 value)
{
	vmwareWriteIndexPort(index);
	vmwareWriteValuePort(value);
}


uint32 VMware::vmwareReadReg(int32 index)
{
	vmwareWriteIndexPort(index);
	return vmwareReadValuePort();
}
/***********************************/
