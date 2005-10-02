/*
 ** Radeon graphics driver for Syllable application server
 *  Copyright (C) 2004 Michael Krueger <invenies@web.de>
 *  Copyright (C) 2003 Arno Klenke <arno_klenke@yahoo.com>
 *  Copyright (C) 1998-2001 Kurt Skauen <kurt@atheos.cx>
 *
 ** This program is free software; you can redistribute it and/or
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
 ** For information about used sources and further copyright notices
 *  see radeon.cpp
 */

#include "radeon.h"

// standard (linear) gamma
static struct {
    uint16 reg;
    bool r200_or_above;
    uint32 slope;
    uint32 offset;
} std_gamma[] = {
    { RADEON_OV0_GAMMA_0_F, false, 0x100, 0x0000 },
    { RADEON_OV0_GAMMA_10_1F, false, 0x100, 0x0020 },
    { RADEON_OV0_GAMMA_20_3F, false, 0x100, 0x0040 },
    { RADEON_OV0_GAMMA_40_7F, false, 0x100, 0x0080 },
    { RADEON_OV0_GAMMA_80_BF, true, 0x100, 0x0100 },
    { RADEON_OV0_GAMMA_C0_FF, true, 0x100, 0x0100 },
    { RADEON_OV0_GAMMA_100_13F, true, 0x100, 0x0200 },
    { RADEON_OV0_GAMMA_140_17F, true, 0x100, 0x0200 },
    { RADEON_OV0_GAMMA_180_1BF, true, 0x100, 0x0300 },
    { RADEON_OV0_GAMMA_1C0_1FF, true, 0x100, 0x0300 },
    { RADEON_OV0_GAMMA_200_23F, true, 0x100, 0x0400 },
    { RADEON_OV0_GAMMA_240_27F, true, 0x100, 0x0400 },
    { RADEON_OV0_GAMMA_280_2BF, true, 0x100, 0x0500 },
    { RADEON_OV0_GAMMA_2C0_2FF, true, 0x100, 0x0500 },
    { RADEON_OV0_GAMMA_300_33F, true, 0x100, 0x0600 },
    { RADEON_OV0_GAMMA_340_37F, true, 0x100, 0x0600 },
    { RADEON_OV0_GAMMA_380_3BF, false, 0x100, 0x0700 },
    { RADEON_OV0_GAMMA_3C0_3FF, false, 0x100, 0x0700 }
};

// color space transformation matrix
typedef struct space_transform
{
    float   RefLuma;	// scaling of luma to use full RGB range
    float   RefRCb;		// b/u -> r
    float   RefRY;		// g/y -> r
    float   RefRCr;		// r/v -> r
    float   RefGCb;
    float   RefGY;
    float   RefGCr;
    float   RefBCb;
    float   RefBY;
    float   RefBCr;
} space_transform;


// Parameters for ITU-R BT.601 and ITU-R BT.709 colour spaces
space_transform trans_yuv[2] =
{
    { 1.1678, 0.0, 1, 1.6007, -0.3929, 1, -0.8154, 2.0232, 1, 0.0 }, /* BT.601 */
    { 1.1678, 0.0, 1, 1.7980, -0.2139, 1, -0.5345, 2.1186, 1, 0.0 }  /* BT.709 */
};


// RGB is a pass through
space_transform trans_rgb =
	{ 1, 0, 0, 1, 0, 1, 0, 1, 0, 0 };


typedef struct {
	uint max_scale;					// maximum src_width/dest_width, 
									// i.e. source increment per screen pixel
	uint8 group_size; 				// size of one filter group in pixels
	uint8 p1_step_by, p23_step_by;	// > 0: log(source pixel increment)+1, 2-tap filter
									// = 0: source pixel increment = 1, 4-tap filter
} hscale_factor;

#define count_of( a ) (sizeof( a ) / sizeof( a[0] ))

// scaling/filter tables depending on overlay colour space:
// magnifying pixels is no problem, but minifying can lead to overload,
// so we have to skip pixels and/or use 2-tap filters

static hscale_factor scale_RGB32[] = {
	{ (2 << 12) / 3,	2, 0, 0 },
	{ (4 << 12) / 3,	4, 1, 1 },
	{ (8 << 12) / 3,	4, 2, 2 },
	{ (4 << 12), 		4, 2, 3 },
	{ (16 << 12) / 3,	4, 3, 3 },
	{ (8 << 12), 		4, 3, 4 },
	{ (32 << 12) / 3,	4, 4, 4 },
	{ (16 << 12),		4, 5, 5 }
};

