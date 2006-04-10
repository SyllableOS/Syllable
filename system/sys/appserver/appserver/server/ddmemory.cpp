/*
 *  The Syllable application server
 *  Copyright (C) 2005 The Syllable Team
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
 

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <atheos/types.h>
#include <atheos/kernel.h>
#include <macros.h>

#include "server.h"
#include "ddriver.h"
#include "sfont.h"
#include "bitmap.h"
#include "sprite.h"

using namespace os;

extern int g_nFrameBufferArea;


/** Initializes the memory manager.
 * \par Description:
 * Can be called by the display driver to initialize the memory manager. If the
 * memory manager is enabled the the appserver will store the window bitmaps there.
 * You should only enable it if your card can do videomemory to framebuffer blits.
 * You can either reserve a specific amount of memory which is enough for the
 * framebuffer of the highest supported screenmode or you can use the memory manager
 * methods to allocate the framebuffer from the display driver.
 * \par
 * \param nOffset - Offset where the managed area begins.
 * \param nSize - Size of the managed area.
 * \param nMemObjAlign - Alignment of the start offset of an memory object.
 *						 At least PAGE_SIZE - 1 is required.
 * \param nRowAlign - Alignment of every row of a bitmap.
 * \sa AllocateMemory(), FreeMemory()
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
void DisplayDriver::InitMemory( uint32 nOffset, uint32 nSize, uint32 nMemObjAlign, uint32 nRowAlign )
{
	/* Initialize one free object */
	m_nMemObjAlign = nMemObjAlign;
	m_nRowAlign = nRowAlign;
	
	__assertw( m_psFirstMemObj == NULL );
	m_psFirstMemObj = new DisplayMemObj;
	m_psFirstMemObj->pcPrev = NULL;
	m_psFirstMemObj->pcNext = NULL;
	m_psFirstMemObj->bFree = true;
	m_psFirstMemObj->nOffset = nOffset;
	m_psFirstMemObj->nObjStart = nOffset;
	m_psFirstMemObj->nSize = nSize;
	
	m_pVideoMem = NULL;
}

void dump_blocks( DisplayMemObj* pcObj )
{
	while( pcObj != NULL )
	{
		dbprintf( "%i %i %i %i\n", (int)pcObj->bFree, (int)pcObj->nOffset, (int)pcObj->nObjStart, (int)pcObj->nSize );
		pcObj = pcObj->pcNext;
	}
	dbprintf( "Dump finished!\n" );
}


/** Allocates memory from the memory manager.
 * \par Description:
 * Called by the appserver to allocate videomemory for the bitmaps of the windows.
 * Can also be used by the display driver to allocate memory for the framebuffer.
 * \par
 * \param nSize - Requested size.
 * \param pnOffset - The offset is stored here if the call had success.
 * \sa FreeMemory(), AllocateBitmap()
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
status_t DisplayDriver::AllocateMemory( uint32 nSize, uint32* pnOffset )
{
	/* Search for a free object */
	DisplayMemObj* pcObj = m_psFirstMemObj;
	while( pcObj != NULL )
	{
		if( pcObj->bFree )
		{
			/* Check size */
			uint32 nObjStart = ( ( pcObj->nOffset + m_nMemObjAlign ) & ~m_nMemObjAlign );
			uint32 nObjEnd = ( ( nObjStart + nSize + m_nMemObjAlign ) & ~m_nMemObjAlign ) - 1;
			if( nObjEnd < pcObj->nOffset + pcObj->nSize )
			{
				pcObj->bFree = false;
				pcObj->nObjStart = nObjStart;
				//dbprintf( "Allocate block %i %i %i %i\n", (int)pcObj->nOffset, (int)pcObj->nSize, (int)nObjStart, (int)nObjEnd );
				if( nObjEnd == pcObj->nOffset + pcObj->nSize - 1 ) {
					//dbprintf( "Full block!!!\n" );
				} else {
					uint32 nLeftSize = pcObj->nSize + pcObj->nOffset - nObjEnd - 1;
					pcObj->nSize = nObjEnd - pcObj->nOffset + 1;
					DisplayMemObj* pcNewObj = new DisplayMemObj;
					pcNewObj->pcPrev = pcObj;
					pcNewObj->pcNext = pcObj->pcNext;
					pcNewObj->bFree = true;
					pcNewObj->nOffset = pcObj->nOffset + pcObj->nSize;
					pcNewObj->nObjStart = pcNewObj->nOffset;
					pcNewObj->nSize = nLeftSize;
					pcObj->pcNext = pcNewObj;
					if( pcNewObj->pcNext != NULL )
						pcNewObj->pcNext->pcPrev = pcNewObj;
					
					//dbprintf( "Free block %i %i\n", (int)pcNewObj->nOffset, (int)pcNewObj->nSize );
				}
				//dump_blocks( m_psFirstMemObj );
				*pnOffset = pcObj->nObjStart;
				return( 0 );
			}
		}
		pcObj = pcObj->pcNext;
	} 
	//dbprintf( "Could not find free memory!\n" );
	return( -1 );
}


