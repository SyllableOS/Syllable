/*
 *  The AtheOS application server
 *  Copyright (C) 1999 - 2001  Kurt Skauen
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

#include <stdio.h>

#include <gui/bitmap.h>
#include <exception>

#include "mga.h"
#include "gx00_crtc1.h"
#include "../../../server/bitmap.h"

using namespace os;

#define PCI_VENDOR_ID_MATROX		0x102B
#define PCI_DEVICE_ID_MATROX_MGA_2	0x0518
#define PCI_DEVICE_ID_MATROX_2064W_PCI	0x0519	// MilleniumI
#define PCI_DEVICE_ID_MATROX_2164W_PCI	0x051B	// MilleniumII
#define PCI_DEVICE_ID_MATROX_2164W_AGP	0x051F	// MilleniumII
#define PCI_DEVICE_ID_MATROX_MGA_IMP	0x0d10
#define PCI_VENDOR_ID_MATROX_G100_PCI	0x1000	// looks strange, but they are from the G100 manual.
#define PCI_VENDOR_ID_MATROX_G100_AGP	0x1001
#define PCI_VENDOR_ID_MATROX_G200_PCI	0x0520
#define PCI_VENDOR_ID_MATROX_G200_AGP	0x0521
#define PCI_VENDOR_ID_MATROX_G400_AGP	0x0525


//#define M2_DEVCTRL	0x0004
#define M2_DWGCTL	0x1c00
#define M2_MACCESS	0x1c04
#define M2_PLNWT	0x1c1c
#define M2_BCOL		0x1c20
#define M2_FCOL		0x1c24
#define M2_XYSTRT	0x1c40
#define M2_XYEND	0x1c44
#define M2_SGN		0x1c58
#define M2_AR0		0x1c60
#define M2_AR3		0x1c6c
#define	M2_AR5		0x1c74
#define M2_CXBNDRY	0x1c80	// Left/right clipping boundary
#define M2_FXBNDRY	0x1c84
#define M2_YDSTLEN	0x1c88
#define M2_PITCH	0x1c8c
#define M2_YDSTORG	0x1c94
#define M2_YTOP		0x1c98
#define M2_YBOT		0x1c9c

#define M2_STATUS	0x1e14




  // Masks and values for the M2_DWGCTL register
#define M2_OPCODE_LINE_OPEN		0x00
#define M2_OPCODE_AUTOLINE_OPEN		0x01
#define M2_OPCODE_LINE_CLOSE		0x02
#define M2_OPCODE_AUTOLINE_CLOSE	0x03
#define M2_OPCODE_TRAP			0x04
#define M2_OPCODE_TRAP_LOAD		0x05
#define M2_OPCODE_BITBLT		0x08
#define M2_OPCODE_FBITBLT		0x0c
#define M2_OPCODE_ILOAD			0x09
#define M2_OPCODE_ILOAD_SCALE		0x0d
#define M2_OPCODE_ILOAD_FILTER		0x0f
#define M2_OPCODE_IDUMP			0x0a
#define M2_OPCODE_ILOAD_HIQH		0x07
#define M2_OPCODE_ILOAD_HIQHV		0x0e
  
  
#define M2_ATYPE_RPL	0x000	// Write (replace)
#define M2_ATYPE_RSTR	0x010	// Read-modify-write (raster)
#define M2_ATYPE_ZI	0x030	// Depth mode with Gouraud
#define M2_ATYPE_BLK	0x040	// Block write mode
#define M2_ATYPE_I	0x090	// Gouraud (with depth compare)
  
#define M2_ZMODE_NOZCMP	0x0000
#define M2_ZMODE_ZE	0x0200
#define M2_ZMODE_ZNE	0x0300
#define M2_ZMODE_ZLT	0x0400
#define M2_ZMODE_ZLTE	0x0500
#define M2_ZMODE_ZGT	0x0600
#define M2_ZMODE_ZGTE	0x0700
  
#define M2_SOLID	0x0800
#define M2_ARZERO	0x1000
#define M2_SGNZERO	0x2000
#define M2_SHFTZERO	0x4000
  
#define	M2_BOP_MASK	0x0f0000
#define	M2_TRANS_MASK	0xf00000
  
#define	M2_BLTMOD_BMONOLEF	0x00000000
#define	M2_BLTMOD_BMONOWF	0x80000000
#define	M2_BLTMOD_BPLAN		0x02000000
#define	M2_BLTMOD_BFCOL		0x04000000
#define	M2_BLTMOD_BUYUV		0x1a000000
#define	M2_BLTMOD_BU32BGR	0x06000000
#define	M2_BLTMOD_BU32RGB	0x0e000000
#define	M2_BLTMOD_BU24BGR	0x16000000
#define	M2_BLTMOD_BU24RGB	0x1e000000
  
#define M2_PATTERN	0x20000000
#define M2_TRANSC	0x40000000

  // Masks and values for the M2_SGN register:
#define M2_SGN_SCANLEFT	0x0001
#define M2_SGN_SDXL	0x0002
#define M2_SGN_SDY	0x0004
#define M2_SGN_SDXR	0x0010

  // Bit masks for M2_STATUS

#define M2_PICKPEN	0x00004
#define M2_VSYNCSTS	0x00008
#define M2_VSYNCPEN	0x00010
#define M2_VLINEPEN	0x00020
#define M2_EXTPEN	0x00040
#define M2_DWGENGSTS	0x10000

// These registers is located on the RAMDAC chip, and is not documented in the MGA-2064M manual :(
#define TVP3026_INDEX		0x3c00
#define TVP3026_WADR_PAL	0x3c00
#define TVP3026_CUR_COL_ADDR	0x3c04
#define TVP3026_CUR_COL_DATA	0x3c05
#define TVP3026_CURSOR_CTL	0x3c06
#define TVP3026_DATA		0x3c0a
#define TVP3026_CUR_XLOW	0x3c0c
#define TVP3026_CUR_RAM		0x3c0b
#define TVP3026_CUR_XHI		0x3c0d
#define TVP3026_CUR_YLOW	0x3c0e
#define TVP3026_CUR_YHI		0x3c0f

// 0x00 = transparent 0x01=xor(not supported?) 0x02=black 0x03=white
/*
#define POINTER_WIDTH 13
#define POINTER_HEIGHT 13
static uint8 g_anMouseImg[]=
{
    0x02,0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x02,0x03,0x02,0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x02,0x03,0x03,0x02,0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x02,0x03,0x03,0x03,0x03,0x02,0x02,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x02,0x03,0x03,0x03,0x03,0x03,0x02,0x02,0x00,0x00,0x00,
    0x00,0x00,0x02,0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x02,0x02,0x00,
    0x00,0x00,0x00,0x02,0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x02,0x00,
    0x00,0x00,0x00,0x02,0x03,0x03,0x03,0x03,0x03,0x03,0x02,0x00,0x00,
    0x00,0x00,0x00,0x00,0x02,0x03,0x03,0x03,0x03,0x02,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x02,0x03,0x03,0x03,0x02,0x03,0x02,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x02,0x03,0x02,0x00,0x02,0x03,0x02,0x00,
    0x00,0x00,0x00,0x00,0x00,0x02,0x02,0x00,0x00,0x00,0x02,0x03,0x02,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x02,0x00,
};
*/
Millenium2::Millenium2() : m_cGELock( "mill2_ge_lock" ), m_cCursorHotSpot(0,0)
{
    bool bFound = false;

    m_bIsInitiated = false;
    m_hRegisterArea = -1;
    m_hFrameBufferArea = -1;

    for ( int i = 0 ;  get_pci_info( &m_cPCIInfo, i ) == 0 ; ++i ) {
	if ( m_cPCIInfo.nVendorID == PCI_VENDOR_ID_MATROX ) {
	    bFound = true;
	    break;
	}
    }
    if ( bFound == false ) {
       dbprintf( "No MGA card\n" );
       return;
//       exception cExc;
//	throw cExc;
    }

    dbprintf( "Found matrox card: %04X\n", m_cPCIInfo.nDeviceID );

    m_hRegisterArea = create_area( "mill2_regs",(void**) &m_pRegisterBase, 1024 * 16,
				   AREA_FULL_ACCESS, AREA_NO_LOCK );

    if ( m_hRegisterArea < 0 ) {
	dbprintf( "Millenium2::Open() failed to create register area (%d)\n", m_hRegisterArea );
	return;
//	throw exception();
    }
  
    if( m_cPCIInfo.nDeviceID == PCI_DEVICE_ID_MATROX_2064W_PCI )
    {
	// The MilleniumI is a bit special, the frame buffer, and the
	//  io area is swapped compared to the other Matrox boards...
	remap_area( m_hRegisterArea, (void*) (m_cPCIInfo.u.h0.nBase0 & PCI_ADDRESS_MEMORY_32_MASK) );
    }
    else
    {
	remap_area( m_hRegisterArea, (void*) (m_cPCIInfo.u.h0.nBase1 & PCI_ADDRESS_MEMORY_32_MASK) );
    }

    // The HW pointer code has only been teset on MilI, and might work diffrently on the other boards...
    if( m_cPCIInfo.nDeviceID == PCI_DEVICE_ID_MATROX_2064W_PCI ||
	m_cPCIInfo.nDeviceID == PCI_DEVICE_ID_MATROX_2164W_PCI ||
	m_cPCIInfo.nDeviceID == PCI_DEVICE_ID_MATROX_2164W_AGP )
    {
	m_nPointerScheme = POINTER_MILLENIUM;
    }
    else
    {
	m_nPointerScheme = POINTER_SPRITE;
    }

    if( /*m_cPCIInfo.nDeviceID == PCI_VENDOR_ID_MATROX_G100_PCI ||
	m_cPCIInfo.nDeviceID == PCI_VENDOR_ID_MATROX_G100_AGP ||*/
	m_cPCIInfo.nDeviceID == PCI_VENDOR_ID_MATROX_G200_PCI ||
	m_cPCIInfo.nDeviceID == PCI_VENDOR_ID_MATROX_G200_AGP ||
	m_cPCIInfo.nDeviceID == PCI_VENDOR_ID_MATROX_G400_AGP )
    {
	m_nCRTCScheme = CRTC_GX00;
    }
    else
    {
	m_nCRTCScheme = CRTC_VESA;
    }
    if( m_nCRTCScheme == CRTC_GX00 )
    {
	// TODO: find out if the actualy is ram for the resolution...
	color_space colspace[3] = { CS_RGB15, CS_RGB16, CS_RGB32 };
	int bpp[3] = { 2, 2, 4 };
	for( int i=0; i<3; i++ )
	{
	    m_cScreenModeList.push_back( ScreenMode(128, 88, (128*bpp[i]+15)&~15, colspace[i]) ); // broken??!?
	    m_cScreenModeList.push_back( ScreenMode(160, 120, (160*bpp[i]+15)&~15, colspace[i]) );
	    m_cScreenModeList.push_back( ScreenMode(320, 240, (320*bpp[i]+15)&~15, colspace[i]) );
	    m_cScreenModeList.push_back( ScreenMode(512, 350, (512*bpp[i]+15)&~15, colspace[i]) ); // broken??!?
	    m_cScreenModeList.push_back( ScreenMode(640, 480, (640*bpp[i]+15)&~15, colspace[i]) );
	    m_cScreenModeList.push_back( ScreenMode(800, 600, (800*bpp[i]+15)&~15, colspace[i]) );
	    m_cScreenModeList.push_back( ScreenMode(1024, 768, (1024*bpp[i]+15)&~15, colspace[i]) );
	    m_cScreenModeList.push_back( ScreenMode(1152, 864, (1152*bpp[i]+15)&~15, colspace[i]) );
	    m_cScreenModeList.push_back( ScreenMode(1280, 1024, (1280*bpp[i]+15)&~15, colspace[i]) );
	    m_cScreenModeList.push_back( ScreenMode(1376, 1068, (1376*bpp[i]+15)&~15, colspace[i]) );
	    m_cScreenModeList.push_back( ScreenMode(1440, 1112, (1440*bpp[i]+15)&~15, colspace[i]) );
	    m_cScreenModeList.push_back( ScreenMode(1536, 1156, (1536*bpp[i]+15)&~15, colspace[i]) );
	    m_cScreenModeList.push_back( ScreenMode(1600, 1200, (1600*bpp[i]+15)&~15, colspace[i]) );
	    m_cScreenModeList.push_back( ScreenMode(2048, 1536, (2048*bpp[i]+15)&~15, colspace[i]) );
	}

	m_hFrameBufferArea = create_area( "mill2_gramebuffer",(void**) &m_pFrameBufferBase, 32*1024*1024,
	    AREA_FULL_ACCESS, AREA_NO_LOCK );
	remap_area( m_hFrameBufferArea, (void*) (m_cPCIInfo.u.h0.nBase0 & PCI_ADDRESS_MEMORY_32_MASK) );
    }
    
    if( m_nPointerScheme == POINTER_MILLENIUM ) {
	m_cLastMousePosition = IPoint( 0, 0 );

//	SetMouseBitmap( g_anMouseImg, POINTER_WIDTH, POINTER_HEIGHT );
    }
    write_pci_config( m_cPCIInfo.nBus, m_cPCIInfo.nDevice, m_cPCIInfo.nFunction, 0x04, 4, 0x00000003 );
    m_bIsInitiated = true;

}

