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
 * Both the old & new streams engine (Mobile series IX & MX) are supported by this driver.  Some additional
 * support for Twister chipsets is absent.  Adding support for those chips would not be difficult.
 * The Savage 2000 streams engine is currently not supported at all.
 *
 * The Savages support YV12 but only in a planer format; Syllable presents the data as three seperate
 * components.  We use YUY2.
 */

bool SavageDriver::CreateVideoOverlay( const IPoint& cSize, const IRect& cDst, color_space eFormat, Color32_s sColorKey, area_id *phArea )
{
	bool bRet = false;

	if( eFormat == CS_YUY2 && m_psCard->eChip != S3_SAVAGE2000 && !m_bVideoOverlayUsed )
	{
		m_sColorKey = sColorKey;
		if( S3_SAVAGE_MOBILE_SERIES( m_psCard->eChip ) )
			bRet = SavageOverlayNew( cSize, cDst, phArea );
		else
			bRet = SavageOverlayOld( cSize, cDst, phArea );
	}
	return bRet;
}

bool SavageDriver::RecreateVideoOverlay( const IPoint& cSize, const IRect& cDst, color_space eFormat, area_id *phArea )
{
	bool bRet = false;

	if( eFormat == CS_YUY2 && m_psCard->eChip != S3_SAVAGE2000 && m_bVideoOverlayUsed )
	{
		delete_area( *phArea );
		FreeMemory( m_nVideoOffset );
		SavageStreamsOff( m_psCard );

		if( S3_SAVAGE_MOBILE_SERIES( m_psCard->eChip ) )
			bRet = SavageOverlayNew( cSize, cDst, phArea );
		else
			bRet = SavageOverlayOld( cSize, cDst, phArea );
	}
	return bRet;
}

void SavageDriver::DeleteVideoOverlay( area_id *phArea )
{
	/* Stop video */
	SavageStreamsOff( m_psCard );

	/* Delete area */
	if( m_bVideoOverlayUsed )
	{
		FreeMemory( m_nVideoOffset );
		delete_area( *phArea );
	}
	m_bVideoOverlayUsed = false;
}

bool SavageDriver::SavageOverlayNew( const IPoint& cSize, const IRect& cDst, area_id *phArea )
{
	savage_s *psCard = m_psCard;

	/* Calculate offset */
	uint32 pitch = 0;
	uint32 totalSize = 0, offset = 0;

	pitch = ( ( cSize.x << 1 ) + 0xff ) & ~0xff;
	totalSize = pitch * cSize.y; 

	/* Allocate overlay space */
	if( AllocateMemory( totalSize, &offset ) != EOK )
	{
		dbprintf("No space for overlay: scrnBytes=0x%4x  totalsize=%li  endfb=0x%4x\n", psCard->scrnBytes, totalSize, psCard->endfb );
		m_bVideoOverlayUsed = false;
		return false;
	}

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
	m_nVideoOffset = offset;
	m_bVideoOverlayUsed = true;
	return true;
}

void SavageDriver::SavageSetColorKeyNew( savage_s *psCard )
{
	/* Here, we reset the colorkey and all the controls. */
	int red, green, blue;

	switch(psCard->scrn.Bpp)
	{
		case 16:
		{
			/* 5:6:5 */
			red = m_sColorKey.red & 0x1f;
			green = m_sColorKey.green & 0x3f;
			blue = m_sColorKey.blue & 0x1f;

			OUTREG( SEC_STREAM_CKEY_LOW, 0x46000000 | (red<<19) | (green<<10) | (blue<<3) );
			OUTREG( SEC_STREAM_CKEY_UPPER, 0x46020002 | (red<<19) | (green<<10) | (blue<<3) );
		    break;
	    }

		case 32:
		{
			red = m_sColorKey.red;
			green = m_sColorKey.green;
			blue = m_sColorKey.blue;

			OUTREG( SEC_STREAM_CKEY_LOW, 0x47000000 | (red<<16) | (green<<8) | (blue) );
			OUTREG( SEC_STREAM_CKEY_UPPER, 0x47000000 | (red<<16) | (green<<8) | (blue) );
			break;
		}
	}

	/* We assume destination colorkey */
	OUTREG( BLEND_CONTROL, (INREG32(BLEND_CONTROL) | (psCard->blendBase << 9) | 0x08 ));
}