static hscale_factor scale_YUV[] = {
	{ (16 << 12) / 16,	2, 0, 0 },
	{ (16 << 12) / 12,	2, 0, 1 },	// mode 4, 1, 0 (as used by YUV12) is impossible
	{ (16 << 12) / 8,	4, 1, 1 },
	{ (16 << 12) / 6,	4, 1, 2 },
	{ (16 << 12) / 4,	4, 2, 2 },
	{ (16 << 12) / 3,	4, 2, 3 },
	{ (16 << 12) / 2,	4, 3, 3 },
	{ (16 << 12) / 1,	4, 4, 4 }
};

static hscale_factor scale_YUV12[] = {
	{ (16 << 12) / 16,			2, 0, 0 },
	{ (16 << 12) / 12,			4, 1, 0 },	
	{ (16 << 12) / 12,			2, 0, 1 },	
	{ (16 << 12) / 8,			4, 1, 1 },
	{ (16 << 12) / 6,			4, 1, 2 },
	{ (16 << 12) / 4,			4, 2, 2 },
	{ (16 << 12) / 3,			4, 2, 3 },
	{ (16 << 12) / 2,			4, 3, 3 },
	{ (int)((16 << 12) / 1.5),	4, 3, 4 },
	{ (int)((16 << 12) / 1.0),	4, 4, 4 },
	{ (int)((16 << 12) / 0.75),	4, 4, 5 },
	{ (int)((16 << 12) / 0.5),	4, 5, 5 }
};


#define min3( a, b, c ) (min( (a), min( (b), (c) )))

// parameters of an overlay colour space 
typedef struct {
	uint8 bpp_shift;				// log2( bytes per pixel (main plain) )
	uint8 bpuv_shift;				// log2( bytes per pixel (uv-plane) ); 
									// if there is one plane only: bpp=bpuv
	uint8 num_planes;				// number of planes
	uint8 h_uv_sub_sample_shift;	// log2( horizontal pixels per uv pair )
	uint8 v_uv_sub_sample_shift;	// log2( vertical pixels per uv pair )
	hscale_factor *factors;			// scaling/filter table
	uint8 num_factors;
} space_params;

static space_params space_params_table[16] = {
	{ 0, 0, 0, 0, 0, NULL, 0 },	// reserved
	{ 0, 0, 0, 0, 0, NULL, 0 },	// reserved
	{ 0, 0, 0, 0, 0, NULL, 0 },	// reserved
	{ 1, 1, 1, 0, 0, NULL, 0 },	// RGB15, disabled
	{ 1, 1, 1, 0, 0, NULL, 0 },	// RGB16, disabled
	{ 0, 0, 0, 0, 0, NULL, 0 },	// reserved
	{ 2, 2, 1, 0, 0, scale_RGB32, count_of( scale_RGB32 ) },	// RGB32
	{ 0, 0, 0, 0, 0, NULL, 0 },	// reserved
	{ 0, 0, 0, 0, 0, NULL, 0 },	// reserved
	{ 0, 0, 3, 2, 2, NULL, 0 },		// YUV9, disabled
	{ 0, 0, 3, 1, 1, scale_YUV12, count_of( scale_YUV12 ) },	// YUV12, three-plane
	{ 1, 1, 1, 1, 0, scale_YUV, count_of( scale_YUV ) },		// VYUY422
	{ 1, 1, 1, 1, 0, scale_YUV, count_of( scale_YUV ) },		// YVYU422
	{ 0, 1, 2, 1, 1, scale_YUV12, count_of( scale_YUV12 ) },	// YUV12, two-plane
	{ 0, 1, 2, 1, 1, NULL, 0 },	// ???
	{ 0, 0, 0, 0, 0, NULL, 0 }	// reserved
};

#define I2FF( a, shift ) ((uint32)((a) * (1 << (shift))))

static inline int ceilShiftDiv( int num, int shift ) { 	return (num + (1 << shift) - 1) >> shift; }
static inline int ceilDiv( int num, int den )
{ 
	if(den == 0) 
		return 0;
	else
		return (num + den - 1) / den;
}

