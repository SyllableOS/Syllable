/*
 *  The Syllable application server
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
#include "mga_tvp3026.h"
#include "mga_regs.h"

#include <atheos/kernel.h>	// For dbprintf()
#include <math.h>				// pow()

DacTVP3026::DacTVP3026( vuint8 *pRegisterBase )
{
	m_pRegisterBase = pRegisterBase;
}

DacTVP3026::~DacTVP3026()
{

}

bool DacTVP3026::SetPixPLL( long vTargetFrequency, int nDepth )
{
	short nN, nM, nP;
	short nLm, nLn, nLp, nLq;
	int i;
	short nMiscCtrl = 0, nLatchCtrl = 0, nTcolCtrl = 0, nMulCtrl = 0;
	bool bRet = false;

	bRet = CalcPixPLL( vTargetFrequency/1000, TI_MAX_PCLK_FREQ, &nN, &nM, &nP );
	if( bRet )
	{
		dbprintf("Got N=%i M=%i P=%i for target frequency of %li\n", nN, nM, nP, (long)vTargetFrequency );

		m_nDacClk[ 0 ] = ( nN & 0x3f ) | 0xc0;
		m_nDacClk[ 1 ] = ( nM & 0x3f );
		m_nDacClk[ 2 ] = ( nP & 0x03 ) | 0xb0;

		// Now that the pixel clock PLL is setup,
		// the loop clock PLL must be setup.
		bRet = CalcLoopPLL( (double)vTargetFrequency/1000, nDepth, &nLn, &nLm, &nLp, &nLq );
		if( bRet )
		{
			dbprintf("Got N=%i M=%i P=%i Q=%i for target frequency of %li\n", nLn, nLm, nLp, nLq, (long)vTargetFrequency );

			// Values for the loop clock PLL registers
			if ( nDepth == 24 )
			{
				// Packed pixel mode values
				m_nDacClk[ 3 ] = ( nLn & 0x3f ) | 0x80;
				m_nDacClk[ 4 ] = ( nLm & 0x3f ) | 0x80;
				m_nDacClk[ 5 ] = ( nLp & 0x03 ) | 0xf8;
			}
			else
			{
				// Non-packed pixel mode values
				m_nDacClk[ 3 ] = ( nLn & 0x3f ) | 0xc0;
				m_nDacClk[ 4 ] = ( nLm & 0x3f );
				m_nDacClk[ 5 ] = ( nLp & 0x03 ) | 0xf0;
			}
			m_nDacClk[ 7 ] = nLq | 0x38;	// pReg->DacRegs[ 18 ]
		}
		else
		{
			dbprintf("Unable to get Loop PLL timing values fortarget frequency %li\n", (long int)vTargetFrequency );
			return( false );
		}
	}
	else
	{
		dbprintf("Unable to get Pix PLL timing values for target frequency %li\n", (long int)vTargetFrequency );
		return( false );
	}

	// Now program the Pixel & Loop PLL with the values we have calculated
	//
	// Set loop and pixel clock PLL PLLEN bits to 0
	outTi3026(TVP3026_PLL_ADDR, 0, 0x2A);
	outTi3026(TVP3026_LOAD_CLK_DATA, 0, 0);
	outTi3026(TVP3026_PIX_CLK_DATA, 0, 0);

	// Program pixel clock PLL
	outTi3026(TVP3026_PLL_ADDR, 0, 0x00);
	for (i = 0; i < 3; i++)
		outTi3026(TVP3026_PIX_CLK_DATA, 0, m_nDacClk[i]);

	// Poll until pixel clock PLL LOCK bit is set
	outTi3026(TVP3026_PLL_ADDR, 0, 0x3F);
	while ( ! (inTi3026(TVP3026_PIX_CLK_DATA) & 0x40) );

	dbprintf("Pixel Clock PLL locked\n");

	// Select pixel clock PLL as clock source
	switch( nDepth )
	{
		case 15:
		case 16:
			outTi3026(TVP3026_CLK_SEL, 0, 0x15);	// Dot Clock / 2, select PIX PLL as source
			break;

		case 24:
			outTi3026(TVP3026_CLK_SEL, 0, 0x25);	// Dot Clock / 4, select PIX PLL as source
			break;

		case 32:
			outTi3026(TVP3026_CLK_SEL, 0, 0x05);	// Dot Clock / 0, select PIX PLL as source
			break;
	}

	// Set Q divider for loop clock PLL
	outTi3026(TVP3026_MCLK_CTL, 0, m_nDacClk[7]);

	// Program loop PLL
	outTi3026(TVP3026_PLL_ADDR, 0, 0x00);
	for (i = 3; i < 6; i++)
		outTi3026(TVP3026_LOAD_CLK_DATA, 0, m_nDacClk[i]);

	// Poll until loop PLL LOCK bit is set
	outTi3026(TVP3026_PLL_ADDR, 0, 0x3F);
	while ( ! (inTi3026(TVP3026_LOAD_CLK_DATA) & 0x40) );

	dbprintf("Loop Clock PLL locked\n");

	SetMemPLL( vTargetFrequency/1000 );

	// Set the correct values for the Misc, Latch, True Color Control &
	// Multiplex Control registers
	//
	// Consult Table 2-17 of the TVP3026 manual for the TCOLCTRL & MULCTRL
	// values

	switch( nDepth )
	{
		case 15:
		{
			nMiscCtrl = 0x20;		// Set PSEL to select True Color
			nLatchCtrl = 0x06;	// 1:1. 2:1, 8:1 & 16:1 multiplex modes
			nTcolCtrl = 0x44;		// 16bit X-R-G-B
			nMulCtrl = 0x54;		// 32bit Pixel Bus, interlaced

			break;
		}

		case 16:
		{
			nMiscCtrl = 0x20;		// Set PSEL to select True Color
			nLatchCtrl = 0x06;	// 1:1. 2:1, 8:1 & 16:1 multiplex modes
			nTcolCtrl = 0x45;		// 16bit R-G-B
			nMulCtrl = 0x54;		// 32bit Pixel Bus, interlaced

			break;
		}

		case 24:
		{
			nMiscCtrl = 0x20;		// Set PSEL to select True Color
			nLatchCtrl = 0x07;	// 2:1 multiplex mode or 8:3 Packed 24bit or 4:3 packed 24bit
			nTcolCtrl = 0x56;		// 24bit R-G-B
			nMulCtrl = 0x5c;		// 32bit Pixel Bus, interlaced

			break;
		}

		case 32:
		{
			nMiscCtrl = 0x20;		// Set PSEL to select True Color
			nLatchCtrl = 0x07;	// 2:1 multiplex mode or 8:3 Packed 24bit or 4:3 packed 24bit
			nTcolCtrl = 0x46;		// 32bit X-R-G-B
			nMulCtrl = 0x5c;		// 32bit Pixel Bus, interlaced

			break;
		}
	}

	outTi3026( TVP3026_MISCCTRL, 0, nMiscCtrl );			// Misc. Control
	outTi3026( TVP3026_LATCHCTRL, 0, nLatchCtrl  );		// Latch Control
	outTi3026( TVP3026_TCOLCTRL, 0, nTcolCtrl );			// True Color Control
	outTi3026( TVP3026_MULCTRL, 0, nMulCtrl );			// Multiplex Control

	dbprintf("DacTVP3026::SetPixPLL has completed\n");
	return( bRet );
}

bool DacTVP3026::CalcLoopPLL( double vPll, int nDepth, short *nN, short *nM, short *nP, short *nQ )
{
	int nLm, nLn, nLp, nLq;
	double vZ;

	// First we figure out lm, ln, and z.
	// Things are different in packed pixel mode (24bpp) though.

	if( nDepth == 24 )
	{
		// ln:lm = ln:3
		nLm = 65 - 3;

		// Check for interleaved mode
		if( ( nDepth / 8 ) == 2 )
			nLn = 65 - 4;		// ln:lm = 4:3
		else
			nLn = 65 - 8;		// ln:lm = 8:3

		// Note: this is actually 100 * z for more precision
		vZ = ( 11000 * ( 65 - nLn )) / (( vPll / 1000 ) * ( 65 - nLm ));
	}
	else
	{
		// ln:lm = ln:4
		nLm = 65 - 4;

		// Note: bpp = bytes per pixel
		nLn = 65 - 4 * ( 64 / 8 ) / (nDepth / 8);

		// Note: this is actually 100 * z for more precision
		vZ = (( 11000 / 4 ) * ( 65 - nLn )) / ( vPll / 1000 );
	}

	// Now we choose dividers lp and lq so that the VCO frequency
	// is within the operating range of 110 MHz to 220 MHz.

	// Assume no lq divider
	nLq = 0;

	// Note: z is actually 100 * z for more precision
	if ( vZ <= 200.0 )
		nLp = 0;
	else if ( vZ <= 400.0 )
		nLp = 1;
	else if ( vZ <= 800.0 )
		nLp = 2;
	else if ( vZ <= 1600.0 )
		nLp = 3;
	else {
		nLp = 3;
		nLq = ( int )( vZ / 1600.0 );
	}

	*nN = (short)nLn;
	*nM = (short)nLm;
	*nP = (short)nLp;
	*nQ = (short)nLq;

	return( true );
}

bool DacTVP3026::SetMemPLL( long vTarget )
{
	double f_pll;
	int mclk_m, mclk_n, mclk_p;
	int pclk_m, pclk_n, pclk_p;
	int mclk_ctl;

	f_pll = (double)vTarget;
	CalcPixPLL( vTarget, TI_MAX_MCLK_FREQ, (short*)&mclk_m, (short*)&mclk_n, (short*)&mclk_p );

	/* Save PCLK settings */
	outTi3026( TVP3026_PLL_ADDR, 0, 0xfc );
	pclk_n = inTi3026( TVP3026_PIX_CLK_DATA );
	outTi3026( TVP3026_PLL_ADDR, 0, 0xfd );
	pclk_m = inTi3026( TVP3026_PIX_CLK_DATA );
	outTi3026( TVP3026_PLL_ADDR, 0, 0xfe );
	pclk_p = inTi3026( TVP3026_PIX_CLK_DATA );
	
	/* Stop PCLK (PLLEN = 0, PCLKEN = 0) */
	outTi3026( TVP3026_PLL_ADDR, 0, 0xfe );
	outTi3026( TVP3026_PIX_CLK_DATA, 0, 0x00 );
	
	/* Set PCLK to the new MCLK frequency (PLLEN = 1, PCLKEN = 0 ) */
	outTi3026( TVP3026_PLL_ADDR, 0, 0xfc );
	outTi3026( TVP3026_PIX_CLK_DATA, 0, ( mclk_n & 0x3f ) | 0xc0 );
	outTi3026( TVP3026_PIX_CLK_DATA, 0, mclk_m & 0x3f );
	outTi3026( TVP3026_PIX_CLK_DATA, 0, ( mclk_p & 0x03 ) | 0xb0 );
	
	/* Wait for PCLK PLL to lock on frequency */
	while (( inTi3026( TVP3026_PIX_CLK_DATA ) & 0x40 ) == 0 );
	
	/* Output PCLK on MCLK pin */
	mclk_ctl = inTi3026( TVP3026_MCLK_CTL );
	outTi3026( TVP3026_MCLK_CTL, 0, mclk_ctl & 0xe7 ); 
	outTi3026( TVP3026_MCLK_CTL, 0, ( mclk_ctl & 0xe7 ) | 0x08 );
	
	/* Stop MCLK (PLLEN = 0 ) */
	outTi3026( TVP3026_PLL_ADDR, 0, 0xfb );
	outTi3026( TVP3026_MEM_CLK_DATA, 0, 0x00 );
	
	/* Set MCLK to the new frequency (PLLEN = 1) */
	outTi3026( TVP3026_PLL_ADDR, 0, 0xf3 );
	outTi3026( TVP3026_MEM_CLK_DATA, 0, ( mclk_n & 0x3f ) | 0xc0 );
	outTi3026( TVP3026_MEM_CLK_DATA, 0, mclk_m & 0x3f );
	outTi3026( TVP3026_MEM_CLK_DATA, 0, ( mclk_p & 0x03 ) | 0xb0 );
	
	/* Wait for MCLK PLL to lock on frequency */
	while (( inTi3026( TVP3026_MEM_CLK_DATA ) & 0x40 ) == 0 );

	/* Output MCLK PLL on MCLK pin */
	outTi3026( TVP3026_MCLK_CTL, 0, ( mclk_ctl & 0xe7 ) | 0x10 );
	outTi3026( TVP3026_MCLK_CTL, 0, ( mclk_ctl & 0xe7 ) | 0x18 );
	
	/* Stop PCLK (PLLEN = 0, PCLKEN = 0 ) */
	outTi3026( TVP3026_PLL_ADDR, 0, 0xfe );
	outTi3026( TVP3026_PIX_CLK_DATA, 0, 0x00 );
	
	/* Restore PCLK (PLLEN = ?, PCLKEN = ?) */
	outTi3026( TVP3026_PLL_ADDR, 0, 0xfc );
	outTi3026( TVP3026_PIX_CLK_DATA, 0, pclk_n );
	outTi3026( TVP3026_PIX_CLK_DATA, 0, pclk_m );
	outTi3026( TVP3026_PIX_CLK_DATA, 0, pclk_p );
	
	/* Wait for PCLK PLL to lock on frequency */
	while (( inTi3026( TVP3026_PIX_CLK_DATA ) & 0x40 ) == 0 );

	return( true );
}

