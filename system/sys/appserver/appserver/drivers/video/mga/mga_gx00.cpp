/*
 *  The Syllable application server
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

#include "mga.h"
#include "mga_gx00.h"
#include "mga_regs.h"

#include <atheos/kernel.h>

DacGx00::DacGx00( vuint8 *pRegisterBase )
{
	m_pRegisterBase = pRegisterBase;
}

DacGx00::~DacGx00()
{

}

bool DacGx00::SetPixPLL( long vTargetFrequency, int nDepth )
{
	short nBestN = 0, nBestM = 0, nBestP = 0;
	bool bRet;
	int nMode = 0, nVga8bit = 0;

	bRet = CalcPixPLL( vTargetFrequency, &nBestN, &nBestM, &nBestP );
	if( bRet )
	{
		switch( nDepth )
		{
			case 15:
				nMode = 1;
				nVga8bit = 8;
				break;

			case 16:
				nMode = 2;
				nVga8bit = 8;
				break;

			case 24:
				nMode = 3;
				nVga8bit = 8;
				break;

			case 32:
				nMode = 7;
				nVga8bit = 8;
				break;
		}

		outGx00( 0x19, 0, (uint8)( nMode & 7 ) );
		outGx00( 0x1E, 0, (uint8)( nVga8bit | 0x17 ) );
		outGx00( 0x4C, 0, (uint8)nBestM );
		outGx00( 0x4D, 0, (uint8)nBestN );
		outGx00( 0x4E, 0, (uint8)nBestP );
	}

	dbprintf("DacGx00::SetPixPLL completed\n");

	return( bRet );
}


bool DacGx00::CalcPixPLL( long vTargetPixelClock, short *nN, short *nM, short *nP )
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
																	// clock of the Gx00 series, but check on this.
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

uint8 DacGx00::inGx00( uint8 nReg )
{
	dac_outb( MGA_PALWTADD, nReg );
	return( dac_inb( MGA_XDATA ) );
}

void DacGx00::outGx00( uint8 nReg, uint8 nMask, uint8 nVal)
{
	unsigned char nTmp = (nMask) ? (dac_inb(nReg) & (nMask)) : 0;
	dac_outb( MGA_PALWTADD, nReg);
	dac_outb( MGA_XDATA, nTmp | (nVal) );
}