// set overlay colour space transformation matrix
void ATIRadeon::SetTransform( os::color_space eFormat,
	float	    bright,
	float	    cont,
	float	    sat, 
	float	    hue,
	float	    red_intensity, 
	float	    green_intensity, 
	float	    blue_intensity,
	uint	    ref)
{
	float	    OvHueSin, OvHueCos;
	float	    CAdjOff;
	float		CAdjRY, CAdjGY, CAdjBY;
	float	    CAdjRCb, CAdjRCr;
	float	    CAdjGCb, CAdjGCr;
	float	    CAdjBCb, CAdjBCr;
	float	    RedAdj,GreenAdj,BlueAdj;
	float	    OvROff, OvGOff, OvBOff;
	float		OvRY, OvGY, OvBY;
	float	    OvRCb, OvRCr;
	float	    OvGCb, OvGCr;
	float	    OvBCb, OvBCr;
	float	    Loff;
	float	    Coff;
	
	uint32	    dwOvROff, dwOvGOff, dwOvBOff;
	uint32		dwOvRY, dwOvGY, dwOvBY;
	uint32	    dwOvRCb, dwOvRCr;
	uint32	    dwOvGCb, dwOvGCr;
	uint32	    dwOvBCb, dwOvBCr;
	
	space_transform	*trans;

	// get proper conversion formula
	switch( eFormat ) {
	case CS_YUV422:
	case CS_YUV12:
		Loff = 16 * 4;		// internal representation is 10 Bits
		Coff = 128 * 4;
		
		if (ref >= 2) 
			ref = 0;
		
		trans = &trans_yuv[ref];
		break;
	case CS_RGB32:
	default:
		Loff = 0;
		Coff = 0;
		trans = &trans_rgb;
	}
	
	OvHueSin = sin(hue);
	OvHueCos = cos(hue);
	
	// get matrix values to convert overlay colour space to RGB
	// applying colour adjustment, saturation and luma scaling
	// (saturation doesn't work with RGB input, perhaps it did with some
	//  maths; this is left to the reader :)
	CAdjRY = cont * trans->RefLuma * trans->RefRY;
	CAdjGY = cont * trans->RefLuma * trans->RefGY;
	CAdjBY = cont * trans->RefLuma * trans->RefBY;
	
	CAdjRCb = sat * -OvHueSin * trans->RefRCr;
	CAdjRCr = sat * OvHueCos * trans->RefRCr;
	CAdjGCb = sat * (OvHueCos * trans->RefGCb - OvHueSin * trans->RefGCr);
	CAdjGCr = sat * (OvHueSin * trans->RefGCb + OvHueCos * trans->RefGCr);
	CAdjBCb = sat * OvHueCos * trans->RefBCb;
	CAdjBCr = sat * OvHueSin * trans->RefBCb;
	
	// adjust black level
	CAdjOff = cont * trans[ref].RefLuma * bright * 1023.0;
	RedAdj = cont * trans[ref].RefLuma * red_intensity * 1023.0;
	GreenAdj = cont * trans[ref].RefLuma * green_intensity * 1023.0;
	BlueAdj = cont * trans[ref].RefLuma * blue_intensity * 1023.0;
	
	OvRY = CAdjRY;
	OvGY = CAdjGY;
	OvBY = CAdjBY;
	OvRCb = CAdjRCb;
	OvRCr = CAdjRCr;
	OvGCb = CAdjGCb;
	OvGCr = CAdjGCr;
	OvBCb = CAdjBCb;
	OvBCr = CAdjBCr;
	// apply offsets
	OvROff = RedAdj + CAdjOff -	CAdjRY * Loff - (OvRCb + OvRCr) * Coff;
	OvGOff = GreenAdj + CAdjOff - CAdjGY * Loff - (OvGCb + OvGCr) * Coff;
	OvBOff = BlueAdj + CAdjOff - CAdjBY * Loff - (OvBCb + OvBCr) * Coff;
	
	dwOvROff = ((int32)(OvROff * 2.0)) & 0x1fff;
	dwOvGOff = ((int32)(OvGOff * 2.0)) & 0x1fff;
	dwOvBOff = ((int32)(OvBOff * 2.0)) & 0x1fff;

	dwOvRY = (((int32)(OvRY * 2048.0))&0x7fff)<<17;
	dwOvGY = (((int32)(OvGY * 2048.0))&0x7fff)<<17;
	dwOvBY = (((int32)(OvBY * 2048.0))&0x7fff)<<17;
	dwOvRCb = (((int32)(OvRCb * 2048.0))&0x7fff)<<1;
	dwOvRCr = (((int32)(OvRCr * 2048.0))&0x7fff)<<17;
	dwOvGCb = (((int32)(OvGCb * 2048.0))&0x7fff)<<1;
	dwOvGCr = (((int32)(OvGCr * 2048.0))&0x7fff)<<17;
	dwOvBCb = (((int32)(OvBCb * 2048.0))&0x7fff)<<1;
	dwOvBCr = (((int32)(OvBCr * 2048.0))&0x7fff)<<17;

	m_cLock.Lock();
	FifoWait(6);
	OUTREG( RADEON_OV0_LIN_TRANS_A, dwOvRCb | dwOvRY );
	OUTREG( RADEON_OV0_LIN_TRANS_B, dwOvROff | dwOvRCr );
	OUTREG( RADEON_OV0_LIN_TRANS_C, dwOvGCb | dwOvGY );
	OUTREG( RADEON_OV0_LIN_TRANS_D, dwOvGOff | dwOvGCr );
	OUTREG( RADEON_OV0_LIN_TRANS_E, dwOvBCb | dwOvBY );
	OUTREG( RADEON_OV0_LIN_TRANS_F, dwOvBOff | dwOvBCr );
	m_cLock.Unlock();
	
	return;
}

