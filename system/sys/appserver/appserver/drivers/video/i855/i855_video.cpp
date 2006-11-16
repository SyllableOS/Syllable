
/***************************************************************************
 
Copyright 2000 Intel Corporation.  All Rights Reserved. 

Permission is hereby granted, free of charge, to any person obtaining a 
copy of this software and associated documentation files (the 
"Software"), to deal in the Software without restriction, including 
without limitation the rights to use, copy, modify, merge, publish, 
distribute, sub license, and/or sell copies of the Software, and to 
permit persons to whom the Software is furnished to do so, subject to 
the following conditions: 

The above copyright notice and this permission notice (including the 
next paragraph) shall be included in all copies or substantial portions 
of the Software. 

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS 
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF 
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. 
IN NO EVENT SHALL INTEL, AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, 
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR 
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR 
THE USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/*
 * i830_video.cpp: i830/i845 Xv driver. 
 *
 * Copyright 2002 by Alan Hourihane and David Dawes
 *
 * Authors: 
 *	Alan Hourihane <alanh@tungstengraphics.com>
 *	David Dawes <dawes@xfree86.org>
 *
 * Derived from i810 Xv driver:
 *
 * Authors of i810 code:
 * 	Jonathan Bian <jonathan.bian@intel.com>
 *      Offscreen Images:
 *        Matt Sottek <matthew.j.sottek@intel.com>
 */

/* Note: Enable YV12 video overlay if the media lib has changed */

#define IMAGE_MAX_WIDTH		1440
#define IMAGE_MAX_HEIGHT	1080
#define Y_BUF_SIZE		(IMAGE_MAX_WIDTH * IMAGE_MAX_HEIGHT)

#include <gui/bitmap.h>
#include "i855.h"

void i855::UpdateVideo()
{
	m_cGELock.Lock();

	BEGIN_RING( 8 );
	OUTRING( MI_FLUSH | MI_WRITE_DIRTY_STATE );
	OUTRING( MI_NOOP );
	if ( !m_bVideoIsOn )
	{
		OUTRING( MI_NOOP );
		OUTRING( MI_NOOP );
		OUTRING( MI_OVERLAY_FLIP | MI_OVERLAY_FLIP_ON );
		m_bVideoIsOn = true;
	}
	else
	{
		OUTRING( MI_WAIT_FOR_EVENT | MI_WAIT_FOR_OVERLAY_FLIP );
		OUTRING( MI_NOOP );
		OUTRING( MI_OVERLAY_FLIP | MI_OVERLAY_FLIP_CONTINUE );
	}
	OUTRING( m_nVideoAddress | 1 );
	OUTRING( MI_WAIT_FOR_EVENT | MI_WAIT_FOR_OVERLAY_FLIP );
	OUTRING( MI_NOOP );	
	FLUSH_RING();

	m_cGELock.Unlock();
}

void i855::VideoOff()
{
	m_cGELock.Lock();

	if ( m_bVideoIsOn )
	{
		BEGIN_RING( 6 );
		OUTRING( MI_FLUSH | MI_WRITE_DIRTY_STATE );
		OUTRING( MI_NOOP );
		OUTRING( MI_OVERLAY_FLIP | MI_OVERLAY_FLIP_OFF );
		OUTRING( m_nVideoAddress | 1 );
		OUTRING( MI_WAIT_FOR_EVENT | MI_WAIT_FOR_OVERLAY_FLIP );
		OUTRING( MI_NOOP );
		FLUSH_RING();
		m_bVideoIsOn = false;
	}

	m_cGELock.Unlock();
}

