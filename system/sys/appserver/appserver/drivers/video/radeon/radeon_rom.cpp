/*
 ** Radeon graphics driver for Syllable application server
 *  Copyright (C) 2004 Michael Krueger <invenies@web.de>
 *  Copyright (C) 2003 Arno Klenke <arno_klenke@yahoo.com>
 *  Copyright (C) 1998-2001 Kurt Skauen <kurt@atheos.cx>
 *
 ** This program is free software; you can redistribute it and/or
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
 *
 ** For information about used sources and further copyright notices
 *  see radeon.cpp
 */

#include "radeon.h"

#define RADEON_I386_ROM_ADDRESS 0x000c0000
#define RADEON_I386_ROM_ENABLE 0x000c0001

inline uint32 ATIRadeon::pci_size(uint32 base, uint32 mask)
{
	uint32 size = base & mask;
	return size & ~(size-1);
}

uint32 ATIRadeon::get_pci_memory_size(int nFd, PCI_Info_s *pcPCIInfo, int nResource)
{
	int nBus = pcPCIInfo->nBus;
	int nDev = pcPCIInfo->nDevice;
	int nFnc = pcPCIInfo->nFunction;
	int nOffset = PCI_BASE_REGISTERS + nResource * 4;
	uint32 nBase = pci_gfx_read_config(nFd, nBus, nDev, nFnc, nOffset, 4);
	
	pci_gfx_write_config(nFd, nBus, nDev, nFnc, nOffset, 4, ~0);
	uint32 nSize = pci_gfx_read_config(nFd, nBus, nDev, nFnc, nOffset, 4);
	pci_gfx_write_config(nFd, nBus, nDev, nFnc, nOffset, 4, nBase);
	if (!nSize || nSize == 0xffffffff) return 0;
	if (nBase == 0xffffffff) nBase = 0;
	if (!(nSize & PCI_ADDRESS_SPACE)) {
		return pci_size(nSize, PCI_ADDRESS_MEMORY_32_MASK);
	} else {
		return pci_size(nSize, PCI_ADDRESS_IO_MASK & 0xffff);
	}
}

uint32 ATIRadeon::get_pci_rom_memory_size(int nFd, PCI_Info_s *pcPCIInfo)
{
	int nBus = pcPCIInfo->nBus;
	int nDev = pcPCIInfo->nDevice;
	int nFnc = pcPCIInfo->nFunction;
	int nOffset = PCI_ROM_BASE;
	uint32 nBase = pci_gfx_read_config(nFd, nBus, nDev, nFnc, nOffset, 4);

	pci_gfx_write_config(nFd, nBus, nDev, nFnc, nOffset, 4, ~0);
	uint32 nSize = pci_gfx_read_config(nFd, nBus, nDev, nFnc, nOffset, 4);
	pci_gfx_write_config(nFd, nBus, nDev, nFnc, nOffset, 4, nBase);
	if (!nSize || nSize == 0xffffffff) return 0;
	if (nBase == 0xffffffff) nBase = 0;
	return pci_size(nSize, PCI_ROM_ADDRESS_MASK);
}

void ATIRadeon::UnmapROM( int nFd, PCI_Info_s *dev )
{
	if (!rinfo.bios_seg)
		return;

	rinfo.bios_seg = NULL;

	m_pROMBase = NULL;

	if(m_hROMArea >= 0)
	{
		/* This is the workaround for the old crash issue
		   - just remapping it to the end of VRAM works -MK */
		if( remap_area (m_hROMArea, (void *)((rinfo.fb_base_phys + rinfo.video_ram))) < 0 ) {
			dbprintf("Radeon :: failed to unmap ROM area (%d)\n", m_hROMArea);
			return;
		}

		delete_area(m_hROMArea);
	}
	
	/* This will disable and set address to unassigned */
	pci_gfx_write_config( nFd, dev->nBus, dev->nDevice,
		dev->nFunction, PCI_ROM_BASE, 4, 0 );
}

