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
#include <atheos/areas.h>

#include "ChipInfo.h"
#include "svga_reg.h"
#include "vmware.h"
#include "vmware_pci_ids.h"
#include "guest_os.h"

bool VMware::CreateFb(void)
{
	if((m_hFbArea != -1) || (m_pFbBase != NULL))
	{
		dbprintf("VMware::CreateFb() - FB member vars are tainted.\n");
		dbprintf("\tm_hFbArea = %d m_pFbBase = %p\n",
					m_hFbArea, m_pFbBase);
		return false;
	}

	/* Read the starting address of the FB */
	m_regFbStart = vmwareReadReg(SVGA_REG_FB_START);
	// dbprintf("\tm_regFbStart is 0x%p\n", (void *)m_regFbStart);

	/* Read the size of the FB */
	m_regFbSize = vmwareReadReg(SVGA_REG_FB_SIZE);
	// dbprintf("\tm_regFbSize is %u\n", (unsigned int) m_regFbSize);

	/* Read the size of the VRAM, this is the size we use
	 * when calling create_area() */
	m_regVramSize = vmwareReadReg(SVGA_REG_VRAM_SIZE);
	// dbprintf("\tm_regVramSize is %u\n", (unsigned int) m_regVramSize);


	/*** Create an area for the FB ***/
	// dbprintf("\tTrying to create area of size m_regVramSize(%u)\n", (unsigned int) m_regVramSize);
	m_hFbArea = create_area("VMware_Fb", (void **) &m_pFbBase,
								m_regVramSize, AREA_FULL_ACCESS,
								AREA_NO_LOCK);

	if(m_hFbArea < 0)
	{
		dbprintf("VMware::CreateFb() - create_area() failed.\n");
		m_hFbArea = -1;
		return false;
	}


	/*** Remap the FB memory ***/
	// dbprintf("\tTrying to remap area to physical adress m_regFbStart(0x%p)\n", (void *) m_regFbStart);
	if(remap_area(m_hFbArea, (void *) m_regFbStart) < 0)
	{
		dbprintf("VMware::CreateFb() - Failed to remap FB area\n");
		dbprintf("VMware::CreateFb() - m_hFbArea (%d)\n", m_hFbArea);
		dbprintf("VMware::CreateFb() - Deleting m_hFbArea\n");

		delete_area(m_hFbArea);
		m_hFbArea = -1;
		m_pFbBase = NULL;
		return false;
	}

	/* m_pFbBase uses byte indexes */
	/* for(unsigned int i = 0; i < m_regVramSize; i++)
	{
		m_pFbBase[i] = 0x00;
	}
	*/

	// dbprintf("\tm_hFbArea = %d m_pFbBase = 0x%p\n", m_hFbArea, m_pFbBase);
	dbprintf("VMware - Total memory mapped for FB: %u\n", (unsigned int) m_regVramSize);
	dbprintf("VMware - FB physical address: 0x%p\n", (void *)m_regFbStart);
	dbprintf("VMware - FB virtual address:  0x%p\n", m_pFbBase);

	return true;
}


