
/*
 *  The AtheOS kernel
 *  Copyright (C) 1999 - 2000  Kurt Skauen
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

#include <posix/errno.h>
#include <atheos/types.h>
#include <atheos/kernel.h>
#include <atheos/semaphore.h>

#include <macros.h>

#include "inc/scheduler.h"
#include "inc/sysbase.h"
#include "inc/areas.h"

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

area_id get_next_area( const area_id hArea )
{
	area_id hNext;

	kassertw( is_semaphore_locked( g_hAreaTableSema ) == false );
	LOCK( g_hAreaTableSema );
	hNext = MArray_GetNextIndex( &g_sAreas, hArea );
	UNLOCK( g_hAreaTableSema );

	return ( hNext );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

MemArea_s *get_area_from_handle( area_id hArea )
{
	MemArea_s *psArea;

	kassertw( is_semaphore_locked( g_hAreaTableSema ) );

	psArea = MArray_GetObj( &g_sAreas, hArea );

	if ( psArea != NULL )
	{
		atomic_inc( &psArea->a_nRefCount );
	}
	
	return ( psArea );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

inline int find_area( MemContext_s *psCtx, uint32 nAddress )
{
	MemArea_s **apsBase = psCtx->mc_apsAreas;
	int nBase = 0;
	int i;
	

	for ( i = psCtx->mc_nAreaCount; i != 0; i >>= 1 )
	{
		int j = nBase + ( i >> 1 );
		MemArea_s *psArea = apsBase[j];

		if ( nAddress >= psArea->a_nStart && ( j == psCtx->mc_nAreaCount - 1 || nAddress < apsBase[j + 1]->a_nStart ) )
		{
			return ( j );
		}
		if ( nAddress > psArea->a_nEnd )
		{
			nBase = j + 1;	// Move right
			i--;
		}
	}
	return ( -1 );
}


/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

inline MemArea_s *get_area( MemContext_s *psCtx, uint32 nAddress )
{
	MemArea_s *psArea = NULL;
	int nIndex;

	if ( nAddress < AREA_FIRST_USER_ADDRESS )
	{
		psCtx = g_psKernelSeg;
	}

	if ( is_semaphore_locked( g_hAreaTableSema ) )
	{
		printk( "Error: get_area() called with g_hAreaTableSema locked\n" );
	}
	LOCK( g_hAreaTableSema );
	nIndex = find_area( psCtx, nAddress );
	if ( nIndex >= 0 )
	{
		psArea = psCtx->mc_apsAreas[nIndex];
		atomic_inc( &psArea->a_nRefCount );
	}
	UNLOCK( g_hAreaTableSema );
	return ( psArea );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

MemArea_s *verify_area( const void *pAddr, uint32 nSize, bool bWrite )
{
	MemArea_s *psArea;
	uint32 nAddr = ( uint32 )pAddr;

	if ( nAddr < AREA_FIRST_USER_ADDRESS )
	{
		printk( "verify_area() got kernel address %08x\n", nAddr );
		return ( NULL );
	}

	psArea = get_area( CURRENT_PROC->tc_psMemSeg, nAddr );

	if ( psArea == NULL )
	{
		printk( "verify_area() no area for address %08x\n", nAddr );
		return ( NULL );
	}
	if ( nAddr + nSize - 1 > psArea->a_nEnd )
	{
		printk( "verify_area() adr=%08x size=%d does not fit in area %08x-%08x\n", nAddr, nSize, psArea->a_nStart, psArea->a_nEnd );
		put_area( psArea );
		return ( NULL );
	}
	return ( psArea );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int insert_area( MemContext_s *psCtx, MemArea_s *psArea )
{
	int nIndex;
	int nError;
	

	psArea->a_nAreaID = MArray_Insert( &g_sAreas, psArea, true );

	if ( psArea->a_nAreaID < 0 )
	{
		printk( "Error: insert_area() failed to add area to global area table\n" );
		return ( psArea->a_nAreaID );
	}

	if ( psCtx->mc_nAreaCount + 1 > psCtx->mc_nMaxAreaCount )
	{
		MemArea_s **apsNewArray;
		int nNewCnt;

		if ( psCtx->mc_nMaxAreaCount > 0 )
		{
			nNewCnt = psCtx->mc_nMaxAreaCount << 1;
		}
		else
		{
			nNewCnt = 32;
		}

		apsNewArray = kmalloc( nNewCnt * sizeof( MemArea_s * ), MEMF_KERNEL | MEMF_NOBLOCK );
		if ( NULL == apsNewArray )
		{
			printk( "Error: insert_area() : out of memory\n" );
			nError = -ENOMEM;
			goto error;
		}
		memcpy( apsNewArray, psCtx->mc_apsAreas, psCtx->mc_nAreaCount * sizeof( MemArea_s * ) );
		memset( apsNewArray + psCtx->mc_nAreaCount, 0, ( psCtx->mc_nMaxAreaCount - psCtx->mc_nAreaCount ) * sizeof( MemArea_s * ) );
		if ( psCtx->mc_apsAreas != NULL )
		{
			kfree( psCtx->mc_apsAreas );
		}
		psCtx->mc_apsAreas = apsNewArray;
		psCtx->mc_nMaxAreaCount = nNewCnt;
	}

	nIndex = find_area( psCtx, psArea->a_nStart ) + 1;

	memmove( psCtx->mc_apsAreas + nIndex + 1, psCtx->mc_apsAreas + nIndex, ( psCtx->mc_nAreaCount - nIndex ) * sizeof( MemArea_s * ) );

	psCtx->mc_apsAreas[nIndex] = psArea;
	psCtx->mc_nAreaCount++;

	if ( nIndex > 0 )
	{
		psCtx->mc_apsAreas[nIndex - 1]->a_psNext = psArea;
		psArea->a_psPrev = psCtx->mc_apsAreas[nIndex - 1];
	}
	else
	{
		psArea->a_psPrev = NULL;
	}

	if ( nIndex < psCtx->mc_nAreaCount - 1 )
	{
		psCtx->mc_apsAreas[nIndex + 1]->a_psPrev = psArea;
		psArea->a_psNext = psCtx->mc_apsAreas[nIndex + 1];
	}
	else
	{
		psArea->a_psNext = NULL;
	}
	return ( 0 );
      error:
	MArray_Remove( &g_sAreas, psArea->a_nAreaID );
	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

void remove_area( MemContext_s *psCtx, MemArea_s *psArea )
{
	int nIndex;
	
	kassertw( is_semaphore_locked( g_hAreaTableSema ) );

	nIndex = find_area( psCtx, psArea->a_nStart );

	if ( nIndex == -1 )
	{
		printk( "Error: remove_area() could not find area %08x - %08x\n", psArea->a_nStart, psArea->a_nEnd );
		list_areas( psCtx );
		return;
	}
	if ( psArea != psCtx->mc_apsAreas[nIndex] )
	{
		printk( "Error: remove_area() found wrong area %p (%08x-%08x) at index %d\n", psArea, psArea->a_nStart, psArea->a_nEnd, nIndex );
		return;
	}
	if ( nIndex > 0 )
	{
		psCtx->mc_apsAreas[nIndex - 1]->a_psNext = psArea->a_psNext;
	}
	if ( nIndex < psCtx->mc_nAreaCount - 1 )
	{
		psCtx->mc_apsAreas[nIndex + 1]->a_psPrev = psArea->a_psPrev;
	}
	memmove( &psCtx->mc_apsAreas[nIndex], &psCtx->mc_apsAreas[nIndex + 1], ( psCtx->mc_nAreaCount - nIndex - 1 ) * sizeof( MemArea_s * ) );
	psCtx->mc_nAreaCount--;

	if ( -1 != psArea->a_nAreaID )
	{
		MArray_Remove( &g_sAreas, psArea->a_nAreaID );
	}
}


/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

uint32 find_unmapped_area( MemContext_s *psCtx, int nAllocMode, uint32 nSize, uint32 nAddress )
{
	MemArea_s *psArea;
	uint32 nStart;
	uint32 nEnd;
	int nIndex;
	
	kassertw( is_semaphore_locked( g_hAreaTableSema ) );

	switch ( nAllocMode & AREA_ADDR_SPEC_MASK )
	{
	case AREA_ANY_ADDRESS:
		if ( nAllocMode & AREA_KERNEL )
		{
			nStart = 1024 * 1024;
			nEnd = 0x7fffffff;
		}
		else
		{
			nStart = g_sSysBase.sb_nFirstUserAddress;
			nEnd = g_sSysBase.sb_nLastUserAddress;
		}
		break;
	case AREA_EXACT_ADDRESS:
		nStart = nAddress;
		nEnd = nAddress + nSize - 1;
		break;
	case AREA_BASE_ADDRESS:
		nStart = nAddress;
		if ( nAllocMode & AREA_KERNEL )
		{
			nEnd = AREA_LAST_KERNEL_ADDRESS;
		}
		else
		{
			nEnd = g_sSysBase.sb_nLastUserAddress;
		}
		break;
	default:
		printk( "find_unmapped_area() : invalid mode %d\n", nAllocMode );
		return ( -1 );
	}

	if ( nStart > nEnd )
	{
		printk( "find_unmapped_area() : invalid range %08x - %08x\n", nStart, nEnd );
		return ( -1 );
	}


	if ( psCtx->mc_nAreaCount == 0 )
	{
		if ( nAllocMode & AREA_TOP_DOWN )
		{
			return ( nEnd - nSize + 1 );
		}
		else
		{
			return ( nStart );
		}
	}

	if ( nAllocMode & AREA_TOP_DOWN )
	{
		nIndex = find_area( psCtx, nEnd );

		if ( nIndex == -1 )
		{
			nIndex = 0;
		}

		for ( psArea = psCtx->mc_apsAreas[nIndex]; psArea != NULL; psArea = psArea->a_psPrev )
		{
			if ( psArea->a_psNext == NULL && nEnd - psArea->a_nMaxEnd >= nSize )
			{	// Fit after last
				return ( nEnd - nSize + 1 );
			}
			else if ( psArea->a_psNext != NULL && psArea->a_psNext->a_nStart - psArea->a_nMaxEnd > nSize )
			{
				return ( psArea->a_psNext->a_nStart - nSize );
			}
			else if ( psArea->a_psPrev == NULL )
			{	// before first
				if ( psArea->a_nStart - nStart >= nSize )
				{
					return ( psArea->a_nStart - nSize );
				}
				else
				{
					return ( -1 );
				}
			}
		}
		panic( "find_unmapped_area() something went wrong while searching backwards\n" );
		return ( -1 );
	}
	else
	{
		nIndex = find_area( psCtx, nStart );

		if ( nIndex == -1 )
		{
			if ( psCtx->mc_apsAreas[0]->a_nStart >= nStart + nSize )
			{
				return ( nStart );
			}
			nIndex = 0;
		}
		psArea = psCtx->mc_apsAreas[nIndex];
		if ( nStart > psArea->a_nMaxEnd )
		{
			if ( ( psArea->a_psNext == NULL && ( nStart + nSize - 1 ) > nStart && ( nStart + nSize - 1 ) <= nEnd ) || ( psArea->a_psNext != NULL && ( nStart + nSize ) <= psArea->a_psNext->a_nStart ) )
			{
				return ( nStart );
			}
		}

		for ( psArea = psCtx->mc_apsAreas[nIndex]; psArea != NULL; psArea = psArea->a_psNext )
		{
			if ( psArea->a_nMaxEnd >= nEnd )
			{
				return ( -1 );
			}
			if ( psArea->a_psNext == NULL || psArea->a_psNext->a_nStart - psArea->a_nMaxEnd > nSize )
			{
				break;
			}
		}
		if ( psArea->a_nMaxEnd + nSize <= nEnd )
		{
			return ( psArea->a_nMaxEnd + 1 );
		}
		else
		{
			return ( -1 );
		}
	}
}
