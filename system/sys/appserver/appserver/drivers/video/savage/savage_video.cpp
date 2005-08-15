/*
 *  S3 Savage driver for Syllable
 *
 *  Based on the original SavageIX/MX driver for Syllable by Hillary Cheng
 *  and the X.org Savage driver.
 *
 *  X.org driver copyright Tim Roberts (timr@probo.com),
 *  Ani Joshi (ajoshi@unixbox.com) & S. Marineau
 *
 *  Copyright 2005 Kristian Van Der Vliet
 *  Copyright 2003 Hilary Cheng (hilarycheng@yahoo.com)
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

#include <savage.h>
#include <savage_driver.h>
#include <savage_streams.h>

#include <atheos/areas.h>
#include <gui/bitmap.h>

using namespace os;
using namespace std;

/*
 * Only the new streams engine, present in the Mobile series (IX & MX) is currently supported by this driver.  The
 * Savage 2000 streams engine is not supported at all.  Adding support for the old streams engine on E.g. the ProSavage,
 * Twister etc. chips is possible; either use the new streams code as a template or contact the Syllable developers and
 * ask.  Parts of the old streams engine are already here E.g. SavageStreamsOn() is common to both and SavageInitStreamsOld()
 * is already here.
 *
 * The Savage support YV12 but only in a planer format; Syllable presents the data as three seperate components.
 * Syllable now supports YUY2, which the Savage supports natively.
 */

bool SavageDriver::CreateVideoOverlay( const IPoint& cSize, const IRect& cDst, color_space eFormat, Color32_s sColorKey, area_id *phArea )
{
	if( eFormat == CS_YUY2 && m_psCard->eChip != S3_SAVAGE2000 && !m_bVideoOverlayUsed )
	{
		m_sColorKey = sColorKey;
		if( S3_SAVAGE_MOBILE_SERIES( m_psCard->eChip ) )	/* We only support new streams engine properly at the moment */
			return SavageOverlayNew( cSize, cDst, phArea );
		else
			return false;	/* XXXKV: Support old streams engine */
	}
	return false;
}

bool SavageDriver::RecreateVideoOverlay( const IPoint& cSize, const IRect& cDst, color_space eFormat, area_id *phArea )
{
	if( eFormat == CS_YUY2 && m_psCard->eChip != S3_SAVAGE2000 && m_bVideoOverlayUsed )
	{
		delete_area( *phArea );
		if( S3_SAVAGE_MOBILE_SERIES( m_psCard->eChip ) )	/* We only support new streams engine properly at the moment */
			return SavageOverlayNew( cSize, cDst, phArea );
		else
			return false;	/* XXXKV: Support old streams engine */
	}
	return false;
}

void SavageDriver::UpdateVideoOverlay( area_id *phArea )
{
}

void SavageDriver::DeleteVideoOverlay( area_id *phArea )
{
	/* Stop video */
	//SavageStreamsOff( m_psCard );	/* XXXKV: Implement */

	/* Delete area */
	if( m_bVideoOverlayUsed )
		delete_area( *phArea );
	m_bVideoOverlayUsed = false;
}