int ATIRadeon::MapROM( int nFd, PCI_Info_s *dev )
{
	uint16 dptr;
	uint8 rom_type;
	unsigned long rom_base_phys;

	/* If this is a primary card, there is a shadow copy of the
	 * ROM somewhere in the first meg (presumably at 0xc0000)
	 */

	/* Fix from ATI for problem with Radeon hardware not leaving ROM enabled */
	unsigned int temp;
	temp = INREG(MPP_TB_CONFIG);
	temp &= 0x00ffffffu;
	temp |= 0x04 << 24;
	OUTREG(MPP_TB_CONFIG, temp);
	temp = INREG(MPP_TB_CONFIG);

	rom_base_phys = RADEON_I386_ROM_ADDRESS;

	pci_gfx_write_config( nFd, dev->nBus, dev->nDevice,
		dev->nFunction, PCI_ROM_BASE, 4, RADEON_I386_ROM_ENABLE );

	RTRACE("Radeon :: ROM base address %x\n", (uint) rom_base_phys);
	RTRACE("Radeon :: ROM size %x\n", (uint) get_pci_rom_memory_size(nFd, dev));

	// map the ROM
	m_pROMBase = NULL;
	m_hROMArea = create_area ("radeon_rom", (void **)&m_pROMBase,
		get_pci_rom_memory_size(nFd, dev), AREA_FULL_ACCESS, AREA_NO_LOCK);
	if( m_hROMArea < 0 ) {
		dbprintf ("Radeon :: failed to create ROM area\n");
		return false;
	}

	if( remap_area (m_hROMArea, (void *)((rom_base_phys))) < 0 ) {
		printf("Radeon :: failed to map ROM area (%d)\n", m_hROMArea);
		return false;
	}

	rinfo.bios_seg = m_pROMBase;

	/* Very simple test to make sure it appeared */
	if (BIOS_IN16(0) != 0xaa55) {
		dbprintf("Radeon :: Warning: Invalid ROM signature %x should be 0xaa55\n",
		       BIOS_IN16(0));
		goto failed;
	}
	/* Look for the PCI data to check the ROM type */
	dptr = BIOS_IN16(0x18);

	/* Check the PCI data signature. If it's wrong, we still assume a normal x86 ROM
	 * for now, until I've verified this works everywhere. The goal here is more
	 * to phase out Open Firmware images.
	 *
	 * Currently, we only look at the first PCI data, we could iteratre and deal with
	 * them all, and we should use fb_bios_start relative to start of image and not
	 * relative start of ROM, but so far, I never found a dual-image ATI card
	 *
	 * typedef struct {
	 * 	uint32	signature;	+ 0x00
	 * 	uint16	vendor;		+ 0x04
	 * 	uint16	device;		+ 0x06
	 * 	uint16	reserved_1;	+ 0x08
	 * 	uint16	dlen;		+ 0x0a
	 * 	uint8	drevision;	+ 0x0c
	 * 	uint8	class_hi;	+ 0x0d
	 * 	uint16	class_lo;	+ 0x0e
	 * 	uint16	ilen;		+ 0x10
	 * 	uint16	irevision;	+ 0x12
	 * 	uint8	type;		+ 0x14
	 * 	uint8	indicator;	+ 0x15
	 * 	uint16	reserved_2;	+ 0x16
	 * } pci_data_t;
	 */
	if (BIOS_IN32(dptr) !=  (('R' << 24) | ('I' << 16) | ('C' << 8) | 'P')) {
		dbprintf("Radeon :: Warning: PCI DATA signature in ROM incorrect: %08x\n",
		       BIOS_IN32(dptr));
		goto anyway;
	}
	rom_type = BIOS_IN8(dptr + 0x14);
	switch(rom_type) {
	case 0:
		dbprintf("Radeon :: Found Intel x86 BIOS ROM Image\n");
		break;
	case 1:
		dbprintf("Radeon :: Found Open Firmware ROM Image\n");
		goto failed;
	case 2:
		dbprintf("Radeon :: Found HP PA-RISC ROM Image\n");
		goto failed;
	default:
		dbprintf("Radeon :: Found unknown type %d ROM Image\n", rom_type);
		goto failed;
	}
 anyway:
	/* Locate the flat panel infos, do some sanity checking !!! */
	rinfo.fp_bios_start = BIOS_IN16(0x48);

	/* Check for ATOM BIOS */
	dptr = rinfo.fp_bios_start + 4;
	if ((BIOS_IN8(dptr)   == 'A' &&
		 BIOS_IN8(dptr+1) == 'T' &&
		 BIOS_IN8(dptr+2) == 'O' &&
		 BIOS_IN8(dptr+3) == 'M') ||
		(BIOS_IN8(dptr)   == 'M' &&
		 BIOS_IN8(dptr+1) == 'O' &&
		 BIOS_IN8(dptr+2) == 'T' &&
		 BIOS_IN8(dptr+3) == 'A'))
		rinfo.bios_type = bios_atom;
	else
		rinfo.bios_type = bios_legacy;

    if (rinfo.bios_type == bios_atom) 
		rinfo.fp_atom_bios_start = BIOS_IN16 (rinfo.fp_bios_start + 32);

	dbprintf( "Radeon :: %s BIOS detected\n", (rinfo.bios_type == bios_atom) ? "ATOM" : "Legacy" );

	return 0;

 failed:
	rinfo.bios_seg = NULL;

	return -ENXIO;
}

int ATIRadeon::FindMemVBios()
{
	/* I simplified this code as we used to miss the signatures in
	 * a lot of case. It's now closer to XFree, we just don't check
	 * for signatures at all... Something better will have to be done
	 * if we end up having conflicts
	 */
	uint32  segstart;

	for(segstart = 0x000c1000; segstart < 0x000f0000; segstart += 0x00001000) {
		if( remap_area (m_hROMArea, (void *)((segstart))) < 0 ) {
			printf("Radeon :: failed to remap ROM area (%d)\n", m_hROMArea);
			return false;
		}
		if ((*m_pROMBase == 0x55) && (((*(m_pROMBase + 1)) & 0xff) == 0xaa))
			break;
	}

	dbprintf("Radeon :: VBIOS at %x detected\n", (uint)segstart);

	/* Locate the flat panel infos, do some sanity checking !!! */
	rinfo.bios_seg = m_pROMBase;
	rinfo.fp_bios_start = BIOS_IN16(0x48);
	return 0;
}

