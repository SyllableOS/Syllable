/*
 *  The AtheOS application server
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

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <atheos/types.h>
#include <atheos/isa_io.h>

#include <atheos/vesa_gfx.h>
#include <atheos/kernel.h>

#include <gui/bitmap.h>

#include "vesadrv.h"


using namespace os;

#define	MAX_MODEINFO_NAME			79

#define	MIF_PALETTE	0x0001		/* palettized screen mode	*/

static area_id	g_nFrameBufArea = -1;

int S3Blit( int  x1, int  y1, int  x2, int  y2, int  width, int  height );

void S3FillRect_32( long  x1, long  y1, long  x2, long  y2, uint color );

long S3DrawLine_32( long   x1, long x2, long y1, long y2, uint color, bool useClip, short clipLeft, short clipTop, short clipRight, short clipBottom );
void	InitS3( void );
void	EnableS3( void );


//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

VesaDriver::VesaDriver()
{
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

VesaDriver::~VesaDriver()
{
}


//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

bool VesaDriver::InitModes( void )
{
    Vesa_Info_s	 	sVesaInfo;
    VESA_Mode_Info_s 	sModeInfo;
    uint16		anModes[1024];
    int			nModeCount;
  
    int			 i=0;

    strcpy( sVesaInfo.VesaSignature, "VBE2" );

    nModeCount = get_vesa_info( &sVesaInfo, anModes, 1024 );

    if ( nModeCount <= 0 ) {
	dbprintf( "Error: VesaDriver::InitModes() no VESA20 modes found\n" );
	return( false );
    }
    
//    dbprintf( "Found %d vesa modes\n", nModeCount );

    int nPagedCount  = 0;
    int nPlanarCount = 0;
    int nBadCount    = 0;
    
    for( i = 0 ; i < nModeCount ; ++i )
    {
	get_vesa_mode_info( &sModeInfo, anModes[i] );

	if( sModeInfo.PhysBasePtr == 0 ) { // We must have linear frame buffer
	    nPagedCount++;
	    continue;
	}
	if( sModeInfo.BitsPerPixel < 8 ) {
	    nPlanarCount++;
	    continue;
	}
	if ( sModeInfo.NumberOfPlanes != 1 ) {
	    nPlanarCount++;
	    continue;
	}

	if ( sModeInfo.BitsPerPixel != 15 && sModeInfo.BitsPerPixel != 16 && sModeInfo.BitsPerPixel != 32 ) {
	    nBadCount++;
	    continue;
	}
	
	if ( sModeInfo.RedMaskSize == 0 && sModeInfo.GreenMaskSize == 0 && sModeInfo.BlueMaskSize == 0 &&
	     sModeInfo.RedFieldPosition == 0 && sModeInfo.GreenFieldPosition == 0 && sModeInfo.BlueFieldPosition == 0 ) {
	    m_cModeList.push_back( VesaMode( sModeInfo.XResolution, sModeInfo.YResolution, sModeInfo.BytesPerScanLine,
					     CS_CMAP8, anModes[i] | 0x4000, sModeInfo.PhysBasePtr ) );
	} else if ( sModeInfo.RedMaskSize == 5 && sModeInfo.GreenMaskSize == 5 && sModeInfo.BlueMaskSize == 5 &&
		    sModeInfo.RedFieldPosition == 10 && sModeInfo.GreenFieldPosition == 5 && sModeInfo.BlueFieldPosition == 0 ) {
	    m_cModeList.push_back( VesaMode( sModeInfo.XResolution, sModeInfo.YResolution, sModeInfo.BytesPerScanLine,
					     CS_RGB15, anModes[i] | 0x4000, sModeInfo.PhysBasePtr ) );
	} else if ( sModeInfo.RedMaskSize == 5 && sModeInfo.GreenMaskSize == 6 && sModeInfo.BlueMaskSize == 5 &&
		    sModeInfo.RedFieldPosition == 11 && sModeInfo.GreenFieldPosition == 5 && sModeInfo.BlueFieldPosition == 0 ) {
	    m_cModeList.push_back( VesaMode( sModeInfo.XResolution, sModeInfo.YResolution, sModeInfo.BytesPerScanLine,
					     CS_RGB16, anModes[i] | 0x4000, sModeInfo.PhysBasePtr ) );
	} else if ( sModeInfo.BitsPerPixel == 32 && sModeInfo.RedMaskSize == 8 && sModeInfo.GreenMaskSize == 8 && sModeInfo.BlueMaskSize == 8 &&
		    sModeInfo.RedFieldPosition == 16 && sModeInfo.GreenFieldPosition == 8 && sModeInfo.BlueFieldPosition == 0 ) {
	    m_cModeList.push_back( VesaMode( sModeInfo.XResolution, sModeInfo.YResolution, sModeInfo.BytesPerScanLine,
					     CS_RGB32, anModes[i] | 0x4000, sModeInfo.PhysBasePtr ) );
	} else {
	    dbprintf( "Found unsupported video mode: %dx%d %d BPP %d BPL - %d:%d:%d, %d:%d:%d\n",
		      sModeInfo.XResolution, sModeInfo.YResolution, sModeInfo.BitsPerPixel, sModeInfo.BytesPerScanLine,
		      sModeInfo.RedMaskSize, sModeInfo.GreenMaskSize, sModeInfo.BlueMaskSize,
		      sModeInfo.RedFieldPosition, sModeInfo.GreenFieldPosition, sModeInfo.BlueFieldPosition );
	}
#if 0
	dbprintf( "Mode %04x: %dx%d %d BPP %d BPL - %d:%d:%d, %d:%d:%d (%p)\n", anModes[i],
		  sModeInfo.XResolution, sModeInfo.YResolution, sModeInfo.BitsPerPixel, sModeInfo.BytesPerScanLine,
		  sModeInfo.RedMaskSize, sModeInfo.GreenMaskSize, sModeInfo.BlueMaskSize,
		  sModeInfo.RedFieldPosition, sModeInfo.GreenFieldPosition, sModeInfo.BlueFieldPosition, (void*)sModeInfo.PhysBasePtr );
#endif	
    }
    dbprintf( "Found total of %d VESA modes. Valid: %d, Paged: %d, Planar: %d, Bad: %d\n",
	      nModeCount, m_cModeList.size(), nPagedCount, nPlanarCount, nBadCount );
    return( true );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

area_id VesaDriver::Open( void ) {
    if ( InitModes() )
    {
	m_nFrameBufferSize = 1024 * 1024 * 4;
//	m_pFrameBuffer = NULL;
	g_nFrameBufArea = create_area( "vesa_io", NULL/*(void**) &m_pFrameBuffer*/, m_nFrameBufferSize,
				       AREA_FULL_ACCESS, AREA_NO_LOCK );
	return( g_nFrameBufArea );
    }
    return( -1 );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void VesaDriver::Close( void )
{
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

int VesaDriver::GetScreenModeCount()
{
    return( m_cModeList.size() );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

bool VesaDriver::GetScreenModeDesc( int nIndex, ScreenMode* psMode )
{
    if ( nIndex >= 0 && nIndex < int(m_cModeList.size()) ) {
	*psMode = m_cModeList[nIndex];
	return( true );
    } else {
	return( false );
    }
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

int VesaDriver::SetScreenMode( int nWidth, int nHeight, color_space eColorSpc,
			       int nPosH, int nPosV, int nSizeH, int nSizeV, float vRefreshRate )
{
    m_nCurrentMode  = -1;
  
    for ( int i = GetScreenModeCount() - 1 ; i >= 0 ; --i ) {
	if ( m_cModeList[i].m_nWidth == nWidth && m_cModeList[i].m_nHeight == nHeight && m_cModeList[i].m_eColorSpace == eColorSpc ) {
	    m_nCurrentMode = i;
	    break;
	}
    
    }

    if ( m_nCurrentMode >= 0  )
    {
	remap_area( g_nFrameBufArea, (void*)(m_cModeList[m_nCurrentMode].m_nFrameBuffer & PAGE_MASK) );
	m_nFrameBufferOffset = m_cModeList[m_nCurrentMode].m_nFrameBuffer & ~PAGE_MASK;
	if ( SetVesaMode( m_cModeList[m_nCurrentMode].m_nVesaMode ) ) {
	    return( 0 );
	}
    }
    return( -1 );
}

int VesaDriver::GetHorizontalRes()
{
    return( m_cModeList[m_nCurrentMode].m_nWidth );
}

int VesaDriver::GetVerticalRes()
{
    return( m_cModeList[m_nCurrentMode].m_nHeight );
}

int VesaDriver::GetBytesPerLine()
{
    return( m_cModeList[m_nCurrentMode].m_nBytesPerLine );
}

color_space VesaDriver::GetColorSpace()
{
    return( m_cModeList[m_nCurrentMode].m_eColorSpace );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

bool VesaDriver::IntersectWithMouse( const IRect& cRect )
{
//  if ( NULL != m_pcMouse ) {
//    return( cRect.DoIntersect( m_pcMouse->GetFrame() ) );
//  } else {
    return( false );
//  }
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void VesaDriver::SetColor( int nIndex, const Color32_s& sColor )
{
    outb_p( nIndex, 0x3c8 );
    outb_p( sColor.red >> 2, 0x3c9 );
    outb_p( sColor.green >> 2, 0x3c9 );
    outb_p( sColor.blue >> 2, 0x3c9 );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

bool VesaDriver::SetVesaMode( uint32 nMode )
{
    struct RMREGS rm;

    memset( &rm, 0, sizeof(struct RMREGS) );

    rm.EAX	= 0x4f02;
    rm.EBX	= nMode;

    realint( 0x10, &rm );
    int nResult = rm.EAX & 0xffff;

    memset( &rm, 0, sizeof(struct RMREGS) );
    rm.EBX = 0x01; // Get display offset.
    rm.EAX = 0x4f07;
    realint( 0x10, &rm );
  
    memset( &rm, 0, sizeof(struct RMREGS) );
    rm.EBX = 0x00;
    rm.EAX = 0x4f07;
    realint( 0x10, &rm );

    memset( &rm, 0, sizeof(struct RMREGS) );
    rm.EBX = 0x01; // Get display offset.
    rm.EAX = 0x4f07;
    realint( 0x10, &rm );

    return( nResult );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

bool VesaDriver::DrawLine( SrvBitmap* psBitMap, const IRect& cClipRect,
			   const IPoint& cPnt1, const IPoint& cPnt2, const Color32_s& sColor, int nMode )
{
/*  
  uint32	nColor;

  nColor	=	(sColor.anRGBA[0] >> (8 - m_pcCurMode->m_nRedBits)) << m_pcCurMode->m_nRedPos |
    (sColor.anRGBA[1] >> (8 - m_pcCurMode->m_nGreenBits)) << m_pcCurMode->m_nGreenPos |
    (sColor.anRGBA[2] >> (8 - m_pcCurMode->m_nBlueBits)) << m_pcCurMode->m_nBluePos;

  if ( psBitMap->m_bVideoMem && nMode == DM_COPY && 0 )
  {
    S3DrawLine_32( cPnt1.X, cPnt1.Y, cPnt2.X, cPnt2.Y, nColor,
		   true, cClipRect.MinX, cClipRect.MinY, cClipRect.MaxX, cClipRect.MaxY );
  }
  else
  {
  */  
    DisplayDriver::DrawLine( psBitMap, cClipRect, cPnt1, cPnt2, sColor, nMode );
//  }
  return( true );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

bool VesaDriver::FillRect( SrvBitmap* psBitMap, const IRect& cRect, const Color32_s& sColor )
{
/*  
  uint32	nColor;

  nColor	=	((sColor.anRGBA[0] >> (8 - m_pcCurMode->m_nRedBits)) << m_pcCurMode->m_nRedPos) |
    ((sColor.anRGBA[1] >> (8 - m_pcCurMode->m_nGreenBits)) << m_pcCurMode->m_nGreenPos) |
    ((sColor.anRGBA[2] >> (8 - m_pcCurMode->m_nBlueBits)) << m_pcCurMode->m_nBluePos);

  if ( psBitMap->m_bVideoMem && 0 )
  {
    S3FillRect_32( cRect.MinX, cRect.MinY, cRect.MaxX, cRect.MaxY, nColor );
    return( true );
  }
  else
  {
  */  
    return( DisplayDriver::FillRect( psBitMap, cRect, sColor ) );
//  }
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

bool VesaDriver::BltBitmap( SrvBitmap* dstbm, SrvBitmap* srcbm, IRect cSrcRect, IPoint cDstPos, int nMode )
{
/*  
  if ( dstbm->m_bVideoMem && srcbm->m_bVideoMem && nMode == DM_COPY && 0 )
  {
    S3Blit( cSrcRect.MinX, cSrcRect.MinY, cDstPos.X, cDstPos.Y, cSrcRect.Width(), cSrcRect.Height() );
  }
  else
  {
  */  
    return( DisplayDriver::BltBitmap( dstbm, srcbm, cSrcRect, cDstPos, nMode ) );
//  }
}