Millenium2::~Millenium2()
{
    if ( m_hRegisterArea != -1 ) {
	delete_area( m_hRegisterArea );
    }
    if( m_nCRTCScheme == CRTC_GX00 && m_hFrameBufferArea != -1 ) {
	delete_area( m_hFrameBufferArea );
    }
}

//-----------------------------------------------------------------------------

int Millenium2::Open()
{
    if( m_nCRTCScheme == CRTC_GX00 )
    {
	return m_hFrameBufferArea;
    }
    else
    {
	area_id nFrameBufferArea = VesaDriver::Open();
	if ( nFrameBufferArea < 0  ) {
	    dbprintf( "Millenium2::Open() Vesa driver failed to initialize\n" );
	    delete_area( m_hRegisterArea );
	    return( -1 );
	}
	// disable ISA IO, enable MemMapped IO
	// WritePCIConfig( sInfo.nBus, sInfo.nDevice, sInfo.nFunction, 0x04, 4, 0x00000002 );
	return( nFrameBufferArea );
    }
}

//-----------------------------------------------------------------------------

int Millenium2::GetScreenModeCount()
{
    if( m_nCRTCScheme == CRTC_GX00 )
	return m_cScreenModeList.size();
    else
	return VesaDriver::GetScreenModeCount();
}

