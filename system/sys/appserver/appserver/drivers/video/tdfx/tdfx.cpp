
/*
 *  Graphics driver for Voodoo Banshee, Voodoo 3 & Voodoo 5 chipsets
 *  (appserver driver),
 *
 *  based on the Linux framebuffer driver (tdfxfb.c),
 *    Copyright (C) 1999 Hannu Mallat
 *
 *  and Arno Klenke's port of the geforcefx driver,
 *    Copyright (C) 2003 Arno Klenke
 *
 *  Copyright (C) 2004 Jan L. Hauffa
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
 *
 */


#include <string.h>
#include <stdlib.h>

#include <atheos/types.h>
#include <atheos/pci.h>
#include <atheos/kernel.h>
#include <appserver/pci_graphics.h>
#include <gui/bitmap.h>

#include "../../../server/bitmap.h"
#include "../../../server/sprite.h"

#include "tdfx.h"
#include "gtf.h"

using namespace os;


TDFX::TDFX( int nFd ):m_cGELock( "tdfx_ge_lock" )
{
	PCI_Info_s cPCIInfo;

	m_bIsInitiated = false;
	m_pRegBase = m_pFrameBufferBase = NULL;

	if( ioctl( nFd, PCI_GFX_GET_PCI_INFO, &cPCIInfo ) != 0 )
	{
		dbprintf( "TDFX: Failed to call PCI_GFX_GET_PCI_INFO\n" );
		return;
	}

	m_nChipsetId = 0;
	uint32 ChipsetId = ( cPCIInfo.nVendorID << 16 ) | cPCIInfo.nDeviceID;

	for( int i = 0; i < NUM_SUPPORTED; i++ )
		if( asSupportedChips[i].nId == ChipsetId )
		{
			m_nChipsetId = asSupportedChips[i].nId;
			m_nMaxPixclock = asSupportedChips[i].nMaxPixclock;
		}

	if( m_nChipsetId == 0 )
	{
		dbprintf( "TDFX: No supported cards found\n" );
		return;
	}

	int nMmioSize = get_pci_memory_size( nFd, &cPCIInfo, 0 );

	m_hRegArea = create_area( "tdfx_mmio", ( void ** )&m_pRegBase, nMmioSize,
        AREA_FULL_ACCESS, AREA_NO_LOCK );
	remap_area( m_hRegArea,
        ( void * )( cPCIInfo.u.h0.nBase0 & PCI_ADDRESS_MEMORY_32_MASK ) );

	m_nFbSize = get_lfb_size();
	dbprintf( "TDFX: using %u kB of video memory\n",
              ( unsigned int )m_nFbSize >> 10 );

	m_hFrameBufferArea = create_area( "tdfx_framebuffer",
        ( void ** )&m_pFrameBufferBase, m_nFbSize, AREA_FULL_ACCESS | AREA_WRCOMB,
        AREA_NO_LOCK );
	m_nFbBasePhys = cPCIInfo.u.h0.nBase1 & PCI_ADDRESS_MEMORY_32_MASK;
	remap_area( m_hFrameBufferArea, ( void * )m_nFbBasePhys );

	os::color_space colspace[] = { CS_RGB16, CS_RGB32 };
	int bsize[] = { 2, 4 };
	float rf[] = { 60.0f, 75.0f, 85.0f, 100.0f };
	for( int i = 0; i < 2; i++ )	// color space
		for( int j = 0; j < 4; j++ )	// refresh rate
		{
			m_cScreenModeList.push_back(
                os::screen_mode( 640, 480, ( 640 * bsize[i] + 15 ) & ~15,
                    colspace[i], rf[j] ) );
			m_cScreenModeList.push_back(
                os::screen_mode( 800, 600, ( 800 * bsize[i] + 15 ) & ~15,
                    colspace[i], rf[j] ) );
			m_cScreenModeList.push_back(
                os::screen_mode( 1024, 768, ( 1024 * bsize[i] + 15 ) & ~15,
                    colspace[i], rf[j] ) );
			m_cScreenModeList.push_back(
                os::screen_mode( 1280, 1024, ( 1280 * bsize[i] + 15 ) & ~15,
                    colspace[i], rf[j] ) );
			if( ( j < 3 ) || ( m_nChipsetId != ID_3DFX_BANSHEE ) )
				m_cScreenModeList.push_back(
                    os::screen_mode( 1600, 1200, ( 1600 * bsize[i] + 15 ) & ~15,
                        colspace[i], rf[j] ) );
		}

	m_bVideoOverlayUsed = false;
	m_bIsInitiated = true;
}