void i855::ResetVideo()
{
	memset( m_psVidRegs, 0, sizeof( *m_psVidRegs ) );

	m_psVidRegs->YRGB_VPH = 0;
	m_psVidRegs->UV_VPH = 0;
	m_psVidRegs->HORZ_PH = 0;
	m_psVidRegs->INIT_PHS = 0;
	m_psVidRegs->DWINPOS = 0;
	m_psVidRegs->DWINSZ = ( IMAGE_MAX_HEIGHT << 16 ) | IMAGE_MAX_WIDTH;
	m_psVidRegs->SWIDTH = IMAGE_MAX_WIDTH | ( IMAGE_MAX_WIDTH << 16 );
	m_psVidRegs->SWIDTHSW = ( IMAGE_MAX_WIDTH >> 3 ) | ( IMAGE_MAX_WIDTH << 12 );
	m_psVidRegs->SHEIGHT = IMAGE_MAX_HEIGHT | ( IMAGE_MAX_HEIGHT << 15 );
	m_psVidRegs->OCLRC0 = 0x01000000;	/* brightness: 0 contrast: 1.0 */
	m_psVidRegs->OCLRC1 = 0x00000080;	/* saturation: bypass */
	m_psVidRegs->AWINPOS = 0;
	m_psVidRegs->AWINSZ = 0;
	m_psVidRegs->FASTHSCALE = 0;

	/*
	 * Enable destination color keying
	 */
	switch ( GetCurrentScreenMode().m_eColorSpace )
	{
	case os::CS_CMAP8:
		m_psVidRegs->DCLRKV = 0;
		m_psVidRegs->DCLRKM = 0xffffff | DEST_KEY_ENABLE;
		break;
	case os::CS_RGB16:
		m_psVidRegs->DCLRKV = RGB16ToColorKey( m_nVideoColorKey );
		m_psVidRegs->DCLRKM = 0x070307 | DEST_KEY_ENABLE;
		break;
	default:
		m_psVidRegs->DCLRKV = m_nVideoColorKey;
		m_psVidRegs->DCLRKM = DEST_KEY_ENABLE;
		break;
	}

	m_psVidRegs->SCLRKVH = 0;
	m_psVidRegs->SCLRKVL = 0;
	m_psVidRegs->SCLRKEN = 0;	/* source color key disable */
	m_psVidRegs->OCONFIG = CC_OUT_8BIT;

	/*
	 * Select which pipe the overlay is enabled on.  Give preference to
	 * pipe A.
	 */
	if ( m_nDisplayPipe == 0 )
		m_psVidRegs->OCONFIG |= OVERLAY_PIPE_A;
	else
		m_psVidRegs->OCONFIG |= OVERLAY_PIPE_B;

	m_psVidRegs->OCMD = YUV_420;

	/* setup hwstate */
	/* Default gamma correction values. */
	m_nVideoGAMC5 = 0xc0c0c0;
	m_nVideoGAMC4 = 0x808080;
	m_nVideoGAMC3 = 0x404040;
	m_nVideoGAMC2 = 0x202020;
	m_nVideoGAMC1 = 0x101010;
	m_nVideoGAMC0 = 0x080808;

	OUTREG( OGAMC5, m_nVideoGAMC5 );
	OUTREG( OGAMC4, m_nVideoGAMC4 );
	OUTREG( OGAMC3, m_nVideoGAMC3 );
	OUTREG( OGAMC2, m_nVideoGAMC2 );
	OUTREG( OGAMC1, m_nVideoGAMC1 );
	OUTREG( OGAMC0, m_nVideoGAMC0 );
}

bool i855::SetCoeffRegs( double *coeff, int mantSize, coeffPtr pCoeff, int pos )
{
	int maxVal, icoeff, res;
	int sign;
	double c;

	sign = 0;
	maxVal = 1 << mantSize;
	c = *coeff;
	if ( c < 0.0 )
	{
		sign = 1;
		c = -c;
	}

	res = 12 - mantSize;
	if ( ( icoeff = ( int )( c * 4 * maxVal + 0.5 ) ) < maxVal )
	{
		pCoeff[pos].exponent = 3;
		pCoeff[pos].mantissa = icoeff << res;
		*coeff = ( double )icoeff / ( double )( 4 * maxVal );
	}
	else if ( ( icoeff = ( int )( c * 2 * maxVal + 0.5 ) ) < maxVal )
	{
		pCoeff[pos].exponent = 2;
		pCoeff[pos].mantissa = icoeff << res;
		*coeff = ( double )icoeff / ( double )( 2 * maxVal );
	}
	else if ( ( icoeff = ( int )( c * maxVal + 0.5 ) ) < maxVal )
	{
		pCoeff[pos].exponent = 1;
		pCoeff[pos].mantissa = icoeff << res;
		*coeff = ( double )icoeff / ( double )( maxVal );
	}
	else if ( ( icoeff = ( int )( c * maxVal * 0.5 + 0.5 ) ) < maxVal )
	{
		pCoeff[pos].exponent = 0;
		pCoeff[pos].mantissa = icoeff << res;
		*coeff = ( double )icoeff / ( double )( maxVal / 2 );
	}
	else
	{
		/* Coeff out of range */
		return false;
	}

	pCoeff[pos].sign = sign;
	if ( sign )
		*coeff = -( *coeff );
	return true;
}