bool Millenium2::GetScreenModeDesc( int nIndex, ScreenMode* psMode )
{
    if( m_nCRTCScheme == CRTC_GX00 )
    {
	if( nIndex<0 || nIndex>=int(m_cScreenModeList.size()) )
	    return false;
	*psMode = m_cScreenModeList[nIndex];
	return true;
    }
    else
    {
	return VesaDriver::GetScreenModeDesc( nIndex, psMode );
    }
}

int Millenium2::SetScreenMode( int nWidth, int nHeight, color_space eColorSpc,
			       int nPosH, int nPosV, int nSizeH, int nSizeV, float vRefreshRate )
{
    int nError = -1;

    printf( "Millenium2::SetScreenMode(vRefreshRate=%f)\n", vRefreshRate );

    // Anti appserver hack 
    if( vRefreshRate == 55 )
	vRefreshRate = 70;
    if( m_nCRTCScheme == CRTC_GX00 )
    {
	m_nCurrentMode = -1;
	for( uint i=0; i<m_cScreenModeList.size(); i++ )
	{
	    if( m_cScreenModeList[i].m_nWidth == nWidth &&
		m_cScreenModeList[i].m_nHeight == nHeight &&
		m_cScreenModeList[i].m_eColorSpace == eColorSpc )
	    {
		m_nCurrentMode = i;
		break;
	    }
        }
	if( m_nCurrentMode >= 0 )
	{
	    CGx00CRTC1 crtc( *this );
	    if( eColorSpc == CS_RGB15 )
	        nError = crtc.SetMode( nWidth, nHeight, 15, vRefreshRate ) ? 0 : -1;
	    if( eColorSpc == CS_RGB16 )
	        nError = crtc.SetMode( nWidth, nHeight, 16, vRefreshRate ) ? 0 : -1;
	    if( eColorSpc == CS_RGB32 )
	        nError = crtc.SetMode( nWidth, nHeight, 32, vRefreshRate ) ? 0 : -1;
	}
    }
    else
    {
        nError = VesaDriver::SetScreenMode( nWidth, nHeight, eColorSpc, nPosH, nPosV,
						nSizeH, nSizeV, vRefreshRate );
    }

    if( m_nPointerScheme == POINTER_MILLENIUM )
    {
	// The BIOS seems to screw around with the mouse registers :(
	SetMousePos( m_cLastMousePosition );

        // show the pointer:
        int tmp = inb(TVP3026_CURSOR_CTL)&0x6c;
        outb( TVP3026_INDEX, TVP3026_CURSOR_CTL );
        outb( TVP3026_DATA, tmp|0x13 );
    }
    
    return( nError );
}