TDFX::~TDFX()
{
}


area_id TDFX::Open()
{
	return m_hFrameBufferArea;
}


void TDFX::Close()
{
}


int TDFX::GetScreenModeCount()
{
	return m_cScreenModeList.size();
}


bool TDFX::GetScreenModeDesc( int nIndex, os::screen_mode * psMode )
{
	if( ( nIndex < 0 ) || ( uint ( nIndex ) >= m_cScreenModeList.size() ) )
		return false;

	*psMode = m_cScreenModeList[nIndex];
	return true;
}


int TDFX::SetScreenMode( os::screen_mode sMode )
{
	struct tdfx_reg reg;
	uint32 hdispend, hsyncsta, hsyncend, htotal;
	uint32 hd, hs, he, ht, hbs, hbe;
	uint32 vd, vs, ve, vt, vbs, vbe;
	uint32 wd;
	int fout, freq, cpp;
	TimingParams t;

	memset( &reg, 0, sizeof( reg ) );

	cpp = bytesPerPixel[sMode.m_eColorSpace];
	if( cpp == -1 )
	{
		dbprintf( "TDFX: color space %d not supported\n", sMode.m_eColorSpace );
		return -EINVAL;
	}
	sMode.m_nBytesPerLine = cpp * sMode.m_nWidth;

	reg.vidcfg = VIDCFG_VIDPROC_ENABLE | VIDCFG_DESK_ENABLE | VIDCFG_CURS_X11 |
        ( ( cpp - 1 ) << VIDCFG_PIXFMT_SHIFT ) |
        ( cpp != 1 ? VIDCFG_DESK_CLUT_BYPASS : 0 );


	gtf_get_mode( sMode.m_nWidth, sMode.m_nHeight, int ( sMode.m_vRefreshRate ),
        &t );

	if( ( ( int32 )PICOS2KHZ( t.pixclock ) ) > m_nMaxPixclock )
	{
		dbprintf( "TDFX: pixclock too high (%ldKHz)\n", PICOS2KHZ( t.pixclock ) );
		return -EINVAL;
	}


	/* PLL settings */
	freq = PICOS2KHZ( t.pixclock );

	reg.dacmode = 0;
	reg.vidcfg &= ~VIDCFG_2X;

	hdispend = sMode.m_nWidth;
	hsyncsta = hdispend + t.right_margin;
	hsyncend = hsyncsta + t.hsync_len;
	htotal = hsyncend + t.left_margin;

	if( freq > m_nMaxPixclock / 2 )
	{
		freq = freq > m_nMaxPixclock ? m_nMaxPixclock : freq;
		reg.dacmode |= DACMODE_2X;
		reg.vidcfg |= VIDCFG_2X;
		hdispend >>= 1;
		hsyncsta >>= 1;
		hsyncend >>= 1;
		htotal >>= 1;
	}

	hd = wd = ( hdispend >> 3 ) - 1;
	hs = ( hsyncsta >> 3 ) - 1;
	he = ( hsyncend >> 3 ) - 1;
	ht = ( htotal >> 3 ) - 1;
	hbs = hd;
	hbe = ht;

	vbs = vd = sMode.m_nHeight - 1;
	vs = vd + t.lower_margin;
	ve = vs + t.vsync_len;
	vbe = vt = ve + t.upper_margin - 1;


	/* this is all pretty standard VGA register stuffing */
	reg.misc[0x00] = 0x0f | ( sMode.m_nWidth < 400 ? 0xa0 :
        sMode.m_nWidth < 480 ? 0x60 : sMode.m_nWidth < 768 ? 0xe0 : 0x20 );

	reg.gra[0x00] = 0x00;
	reg.gra[0x01] = 0x00;
	reg.gra[0x02] = 0x00;
	reg.gra[0x03] = 0x00;
	reg.gra[0x04] = 0x00;
	reg.gra[0x05] = 0x40;
	reg.gra[0x06] = 0x05;
	reg.gra[0x07] = 0x0f;
	reg.gra[0x08] = 0xff;

	reg.att[0x00] = 0x00;
	reg.att[0x01] = 0x01;
	reg.att[0x02] = 0x02;
	reg.att[0x03] = 0x03;
	reg.att[0x04] = 0x04;
	reg.att[0x05] = 0x05;
	reg.att[0x06] = 0x06;
	reg.att[0x07] = 0x07;
	reg.att[0x08] = 0x08;
	reg.att[0x09] = 0x09;
	reg.att[0x0a] = 0x0a;
	reg.att[0x0b] = 0x0b;
	reg.att[0x0c] = 0x0c;
	reg.att[0x0d] = 0x0d;
	reg.att[0x0e] = 0x0e;
	reg.att[0x0f] = 0x0f;
	reg.att[0x10] = 0x41;
	reg.att[0x11] = 0x00;
	reg.att[0x12] = 0x0f;
	reg.att[0x13] = 0x00;
	reg.att[0x14] = 0x00;

	reg.seq[0x00] = 0x03;
	reg.seq[0x01] = 0x01;	/* fixme: clkdiv2? */
	reg.seq[0x02] = 0x0f;
	reg.seq[0x03] = 0x00;
	reg.seq[0x04] = 0x0e;

	reg.crt[0x00] = ht - 4;
	reg.crt[0x01] = hd;
	reg.crt[0x02] = hbs;
	reg.crt[0x03] = 0x80 | ( hbe & 0x1f );
	reg.crt[0x04] = hs;
	reg.crt[0x05] = ( ( hbe & 0x20 ) << 2 ) | ( he & 0x1f );
	reg.crt[0x06] = vt;
	reg.crt[0x07] = ( ( vs & 0x200 ) >> 2 ) | ( ( vd & 0x200 ) >> 3 ) |
        ( ( vt & 0x200 ) >> 4 ) | 0x10 | ( ( vbs & 0x100 ) >> 5 ) |
        ( ( vs & 0x100 ) >> 6 ) | ( ( vd & 0x100 ) >> 7 ) |
        ( ( vt & 0x100 ) >> 8 );
	reg.crt[0x08] = 0x00;
	reg.crt[0x09] = 0x40 | ( ( vbs & 0x200 ) >> 4 );
	reg.crt[0x0a] = 0x00;
	reg.crt[0x0b] = 0x00;
	reg.crt[0x0c] = 0x00;
	reg.crt[0x0d] = 0x00;
	reg.crt[0x0e] = 0x00;
	reg.crt[0x0f] = 0x00;
	reg.crt[0x10] = vs;
	reg.crt[0x11] = ( ve & 0x0f ) | 0x20;
	reg.crt[0x12] = vd;
	reg.crt[0x13] = wd;
	reg.crt[0x14] = 0x00;
	reg.crt[0x15] = vbs;
	reg.crt[0x16] = vbe + 1;
	reg.crt[0x17] = 0xc3;
	reg.crt[0x18] = 0xff;

	/* Banshee's nonvga stuff */
	reg.ext[0x00] = ( ( ( ht & 0x100 ) >> 8 ) | ( ( hd & 0x100 ) >> 6 ) |
        ( ( hbs & 0x100 ) >> 4 ) | ( ( hbe & 0x40 ) >> 1 ) |
        ( ( hs & 0x100 ) >> 2 ) | ( ( he & 0x20 ) << 2 ) );
	reg.ext[0x01] = ( ( ( vt & 0x400 ) >> 10 ) | ( ( vd & 0x400 ) >> 8 ) |
        ( ( vbs & 0x400 ) >> 6 ) | ( ( vbe & 0x400 ) >> 4 ) );

	reg.vgainit0 = VGAINIT0_8BIT_DAC | VGAINIT0_EXT_ENABLE |
        VGAINIT0_WAKEUP_3C3 | VGAINIT0_ALT_READBACK | VGAINIT0_EXTSHIFTOUT;
	reg.vgainit1 = tdfx_inl( VGAINIT1 ) & 0x1fffff;

	reg.cursloc = 0;
	reg.cursc0 = 0;
	reg.cursc1 = 0xffffff;

	reg.stride = sMode.m_nBytesPerLine;
	reg.startaddr = 0;
	reg.srcbase = 0;
	reg.dstbase = 0;

	/* PLL settings */
	freq = PICOS2KHZ( t.pixclock );

	reg.dacmode &= ~DACMODE_2X;
	reg.vidcfg &= ~VIDCFG_2X;
	if( freq > ( m_nMaxPixclock / 2 ) )
	{
		freq = freq > m_nMaxPixclock ? m_nMaxPixclock : freq;
		reg.dacmode |= DACMODE_2X;
		reg.vidcfg |= VIDCFG_2X;
	}
	reg.vidpll = calc_pll( freq, &fout );

	reg.screensize = sMode.m_nWidth | ( sMode.m_nHeight << 12 );
	reg.vidcfg &= ~VIDCFG_HALF_MODE;
	reg.miscinit0 = tdfx_inl( MISCINIT0 );

	write_regs( &reg );

	m_sCurrentMode = sMode;
	return 0;
}