bool SavageDriver::SavageOverlayNew( const IPoint& cSize, const IRect& cDst, area_id *phArea )
{
	savage_s *psCard = m_psCard;

	/* Calculate offset */
	uint32 pitch = 0;
	uint32 totalSize = 0;

	pitch = ( ( cSize.x << 1 ) + 0xff ) & ~0xff;
	totalSize = pitch * cSize.y; 

	/* There must be enough room between the end of the visible display & the end of the framebuffer for the overlay data.. */
	if( psCard->scrnBytes + totalSize > (unsigned)psCard->endfb )
	{
		dbprintf("No space for overlay: scrnBytes=0x%4x  totalsize=%li  endfb=0x%4x\n", psCard->scrnBytes, totalSize, psCard->endfb );
		m_bVideoOverlayUsed = false;
		return false;
	}

	/* Allocate the overlay space at the end of the framebuffer (Leave room for the framebuffer to grow.  Hopefully!) */
	uint32 offset = PAGE_ALIGN( psCard->endfb - totalSize - PAGE_SIZE );

	//dbprintf("Create overlay: scrnBytes=0x%x  totalSize=0x%x (offset=0x%x) endfb=0x%x\n", psCard->scrnBytes, totalSize, offset, psCard->endfb );

	*phArea = create_area( "savage_overlay", NULL, PAGE_ALIGN( totalSize ), AREA_FULL_ACCESS, AREA_NO_LOCK );
	remap_area( *phArea, (void*)(m_nFramebufferAddr + offset) );

	/* XXXKV: Start video */
	int y1 = cDst.top;
	int	x1 = cDst.left;
	int	x2 = cDst.right;

	short src_w = cSize.x;
	short src_h = cSize.y;
	short drw_w = cDst.Width();
	short drw_h = cDst.Height();

	//dbprintf("x1=%i y1=%i x2=%i y2=%i src_w=%i src_h=%i drw_w=%i drw_h=%i\n", x1, y1, x2, y2, src_w, src_h, drw_w, drw_h );

	int vgaCRIndex, vgaCRReg, vgaIOBase;

	/* XXXKV: Need to know value of vgaIOBase; probably 0x3d0 (!) */
	vgaIOBase = 0x3d0;
	vgaCRIndex = vgaIOBase + 4;
	vgaCRReg = vgaIOBase + 5;

	SavageStreamsOn( psCard );

	/* Set blend */
	psCard->blendBase = 1;	/* Taken from GetBlendForFourCC( YUY2 ) */
	OUTREG( BLEND_CONTROL, (INREG32(BLEND_CONTROL) | (psCard->blendBase << 9) | 0x08 ));

	SavageSetColorKeyNew( psCard );

	OUTREG(SEC_STREAM_HSCALING, ((src_w&0xfff)<<20) | ((65536 * src_w / drw_w) & 0x1FFFF ));
	/* BUGBUG need to add 00040000 if src stride > 2048 */
	OUTREG(SEC_STREAM_VSCALING, ((src_h&0xfff)<<20) | ((65536 * src_h / drw_h) & 0x1FFFF ));

	/*
	 * Set surface location and stride.  We use x1>>15 because all surfaces
	 * are 2 bytes/pixel.
	 */
	OUTREG(SEC_STREAM_FBUF_ADDR0, (offset + (x1>>15)) & (0x7ffffff & ~BASE_PAD));
	OUTREG(SEC_STREAM_STRIDE, pitch & 0xfff );
	OUTREG(SEC_STREAM_WINDOW_START, ((x1+1) << 16) | (y1+1) );
	OUTREG(SEC_STREAM_WINDOW_SZ, ((x2-x1) << 16) | (x2-x1) );

	/* Set FIFO L2 on second stream. */
	/* Is CR92 shadowed for crtc2? -- AGD */
	if( psCard->lastKnownPitch != pitch )
	{
		unsigned char cr92;
		psCard->lastKnownPitch = pitch;

		pitch = (pitch + 7) / 8 - 4;
		VGAOUT8(vgaCRIndex, 0x92);
		cr92 = VGAIN8(vgaCRReg);
		VGAOUT8(vgaCRReg, (cr92 & 0x40) | (pitch >> 8) | 0x80);
		VGAOUT8(vgaCRIndex, 0x93);
		VGAOUT8(vgaCRReg, pitch);
	}

	/* Overlay is on */
	m_bVideoOverlayUsed = true;
	return true;
}

void SavageDriver::SavageSetColorKeyNew( savage_s *psCard )
{
	/* Here, we reset the colorkey and all the controls. */
	int red = m_sColorKey.red;
	int green = m_sColorKey.green;
	int blue = m_sColorKey.blue;

	switch(psCard->scrn.Bpp)
	{
		case 16:
		{
			OUTREG( SEC_STREAM_CKEY_LOW, 0x46000000 | (red<<19) | (green<<10) | (blue<<3) );
			OUTREG( SEC_STREAM_CKEY_UPPER, 0x46020002 | (red<<19) | (green<<10) | (blue<<3) );
		    break;
	    }

		case 32:
		{
			OUTREG( SEC_STREAM_CKEY_LOW, 0x47000000 | (red<<16) | (green<<8) | (blue) );
			OUTREG( SEC_STREAM_CKEY_UPPER, 0x47000000 | (red<<16) | (green<<8) | (blue) );
			break;
		}
	}

	/* We assume destination colorkey */
	OUTREG( BLEND_CONTROL, (INREG32(BLEND_CONTROL) | (psCard->blendBase << 9) | 0x08 ));
}

/* Common to both old & new streams engines */
void SavageDriver::SavageStreamsOn( savage_s *psCard )
{
	unsigned char jStreamsControl;
	int vgaCRIndex, vgaCRReg, vgaIOBase;

	/* XXXKV: Need to know value of vgaIOBase; probably 0x3d0 (!) */
	vgaIOBase = 0x3d0;
	vgaCRIndex = vgaIOBase + 4;
	vgaCRReg = vgaIOBase + 5;

	/* Sequence stolen from streams.c in M7 NT driver */

	/* Unlock extended registers. */
	VGAOUT16(vgaCRIndex, 0x4838);
	VGAOUT16(vgaCRIndex, 0xa039);
	VGAOUT16(0x3c4, 0x0608);

	VGAOUT8( vgaCRIndex, EXT_MISC_CTRL2 );

	if( S3_SAVAGE_MOBILE_SERIES(psCard->eChip) )
	{
		SavageInitStreamsNew( psCard );

		jStreamsControl = VGAIN8( vgaCRReg ) | ENABLE_STREAM1;

		/* Wait for VBLANK. */	
		VerticalRetraceWait();
		/* Fire up streams! */
		VGAOUT16( vgaCRIndex, (jStreamsControl << 8) | EXT_MISC_CTRL2 );
	}
	else
	{
		jStreamsControl = VGAIN8( vgaCRReg ) | ENABLE_STREAMS_OLD;

		/* Wait for VBLANK. */
		VerticalRetraceWait();
		/* Fire up streams! */
		VGAOUT16( vgaCRIndex, (jStreamsControl << 8) | EXT_MISC_CTRL2 );

		SavageInitStreamsOld( psCard );
	}

	/* Wait for VBLANK. */
	VerticalRetraceWait();

	/* Turn on secondary stream TV flicker filter, once we support TV. */
	/* SR70 |= 0x10 */
}