bool VMware::InitFb(void)
{
	/*** Get the device capabilities and frame buffer dimensions ***/
	m_regCapabilities = vmwareReadReg(SVGA_REG_CAPABILITIES);
	m_regMaxWidth = vmwareReadReg(SVGA_REG_MAX_WIDTH);
	m_regMaxHeight = vmwareReadReg(SVGA_REG_MAX_HEIGHT);

	m_regHostBPP = vmwareReadReg(SVGA_REG_HOST_BITS_PER_PIXEL);
	m_regBitsPerPixel = vmwareReadReg(SVGA_REG_BITS_PER_PIXEL);

	/*
	dbprintf("\tm_regCapabilities = 0x%08x\n", (unsigned int) m_regCapabilities);
	dbprintf("\tm_regMaxWidth = %u\n", (unsigned int) m_regMaxWidth);
	dbprintf("\tm_regMaxHeight = %u\n", (unsigned int) m_regMaxHeight);
	dbprintf("\tm_regBitsPerPixel = %u\n", (unsigned int) m_regBitsPerPixel);
	dbprintf("\tm_regHostBPP = %u\n", (unsigned int) m_regHostBPP);
	*/


	/*** Report the guest operating system ***/
	vmwareWriteReg(SVGA_REG_GUEST_ID, GUEST_OS_OTHER);
	m_regGuestId = vmwareReadReg(SVGA_REG_GUEST_ID);

	if(m_regGuestId !=  GUEST_OS_OTHER)
	{
		dbprintf("VMware::InitFb() - Could not set SVGA_GUEST_ID!!!!\n");
		return false;
	}

	/*** Set Mode ***/
	SetMode(800, 600, m_regHostBPP);

	/* Check that we read the values we just wrote */
	if((m_regWidth != 800) || (m_regHeight != 600) || (m_regBitsPerPixel != m_regHostBPP))
	{
		dbprintf("VMware::InitFb() - Could not set initial mode!!!\n");
		return false;
	}

	m_regFbOffset = vmwareReadReg(SVGA_REG_FB_OFFSET);
	m_regBytesPerLine = vmwareReadReg(SVGA_REG_BYTES_PER_LINE);
	m_regDepth = vmwareReadReg(SVGA_REG_DEPTH);
	m_regPseudoColor = vmwareReadReg(SVGA_REG_PSEUDOCOLOR);
	m_regRedMask = vmwareReadReg(SVGA_REG_RED_MASK);
	m_regGreenMask = vmwareReadReg(SVGA_REG_GREEN_MASK);
	m_regBlueMask = vmwareReadReg(SVGA_REG_BLUE_MASK);

	/*** Enable SVGA ***/
	vmwareWriteReg(SVGA_REG_ENABLE, 1);
	m_regEnable = vmwareReadReg(SVGA_REG_ENABLE);

	if(m_regEnable != 1)
	{
		dbprintf("VMware::InitFb() - Wrote 1 to SVGA_REG_ENABLE, but read nonzero!!!\n");
		return false;
	}

	return true;
}


bool VMware::SetMode(uint32 width, uint32 height, uint32 bpp)
{
	if((width > m_regMaxWidth) || (height > m_regMaxHeight))
	{
		dbprintf("VMware::SetMode() - width(%u) or height (%u) are to large.\n",
					(unsigned int) width, (unsigned int) height);
		dbprintf("\tMax-Width is %u Max-Height is %u\n",
					(unsigned int) m_regMaxWidth,
					(unsigned int) m_regMaxHeight);
		return false;
	}

	if((bpp != m_regHostBPP) && (bpp != 8))
	{
		dbprintf("VMware::SetMode() - bpp(%u) is not same as host bpp or not 8.\n",
					(unsigned int) bpp);
		dbprintf("\tHost bpp is %u\n", (unsigned int)m_regHostBPP);
		return false;
	}

	vmwareWriteReg(SVGA_REG_ENABLE, 0);
	vmwareWriteReg(SVGA_REG_WIDTH, width);
	vmwareWriteReg(SVGA_REG_HEIGHT, height);
	vmwareWriteReg(SVGA_REG_BITS_PER_PIXEL, bpp);
	vmwareWriteReg(SVGA_REG_GUEST_ID, GUEST_OS_OTHER);
	vmwareWriteReg(SVGA_REG_ENABLE, 1);

	m_regEnable = vmwareReadReg(SVGA_REG_ENABLE);
	m_regWidth = vmwareReadReg(SVGA_REG_WIDTH);
	m_regHeight = vmwareReadReg(SVGA_REG_HEIGHT);
	m_regBitsPerPixel = vmwareReadReg(SVGA_REG_BITS_PER_PIXEL);
	m_regDepth = vmwareReadReg(SVGA_REG_DEPTH);
	m_regFbOffset = vmwareReadReg(SVGA_REG_FB_OFFSET);
	m_regFbSize = vmwareReadReg(SVGA_REG_FB_SIZE);
	m_regBytesPerLine = vmwareReadReg(SVGA_REG_BYTES_PER_LINE);
	m_regDepth = vmwareReadReg(SVGA_REG_DEPTH);
	m_regPseudoColor = vmwareReadReg(SVGA_REG_PSEUDOCOLOR);
	m_regRedMask = vmwareReadReg(SVGA_REG_RED_MASK);
	m_regGreenMask = vmwareReadReg(SVGA_REG_GREEN_MASK);
	m_regBlueMask = vmwareReadReg(SVGA_REG_BLUE_MASK);

	if((m_regWidth != width) || (m_regHeight != height) || (m_regBitsPerPixel != bpp))
	{
		dbprintf("VMware::SetMode() - Mode change seems to have failed!!!\n");
		return false;
	}

	return true;
}
