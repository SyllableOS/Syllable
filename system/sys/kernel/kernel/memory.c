
/*
 *  The Syllable kernel
 *  Real memory heap manager
 *  Copyright (C) 2003 The Syllable Team
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

#include <atheos/types.h>
#include <atheos/kernel.h>
#include <atheos/spinlock.h>
#include <atheos/kernel.h>
#include <atheos/resource.h>
#include <posix/errno.h>

#include <macros.h>

#include "inc/sysbase.h"
#include "inc/mman.h"


SPIN_LOCK( g_sRealPoolLock, "real_pool_slock" );

static MemHeader_s g_sRealMemHdr;	/* Real memory  (mem below 1M)          */


//****************************************************************************/
/** Allocates memory for a new memory heap.
 * \internal
 * \ingroup Memory
 * \param psHeader the memory pool to use.
 * \param pnAddress a pointer to the address of the memory to use; only used if
 *     <i>bExactAddress</i> is true.  Will be set to the <b>real</b> address if
 *     the call is successful.
 * \param bExactAddress <code>true</code> to use <i>pnAddress</i>.
 * \param bHighest if <code>true</code> then the memory at the highest address
 *     will be returned.
 * \param nSize the size in bytes to allocate.
 * \return <code>0</code> if successful.
 * \sa FreeHeapMem()
 * \author Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
status_t AllocateHeapMem( MemHeader_s *psHeader, uint32 *pnAddress, bool bExactAddress, bool bHighest, uint32 nSize )
{
	MemChunk_s *psChunk = psHeader->mh_psFirst;
	MemChunk_s *psValidChunk = NULL;

	if ( pnAddress == NULL )
	{
		printk( "Error: AllocateHeapMem() called with invalid address parameter\n" );
		return ( -ENOMEM );
	}

	/* We do not want to search if it is useless */
	if ( nSize > psHeader->mh_nTotalSize )
		return ( -ENOMEM );

	/* Try to find a matching chunk */
	while ( psChunk != NULL )
	{
		/* Found it! */
		if ( !bExactAddress && !psChunk->mc_bUsed && ( psChunk->mc_nSize == nSize || psChunk->mc_nSize > nSize ) )
		{
			psValidChunk = psChunk;
			if ( !bHighest )
				break;
		}
		else if ( bExactAddress && !psChunk->mc_bUsed && psChunk->mc_nAddress <= *pnAddress && ( ( psChunk->mc_nAddress + psChunk->mc_nSize ) >= ( *pnAddress + nSize ) ) )
		{
			//printk( "Using Chunk @ %x %x %i\n", (uint)psChunk->mc_nAddress, (uint)psChunk->mc_nSize,
			//                                                              psChunk->mc_bUsed );
			psValidChunk = psChunk;
			if ( !bHighest )
				break;
		}

		psChunk = psChunk->mc_psNext;
	}

	/* Nothing found */
	if ( psValidChunk == NULL )
		return ( -ENOMEM );

	if ( psValidChunk->mc_nSize == nSize )
	{
		/* Mark the chunk as used  */
		psValidChunk->mc_bUsed = true;
		*pnAddress = psValidChunk->mc_nAddress;
	}
	else
	{
		if ( !bExactAddress )
		{
			/* Split the chunk */
			MemChunk_s *psNewChunk = kmalloc( sizeof( MemChunk_s ), MEMF_KERNEL | MEMF_NOBLOCK );

			if ( !psNewChunk )
			{
				printk( "Error: AllocateHeapMem() failed to allocate memory\n" );
				return ( -ENOMEM );
			}

			memset( psNewChunk, 0, sizeof( MemChunk_s ) );

			/* Fill data */
			psNewChunk->mc_psNext = psValidChunk->mc_psNext;
			psNewChunk->mc_nAddress = psValidChunk->mc_nAddress + nSize;
			psNewChunk->mc_nSize = psValidChunk->mc_nSize - nSize;

			psValidChunk->mc_psNext = psNewChunk;
			psValidChunk->mc_bUsed = true;
			psValidChunk->mc_nSize = nSize;

			*pnAddress = psValidChunk->mc_nAddress;
		}
		else
		{
			if ( psValidChunk->mc_nAddress != *pnAddress )
			{
				/* Split the chunk */
				MemChunk_s *psNewChunk = kmalloc( sizeof( MemChunk_s ), MEMF_KERNEL | MEMF_NOBLOCK );

				if ( !psNewChunk )
				{
					printk( "Error: AllocateHeapMem() failed to allocate memory\n" );
					return ( -ENOMEM );
				}

				memset( psNewChunk, 0, sizeof( MemChunk_s ) );

				/* Fill data */
				psNewChunk->mc_psNext = psValidChunk->mc_psNext;
				psNewChunk->mc_nAddress = psValidChunk->mc_nAddress + ( *pnAddress - psValidChunk->mc_nAddress );
				psNewChunk->mc_nSize = psValidChunk->mc_nSize - ( *pnAddress - psValidChunk->mc_nAddress );

				psValidChunk->mc_psNext = psNewChunk;
				psValidChunk->mc_nSize = *pnAddress - psValidChunk->mc_nAddress;

				psValidChunk = psNewChunk;
			}
			if ( ( psValidChunk->mc_nAddress + psValidChunk->mc_nSize ) != ( *pnAddress + nSize ) )
			{
				/* Split the chunk */
				uint32 nNewSize = psValidChunk->mc_nSize - ( ( psValidChunk->mc_nAddress + psValidChunk->mc_nSize ) - ( *pnAddress + nSize ) );
				MemChunk_s *psNewChunk = kmalloc( sizeof( MemChunk_s ), MEMF_KERNEL | MEMF_NOBLOCK );

				if ( !psNewChunk )
				{
					printk( "Error: AllocateHeapMem() failed to allocate memory\n" );
					return ( -ENOMEM );
				}

				memset( psNewChunk, 0, sizeof( MemChunk_s ) );

				/* Fill data */
				psNewChunk->mc_psNext = psValidChunk->mc_psNext;
				psNewChunk->mc_nAddress = psValidChunk->mc_nAddress + nNewSize;
				psNewChunk->mc_nSize = psValidChunk->mc_nSize - nNewSize;

				psValidChunk->mc_psNext = psNewChunk;
				psValidChunk->mc_nSize = nNewSize;
			}
			psValidChunk->mc_bUsed = true;
		}
	}


	return ( 0 );
}

