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

#ifndef __MGA_TVP3026_H_
#define __MGA_TVP3026_H_

#include "mga_dac.h"
#include <atheos/types.h>

#define TVP_MIN_N				40
#define TVP_MIN_M				1
#define TVP_MIN_P				0
#define TVP_MAX_N				62
#define TVP_MAX_M				62
#define TVP_MAX_P				3

#define TVP_MIN_VCO 			110000
#define TVP_MAX_VCO 			220000

#define TVP_MIN_PLL 			14000
#define TVP_MAX_PLL 			TVP_MAX_VCO

#define TI_MAX_PCLK_FREQ		TVP_MAX_PLL
#define TI_MAX_MCLK_FREQ		100000

#define TVP_REF_FREQ	14318.18

class DacTVP3026 : public MgaDac
{
	public:
		DacTVP3026( vuint8 *pRegisterBase );
		~DacTVP3026();

		bool SetPixPLL( long vTargetFrequency, int nDepth );
		MGADACType GetType( void ) { return( TVP3026 ); };

	private:
		bool CalcPixPLL( long vTarget, long vMax, short *nN, short *nM, short *nP );
		bool CalcLoopPLL( double vPll, int nDepth, short *nN, short *nM, short *nP, short *nQ );
		bool SetMemPLL( long vTarget );

		uint8 inTi3026( uint8 nReg );
		void outTi3026( uint8 nReg, uint8 nMask, uint8 nVal);

		inline void outb( uint32 nAddress, uint8 nValue ) const { *((vuint8*)(m_pRegisterBase + nAddress)) = nValue; }
		inline uint8 inb( uint32 nAddress ) const { return( *((vuint8*)(m_pRegisterBase + nAddress)) ); }

		vuint8 *m_pRegisterBase;
		unsigned short m_nDacClk[7];	// 0 - 6 are Clock registers
											// 7 is the Q post divider clock register
};

#endif	// __MGA_TVP3026_H_