static hscale_factor *getHScaleFactor( space_params *params, 
	uint32 src_left, uint32 src_right, uint32 *h_inc )
{
	uint words_per_p1_line, words_per_p23_line, max_words_per_line;
	bool p1_4tap_allowed, p23_4tap_allowed;
	uint i;
	uint num_factors;
	hscale_factor *factors;

	// check whether fifo is large enough to feed vertical 4-tap-filter
	
	words_per_p1_line = 
		ceilShiftDiv( (src_right - 1) << params->bpp_shift, 4 ) - 
		((src_left << params->bpp_shift) >> 4) + 1;
	words_per_p23_line = 
		ceilShiftDiv( (src_right - 1) << params->bpuv_shift, 4 ) - 
		((src_left << params->bpuv_shift) >> 4) + 1;
		
	// overlay buffer for one line; this value is probably 
	// higher on newer Radeons (or smaller on older Radeons?)
	max_words_per_line = 96;

	switch( params->num_planes ) {
	case 3:
		p1_4tap_allowed = words_per_p1_line < max_words_per_line / 2;
		p23_4tap_allowed = words_per_p23_line < max_words_per_line / 4;
		break;
	case 2:
		p1_4tap_allowed = words_per_p1_line < max_words_per_line / 2;
		p23_4tap_allowed = words_per_p23_line < max_words_per_line / 2;
		break;
	case 1:
	default:
		p1_4tap_allowed = p23_4tap_allowed = words_per_p1_line < max_words_per_line;
		break;
	}
	

	// search for proper scaling/filter entry
	factors = params->factors;
	num_factors = params->num_factors;
		
	if( factors == NULL || num_factors == 0 )
		return NULL;
		
	for( i = 0; i < num_factors; ++i, ++factors ) {
		if( *h_inc <= factors->max_scale && 
			(factors->p1_step_by > 0 || p1_4tap_allowed) &&
			(factors->p23_step_by > 0 || p23_4tap_allowed))
			break;
	}
	
	if( i == num_factors ) {
		// overlay is asked to be scaled down more than allowed,
		// so use least scaling factor supported
		--factors;
		*h_inc = factors->max_scale;
	}
	
	return factors;
}			

void ATIRadeon::SetOverlayColorKey( os::Color32_s sColorKey )
{
	uint32 m_nColorKey;
	uint32 key_msk = 0xffffff;
	
	switch( BitsPerPixel( m_sCurrentMode.m_eColorSpace ) )
	{
		case 16:
			key_msk = 0xFFFF;
			m_nColorKey = COL_TO_RGB16( sColorKey );
			FifoWait(3);
			OUTREG(RADEON_OV0_GRAPHICS_KEY_MSK, key_msk);
			OUTREG(RADEON_OV0_GRAPHICS_KEY_CLR, m_nColorKey);
			OUTREG(RADEON_OV0_KEY_CNTL,(RADEON_VIDEO_KEY_FN_TRUE|
					RADEON_GRAPHIC_KEY_FN_EQ|RADEON_CMP_MIX_AND));
			break;
		default:
		case 32:
			key_msk = 0xFFFFFF;
			m_nColorKey = COL_TO_RGB32( sColorKey );
			OUTREG(RADEON_OV0_GRAPHICS_KEY_CLR_LOW, m_nColorKey);
			OUTREG(RADEON_OV0_GRAPHICS_KEY_CLR_HIGH, m_nColorKey);
			OUTREG(RADEON_OV0_KEY_CNTL,(RADEON_VIDEO_KEY_FN_TRUE|
					RADEON_GRAPHIC_KEY_FN_EQ|RADEON_CMP_MIX_AND));
			break;
	}

	m_cColorKey = sColorKey;

	return;
}