void DumpHeapMem( MemHeader_s *psHeader )
{
	MemChunk_s *psChunk = psHeader->mh_psFirst;

	while ( psChunk != NULL )
	{
		printk( "%x --> %x %s\n", ( uint )psChunk->mc_nAddress, ( uint )psChunk->mc_nAddress + ( uint )psChunk->mc_nSize, psChunk->mc_bUsed ? "used" : "unused" );
		psChunk = psChunk->mc_psNext;
	}
}

//****************************************************************************/
/* Tries to defragment a memory heap.
 * \internal
 * \ingroup Memory
 * \param psHeader the memory pool to defragment.
 * \author Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
void DefragmentHeapMem( MemHeader_s *psHeader )
{
	MemChunk_s *psChunk = psHeader->mh_psFirst;

	/* Try to connect two unused chunks */
	while ( psChunk != NULL )
	{
		if ( psChunk->mc_psNext != NULL )
		{
			if ( !psChunk->mc_bUsed && !psChunk->mc_psNext->mc_bUsed )
			{
				MemChunk_s *psDeleteChunk = psChunk->mc_psNext;

				/* Test if heap is valid */
				if ( psChunk->mc_nAddress + psChunk->mc_nSize != psChunk->mc_psNext->mc_nAddress )
				{
					printk( "Error: Memory heap corrupted\n" );
					return;
				}

				psChunk->mc_nSize += psChunk->mc_psNext->mc_nSize;
				psChunk->mc_psNext = psChunk->mc_psNext->mc_psNext;
				kfree( psDeleteChunk );
			}
		}

		psChunk = psChunk->mc_psNext;
	}
}

//****************************************************************************/
/** Frees memory from the specified memory heap.
 * \internal
 * \ingroup Memory
 * \param psHeader the memory pool to use.
 * \param nAddress the address of the memory to free.
 * \return <code>0</code> if successful.
 * \sa AllocateHeapMem()
 * \author Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
status_t FreeHeapMem( MemHeader_s *psHeader, uint32 nAddress )
{
	MemChunk_s *psChunk = psHeader->mh_psFirst;

	/* Try to find the matching chunk */
	while ( psChunk != NULL )
	{
		if ( psChunk->mc_nAddress == nAddress )
		{
			if ( !psChunk->mc_bUsed )
			{
				printk( "Warning: FreeHeapMem() called to free not used memory\n" );
				return ( -EFAULT );
			}

			psChunk->mc_bUsed = false;
			DefragmentHeapMem( psHeader );
			return ( 0 );
		}

		psChunk = psChunk->mc_psNext;
	}
	printk( "Error: FreeHeapMem() called with invalid memory address\n" );
	return ( -EFAULT );
}


