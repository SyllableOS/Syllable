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

#ifndef __MGA_DAC_H_
#define __MGA_DAC_H_

#include <atheos/types.h>
#include "mga_regs.h"

#define RAMDAC_OFFSET	0x3c00	// Offset from the first MGA register to the
									// first DAC register

#define dac_inb( nReg ) inb( ( RAMDAC_OFFSET + nReg ) )
#define dac_outb( nReg, nVal) outb( ( RAMDAC_OFFSET + nReg ), nVal )

class MgaDac
{
	public:
		MgaDac() { };
		virtual ~MgaDac() { };

		virtual bool SetPixPLL( long vTargetFrequency, int nDepth ) { return( false); };
		virtual MGADACType GetType( void ) { return( NONE ); };
};

#endif	// __MGA_DAC_H_