int Millenium2::GetHorizontalRes()
{
    if( m_nCRTCScheme == CRTC_GX00 )
	return m_cScreenModeList[m_nCurrentMode].m_nWidth;
    else
	return VesaDriver::GetHorizontalRes();
}

int Millenium2::GetVerticalRes()
{
    if( m_nCRTCScheme == CRTC_GX00 )
	return m_cScreenModeList[m_nCurrentMode].m_nHeight;
    else
	return VesaDriver::GetVerticalRes();
}

int Millenium2::GetBytesPerLine()
{
    if( m_nCRTCScheme == CRTC_GX00 )
	return m_cScreenModeList[m_nCurrentMode].m_nBytesPerLine;
    else
	return VesaDriver::GetBytesPerLine();
}

color_space Millenium2::GetColorSpace()
{
    if( m_nCRTCScheme == CRTC_GX00 )
	return m_cScreenModeList[m_nCurrentMode].m_eColorSpace;
    else
	return VesaDriver::GetColorSpace();
}
 
//-----------------------------------------------------------------------------

void Millenium2::MouseOn( void )
{
    if( m_nPointerScheme == POINTER_SPRITE )
	VesaDriver::MouseOn();
}

void Millenium2::MouseOff( void )
{
    if( m_nPointerScheme == POINTER_SPRITE )
	VesaDriver::MouseOff();
}