bool ATIRadeon::CreateVideoOverlay( const os::IPoint& cSize, const os::IRect& cDst, 
								os::color_space eFormat, os::Color32_s sColorKey, area_id *pBuffer )
{
	if( rinfo.family >= CHIP_FAMILY_R300 )
	{
		dbprintf("Radeon :: Video overlay not supported on Radeon 9500 or higher\n");
		dbprintf("Radeon :: Please use video bitmap output driver!\n");
		return( false );
	}
	if( ( eFormat == CS_YUV422 || eFormat == CS_RGB32 ) 
			&& (!m_bVideoOverlayUsed || m_bOVToRecreate) ) {
		uint i;
		uint32 ecp_div = 0;
		uint32 v_inc, h_inc;
		uint32 src_v_inc, src_h_inc;
		uint32 src_left, src_top, src_right, src_bottom;
		int32 dest_left, dest_top, dest_right, dest_bottom;
		uint32 offset1, offset, rel_offset, totalsize, bytes_per_row;
		uint32 tmp;
		uint32 p1_h_accum_init, p23_h_accum_init, p1_v_accum_init, p23_v_accum_init;
		uint32 p1_active_lines, p23_active_lines;
		hscale_factor *factors;
		space_params *params;
	
		uint32 p1_h_inc, p23_h_inc;
		uint32 p1_x_start, p1_x_end;
		uint32 p23_x_start, p23_x_end;

		uint ati_space, test_reg, bpp;

		if(!m_bOVToRecreate)
		{
			m_cLock.Lock();
			EngineIdle();

			FifoWait(9);
			OUTREG(RADEON_OV0_SCALE_CNTL, RADEON_SCALER_SOFT_RESET );
			OUTREG(RADEON_OV0_AUTO_FLIP_CNTRL, RADEON_OV0_VID_PORT_SELECT_SOFTWARE );
			OUTREG(RADEON_OV0_FILTER_CNTL, 			// use fixed filter coefficients
				RADEON_OV0_HC_COEF_ON_HORZ_Y |
				RADEON_OV0_HC_COEF_ON_HORZ_UV |
				RADEON_OV0_HC_COEF_ON_VERT_Y |
				RADEON_OV0_HC_COEF_ON_VERT_UV );
			OUTREG(RADEON_OV0_KEY_CNTL, RADEON_GRAPHIC_KEY_FN_EQ |
				RADEON_VIDEO_KEY_FN_FALSE |
				RADEON_CMP_MIX_OR );
			OUTREG(RADEON_OV0_TEST, 0 );
			//OUTREG( 0x0910, 4 );	// disable capture clock
			//OUTREG( RADEON_CAP0_TRIG_CNTL, 0 );				// disable capturing
			OUTREG(RADEON_OV0_REG_LOAD_CNTL, 0 );
			// tell deinterlacer to always show recent field
			OUTREG(RADEON_OV0_DEINTERLACE_PATTERN, 
				0xaaaaa | (9 << RADEON_OV0_DEINT_PAT_LEN_M1_SHIFT) );

			// set gamma
			for( i = 0; i < sizeof( std_gamma ) / sizeof( std_gamma[0] ); ++i ) {
				if( !std_gamma[i].r200_or_above || rinfo.family >= CHIP_FAMILY_R200) {
					FifoWait(1);
					OUTREG( std_gamma[i].reg,	
						(std_gamma[i].slope << 16) | std_gamma[i].offset );
				}
			}

			// overlay unit can only handle up to 175 MHz, if pixel clock is higher,
			// only every second pixel is handled
			//if( rinfo.dotClock < 17500 )
			//	ecp_div = 0;
			//else
			//	ecp_div = 1;
			ecp_div = (INPLL(RADEON_VCLK_ECP_CNTL) >> 8) & 3;

			//FifoWait(1);
			//OUTPLLP( RADEON_VCLK_ECP_CNTL, ecp_div << RADEON_ECP_DIV_SHIFT, ~RADEON_ECP_DIV_MASK );

			SetOverlayColorKey( sColorKey );

			EngineIdle();
			m_cLock.Unlock();
		}

    	/* Calculate offset */
		switch( eFormat ) {
		case CS_YUV422:
			bpp = 2;
			ati_space = RADEON_SCALER_SOURCE_YVYU422 >> 8;
			test_reg = 0;
			break;
		case CS_RGB32:
		default:
			bpp = 4; 
			ati_space = RADEON_SCALER_SOURCE_32BPP >> 8;
			test_reg = 0;
			break;
		}

		bytes_per_row = (cSize.x * bpp + 0xff) & ~0xff;
		totalsize = bytes_per_row * cSize.y;
    	offset1 = PAGE_ALIGN( rinfo.video_ram - totalsize - PAGE_SIZE );

		*pBuffer = create_area( "radeon_overlay", NULL, PAGE_ALIGN( totalsize ), AREA_FULL_ACCESS, AREA_NO_LOCK );
		if( *pBuffer < 0 )
			return( false );
		remap_area( *pBuffer, (void*)(( rinfo.fb_base_phys ) + offset1) );
		
		/* Start video */
		// scaling is independant of clipping, get this first
		{
			uint32 src_width, src_height;

			src_width = cSize.x;
			src_height = cSize.y;
	
			// this is for graphics card
			v_inc = (src_height << 20) / cDst.Height();
			h_inc = (src_width << (12 + ecp_div)) / cDst.Width();
	
			// this is for us	
			src_v_inc = (src_height << 16) / cDst.Height();
			src_h_inc = (src_width << 16) / cDst.Width();
		}

		// calculate unclipped position/size
		// TBD: I assume that overlay_window.offset_xyz is only a hint where 
		//      no overlay is visible; another interpretation were to zoom 
		//      the overlay so it fits into remaining space
		//src_left = /*(ov.h_start << 16) + cDst.left * src_h_inc*/ 0;
		//src_top = /*(ov.v_start << 16) + cDst.top * src_v_inc*/ 0;
		//src_right = ((ov.h_start + cSize.x) << 16) - cDst.right * src_h_inc;
		//src_bottom = ((ov.v_start + cSize.y) << 16) - cDst.top * src_v_inc;
		src_left = 0;
		src_top = 0;
		src_right = cSize.x << 16;
		src_bottom = cSize.y << 16;
		dest_left = cDst.left;
		dest_top = cDst.top;
		dest_right = cDst.right;
		dest_bottom = cDst.bottom;

		// let's calculate all those nice register values
		params = &space_params_table[ati_space];

		// choose proper scaler
		{
			factors = getHScaleFactor( params, src_left >> 16, src_right >> 16, &h_inc );
			if( factors == NULL )
			{
				delete_area( *pBuffer );
				return( false );
			}
			
			p1_h_inc = factors->p1_step_by > 0 ? 
				h_inc >> (factors->p1_step_by - 1) : h_inc;
			p23_h_inc = 
				(factors->p23_step_by > 0 ? h_inc >> (factors->p23_step_by - 1) : h_inc) 
				>> params->h_uv_sub_sample_shift;
		}

		// get register value for start/end position of overlay image (pixel-precise only)
		{
			uint32 p1_step_size, p23_step_size;
			uint32 p1_left, p1_right, p1_width;
			uint32 p23_left, p23_right, p23_width;
		
			p1_left = src_left >> 16;
			p1_right = src_right >> 16;
			p1_width = p1_right - p1_left;
		
			p1_step_size = factors->p1_step_by > 0 ? (1 << (factors->p1_step_by - 1)) : 1;
			p1_x_start = p1_left % (16 >> params->bpp_shift);
			p1_x_end = ((p1_x_start + p1_width - 1) / p1_step_size) * p1_step_size;
			
			p23_left = (src_left >> 16) >> params->h_uv_sub_sample_shift;
			p23_right = (src_right >> 16) >> params->h_uv_sub_sample_shift;
			p23_width = p23_right - p23_left;
	
			p23_step_size = factors->p23_step_by > 0 ? (1 << (factors->p23_step_by - 1)) : 1;
			// if resolution of Y and U/V differs but YUV are stored in one 
			// plane then UV alignment depends on Y data, therefore the hack
			// (you are welcome to replace this with some cleaner code ;)
			p23_x_start = p23_left % 
				((16 >> params->bpuv_shift) / 
				 (ati_space == 11 || ati_space == 12 ? 2 : 1));
			p23_x_end = (int)((p23_x_start + p23_width - 1) / p23_step_size) * p23_step_size;
		
			// get memory location of first word to be read by scaler
			// (save relative offset for fast update)
			rel_offset = (src_top >> 16) * bytes_per_row + 
				((p1_left << params->bpp_shift) & ~0xf);
			offset = offset1 + rel_offset;
		
		}
	
		// get active lines for scaler
		// (we could add additional blank lines for DVD letter box mode,
		//  but this is not supported by API; additionally, this only makes
		//  sense if want to put subtitles onto the black border, which is
		//  supported neither)
		{
			uint16 int_top, int_bottom;
		
			int_top = src_top >> 16;
			int_bottom = (src_bottom >> 16);
		
			p1_active_lines = int_bottom - int_top - 1;
			p23_active_lines = 
				ceilShiftDiv( int_bottom - 1, params->v_uv_sub_sample_shift ) - 
				(int_top >> params->v_uv_sub_sample_shift);
		}
	
		// if picture is stretched for flat panel, we need to scale all
		// vertical values accordingly
		// TBD: there is no description at all concerning this, so v_accum_init may
		//      need to be initialized based on original value
#if 0
		{
			if( rinfo.mon1_type == MT_LCD ) {
				uint64 v_ratio;
			
				// convert 32.32 format to 16.16 format; else we 
				// cannot multiply two fixed point values without
				// overflow
				v_ratio = si->fp_port.v_ratio >> (FIX_SHIFT - 16);
			
				v_inc = (v_inc * v_ratio) >> 16;
			}
		}
#endif
	
		// get initial horizontal scaler values, taking care of precharge
		// don't ask questions about formulas - take them as is
		// (TBD: home-brewed sub-pixel source clipping may be wrong, 
		//       especially for uv-planes)
		{
			uint32 p23_group_size;

	 	   tmp = ((src_left & 0xffff) >> 11) + (
	 	   		(
			    	I2FF( p1_x_start % factors->group_size, 12 ) + 
			    	I2FF( 2.5, 12 ) + 
			    	p1_h_inc / 2 +
			    	I2FF( 0.5, 12-5 )	// rounding
		        ) >> (12 - 5));	// scaled by 1 << 5
	        

			p1_h_accum_init = 
				((tmp << 15) & RADEON_OV0_P1_H_ACCUM_INIT_MASK) |
				((tmp << 23) & RADEON_OV0_P1_PRESHIFT_MASK);
		
		
			p23_group_size = 2;
		
			tmp = ((src_left & 0xffff) >> 11) + (
				(
					I2FF( p23_x_start % p23_group_size, 12 ) + 
					I2FF( 2.5, 12 ) +
					p23_h_inc / 2 +
					I2FF( 0.5, 12-5 )	// rounding 
				) >> (12 - 5)); // scaled by 1 << 5
		
			p23_h_accum_init = 
				((tmp << 15) & RADEON_OV0_P23_H_ACCUM_INIT_MASK) |
				((tmp << 23) & RADEON_OV0_P23_PRESHIFT_MASK);
		}

		// get initial vertical scaler values, taking care of precharge
		{
			uint extra_full_line;

			extra_full_line = factors->p1_step_by == 0 ? 1 : 0;
	
		    tmp = ((src_top & 0x0000ffff) >> 11) + (
	 		   	(min( 
			    	I2FF( 1.5, 20 ) + I2FF( extra_full_line, 20 ) + v_inc / 2, 
		    		I2FF( 2.5, 20 ) + 2 * I2FF( extra_full_line, 20 )
		    	 ) + I2FF( 0.5, 20-5 )) // rounding
		    	>> (20 - 5)); // scaled by 1 << 5
	    	

			p1_v_accum_init = 
				((tmp << 15) & RADEON_OV0_P1_V_ACCUM_INIT_MASK) | 0x00000001;

	
			extra_full_line = factors->p23_step_by == 0 ? 1 : 0;
	
			if( params->v_uv_sub_sample_shift > 0 ) {
				tmp = ((src_top & 0x0000ffff) >> 11) + (
					(min( 
						I2FF( 1.5, 20 ) + 
							I2FF( extra_full_line, 20 ) + 
							((v_inc / 2) >> params->v_uv_sub_sample_shift), 
						I2FF( 2.5, 20 ) + 
							2 * I2FF( extra_full_line, 20 )
					 ) + I2FF( 0.5, 20-5 )) // rounding
					>> (20 - 5)); // scaled by 1 << 5
			} else {
				tmp = ((src_top & 0x0000ffff) >> 11) + (
					(
						I2FF( 2.5, 20 ) + 
						2 * I2FF( extra_full_line, 20 ) +
						I2FF( 0.5, 20-5 )	// rounding
					) >> (20 - 5)); // scaled by 1 << 5
			}
	
			p23_v_accum_init = 
				((tmp << 15) & RADEON_OV0_P23_V_ACCUM_INIT_MASK) | 0x00000001;		
		}

		// show me what you've got!
		// we could lock double buffering of overlay unit during update
		// (new values are copied during vertical blank, so if we've updated
		// only some of them, you get a whole frame of mismatched values)
		// but during tests I couldn't get the artifacts go away, so
		// we use the dangerous way which has the pro to not require any
		// waiting
		m_cLock.Lock();

		EngineIdle();

		FifoWait(19);
		OUTREG( RADEON_OV0_VID_BUF0_BASE_ADRS, offset );
		OUTREG( RADEON_OV0_VID_BUF_PITCH0_VALUE, bytes_per_row );
	
		OUTREG( RADEON_OV0_H_INC, p1_h_inc | (p23_h_inc << 16) );
		OUTREG( RADEON_OV0_STEP_BY, factors->p1_step_by | (factors->p23_step_by << 8) );
		OUTREG( RADEON_OV0_V_INC, v_inc );
	
		OUTREG( RADEON_OV0_Y_X_START, (dest_left) | (dest_top << 16) );
		OUTREG( RADEON_OV0_Y_X_END, (dest_right - 1) | ((dest_bottom - 1) << 16) );

		OUTREG( RADEON_OV0_P1_BLANK_LINES_AT_TOP, RADEON_P1_BLNK_LN_AT_TOP_M1_MASK
			| (p1_active_lines << 16) );
		OUTREG( RADEON_OV0_P1_X_START_END, p1_x_end | (p1_x_start << 16) );
		OUTREG( RADEON_OV0_P1_H_ACCUM_INIT, p1_h_accum_init );
		OUTREG( RADEON_OV0_P1_V_ACCUM_INIT, p1_v_accum_init );

		OUTREG( RADEON_OV0_P23_BLANK_LINES_AT_TOP, RADEON_P23_BLNK_LN_AT_TOP_M1_MASK
			| (p23_active_lines << 16) );
		OUTREG( RADEON_OV0_P2_X_START_END, p23_x_end | (p23_x_start << 16) );
		OUTREG( RADEON_OV0_P3_X_START_END, p23_x_end | (p23_x_start << 16) );
		OUTREG( RADEON_OV0_P23_H_ACCUM_INIT, p23_h_accum_init );
		OUTREG( RADEON_OV0_P23_V_ACCUM_INIT, p23_v_accum_init );
	
		OUTREG( RADEON_OV0_TEST, test_reg );
		OUTREG( RADEON_OV0_SCALE_CNTL, RADEON_SCALER_ENABLE | RADEON_SCALER_DOUBLE_BUFFER | 
			(ati_space << 8) | /*RADEON_SCALER_ADAPTIVE_DEINT |*/ ( 0 ) );
	
		//si->overlay_mgr.auto_flip_reg ^= RADEON_OV0_SOFT_EOF_TOGGLE;
	
		OUTREG( RADEON_OV0_AUTO_FLIP_CNTRL, RADEON_OV0_VID_PORT_SELECT_SOFTWARE
			^RADEON_OV0_SOFT_EOF_TOGGLE );
	
		m_cLock.Unlock();
  	 	
		m_bOVToRecreate = false;
		m_bVideoOverlayUsed = true;
		return( true );
	}
	return( false );
}