void i855::UpdateCoeff( int taps, double fCutoff, bool isHoriz, bool isY, coeffPtr pCoeff )
{
	int i, j, j1, num, pos, mantSize;
	double pi = 3.1415926535, val, sinc, window, sum;
	double rawCoeff[MAX_TAPS * 32], coeffs[N_PHASES][MAX_TAPS];
	double diff;
	int tapAdjust[MAX_TAPS], tap2Fix;
	bool isVertAndUV;

	if ( isHoriz )
		mantSize = 7;
	else
		mantSize = 6;

	isVertAndUV = !isHoriz && !isY;
	num = taps * 16;
	for ( i = 0; i < num * 2; i++ )
	{
		val = ( 1.0 / fCutoff ) * taps * pi * ( i - num ) / ( 2 * num );
		if ( val == 0.0 )
			sinc = 1.0;
		else
			sinc = sin( val ) / val;

		/* Hamming window */
		window = ( 0.5 - 0.5 * cos( i * pi / num ) );
		rawCoeff[i] = sinc * window;
	}

	for ( i = 0; i < N_PHASES; i++ )
	{
		/* Normalise the coefficients. */
		sum = 0.0;
		for ( j = 0; j < taps; j++ )
		{
			pos = i + j * 32;
			sum += rawCoeff[pos];
		}
		for ( j = 0; j < taps; j++ )
		{
			pos = i + j * 32;
			coeffs[i][j] = rawCoeff[pos] / sum;
		}

		/* Set the register values. */
		for ( j = 0; j < taps; j++ )
		{
			pos = j + i * taps;
			if ( ( j == ( taps - 1 ) / 2 ) && !isVertAndUV )
				SetCoeffRegs( &coeffs[i][j], mantSize + 2, pCoeff, pos );
			else
				SetCoeffRegs( &coeffs[i][j], mantSize, pCoeff, pos );
		}

		tapAdjust[0] = ( taps - 1 ) / 2;
		for ( j = 1, j1 = 1; j <= tapAdjust[0]; j++, j1++ )
		{
			tapAdjust[j1] = tapAdjust[0] - j;
			tapAdjust[++j1] = tapAdjust[0] + j;
		}

		/* Adjust the coefficients. */
		sum = 0.0;
		for ( j = 0; j < taps; j++ )
			sum += coeffs[i][j];
		if ( sum != 1.0 )
		{
			for ( j1 = 0; j1 < taps; j1++ )
			{
				tap2Fix = tapAdjust[j1];
				diff = 1.0 - sum;
				coeffs[i][tap2Fix] += diff;
				pos = tap2Fix + i * taps;
				if ( ( tap2Fix == ( taps - 1 ) / 2 ) && !isVertAndUV )
					SetCoeffRegs( &coeffs[i][tap2Fix], mantSize + 2, pCoeff, pos );
				else
					SetCoeffRegs( &coeffs[i][tap2Fix], mantSize, pCoeff, pos );

				sum = 0.0;
				for ( j = 0; j < taps; j++ )
					sum += coeffs[i][j];
				if ( sum == 1.0 )
					break;
			}
		}
	}
}