bool DacTVP3026::CalcPixPLL( long vTarget, long vMax, short *nN, short *nM, short *nP )
{
	short nBestN = 0, nBestM = 0, nBestP = 0;
	double nPixN, nPixM, nPixP;
	double vPLL = 0, vVCO = 0, vDiffVCO = 0, vBestVCO = (double)vTarget;

	dbprintf("Target is %li\n", vTarget );
	if( vTarget < TVP_MIN_PLL || vTarget > TVP_MAX_PLL )
	{
		dbprintf("PLL target of %li is out of range (Min %i, Max %i)\n", vTarget, TVP_MIN_PLL, TVP_MAX_PLL );
		return( false );
	}

	// Choose P first from the target VCO, where the PLL frequency is VCO / ( 2 ^ P ) and the
	// PLL frequency falls within TVP_MIN_PLL and vMax.
	for( nPixP = TVP_MIN_P; nPixP < TVP_MAX_P; nPixP++ )
	{
		vPLL = vTarget * ( pow( 2, nPixP ) );
		if( vPLL >= TVP_MIN_VCO && vPLL <= vMax )
			break;
	}

	nBestP = (short)nPixP;	// Note that if we tried P = 3 and still did not get a match, we'll
								// default to P = 3

	// Now we try all possible value of N and M and try to get as close as possible to the target VCO
	for( nPixM = TVP_MIN_M; nPixM <= TVP_MAX_M; nPixM++ )
	{
		for( nPixN = TVP_MIN_N; nPixN <= TVP_MAX_N; nPixN++ )
		{
			vVCO = ( 8 * TVP_REF_FREQ ) * ( 65 - nPixM ) / ( 65 - nPixN );

			// Discard any that are outside the min. & max. VCO limits
			if( vVCO < TVP_MIN_VCO || vVCO > vMax )
				continue;

			if( nPixP > 0 )
				vVCO /= pow( 2, nPixP );

			// The difference between the target could be positive or negative; we don't care, we just want
			// the closest to the target.
			// If we get a closer match than the one we already have, then we store the M & N values and
			// carry on checking.
			vDiffVCO = vTarget - vVCO;
			if( ( vDiffVCO > 0 && ( vDiffVCO < vBestVCO ) ) || ( vDiffVCO < 0 ) && ( vDiffVCO > vBestVCO ) )
			{
				vBestVCO = vDiffVCO;
				nBestM = (short)nPixM;
				nBestN = (short)nPixN;

				dbprintf("Got a new best VCO of %li with N=%i M=%i P=%i\n", (long int)vBestVCO, nBestN, nBestM, nBestP );
			}
		}
	}

	// We must have something now, even if it is only the maximum possible mode setting.  Note that the
	// values we have will be valid (E.g. they will produce an output within the allowable PLL ranges) but
	// they may not be as close to the target frequency as we would like for some very odd targets.  There
	// is not much we can do about that.
	*nM = (short)nBestM;
	*nN = (short)nBestN;
	*nP = (short)nBestP;

	return( true );
}

uint8 DacTVP3026::inTi3026( uint8 nReg )
{
	dac_outb(TVP3026_INDEX, nReg);
	return( dac_inb(TVP3026_DATA) );
}

void DacTVP3026::outTi3026( uint8 nReg, uint8 nMask, uint8 nVal)
{
	unsigned char nTmp = (nMask) ? (dac_inb(nReg) & (nMask)) : 0;
	dac_outb(TVP3026_INDEX, nReg);
	dac_outb(TVP3026_DATA, nTmp | (nVal));
}