void SavageDriver::SavageInitStreamsOld( savage_s *psCard )
{
	unsigned long format = 0;

	/*
	 * For the OLD streams engine, several of these registers
	 * cannot be touched unless streams are on.  Seems backwards to me;
	 * I'd want to set 'em up, then cut 'em loose.
	 */

	switch( psCard->scrn.Bpp )
	{
	    case 16:
			format = 5 << 24;
			break;
	    case 32:
			format = 7 << 24;
			break;
	}
	OUTREG(PSTREAM_FBSIZE_REG, psCard->scrn.Height * psCard->scrn.Width * (psCard->scrn.Bpp >> 3));

	OUTREG(FIFO_CONTROL, 0x18ffeL);
    
	OUTREG( PSTREAM_WINDOW_START_REG, OS_XY(0,0) );
	OUTREG( PSTREAM_WINDOW_SIZE_REG, OS_WH(psCard->scrn.Width, psCard->scrn.Height) );
	OUTREG( PSTREAM_CONTROL_REG, format );

	OUTREG( COL_CHROMA_KEY_CONTROL_REG, 0 );
	OUTREG( SSTREAM_CONTROL_REG, 0 );
	OUTREG( CHROMA_KEY_UPPER_BOUND_REG, 0 );
	OUTREG( SSTREAM_STRETCH_REG, 0 );
	OUTREG( COLOR_ADJUSTMENT_REG, 0 );
	OUTREG( BLEND_CONTROL_REG, 1 << 24 );
	OUTREG( DOUBLE_BUFFER_REG, 0 );
	OUTREG( SSTREAM_FBADDR0_REG, 0 );
	OUTREG( SSTREAM_FBADDR1_REG, 0 );
	OUTREG( SSTREAM_FBADDR2_REG, 0 );
	OUTREG( SSTREAM_FBSIZE_REG, 0 );
	OUTREG( SSTREAM_STRIDE_REG, 0 );
	OUTREG( SSTREAM_VSCALE_REG, 0 );
	OUTREG( SSTREAM_LINES_REG, 0 );
	OUTREG( SSTREAM_VINITIAL_REG, 0 );
	OUTREG( SSTREAM_WINDOW_START_REG, OS_XY(0xfffe, 0xfffe) );
	OUTREG( SSTREAM_WINDOW_SIZE_REG, OS_WH(10,2) );

#if 0
	if (S3_MOBILE_TWISTER_SERIES(psav->Chipset) && psav->FPExpansion)
        OverlayTwisterInit(pScrn);
#endif
}

void SavageDriver::SavageInitStreamsNew( savage_s *psCard )
{
	OUTREG(PRI_STREAM_BUFFERSIZE, psCard->scrn.Height * psCard->scrn.Width * (psCard->scrn.Bpp >> 3));

	OUTREG( SEC_STREAM_CKEY_LOW, 0 );
	OUTREG( SEC_STREAM_CKEY_UPPER, 0 );
	OUTREG( SEC_STREAM_HSCALING, 0 );
	OUTREG( SEC_STREAM_VSCALING, 0 );
	OUTREG( BLEND_CONTROL, 0 );
	OUTREG( SEC_STREAM_FBUF_ADDR0, 0 );
	OUTREG( SEC_STREAM_FBUF_ADDR1, 0 );
	OUTREG( SEC_STREAM_FBUF_ADDR2, 0 );
	OUTREG( SEC_STREAM_WINDOW_START, 0 );
	OUTREG( SEC_STREAM_WINDOW_SZ, 0 );
	OUTREG( SEC_STREAM_TILE_OFF, 0 );
	OUTREG( SEC_STREAM_OPAQUE_OVERLAY, 0 );
	OUTREG( SEC_STREAM_STRIDE, 0 );

	/* These values specify brightness, contrast, saturation and hue. */
	OUTREG( SEC_STREAM_COLOR_CONVERT1, 0x0000C892 );
	OUTREG( SEC_STREAM_COLOR_CONVERT2, 0x00039F9A );
	OUTREG( SEC_STREAM_COLOR_CONVERT3, 0x01F1547E );
}