bool i855::SetupVideo( const os::IPoint & cSize, const os::IRect & cDst, os::color_space eFormat, area_id *pBuffer )
{
	/* Calculate offset */
	uint32 pitch = 0;
	uint32 totalSize = 0;
	uint32 offset = 0;

	os::IRect cDest = cDst;
	int nDestHeight = cDest.Height() + 1;

	if( eFormat == os::CS_YUV12 ) {
		pitch = ( ( cSize.x + 0x1ff ) & ~0x1ff ) << 1;
		totalSize = pitch * cSize.y;	
		pitch = ( ( cSize.x ) + 0x1ff ) & ~0x1ff;
	} else {
		pitch = ( ( cSize.x << 1 ) + 255 ) & ~255;
		totalSize = pitch * cSize.y;
	}

	if( AllocateMemory( totalSize, &offset ) != 0 )
		return( false );
	offset += m_nFrameBufferOffset;

	*pBuffer = create_area( "i855_overlay", NULL, PAGE_ALIGN( totalSize ), AREA_FULL_ACCESS, AREA_NO_LOCK );
	if ( *pBuffer < 0 )
		return ( false );
	if ( remap_area( *pBuffer, ( void * )( ( m_cPCIInfo.u.h0.nBase0 & PCI_ADDRESS_MEMORY_32_MASK ) + offset ) ) < 0 )
		return ( false );
		
	//dbprintf( "Video @ 0x%x %i %i %i %i %i %i %i %i\n", (uint)offset, cSize.x, cSize.y, pitch, 
		//	(int)cDst.left, (int)cDst.top, (int)cDst.right, (int)cDst.bottom );
	
	m_cGELock.Lock();
	
	
	if( cDst.left < 0 || cDst.top < 0 || cDst.right >= GetCurrentScreenMode().m_nWidth ||
		cDst.bottom >= GetCurrentScreenMode().m_nHeight )
	{
		UpdateVideo();
		VideoOff();
		m_cGELock.Unlock();
		return( true );
	}
	
	m_psVidRegs->OBUF_0Y = offset;
	m_psVidRegs->OBUF_0U = offset + ( pitch * cSize.y * 5 / 4 );
	m_psVidRegs->OBUF_0V = offset + ( pitch * cSize.y );
	
	
	if( eFormat == os::CS_YUV12 ) {
		unsigned int nSWidth = ( ( cSize.x + 0x1ff ) & ~0x1ff );
		m_psVidRegs->SWIDTH = ( nSWidth << 15 ) | nSWidth;
		m_psVidRegs->SWIDTHSW = ( nSWidth << 12 ) | ( nSWidth >> 3 );
	} else {
		unsigned int nSWidth = ( ( cSize.x << 1 ) + 255 ) & ~255;

		m_psVidRegs->SWIDTH = nSWidth;
		m_psVidRegs->SWIDTHSW = ( nSWidth ) >> 3;
	}
	m_psVidRegs->SHEIGHT = cSize.y | ( ( cSize.y / 2 ) << 16 );

	if ( m_bVideoOneLine )
	{
		cDest.top = ( ( ( cDest.top - 1 ) * m_nVideoScale ) >> 16 ) + 1;
		cDest.bottom = ( ( cDest.bottom * m_nVideoScale ) >> 16 ) + 1;

		nDestHeight = cDest.bottom - cDest.top + 1;
		if ( nDestHeight < cSize.y )
			nDestHeight = cSize.y;
	}

	m_psVidRegs->DWINPOS = ( cDest.top << 16 ) | cDest.left;
	m_psVidRegs->DWINSZ = ( ( cDest.Height() + 1 ) << 16 ) | ( cDest.Width(  ) + 1 );

	
	m_psVidRegs->OCMD = OVERLAY_ENABLE;

	/* 
	 * Calculate horizontal and vertical scaling factors and polyphase
	 * coefficients.
	 */

	{
		bool scaleChanged = false;
		int xscaleInt, xscaleFract, yscaleInt, yscaleFract;
		int xscaleIntUV, xscaleFractUV;
		int yscaleIntUV, yscaleFractUV;

		/* UV is half the size of Y -- YUV420 */
		int uvratio = 2;
		uint32 newval;
		coeffRec xcoeffY[N_HORIZ_Y_TAPS * N_PHASES];
		coeffRec xcoeffUV[N_HORIZ_UV_TAPS * N_PHASES];
		int i, j, pos;

		/*
		 * Y down-scale factor as a multiple of 4096.
		 */
		xscaleFract = ( ( cSize.x/* + 1*/ ) << 12 ) / ( cDest.Width() + 1 );
		yscaleFract = ( ( cSize.y/* + 1*/ ) << 12 ) / nDestHeight;

		/* Calculate the UV scaling factor. */
		xscaleFractUV = xscaleFract / uvratio;
		yscaleFractUV = yscaleFract / uvratio;

		/*
		 * To keep the relative Y and UV ratios exact, round the Y scales
		 * to a multiple of the Y/UV ratio.
		 */
		xscaleFract = xscaleFractUV * uvratio;
		yscaleFract = yscaleFractUV * uvratio;

		/* Integer (un-multiplied) values. */
		xscaleInt = xscaleFract >> 12;
		yscaleInt = yscaleFract >> 12;

		xscaleIntUV = xscaleFractUV >> 12;
		yscaleIntUV = yscaleFractUV >> 12;


		newval = ( xscaleInt << 16 ) | ( ( xscaleFract & 0xFFF ) << 3 ) | ( ( yscaleFract & 0xFFF ) << 20 );
		if ( newval != m_psVidRegs->YRGBSCALE )
		{
			scaleChanged = true;
			m_psVidRegs->YRGBSCALE = newval;
		}

		newval = ( xscaleIntUV << 16 ) | ( ( xscaleFractUV & 0xFFF ) << 3 ) | ( ( yscaleFractUV & 0xFFF ) << 20 );
		if ( newval != m_psVidRegs->UVSCALE )
		{
			scaleChanged = true;
			m_psVidRegs->UVSCALE = newval;
		}

		newval = yscaleInt << 16 | yscaleIntUV;
		if ( newval != m_psVidRegs->UVSCALEV )
		{
			scaleChanged = true;
			m_psVidRegs->UVSCALEV = newval;
		}

		/* Recalculate coefficients if the scaling changed. */

		/*
		 * Only Horizontal coefficients so far.
		 */
		if ( scaleChanged )
		{
			double fCutoffY;
			double fCutoffUV;

			fCutoffY = xscaleFract / 4096.0;
			fCutoffUV = xscaleFractUV / 4096.0;

			/* Limit to between 1.0 and 3.0. */
			if ( fCutoffY < MIN_CUTOFF_FREQ )
				fCutoffY = MIN_CUTOFF_FREQ;
			if ( fCutoffY > MAX_CUTOFF_FREQ )
				fCutoffY = MAX_CUTOFF_FREQ;
			if ( fCutoffUV < MIN_CUTOFF_FREQ )
				fCutoffUV = MIN_CUTOFF_FREQ;
			if ( fCutoffUV > MAX_CUTOFF_FREQ )
				fCutoffUV = MAX_CUTOFF_FREQ;

			UpdateCoeff( N_HORIZ_Y_TAPS, fCutoffY, true, true, xcoeffY );
			UpdateCoeff( N_HORIZ_UV_TAPS, fCutoffUV, true, false, xcoeffUV );

			for ( i = 0; i < N_PHASES; i++ )
			{
				for ( j = 0; j < N_HORIZ_Y_TAPS; j++ )
				{
					pos = i * N_HORIZ_Y_TAPS + j;
					m_psVidRegs->Y_HCOEFS[pos] = xcoeffY[pos].sign << 15 | xcoeffY[pos].exponent << 12 | xcoeffY[pos].mantissa;
				}
			}
			for ( i = 0; i < N_PHASES; i++ )
			{
				for ( j = 0; j < N_HORIZ_UV_TAPS; j++ )
				{
					pos = i * N_HORIZ_UV_TAPS + j;
					m_psVidRegs->UV_HCOEFS[pos] = xcoeffUV[pos].sign << 15 | xcoeffUV[pos].exponent << 12 | xcoeffUV[pos].mantissa;
				}
			}
		}
	}
	
	if( eFormat == os::CS_YUV12 ) {
		m_psVidRegs->OSTRIDE = ( pitch ) | ( ( ( pitch / 2 ) ) << 16 );
		m_psVidRegs->OCMD &= ~SOURCE_FORMAT;
		m_psVidRegs->OCMD &= ~OV_BYTE_ORDER;
		m_psVidRegs->OCMD |= YUV_420;
	} else {
		m_psVidRegs->OSTRIDE = pitch;
		m_psVidRegs->OCMD &= ~SOURCE_FORMAT;
		m_psVidRegs->OCMD |= YUV_422;
		m_psVidRegs->OCMD &= ~OV_BYTE_ORDER;
		m_psVidRegs->OCMD |= Y_SWAP;
	}
	m_psVidRegs->OCMD &= ~( BUFFER_SELECT | FIELD_SELECT );
	m_psVidRegs->OCMD |= BUFFER0;

	UpdateVideo();

	m_bVideoOverlayUsed = true;
	m_cVideoSize = cSize;
	m_nVideoOffset = offset;
	
	m_cGELock.Unlock();

	return ( true );
}

