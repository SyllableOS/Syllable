/*
 *  SavageIX Driver for Syllable application server
 *  Copyright (C) 2003 Hilary Cheng (hilarycheng@yahoo.com)
 *
 *  Based on the S3 Savage XFre86 Driver and Savage DirectFB Driver
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
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
#include <posix/unistd.h>

#include <atheos/types.h>
#include <atheos/kernel.h>
#include <atheos/isa_io.h>
#include <appserver/pci_graphics.h>
#include "../../../server/bitmap.h"
#include "../../../server/sprite.h"

#include <atheos/pci.h>
#include "savage.h"

#include <gui/bitmap.h>

clone_info sCardInfo;

#define v_inb(a)	*((vuchar *)(sCardInfo.base0 + (a)))
#define v_outb(a, b)	*((vuchar *)(sCardInfo.base0 + (a))) = (b)
#define v_inl(a)	*((vuint32 *)(sCardInfo.base0 + (a)))
#define v_outl(a, l)	*((vuint32 *)(sCardInfo.base0 + (a))) = (l)

#define VGAOUT8(a, b)      *((vuchar *)(sCardInfo.base0 + a)) = b
#define VGAIN8(a)          *((vuchar *)(sCardInfo.base0 + a))
#define VGAOUT16(a, b)     *((vuint16*)(sCardInfo.base0 + a)) = b
#define VGAIN16(a)         *((vuint16*)(sCardInfo.base0 + a))

#define OUTREG(a, b)       *((uint32 *)(sCardInfo.base1 + a)) = b
#define INREG(a)           *((uint32 *)(sCardInfo.base1 + a))
#define OUTREG16(a, b)     *((vuint16*)(sCardInfo.base1 + a)) = b
#define INREG16(a)         *((vuint16*)(sCardInfo.base1 + a))

static area_id	g_nFrameBufArea = -1;
static area_id  g_nMMIOArea     = -1;

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

enum S3CHIPTAGS {
  S3_UNKNOWN = 0,
  S3_SAVAGE3D,
  S3_SAVAGE_MX,
  S3_SAVAGE4,
  S3_PROSAVAGE,
  S3_SUPERSAVAGE,
  S3_SAVAGE2000,
  S3_LAST
};

SavageDriver::SavageDriver( int nFd ): m_cGELock( "savage_ge_lock" )
{
    /* Get Info */
	if( ioctl( nFd, PCI_GFX_GET_PCI_INFO, &sCardInfo.pcii ) != 0 )
	{
		dbprintf( "Error: Failed to call PCI_GFX_GET_PCI_INFO\n" );
		return;
	}
	s3chip = S3_SAVAGE_MX;
	dbprintf( "Savage IX/MX VGA Card found\n");
  
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