/** Allocates a bitmap in the videomemory.
 * \par Description:
 * Works like AllocateMemory() but already creates a bitmap that uses the 
 * allocated memory. The destructor of the SrvBitmap will free the memory.
 * \par
 * \param nWidth - Width.
 * \param nHeight - Height.
 * \parma eColorSpc - Colorspace.
 * \sa FreeMemory(), AllocateMemory()
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
SrvBitmap* DisplayDriver::AllocateBitmap( int nWidth, int nHeight, os::color_space eColorSpc )
{
	uint32 nBytesPerLine = ( ( nWidth * BitsPerPixel( eColorSpc ) ) / 8 + m_nRowAlign ) & ~m_nRowAlign;
	uint32 nOffset;
	
	/* Allocate memory */
	if( AllocateMemory( nBytesPerLine * nHeight, &nOffset ) != 0 )
		return( NULL );
		
	/* Get pointer to the video memory */
	if( m_pVideoMem == NULL )
	{
		AreaInfo_s sFBAreaInfo;
		get_area_info( g_nFrameBufferArea, &sFBAreaInfo );
		m_pVideoMem = (uint8*)sFBAreaInfo.pAddress;
	}
	
	/* Create bitmap */
	uint8* pRaster = m_pVideoMem + nOffset;
	SrvBitmap* pcBitmap = new SrvBitmap( nWidth, nHeight, eColorSpc, pRaster, nBytesPerLine );
	pcBitmap->m_bVideoMem = true;
	pcBitmap->m_bFreeRaster = true;
	pcBitmap->m_nVideoMemOffset = nOffset;
	pcBitmap->m_nFlags = Bitmap::NO_ALPHA_CHANNEL;
	return( pcBitmap );
}


/** Frees allocated videomemory.
 * \par Description:
 * Called to free videomemory which has been allocated with the AllocateMemory()
 * method.
 * \par
 * \param nOffset - Offset of the memory.
 * \sa AllocateMemory(), AllocateBitmap()
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
void DisplayDriver::FreeMemory( uint32 nOffset )
{
	/* Search for a free object */
	DisplayMemObj* pcObj = m_psFirstMemObj;
	while( pcObj != NULL )
	{
		if( !pcObj->bFree && pcObj->nObjStart == nOffset )
		{
			/* Check if we can merge this object with the previous one */
			if( pcObj->pcPrev != NULL && pcObj->pcPrev->bFree )
			{
				DisplayMemObj* pcPrevObj = pcObj->pcPrev;
				pcPrevObj->nSize += pcObj->nSize;
				pcPrevObj->pcNext = pcObj->pcNext;
				if( pcPrevObj->pcNext != NULL )
					pcPrevObj->pcNext->pcPrev = pcPrevObj;
				delete( pcObj );
				pcObj = pcPrevObj;
			}
			/* Check if we can merge this object with the next one */
			if( pcObj->pcNext != NULL && pcObj->pcNext->bFree )
			{
				DisplayMemObj* pcNextObj = pcObj->pcNext;
				pcObj->nSize += pcNextObj->nSize;
				pcObj->pcNext = pcNextObj->pcNext;
				if( pcObj->pcNext != NULL )
					pcObj->pcNext->pcPrev = pcObj;
				delete( pcNextObj );
			}
			pcObj->bFree = true;
			//dump_blocks( m_psFirstMemObj );
			return;
		}
		pcObj = pcObj->pcNext;
	}
	//dbprintf( "Could not find object to free!\n" );
}