os::screen_mode TDFX::GetCurrentScreenMode()
{
	return m_sCurrentMode;
}



//-----------------------------------------------------------------------------
//                          utility functions
//-----------------------------------------------------------------------------


inline uint32 TDFX::pci_size( uint32 base, uint32 mask )
{
	uint32 size = base & mask;

	return size & ~( size - 1 );
}


uint32 TDFX::get_pci_memory_size( int nFd, const PCI_Info_s * pcPCIInfo,
                                  int nResource )
{
	int nBus = pcPCIInfo->nBus;
	int nDev = pcPCIInfo->nDevice;
	int nFnc = pcPCIInfo->nFunction;
	int nOffset = PCI_BASE_REGISTERS + nResource * 4;
	uint32 nBase = pci_gfx_read_config( nFd, nBus, nDev, nFnc, nOffset, 4 );

	pci_gfx_write_config( nFd, nBus, nDev, nFnc, nOffset, 4, ~0 );
	uint32 nSize = pci_gfx_read_config( nFd, nBus, nDev, nFnc, nOffset, 4 );

	pci_gfx_write_config( nFd, nBus, nDev, nFnc, nOffset, 4, nBase );
	if( !nSize || nSize == 0xffffffff )
		return 0;
	if( nBase == 0xffffffff )
		nBase = 0;

	if( !( nSize & PCI_ADDRESS_SPACE ) )
		return pci_size( nSize, PCI_ADDRESS_MEMORY_32_MASK );
	else
		return pci_size( nSize, PCI_ADDRESS_IO_MASK & 0xffff );
}


