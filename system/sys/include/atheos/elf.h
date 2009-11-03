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

#ifndef __F_SYLLABLE_ELF_H__
#define __F_SYLLABLE_ELF_H__

#include <syllable/types.h>

enum
{
	EI_MAG0,
	EI_MAG1,
	EI_MAG2,
	EI_MAG3,
	EI_CLASS,
	EI_DATA,
	EI_VERSION,
	EI_PAD,
	EI_NIDENT = 16
};

enum
{
	ELFMAG0	= 0x7f,
	ELFMAG1	= 'E',
	ELFMAG2	= 'L',
	ELFMAG3	= 'F',
};

typedef struct
{
    char   e_anIdent[ EI_NIDENT ];
    uint16 e_nType;			// File type
    uint16 e_nMachine;			// Target machine for this executable
    int	   e_nVersion;			// File format version number
    uint32 e_nEntry;			// Executables entry point
    int	   e_nProgHdrOff;		// File offset to first program header
    int	   e_nSecHdrOff;		// File offset to first section header
    uint32 e_nFlags;
    uint16 e_nElfHdrSize;		// Size of this structure ??!??!?
    uint16 e_nProgHdrEntrySize;		// Size of each program header
    uint16 e_nProgHdrCount;		// Number of programm headers
    uint16 e_nSecHdrSize;		// Size of each section header
    uint16 e_nSecHdrCount;		// Number of section headers
    uint16 e_nSecNameStrTabIndex;	// String table for section names
} Elf32_ElfHdr_s;

typedef struct
{
	int	   sh_nName;
	int	   sh_nType;
	uint32 sh_nFlags;
	uint32 sh_nAddress;
	uint32 sh_nOffset;
	uint32 sh_nSize;
	uint32 sh_nLink;
	uint32 sh_nInfo;
	uint32 sh_nAddrAlign;	
	uint32 sh_nEntrySize;	// Size of each entry if the section contains a table
} Elf32_SectionHeader_s;

#endif	/* __F_SYLLABLE_ELF_H__ */