bool SavageDriver::SavageOverlayOld( const os::IPoint& cSize, const os::IRect& cDst, area_id *phArea )
{
	savage_s *psCard = m_psCard;
	uint32 ssControl;
	int scalratio;

	/* Calculate offset */
	uint32 pitch = 0;
	uint32 totalSize = 0, offset = 0;

	pitch = ( ( cSize.x << 1 ) + 0xff ) & ~0xff;
	totalSize = pitch * cSize.y; 

	/* Allocate overlay space */
	if( AllocateMemory( totalSize, &offset ) != EOK )
	{
		dbprintf("No space for overlay: scrnBytes=0x%4x  totalsize=%li  endfb=0x%4x\n", psCard->scrnBytes, totalSize, psCard->endfb );
		m_bVideoOverlayUsed = false;
		return false;
	}

	*phArea = create_area( "savage_overlay", NULL, PAGE_ALIGN( totalSize ), AREA_FULL_ACCESS, AREA_NO_LOCK );
	remap_area( *phArea, (void*)(m_nFramebufferAddr + offset) );

	/* XXXKV: Start video */
	int y1 = cDst.top;
	int y2 = cDst.bottom;
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
	SavageSetColorKeyOld( psCard );

	/* XXXKV: Extra scaling calculations for Twisters goes here */

	/* Process horizontal scaling
	 *  upscaling and downscaling smaller than 2:1 controled by MM8198
	 *  MM8190 controls downscaling mode larger than 2:1 */
	scalratio = 0;
	ssControl = 0;

    if (src_w >= (drw_w * 2))
	{
		if (src_w < (drw_w * 4))
			scalratio = HSCALING(2,1);
		else if (src_w < (drw_w * 8))
			ssControl |= HDSCALE_4;
		else if (src_w < (drw_w * 16))
			ssControl |= HDSCALE_8;
		else if (src_w < (drw_w * 32))
			ssControl |= HDSCALE_16;
		else if (src_w < (drw_w * 64))
			ssControl |= HDSCALE_32;
		else
			ssControl |= HDSCALE_64;
	}
	else 
		scalratio = HSCALING(src_w,drw_w);

	ssControl |= src_w;
	ssControl |= (1 << 24);

	/* Wait for VBLANK. */
	VerticalRetraceWait();
	OUTREG(SSTREAM_CONTROL_REG, ssControl);
	if (scalratio)
		OUTREG(SSTREAM_STRETCH_REG,scalratio);

	/* Calculate vertical scale factor. */
	OUTREG(SSTREAM_VINITIAL_REG, 0 );
	OUTREG(SSTREAM_VSCALE_REG, VSCALING(src_h,drw_h));

	/* Set surface location and stride. */
	OUTREG(SSTREAM_FBADDR0_REG, (offset + (x1>>15)) & (0x1ffffff & ~BASE_PAD) );
	OUTREG(SSTREAM_FBADDR1_REG, 0);
	OUTREG(SSTREAM_STRIDE_REG, pitch & 0xfff );
                                                                             
	OUTREG(SSTREAM_WINDOW_START_REG, OS_XY(x1, y1) );
	OUTREG(SSTREAM_WINDOW_SIZE_REG, OS_WH(x2-x1, y2-y1));

	/* MM81E8:Secondary Stream Source Line Count
	 *   bit_0~10: # of lines in the source image (before scaling)
	 *   bit_15 = 1: Enable vertical interpolation
	 *            0: Line duplicaion */

	/* XXXKV: No vertical Interpolation used; always use line doubling. */
	OUTREG(SSTREAM_LINES_REG, src_h );

	/* Set FIFO L2 on second stream. */
	if( psCard->lastKnownPitch != pitch )
	{
		unsigned char cr92;
		psCard->lastKnownPitch = pitch;

		pitch = (pitch + 7) / 8;
		VGAOUT8(vgaCRIndex, 0x92);
		cr92 = VGAIN8(vgaCRReg);
		VGAOUT8(vgaCRReg, (cr92 & 0x40) | (pitch >> 8) | 0x80);
		VGAOUT8(vgaCRIndex, 0x93);

		/* Tiling is always off */
		VGAOUT8(vgaCRReg, pitch);
	}

	/* Overlay is on */
	m_nVideoOffset = offset;
	m_bVideoOverlayUsed = true;
	return true;
}

void SavageDriver::SavageSetColorKeyOld( savage_s *psCard )
{
	/* Here, we reset the colorkey and all the controls. */
	int red, green, blue;

	switch(psCard->scrn.Bpp)
	{
		case 16:
		{
			/* 5:6:5 */
			red = m_sColorKey.red & 0x1f;
			green = m_sColorKey.green & 0x3f;
			blue = m_sColorKey.blue & 0x1f;

			OUTREG( SEC_STREAM_CKEY_LOW, 0x46000000 | (red<<19) | (green<<10) | (blue<<3) );
			OUTREG( SEC_STREAM_CKEY_UPPER, 0x46020002 | (red<<19) | (green<<10) | (blue<<3) );
		    break;
	    }

		case 32:
		{
			red = m_sColorKey.red;
			green = m_sColorKey.green;
			blue = m_sColorKey.blue;

			OUTREG( COL_CHROMA_KEY_CONTROL_REG, 0x17000000 | (red<<16) | (green<<8) | (blue) );
			OUTREG( CHROMA_KEY_UPPER_BOUND_REG, 0x00000000 | (red<<16) | (green<<8) | (blue) );
			break;
		}
	}

	/* We assume destination colorkey */
	OUTREG( BLEND_CONTROL_REG, 0x05000000 );
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

void SavageDriver::SavageStreamsOff( savage_s *psCard )
{
	unsigned char jStreamsControl;
	int vgaCRIndex, vgaCRReg, vgaIOBase;

	/* XXXKV: Need to know value of vgaIOBase; probably 0x3d0 (!) */
	vgaIOBase = 0x3d0;
	vgaCRIndex = vgaIOBase + 4;
	vgaCRReg = vgaIOBase + 5;

	/* Unlock extended registers. */
	VGAOUT16(vgaCRIndex, 0x4838);
	VGAOUT16(vgaCRIndex, 0xa039);
	VGAOUT16(0x3c4, 0x0608);

	VGAOUT8( vgaCRIndex, EXT_MISC_CTRL2 );
	if( S3_SAVAGE_MOBILE_SERIES(psCard->eChip) || (psCard->eChip == S3_SAVAGE2000) )
		jStreamsControl = VGAIN8( vgaCRReg ) & NO_STREAMS;
	else
		jStreamsControl = VGAIN8( vgaCRReg ) & NO_STREAMS_OLD;

	/* Wait for VBLANK. */
	VerticalRetraceWait();

	/* Kill streams. */
	VGAOUT16( vgaCRIndex, (jStreamsControl << 8) | EXT_MISC_CTRL2 );

	VGAOUT16( vgaCRIndex, 0x0093 );
	VGAOUT8( vgaCRIndex, 0x92 );
	VGAOUT8( vgaCRReg, VGAIN8(vgaCRReg) & 0x40 );
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