inline void TDFX::fifo_make_room( unsigned int size )
{
	/* Note: The Voodoo3's onboard FIFO has 32 slots. This loop
	 * won't quit if you ask for more. */
	while( ( tdfx_inl( STATUS ) & 0x1f ) < size - 1 );
}


void TDFX::wait_idle()
{
	int i = 0;

	fifo_make_room( 1 );
	tdfx_outl( COMMAND_3D, COMMAND_3D_NOP );

	while( i != 3 )
		i = ( tdfx_inl( STATUS ) & STATUS_BUSY ) ? 0 : i + 1;
}


uint32 TDFX::calc_pll( int freq, int *freq_out )
{
	int m, n, k, best_m, best_n, best_k, f_cur, best_error;
	int fref = 14318;

	/* this really could be done with more intelligence --
	   255*63*4 = 64260 iterations is silly */
	best_error = freq;
	best_n = best_m = best_k = 0;
	for( n = 1; n < 256; n++ )
		for( m = 1; m < 64; m++ )
			for( k = 0; k < 4; k++ )
			{
				f_cur = fref * ( n + 2 ) / ( m + 2 ) / BIT( k );
				if( abs( f_cur - freq ) < best_error )
				{
					best_error = abs( f_cur - freq );
					best_n = n;
					best_m = m;
					best_k = k;
				}
			}

	n = best_n;
	m = best_m;
	k = best_k;
	*freq_out = fref * ( n + 2 ) / ( m + 2 ) / BIT( k );
	return ( n << 8 ) | ( m << 2 ) | k;
}