void i855::SetupVideoOneLine()
{
	m_bVideoOneLine = false;
	m_nVideoScale = 0x10000;

	if ( m_nDisplayPipe == 1 && ( m_nDisplayInfo & ( PIPE_LFP << 8 ) ) )
	{
		int nSize = INREG( PIPEBSRC );
		int nHeight = nSize & 0x7ff;
		int nWidth = ( nSize >> 16 ) & 0x7ff;
		int nActive = INREG( VTOTAL_B ) & 0x7ff;
		
		//dbprintf( "Panel %i %i\n", nWidth, nHeight );

		if ( nHeight < nActive && nWidth > 1024 )
		{
			int nVertScale;
			uint32 nPanelFit = INREG( PFIT_CONTROLS );

			if ( nPanelFit & PFIT_ON_MASK )
			{
				if ( nPanelFit & PFIT_AUTOVSCALE_MASK )
				{
					nVertScale = INREG( PFIT_AUTOSCALE_RATIO ) >> 16;
				}
				else
				{
					nVertScale = INREG( PFIT_PROGRAMMED_SCALE_RATIO ) >> 16;
				}

				if ( nVertScale != 0 )
				{
					m_bVideoOneLine = true;
					m_nVideoScale = (int)(( ( double )0x10000 / ( double )nVertScale ) * 0x10000);
				}
			}
		}

		if ( m_nVideoScale & 0xFFFE0000 )
		{
			m_nVideoScale = ( int )( ( ( float )nActive * 65536 ) / ( float )nHeight );
		}
	}
}

