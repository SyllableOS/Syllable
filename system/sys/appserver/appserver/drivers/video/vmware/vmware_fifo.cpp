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

#include "ChipInfo.h"
#include "svga_reg.h"
#include "vmware.h"
#include "vmware_pci_ids.h"

bool VMware::CreateFifo(void)
{
	if((m_hFifoArea != -1) || (m_pFifoBase != NULL))
	{
		dbprintf("VMware::CreateFifo() - FIFO variables are not safe values.\n");
		dbprintf("\tm_hFifoArea = %d m_pFifoBase = %p\n",
					m_hFifoArea, m_pFifoBase);
		return false;
	}

	/* Read the starting address of the FIFO */
	m_regMemStart = vmwareReadReg(SVGA_REG_MEM_START);
	// dbprintf("\tm_regMemStart is 0x%p\n", (void *) m_regMemStart);

	/* Read the size of the FIFO */
	m_regMemSize = vmwareReadReg(SVGA_REG_MEM_SIZE);
	// dbprintf("\tm_regMemSize is %u\n",	(unsigned int) m_regMemSize);


	/*** Create an area for the FIFO ***/
	// dbprintf("Trying to create area of size m_regMemSize(%u)\n", (unsigned int) m_regMemSize);
	m_hFifoArea =	create_area("VMware_fifo", (void **) &m_pFifoBase,
									m_regMemSize, AREA_FULL_ACCESS,
									AREA_NO_LOCK);

	if(m_hFifoArea < 0)
	{
		dbprintf("VMware::CreateFifo() - create_area() failed.\n");
		m_hFifoArea = -1;
		return false;
	}


	/*** Remap the FIFO memory ***/
	// dbprintf("Trying to remap area to physical adress m_regMemStart(0x%p)\n", (void *) m_regMemStart);
	if(remap_area(m_hFifoArea, (void *)m_regMemStart) < 0)
	{
		dbprintf("VMware - failed to remap FIFO area\n");
		dbprintf("VMware - m_hFifoArea (%d)\n", m_hFifoArea);
		dbprintf("VMware - deleting m_hFifoArea\n");

		delete_area(m_hFifoArea);
		m_hFifoArea = -1;
		m_pFifoBase = NULL;
		return false;
	}

	dbprintf("VMware - Total mapped memory for FIFO: %u\n", (unsigned int) m_regMemSize);
	dbprintf("VMware - FIFO physical address:  0x%p\n", (void *) m_regMemStart);
	dbprintf("VMware - FIFO virtual address:   0x%p\n", (void *) m_pFifoBase);

	return true;
}


bool VMware::InitFifo(void)
{
	if((m_hFifoArea == -1) || (m_pFifoBase == NULL))
	{
		dbprintf("VMware::InitFifo() - FIFO has not been setup.\n");
		dbprintf("\tm_hFifoArea = %d m_pFifoBase = %p\n", m_hFifoArea, m_pFifoBase);
		return false;
	}

	/*
	A minimum sized FIFO would have these values:
		mem[SVGA_FIFO_MIN] = 16;
		mem[SVGA_FIFO_MAX] = 16 + (10 * 1024);
		mem[SVGA_FIFO_NEXT_CMD] = 16;
		mem[SVGA_FIFO_STOP] = 16;

	Set SVGA_REG_CONFIG_DONE to 1 after these values have been set.
	*/

	m_pFifoBase[SVGA_FIFO_MIN] = 4 * sizeof(uint32);
	m_pFifoBase[SVGA_FIFO_MAX] = m_regMemSize & ~3;
	m_pFifoBase[SVGA_FIFO_NEXT_CMD] = 4 * sizeof(uint32);
	m_pFifoBase[SVGA_FIFO_STOP] = 4 * sizeof(uint32);
	vmwareWriteReg(SVGA_REG_CONFIG_DONE, 1);
	m_regConfigDone = vmwareReadReg(SVGA_REG_CONFIG_DONE);

	return true;
}


void VMware::FifoSync(void)
{
	vmwareWriteReg(SVGA_REG_SYNC, 1);
	while(vmwareReadReg(SVGA_REG_BUSY));
	m_bFifoCmds = false;
}


void VMware::Fifo_WriteWord(uint32 value)
{
	/* Need to sync? */
	if ((m_pFifoBase[SVGA_FIFO_NEXT_CMD] + sizeof(uint32) == m_pFifoBase[SVGA_FIFO_STOP])
		|| (m_pFifoBase[SVGA_FIFO_NEXT_CMD] == m_pFifoBase[SVGA_FIFO_MAX] - sizeof(uint32) &&
			m_pFifoBase[SVGA_FIFO_STOP] == m_pFifoBase[SVGA_FIFO_MIN]))
	{
		FifoSync();
	}

	m_pFifoBase[m_pFifoBase[SVGA_FIFO_NEXT_CMD] / sizeof(uint32)] = value;
	if(m_pFifoBase[SVGA_FIFO_NEXT_CMD] == m_pFifoBase[SVGA_FIFO_MAX] - sizeof(uint32))
	{
		m_pFifoBase[SVGA_FIFO_NEXT_CMD] = m_pFifoBase[SVGA_FIFO_MIN];
	}
	else
	{
		m_pFifoBase[SVGA_FIFO_NEXT_CMD] += sizeof(uint32);
	}
}


void VMware::Fifo_UpdateRect(uint32 dstX, uint32 dstY, uint32 width,
								uint32 height)
{
	Fifo_WriteWord(SVGA_CMD_UPDATE);
	Fifo_WriteWord(dstX);
	Fifo_WriteWord(dstY);
	Fifo_WriteWord(width);
	Fifo_WriteWord(height);
}


void VMware::Fifo_RectFill(uint32 dstX, uint32 dstY, uint32 width, uint32 height,
							uint32 color)
{
	Fifo_WriteWord(SVGA_CMD_RECT_FILL);
	Fifo_WriteWord(color);
	Fifo_WriteWord(dstX);
	Fifo_WriteWord(dstY);
	Fifo_WriteWord(width);
	Fifo_WriteWord(height);
}


void VMware::Fifo_RectCopy(uint32 srcX, uint32 srcY, uint32 dstX, uint32 dstY,
							uint32 width, uint32 height)
{
	Fifo_WriteWord(SVGA_CMD_RECT_COPY);
	Fifo_WriteWord(srcX);
	Fifo_WriteWord(srcY);
	Fifo_WriteWord(dstX);
	Fifo_WriteWord(dstY);
	Fifo_WriteWord(width);
	Fifo_WriteWord(height);
}