void TDFX::write_regs( struct tdfx_reg *reg )
{
	int i;

	wait_idle();

	tdfx_outl( MISCINIT1, tdfx_inl( MISCINIT1 ) | 0x01 );

	crt_outb( 0x11, crt_inb( 0x11 ) & 0x7f );	/* CRT unprotect */

	fifo_make_room( 3 );
	tdfx_outl( VGAINIT1, reg->vgainit1 & 0x001FFFFF );
	tdfx_outl( VIDPROCCFG, reg->vidcfg & ~0x00000001 );
	tdfx_outl( PLLCTRL0, reg->vidpll );

	vga_outb( MISC_W, reg->misc[0x00] | 0x01 );

	for( i = 0; i < 5; i++ )
		seq_outb( i, reg->seq[i] );
	for( i = 0; i < 25; i++ )
		crt_outb( i, reg->crt[i] );
	for( i = 0; i < 9; i++ )
		gra_outb( i, reg->gra[i] );
	for( i = 0; i < 21; i++ )
		att_outb( i, reg->att[i] );

	crt_outb( 0x1a, reg->ext[0] );
	crt_outb( 0x1b, reg->ext[1] );

	vga_enable_palette();
	vga_enable_video();

	fifo_make_room( 11 );
	tdfx_outl( VGAINIT0, reg->vgainit0 );
	tdfx_outl( DACMODE, reg->dacmode );
	tdfx_outl( VIDDESKSTRIDE, reg->stride );

	tdfx_outl( HWCURPATADDR, m_nFbSize - 1024 );
	tdfx_outl( HWCURC0, 0 );
	tdfx_outl( HWCURC1, 0xffffff );

	tdfx_outl( VIDSCREENSIZE, reg->screensize );
	tdfx_outl( VIDDESKSTART, reg->startaddr );
	tdfx_outl( VIDPROCCFG, reg->vidcfg );
	tdfx_outl( VGAINIT1, reg->vgainit1 );
	tdfx_outl( MISCINIT0, reg->miscinit0 );

	fifo_make_room( 8 );
	tdfx_outl( SRCBASE, reg->srcbase );
	tdfx_outl( DSTBASE, reg->dstbase );
	tdfx_outl( COMMANDEXTRA_2D, 0 );
	tdfx_outl( CLIP0MIN, 0 );
	tdfx_outl( CLIP0MAX, 0x0fff0fff );
	tdfx_outl( CLIP1MIN, 0 );
	tdfx_outl( CLIP1MAX, 0x0fff0fff );
	tdfx_outl( SRCXY, 0 );

	wait_idle();
}


