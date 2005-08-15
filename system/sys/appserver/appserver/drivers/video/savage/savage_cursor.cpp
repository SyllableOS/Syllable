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

#include <savage_driver.h>
#include <savage_regs.h>

/* As nice as HW cursors would be, the Savage only supports a mono cursor which uses an interleaved
   source & mask format.  This is totally unlike most other cursor formats we have to deal with in Syllable
   so converting from a mono bitmap to the bitmask + mask format required by the Savage is a lot of work
   for very little gain.  By default the HW cursor is simply disabled; you can enable it for mono
   cursors by setting m_bEnableHWC true (Top of the constructor) but without fixes to the bitmap-mangling
   code you'll find that most cursors don't display correctly ("standard" I-beam cursor is fine, most others
   are corrupted)
*/

using namespace os;
using namespace std;

void SavageDriver::SetCursorBitmap( mouse_ptr_mode eMode, const IPoint& cHotSpot, const void *pRaster, int nWidth, int nHeight )
{
	savage_s *psCard = m_psCard;

	if( eMode != MPTR_MONO || nWidth > CURS_WIDTH || nHeight > CURS_HEIGHT || !m_bEnableHWC )
	{
		if( m_bHWCursor == true )
		{
		    /* Turn HW cursor off. */
			if( S3_SAVAGE4_SERIES( psCard->eChip ) )
		       waitHSync(5);
			outCRReg( 0x45, inCRReg(0x45) & 0xfe );
		}
		m_bHWCursor = false;
		VesaDriver::SetCursorBitmap( eMode, cHotSpot, pRaster, nWidth, nHeight );

		return;
	}
	m_bHWCursor = true;
	dbprintf( "Using HW cursor\n");

	/* Turn SW & HW cursor off */
	DisplayDriver::MouseOff();

	if( S3_SAVAGE4_SERIES( psCard->eChip ) )
       waitHSync(5);
	outCRReg( 0x45, inCRReg(0x45) & 0xfe );

	/* Set cursor location in frame buffer.  */
	outCRReg( 0x4d, (0xff & (uint32)psCard->CursorKByte));
	outCRReg( 0x4c, (0xff00 & (uint32)psCard->CursorKByte) >> 8);

	/* Upload the cursor image to the frame buffer.
	 *
	 * The cursor is 64x64.  The image is a bitmap; 0 selects the background, 1 select the foreground.
	 * The background & foreground colours are 24bit values set in the background & foreground registers.
	 * 
	 * Each row is 16 bytes wide.  The mask is interleved into the pixel data; two bytes of mask data (16 pixels) followed
	 * by two bytes of pixel data.
	 *
	 * 0 (mask) pixel & 0 (bg) mask pixel == bg
	 * 0 (mask) pixel & 1 (fg) mask pixel == fg
	 * 1 (mask) pixel & 0 (bg) pixel == transparent
	 * 1 (mask) pixel & 1 (fg) pixel == inverse
	 *
	 * If you start with 0xff ff 00 00 (All pixels transparent):
	 *
	 * o Black - flip the mask bit to 0
	 * o White - flip the mask bit to 0 & source bit to 1
	 *
	 */

	uint8 anSaved[1024];
	uint16 *pnSaved = (uint16*)anSaved;

	/* Set all pixels transparent to start */
	int x,y;

	for( y = 0; y < CURS_HEIGHT; y++ )
		for( x = 0; x < CURS_WIDTH / 16; x++ )
		{
			*pnSaved++ = 0xffff;
			*pnSaved++ = 0x0000;	/* All transparent */
		}

	/* The Syllable mono-cursor format uses one of four values per pixel.  The values represent:
	   transparent, black (bg), black (bg), white (fg) */

	const uint8 *pnSrc = (const uint8*)pRaster;
	pnSaved = (uint16*)anSaved;
	for( y = 0; y < nHeight; y++ )
	{
		uint16 nMask;
		uint16 nSource;

		for( x = 0; x < CURS_WIDTH / 16; x++ )
		{
			/* Start with 16 transparent pixels */
			nMask = 0xffff;
			nSource = 0x0000;

			/* Process 16 pixels at a time */
			for( int g = 0; g < 16; g++ )
			{
				if( (x*16) + g < nWidth )
				{
					uint8 nPix = *pnSrc++;

					if( nPix == 1 || nPix == 2 )		/* Black (background) */
						nMask &= ~(1 << (15 - g));		/* Flip mask bit to 0 */
					else if( nPix == 3 )				/* White (foreground) */
					{
						nMask &= ~(1 << (15 - g));		/* Flip mask bit to 0 */
						nSource |= (1 << (15 - g));		/* & source bit to 1 */
					}
				}
			}

			/* Store that group of 16 pixels */
			*pnSaved++ = nMask;
			*pnSaved++ = nSource;
		}
	}

	/* Copy cursor data to the cursor framebuffer location */
	memcpy((void*)(psCard->FBBase + psCard->CursorKByte * 1024), (void*)anSaved, 1024);

	/* Set the "foreground" to white & "background" to black; note the 24bit values */

	/* Reset the cursor color stack pointer */
	inCRReg(0x45);
	/* Write low, mid, high bytes - foreground */
	outCRReg(0x4a, 0xff);
	outCRReg(0x4a, 0xff);
	outCRReg(0x4a, 0xff);
	/* Reset the cursor color stack pointer */
	inCRReg(0x45);
	/* Write low, mid, high bytes - background */
	outCRReg(0x4b, 0x00);
	outCRReg(0x4b, 0x00);
	outCRReg(0x4b, 0x00);

	if( S3_SAVAGE4_SERIES( psCard->eChip ) )
	{
		/*
		 * Bug in Savage4 Rev B requires us to do an MMIO read after
		 * loading the cursor.
		 */
		volatile unsigned int i = ALT_STATUS_WORD0;
		(void)i++;	/* Not to be optimised out */
	}
    /* Turn HW cursor on. */
	outCRReg( 0x45, inCRReg(0x45) | 0x01 );
}

