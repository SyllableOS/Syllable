/*
 *  The Syllable application server
 *  Copyright (C) 1999 - 2001  Kurt Skauen
 *  Copyright (C) 2003  Kristian Van Der Vliet
 *  Copyright 1994 by Robin Cutshaw <robin@XFree86.org>
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

#include "mga.h"
#include "mga_crtc.h"
#include "mga_regs.h"

MgaCRTC::MgaCRTC( vuint8 *pRegisterBase, MgaDac *pcDac, PCI_Info_s cPCIInfo )
{
	m_pRegisterBase = pRegisterBase;
	m_pcDac = pcDac;
	m_cPCIInfo = cPCIInfo;
}

MgaCRTC::~MgaCRTC()
{

}

bool MgaCRTC::SetMode( int nXRes, int nYRes, int nBPP, float vRefreshRate )
{
	bool bRet;
	struct CRTCModeData sModeData;

	bRet = CalcMode( nXRes, nYRes, nBPP, vRefreshRate, &sModeData );

	if( bRet )
	{
		WaitVBlank();
		DisplayBlank();
		SetMGAMode();
		CRTCWriteEnable();

		// FIXME: Change these all to defined constants
		outb( MGA_MISC, 0xef );
		GCTLWrite( 0x06, 0x05 );

		CRTCWrite( 0x00, uint8( sModeData.nHtotal ) );
		CRTCWrite( 0x01, uint8( sModeData.nHdispend ) );
		CRTCWrite( 0x02, uint8( sModeData.nHblkstr ) );
		CRTCWrite( 0x03, uint8( sModeData.nHblkend & 0x1f ) );
		CRTCWrite( 0x04, uint8( sModeData.nHsyncstr ) );
		CRTCWrite( 0x05, uint8( ( sModeData.nHsyncend & 0x1f ) | ( ( sModeData.nHblkend & 0x20 ) << 2 ) ) );
		CRTCWrite( 0x06, uint8( sModeData.nVtotal & 0xff ) );
		CRTCWrite( 0x07, uint8( ( ( sModeData.nVtotal & 0x100 ) >> 8 ) | ( ( sModeData.nVtotal & 0x200 ) >> 4 ) | ( ( sModeData.nVdispend & 0x100 ) >> 7 ) | ( ( sModeData.nVdispend & 0x200 ) >> 3 ) | ( ( sModeData.nVblkstr & 0x100 ) >> 5 ) | ( ( sModeData.nVsyncstr & 0x100 ) >> 6 ) | ( ( sModeData.nVsyncstr & 0x200 ) >> 2 ) | ( ( sModeData.nLinecomp & 0x100 ) >> 4 ) ) );
		CRTCWrite( 0x08, 0 );
		CRTCWrite( 0x09, uint8( ( ( sModeData.nVblkstr & 0x200 ) >> 4 ) | ( ( sModeData.nLinecomp & 0x200 ) >> 3 ) | ( sModeData.nYShift & 0x1f ) ) );
		CRTCWrite( 0x0C, 0 );
		CRTCWrite( 0x0D, 0 );
		CRTCWrite( 0x10, uint8( sModeData.nVsyncstr ) );
		CRTCWrite( 0x11, uint8( ( sModeData.nVsyncend & 0x0f ) | 0x20 ) );
		CRTCWrite( 0x12, uint8( sModeData.nVdispend ) );
		CRTCWrite( 0x13, uint8( sModeData.nOffset ) );
		CRTCWrite( 0x14, 0 );
		CRTCWrite( 0x15, uint8( sModeData.nVblkstr ) );
		CRTCWrite( 0x16, uint8( sModeData.nVblkend ) );
		CRTCWrite( 0x17, 0xc3 );
		CRTCWrite( 0x18, uint8( sModeData.nLinecomp ) );

		CRTCEXTWrite( 0x00, uint8( ( sModeData.nOffset & 0x300 ) >> 4 ) );
		CRTCEXTWrite( 0x01, uint8( ( ( sModeData.nHtotal & 0x100 ) >> 8 ) | ( ( sModeData.nHblkstr & 0x100 ) >> 7 ) | ( ( sModeData.nHsyncstr & 0x100 ) >> 6 ) | ( sModeData.nHblkend & 0x40 ) ) );
		CRTCEXTWrite( 0x02, uint8( ( ( sModeData.nVtotal & 0xc00 ) >> 10 ) | ( ( sModeData.nVdispend & 0x400 ) >> 8 ) | ( ( sModeData.nVblkstr & 0xc00 ) >> 7 ) | ( ( sModeData.nVsyncstr & 0xc00 ) >> 5 ) | ( ( sModeData.nLinecomp & 0x400 ) >> 3 ) ) );
		CRTCEXTWrite( 0x03, uint8( 0x80 | ( sModeData.nScale & 7 ) ) );

		// FIXME: Document these mystery values
		SEQWrite( 0x00, 0x03 );
		SEQWrite( 0x01, 0x21 );
		SEQWrite( 0x02, 0x0F );
		SEQWrite( 0x03, 0x00 );
		SEQWrite( 0x04, 0x0E );
		SEQWrite( 0x04, 0x0E );

		//	Convert the desired refreshrate to a pixelclock & set the PLL's & DAC dependent stuff
		long vNormalPixelClock = ( ( sModeData.nHtotal + 5 ) << 3 ) * ( sModeData.nVtotal + 2 ) * (long)vRefreshRate;
		dbprintf( "Desired pixelclock is %ld\n", vNormalPixelClock );

		m_pcDac->SetPixPLL( vNormalPixelClock, nBPP );

		XWrite( 0x38, uint8( sModeData.nXShift & 3 ) );

		//	Set up the VGA LUT. We could implement a gamma for
		//	15, 16, 24 and 32 bit if we wanted because they use the LUT

		switch( nBPP )
		{
			case 15:
			{
				dac_outb( MGA_PALWTADD, 0 );
				for( int i=0; i<256; i+=1 )
				{
					dac_outb( MGA_PALDATA, i<<3 );
					dac_outb( MGA_PALDATA, i<<3 );
					dac_outb( MGA_PALDATA, i<<3 );
				}
				break;
			}

			case 16:
			{
				dac_outb( MGA_PALWTADD, 0 );
				for( int i=0; i<256; i+=1 )
				{
					dac_outb( MGA_PALDATA, i<<3 );
					dac_outb( MGA_PALDATA, i<<2 );
					dac_outb( MGA_PALDATA, i<<3 );
				}
				break;
			}

			case 24:
			case 32:
			{
				dac_outb( MGA_PALWTADD, 0 );
				for( int i=0; i<256; i+=1 )
				{
					dac_outb( MGA_PALDATA, i );
					dac_outb( MGA_PALDATA, i );
					dac_outb( MGA_PALDATA, i );
				}
				break;
			}
		}

		// Set the PCI OPTION register if required
		if( m_pcDac->GetType() == TVP3026 )
		{
			uint32 nOption = 0x402C1100;	// Standard for 2064 and 2164, + interlaced

			write_pci_config( m_cPCIInfo.nBus, m_cPCIInfo.nDevice, m_cPCIInfo.nFunction, 0x40, 4, nOption );
		}

		WaitVBlank();
		DisplayUnBlank();
	}

	dbprintf("MgaCRTC::SetMode has completed\n");
	return( bRet );
}

bool MgaCRTC::CalcMode( int nX, int nY, int nBPP, float vRefreshRate, struct CRTCModeData *psModeData )
{
	if( nX == 0 || nY == 0 )
	{
		dbprintf("nX = %i, nY = %i\n", nX, nY );
		return( false );
	}

	//	Make this a possible resolution if x or y is below limits
	//	by using the hardware zoom factors.
	//	These limits are completely arbitrary, the doc is vague
	//
	// FIXME: Zooming will now never be used as we only support the lowest
	// resolution of 640x480.  Remove all this code & hardcode the zoom
	// register values

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

	//	Set bit depth dependant values

	int nScale = 0;
	int nStartaddfactor = 0;

	switch( nBPP )
	{
		case 15:
			nScale = 1;	//doc says 0
			nStartaddfactor = 3;
			break;

		case 16:
			nScale = 1;	//doc says 0
			nStartaddfactor = 3;
			break;

		case 24:
			nScale = 2;
			nStartaddfactor = 3;
			break;

		case 32:
			nScale = 3;	//doc says 1
			nStartaddfactor = 3;
			break;
	}

	psModeData->nHtotal = nHtotal;
	psModeData->nHdispend = nHdispend;
	psModeData->nHblkstr = nHblkstr;
	psModeData->nHblkend = nHblkend;
	psModeData->nHsyncstr = nHsyncstr;
	psModeData->nHsyncend = nHsyncend;

	psModeData->nVtotal = nVtotal;
	psModeData->nVdispend = nVdispend;
	psModeData->nVblkstr = nVblkstr;
	psModeData->nVblkend = nVblkend;
	psModeData->nVsyncstr = nVsyncstr;
	psModeData->nVsyncend = nVsyncend;

	psModeData->nOffset = short( ( nXRes * (( nBPP + 1 ) >> 3) + 15 ) >> 4 );
	psModeData->nLinecomp = nLinecomp;
	psModeData->nScale = nScale;

	psModeData->nXShift = nXShift;
	psModeData->nYShift = nYShift;

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