bool i855::CreateVideoOverlay( const os::IPoint & cSize, const os::IRect & cDst, os::color_space eFormat, os::Color32_s sColorKey, area_id *pBuffer )
{
	if ( ( eFormat == os::CS_YUV422/* || eFormat == os::CS_YUV12*/ ) && !m_bVideoOverlayUsed)
	{
		/* Start video */
		switch ( os::BitsPerPixel( GetCurrentScreenMode().m_eColorSpace ) )
		{
		case 16:
			m_nVideoColorKey = os::COL_TO_RGB16( sColorKey );
			break;
		case 32:
			m_nVideoColorKey = os::COL_TO_RGB32( sColorKey );
			break;

		default:
			return ( false );
		}
		ResetVideo();
		SetupVideoOneLine();


		return ( SetupVideo( cSize, cDst, eFormat, pBuffer ) );
	}
	return ( false );
}


//-----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//-----------------------------------------------------------------------------

bool i855::RecreateVideoOverlay( const os::IPoint & cSize, const os::IRect & cDst, os::color_space eFormat, area_id *pBuffer )
{

	if ( eFormat == os::CS_YUV422/* || eFormat == os::CS_YUV12*/ )
	{
		delete_area( *pBuffer );
		FreeMemory( m_nVideoOffset - m_nFrameBufferOffset );
		return ( SetupVideo( cSize, cDst, eFormat, pBuffer ) );
	}
	return ( false );
}


//-----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//-----------------------------------------------------------------------------

void i855::DeleteVideoOverlay( area_id *pBuffer )
{
	if ( m_bVideoOverlayUsed )
	{
		m_cGELock.Lock();
		m_psVidRegs->OCMD &= ~OVERLAY_ENABLE;
		UpdateVideo();
		VideoOff();
		WaitForIdle();
		m_cGELock.Unlock();

		delete_area( *pBuffer );
		FreeMemory( m_nVideoOffset - m_nFrameBufferOffset );
	}
	m_bVideoOverlayUsed = false;
}