uint32 TDFX::get_lfb_size()
{
	uint32 draminit0 = 0;
	uint32 draminit1 = 0;
	uint32 miscinit1 = 0;
	uint32 lfbsize = 0;
	bool sgram_p = false;

	draminit0 = tdfx_inl( DRAMINIT0 );
	draminit1 = tdfx_inl( DRAMINIT1 );

	if( ( m_nChipsetId == ID_3DFX_BANSHEE ) ||
        ( m_nChipsetId == ID_3DFX_VOODOO3 ) )
	{
		sgram_p = !( draminit1 & DRAMINIT1_MEM_SDRAM );
		if( sgram_p )
			lfbsize = ( ( ( draminit0 & DRAMINIT0_SGRAM_NUM ) ? 2 : 1 ) *
               ( ( draminit0 & DRAMINIT0_SGRAM_TYPE ) ? 8 : 4 ) * 1024 * 1024 );
		else
			lfbsize = 16 * 1024 * 1024;
	}
	else			// Voodoo 4/5
	{
		uint32 chips, psize, banks;

		chips = ( ( draminit0 & BIT( 26 ) ) == 0 ) ? 4 : 8;
		psize = 1 << ( ( draminit0 & 0x38000000 ) >> 28 );
		banks = ( ( draminit0 & BIT( 30 ) ) == 0 ) ? 2 : 4;
		lfbsize = chips * psize * banks;
		lfbsize <<= 20;
	}

	/* disable block writes for SDRAM (why?) */
	miscinit1 = tdfx_inl( MISCINIT1 );
	miscinit1 |= sgram_p ? 0 : MISCINIT1_2DBLOCK_DIS;
	miscinit1 |= MISCINIT1_CLUT_INV;
	fifo_make_room( 1 );
	tdfx_outl( MISCINIT1, miscinit1 );

	return lfbsize;
}


//-----------------------------------------------------------------------------
//                          Accelerated Functions
//-----------------------------------------------------------------------------


bool TDFX::DrawLine( SrvBitmap * pcBitMap, const IRect & cClipRect,
                     const IPoint & cPnt1, const IPoint & cPnt2,
                     const Color32_s & sColor, int nMode )
{
	if( ( pcBitMap->m_bVideoMem == false ) || ( lineRop[nMode] == -1 ) )
		return DisplayDriver::DrawLine( pcBitMap, cClipRect, cPnt1, cPnt2,
                                        sColor, nMode );


	int x1 = cPnt1.x, y1 = cPnt1.y, x2 = cPnt2.x, y2 = cPnt2.y;

	if( DisplayDriver::ClipLine( cClipRect, &x1, &y1, &x2, &y2 ) == false )
		return false;


	uint32 nColor;

	switch ( pcBitMap->m_eColorSpc )
	{
	case CS_RGB32:
		nColor = COL_TO_RGB32( sColor );
		break;
	case CS_RGB16:
		nColor = COL_TO_RGB16( sColor );
		break;
	default:
		return DisplayDriver::DrawLine( pcBitMap, cClipRect, cPnt1, cPnt2,
                                        sColor, nMode );
	}

	m_cGELock.Lock();

	fifo_make_room( 5 );
	tdfx_outl( SRCXY, x1 | ( y1 << 16 ) );
	tdfx_outl( COLORBACK, nColor );
	tdfx_outl( COLORFORE, nColor );
	tdfx_outl( COMMAND_2D, COMMAND_2D_LINE | ( lineRop[nMode] << 24 ) );
	tdfx_outl( LAUNCH_2D, x2 | ( y2 << 16 ) );
	wait_idle();

	m_cGELock.Unlock();
	return true;
}