void Millenium2::SetMousePos( IPoint cNewPos )
{
    if( m_nPointerScheme == POINTER_MILLENIUM )
    {
	int x = cNewPos.x - m_cCursorHotSpot.x + 64;
	int y = (cNewPos.y*2) - (m_cCursorHotSpot.y*2) + 64;
	m_cGELock.Lock(); // The pointer stuff is likely to be independent from the blitter stuff,
			  //  but just to be sure...
	outb( TVP3026_CUR_XLOW, x&0xff );
	outb( TVP3026_CUR_XHI, (x>>8)&0x0f );
	outb( TVP3026_CUR_YLOW, y&0xff );
	outb( TVP3026_CUR_YHI, (y>>8)&0x0f );
	m_cGELock.Unlock();
	
	m_cLastMousePosition = cNewPos;
    }
    else
	VesaDriver::SetMousePos( cNewPos );
}

void Millenium2::SetCursorBitmap( mouse_ptr_mode eMode, const IPoint& cHotSpot, const void* pRaster, int nWidth, int nHeight )
{
    m_cCursorHotSpot = cHotSpot;
    if( eMode == MPTR_MONO && m_nPointerScheme == POINTER_MILLENIUM ) {
	const uint8* bits = static_cast<const uint8*>(pRaster);
	m_cGELock.Lock();

	  // Disable cursor
        int tmp = inb(TVP3026_CURSOR_CTL)&0xfc;
        outb( TVP3026_INDEX, TVP3026_CURSOR_CTL );
        outb( TVP3026_DATA, tmp );
	
	
        tmp = inb(TVP3026_CURSOR_CTL)&0xf3;
        outb( TVP3026_INDEX, TVP3026_CURSOR_CTL );
        outb( TVP3026_DATA, tmp );
        outb( TVP3026_WADR_PAL, 0x00 );
        for( int imask=0x01; imask<=0x02; imask<<=1 )
        {
	    for( int iy=0; iy<64; iy++ )
	    {
	        for( int ix=0; ix<64; ix+=8 )
	        {
		    uint8 bit = 0;
		    for( int ix2=0; ix2<8; ix2++ )
		    {
		        int srcy = iy/2; // for some unknown reason, only every second scanline shows up,
					 // maybe it's an interlace thing?
		        uint8 col = ((ix+ix2)<nWidth && srcy < nHeight) ? bits[(ix+ix2)+srcy*nWidth] : 0x00;
		        bit |= ((col&imask)?1:0)<<(7-ix2);
		    }
		    while (inb(0x1fda) & 0x01);
		    while (!(inb(0x1fda) & 0x01));
		    outb( TVP3026_CUR_RAM, bit );
	        }
	    }
        }

        // set pointer colors:
        outb(TVP3026_CUR_COL_ADDR, 1);
        outb(TVP3026_CUR_COL_DATA, 0x00 );
        outb(TVP3026_CUR_COL_DATA, 0x00 );
        outb(TVP3026_CUR_COL_DATA, 0x00 );
        outb(TVP3026_CUR_COL_ADDR, 2);
        outb(TVP3026_CUR_COL_DATA, 0xff );
        outb(TVP3026_CUR_COL_DATA, 0xff );
        outb(TVP3026_CUR_COL_DATA, 0xff );

	  // Enable cursor
        tmp = inb(TVP3026_CURSOR_CTL)&0x6c;
        outb( TVP3026_INDEX, TVP3026_CURSOR_CTL );
        outb( TVP3026_DATA, tmp|0x13 );
	
	m_cGELock.Unlock();
	SetMousePos( m_cLastMousePosition );
    } else {
	VesaDriver::SetCursorBitmap( eMode, cHotSpot, pRaster, nWidth, nHeight );
    }
}