bool ATIRadeon::RecreateVideoOverlay( const os::IPoint& cSize, const os::IRect& cDst, 
								os::color_space eFormat, area_id *pBuffer )
{
	if( eFormat == CS_YUV422 || eFormat == CS_RGB32 ) {
		delete_area( *pBuffer );
		m_bOVToRecreate = true;
		return( CreateVideoOverlay( cSize, cDst, eFormat, m_cColorKey, pBuffer ) );
	}
	return( false );
}

void ATIRadeon::DeleteVideoOverlay( area_id *pBuffer )
{
	if( m_bVideoOverlayUsed ) {

		//m_cLock.Lock();
		EngineIdle();

		FifoWait(7);
		OUTREG(RADEON_OV0_SCALE_CNTL, RADEON_SCALER_SOFT_RESET );
		OUTREG(RADEON_OV0_AUTO_FLIP_CNTRL, RADEON_OV0_VID_PORT_SELECT_SOFTWARE );
		OUTREG(RADEON_OV0_FILTER_CNTL, 			// use fixed filter coefficients
			RADEON_OV0_HC_COEF_ON_HORZ_Y |
			RADEON_OV0_HC_COEF_ON_HORZ_UV |
			RADEON_OV0_HC_COEF_ON_VERT_Y |
			RADEON_OV0_HC_COEF_ON_VERT_UV );
		OUTREG(RADEON_OV0_KEY_CNTL, RADEON_GRAPHIC_KEY_FN_EQ |
			RADEON_VIDEO_KEY_FN_FALSE |
			RADEON_CMP_MIX_OR );
		OUTREG(RADEON_OV0_TEST, 0 );
		//	OUTREG( RADEON_FCP_CNTL, RADEON_FCP_CNTL_GND );	// disable capture clock
		//	OUTREG( regs, RADEON_CAP0_TRIG_CNTL, 0 );				// disable capturing
		OUTREG(RADEON_OV0_REG_LOAD_CNTL, 0 );
		// tell deinterlacer to always show recent field
		OUTREG(RADEON_OV0_DEINTERLACE_PATTERN, 
			0xaaaaa | (9 << RADEON_OV0_DEINT_PAT_LEN_M1_SHIFT) );

		EngineIdle();
		//m_cLock.Unlock();

		delete_area( *pBuffer );
	}
	m_bOVToRecreate = false;
	m_bVideoOverlayUsed = false;
}


