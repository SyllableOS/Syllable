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

#ifndef __MGA_CRTC_H__
#define __MGA_CRTC_H__

#include "mga_dac.h"

#include <atheos/types.h>
#include <atheos/kernel.h>	// For dbprintf()

struct CRTCModeData
{
	int nHtotal;
	int nHdispend;
	int nHblkstr;
	int nHblkend;
	int nHsyncstr;
	int nHsyncend;

	int nVtotal;
	int nVdispend;
	int nVblkstr;
	int nVblkend;
	int nVsyncstr;
	int nVsyncend;

	short nOffset;
	int nLinecomp;
	int nScale;

	int nXShift;
	int nYShift;
};

class	MgaCRTC
{
	public:
		MgaCRTC( vuint8 *pRegisterBase, MgaDac *pcDac, PCI_Info_s cPCIInfo );
		~MgaCRTC();

		bool SetMode( int nXRes, int nYRes, int nBPP, float vRefreshRate );

	private:
		inline void outb( uint32 nAddress, uint8 nValue ) const { *((vuint8*)(m_pRegisterBase + nAddress)) = nValue; }
		inline uint8 inb( uint32 nAddress ) const { return( *((vuint8*)(m_pRegisterBase + nAddress)) ); }

		bool CalcMode( int nX, int nY, int nBPP, float vRefreshRate, struct CRTCModeData *psModeData );

		void SetMGAMode();
		void CRTCWriteEnable();
		void WaitVBlank();
		void DisplayUnBlank();
		void DisplayBlank();

		void GCTLWrite( int nReg, uint8 nVal );
		void CRTCWrite( int nReg, uint8 nVal );
		void CRTCEXTWrite( int nReg, uint8 nVal );
		void SEQWrite( int nReg, uint8 nVal );
		void XWrite( int nReg, uint8 nVal );

		vuint8 *m_pRegisterBase;
		MgaDac *m_pcDac;
		PCI_Info_s m_cPCIInfo;
};

#endif	// __MGA_CRTC_H__