void SavageDriver::MouseOn()
{
	savage_s *psCard = m_psCard;

	if( m_bHWCursor == false )
		return DisplayDriver::MouseOn();

    /* Turn cursor on. */
	outCRReg( 0x45, inCRReg(0x45) | 0x01 );
}

void SavageDriver::MouseOff()
{
	savage_s *psCard = m_psCard;

	if( m_bHWCursor == false )
		return DisplayDriver::MouseOff();

    /* Turn cursor off. */
	if( S3_SAVAGE4_SERIES( psCard->eChip ) )
       waitHSync(5);

	outCRReg( 0x45, inCRReg(0x45) & 0xfe );
}

void SavageDriver::SetMousePos( IPoint cNewPos )
{
	savage_s *psCard = m_psCard;
	unsigned char xoff, yoff, byte;
	int x = cNewPos.x;
	int y = cNewPos.y;

	if( m_bHWCursor == false )
		return DisplayDriver::SetMousePos( cNewPos );

	if( S3_SAVAGE4_SERIES( psCard->eChip ) )
		waitHSync(5);

	/*
	 * Make these even when used.  There is a bug/feature on at least
	 * some chipsets that causes a "shadow" of the cursor in interlaced
	 * mode.  Making this even seems to have no visible effect, so just
	 * do it for the generic case.
	 */

	if (x < 0)
	{
		xoff = ((-x) & 0xFE);
		x = 0;
	}
	else
		xoff = 0;

	if (y < 0)
	{
		yoff = ((-y) & 0xFE);
		y = 0;
	}
	else
		yoff = 0;

	/* This is the recomended order to move the cursor */
	outCRReg( 0x46, (x & 0xff00)>>8 );
	outCRReg( 0x47, (x & 0xff) );
	outCRReg( 0x49, (y & 0xff) );
	outCRReg( 0x4e, xoff );
	outCRReg( 0x4f, yoff );
	outCRReg( 0x48, (y & 0xff00)>>8 );

	/* fix for HW cursor on crtc2 */
	byte = inCRReg( 0x46 );
	outCRReg( 0x46, byte );
}