//-----------------------------------------------------------------------------

bool Millenium2::DrawLine( SrvBitmap* pcBitMap, const IRect& cClipRect,
			   const IPoint& cPnt1, const IPoint& cPnt2, const Color32_s& sColor, int nMode )
{
    if ( pcBitMap->m_bVideoMem == false || nMode != DM_COPY ) {
	return( VesaDriver::DrawLine( pcBitMap, cClipRect, cPnt1, cPnt2, sColor, nMode ) );
    }
    int nYDstOrg = 0; // Number of pixels (not bytes) from top of memory to frame buffer
    int nPitch = pcBitMap->m_nBytesPerLine ;
    uint32 nColor;
  
    uint32 nCtrl = M2_OPCODE_AUTOLINE_CLOSE | M2_ATYPE_RPL | M2_SOLID | M2_SHFTZERO | M2_BLTMOD_BFCOL | 0xc0000;


    int x1 = cPnt1.x;
    int y1 = cPnt1.y;
    int x2 = cPnt2.x;
    int y2 = cPnt2.y;
  
    IRect cBmRect( -32768, -32768, 32767, 32767 );
      // Clip the line to valid values
    if ( ClipLine( cBmRect, &x1, &y1, &x2, &y2 ) == false ) {
	return( false );
    }
  
    switch( pcBitMap->m_eColorSpc )
    {
//    case CS_CMAP8:
//      break;
	case CS_RGB15:
	    nColor = COL_TO_RGB15( sColor );
	    nColor |= nColor << 16;
	    nPitch /= 2;
	    break;
	case CS_RGB16:
	    nColor = COL_TO_RGB16( sColor );
	    nColor |= nColor << 16;
	    nPitch /= 2;
	    break;
	case CS_RGB24:
	    nColor = COL_TO_RGB32( sColor );
	    nPitch /= 3;
	    break;
	case CS_RGB32:
	    nColor = COL_TO_RGB32( sColor );
	    nPitch /= 4;
	    break;
	default:
	    nColor = 0;
	    dbprintf( "Millenium2::BltBitmap() invalid color space %d\n", pcBitMap->m_eColorSpc );
	    break;
    }
  
    m_cGELock.Lock();
  
    outl( M2_DWGCTL, nCtrl  );
    outl( M2_PITCH, nPitch );
  
      // Init the clipper
    outl( M2_CXBNDRY, cClipRect.left | (cClipRect.right << 16) );	// Left/right clipping boundary
    outl( M2_YTOP, cClipRect.top * nPitch + nYDstOrg );
    outl( M2_YBOT, cClipRect.bottom * nPitch + nYDstOrg );
      // Set line coordinates

    outl( M2_XYSTRT, (x1 & 0xffff) | ((y1 & 0xffff) << 16) );
    outl( M2_XYEND, (x2 & 0xffff) | ((y2 & 0xffff) << 16) );

    outl( M2_FCOL + 0x100, nColor );
  
    while( inl( M2_STATUS ) & M2_DWGENGSTS );
  
    m_cGELock.Unlock();
    return( true );
}