bool TDFX::FillRect( SrvBitmap * pcBitMap, const IRect & cRect,
                     const Color32_s & sColor, int nMode )
{
	if( pcBitMap->m_bVideoMem == false || nMode != DM_COPY )
		return DisplayDriver::FillRect( pcBitMap, cRect, sColor, nMode );

	int nWidth = cRect.Width() + 1;
	int nHeight = cRect.Height() + 1;
	uint32 nColor;

	switch ( pcBitMap->m_eColorSpc )
	{
	case CS_RGB32:
		nColor = COL_TO_RGB32( sColor );
		break;
	case CS_RGB16:
		nColor = COL_TO_RGB16( sColor );
		break;
	default:
		return DisplayDriver::FillRect( pcBitMap, cRect, sColor, nMode );
	}

	uint32 stride = m_sCurrentMode.m_nBytesPerLine;
	uint32 bpp = ( stride / m_sCurrentMode.m_nWidth ) * 8;
	uint32 fmt = stride | ( ( bpp + ( ( bpp == 8 ) ? 0 : 8 ) ) << 13 );

	m_cGELock.Lock();

	fifo_make_room( 7 );
	tdfx_outl( DSTFORMAT, fmt );
	tdfx_outl( COLORFORE, nColor );
	tdfx_outl( COMMAND_2D, COMMAND_2D_FILLRECT | ( TDFX_ROP_SRCCOPY << 24 ) );
	tdfx_outl( DSTSIZE, nWidth | ( nHeight << 16 ) );
	tdfx_outl( LAUNCH_2D, cRect.left | ( cRect.top << 16 ) );
	wait_idle();

	m_cGELock.Unlock();
	return true;
}


bool TDFX::BltBitmap( SrvBitmap * pcDstBitMap, SrvBitmap * pcSrcBitMap,
                      IRect cSrcRect, IRect cDstRect, int nMode, int nAlpha )
{
	if( ( pcSrcBitMap->m_bVideoMem == false ) ||
        ( pcDstBitMap->m_bVideoMem == false ) || ( nMode != DM_COPY ) || cSrcRect.Size() != cDstRect.Size() )
		return DisplayDriver::BltBitmap( pcDstBitMap, pcSrcBitMap, cSrcRect,
                                         cDstRect, nMode, nAlpha );

	IPoint cDstPos = cDstRect.LeftTop();
	int nWidth = cSrcRect.Width() + 1;
	int nHeight = cSrcRect.Height() + 1;

	uint32 stride = m_sCurrentMode.m_nBytesPerLine;
	uint32 bpp = ( stride / m_sCurrentMode.m_nWidth ) * 8;
	uint32 fmt = stride | ( ( bpp + ( ( bpp == 8 ) ? 0 : 8 ) ) << 13 );
	uint32 blitcmd = COMMAND_2D_S2S_BITBLT | ( TDFX_ROP_SRCCOPY << 24 );
	uint32 sx = cSrcRect.left, sy = cSrcRect.top, dx = cDstPos.x,
        dy = cDstPos.y;

	if( sx <= dx )
	{
		/* -X */
		blitcmd |= BIT( 14 );
		sx += nWidth - 1;
		dx += nWidth - 1;
	}
	if( sy <= dy )
	{
		/* -Y */
		blitcmd |= BIT( 15 );
		sy += nHeight - 1;
		dy += nHeight - 1;
	}

	m_cGELock.Lock();

	fifo_make_room( 6 );
	tdfx_outl( SRCFORMAT, fmt );
	tdfx_outl( DSTFORMAT, fmt );
	tdfx_outl( COMMAND_2D, blitcmd );
	tdfx_outl( DSTSIZE, nWidth | ( nHeight << 16 ) );
	tdfx_outl( DSTXY, dx | ( dy << 16 ) );
	tdfx_outl( LAUNCH_2D, sx | ( sy << 16 ) );
	wait_idle();

	m_cGELock.Unlock();
	return true;
}



//-----------------------------------------------------------------------------
//                          Video Overlay Functions
//-----------------------------------------------------------------------------