SavageDriver::~SavageDriver()
{
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void SavageDriver::EnableMMIO( void )
{
  uint8 val;

  val = VGAIN8(0x3c3);
  VGAOUT8(0x3c3, val | 0x01);
  val = VGAIN8(VGA_MISC_OUT_R);
  VGAOUT8(VGA_MISC_OUT_W, val | 0x01);

  // FixME : Detect VGA IO Base as COLOR or MONO (pls refer to XFree86 Sources)
  vgaCRIndex = 0x3D0 + 4;
  vgaCRReg   = 0x3D0 + 5;

  if (s3chip >= S3_SAVAGE4) {
    VGAOUT8(vgaCRIndex, 0x40);
    val = VGAIN8(vgaCRReg);
    VGAOUT8(vgaCRReg, val | 1);
  }
}

area_id SavageDriver::Open( void )
{
    uint8    temp;
    uint8    config1;
    uint8    cr66;
    unsigned int FrameBufferBase;
    unsigned int MmioBase;

    if (s3chip >= S3_SAVAGE3D && s3chip <= S3_SAVAGE_MX) {
      FrameBufferBase = sCardInfo.pcii.u.h0.nBase0 & 0xffffff00;
      MmioBase = FrameBufferBase + SAVAGE_NEWMMIO_REGBASE_S3;
      dbprintf("PCI Base0 : %08X\n", sCardInfo.pcii.u.h0.nBase0);
      dbprintf("Savage Base : %08X %08X\n", MmioBase, FrameBufferBase);
    } else {
      MmioBase        = sCardInfo.pcii.u.h0.nBase0 & 0xffffff00 + SAVAGE_NEWMMIO_REGBASE_S4;
      FrameBufferBase = sCardInfo.pcii.u.h0.nBase1 & 0xffffff00;
    }

    m_pFrameBuffer = NULL;
    m_pMMIOBuffer  = NULL;

    g_nMMIOArea     = create_area("savage_io", (void**) &m_pMMIOBuffer, SAVAGE_NEWMMIO_REGSIZE,
				  AREA_FULL_ACCESS, AREA_NO_LOCK);

    g_nFrameBufArea = create_area("savage_frame",(void**) &m_pFrameBuffer, 1024 * 1024 * 16,
				  AREA_FULL_ACCESS, AREA_NO_LOCK );

    remap_area(g_nFrameBufArea, (void*) FrameBufferBase);
    remap_area(g_nMMIOArea,     (void*) MmioBase);

    m_pBCIBuffer = (uint8*) (m_pMMIOBuffer + 0x10000);

    // This is the pointer for memory mapped IO
    sCardInfo.base1 = (vuchar *) m_pMMIOBuffer;
    sCardInfo.base0 = (vuchar *) (m_pMMIOBuffer + 0x8000);	// little endian area
    sCardInfo.BciMem = (vuchar *) m_pBCIBuffer;

    // This is the pointer to the beginning of the video memory, as mapped in
    // the add-on memory adress space. This add-on puts the frame buffer just at
    // the beginning of the video memory.
    sCardInfo.scrnBase = (uchar *) m_pFrameBuffer;

    // Reset Start
    {
      uint8 v;

      VGAOUT8(0x3C4, 0x00);
      v = VGAIN8(0x3C5);

      VGAOUT8(0x3C4, 0x00);
      VGAOUT8(0x3C5, 0x01);
      VGAOUT8(0x3C4, 0x01);
      VGAOUT8(0x3C5, v | 0x20);
      VGAOUT8(0x3C4, 0x00);
      VGAOUT8(0x3C5, 0x03);
    }

    EnableMMIO();

    // Initialize the s3 chip in a reasonable configuration. Turn the chip on and
    // unlock all the registers we need to access (and even more...). Do a few
    // standard vga config. Then we're ready to identify the DAC.

    // Unprotect CRTC[0-7]
    VGAOUT8(vgaCRIndex, 0x11);
    temp = VGAIN8(vgaCRReg);
    VGAOUT8(vgaCRReg, temp & 0x7f);

    // Unlock extends register
    VGAOUT16(vgaCRIndex, 0x4838);
    VGAOUT16(vgaCRIndex, 0xA039);
    VGAOUT16(     0x3c4, 0x0608);

    VGAOUT8(vgaCRIndex, 0x40);
    temp = VGAIN8(vgaCRReg);
    VGAOUT8(vgaCRReg, temp & ~0x01);

    // Unlock System Register
    VGAOUT8(vgaCRIndex, 0x38);
    VGAOUT8(vgaCRReg,   0x48);

    // FixME Gramme Setting ??

    // Get Video RAM
    VGAOUT16(vgaCRIndex, 0x4838);

    VGAOUT8(vgaCRIndex, 0x36);
    config1 = VGAIN8(vgaCRReg);

    {
      static unsigned char RamSavageMX[] = {2, 8, 4, 16, 8, 16, 4, 16};

      sCardInfo.theMem = RamSavageMX[(config1 & 0x0E) >> 1] * 1024;
      dbprintf("Savage VGA Card Memory Size : %d\n", sCardInfo.theMem);

      sCardInfo.cobSize  = 1 << 17;
      sCardInfo.cobIndex = 7;
      sCardInfo.cobOffset = sCardInfo.theMem * 1024 - sCardInfo.cobSize;
      sCardInfo.CursorKByte = (sCardInfo.cobOffset >> 10)  - 4;
    }

    // Reset Engine to Avoid Memory Corruption
    VGAOUT8(vgaCRIndex, 0x66);
    cr66 = VGAIN8(vgaCRReg);
    VGAOUT8(vgaCRReg, cr66 | 0x02);
    usleep(10000);

    // Clear Reset Flag
    VGAOUT8(vgaCRIndex, 0x66);
    VGAOUT8(vgaCRReg, cr66 & ~0x02);
    usleep(10000);

    // Detect current mclk
    {
      float mclk;
      uint8 sr8, n, m, n1, n2;
      
      v_outb(0x83c4, 0x08);
      sr8 = v_inb(0x83c5);
      v_outb(0x83c5, 0x06);
      v_outb(0x83c4, 0x10);
      n = v_inb(0x83c5);
      v_outb(0x83c4, 0x11);
      m = v_inb(0x83c5);
      v_outb(0x83c4, 0x08);
      v_outb(0x83c5, sr8);

      m &= 0x7f;
      n1 = n & 0x1f;
      n2 = (n >> 5) & 0x03;
      mclk = ((1431818 * (m + 2)) / (n1 + 2) / (1 << n2) + 50) / 100;
    }

    dbprintf( "Available VideMemory: %d\n", sCardInfo.theMem );

    /*
    float rf[] = { 60.0f, 75.0f, 85.0f };
    for( int i = 0; i < 3; i++ )
    {

    if ( sCardInfo.theMem >= 1024 ) {
	m_cModes.push_back( os::screen_mode( 640,  480,  640,  CS_CMAP8, rf[i] ) );
	m_cModes.push_back( os::screen_mode( 800,  600,  800,  CS_CMAP8, rf[i] ) );
	m_cModes.push_back( os::screen_mode( 1024, 768,  1024, CS_CMAP8, rf[i] ) );
	m_cModes.push_back( os::screen_mode( 1152, 900,  1152, CS_CMAP8, rf[i] ) );
      
	m_cModes.push_back( os::screen_mode( 640,  480,  640*2,  CS_RGB16, rf[i] ) );
	m_cModes.push_back( os::screen_mode( 800,  600,  800*2,  CS_RGB16, rf[i] ) );
    }
    if ( sCardInfo.theMem >= 2048 ) {
	m_cModes.push_back( os::screen_mode( 1280, 1024, 1280, CS_CMAP8, rf[i] ) );
	m_cModes.push_back( os::screen_mode( 1600, 1200, 1600, CS_CMAP8, rf[i] ) );

	m_cModes.push_back( os::screen_mode( 1024, 768,  1024*2, CS_RGB16, rf[i] ) );
	m_cModes.push_back( os::screen_mode( 1152, 900,  1152*2, CS_RGB16, rf[i] ) );
    }
    if ( sCardInfo.theMem >= 4096 ) {
	m_cModes.push_back( os::screen_mode( 1280, 1024, 1280*2, CS_RGB16, rf[i] ) );
	m_cModes.push_back( os::screen_mode( 1600, 1200, 1600*2, CS_RGB16, rf[i] ) );
    }
    }
    */

    sCardInfo.scrnBufBase = (uchar*) m_pFrameBuffer + (sCardInfo.theMem * 1024) - 1024;

    dbprintf("   scrnBase: 0x%p\nscrnBufBase: 0x%p\n", sCardInfo.scrnBase, sCardInfo.scrnBufBase);
  
    dbprintf( "Gfx card initiated\n" );

    m_nFrameBufferSize = sCardInfo.theMem * 1024;

    // VesaDriver::Open();
  
    VesaDriver::Open();

    dbprintf("Vesa Mode Initied\n");

    return( g_nFrameBufArea );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void SavageDriver::Close( void )
{
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------
void SavageDriver::Initialize2DEngine( void )
{
  VGAOUT16(vgaCRIndex, 0x0140);
  VGAOUT8(vgaCRIndex, 0x31);
  VGAOUT8(vgaCRReg, 0x0C);

  /* Setup plane mask */
  OUTREG(0x8128, ~0); /* enable all write planes */
  OUTREG(0x812C, ~0); /* enable all read planes */
  OUTREG16(0x8134, 0x27);
  OUTREG16(0x8136, 0x07);

  switch (s3chip) {
  case S3_SAVAGE_MX:
    /* Disable BCI */
    OUTREG(0x48C18, INREG(0x48C18) & 0x3FF0);
    /* Setup BCI command overflow buffer */
    OUTREG(0x48C14, (sCardInfo.cobOffset >> 11) | (sCardInfo.cobIndex << 29));
    /* Program Shadow Status Update */
    OUTREG(0x48C10, 0x78207220);
    OUTREG(0x48C0C, 0);
    OUTREG(0x48C18, INREG(0x48C18) | 0x0C);
    break;
  }

  SetGBD();
}

void SavageDriver::SetGBD( void )
{
  uint32 GlobalBitmapDescriptor;

  GlobalBitmapDescriptor = 1 | 8 | BCI_BD_BW_DISABLE;
  BCI_BD_SET_BPP(GlobalBitmapDescriptor, sCardInfo.scrnColors);
  BCI_BD_SET_STRIDE(GlobalBitmapDescriptor, sCardInfo.scrnWidth);

  /* Turn on 16-bit register access */
  VGAOUT8(vgaCRIndex, 0x31);
  VGAOUT8(vgaCRReg,   0x0C);

  /* Set stride to use GBD */
  VGAOUT8(vgaCRIndex, 0x50);
  VGAOUT8(vgaCRReg, VGAIN8(vgaCRReg) | 0xC1);

  /* Enable 2D Engine */
  VGAOUT16(vgaCRIndex, 0x0140);

  /* Now set the GBD and SBDs */
  OUTREG(0x8168, 0);
  OUTREG(0x816C, GlobalBitmapDescriptor);
  OUTREG(0x8170, 0);
  OUTREG(0x8174, GlobalBitmapDescriptor);
  OUTREG(0x8178, 0);
  OUTREG(0x817C, GlobalBitmapDescriptor);

  OUTREG(PRI_STREAM_STRIDE, sCardInfo.scrnWidth * sCardInfo.scrnColors >> 3);
  OUTREG(SEC_STREAM_STRIDE, sCardInfo.scrnWidth * sCardInfo.scrnColors >> 3);
}

int SavageDriver::SetScreenMode( os::screen_mode sMode )
{
  int result;

    sCardInfo.crtPosH     = (int)sMode.m_vHPos;
    sCardInfo.crtSizeH    = (int)sMode.m_vHSize;
    sCardInfo.crtPosV     = (int)sMode.m_vVPos;
    sCardInfo.crtSizeV    = (int)sMode.m_vVSize;
    sCardInfo.scrnRate	  = sMode.m_vRefreshRate;
    sCardInfo.scrnWidth   = sMode.m_nWidth;
    sCardInfo.scrnHeight  = sMode.m_nHeight;
    sCardInfo.scrnRowByte = sMode.m_nWidth;
    sCardInfo.colorspace  = sMode.m_eColorSpace;

    switch( sMode.m_eColorSpace )
    {
    case CS_CMAP8:
      sCardInfo.scrnColors = 8;
      break;
    case CS_RGB16:
      sCardInfo.scrnColors = 16;
      sCardInfo.scrnRowByte <<= 1;
      break;
    case CS_RGB24:
      sCardInfo.scrnColors = 24;
      break;
    case CS_RGB32:
      sCardInfo.scrnColors = 32;
      break;
    default:
      dbprintf( "savage_set_creen_mode() invalide color dept %d\n", sCardInfo.scrnColors );
      return( -1 );
    }

    /*
    if ( sMode.m_nWidth == 640 && sMode.m_nHeight == 480 ) {
	sCardInfo.scrnResNum = 0;
    } else if ( sMode.m_nWidth == 800 && sMode.m_nHeight == 600 ) {
	sCardInfo.scrnResNum = 1;
    } else if ( sMode.m_nWidth == 1024 && sMode.m_nHeight == 768 ) {
       sCardInfo.scrnResNum = 2;
    } else if ( sMode.m_nWidth == 1152 && sMode.m_nHeight == 900 ) {
	sCardInfo.scrnResNum = 2;
    } else if ( sMode.m_nWidth == 1280 && sMode.m_nHeight == 1024 ) {
	sCardInfo.scrnResNum = 3;
    } else if ( sMode.m_nWidth == 1600 && sMode.m_nHeight == 1200 ) {
	sCardInfo.scrnResNum = 4;
    } else  {
	dbprintf( "Invalid resolution %d-%d\n", sMode.m_nWidth, sMode.m_nHeight );
	return( -1 );
    }
    if ( sCardInfo.scrnColors == 16 ) {
	sCardInfo.scrnResNum += 5;
    } else if ( sCardInfo.scrnColors == 32 ) {
	sCardInfo.scrnResNum += 10;
    }
  
    sCardInfo.offscrnWidth  = sCardInfo.scrnWidth;
    sCardInfo.offscrnHeight = sCardInfo.scrnHeight;
    sCardInfo.scrnPosH = 0;
    sCardInfo.scrnPosV = 0;
    
    m_sCurrentMode = sMode;
    m_sCurrentMode.m_nBytesPerLine = sMode.m_nWidth * sCardInfo.scrnColors / 8;
    */

    dbprintf("Setting mode: %dx%dx%d@%f\n",
	     sCardInfo.scrnWidth, sCardInfo.scrnHeight, sCardInfo.scrnColors, sCardInfo.scrnRate);

    {
      uint8 v;

      VGAOUT8(0x3C4, 0x00);
      v = VGAIN8(0x3C5);

      VGAOUT8(0x3C4, 0x00);
      VGAOUT8(0x3C5, 0x01);
      VGAOUT8(0x3C4, 0x01);
      VGAOUT8(0x3C5, v & ~0x20);
      VGAOUT8(0x3C4, 0x00);
      VGAOUT8(0x3C5, 0x03);
    }

    result = VesaDriver::SetScreenMode(sMode);

    VGAOUT16(vgaCRIndex, 0x4838);
    VGAOUT16(vgaCRIndex, 0xA039);
    VGAOUT16(0x3c4, 0x0608);
    
    /* Enable Linear Addressing */
    VGAOUT16(vgaCRIndex, 0x1358);

    /* Disable old MMIO */
    VGAOUT8(vgaCRIndex, 0x53);
    VGAOUT8(vgaCRReg, VGAIN8(vgaCRReg) & ~0x10);

    /* Set Color Mode */
    VGAOUT8(vgaCRIndex, 0x67);
    switch (sCardInfo.scrnColors) {
    case 8:
      VGAOUT8(vgaCRReg, 0x00);
      break;
    case 15:
      VGAOUT8(vgaCRReg, 0x30);
      break;
    case 16:
      VGAOUT8(vgaCRReg, 0x50);
      break;
    case 24:
      VGAOUT8(vgaCRReg, 0x70);
      break;
    case 32:
      VGAOUT8(vgaCRReg, 0xD0);
      break;
    }

    /* Make sure 16-bit memory access is enabled */
    VGAOUT16(vgaCRIndex, 0x0c31);

    /* Enable the graphics engine */
    VGAOUT16(vgaCRIndex, 0x0140);

    /* handle the pitch */
    VGAOUT8(vgaCRIndex, 0x50);
    VGAOUT8(vgaCRReg, VGAIN8(vgaCRReg) | 0xC1);

    {
      uint16 width = (sMode.m_nWidth * (sCardInfo.scrnColors / 8)) >> 3;
      dbprintf("VGA OUT Port Width %d\n", width);
      VGAOUT16(vgaCRIndex, ((width & 0xff)  << 8) | 0x13);
      VGAOUT16(vgaCRIndex, ((width & 0x300) << 4) | 0x51);
    }

    dbprintf("Init 2D Engine\n");
    Initialize2DEngine();
    dbprintf("Set Global Bitmap Descriptor\n");
    SetGBD();
    VGAOUT16(vgaCRIndex, 0x0140);
    dbprintf("Set Global Bitmap Descriptor\n");
    SetGBD();

    return result;
}

int SavageDriver::GetScreenModeCount()
{
  return VesaDriver::GetScreenModeCount();
}

bool SavageDriver::GetScreenModeDesc( int nIndex, os::screen_mode* psMode )
{
  return VesaDriver::GetScreenModeDesc(nIndex, psMode);
}

os::screen_mode SavageDriver::GetCurrentScreenMode()
{
  return VesaDriver::GetCurrentScreenMode();
}


//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------
void SavageDriver::SetCursorBitmap( os::mouse_ptr_mode eMode, const os::IPoint& cHotSpot,
				    const void* pRaster, int nWidth, int nHeight )
{
  VesaDriver::SetCursorBitmap(eMode, cHotSpot, pRaster, nWidth, nHeight);
}

void SavageDriver::MouseOn()
{
  VesaDriver::MouseOn();
}

void SavageDriver::MouseOff()
{
  VesaDriver::MouseOff();
}

void SavageDriver::SetMousePos( os::IPoint cNewPos )
{
  VesaDriver::SetMousePos( cNewPos );
}

bool SavageDriver::IntersectWithMouse( const IRect& cRect )
{
  return( false );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void SavageDriver::WaitQueue( int v )
{
  int loop  = 0;
  int slots = MAXFIFO - v;

  loop &= STATUS_WORD0;

  while (((STATUS_WORD0 & 0x0000FFFF) > slots) && (loop < MAXLOOP))
    loop++;
}

void SavageDriver::WaitIdle( void )
{
  int loop = 0;

  loop &= STATUS_WORD0;

  while( ((STATUS_WORD0 & 0x0008FFFF) != 0x80000) && (loop < MAXLOOP) )
    loop++;
}

bool SavageDriver::DrawLine( SrvBitmap* psBitMap, const IRect& cClipRect,
			     const IPoint& cPnt1, const IPoint& cPnt2,
			     const Color32_s& sColor, int nMode )
{
  volatile unsigned int *bci_ptr = (unsigned int *) sCardInfo.BciMem;
  uint32 cmd;
  int dx, dy;
  int min, max, xp, yp, ym;
  uint32 nColor;

  if ( psBitMap->m_bVideoMem && nMode == DM_COPY ) {
    int x1 = cPnt1.x;
    int y1 = cPnt1.y;
    int x2 = cPnt2.x;
    int y2 = cPnt2.y;

    m_cGELock.Lock();

    if (psBitMap->m_eColorSpc == CS_RGB32) {
      nColor = COL_TO_RGB32(sColor);
    } else {
      // we only support CS_RGB32 and CS_RGB16
      nColor = COL_TO_RGB16(sColor);
    }

    dx = x2 - x1;
    dy = y2 - y1;

    xp = (dx >= 0);
    if (!xp) dx = -dx;

    yp = (dy >= 0);
    if (!yp) dy = -dy;

    ym = (dy > dx);
    if (ym) {
      max = dy; min = dx;
    } else {
      max = dx; min = dy;
    }

    cmd = (0x58000000 | BCI_CMD_CLIP_CURRENT |
	   BCI_CMD_RECT_XP | BCI_CMD_RECT_YP |
	   BCI_CMD_DEST_GBD | BCI_CMD_SRC_SOLID |
	   (0xcc << 16));

    WaitQueue( 9 );

    BCI_SEND( BCI_CMD_NOP | BCI_CMD_CLIP_NEW );
    BCI_SEND( BCI_CLIP_TL( cClipRect.top, cClipRect.left ) );
    BCI_SEND( BCI_CLIP_BR( cClipRect.bottom, cClipRect.right ) );

    BCI_SEND( BCI_CMD_NOP | BCI_CMD_SEND_COLOR );
    BCI_SEND( nColor );

    BCI_SEND( cmd );
    BCI_SEND( BCI_LINE_X_Y( x1, y1 ) );
    BCI_SEND( BCI_LINE_STEPS( 2 * (min - max), 2 * min ) );
    BCI_SEND( BCI_LINE_MISC( max, ym, xp, yp, 2 * min - max ) );

    WaitIdle( );

    m_cGELock.Unlock();

    return true;
  } else {
    return DisplayDriver::DrawLine( psBitMap, cClipRect, cPnt1, cPnt2, sColor, nMode );
  }
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

bool SavageDriver::FillRect( SrvBitmap* psBitMap, const IRect& cRect, const Color32_s& sColor )
{
  uint32 nColor;

  if (psBitMap->m_eColorSpc == CS_RGB32) {
    nColor = COL_TO_RGB32(sColor);
  } else {
    // we only support CS_RGB32 and CS_RGB16
    nColor = COL_TO_RGB16(sColor);
  }

  if ( psBitMap->m_bVideoMem ) {
    volatile unsigned int *bci_ptr = (unsigned int *) sCardInfo.BciMem;
    uint32 command;

    m_cGELock.Lock();

    WaitQueue( 5 );

    command = BCI_CMD_RECT | BCI_CMD_RECT_XP | BCI_CMD_RECT_YP | BCI_CMD_DEST_GBD |
      BCI_CMD_SRC_SOLID | (0x00CC << 16);

    BCI_SEND( BCI_CMD_NOP | BCI_CMD_SEND_COLOR );
    BCI_SEND( nColor );

    BCI_SEND( command );
    BCI_SEND( BCI_X_Y( cRect.left, cRect.top ) );
    BCI_SEND( BCI_W_H( cRect.Width() + 1, cRect.Height() + 1 ) );

    WaitIdle( );

    m_cGELock.Unlock();

    return true;
  } else {
    return( DisplayDriver::FillRect( psBitMap, cRect, sColor ) );
  }
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

bool SavageDriver::BltBitmap(SrvBitmap* dstbm, SrvBitmap* srcbm, IRect cSrcRect,
			     IPoint cDstPos, int nMode)
{
  if (dstbm->m_bVideoMem == false || srcbm->m_bVideoMem == false || nMode != DM_COPY)
    {
      return( VesaDriver::BltBitmap( dstbm, srcbm, cSrcRect, cDstPos, nMode ) );
    }

  {
    volatile unsigned int *bci_ptr = (unsigned int *) sCardInfo.BciMem;
    uint32 command;
    int x1 = cSrcRect.left;
    int y1 = cSrcRect.top;
    int x2 = cDstPos.x;
    int y2 = cDstPos.y;
    int width  = cSrcRect.Width() + 1;
    int height = cSrcRect.Height() + 1;

    // Check degenerated blit (source == destination)
    if ( x1 == x2 && y1 == y2 ) {
      return( VesaDriver::BltBitmap( dstbm, srcbm, cSrcRect, cDstPos, nMode ) );
    }

    m_cGELock.Lock();

    command = (BCI_CMD_RECT | BCI_CMD_DEST_GBD | BCI_CMD_SRC_GBD | (0x00CC << 16));

    if (x2 <= x1) {
      command |= BCI_CMD_RECT_XP;
    } else {
      x2 += (width - 1);
      x1 += (width - 1);
    }

    if (y2 <= y1) {
      command |= BCI_CMD_RECT_YP;
    } else {
      y2 += (height - 1);
      y1 += (height - 1);
    }

    WaitQueue( 4 );

    BCI_SEND( command );
    BCI_SEND( BCI_X_Y( x1, y1 ) );
    BCI_SEND( BCI_X_Y( x2, y2 ) );
    BCI_SEND( BCI_W_H( width, height ) );

    WaitIdle( );

    m_cGELock.Unlock();

    return true;
  }
}



//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void UpdateScreenMode()
{
    dbprintf( "Update screen mode\n" );
}

extern "C" DisplayDriver* init_gfx_driver( int nFd )
{
    dbprintf( "s3_savage attempts to initialize\n" );

    try {
	DisplayDriver* pcDriver = new SavageDriver( nFd );
	return( pcDriver );
    }
    catch( exception&  cExc ) {
	return( NULL );
    }
}