bool Millenium2::FillRect( SrvBitmap* pcBitMap, const IRect& cRect, const Color32_s& sColor )
{
    if ( pcBitMap->m_bVideoMem == false /*|| nMode != DM_COPY*/ ) {
	return( VesaDriver::FillRect( pcBitMap, cRect, sColor ) );
    }
      // Page 242
    int nHeight= cRect.Height() + 1;
    int nPitch = pcBitMap->m_nBytesPerLine;
    int nYDstOrg = 0; // Number of pixels (not bytes) from top of memory to frame buffer
    uint32 nColor;
  
      // Page 87/243
    uint32 nCtrl =
	M2_OPCODE_TRAP |
	M2_ATYPE_RPL |
	M2_SOLID |
	M2_ARZERO |
	M2_SGNZERO |
	M2_SHFTZERO |
	M2_BLTMOD_BMONOLEF |
	M2_TRANSC |
	0xc0000;

    m_cGELock.Lock();
    outl( M2_DWGCTL, nCtrl  );

    switch( pcBitMap->m_eColorSpc )
    {
//    case CS_CMAP8:
//      outl( M2_MACCESS, 0x00 );
//      break;
	case CS_RGB15:
	    nPitch /= 2;
	    outl( M2_MACCESS, 0x01 );
	    nColor = COL_TO_RGB15( sColor );
	    nColor |= nColor << 16;
	    break;
	case CS_RGB16:
	    nPitch /= 2;
	    outl( M2_MACCESS, 0x01 );
	    nColor = COL_TO_RGB16( sColor );
	    nColor |= nColor << 16;
	    break;
	case CS_RGB24:
	    nPitch /= 3;
	    outl( M2_MACCESS, 0x03 );
	    nColor = COL_TO_RGB32( sColor );
	    break;
	case CS_RGB32:
	    nPitch /= 4;
	    outl( M2_MACCESS, 0x02 );
	    nColor = COL_TO_RGB32( sColor );
	    break;
	default:
	    nColor = 0;
	    dbprintf( "Millenium2::BltBitmap() invalid color space %d\n", pcBitMap->m_eColorSpc );
	    break;
    }
    outl( M2_PITCH, nPitch );

    outl( M2_YDSTLEN, (cRect.top << 16) | nHeight );
    outl( M2_YDSTORG, nYDstOrg );
  
    outl( M2_CXBNDRY, 0 | ((pcBitMap->m_nWidth-1) << 16) );	// Left/right clipping boundary
    outl( M2_YTOP, 0 * nPitch + nYDstOrg );
    outl( M2_YBOT, (pcBitMap->m_nHeight-1) * nPitch + nYDstOrg );
    outl( M2_PLNWT, ~0L );
    outl( M2_FXBNDRY, cRect.left | ((cRect.right + 1) << 16) );
    outl( M2_FCOL + 0x100, nColor );
  
    while( inl( M2_STATUS ) & M2_DWGENGSTS );
    m_cGELock.Unlock();
    return( true );
  
}

