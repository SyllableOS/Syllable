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

#ifndef __F_KERNEL_ELF_H__
#define __F_KERNEL_ELF_H__

#include <kernel/types.h>
#include <syllable/elf.h>

/* Class constants */
enum
{
	ELFCLASSNONE,
	ELFCLASS32,
	ELFCLASS64
};

/* Endian constants */
enum
{
	ELFDATANONE,
	ELFDATALSB,
	ELFDATAMSB,
};

/* Version numbers */
enum
{
	AEV_CURRENT = 1		/* Current version */
};

/* Elf header type */
enum
{
	ET_NONE,
	ET_REL,
	ET_EXEC,
	ET_DYN,
	ET_CORE,
	ET_LOPROC = 0xff00,
	ET_HIPROC = 0xffff
};

/* Machine architectures */
enum
{
	EM_NONE,
	EM_M32,
	EM_SPARC,
	EM_386,
	EM_68K,
	EM_88K,
	EM_860,
	EM_MIPS
};

/* Special section indexes */
enum
{
	SHN_UNDEF,
	SHN_LORESERVE = 0xff00,
	SHN_LOPROC	  = 0xff00,
	SHN_HIPROC	  = 0xff1f,
	SHN_ABS	  = 0xfff1,
	SHN_COMMON	  = 0xfff2,
	SHN_HIRESERVE = 0xffff
};

typedef struct
{
    uint32 ph_nType;
    uint32 ph_nOffset;
    uint32 ph_nVirtualAddress;
    uint32 ph_nPhysicalAddress;
    uint32 ph_nFileSize;
    uint32 ph_nMemSize;
    uint32 ph_nFlags;
    uint32 ph_nAlign;
} Elf32_PrgHeader_s;

/* Section types */
enum
{
	SHT_NULL	 = 0,
	SHT_PROGBITS = 1,	// .text segment
	SHT_SYMTAB   = 2,		// symbol table
	SHT_STRTAB	 = 3,		// string table
	SHT_RELA	 = 4,		// reloc table whith explicit addends
	SHT_HASH	 = 5,		// symbol hash table
	SHT_DYNAMIC  = 6,
	SHT_NOTE	 = 7,
	SHT_NOBITS	 = 8,		// .bss segment
	SHT_REL	 = 9,		// reloc table whithout addends
	SHT_SHLIB	 = 10,
	SHT_DYNSYM	 = 11,		// symbols needed for dynamic linking
	SHT_LOPROC	= 0x70000000,
	SHT_HIPROC	= 0x7fffffff,
	SHT_LOUSER	= 0x80000000,
	SHT_RELOC_MAP,
	SHT_LIBTAB,
	SHT_HIUSER	= 0xffffffff
};

enum
{
	PHT_PROGRAM = 1024
};

/* Section attribute flags */
enum
{
	SHF_WRITE	 = 0x01,
	SHF_ALLOC	 = 0x02,
	SHF_EXECINST = 0x04,
	SHF_MASCPROC = 0xf0000000
};

typedef	struct
{
	int	 	st_nName;
	uint32 	st_nValue;
	int	 	st_nSize;
	uint8	st_nInfo;
	char	st_nOther;
	int16	st_nSecIndex;
} Elf32_Symbol_s;

#define	ELF32_ST_BIND(i)		(((i)>>4) & 0x0f)
#define	ELF32_ST_TYPE(i)		((i) & 0x0f)
#define	ELF32_ST_INFO(b,t)	(((b) << 4) + ((t) & 0x0f))

/* Symbol binding (ELF32_ST_BIND()) */
enum
{
	STB_LOCAL,
	STB_GLOBAL,
	STB_WEAK,
	STB_LOPROC	 = 13,
	STB_EXPORTED = STB_LOPROC,
	STB_HIPROC	 = 15
};

/* Symbol types (AELF32_ST_TYPE()) */
enum
{
	STT_NOTYPE,
	STT_OBJECT,
	STT_FUNC,
	STT_SECTION,
	STT_FILE,
	STT_LOPROC	= 13,
	STT_HIPROC	= 15
};

/* This structure correspond to "Elf32_Rela" in the elf specs. Syllable does't use "Elf32_Rel" */
typedef struct
{
	uint32	r_nOffset;
	uint32	r_nInfo;
	uint32	r_nAddend;
} Elf32_RelocA_s;

typedef struct
{
	uint32	r_nOffset;
	uint32	r_nInfo;
} Elf32_Reloc_s;

#define	ELF32_R_SYM(i)	  ((i)>>8)
#define	ELF32_R_TYPE(i)	  ((unsigned char)(i))
#define	ELF32_R_INFO(s,t) (((s)<<8) + (unsigned char) (t) )

enum
{
	R_386_NONE,
	R_386_32,
	R_386_PC32,
	R_386_GOT32,
	R_386_PLT32,
	R_386_COPY,
	R_386_GLOB_DATA,
	R_386_JMP_SLOT,
	R_386_RELATIVE,
	R_386_GOTOFF,
	R_386_GOTPC
};

typedef struct
{
	int32	d_nTag;
	union
	{
		int32  d_nValue;
		uint32 d_nPointer;
	} d_un;
} Elf32_Dynamic_s;

enum
{
	DT_NULL,		/* Marks the end of the _DYNAMIC array */
	DT_NEEDED,		/* Offset to a needed dll name in the DT_STRTAB string section	*/
	DT_PLTRELSZ,	/* Total size for the reloc table associated with the PLT	*/
	DT_PLTGOT,
	DT_HASH,		/* Hash table for the DT_SYMTAB section				*/
	DT_STRTAB,		/* Address of the string table					*/
	DT_SYMTAB,		/* Address of the symbol table					*/
	DT_RELA,		/* Reloc table with explicit addends				*/
	DT_RELASZ,		/* Size of DT_RELA						*/
	DT_RELAENT,		/* Size in bytes of an entry in DT_RELA				*/
	DT_STRSZ,		/* Size in bytes of the string table				*/
	DT_SYMENT,		/* Size in byte of an entry in DT_SYMTAB			*/
	DT_INIT,		/* Address of the initialization function			*/
	DT_FINI,		/* Address of the termination function				*/
	DT_SONAME,		/* Offset into DT_STRTAB of this dll's name			*/
	DT_RPATH,		/* Offset into DT_STRTAB for the dll search path		*/
	DT_SYMBOLIC,	/* If present start symbol search in the dll itself		*/
	DT_REL,		/* Reloc table with implicit addends				*/
	DT_RELSZ,		/* Size of DT_REL						*/
	DT_RELENT,		/* Size in bytes of an entry in DT_REL				*/
	DT_PLTREL,		/* Type of reloc table for PLT (DT_REL or DT_RELA)		*/
	DT_DEBUG,		/* Not defined							*/
	DT_TEXTREL,		/* If present relocs may alter read-only sections		*/
	DT_JMPREL,		/* Reloc entries for PLT (If lazy binding is enabled)		*/
	DT_LOPROC  = 0x70000000,
	DT_HIPROC  = 0x7fffffff
};

typedef struct
{
	uint32 h_nBucketEntries;
	uint32 h_nChainEntries;
} Elf32_HashTable_s;

enum
{
	STN_UNDEF
};

#endif /* __F_KERNEL_ELF_H__ */
