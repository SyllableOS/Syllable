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

#include "mga_dac.h"
#include <atheos/types.h>

class DacGx00 : public MgaDac
{
	public:
		DacGx00( vuint8 *pRegisterBase );
		~DacGx00();

		bool SetPixPLL( long vTargetFrequency, int nDepth );
		MGADACType GetType( void ) { return( MGAGx00 ); };

	private:
		bool CalcPixPLL( long vTarget, short *nN, short *nM, short *nP );

		uint8 inGx00( uint8 nReg );
		void outGx00( uint8 nReg, uint8 nMask, uint8 nVal);

		inline void outb( uint32 nAddress, uint8 nValue ) const { *((vuint8*)(m_pRegisterBase + nAddress)) = nValue; }
		inline uint8 inb( uint32 nAddress ) const { return( *((vuint8*)(m_pRegisterBase + nAddress)) ); }

		inline long muldiv64( long a, long b, long c ){ return( ( int64( a ) * int64( b ) ) / int64( c ) ); };

		vuint8 *m_pRegisterBase;
};