bool Millenium2::BltBitmap( SrvBitmap* pcDstBitMap, SrvBitmap* pcSrcBitMap,
			    IRect cSrcRect, IPoint cDstPos, int nMode )
{
    if ( pcDstBitMap->m_bVideoMem == false || pcSrcBitMap->m_bVideoMem == false || nMode != DM_COPY ) {
	return( VesaDriver::BltBitmap( pcDstBitMap, pcSrcBitMap, cSrcRect, cDstPos, nMode ) );
    }
      // Page 242
    int nWidth = cSrcRect.Width() + 1;
    int nHeight= cSrcRect.Height() + 1;
    int nPitch = pcSrcBitMap->m_nBytesPerLine;
    int nYDstOrg = 0; // Number of pixels (not bytes) from top of memory to frame buffer
    uint32 nSign = 0;
  
      // Page 87/243
    uint32 nCtrl = M2_OPCODE_BITBLT | M2_ATYPE_RPL | M2_SHFTZERO | M2_BLTMOD_BFCOL | 0xc0000;

    switch( pcSrcBitMap->m_eColorSpc )
    {
	case CS_RGB15:
	case CS_RGB16:
	    nPitch /= 2;
	    break;
	case CS_RGB24:
	    nPitch /= 3;
	    break;
	case CS_RGB32:
	    nPitch /= 4;
	    break;
	default:
	    dbprintf( "Millenium2::BltBitmap() invalid color space %d\n", pcSrcBitMap->m_eColorSpc );
	    break;
    }
    int nSrcStartAddr = cSrcRect.left + cSrcRect.top * nPitch;
  
    m_cGELock.Lock();
  
    outl( M2_DWGCTL, nCtrl  );
    outl( M2_PITCH, nPitch );

    if ( cSrcRect.top < cDstPos.y ) {
	nSrcStartAddr = cSrcRect.left + (cSrcRect.bottom) * nPitch;
//	outl( M2_AR5, (-nPitch) & 0x3ffff );	// For Mil->G200
//	outl( M2_AR5, (-nPitch) & 0x3fffff );	// For G400
	outl( M2_AR5, (-nPitch) );		// Safe according to the manuals
	outl( M2_YDSTLEN, ((cDstPos.y + nHeight - 1) << 16) | nHeight );
	nSign |= M2_SGN_SDY; // Negative delta y
    } else {
	outl( M2_AR5, nPitch );
	outl( M2_YDSTLEN, (cDstPos.y << 16) | nHeight );
    }
    outl( M2_YDSTORG, nYDstOrg );

    switch( pcSrcBitMap->m_eColorSpc )
    {
	case CS_CMAP8:
	    outl( M2_MACCESS, 0x00 );
	    break;
	case CS_RGB15:
	case CS_RGB16:
	    outl( M2_MACCESS, 0x01 );
	    break;
	case CS_RGB24:
	    outl( M2_MACCESS, 0x03 );
	    break;
	case CS_RGB32:
	    outl( M2_MACCESS, 0x02 );
	    break;
	default:
	    dbprintf( "Millenium2::BltBitmap() invalid color space %d\n", pcSrcBitMap->m_eColorSpc );
	    break;
    }
    outl( M2_CXBNDRY, 0 | ((pcDstBitMap->m_nWidth-1) << 16) );	// Left/right clipping boundary
    outl( M2_YTOP, 0 * nPitch + nYDstOrg );
    outl( M2_YBOT, (pcDstBitMap->m_nHeight-1) * nPitch + nYDstOrg );
    outl( M2_PLNWT, ~0L );

    outl( M2_FXBNDRY, cDstPos.x | ((cDstPos.x + nWidth - 1) << 16) );

    if ( cSrcRect.left < cDstPos.x ) {
	outl( M2_AR0, nSrcStartAddr );
	outl( M2_AR3, nSrcStartAddr + nWidth - 1 );
	nSign |= M2_SGN_SCANLEFT;
    } else {
	outl( M2_AR3, nSrcStartAddr );
	outl( M2_AR0, nSrcStartAddr + nWidth - 1 );
    }
    outl( M2_SGN + 0x100, nSign );

    while( inl( M2_STATUS ) & M2_DWGENGSTS );
    m_cGELock.Unlock();
    return( true );
}

//-----------------------------------------------------------------------------
/*
uint8 Millenium2::ReadConfigBYTE( uint8 nReg )
{
}

uint32 Millenium2::ReadConfigDWORD( uint8 nReg )
{
}
*/

void Millenium2::WriteConfigBYTE( uint8 nReg, uint8 nValue )
{
    write_pci_config( m_cPCIInfo.nBus, m_cPCIInfo.nDevice, m_cPCIInfo.nFunction, nReg, 1, nValue );
}

void Millenium2::WriteConfigDWORD( uint8 nReg, uint32 nValue )
{
    write_pci_config( m_cPCIInfo.nBus, m_cPCIInfo.nDevice, m_cPCIInfo.nFunction, nReg, 4, nValue );
}

//-----------------------------------------------------------------------------

extern "C" DisplayDriver* init_gfx_driver()
{
    dbprintf( "mga_generic attempts to initialize\n" );
  
    try {
	Millenium2* pcDriver = new Millenium2();
	if ( pcDriver->IsInitiated() ) {
	    return( pcDriver );
	} else {
	    delete pcDriver;
	    return( NULL );
	}
    }
    catch( exception&  cExc ) {
       dbprintf( "Got exception\n" );
	return( NULL );
    }
}

//-----------------------------------------------------------------------------

