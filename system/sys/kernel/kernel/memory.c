
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

#include <atheos/types.h>
#include <atheos/kernel.h>
#include <atheos/spinlock.h>

#include <atheos/kernel.h>

#include <macros.h>

#include "inc/sysbase.h"
#include "inc/mman.h"


SPIN_LOCK( g_sRealPoolLock, "real_pool_slock" );

static void *Allocate( MemHeader_s *mh, uint32 RequestSize )
{
	uint32 MemSize;
	struct MemChunk *mc, *prevmc;
	char *MemAdr = NULL;

	MemSize = ( ( RequestSize + 7L ) & ~7L );

	prevmc = ( void * )&mh->mh_First;

	for ( mc = mh->mh_First; mc; mc = mc->mc_Next )	/* Scan trough memory chunks    */
	{
		if ( ( ( ( uint32 )mc ) < ( ( uint32 )mh->mh_Lower ) ) || ( ( ( uint32 )mc ) > ( ( uint32 )( mh->mh_Upper ) ) ) )	/* Check for invalid chunks     */
		{
			printk( "ERROR : Memory list insane\n" );
			return ( NULL );
		}

		if ( mc->mc_Bytes == MemSize )	/* bulls eye, remove the chunk and give it away */
		{
			prevmc->mc_Next = mc->mc_Next;
			MemAdr = ( void * )mc;
			break;
		}

		if ( mc->mc_Bytes > MemSize )
		{
			prevmc->mc_Next = ( void * )( ( ( uint32 )( prevmc->mc_Next ) ) + MemSize );

			prevmc->mc_Next->mc_Next = mc->mc_Next;
			prevmc->mc_Next->mc_Bytes = mc->mc_Bytes - MemSize;
			MemAdr = ( void * )mc;
			break;
		}
		prevmc = mc;
	}
	if ( NULL != MemAdr )
	{
		mh->mh_Free -= MemSize;
	}
	return ( MemAdr );
}

static void Deallocate( MemHeader_s *mh, void *MemBlock, uint32 Size )
{
	struct MemChunk *prevmc, *mc, *thismc;
	uint32 BlockSize;

	uint32 *Ptr;

	BlockSize = ( ( Size + 7L ) & ~7L );

	mh->mh_Free += BlockSize;

	Ptr = MemBlock;

	thismc = ( void * )MemBlock;
	thismc->mc_Bytes = BlockSize;
	prevmc = ( void * )&mh->mh_First;

	if ( ( ( uint32 )thismc ) < ( ( uint32 )( mh->mh_First ) ) )	/* Before first chunk   */
	{
		thismc->mc_Next = mh->mh_First;
		mh->mh_First = thismc;

		if ( ( ( uint32 )thismc ) + thismc->mc_Bytes == ( ( uint32 )( thismc->mc_Next ) ) )	/* Join if possible     */
		{
			thismc->mc_Bytes += thismc->mc_Next->mc_Bytes;
			thismc->mc_Next = thismc->mc_Next->mc_Next;
		}
		return;
	}
	else
	{
		for ( mc = mh->mh_First; mc; mc = mc->mc_Next )
		{
			if ( ( ( ( uint32 )mc ) < ( ( uint32 )( mh->mh_Lower ) ) ) || ( ( ( uint32 )mc ) > ( ( uint32 )( mh->mh_Upper ) ) ) )	/* Check for invalid memory chunks      */
			{
				printk( "ERROR : Invalid memory list!----------------------\n" );
				return;
			}

			if ( ( ( uint32 )mc ) > ( ( uint32 )thismc ) )	/* Find area for insertion.     */
			{
				if ( ( ( uint32 )prevmc ) + prevmc->mc_Bytes == ( ( uint32 )thismc ) )	/* Join with previous if possible       */
				{
					prevmc->mc_Bytes += thismc->mc_Bytes;	/* Increase size        */
					if ( ( ( uint32 )prevmc ) + prevmc->mc_Bytes == ( ( uint32 )( prevmc->mc_Next ) ) )	/* Join with next if possible   */
					{
						prevmc->mc_Bytes += prevmc->mc_Next->mc_Bytes;	/* Increase size.       */
						prevmc->mc_Next = prevmc->mc_Next->mc_Next;	/* Skip this chunk      */
					}
				}
				else	/* Not lined up width previous;     */
				{
					thismc->mc_Next = prevmc->mc_Next;
					prevmc->mc_Next = thismc;

					if ( ( ( uint32 )thismc ) + thismc->mc_Bytes == ( ( uint32 )( thismc->mc_Next ) ) )	/* Join with next if possible   */
					{
						thismc->mc_Bytes += thismc->mc_Next->mc_Bytes;
						thismc->mc_Next = thismc->mc_Next->mc_Next;
					}
				}
				return;
			}
			prevmc = mc;
		}
	}
	prevmc->mc_Next = thismc;	/* Add node at end of list      */
	thismc->mc_Next = 0;

	if ( ( ( uint32 )prevmc ) + prevmc->mc_Bytes == ( ( uint32 )( prevmc->mc_Next ) ) )
	{
		prevmc->mc_Bytes += prevmc->mc_Next->mc_Bytes;
		prevmc->mc_Next = prevmc->mc_Next->mc_Next;
	}
}



void *alloc_real( uint32 RequestSize, uint32 Flags )
{
	uint32 *MemAdr = NULL;
	int nFlg = cli();

	spinlock( &g_sRealPoolLock );

	RequestSize += 4;	/* reserve 32 bit for the block size    */
	RequestSize = ( RequestSize + 0x07 ) & ~0x07;

	MemAdr = Allocate( &g_sSysBase.ex_sRealMemHdr, RequestSize );

	spinunlock( &g_sRealPoolLock );
	put_cpu_flags( nFlg );

	if ( MemAdr )
	{
		if ( Flags & MEMF_CLEAR )
		{
			memset( MemAdr, 0, RequestSize );
		}
	}
	else
	{
		printk( "ERROR : alloc_real( %ld, %lx ) failed\n", RequestSize, Flags );
	}

	if ( MemAdr != NULL )
	{
		MemAdr[0] = RequestSize;
		return ( &MemAdr[1] );
	}
	return ( NULL );
}


void free_real( void *Block )
{
	uint32 *MemBlock = Block;
	uint32 size;
	int nFlg;

	if ( ( ( uint32 )Block ) >= 1024 * 1024 )
	{
		printk( "Error: free_real() called width invalid pointer %p\n", Block );
		return;
	}

	MemBlock--;

	size = MemBlock[0];

	nFlg = cli();
	spinlock( &g_sRealPoolLock );
	Deallocate( &g_sSysBase.ex_sRealMemHdr, MemBlock, size );
	spinunlock( &g_sRealPoolLock );
	put_cpu_flags( nFlg );
}


void sys_MemClear( void *LinAddr, uint32 Size )
{
	memset( LinAddr, 0, Size );
}
