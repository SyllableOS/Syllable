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

#include <atheos/types.h>
#include <atheos/kernel.h>	// For dbprintf()

struct	Gx00CRTCRegs
{
	uint8 CRTC0;		//htotal<0:7>
	uint8 CRTC1;		//hdispend<0:7>
	uint8 CRTC2;		//hblkstr<0:7>
	uint8 CRTC3;		//hblkend<0:4>
						//bit 5-6 = hdispskew<0:1>
	uint8 CRTC4;		//hsyncstr<0:7>
	uint8 CRTC5;		//bit 0-4 = hsyncend<0:4>
						//bit 5-6 = hsyncdel<0:1>
    					//bit 7 = hblkend<5>
	uint8 CRTC6;		//vtotal<0:7>
	uint8 CRTC7;		//bit 0 = vtotal<8>
						//bit 1 = vdispend<8>
						//bit 2 = vsyncstr<8>
						//bit 3 = vblkstr<8>
						//bit 4 = linecomp<8>
						//bit 5 = vtotal<9>
						//bit 6 = vdispend<9>
						//bit 7 = vsyncstr<9>
	uint8 CRTC8;		//bit 5-6 = bytepan<0:1>
	uint8 CRTC9;		//bit 0-4 = maxscan
    					//bit 5 = vblkstr<9>
						//bit 6 = linecomp<9>
						//bit 7 = conv2t4
	uint8 CRTCC;		//startadd<8:15>
	uint8 CRTCD;		//startadd<0:7>
	uint8 CRTC10;		//vsyncstr<0:7>
	uint8 CRTC11;		//bit 0-3 = vsyncend<0:3>
						//bit 5 = disable vblank int
	uint8 CRTC12;		//vdispend<0:7>
	uint8 CRTC13;		//offset<8:0>
	uint8 CRTC14;		//bit 6 = dword
	uint8 CRTC15;		//vblkstr<0:7>
	uint8 CRTC16;		//vblkend<0:7>
	uint8 CRTC17;		//bit 0 = cms
    					//bit 1 = selrowscan
		    			//bit 2 = hsyncsel<0>
						//bit 6 = wbmode
						//bit 7 = allow syncs to run
	uint8 CRTC18;		//linecomp<0:7>
	uint8 CRTCEXT0;	//bit 0-3 = startadd<16:20>
						//bit 4-5 = offset<8:9>
	uint8 CRTCEXT1;	//bit 0 = htotal<8>
						//bit 1 = hblkstr<8>
						//bit 2 = hsyncstr<8>
						//bit 6 = hblkend<6>
	uint8 CRTCEXT2;	//bit 0-1 = vtotal<10:11>
						//bit 2 = vdispend<10>
						//bit 3-4 = vblkstr<10:11>
						//bit 5-6 = vsyncstr<10:11>
						//bit 7 = linecomp<10>
	uint8 CRTCEXT3;	//bit 0-2 = scale<0:2>
	uint8 SEQ1;		//bit 3 = dotclkrt
	uint8 XMULCTRL;	//bit 0-2 = depth
	uint8 XMISCCTRL;
	uint8 XZOOMCTRL;	//bit 0-1 = hzoom
	uint8 XPIXPLLM;	//bit 0-6
	uint8 XPIXPLLN;	//bit 0-6
	uint8 XPIXPLLP;	//bit 0-6
};

class	MgaCRTC
{
	public:
		MgaCRTC( vuint8 *pRegisterBase );
		~MgaCRTC();

		bool SetMode( int nXRes, int nYRes, int nBPP, float vRefreshRate );

	private:
		inline void   outb( uint32 nAddress, uint8 nValue ) const { *((vuint8*)(m_pRegisterBase + nAddress)) = nValue; }
		inline uint8  inb( uint32 nAddress ) const { return( *((vuint8*)(m_pRegisterBase + nAddress)) ); }

		inline long muldiv64( long a, long b, long c ){ return( ( int64( a ) * int64( b ) ) / int64( c ) ); };

		bool CalcMode( Gx00CRTCRegs *pDest, int nX, int nY, int nBPP, float vRefreshRate );
		bool CalcPixPLL( long vTargetPixelClock, short *nN, short *nM, short *nP );

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
};

#endif	// __MGA_CRTC_H__