//****************************************************************************/
/** Allocates memory in the real memory region.
 * \ingroup DriverAPI
 * \param nSize the size in bytes to allocate.
 * \param nFlags <code>MEMF_CLEAR</code> is valid.
 * \return a pointer to the allocated memory.
 * \sa free_real()
 * \author Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
void *alloc_real( uint32 nSize, uint32 nFlags )
{
	uint32 nMemAddr = 0;
	int nFlg;
	status_t nReturn;

	nFlg = spinlock_disable( &g_sRealPoolLock );

	nSize = ( nSize + 0x07 ) & ~0x07;

	nReturn = AllocateHeapMem( &g_sRealMemHdr, &nMemAddr, false, false, nSize );

	spinunlock_enable( &g_sRealPoolLock, nFlg );

	if ( nReturn == 0 )
	{
		if ( nFlags & MEMF_CLEAR )
		{
			memset( ( void * )nMemAddr, 0, nSize );
		}
	}
	else
	{
		printk( "ERROR : alloc_real( %d, %x ) failed\n", nSize, nFlags );
	}

	if ( nReturn == 0 )
	{
		return ( ( void * )nMemAddr );
	}
	return ( NULL );
}


//****************************************************************************/
/** Frees memory in the real memory region.
 * \ingroup DriverAPI
 * \param pAddress a pointer to the allocated memory.
 * \sa alloc_real()
 * \author Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
void free_real( void *pAddress )
{
	uint32 nMemAddr = ( uint32 )pAddress;
	int nFlg;
	status_t nReturn;

	if ( nMemAddr >= 1024 * 1024 )
	{
		printk( "Error: free_real() called width invalid pointer %p\n", pAddress );
		return;
	}


	nFlg = spinlock_disable( &g_sRealPoolLock );
	nReturn = FreeHeapMem( &g_sRealMemHdr, nMemAddr );

	spinunlock_enable( &g_sRealPoolLock, nFlg );

	if ( nReturn != 0 )
	{
		printk( "Error: free_real() called width invalid pointer %p\n", pAddress );
	}
}

void sys_MemClear( void *LinAddr, uint32 Size )
{
	memset( LinAddr, 0, Size );
}


void init_memory_pools( char* pRealMemBase, MultiBootHeader_s* psHeader )
{
	/* Create real memory pool */
	g_sRealMemHdr.mh_nTotalSize = 0xa0000 - 1 - ( uint )pRealMemBase;
	g_sRealMemHdr.mh_psFirst = kmalloc( sizeof( MemChunk_s ), MEMF_KERNEL );
	memset( g_sRealMemHdr.mh_psFirst, 0, sizeof( MemChunk_s ) );
	g_sRealMemHdr.mh_psFirst->mc_nAddress = ( uint )pRealMemBase;
	g_sRealMemHdr.mh_psFirst->mc_nSize = g_sRealMemHdr.mh_nTotalSize;
	
	/* Reserve physical memory */
	boot_reserve_mem_region(0, (g_sSysBase.ex_nTotalPageCount * PAGE_SIZE) - 1, "Physical RAM");

	
	/* Parse the multiboot memory map and reserve the memory */
	if( psHeader && psHeader->mbh_nFlags & MB_INFO_MEM_MAP )
	{
		printk( "Multiboot memory map:\n" );
		uint32 nAddr;
		const char *pzType;
		for( nAddr = psHeader->mbh_nMemoryMapAddr; nAddr < psHeader->mbh_nMemoryMapAddr +
			psHeader->mbh_nMemoryMapLength; nAddr += *( ( unsigned long* )nAddr ) + 4 )
		{
			MBMemoryMapEntry_s* psEntry = ( MBMemoryMapEntry_s* )nAddr;
			
			switch ( psEntry->mm_nType )
			{
				case MB_MM_MEMORY:
					pzType = "(usable)";
					break;
				case MB_MM_RESERVED:
					pzType = "(reserved)";
					break;
				case MB_MM_ACPI_DATA:
					pzType = "(ACPI data)";
					break;
				case MB_MM_ACPI_NVS:
					pzType = "(ACPI NVS)";
					break;
				default:
					pzType = "(unknown)";
					break;
			}
			
			boot_reserve_mem_region(psEntry->mm_nBaseAddr,
			    psEntry->mm_nBaseAddr + psEntry->mm_nLength - 1, pzType);
			printk( "  %016Lx - %016Lx %s\n", psEntry->mm_nBaseAddr,
			psEntry->mm_nBaseAddr + psEntry->mm_nLength, pzType );
		}
	}
}

