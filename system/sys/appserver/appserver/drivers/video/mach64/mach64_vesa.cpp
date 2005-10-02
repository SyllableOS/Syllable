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

#include "mach64_vesa.h"


using namespace os;

#define	MAX_MODEINFO_NAME			79

#define	MIF_PALETTE	0x0001		/* palettized screen mode	*/

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

M64VesaDriver::M64VesaDriver( int nFd )
{
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

M64VesaDriver::~M64VesaDriver()
{
}


//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

bool M64VesaDriver::InitModes( void )
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
					     CS_CMAP8, 60.0f, anModes[i] | 0x4000, sModeInfo.PhysBasePtr ) );
	} else if ( sModeInfo.RedMaskSize == 5 && sModeInfo.GreenMaskSize == 6 && sModeInfo.BlueMaskSize == 5 &&
		    sModeInfo.RedFieldPosition == 11 && sModeInfo.GreenFieldPosition == 5 && sModeInfo.BlueFieldPosition == 0 ) {
	    m_cModeList.push_back( VesaMode( sModeInfo.XResolution, sModeInfo.YResolution, sModeInfo.BytesPerScanLine,
					     CS_RGB16, 60.0f, anModes[i] | 0x4000, sModeInfo.PhysBasePtr ) );
	} else if ( sModeInfo.BitsPerPixel == 32 && sModeInfo.RedMaskSize == 8 && sModeInfo.GreenMaskSize == 8 && sModeInfo.BlueMaskSize == 8 &&
		    sModeInfo.RedFieldPosition == 16 && sModeInfo.GreenFieldPosition == 8 && sModeInfo.BlueFieldPosition == 0 ) {
	    m_cModeList.push_back( VesaMode( sModeInfo.XResolution, sModeInfo.YResolution, sModeInfo.BytesPerScanLine,
					     CS_RGB32, 60.0f, anModes[i] | 0x4000, sModeInfo.PhysBasePtr ) );
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

area_id M64VesaDriver::Open( void ) {
    return( -1 );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void M64VesaDriver::Close( void )
{
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

int M64VesaDriver::GetScreenModeCount()
{
    return( m_cModeList.size() );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

bool M64VesaDriver::GetScreenModeDesc( int nIndex, screen_mode* psMode )
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

int M64VesaDriver::SetScreenMode( screen_mode sMode )
{
    m_nCurrentMode  = -1;
  
    for ( int i = GetScreenModeCount() - 1 ; i >= 0 ; --i ) {
	if ( m_cModeList[i].m_nWidth == sMode.m_nWidth && m_cModeList[i].m_nHeight == sMode.m_nHeight && m_cModeList[i].m_eColorSpace == sMode.m_eColorSpace ) {
	    m_nCurrentMode = i;
	    break;
	}
    
    }

    if ( m_nCurrentMode >= 0  )
    {
	if ( SetVesaMode( m_cModeList[m_nCurrentMode].m_nVesaMode ) ) {
	    return( 0 );
	}
    }
    return( -1 );
}

screen_mode M64VesaDriver::GetCurrentScreenMode()
{
    return( m_cModeList[m_nCurrentMode] );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

bool M64VesaDriver::IntersectWithMouse( const IRect& cRect )
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

bool M64VesaDriver::SetVesaMode( uint32 nMode )
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







