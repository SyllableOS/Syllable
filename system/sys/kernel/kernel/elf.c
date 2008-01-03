/*
 *  The AtheOS kernel
 *  Copyright (C) 1999 - 2001 Kurt Skauen
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

#include <posix/fcntl.h>

#include <posix/errno.h>
#include <posix/limits.h>
#include <posix/unistd.h>

#include <atheos/types.h>
#include <atheos/kernel.h>
#include <atheos/smp.h>
#include <atheos/elf.h>
#include <atheos/semaphore.h>
#include <atheos/ctype.h>
#include <atheos/device.h>

#include <macros.h>

#include "inc/scheduler.h"
#include "inc/sysbase.h"
#include "inc/global.h"
#include "inc/areas.h"
#include "inc/image.h"
#include "inc/mman.h"
#include "inc/ksyms.h"

static sem_id g_hImageList;
static ImageContext_s *g_psKernelCtx;

int Seek( int, int, int );

static ElfImage_s *g_psFirstImage = NULL;

#define	CMP_INODES( a, b ) ((a)->i_nInode == (b)->i_nInode && (a)->i_psVolume->v_nDevNum == (b)->i_psVolume->v_nDevNum)


static uint32 elf_sym_hash( const char *pzName )
{
	uint32 h = 0;

	while ( *pzName != '\0' )
	{
		h = ( h << 2 ) + ( *pzName++ ) * 1009;
//    if ( (g = h & 0xf0000000) != 0 ) {
//      h ^= g >> 24;
//    }
//    h &= -g;
	}
	return ( h );
}

static uint32 elf_sysv_sym_hash( const char *pzName )
{
	uint32 h = 0, g;

	while ( *pzName )
	{
		h = ( h << 4 ) + ( *pzName++ );
		if( ( g = ( h & 0xf0000000 ) ) )
			h ^= g >> 24;
		h &= ~g;
	}	

	return ( h );
}

static void create_kernel_image( void )
{
	ElfImage_s *psImage;
	ElfImageInst_s *psInst;
	int i;

	psImage = kmalloc( sizeof( ElfImage_s ), MEMF_KERNEL | MEMF_CLEAR | MEMF_OKTOFAIL );
	psInst = kmalloc( sizeof( ElfImageInst_s ), MEMF_KERNEL | MEMF_CLEAR | MEMF_OKTOFAIL );

	if ( psImage == NULL || psInst == NULL )
	{
		panic( "create_kernel_image() no memory for image\n" );
		return;
	}

	psInst->ii_nTextArea = -1;
	psInst->ii_nDataArea = -1;

	psImage->im_nSymCount = ksym_get_symbol_count();
	psImage->im_pasSymbols = kmalloc( psImage->im_nSymCount * sizeof( ElfSymbol_s ), MEMF_KERNEL | MEMF_CLEAR | MEMF_OKTOFAIL );

	if ( psImage->im_pasSymbols == NULL )
	{
		panic( "create_kernel_image() no memory for symbols\n" );
		return;
	}
	g_psKernelCtx = create_image_ctx();

	if ( g_psKernelCtx == NULL )
	{
		panic( "create_kernel_image() no memory for kernel image context\n" );
		return;
	}

	for ( i = 0; i < psImage->im_nSymCount; ++i )
	{
		void *pValue;
		const char *pzName;
		uint32 nHash;

		pzName = ksym_get_symbol( i, &pValue );

		psImage->im_pasSymbols[i].s_pzName = pzName;
		psImage->im_pasSymbols[i].s_nInfo = ELF32_ST_INFO( STB_GLOBAL, STT_FUNC );
		psImage->im_pasSymbols[i].s_nImage = 0;
		psImage->im_pasSymbols[i].s_nAddress = ( uint32 )pValue;
		psImage->im_pasSymbols[i].s_nSize = 0;
		psImage->im_pasSymbols[i].s_nSection = 1;

		nHash = elf_sym_hash( pzName ) % ELF_KERNEL_SYM_HASHTAB_SIZE;
		psImage->im_pasSymbols[i].s_psHashNext = psImage->im_apsKernelSymHash[nHash];
		psImage->im_apsKernelSymHash[nHash] = &psImage->im_pasSymbols[i];
	}
	psImage->im_psNext = NULL;
	strcpy( psImage->im_zName, "libkernel.so" );
	psImage->im_pzPath = "";
	atomic_set( &psImage->im_nOpenCount, 1 );
	psImage->im_nSectionCount = 0;
	psImage->im_pasSections = NULL;
	psImage->im_pzStrings = NULL;
	psImage->im_nRelocCount = 0;
	psImage->im_pasRelocs = 0;
	psImage->im_nSubImageCount = 0;
	psImage->im_apzSubImages = NULL;
	psImage->im_nVirtualAddress = 1024 * 1024;	/* Address the linker ment to put it one     */
	psImage->im_nTextSize = 0;
	psImage->im_nOffset = 0;	/* File offset into text section (im_nVirtualAddress) */
	psImage->im_nEntry = 0;	/* Offset to entry point */
	psImage->im_nInit = 0;	/* Offset to init routine (c-tors) */
	psImage->im_nFini = 0;	/* Offset to fini routine (d-tors) */
	psImage->im_psFile = NULL;


	psInst->ii_psImage = psImage;
	psInst->ii_apsSubImages = NULL;
	psInst->ii_nHandle = 0;
	atomic_set( &psInst->ii_nOpenCount, 1 );	/* Total number of references                 */
	atomic_set( &psInst->ii_nAppOpenCount, 1 );	/* Number of times loaded by load_library()   */
	psInst->ii_nTextArea = -1;
	psInst->ii_nDataArea = -1;
	psInst->ii_bRelocated = true;

	g_psFirstImage = psImage;

	g_psKernelCtx->ic_psInstances[0] = psInst;
}