bool TDFX::CreateOverlayCommon( const os::IPoint & cSize,
                                const os::IRect & cDst, area_id *pBuffer )
{
	// calculate position of overlay buffer in offscreen memory
	uint32 pitch = ( ( cSize.x * 2 ) + 0xff ) & ~0xff;
	uint32 totalSize = pitch * cSize.y;

	uint32 offset = ( m_nFbSize - totalSize - 1024 ) & 0xfffff000;

	if( offset < ( unsigned int )( m_sCurrentMode.m_nHeight *
                                   m_sCurrentMode.m_nBytesPerLine ) )
	{
		dbprintf( "TDFX: Not enough unused video memory for overlay. Lower " \
                  "your display resolution / color depth and try again.\n" );
		return false;
	}

	*pBuffer = create_area( "tdfx_overlay", NULL, PAGE_ALIGN( totalSize ),
                            AREA_FULL_ACCESS, AREA_NO_LOCK );
	remap_area( *pBuffer, ( void * )( m_nFbBasePhys + offset ) );

	// video processor setup
	uint32 vidcfg = tdfx_inl( VIDPROCCFG ) & VIDCFG_OVERLAY_MASK;

	vidcfg |= VIDCFG_OVERLAY_ENABLE | VIDCFG_FMT_UYVY422 |
        VIDCFG_OVL_CLUT_BYPASS;
	tdfx_outl( VIDPROCCFG, vidcfg );

	tdfx_outl( VIDCHRMIN, m_nColorKey );
	tdfx_outl( VIDCHRMAX, m_nColorKey );

	tdfx_outl( VIDOVRSTARTCRD, cDst.left | ( cDst.top << 12 ) );
	tdfx_outl( VIDOVRENDCRD, ( cDst.right - 1 ) | ( ( cDst.bottom - 1 ) << 12 ) );
	tdfx_outl( VIDOVRDUDX, 0 );
	tdfx_outl( VIDOVRDUDXOFF, pitch << 19 );
	tdfx_outl( VIDOVRDVDY, 0 );
	tdfx_outl( VIDOVRDVDYOFF, 0 );

	uint32 stride = m_sCurrentMode.m_nBytesPerLine & 0x0000ffff;

	stride |= pitch << 16;
	tdfx_outl( VIDDESKSTRIDE, stride );
	tdfx_outl( VIDINADDR0, offset );

	return true;
}


bool TDFX::CreateVideoOverlay( const os::IPoint & cSize, const os::IRect & cDst,
                               os::color_space eFormat, os::Color32_s sColorKey,
                               area_id *pBuffer )
{
	if( eFormat == CS_YUV422 && !m_bVideoOverlayUsed )
	{
		switch ( m_sCurrentMode.m_eColorSpace )
		{
		case CS_RGB16:
			m_nColorKey = COL_TO_RGB16( sColorKey );
			break;
		case CS_RGB32:
			m_nColorKey = COL_TO_RGB32( sColorKey );
			break;
		default:
			return false;
		}

		m_bVideoOverlayUsed = CreateOverlayCommon( cSize, cDst, pBuffer );
		return m_bVideoOverlayUsed;
	}

	return false;
}


bool TDFX::RecreateVideoOverlay( const os::IPoint & cSize,
                                 const os::IRect & cDst,
                                 os::color_space eFormat, area_id *pBuffer )
{
	if( eFormat == CS_YUV422 )
	{
		delete_area( *pBuffer );
		m_bVideoOverlayUsed = CreateOverlayCommon( cSize, cDst, pBuffer );
		return m_bVideoOverlayUsed;
	}

	return false;
}

void TDFX::DeleteVideoOverlay( area_id *pBuffer )
{
	if( m_bVideoOverlayUsed )
	{
		delete_area( *pBuffer );
		tdfx_outl( VIDPROCCFG, tdfx_inl( VIDPROCCFG ) & VIDCFG_OVERLAY_MASK );
	}

	m_bVideoOverlayUsed = false;
}



//-----------------------------------------------------------------------------


extern "C" DisplayDriver * init_gfx_driver( int nFd )
{
	dbprintf( "TDFX: attempts to initialize\n" );

	try
	{
		TDFX *pcDriver = new TDFX( nFd );

		if( pcDriver->IsInitiated() )
			return pcDriver;

		return NULL;
	}
	catch( std::exception & cExc )
	{
		dbprintf( "TDFX: got exception\n" );
		return NULL;
	}
}
