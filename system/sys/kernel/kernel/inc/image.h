
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

#ifndef __F_ELF_H__
#define __F_ELF_H__

#include <atheos/filesystem.h>
#include <atheos/elf.h>
#include <atheos/image.h>
#include <atheos/bootmodules.h>

#include "typedefs.h"
#include "../vfs/vfs.h"

#ifdef __cplusplus
extern "C"
{
#if 0
}				/*make emacs indention work */
#endif
#endif

typedef struct _ElfSymbol ElfSymbol_s;
struct _ElfSymbol
{
	ElfSymbol_s *s_psHashNext;
	const char *s_pzName;
	uint16 s_nInfo;
	int16 s_nImage;		/* -1 = parent, 0 = this, > 0 = subimage[n-1] */
	uint32 s_nAddress;
	int s_nSize;
	int s_nSection;
};

#define MAX_IMAGE_COUNT 256

typedef struct _ElfImageInst ElfImageInst_s;
typedef struct _ElfImage ElfImage_s;

struct _ElfImageInst
{
	ElfImage_s *ii_psImage;
	ElfImageInst_s **ii_apsSubImages;
	int ii_nHandle;
	atomic_t ii_nOpenCount;	/* Total number of references                    */
	atomic_t ii_nAppOpenCount;	/* Number of times loaded by load_library()      */
	uint32 ii_nTextAddress;	/* The text-area address                         */
	area_id ii_nTextArea;
	area_id ii_nDataArea;
	bool ii_bRelocated;
};

#define ELF_KERNEL_SYM_HASHTAB_SIZE	(1024)
struct _ElfImage
{
	ElfImage_s *im_psNext;
	char im_zName[64];
	char *im_pzPath;
	char *im_pzArguments;
	atomic_t im_nOpenCount;
	int im_nSectionCount;
	Elf32_SectionHeader_s *im_pasSections;

	char *im_pzStrings;
	int im_nSymCount;
	ElfSymbol_s *im_pasSymbols;
	ElfSymbol_s *im_apsKernelSymHash[ELF_KERNEL_SYM_HASHTAB_SIZE];
	int im_nRelocCount;
	Elf32_Reloc_s *im_pasRelocs;
	
	Elf32_HashTable_s* im_psHashTable;
	uint32* im_pnHashBucket;
	uint32* im_pnHashChain;

	int im_nSubImageCount;
	const char **im_apzSubImages;
	uint32 im_nVirtualAddress;	/* Address the linker ment to put it one     */
	uint32 im_nTextSize;
	uint32 im_nOffset;	/* File offset into text section (im_nVirtualAddress) */
	uint32 im_nEntry;	/* Offset to entry point */
	uint32 im_nInit;	/* Offset to init routine (c-tors) */
	uint32 im_nFini;	/* Offset to fini routine (d-tors) */
	File_s *im_psFile;
	BootModule_s *im_psModule;
};

typedef struct
{
	ElfImageInst_s *ic_psInstances[MAX_IMAGE_COUNT];
	sem_id ic_hLock;
} ImageContext_s;


ImageContext_s *create_image_ctx( void );

//ImageContext_s* clone_image_ctx( Process_s* psSrcProc );
ImageContext_s *clone_image_context( ImageContext_s * psOrig, MemContext_s *psNewMemCtx );
void close_all_images( ImageContext_s * psCtx );
void delete_image_context( ImageContext_s * psCtx );
int load_image_inst( ImageContext_s * psCtx, const char *pzPath, BootModule_s * psModule, int nMode, ElfImageInst_s **ppsInst, const char *pzLibraryPath );
Inode_s *get_image_inode( int nLibrary );
void init_image_module( void );


#ifdef __cplusplus
}
#endif

#endif /* __F_ELF_H__ */