int read_image_data( ElfImage_s *psImage, void *pBuffer, off_t nOffset, size_t nSize )
{
	if ( psImage->im_psModule != NULL )
	{
		if ( nOffset + nSize > psImage->im_psModule->bm_nSize )
		{
			nSize = psImage->im_psModule->bm_nSize - nOffset;
		}
		if ( nSize < 0 )
		{
			return ( 0 );
		}
		memcpy( pBuffer, ( char * )psImage->im_psModule->bm_pAddress + nOffset, nSize );
		return ( nSize );
	}
	else
	{
		return ( read_pos_p( psImage->im_psFile, nOffset, pBuffer, nSize ) );
	}

}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int open_library_file( const char *pzName, const char *pzSearchPath, char *pzFullPath )
{
	char zPath[512];
	char zSysLibPath[256];
	int nPathLen;
	int nFile;

	nFile = open( pzName, O_RDONLY );

	if ( nFile >= 0 )
	{
		return ( nFile );
	}

	if ( pzSearchPath == NULL )
	{
		sys_get_system_path( zSysLibPath, 128 );
		nPathLen = strlen( zSysLibPath );

		if ( '/' != zSysLibPath[nPathLen - 1] )
		{
			zSysLibPath[nPathLen] = '/';
			zSysLibPath[nPathLen + 1] = '\0';
		}

		strcat( zSysLibPath, "system/libraries/:/system/indexes/lib/" );
		pzSearchPath = zSysLibPath;
	}

	if ( pzSearchPath != NULL )
	{
		const int nNameLen = strlen( pzName );
		const char *pzStart = pzSearchPath;

		for ( ;; )
		{
			const char *pzSep = strchr( pzStart, ':' );
			int nLen;

			if ( pzSep == NULL )
			{
				nLen = strlen( pzStart );
			}
			else
			{
				nLen = pzSep - pzStart;
			}
			if ( nLen != 0 )
			{
				int nTotLen = nLen + nNameLen + 1;
				Inode_s *psParent = NULL;

				if ( pzStart[nLen - 1] != '/' )
				{
					nTotLen++;	// We must add a "/" to the path
				}
				if ( nLen > 8 && strncmp( pzStart, "@bindir@/", 9 ) == 0 )
				{
					pzStart += 9;
					nLen -= 9;
					psParent = CURRENT_PROC->pr_psIoContext->ic_psBinDir;
				}
				if ( nTotLen > sizeof( zPath ) )
				{
					pzStart += nLen + 1;
					continue;
				}
				memcpy( zPath, pzStart, nLen );
				pzStart += nLen + 1;
				if ( zPath[nLen - 1] != '/' )
				{
					zPath[nLen++] = '/';
				}
				strcpy( zPath + nLen, pzName );
				nFile = extended_open( true, psParent, zPath, NULL, O_RDONLY, 0 );

				if ( nFile >= 0 )
				{
					return ( nFile );
				}
			}
			else
			{
				pzStart++;
			}
			if ( pzSep == 0 )
			{
				break;
			}
		}
	}
	printk( "WARNING: open_library_file() failed to find %s in search path\n", pzName );
	return ( -1 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 *	Locate and loads the relocation tables.
 *	Build a list of needed libraries.
 *	Find the init and fini sections.
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int parse_dynamic_section( ElfImage_s *psImage, Elf32_SectionHeader_s * psSection )
{
	int i;
	int nLibNum;
	int nDynRelSize = 0;
	uint32 nDynRelAddress = -1;
	int nDynPltRelSize = 0;
	uint32 nDynPltRelAddress = -1;
	int nDynRelEntSize = sizeof( Elf32_Reloc_s );
	Elf32_Dynamic_s *psDynTab;
	int nError;

	psDynTab = kmalloc( psSection->sh_nSize, MEMF_KERNEL | MEMF_OKTOFAIL );
	if ( psDynTab == NULL )
	{
		printk( "Error: parse_dynamic_section() no memory for dynamic section\n" );
		nError = -ENOMEM;
		goto error1;
	}

	nError = read_image_data( psImage, psDynTab, psSection->sh_nOffset, psSection->sh_nSize );

	if ( nError != psSection->sh_nSize )
	{
		if ( nError >= 0 )
		{
			nError = -ENOEXEC;
		}
		printk( "Error: parse_dynamic_section() failed to load dynamic section\n" );
		goto error2;
	}

	for ( i = 0; i < psSection->sh_nSize / psSection->sh_nEntrySize; ++i )
	{
		Elf32_Dynamic_s *psDyn = &psDynTab[i];

		switch ( psDyn->d_nTag )
		{
		case DT_NEEDED:
			psImage->im_nSubImageCount++;
			break;
		case DT_REL:
			nDynRelAddress = psDyn->d_un.d_nPointer;
			break;
		case DT_RELSZ:
			nDynRelSize = psDyn->d_un.d_nValue;
			break;
		case DT_JMPREL:
			nDynPltRelAddress = psDyn->d_un.d_nPointer;
			break;
		case DT_PLTRELSZ:
			nDynPltRelSize = psDyn->d_un.d_nValue;
			break;
		case DT_RELENT:
			nDynRelEntSize = psDyn->d_un.d_nValue;
			break;
		case DT_INIT:
			psImage->im_nInit = psDyn->d_un.d_nPointer;
			break;
		case DT_FINI:
			psImage->im_nFini = psDyn->d_un.d_nPointer;
			break;
		default:
//      printk( "Unknown dyamic entry %d, %d\n", psDyn->d_nTag, psDyn->d_un.d_nValue );
			break;
		}
	}

	nDynRelEntSize = 8;

	if ( nDynRelSize + nDynPltRelSize > 0 )
	{
		psImage->im_pasRelocs = kmalloc( nDynRelSize + nDynPltRelSize, MEMF_KERNEL | MEMF_OKTOFAIL );

		if ( psImage->im_pasRelocs == NULL )
		{
			printk( "Error: parse_dynamic_section() no memory for reloc table\n" );
			nError = -ENOMEM;
			goto error2;
		}

		if ( nDynRelSize > 0 )
		{
			nError = read_image_data( psImage, psImage->im_pasRelocs, nDynRelAddress - psImage->im_nVirtualAddress + psImage->im_nOffset, nDynRelSize );

			if ( nError != nDynRelSize )
			{
				if ( nError >= 0 )
				{
					nError = -ENOEXEC;
				}
				printk( "Error: parse_dynamic_section() failed to read reloc table\n" );
				goto error3;
			}
		}
		if ( nDynPltRelSize > 0 )
		{
			nError = read_image_data( psImage, ( ( char * )psImage->im_pasRelocs ) + nDynRelSize, nDynPltRelAddress - psImage->im_nVirtualAddress + psImage->im_nOffset, nDynPltRelSize );

			if ( nError != nDynPltRelSize )
			{
				if ( nError >= 0 )
				{
					nError = -ENOEXEC;
				}
				printk( "Error: parse_dynamic_section() failed to read plt reloc table\n" );
				goto error3;
			}
		}
		psImage->im_nRelocCount = ( nDynRelSize + nDynPltRelSize ) / sizeof( Elf32_Reloc_s );
	}

	psImage->im_apzSubImages = kmalloc( sizeof( char * ) * psImage->im_nSubImageCount, MEMF_KERNEL | MEMF_CLEAR | MEMF_OKTOFAIL );

	if ( psImage->im_apzSubImages == NULL )
	{
		printk( "Error: parse_dynamic_section() no memory for subimage list\n" );
		nError = -ENOMEM;
		goto error3;
	}

	nLibNum = 0;
	for ( i = 0; i < psSection->sh_nSize / psSection->sh_nEntrySize; ++i )
	{
		Elf32_Dynamic_s *psDyn = &psDynTab[i];

		if ( psDyn->d_nTag == DT_NEEDED )
		{
			psImage->im_apzSubImages[nLibNum] = psImage->im_pzStrings + psDyn->d_un.d_nValue;
			nLibNum++;
		}
	}
	kfree( psDynTab );
	return ( EOK );
      error3:
	kfree( psImage->im_pasRelocs );
	psImage->im_pasRelocs = NULL;
      error2:
	kfree( psDynTab );
      error1:
	return ( nError );
}


/*****************************************************************************
 * NAME:
 * DESC:
 *	Load the section headers from file.
 *	Find the virtual address, and file offset of the first
 *	interesting section (Ie. not of SHT_NULL type)
 *	Allocate and initialize the symbol table.
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int parse_section_headers( ElfImage_s *psImage )
{
	Elf32_Symbol_s *psSymTab = NULL;
	int nDynSymSec = -1;
	int nDynamicSec = -1;
	int nHashSec = -1;
	uint32 nEndAddress = 0;
	int nError = EOK;
	int i;

	psImage->im_nVirtualAddress = -1;
	
	for ( i = 0; i < psImage->im_nSectionCount; ++i )
	{
		Elf32_SectionHeader_s *psSection;

		psSection = &psImage->im_pasSections[i];

		if ( psSection->sh_nType != SHT_NULL && psImage->im_nVirtualAddress == -1 )
		{
			psImage->im_nVirtualAddress = psSection->sh_nAddress;
			psImage->im_nOffset = psSection->sh_nOffset;
		}

		switch ( psSection->sh_nType )
		{
		case SHT_PROGBITS:
			if ( psSection->sh_nAddress + psSection->sh_nSize > nEndAddress )
			{
				nEndAddress = psSection->sh_nAddress + psSection->sh_nSize;
			}
			break;
		case SHT_DYNSYM:
			nDynSymSec = i;
			break;
		case SHT_DYNAMIC:
			nDynamicSec = i;
			break;
		case SHT_HASH:
			nHashSec = i;
			break;
		}
	}

	if ( psImage->im_nVirtualAddress == -1 )
	{
		printk( "Error: parse_section_headers() : Image has no text segment!\n" );
		return ( -ENOEXEC );
	}
	
	psImage->im_nOffset -= psImage->im_nVirtualAddress & ~PAGE_MASK;
	psImage->im_nVirtualAddress &= PAGE_MASK;

	psImage->im_nTextSize = nEndAddress - psImage->im_nVirtualAddress;

	if ( nDynSymSec != -1 )
	{
		Elf32_SectionHeader_s *psSection = &psImage->im_pasSections[nDynSymSec];
		Elf32_SectionHeader_s *psStrSection = &psImage->im_pasSections[psSection->sh_nLink];

		psImage->im_nSymCount = psSection->sh_nSize / psSection->sh_nEntrySize;

		psSymTab = kmalloc( psSection->sh_nSize, MEMF_KERNEL | MEMF_OKTOFAIL );

		if ( psSymTab == NULL )
		{
			nError = -ENOMEM;
			printk( "parse_section_headers() no memory for raw symbol table\n" );
			goto error1;
		}
		psImage->im_pzStrings = kmalloc( psStrSection->sh_nSize, MEMF_KERNEL | MEMF_OKTOFAIL );

		if ( psImage->im_pzStrings == NULL )
		{
			nError = -ENOMEM;
			printk( "parse_section_headers() no memory for string table\n" );
			goto error2;
		}
		nError = read_image_data( psImage, psImage->im_pzStrings, psStrSection->sh_nOffset, psStrSection->sh_nSize );
		if ( nError != psStrSection->sh_nSize )
		{
			printk( "parse_section_headers() failed to load string table\n" );
			if ( nError >= 0 )
			{
				nError = -ENOEXEC;
			}
			goto error3;
		}
		nError = read_image_data( psImage, psSymTab, psSection->sh_nOffset, psSection->sh_nSize );
		if ( nError != psSection->sh_nSize )
		{
			printk( "parse_section_headers() failed to load symbol table\n" );
			if ( nError >= 0 )
			{
				nError = -ENOEXEC;
			}
			goto error3;
		}
		psImage->im_pasSymbols = kmalloc( psImage->im_nSymCount * sizeof( ElfSymbol_s ), MEMF_KERNEL | MEMF_OKTOFAIL );

		if ( psImage->im_pasSymbols == NULL )
		{
			printk( "Error: parse_section_headers() no memory for symbol table\n" );
			nError = -ENOMEM;
			goto error3;
		}

		for ( i = 0; i < psImage->im_nSymCount; ++i )
		{
//			uint32 nHash;

			psImage->im_pasSymbols[i].s_pzName = psImage->im_pzStrings + psSymTab[i].st_nName;
			psImage->im_pasSymbols[i].s_nInfo = psSymTab[i].st_nInfo;
			psImage->im_pasSymbols[i].s_nAddress = psSymTab[i].st_nValue;
			psImage->im_pasSymbols[i].s_nSize = psSymTab[i].st_nSize;
			psImage->im_pasSymbols[i].s_nSection = psSymTab[i].st_nSecIndex;
			psImage->im_pasSymbols[i].s_nImage = 0;

			#if 0
			if ( psImage->im_pasSymbols[i].s_nSection != SHN_UNDEF )
			{
				nHash = elf_sym_hash( psImage->im_pasSymbols[i].s_pzName ) % ELF_SYM_HASHTAB_SIZE;
				psImage->im_pasSymbols[i].s_psHashNext = psImage->im_apsSymHash[nHash];
				psImage->im_apsSymHash[nHash] = &psImage->im_pasSymbols[i];
			}
			#endif
		}
		kfree( psSymTab );
		psSymTab = NULL;
	}

	if ( nDynSymSec != -1 )
	{
		nError = parse_dynamic_section( psImage, &psImage->im_pasSections[nDynamicSec] );
		if ( nError < 0 )
		{
			goto error4;
		}
	}
	
	if( nHashSec != -1 )
	{
		/* Load hash table */
		Elf32_SectionHeader_s *psSection = &psImage->im_pasSections[nHashSec];

		psImage->im_psHashTable = kmalloc( psSection->sh_nSize, MEMF_KERNEL | MEMF_OKTOFAIL );
	
		if ( psImage->im_psHashTable == NULL )
		{
			nError = -ENOMEM;
			printk( "parse_section_headers() no memory for hash symbol table\n" );
			goto error4;
		}
		nError = read_image_data( psImage, psImage->im_psHashTable, psSection->sh_nOffset, psSection->sh_nSize );
		
		if ( nError != psSection->sh_nSize )
		{
			printk( "parse_section_headers() failed to load hash table\n" );
			if ( nError >= 0 )
			{
				nError = -ENOEXEC;
			}
			goto error5;
		}
		psImage->im_pnHashBucket = (uint32*)( psImage->im_psHashTable + 1 );
		psImage->im_pnHashChain = psImage->im_pnHashBucket + psImage->im_psHashTable->h_nBucketEntries;
		
		kerndbg( KERN_DEBUG_LOW, "Buckets: %i Chains: %i\n", (int)psImage->im_psHashTable->h_nBucketEntries, (int)psImage->im_psHashTable->h_nChainEntries );
		
		for( i = 0; i < psImage->im_psHashTable->h_nBucketEntries; i++ )
			kerndbg( KERN_DEBUG_LOW, "Entry at %i\n", psImage->im_pnHashBucket[i] );
		
	}

	return ( 0 );
	  error5:
	kfree( psImage->im_psHashTable );
      error4:
	kfree( psImage->im_pasSymbols );
	psImage->im_pasSymbols = NULL;
      error3:
	kfree( psImage->im_pzStrings );
	psImage->im_pzStrings = NULL;
      error2:
	kfree( psSymTab );
      error1:
	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static ElfSymbol_s *lookup_symbol( ElfImage_s *psImage, const char *pzName )
{
	ElfSymbol_s *psSym;
	
	if( psImage->im_psHashTable )
	{
		/* Use hashtable provided by the image */
		Elf32_HashTable_s* psHashTab = psImage->im_psHashTable;
		uint32 nSysvHash = elf_sysv_sym_hash( pzName );
		kerndbg( KERN_DEBUG_LOW, "Symbol %s Hash %x Entry %x\n", pzName, nSysvHash, nSysvHash % psHashTab->h_nBucketEntries );
		nSysvHash %= psHashTab->h_nBucketEntries;
		
	
		if( psImage->im_pnHashBucket[nSysvHash] != STN_UNDEF )
		{
			uint32 nSymbol = psImage->im_pnHashBucket[nSysvHash];
			do
			{
				psSym = &psImage->im_pasSymbols[nSymbol];
				if ( psSym->s_nSection != SHN_UNDEF && strcmp( psSym->s_pzName, pzName ) == 0 )
				{
					kerndbg( KERN_DEBUG_LOW, "Found symbol Num: %i Name: %s\n", nSymbol, psSym->s_pzName );
					return ( psSym );
				}
				nSymbol = psImage->im_pnHashChain[nSymbol];
			} while( nSymbol != STN_UNDEF );
		}	
	}
	else
	{
		/* For the kernel library use the created hash table */
		uint32 nHash = elf_sym_hash( pzName ) % ELF_KERNEL_SYM_HASHTAB_SIZE;
		for ( psSym = psImage->im_apsKernelSymHash[nHash]; psSym != NULL; psSym = psSym->s_psHashNext )
		{
			if ( psSym->s_nSection != SHN_UNDEF && strcmp( psSym->s_pzName, pzName ) == 0 )
			{			
				return ( psSym );
			}
		}
	}
	return ( NULL );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int find_symbol( ImageContext_s * psCtx, const char *pzName, ElfImageInst_s **ppsInst, ElfSymbol_s **ppsSym )
{
	ElfSymbol_s *psSym;
	ElfSymbol_s *psWeak = NULL;
	ElfImageInst_s *psWeakInst = NULL;
	int i;

	for ( i = 0; i < MAX_IMAGE_COUNT; ++i )
	{
		ElfImageInst_s *psInst = psCtx->ic_psInstances[i];
		ElfImage_s *psImage;

		if ( psInst == NULL )
		{
			continue;
		}
		psImage = psInst->ii_psImage;
		psSym = lookup_symbol( psImage, pzName );

		if ( psSym != NULL )
		{
			if ( ELF32_ST_BIND( psSym->s_nInfo ) == STB_LOCAL )
			{
				printk( "Warning: find_symbol() Found local symbol '%s'\n", pzName );
			}
			if ( ELF32_ST_BIND( psSym->s_nInfo ) == STB_WEAK )
			{
				if ( psWeak == NULL )
				{
					psWeak = psSym;
					psWeakInst = psInst;
				}
			}
			else
			{
				if ( psWeak != NULL )
				{
					kerndbg( KERN_DEBUG, "Symbol '%s' overrided by nonweek\n", pzName );
				}
				*ppsSym = psSym;
				*ppsInst = psInst;
				return ( EOK );
			}
		}
	}
	if ( psWeak != NULL )
	{
		*ppsSym = psWeak;
		*ppsInst = psWeakInst;
		return ( EOK );

	}
	
	return ( -ENOSYM );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static void unload_image( ElfImage_s *psImage )
{
	int nLastUnload = 1;

	if ( atomic_read( &psImage->im_nOpenCount ) != -1 )
	{
		nLastUnload = atomic_dec_and_test( &psImage->im_nOpenCount );

		if ( atomic_read( &psImage->im_nOpenCount ) < 0 )
		{
			printk( "PANIC : unload_image() Image %s got open count of %d\n", psImage->im_zName, atomic_read( &psImage->im_nOpenCount ) );
			return;
		}
	}
	if ( nLastUnload )
	{
		ElfImage_s **ppsTmp;

		LOCK( g_hImageList );
		for ( ppsTmp = &g_psFirstImage; NULL != *ppsTmp; ppsTmp = &( *ppsTmp )->im_psNext )
		{
			if ( *ppsTmp == psImage )
			{
				*ppsTmp = psImage->im_psNext;
				break;
			}
		}
		UNLOCK( g_hImageList );
		if ( psImage->im_pasSections != NULL )
		{
			kfree( psImage->im_pasSections );
		}
		if ( psImage->im_pasSymbols != NULL )
		{
			kfree( psImage->im_pasSymbols );
		}
		if ( psImage->im_pasRelocs != NULL )
		{
			kfree( psImage->im_pasRelocs );
		}
		if ( psImage->im_psHashTable != NULL )
		{
			//printk( "Freeing hash table!\n" );
			kfree( psImage->im_psHashTable );
		}
		if ( psImage->im_apzSubImages != NULL )
		{
			kfree( psImage->im_apzSubImages );
		}
		if ( psImage->im_pzStrings != NULL )
		{
			kfree( psImage->im_pzStrings );
		}
		if ( psImage->im_psFile != NULL )
		{
			put_fd( psImage->im_psFile );
		}
		kfree( psImage );
		atomic_dec( &g_sSysBase.ex_nLoadedImageCount );
	}
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static ElfImage_s *find_image( File_s *psFile )
{
	ElfImage_s *psImage;

	if ( psFile == NULL )
	{
		printk( "Panic: find_image() psFile == NULL\n" );
		return ( NULL );
	}

	for ( psImage = g_psFirstImage; psImage != NULL; psImage = psImage->im_psNext )
	{
		if ( psImage->im_psFile != NULL && CMP_INODES( psImage->im_psFile->f_psInode, psFile->f_psInode ) )
		{
			break;
		}
	}
	return ( psImage );
}


/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int load_image( const char *pzImageName, const char *pzPath, int nFile, BootModule_s * psModule, ElfImage_s **ppsRes )
{
	int nError;
	ElfImage_s *psImage = NULL;
	Elf32_ElfHdr_s sElfHdr;
	File_s *psFile = false;
	int nPathLen = 0;

	if ( pzPath != NULL )
	{
		nPathLen = strlen( pzPath ) + 1;
	}
      again:
	LOCK( g_hImageList );
	if ( psModule == NULL )
	{
		psFile = get_fd( true, nFile );

		if ( psFile == NULL )
		{
			UNLOCK( g_hImageList );
			return ( -EBADF );
		}
		if ( psFile->f_nType == FDT_DIR )
		{
			UNLOCK( g_hImageList );
			return ( -EISDIR );
		}
		if ( psFile->f_nType == FDT_INDEX_DIR || psFile->f_nType == FDT_ATTR_DIR || psFile->f_nType == FDT_SYMLINK )
		{
			UNLOCK( g_hImageList );
			return ( -EINVAL );
		}

		psImage = find_image( psFile );

		if ( NULL != psImage )
		{
			if ( atomic_read( &psImage->im_nOpenCount ) == -1 )
			{	// Image is about to being loaded
				UNLOCK( g_hImageList );
				snooze( 1000LL );
				goto again;
			}
			else
			{
				put_fd( psFile );
				atomic_inc( &psImage->im_nOpenCount );
				*ppsRes = psImage;
				UNLOCK( g_hImageList );
				return ( 0 );
			}
		}
	}
	psImage = kmalloc( sizeof( ElfImage_s ) + nPathLen + 1, MEMF_KERNEL | MEMF_CLEAR | MEMF_OKTOFAIL );

	if ( psImage == NULL )
	{
		printk( "Error: load_image() out of memory\n" );
		if ( psFile != NULL )
		{
			put_fd( psFile );
		}
		UNLOCK( g_hImageList );
		return ( -ENOMEM );
	}
	atomic_set( &psImage->im_nOpenCount, -1 );	// Tell other threads that the image is about to be loaded.
	psImage->im_psFile = psFile;
	psImage->im_psModule = psModule;

	psImage->im_psNext = g_psFirstImage;
	g_psFirstImage = psImage;

	UNLOCK( g_hImageList );

	psImage->im_pzPath = ( char * )( psImage + 1 );
	memcpy( psImage->im_pzPath, pzPath, nPathLen );
	psImage->im_pzPath[nPathLen] = '\0';

	strncpy( psImage->im_zName, pzImageName, OS_NAME_LENGTH - 1 );

	nError = read_image_data( psImage, &sElfHdr, 0, sizeof( Elf32_ElfHdr_s ) );
	if ( nError != sizeof( Elf32_ElfHdr_s ) )
	{
		printk( "Error: load_image() failed to load ELF header\n" );
		if ( nError >= 0 )
		{
			nError = -ENOEXEC;
		}
		goto error;
	}

	if ( sElfHdr.e_nSecHdrSize != sizeof( Elf32_SectionHeader_s ) )
	{
		printk( "Error: load_image() Invalid section size %d, expected %d\n", sElfHdr.e_nSecHdrSize, sizeof( Elf32_SectionHeader_s ) );
		nError = -EINVAL;
		goto error;
	}

	psImage->im_nSectionCount = sElfHdr.e_nSecHdrCount;
	psImage->im_nEntry = sElfHdr.e_nEntry;

	psImage->im_pasSections = kmalloc( sizeof( Elf32_SectionHeader_s ) * psImage->im_nSectionCount, MEMF_KERNEL | MEMF_OKTOFAIL );

	if ( psImage->im_pasSections == NULL )
	{
		printk( "Error: load_image() out of memory\n" );
		nError = -ENOMEM;
		goto error;
	}

	// Read all section headers.
	nError = read_image_data( psImage, psImage->im_pasSections, sElfHdr.e_nSecHdrOff, sElfHdr.e_nSecHdrCount * sizeof( Elf32_SectionHeader_s ) );

	if ( nError != sElfHdr.e_nSecHdrCount * sizeof( Elf32_SectionHeader_s ) )
	{
		printk( "Error: load_image() failed to load section header's\n" );
		if ( nError >= 0 )
		{
			nError = -ENOEXEC;
		}
		goto error;
	}

	nError = parse_section_headers( psImage );
	if ( nError < 0 )
	{
		goto error;
	}


	atomic_inc( &g_sSysBase.ex_nLoadedImageCount );
	atomic_set( &psImage->im_nOpenCount, 1 );

	*ppsRes = psImage;
	return ( 0 );
      error:
	unload_image( psImage );
	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static status_t memmap_instance( ElfImageInst_s *psInst, int nMode )
{
	ElfImage_s *psImage = psInst->ii_psImage;
	status_t nError = 0;
	uint32 nTextStart = -1;
	uint32 nTextEnd = -1;
	uint32 nTextOffset = 0;
	uint32 nFileTextSize;
	uint32 nMemTextSize;
	uint32 nDataStart = -1;
	uint32 nDataEnd = -1;
	uint32 nBSSEnd = -1;
	uint32 nDataOffset = 0;
	uint32 nFileDataSize;
	uint32 nMemDataSize;
	int nAllocMode;
	int bData = 0;

	int nLocking = AREA_NO_LOCK;
	uint32 nAllocAddress;
	int nAreaCount;
	uint32 anAreaSizes[3];
	uint32 anAreaOffsets[3];
	area_id anAreas[3];
	char zTextName[64];
	char zBSSName[64];
	const char *apzAreaNames[] = { zTextName, zBSSName };
	AreaInfo_s sAreaInfo;
	int i;

	if ( strcmp( psImage->im_zName, "libkernel.so" ) == 0 )
	{
		printk( "Eeek sombody tried to memmap the kernel image!!!\n" );
		return ( 0 );
	}
	strcpy( zTextName, "ro_" );
	strncpy( zTextName + 3, psImage->im_zName, 64 - 3 );
	zTextName[63] = '\0';

	strcpy( zBSSName, "rw_" );
	strncpy( zBSSName + 3, psImage->im_zName, 64 - 3 );
	zBSSName[63] = '\0';

	for ( i = 0; i < psImage->im_nSectionCount; ++i )
	{
		Elf32_SectionHeader_s *psSection = &psImage->im_pasSections[i];

		if ( psSection->sh_nType == SHT_NULL )
		{
			continue;
		}
		if ( psSection->sh_nFlags & SHF_ALLOC )
		{
			if ( ( psSection->sh_nFlags & SHF_WRITE ) == 0 && bData == 0 )
			{
				if ( nTextStart == -1 )
				{
					nTextStart = psSection->sh_nAddress;
					nTextOffset = psSection->sh_nOffset;
				}
				nTextEnd = psSection->sh_nAddress + psSection->sh_nSize - 1;
			}
			else
			{
				if ( nDataStart == -1 )
				{
					nDataStart = psSection->sh_nAddress;
					nDataOffset = psSection->sh_nOffset;
					bData = 1;	/* Ignore any non-data sections which might exist between
								   the first & last data section */
				}
				if ( psSection->sh_nType == SHT_NOBITS )
				{
					nBSSEnd = psSection->sh_nAddress + psSection->sh_nSize - 1;
					break;
				}
				else
				{
					nDataEnd = psSection->sh_nAddress + psSection->sh_nSize - 1;
					nBSSEnd = psSection->sh_nAddress + psSection->sh_nSize - 1;
				}
			}
		}
	}
	nTextOffset -= nTextStart & ~PAGE_MASK;
	nTextStart &= PAGE_MASK;
	nFileTextSize = nTextEnd - nTextStart + 1;
	nMemTextSize = ( nTextEnd - nTextStart + 1 + PAGE_SIZE - 1 ) & PAGE_MASK;

	nDataOffset -= nDataStart & ~PAGE_MASK;
	nDataStart &= PAGE_MASK;
	nFileDataSize = nDataEnd - nDataStart + 1;
	nMemDataSize = ( nBSSEnd - nDataStart + 1 + PAGE_SIZE - 1 ) & PAGE_MASK;

	if ( nTextStart + nMemTextSize > nDataStart )
	{
		printk( "memmap_instance() RO overlap RW (%08x + %08x -> %08x)\n", nTextStart, nMemTextSize, nDataStart );
		return ( -EINVAL );
	}

	if ( IM_APP_SPACE == nMode )
	{
		nAllocMode = AREA_EXACT_ADDRESS | AREA_FULL_ACCESS;
		nAllocAddress = nTextStart;
	}
	else if ( nMode == IM_KERNEL_SPACE )
	{
		nAllocMode = AREA_ANY_ADDRESS | AREA_KERNEL | AREA_FULL_ACCESS;
		nLocking = AREA_FULL_LOCK;
		nAllocAddress = 0;
	}
	else
	{
		nAllocMode = AREA_BASE_ADDRESS | AREA_FULL_ACCESS;
		nAllocAddress = AREA_FIRST_USER_ADDRESS + 512 * 1024 * 1024;
	}

	anAreaOffsets[0] = 0;
	anAreaOffsets[1] = nDataStart - nTextStart;
	anAreaSizes[0] = nMemTextSize;
	anAreaSizes[1] = nMemDataSize;
	anAreas[1] = -1;
	

	nAreaCount = ( nMemDataSize > 0 ) ? 2 : 1;

	nError = alloc_area_list( nAllocMode, nLocking, nAllocAddress, nAreaCount, apzAreaNames, anAreaOffsets, anAreaSizes, anAreas );
	if ( nError < 0 )
	{
		printk( "Error: memmap_instance() failed to alloc areas\n" );
		return ( nError );
	}

	psInst->ii_nTextArea = anAreas[0];
	psInst->ii_nDataArea = anAreas[1];

	get_area_info( psInst->ii_nTextArea, &sAreaInfo );
	psInst->ii_nTextAddress = ( uint32 )sAreaInfo.pAddress;

	if ( nMode == IM_KERNEL_SPACE )
	{
		nError = read_image_data( psImage, sAreaInfo.pAddress, nTextOffset, nFileTextSize );
		if ( nError != nFileTextSize )
		{
			printk( "Error: memmap_instance() failed to load text segment\n" );
			if ( nError >= 0 )
			{
				nError = -ENOEXEC;
			}
			goto error;
		}
		if ( nFileDataSize > 0 )
		{
			get_area_info( psInst->ii_nDataArea, &sAreaInfo );
			nError = read_image_data( psImage, sAreaInfo.pAddress, nDataOffset, nFileDataSize );
			if ( nError != nFileDataSize )
			{
				printk( "Error: memmap_instance() failed to load data segment\n" );
				if ( nError >= 0 )
				{
					nError = -ENOEXEC;
				}
				goto error;
			}
		}
	}
	else
	{
		nError = map_area_to_file( psInst->ii_nTextArea, psImage->im_psFile, AREA_FULL_ACCESS, nTextOffset, nFileTextSize );
		if ( nError < 0 )
		{
			printk( "Error: memmap_instance() failed to memmap text segment\n" );
			goto error;
		}
		if ( nFileDataSize > 0 )
		{
			nError = map_area_to_file( psInst->ii_nDataArea, psImage->im_psFile, AREA_FULL_ACCESS, nDataOffset, nFileDataSize );
			if ( nError < 0 )
			{
				printk( "Error: memmap_instance() failed to memmap text segment\n" );
				goto error;
			}
		}
	}
      error:
	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static char *get_sym_address( ElfImageInst_s *psInst, ElfSymbol_s *psSym )
{
	ElfImage_s *psImage = psInst->ii_psImage;

	if ( psInst == NULL )
	{
		printk( "PANIC : get_sym_address() psSymInst == NULL\n" );
		return ( NULL );
	}
	if ( psInst->ii_nTextArea == -1 )
	{			// Only true for kernel symbols
		return ( ( char * )psSym->s_nAddress );
	}
	return ( ( char * )( psInst->ii_nTextAddress + psSym->s_nAddress - psImage->im_nVirtualAddress ) );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int do_reloc_image( ImageContext_s * psCtx, ElfImageInst_s *psInst, int nMode, ElfImageInst_s *psParent, Elf32_Reloc_s * psRelTab, int nRelCount )
{
	ElfImage_s *psImage = psInst->ii_psImage;
	int nError;
	int i;
	bool bNeedReloc = psInst->ii_nTextAddress != psImage->im_nVirtualAddress;
	MemContext_s *psMemCtx = CURRENT_PROC->tc_psMemSeg;
	
	for ( i = 0; i < nRelCount; ++i )
	{
		Elf32_Reloc_s *psReloc = &psRelTab[i];
		ElfSymbol_s *psSym = &psImage->im_pasSymbols[ELF32_R_SYM( psReloc->r_nInfo )];
		int nRelocType = ELF32_R_TYPE( psReloc->r_nInfo );
		uint32 *pTarget;

		if ( bNeedReloc == false && ( psSym->s_nSection != SHN_UNDEF || nRelocType == R_386_RELATIVE ) )
		{
			continue;
		}
		pTarget = ( void * )( psInst->ii_nTextAddress + psReloc->r_nOffset - psImage->im_nVirtualAddress );

		if ( nMode != IM_KERNEL_SPACE && nRelocType != R_386_NONE )
		{
			pgd_t *pPgd;
			pte_t *pPte;
			uintptr_t nAddr;
			uintptr_t nStart = ( uint32 )pTarget & PAGE_MASK;
			uintptr_t nEnd = ( ( uint32 )pTarget + sizeof( *pTarget ) + PAGE_SIZE - 1 ) & PAGE_MASK;
			
			for ( nAddr = nStart; nAddr < nEnd; nAddr += PAGE_SIZE )
			{
				pPgd = pgd_offset( psMemCtx, nAddr );
				pPte = pte_offset( pPgd, nAddr );
				if ( !PTE_ISPRESENT( *pPte ) || !PTE_ISWRITE( *pPte ) )
				{
					nError = verify_mem_area( pTarget, sizeof( *pTarget ), true );
					if ( nError < 0 )
					{
						printk( "Error: do_reloc_image() failed to lock reloc target : %d\n", nError );
						return ( nError );
					}
					break;
				}
			}
		}

		nError = 0;
		switch ( nRelocType )
		{
		case R_386_NONE:
			kerndbg( KERN_DEBUG, "R_386_NONE <%s>\n", psImage->im_zName );
			break;
		case R_386_32:
			{
				ElfImageInst_s *psDllInst;
				ElfSymbol_s *psDllSym;

				nError = find_symbol( psCtx, psSym->s_pzName, &psDllInst, &psDllSym );
				if ( nError < 0 )
				{
					printk( "Could not find symbol %s\n", psSym->s_pzName );
					break;
				}
				*pTarget = *pTarget + ( ( uint32 )get_sym_address( psDllInst, psDllSym ) );
				break;
			}
		case R_386_PC32:
			{
				ElfImageInst_s *psDllInst;
				ElfSymbol_s *psDllSym;

				nError = find_symbol( psCtx, psSym->s_pzName, &psDllInst, &psDllSym );
				if ( nError < 0 )
				{
					printk( "Could not find symbol %s\n", psSym->s_pzName );
					break;
				}
				*pTarget = *pTarget + ( ( uint32 )get_sym_address( psDllInst, psDllSym ) ) - ( ( uint32 )pTarget );
				break;
			}
		case R_386_GOT32:
			printk( "R_386_GOT32\n" );
			break;
		case R_386_PLT32:
			printk( "R_386_PLT32\n" );
			break;
		case R_386_COPY:
			{
				ElfImageInst_s *psDllInst;
				ElfSymbol_s *psDllSym;

				nError = find_symbol( psCtx, psSym->s_pzName, &psDllInst, &psDllSym );
				if ( nError < 0 )
				{
					printk( "Could not find symbol %s\n", psSym->s_pzName );
					break;
				}

				if ( NULL != psDllSym )
				{
					printk( "R_386_COPY %d %d %08x - %08x\n", psDllSym->s_nSize, psSym->s_nSize, psDllSym->s_nAddress, psSym->s_nAddress );
					printk( "Copy %d bytes from %p to %p\n", psDllSym->s_nSize, get_sym_address( psDllInst, psDllSym ), pTarget );
					memcpy( pTarget, get_sym_address( psDllInst, psDllSym ), psDllSym->s_nSize );
				}
				break;
			}
		case R_386_GLOB_DATA:
			{
				ElfImageInst_s *psDllInst;
				ElfSymbol_s *psDllSym;

				nError = find_symbol( psCtx, psSym->s_pzName, &psDllInst, &psDllSym );
				if ( nError < 0 )
				{
					/* Ignore weak undefined symbols */
					if( ELF32_ST_BIND( psSym->s_nInfo ) == STB_WEAK )
					{
						*pTarget = 0;
						nError = 0;
					} else {
						printk( "Could not find symbol %s\n", psSym->s_pzName );
					}
					break;
				}
				
				*pTarget = ( uint32 )get_sym_address( psDllInst, psDllSym );
				break;
			}
		case R_386_JMP_SLOT:
			{
				ElfImageInst_s *psDllInst;
				ElfSymbol_s *psDllSym;

				nError = find_symbol( psCtx, psSym->s_pzName, &psDllInst, &psDllSym );
				if ( nError < 0 )
				{
					/* Ignore weak undefined symbols */
					if( ELF32_ST_BIND( psSym->s_nInfo ) == STB_WEAK )
					{
						*pTarget = 0;
						nError = 0;
					} else {
						printk( "Could not find symbol %s\n", psSym->s_pzName );
					}
					break;
				}
				
				*pTarget = ( uint32 )get_sym_address( psDllInst, psDllSym );
				break;
			}
		case R_386_RELATIVE:
			if ( *pTarget != 0 )
			{	// Not quite sure if this is correct, but it made C++ exceptions work...
				*pTarget = *pTarget + psInst->ii_nTextAddress - psImage->im_nVirtualAddress;
			}
			break;
		case R_386_GOTOFF:
			printk( "R_386_GOTOFF\n" );
			break;
		case R_386_GOTPC:
			printk( "R_386_GOTPC\n" );
			break;
		default:
			printk( "Error: Unknown relocation type %d\n", ELF32_R_TYPE( psReloc->r_nInfo ) );
			break;
		}
		if ( nError < 0 )
		{
			return ( nError );
		}
	}
//  printk( "%d of %d symbols in %s are used\n", nNotUsed, nRelCount, psImage->im_zName );
	return ( EOK );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int reloc_image( ImageContext_s * psCtx, ElfImageInst_s *psInst, int nMode, ElfImageInst_s *psParent )
{
	ElfImage_s *psImage = psInst->ii_psImage;
	int i;
	int nError = EOK;

	if ( psInst == NULL )
	{
		printk( "Panic: reloc_image() psInst == NULL!\n" );
		return ( -EINVAL );
	}
	if ( psImage == NULL )
	{
		printk( "Panic: reloc_image() psImage == NULL!\n" );
		return ( -EINVAL );
	}
	if ( psInst->ii_bRelocated )
	{
		return ( 0 );
	}
	psInst->ii_bRelocated = true;

	for ( i = 0; i < psImage->im_nSubImageCount; ++i )
	{
		nError = reloc_image( psCtx, psInst->ii_apsSubImages[i], nMode, psInst );
		if ( nError < 0 )
		{
			goto error;
		}
	}

	if ( psImage->im_nRelocCount > 0 )
	{
		nError = do_reloc_image( psCtx, psInst, nMode, psParent, psImage->im_pasRelocs, psImage->im_nRelocCount );
	}
      error:
	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static ElfImageInst_s *find_inst( ImageContext_s * psCtx, int nFile )
{
	File_s *psFile = get_fd( true, nFile );
	int i;

	if ( psFile == NULL )
	{
		printk( "PANIC: find_inst() psFile == NULL\n" );
		return ( NULL );
	}

	for ( i = 0; i < MAX_IMAGE_COUNT; ++i )
	{
		if ( psCtx->ic_psInstances[i] != NULL && psCtx->ic_psInstances[i]->ii_psImage->im_psFile != NULL )
		{
			if ( CMP_INODES( psCtx->ic_psInstances[i]->ii_psImage->im_psFile->f_psInode, psFile->f_psInode ) )
			{
				put_fd( psFile );
				return ( psCtx->ic_psInstances[i] );
			}
		}
	}
	put_fd( psFile );
	return ( NULL );
}

static void increase_instance_refcount( ElfImageInst_s *psInst )
{
	int i;

	atomic_inc( &psInst->ii_nOpenCount );

	for ( i = 0; i < psInst->ii_psImage->im_nSubImageCount; ++i )
	{
		increase_instance_refcount( psInst->ii_apsSubImages[i] );
	}
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static void close_image_inst( ImageContext_s * psCtx, ElfImageInst_s *psInst )
{
	ElfImage_s *psImage = psInst->ii_psImage;


	if ( psInst->ii_apsSubImages != NULL )
	{
		int i;

		for ( i = 0; i < psImage->im_nSubImageCount; ++i )
		{
			close_image_inst( psCtx, psInst->ii_apsSubImages[i] );
		}
	}

	if ( atomic_dec_and_test( &psInst->ii_nOpenCount ) )
	{
		if ( psInst->ii_apsSubImages != NULL )
		{
			kfree( psInst->ii_apsSubImages );
		}
		psCtx->ic_psInstances[psInst->ii_nHandle] = NULL;

		if ( psInst->ii_nTextArea != -1 )
		{
			delete_area( psInst->ii_nTextArea );
		}
		if ( psInst->ii_nDataArea != -1 )
		{
			delete_area( psInst->ii_nDataArea );
		}
		kfree( psInst );
		atomic_dec( &g_sSysBase.ex_nImageInstanceCount );

		unload_image( psImage );
	}
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int load_image_inst( ImageContext_s * psCtx, const char *pzPath, BootModule_s * psModule, int nMode, ElfImageInst_s **ppsInst, const char *pzLibraryPath )
{
	ElfImage_s *psImage;
	ElfImageInst_s *psInst = NULL;
	const char *pzImageName = strrchr( pzPath, '/' );
	int nFile = -1;
	Inode_s *psParentInode = NULL;
	int nError;
	int i;

	if ( nMode == IM_KERNEL_SPACE && strcmp( pzPath, "libkernel.so" ) == 0 )
	{
		atomic_inc( &g_psKernelCtx->ic_psInstances[0]->ii_nOpenCount );
		*ppsInst = g_psKernelCtx->ic_psInstances[0];
		return ( 0 );
	}

	if ( psModule == NULL )
	{
		// FIXME: We should support kernel add-ons to link agains dll's
		if ( nMode == IM_APP_SPACE )
		{
			nFile = extended_open( true, NULL, pzPath, &psParentInode, O_RDONLY, 0 );
		}
		else if ( nMode == IM_KERNEL_SPACE )
		{
			nFile = open( pzPath, O_RDONLY );
		}
		else
		{
			nFile = open_library_file( pzPath, pzLibraryPath, NULL );
			if ( nFile < 0 )
			{
				printk( "Error: Failed to open library %s\n", pzPath );
			}
		}
		if ( nFile < 0 )
		{
			nError = nFile;
			goto error1;
		}
		psInst = find_inst( psCtx, nFile );
	}
	if ( psInst != NULL )
	{
		increase_instance_refcount( psInst );
		*ppsInst = psInst;
		close( nFile );
		if ( psParentInode != NULL )
		{
			put_inode( psParentInode );	// Should never happen.
		}
		return ( 0 );
	}

	if ( pzImageName != NULL )
	{
		pzImageName++;
	}
	else
	{
		pzImageName = pzPath;
	}

	kassertw( psModule != NULL || nFile >= 0 );

	nError = load_image( pzImageName, pzPath, nFile, psModule, &psImage );

	if ( psModule == NULL )
	{
		close( nFile );
	}

	if ( nError < 0 )
	{
		goto error1;
	}

	psInst = kmalloc( sizeof( ElfImageInst_s ), MEMF_KERNEL | MEMF_CLEAR | MEMF_OKTOFAIL );

	if ( psInst == NULL )
	{
		nError = -ENOMEM;
		goto error2;
	}

	psInst->ii_nTextArea = -1;
	psInst->ii_nDataArea = -1;

	atomic_set( &psInst->ii_nOpenCount, 1 );
	psInst->ii_apsSubImages = kmalloc( psImage->im_nSubImageCount * sizeof( ElfImageInst_s * ), MEMF_KERNEL | MEMF_CLEAR | MEMF_OKTOFAIL );

	if ( psInst->ii_apsSubImages == NULL )
	{
		nError = -ENOMEM;
		goto error3;
	}

	psInst->ii_psImage = psImage;

	nError = memmap_instance( psInst, nMode );

	if ( nError < 0 )
	{
		goto error4;
	}

	psInst->ii_nHandle = -1;
	for ( i = 0; i < MAX_IMAGE_COUNT; ++i )
	{
		if ( psCtx->ic_psInstances[i] == NULL )
		{
			psCtx->ic_psInstances[i] = psInst;
			psInst->ii_nHandle = i;
			break;
		}
	}
	if ( psInst->ii_nHandle < 0 )
	{
		printk( "load_image_inst(): too many open images\n" );
		nError = -EMFILE;
		goto error5;
	}

	if ( nMode == IM_APP_SPACE )
	{
		Process_s *psProc = CURRENT_PROC;

		if ( psProc->pr_psIoContext->ic_psBinDir != NULL )
		{
			put_inode( psProc->pr_psIoContext->ic_psBinDir );
		}
		psProc->pr_psIoContext->ic_psBinDir = psParentInode;
		psParentInode = NULL;
	}

	for ( i = 0; i < psImage->im_nSubImageCount; ++i )
	{
		if ( nMode == IM_KERNEL_SPACE )
		{
			nError = load_image_inst( psCtx, psImage->im_apzSubImages[i], NULL, IM_KERNEL_SPACE, &psInst->ii_apsSubImages[i], pzLibraryPath );
		}
		else
		{
			nError = load_image_inst( psCtx, psImage->im_apzSubImages[i], NULL, IM_LIBRARY_SPACE, &psInst->ii_apsSubImages[i], pzLibraryPath );
		}
		if ( nError < 0 )
		{
			int j;

			for ( j = 0; j < i; ++j )
			{
				close_image_inst( psCtx, psInst->ii_apsSubImages[j] );
			}
			goto error6;
		}
	}
	if ( nMode != IM_LIBRARY_SPACE )
	{
		nError = reloc_image( psCtx, psInst, nMode, ( nMode == IM_ADDON_SPACE ) ? psCtx->ic_psInstances[0] : NULL );
		if ( nError < 0 )
		{
			goto error7;
		}
	}
	*ppsInst = psInst;

	atomic_inc( &g_sSysBase.ex_nImageInstanceCount );

	return ( 0 );
      error7:
	for ( i = 0; i < psImage->im_nSubImageCount; ++i )
	{
		close_image_inst( psCtx, psInst->ii_apsSubImages[i] );
	}
      error6:
	psCtx->ic_psInstances[psInst->ii_nHandle] = NULL;
      error5:
	if ( psInst->ii_nTextArea != -1 )
	{
		delete_area( psInst->ii_nTextArea );
	}
	if ( psInst->ii_nDataArea != -1 )
	{
		delete_area( psInst->ii_nDataArea );
	}
      error4:
	kfree( psInst->ii_apsSubImages );
      error3:
	kfree( psInst );
      error2:
	unload_image( psImage );
      error1:
	if ( psParentInode != NULL )
	{
		put_inode( psParentInode );
	}
	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

ImageContext_s *create_image_ctx( void )
{
	ImageContext_s *psCtx;

	psCtx = kmalloc( sizeof( ImageContext_s ), MEMF_KERNEL | MEMF_CLEAR | MEMF_OKTOFAIL );

	if ( psCtx == NULL )
	{
		printk( "create_image_ctx() out of memory\n" );
		return ( NULL );
	}
	psCtx->ic_hLock = create_semaphore( "proc_images", 1, SEM_RECURSIVE );
	if ( psCtx->ic_hLock < 0 )
	{
		kfree( psCtx );
		return ( NULL );
	}
	return ( psCtx );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

void delete_image_context( ImageContext_s * psCtx )
{
	delete_semaphore( psCtx->ic_hLock );
	kfree( psCtx );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

void close_all_images( ImageContext_s * psCtx )
{
	int i;

	LOCK( psCtx->ic_hLock );
	for ( i = 0; i < MAX_IMAGE_COUNT; ++i )
	{
		if ( psCtx->ic_psInstances[i] != NULL )
		{
			unload_image( psCtx->ic_psInstances[i]->ii_psImage );

			kfree( psCtx->ic_psInstances[i]->ii_apsSubImages );
			kfree( psCtx->ic_psInstances[i] );
			psCtx->ic_psInstances[i] = NULL;
			atomic_dec( &g_sSysBase.ex_nImageInstanceCount );
		}
	}
	UNLOCK( psCtx->ic_hLock );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

void clone_image_inst( ElfImageInst_s *psDst, ElfImageInst_s *psSrc, MemContext_s *psNewMemCtx )
{
	*psDst = *psSrc;

	psDst->ii_apsSubImages = NULL;

	atomic_inc( &psDst->ii_psImage->im_nOpenCount );

	psDst->ii_nTextArea = -1;
	if ( psSrc->ii_nTextArea != -1 )
	{
		AreaInfo_s sInfo;
		MemArea_s *psArea;

		get_area_info( psSrc->ii_nTextArea, &sInfo );
		psArea = get_area( psNewMemCtx, ( uint32 )sInfo.pAddress );

		if ( psArea != NULL )
		{
			psDst->ii_nTextArea = psArea->a_nAreaID;
			put_area( psArea );
		}
	}
	psDst->ii_nDataArea = -1;
	if ( psSrc->ii_nDataArea != -1 )
	{
		AreaInfo_s sInfo;
		MemArea_s *psArea;

		get_area_info( psSrc->ii_nDataArea, &sInfo );
		psArea = get_area( psNewMemCtx, ( uint32 )sInfo.pAddress );

		if ( psArea != NULL )
		{
			psDst->ii_nDataArea = psArea->a_nAreaID;
			put_area( psArea );
		}
	}
}

ImageContext_s *clone_image_context( ImageContext_s * psOrig, MemContext_s *psNewMemCtx )
{
	ImageContext_s *psNewCtx;
	int i;

	if ( psOrig == NULL )
	{
		printk( "Error: clone_image_context(): psOrig == NULL\n" );
		return ( NULL );
	}

	psNewCtx = create_image_ctx();

	if ( psNewCtx == NULL )
	{
		return ( NULL );
	}

	LOCK( psOrig->ic_hLock );
	for ( i = 0; i < MAX_IMAGE_COUNT; ++i )
	{
		if ( psOrig->ic_psInstances[i] != NULL )
		{
			ElfImageInst_s *psSrc = psOrig->ic_psInstances[i];
			ElfImageInst_s *psDst;
			psDst = kmalloc( sizeof( ElfImageInst_s ), MEMF_KERNEL | MEMF_OKTOFAIL );

			if ( psDst == NULL )
			{
				int j;

				printk( "clone_image_context() no memory for image instance\n" );
				for ( j = i - 1; j >= 0; --j )
				{
					if ( psNewCtx->ic_psInstances[j] != NULL )
					{
						kfree( psNewCtx->ic_psInstances[j] );
						atomic_dec( &g_sSysBase.ex_nImageInstanceCount );
					}
				}
				delete_image_context( psNewCtx );
				psNewCtx = NULL;
				break;
			}
			psDst->ii_nTextArea = -1;
			psDst->ii_nDataArea = -1;

			psNewCtx->ic_psInstances[i] = psDst;

			clone_image_inst( psDst, psSrc, psNewMemCtx );
			atomic_inc( &g_sSysBase.ex_nImageInstanceCount );
		}
	}
	if ( psNewCtx != NULL )
	{
		for ( i = 0; i < MAX_IMAGE_COUNT; ++i )
		{
			if ( psOrig->ic_psInstances[i] != NULL )
			{
				ElfImageInst_s *psSrc = psOrig->ic_psInstances[i];
				ElfImageInst_s *psDst = psNewCtx->ic_psInstances[i];
				int j;

				psDst->ii_apsSubImages = kmalloc( psDst->ii_psImage->im_nSubImageCount * sizeof( ElfImageInst_s * ), MEMF_KERNEL | MEMF_CLEAR | MEMF_OKTOFAIL );
				if ( psDst->ii_apsSubImages == NULL )
				{
					for ( j = 0; j < MAX_IMAGE_COUNT; ++j )
					{
						if ( psNewCtx->ic_psInstances[j] != NULL )
						{
							if ( psNewCtx->ic_psInstances[j]->ii_apsSubImages != NULL )
							{
								kfree( psNewCtx->ic_psInstances[j]->ii_apsSubImages );
							}
							atomic_dec( &psNewCtx->ic_psInstances[j]->ii_psImage->im_nOpenCount );
							kfree( psNewCtx->ic_psInstances[j] );
							atomic_dec( &g_sSysBase.ex_nImageInstanceCount );
						}
					}
					delete_image_context( psNewCtx );
					psNewCtx = NULL;
					break;
				}
				for ( j = 0; j < psDst->ii_psImage->im_nSubImageCount; ++j )
				{
					psDst->ii_apsSubImages[j] = psNewCtx->ic_psInstances[psSrc->ii_apsSubImages[j]->ii_nHandle];
				}
			}
		}
	}
	UNLOCK( psOrig->ic_hLock );
	return ( psNewCtx );
}

static int do_find_module_by_address( ImageContext_s * psCtx, uint32 nAddress )
{
	ElfImageInst_s *psInst = NULL;
	AreaInfo_s sAreaInfo;
	int i;
	int nModule = -ENOENT;

	LOCK( psCtx->ic_hLock );
	for ( i = 0; i < MAX_IMAGE_COUNT; ++i )
	{
		psInst = psCtx->ic_psInstances[i];
		if ( psInst == NULL )
		{
			continue;
		}
		if ( psInst->ii_nTextArea != -1 && get_area_info( psInst->ii_nTextArea, &sAreaInfo ) >= 0 && nAddress >= ( ( uint32 )sAreaInfo.pAddress ) && nAddress < ( ( uint32 )sAreaInfo.pAddress ) + sAreaInfo.nSize )
		{
			break;
		}
		if ( psInst->ii_nDataArea != -1 && get_area_info( psInst->ii_nDataArea, &sAreaInfo ) >= 0 && nAddress >= ( ( uint32 )sAreaInfo.pAddress ) && nAddress < ( ( uint32 )sAreaInfo.pAddress ) + sAreaInfo.nSize )
		{
			break;
		}
		psInst = NULL;
	}
	if ( psInst != NULL )
	{
		nModule = i;
	}
	UNLOCK( psCtx->ic_hLock );
	return ( nModule );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int find_module_by_address( const void *pAddress )
{
	ImageContext_s *psCtx;

	if ( ( ( uint32 )pAddress ) < AREA_FIRST_USER_ADDRESS )
	{
		psCtx = g_psKernelCtx;
	}
	else
	{
		psCtx = CURRENT_PROC->pr_psImageContext;
	}
	if ( psCtx == NULL )
	{
		return ( -EINVAL );
	}
	return ( do_find_module_by_address( psCtx, ( uint32 )pAddress ) );
}

int sys_find_module_by_address( const void *pAddress )
{
	return ( do_find_module_by_address( CURRENT_PROC->pr_psImageContext, ( uint32 )pAddress ) );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int get_symbol_by_address( int nLibrary, const char *pAddress, char *pzName, int nMaxNamLen, void **ppAddress )
{
	ImageContext_s *psCtx;
	
	if ( ( ( uint32 )pAddress ) < AREA_FIRST_USER_ADDRESS )
	{
		psCtx = g_psKernelCtx;
	}
	else
	{
		psCtx = CURRENT_PROC->pr_psImageContext;
	}
	
	ElfImageInst_s *psInst;
	ElfImage_s *psImage;
	ElfSymbol_s *psClosestSymbol = NULL;
	uint32 nClosest = 0xffffffff;
	int i;

	if ( nLibrary < 0 || nLibrary >= MAX_IMAGE_COUNT )
	{
		return ( -EINVAL );
	}
	LOCK( psCtx->ic_hLock );
	psInst = psCtx->ic_psInstances[nLibrary];

	if ( psInst == NULL )
	{
		UNLOCK( psCtx->ic_hLock );
		return ( -EINVAL );	// FIXME: Should may return someting like EBADF instead?
	}

	psImage = psInst->ii_psImage;

	for ( i = psImage->im_nSymCount - 1; i >= 0; --i )
	{
		ElfSymbol_s *psSymbol = &psImage->im_pasSymbols[i];
		char *pSymAddr = get_sym_address( psInst, psSymbol );

		if ( pSymAddr > pAddress )
		{
			continue;
		}
		if ( pSymAddr == pAddress )
		{
			psClosestSymbol = psSymbol;
			*ppAddress = pSymAddr;
			break;
		}
		if ( pAddress - pSymAddr < nClosest )
		{
			nClosest = pAddress - pSymAddr;
			psClosestSymbol = psSymbol;
			*ppAddress = pSymAddr;
		}
	}
	UNLOCK( psCtx->ic_hLock );

	if ( psClosestSymbol == NULL )
	{
		return ( -ENOSYM );
	}
	strncpy( pzName, psClosestSymbol->s_pzName, nMaxNamLen );
	pzName[nMaxNamLen - 1] = '\0';
	return ( strlen( pzName ) );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int sys_load_library( const char *a_pzPath, uint32 nFlags, const char *pzSearchPath )
{
	ImageContext_s *psCtx = CURRENT_PROC->pr_psImageContext;
	ElfImageInst_s *psInst;
	char *pzPath;
	int nError;

	nError = strndup_from_user( a_pzPath, PATH_MAX, &pzPath );
	if ( nError < 0 )
	{
		return ( nError );
	}
	LOCK( psCtx->ic_hLock );
	nError = load_image_inst( psCtx, pzPath, NULL, IM_ADDON_SPACE, &psInst, pzSearchPath );

	kfree( pzPath );

	if ( nError >= 0 )
	{
		atomic_inc( &psInst->ii_nAppOpenCount );
		nError = psInst->ii_nHandle;
	}
	UNLOCK( psCtx->ic_hLock );
	return ( nError );
}

int sys_old_load_library( const char *pzPath, uint32 nFlags )
{
	printk( "Warning: Obsolete version of load_library() called. Upgrade GLIBC\n" );
	return ( sys_load_library( pzPath, nFlags, NULL ) );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int sys_unload_library( int nLibrary )
{
	ImageContext_s *psCtx = CURRENT_PROC->pr_psImageContext;
	ElfImageInst_s *psInst;
	int nError = 0;

	if ( nLibrary < 0 || nLibrary >= MAX_IMAGE_COUNT )
	{
		return ( -EINVAL );
	}
	LOCK( psCtx->ic_hLock );

	psInst = psCtx->ic_psInstances[nLibrary];

	if ( psInst == NULL )
	{
		nError = -EINVAL;	// FIXME: Should may return someting like EBADF instead?
		goto error;
	}
	if ( atomic_read( &psInst->ii_nAppOpenCount ) == 0 )
	{
		printk( "sys_unload_library() attempt to unload non load_library() loaded image %s\n", psInst->ii_psImage->im_zName );
		nError = -EINVAL;
		goto error;
	}
	atomic_dec( &psInst->ii_nAppOpenCount );
	close_image_inst( psCtx, psInst );
      error:
	UNLOCK( psCtx->ic_hLock );
	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int do_get_symbol_address( ImageContext_s * psCtx, int nLibrary, const char *pzName, int nIndex, void **pPtr )
{
	ElfImageInst_s *psInst;
	ElfSymbol_s *psSymbol;
	int nError = 0;

	if ( nLibrary < 0 || nLibrary >= MAX_IMAGE_COUNT )
	{
		return ( -EINVAL );
	}
	LOCK( psCtx->ic_hLock );
	psInst = psCtx->ic_psInstances[nLibrary];

	if ( psInst == NULL )
	{
		nError = -EINVAL;	// FIXME: Should may return someting like EBADF instead?
		goto error;
	}
	if ( nIndex < 0 )
	{
		psSymbol = lookup_symbol( psInst->ii_psImage, pzName );
	}
	else
	{
		if ( nIndex < psInst->ii_psImage->im_nSymCount )
		{
			psSymbol = &psInst->ii_psImage->im_pasSymbols[nIndex];
		}
		else
		{
			psSymbol = NULL;
		}
	}

	if ( psSymbol == NULL )
	{
		nError = -ENOSYM;
		goto error;
	}
	*pPtr = get_sym_address( psInst, psSymbol );
      error:
	UNLOCK( psCtx->ic_hLock );
	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int sys_get_symbol_address( int nLibrary, const char *pzName, int nIndex, void **pPtr )
{
	return ( do_get_symbol_address( CURRENT_PROC->pr_psImageContext, nLibrary, pzName, nIndex, pPtr ) );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int get_symbol_address( int nLibrary, const char *pzName, int nIndex, void **pPtr )
{
	return ( do_get_symbol_address( g_psKernelCtx, nLibrary, pzName, nIndex, pPtr ) );
}

Inode_s *get_image_inode( int nLibrary )
{
	ImageContext_s *psCtx = CURRENT_PROC->pr_psImageContext;	// FIXME: Should also consider kernel images
	ElfImageInst_s *psInst;
	Inode_s *psInode = NULL;

	if ( nLibrary < 0 || nLibrary >= MAX_IMAGE_COUNT )
	{
		return ( NULL );
	}
	LOCK( psCtx->ic_hLock );
	psInst = psCtx->ic_psInstances[nLibrary];

	if ( psInst == NULL )
	{
		UNLOCK( psCtx->ic_hLock );
		return ( NULL );	// FIXME: Should may return someting like EBADF instead?
	}
	if ( psInst->ii_psImage->im_psFile != NULL )
	{
		psInode = psInst->ii_psImage->im_psFile->f_psInode;
	}
	if ( psInode != NULL )
	{
		atomic_inc( &psInode->i_nCount );
	}
	UNLOCK( psCtx->ic_hLock );
	return ( psInode );
}


/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int load_kernel_driver( const char *a_pzPath )
{
	ImageContext_s *psCtx = g_psKernelCtx;
	ElfImageInst_s *psInst;
	char *pzPath;
	int nError;
	int i;

	if ( g_bRootFSMounted )
	{
		nError = normalize_path( a_pzPath, &pzPath );
		if ( nError < 0 )
		{
			return ( nError );
		}
	}
	else
	{
		pzPath = ( char * )a_pzPath;
	}

	LOCK( psCtx->ic_hLock );
	for ( i = 0; i < MAX_IMAGE_COUNT; ++i )
	{
		if ( psCtx->ic_psInstances[i] != NULL && strcmp( pzPath, psCtx->ic_psInstances[i]->ii_psImage->im_pzPath ) == 0 )
		{
			atomic_inc( &psCtx->ic_psInstances[i]->ii_nAppOpenCount );
			nError = i;
			goto done;
		}
	}

	nError = load_image_inst( psCtx, pzPath, NULL, IM_KERNEL_SPACE, &psInst, NULL );

	if ( nError >= 0 )
	{
		atomic_inc( &psInst->ii_nAppOpenCount );
		nError = psInst->ii_nHandle;
	}
      done:
	UNLOCK( psCtx->ic_hLock );
	if ( pzPath != a_pzPath )
	{
		kfree( pzPath );
	}
	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int unload_kernel_driver( int nLibrary )
{
	ImageContext_s *psCtx = g_psKernelCtx;
	ElfImageInst_s *psInst;
	int nError = 0;

	LOCK( psCtx->ic_hLock );

	if ( nLibrary < 0 || nLibrary >= MAX_IMAGE_COUNT )
	{
		printk( "Error: unload_kernel_driver:1() invalid image handle %d\n", nLibrary );
		nError = -EINVAL;
		goto error;
	}
	psInst = psCtx->ic_psInstances[nLibrary];

	if ( psInst == NULL )
	{
		printk( "Error: unload_kernel_driver:2() invalid image handle %d\n", nLibrary );
		nError = -EINVAL;	// FIXME: Should may return someting like EBADF instead?
		goto error;
	}
	if ( atomic_read( &psInst->ii_nAppOpenCount ) == 0 )
	{
		printk( "unload_kernel_driver() attempt to unload non load_kernel_driver() loaded image %s\n", psInst->ii_psImage->im_zName );
		nError = -EINVAL;
		goto error;
	}
	atomic_dec( &psInst->ii_nAppOpenCount );
	close_image_inst( psCtx, psInst );
      error:
	UNLOCK( psCtx->ic_hLock );
	return ( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static void do_get_image_info( image_info * psInfo, ElfImageInst_s *psInst )
{
	ElfImage_s *psImage = psInst->ii_psImage;
	uint32 nPhysAddr = psInst->ii_nTextAddress;
	AreaInfo_s sTextInfo;
	AreaInfo_s sDataInfo;

	strcpy( psInfo->ii_name, psImage->im_zName );

	if ( psImage->im_nEntry != 0 )
	{
		psInfo->ii_entry_point = ( image_entry * )( nPhysAddr + psImage->im_nEntry - psImage->im_nVirtualAddress );
	}
	else
	{
		psInfo->ii_entry_point = NULL;
	}
	if ( psImage->im_nInit != 0 )
	{
		psInfo->ii_init = ( image_init * )( nPhysAddr + psImage->im_nInit - psImage->im_nVirtualAddress );
	}
	else
	{
		psInfo->ii_init = NULL;
	}
	if ( psImage->im_nFini != 0 )
	{
		psInfo->ii_fini = ( image_term * )( nPhysAddr + psImage->im_nFini - psImage->im_nVirtualAddress );
	}
	else
	{
		psInfo->ii_fini = NULL;
	}

	get_area_info( psInst->ii_nTextArea, &sTextInfo );
	get_area_info( psInst->ii_nDataArea, &sDataInfo );

	psInfo->ii_image_id = psInst->ii_nHandle;
	psInfo->ii_type = IM_APP_SPACE;
	psInfo->ii_open_count = atomic_read( &psInst->ii_nOpenCount );
	psInfo->ii_sub_image_count = psImage->im_nSubImageCount;
	psInfo->ii_init_order = 0;
	if ( psImage->im_psFile != NULL )
	{
		psInfo->ii_device = psImage->im_psFile->f_psInode->i_psVolume->v_nDevNum;
		psInfo->ii_inode = psImage->im_psFile->f_psInode->i_nInode;
	}
	else
	{
		psInfo->ii_device = 0;
		psInfo->ii_inode = 0;
	}
	psInfo->ii_text_addr = sTextInfo.pAddress;
	psInfo->ii_data_addr = sDataInfo.pAddress;
	psInfo->ii_text_size = sTextInfo.nSize;
	psInfo->ii_data_size = sDataInfo.nSize;
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int get_image_info( bool bKernel, int nImage, int nSubImage, image_info * psInfo )
{
	ImageContext_s *psCtx;
	ElfImageInst_s *psInst;
	int nError = 0;

	if ( bKernel )
	{
		psCtx = g_psKernelCtx;
	}
	else
	{
		psCtx = CURRENT_PROC->pr_psImageContext;
	}
	LOCK( psCtx->ic_hLock );

	if ( nImage < 0 || nImage >= MAX_IMAGE_COUNT || psCtx->ic_psInstances[nImage] == NULL )
	{
		nError = -EINVAL;
		goto error;
	}

	psInst = psCtx->ic_psInstances[nImage];

	if ( nSubImage >= 0 )
	{
		if ( nSubImage >= psInst->ii_psImage->im_nSubImageCount )
		{
			nError = -EINVAL;
			goto error;
		}
		else
		{
			psInst = psInst->ii_apsSubImages[nSubImage];
		}
	}
	do_get_image_info( psInfo, psInst );

      error:
	UNLOCK( psCtx->ic_hLock );
	return ( nError );
}

int sys_get_image_info( int nImage, int nSubImage, image_info * psInfo )
{
	int nError;

	if ( verify_mem_area( psInfo, sizeof( *psInfo ), true ) < 0 )
	{
		return ( -EFAULT );
	}
	nError = get_image_info( false, nImage, nSubImage, psInfo );
	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

void db_list_process_images( Process_s *psProc )
{
	ImageContext_s *psCtx = psProc->pr_psImageContext;
	int i;

	dbprintf( DBP_DEBUGGER, "ID OC LC PADDRESS VADDRESS SCNT RCNT NAME\n" );

	for ( i = 0; i < MAX_IMAGE_COUNT; ++i )
	{
		if ( psCtx->ic_psInstances[i] != NULL )
		{
			ElfImageInst_s *psInst = psCtx->ic_psInstances[i];
			ElfImage_s *psImage = psInst->ii_psImage;

			dbprintf( DBP_DEBUGGER, "%03d %02d %02d %08x %08x %04d %04d %s\n", i, atomic_read( &psInst->ii_nOpenCount ), atomic_read( &psInst->ii_nAppOpenCount ), psInst->ii_nTextAddress, psImage->im_nVirtualAddress, psImage->im_nSymCount, psImage->im_nRelocCount, psImage->im_zName );
		}
	}
	dbprintf( DBP_DEBUGGER, "\n" );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

void db_list_images( int argc, char **argv )
{
	if ( argc == 1 )
	{
		proc_id hProc;

		for ( hProc = 0; hProc != -1; hProc = sys_get_next_proc( hProc ) )
		{
			Process_s *psProc = get_proc_by_handle( hProc );

			if ( NULL != psProc )
			{
				dbprintf( DBP_DEBUGGER, "Image list for %d - %s\n", hProc, psProc->tc_zName );
				db_list_process_images( psProc );
			}
		}
	}
	else if ( argc == 2 )
	{
		proc_id hProc = atol( argv[1] );
		Process_s *psProc = get_proc_by_handle( hProc );

		if ( NULL != psProc )
		{
			dbprintf( DBP_DEBUGGER, "Image list for %d - %s\n", hProc, psProc->tc_zName );
			db_list_process_images( psProc );
		}
		else
		{
			dbprintf( DBP_DEBUGGER, "No process with ID %d\n", hProc );
		}
	}
}

static void init_boot_modules( void )
{
	int i;

	printk( "Init boot modules:\n" );
	for ( i = 0; i < g_sSysBase.ex_nBootModuleCount; ++i )
	{
		BootModule_s *psModule = g_sSysBase.ex_asBootModules + i;
		ElfImageInst_s *psInst;
		char zFullPath[1024];
		const char *pzPath;
		int nError;
		int j;

		pzPath = strstr( psModule->bm_pzModuleArgs, "/drivers/" );
		if ( pzPath == NULL )
		{
			continue;
		}
		strcpy( zFullPath, "/boot/system" );
		j = strlen( zFullPath );
		while ( *pzPath != '\0' && isspace( *pzPath ) == false )
		{
			zFullPath[j++] = *pzPath++;
		}
		zFullPath[j] = '\0';
		while ( isspace( *pzPath ) )
			pzPath++;

		if ( *pzPath != '\0' )
		{
			const char *pzArg = pzPath;

			for ( j = 0;; ++j )
			{
				int nLen;

				if ( pzPath[j] != '\0' && isspace( pzPath[j] ) == false )
				{
					continue;
				}
				nLen = pzPath + j - pzArg;
				get_str_arg( zFullPath, "path=", pzArg, nLen );

				if ( pzPath[j] == '\0' )
				{
					break;
				}
				pzArg = pzPath + j + 1;
			}
		}


		printk( "  Init boot module '%s'\n", zFullPath );
		nError = load_image_inst( g_psKernelCtx, zFullPath, psModule, IM_KERNEL_SPACE, &psInst, NULL );
		if ( nError < 0 )
		{
			printk( "  Error: init_boot_modules() failed to initialize %s\n", zFullPath );
			continue;
		}
		atomic_inc( &psInst->ii_nAppOpenCount );
	}
	for ( i = 0; i < MAX_IMAGE_COUNT; ++i )
	{
		if ( g_psKernelCtx->ic_psInstances[i] != NULL && strstr( g_psKernelCtx->ic_psInstances[i]->ii_psImage->im_pzPath, "/drivers/dev/" ) != NULL )
		{
			init_boot_device( g_psKernelCtx->ic_psInstances[i]->ii_psImage->im_pzPath );
		}
	}
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

void init_elf_loader( void )
{
	register_debug_cmd( "ls_images", "list the images loaded by a process", db_list_images );

	g_hImageList = create_semaphore( "image_list", 1, SEM_RECURSIVE );
	kassertw( g_hImageList >= 0 );
	create_kernel_image();
	init_boot_modules();
}

