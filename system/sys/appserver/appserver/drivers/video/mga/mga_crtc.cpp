/*
 *  The Syllable application server
 *  Copyright (C) 1999 - 2001  Kurt Skauen
 *  Copyright (C) 2003  Kristian Van Der Vliet
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

#include "mga_crtc.h"
#include "mga_regs.h"

MgaCRTC::MgaCRTC( vuint8 *pRegisterBase )
{
	m_pRegisterBase = pRegisterBase;
}

MgaCRTC::~MgaCRTC()
{

}

bool MgaCRTC::SetMode( int nXRes, int nYRes, int nBPP, float vRefreshRate )
{
	Gx00CRTCRegs sRegs;
	bool bRet;

	bRet = CalcMode( &sRegs, nXRes, nYRes, nBPP, vRefreshRate );

	if( bRet )
	{
		WaitVBlank();
		DisplayBlank();
		SetMGAMode();
		CRTCWriteEnable();

		//	Set up the registers calculated by CalcMode()
		//
		// FIXME: Change these to defined constants

		outb( MGA_MISC, 0xef );
		GCTLWrite( 0x06, 0x05 );
		CRTCWrite( 0x00, sRegs.CRTC0 );
		CRTCWrite( 0x01, sRegs.CRTC1 );
		CRTCWrite( 0x02, sRegs.CRTC2 );
		CRTCWrite( 0x03, sRegs.CRTC3 );
		CRTCWrite( 0x04, sRegs.CRTC4 );
		CRTCWrite( 0x05, sRegs.CRTC5 );
		CRTCWrite( 0x06, sRegs.CRTC6 );
		CRTCWrite( 0x07, sRegs.CRTC7 );
		CRTCWrite( 0x08, sRegs.CRTC8 );
		CRTCWrite( 0x09, sRegs.CRTC9 );
		CRTCWrite( 0x0C, sRegs.CRTCC );
		CRTCWrite( 0x0D, sRegs.CRTCD );
		CRTCWrite( 0x10, sRegs.CRTC10 );
		CRTCWrite( 0x11, sRegs.CRTC11 );
		CRTCWrite( 0x12, sRegs.CRTC12 );
		CRTCWrite( 0x13, sRegs.CRTC13 );
		CRTCWrite( 0x14, sRegs.CRTC14 );
		CRTCWrite( 0x15, sRegs.CRTC15 );
		CRTCWrite( 0x16, sRegs.CRTC16 );
		CRTCWrite( 0x17, sRegs.CRTC17 );
		CRTCWrite( 0x18, sRegs.CRTC18 );
		CRTCEXTWrite( 0x00, sRegs.CRTCEXT0 );
		CRTCEXTWrite( 0x01, sRegs.CRTCEXT1 );
		CRTCEXTWrite( 0x02, sRegs.CRTCEXT2 );
		CRTCEXTWrite( 0x03, sRegs.CRTCEXT3 );
		CRTCEXTWrite( 0x04, 0 );
		SEQWrite( 0x01, sRegs.SEQ1 );
		SEQWrite( 0x00, 0x03 );
		SEQWrite( 0x02, 0x0F );
		SEQWrite( 0x03, 0x00 );
		SEQWrite( 0x04, 0x0E );
		SEQWrite( 0x04, 0x0E );
		XWrite( 0x19, sRegs.XMULCTRL );
		XWrite( 0x1E, sRegs.XMISCCTRL );
		XWrite( 0x38, sRegs.XZOOMCTRL );
		XWrite( 0x4C, sRegs.XPIXPLLM );
		XWrite( 0x4D, sRegs.XPIXPLLN );
		XWrite( 0x4E, sRegs.XPIXPLLP );

		//	Set up the VGA LUT. We could implement a gamma for
		//	15, 16, 24 and 32 bit if we wanted because they use the LUT

		switch( nBPP )
		{
			case 4:	//	not implemented
			case 8:	//	nothing to do
				break;
				
			case 15:
			{
				outb( MGA_PALWTADD, 0 );
				for( int i=0; i<256; i+=1 )
				{
					outb( MGA_PALDATA, i<<3 );
					outb( MGA_PALDATA, i<<3 );
					outb( MGA_PALDATA, i<<3 );
				}
				break;
			}

			case 16:
			{
				outb( MGA_PALWTADD, 0 );
				for( int i=0; i<256; i+=1 )
				{
					outb( MGA_PALDATA, i<<3 );
					outb( MGA_PALDATA, i<<2 );
					outb( MGA_PALDATA, i<<3 );
				}
				break;
			}

			case 24:
			case 32:
			{
				outb( MGA_PALWTADD, 0 );
				for( int i=0; i<256; i+=1 )
				{
					outb( MGA_PALDATA, i );
					outb( MGA_PALDATA, i );
					outb( MGA_PALDATA, i );
				}
				break;
			}
		}

		WaitVBlank();
		DisplayUnBlank();
	}

	return( bRet );
}

bool MgaCRTC::CalcMode( Gx00CRTCRegs *pDest, int nX, int nY, int nBPP, float vRefreshRate )
{
	if( nX == 0 || nY == 0 )
	{
		dbprintf("nX = %i, nY = %i\n", nX, nY );
		return( false );
	}

	//	Make this a possible resolution if x or y is below limits
	//	by using the hardware zoom factors.
	//	These limits are completely arbitrary, the doc is vague

	int nXRes=short(nX);
	int	nXShift=0;
	while( nXRes < 512 )
	{
		nXShift++;
		nX<<=1;
	}

	int	nYRes=short(nY);
	int	nYShift=0;
	while( nYRes < 350 )
	{
		nYShift+=1;
		nYRes<<=1;
	}

	if( nXShift > 2 || nYShift > 2 )
	{
		dbprintf("nXShift = %i, nYShift = %i\n", nXShift, nYShift );
		return( false );
	}

	if( nXShift == 2 )
		nXShift=3;

	if( nYShift==2 )
		nYShift=3;

	if( nXRes > 2048 || nYRes > 1536 )
	{
		dbprintf("nXRes = %i, nYRes = %i\n", nXRes, nYRes );
		return( false );
	}

	//
	//	The usual h and v red tape, blah blah
	//
	// FIXME: This clearly needs a bit more of an explanation!

	int	nHtotal=( nXRes >> 3 ) + 20;
	if( nXRes >= 1024 )
		nHtotal += nHtotal >> 3;

	if( nXRes >= 1280 )
		nHtotal += nHtotal >> 4;

	nHtotal -= 5;

	int nHdispend = ( nXRes >> 3 ) - 1;
	int nHblkstr = nHdispend;
	int nHblkend = nHtotal + 5 - 1;
	int nHsyncstr = ( nXRes >> 3 ) + 1;
	int nHsyncend = ( nHsyncstr + 12 ) & 0x1f;

	int nVtotal = ( nYRes ) + 45 - 2;
	int nVdispend = nYRes - 1;
	int nVblkstr = nYRes - 1;
	int nVblkend = ( nVblkstr - 1 + 46 ) & 0xff;
	int nVsyncstr = nYRes + 9;
	int nVsyncend = ( nVsyncstr +2 ) & 0x0f;
	int nLinecomp = 1023;

	//
	//	Set up bit depth dependant values
	//

	int nScale = 0;
	int nDepth = 0;
	int nStartaddfactor = 0;
	int nVga8bit = 0;

	switch( nBPP )
	{
		case 8:
			nScale = 0;
			nDepth = 0;
			nStartaddfactor = 3;
			nVga8bit = 0;
			break;

		case 15:
			nScale = 1; //doc says 0
			nDepth = 1;
			nStartaddfactor = 3;
			nVga8bit = 8;
			break;

		case 16:
			nScale = 1;	//doc says 0
			nDepth = 2;
			nStartaddfactor = 3;
			nVga8bit = 8;
			break;

		case 24:
			nScale = 2;
			nDepth = 3;
			nStartaddfactor = 3;
			nVga8bit = 8;
			break;

		case 32:
			nScale = 3;	//doc says 1
			nDepth = 7;
			nStartaddfactor = 3;
			nVga8bit = 8;
			break;
	}

	//
	//	Convert the desired refreshrate to a pixelclock.  Incidentally
	//	this is also a VESA pixelclock, suitable for GTF calculations
	//

	long vNormalPixelClock = ( ( nHtotal + 5 ) << 3 ) * ( nVtotal + 2 ) * (long)vRefreshRate;
	dbprintf( "Desired pixelclock is %ld\n", vNormalPixelClock );

	short nBestN = 0, nBestM = 0, nBestP = 0;
	CalcPixPLL( vNormalPixelClock, &nBestN, &nBestM, &nBestP );

	//
	//	Now all we have to do is scatter all the bits out to the registers
	//

	short nBytedepth = short( ( nBPP + 1 ) >> 3 );
	short nOffset = short( ( nXRes * nBytedepth + 15 ) >> 4 );

	pDest->CRTC0 = uint8( nHtotal );
	pDest->CRTC1 = uint8( nHdispend );
	pDest->CRTC2 = uint8( nHblkstr );
	pDest->CRTC3 = uint8( nHblkend & 0x1f );
	pDest->CRTC4 = uint8( nHsyncstr );
	pDest->CRTC5 = uint8( ( nHsyncend & 0x1f ) | ( ( nHblkend & 0x20 ) << 2 ) );
	pDest->CRTC6 = uint8( nVtotal & 0xff );
	pDest->CRTC7 = uint8( ( ( nVtotal & 0x100 ) >> 8 ) | ( ( nVtotal & 0x200 ) >> 4 ) | ( ( nVdispend & 0x100 ) >> 7 ) | ( ( nVdispend & 0x200 ) >> 3 ) | ( ( nVblkstr & 0x100 ) >> 5 ) | ( ( nVsyncstr & 0x100 ) >> 6 ) | ( ( nVsyncstr & 0x200 ) >> 2 ) | ( ( nLinecomp & 0x100 ) >> 4 ) );
	pDest->CRTC8 = 0;
	pDest->CRTC9 = uint8( ( ( nVblkstr & 0x200 ) >> 4 ) | ( ( nLinecomp & 0x200 ) >> 3 ) | ( nYShift & 0x1f ) );
	pDest->CRTCC = 0;
	pDest->CRTCD = 0;
	pDest->CRTC10 = uint8( nVsyncstr );
	pDest->CRTC11 = uint8( ( nVsyncend & 0x0f ) | 0x20 );
	pDest->CRTC12 = uint8( nVdispend );
	pDest->CRTC13 = uint8( nOffset );
	pDest->CRTC14 = 0;
	pDest->CRTC15 = uint8( nVblkstr );
	pDest->CRTC16 = uint8( nVblkend );
	pDest->CRTC17 = 0xc3;
	pDest->CRTC18 = uint8( nLinecomp );

	pDest->CRTCEXT0 = uint8( ( nOffset & 0x300 ) >> 4 );
	pDest->CRTCEXT1 = uint8( ( ( nHtotal & 0x100 ) >> 8 ) | ( ( nHblkstr & 0x100 ) >> 7 ) | ( ( nHsyncstr & 0x100 ) >> 6 ) | ( nHblkend & 0x40 ) );
	pDest->CRTCEXT2 = uint8( ( ( nVtotal & 0xc00 ) >> 10 ) | ( ( nVdispend & 0x400 ) >> 8 ) | ( ( nVblkstr & 0xc00 ) >> 7 ) | ( ( nVsyncstr & 0xc00 ) >> 5 ) | ( ( nLinecomp & 0x400 ) >> 3 ) );
	pDest->CRTCEXT3 = uint8( 0x80 | ( nScale & 7 ) );

	pDest->SEQ1 = 0x21;

	pDest->XMULCTRL = uint8( nDepth & 7 );
	pDest->XMISCCTRL = uint8( nVga8bit | 0x17 );
	pDest->XZOOMCTRL = uint8( nXShift & 3 );
	pDest->XPIXPLLM = uint8( nBestM );
	pDest->XPIXPLLN = uint8( nBestN );
	pDest->XPIXPLLP = uint8( nBestP );

	return( true );
}

bool MgaCRTC::CalcPixPLL( long vTargetPixelClock, short *nN, short *nM, short *nP )
{
	//	Determine values for the clock circuitry that will produce
	//	a pixelclock close enough to what we want

	long nBestPixelClock=0;
	short nBestN = 0, nBestM = 0, nBestP = 0, nBestS = 0;
	short nPixN, nPixM, nPixP;
	short i;

	for( nPixM = 1; nPixM <= 31; nPixM++ )
	{
		for( i = 0; i <= 3; i++ )
		{
			nPixP = short( ( 1 << i ) - 1 );

			long n;
			n = ( nPixP + 1 ) * ( nPixM + 1 );
			n = muldiv64( n, vTargetPixelClock, 27000000 );	// KV: 27000000 is the max pixel
																	// clock of the G200 series, but check on this.
			nPixN = short( n - 1 );

			if( nPixN >= 7 && nPixN <= 127 )
			{
				long nVCO;

				nVCO = muldiv64( 27000000, nPixN + 1, nPixM + 1 );	// KV: See note above about max clock
				n = muldiv64( 1, nVCO, ( nPixP + 1 ) );

				long best, d;

				best = nBestPixelClock - vTargetPixelClock;
				d = n - vTargetPixelClock;

				if( best < 0 )
					best = -best;

				if( d < 0 )
					d = -d;

				if( d < best )
				{
					if( 50000000 <= nVCO && nVCO < 110000000 )
						nBestS = 0;
					else if( 110000000 <= nVCO && nVCO < 170000000 )
						nBestS = 1;
					else if( 170000000 <= nVCO && nVCO < 240000000 )
						nBestS = 2;
					else if( 240000000 <= nVCO && nVCO < 310000000 )
						nBestS = 3;
					else
						continue;

					nBestPixelClock = n;
					*nN = nBestN = nPixN;
					*nM = nBestM = nPixM;
					*nP = nBestP = nPixP;
				}
			}
		}
	}

	dbprintf( "N,M,P,S=%d,%d,%d,%d\n", nBestN, nBestM, nBestP, nBestS );
	dbprintf( "Clock=%ld\n", nBestPixelClock );

	return( true );
}

void MgaCRTC::SetMGAMode()
{
	outb( MGA_CRTCEXTI, 3 );
	outb( MGA_CRTCEXTD, inb( MGA_CRTCEXTD ) | 0x80 );
}

void MgaCRTC::CRTCWriteEnable()
{
	outb( MGA_CRTCI, 0x11 );
	outb( MGA_CRTCD, inb( MGA_CRTCD ) & 0x7F );
}

void MgaCRTC::WaitVBlank()
{
	while( ( inb( MGA_INSTS1 ) & 0x08 ) == 0 ) {}
}

void MgaCRTC::DisplayUnBlank()
{
	outb( MGA_SEQI, 0x01 );
	outb( MGA_SEQD, inb( MGA_SEQD ) & ~0x20 );
}

void MgaCRTC::DisplayBlank()
{
	outb( MGA_SEQI, 0x01 );
	outb( MGA_SEQD, inb( MGA_SEQD ) | 0x20 );
}

void MgaCRTC::GCTLWrite( int nReg, uint8 nVal )
{
	outb( MGA_GCTLI, nReg );
	outb( MGA_GCTLD, nVal );
}

void MgaCRTC::CRTCWrite( int nReg, uint8 nVal )
{
	outb( MGA_CRTCI, nReg );
	outb( MGA_CRTCD, nVal );
}

void MgaCRTC::CRTCEXTWrite( int nReg, uint8 nVal )
{
	outb( MGA_CRTCEXTI, nReg );
	outb( MGA_CRTCEXTD, nVal );
}

void MgaCRTC::SEQWrite( int nReg, uint8 nVal )
{
	outb( MGA_SEQI, nReg );
	outb( MGA_SEQD, nVal );
}

void MgaCRTC::XWrite( int nReg, uint8 nVal )
{
	outb( MGA_PALWTADD, nReg );
	outb( MGA_XDATA, nVal );
}

