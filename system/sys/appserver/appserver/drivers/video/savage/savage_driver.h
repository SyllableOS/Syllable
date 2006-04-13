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

#ifndef SYLLABLE_SAVAGE_DRIVER_H__
#define SYLLABLE_SAVAGE_DRIVER_H__

#include <savage.h>
#include <savage_regs.h>

#include <atheos/types.h>
#include <atheos/pci.h>
#include <gui/guidefines.h>

#include <vector>

#include "../../../server/vesadrv.h"

enum wait_type
{
	S3_WAIT_3D,
	S3_WAIT_4,
	S3_WAIT_2K,
	S3_WAIT_SHADOW
};

/* Bitmap descriptor structures for BCI */
typedef struct _HIGH {
    ushort Stride;
    uchar Bpp;
    uchar ResBWTile;
} HIGH;

typedef struct _BMPDESC1 {
    ulong Offset;
    HIGH  HighPart;
} BMPDESC1;

typedef struct _BMPDESC2 {
    ulong LoPart;
    ulong HiPart;
} BMPDESC2;

typedef union _BMPDESC {
    BMPDESC1 bd1;
    BMPDESC2 bd2;
} BMPDESC;

typedef struct savage
{
	PCI_Info_s sPCI;
	savage_type eChip;

	/* These are (virtual) linear addresses. */
	vuint8 *MapBase;		/* m_pRegisterBase */
	vuint8 *BciMem;			/* Start of BCI command buffer */
	vuint8 *VgaMem;			/* Location of standard VGA regs */
	vuint8 *MapBaseDense;
	vuint8 *FBBase;			/* m_pFramebufferBase */
	vuint8 *ApertureMap;
	vuint8 *FBStart;
	vuint32 *ShadowVirtual;

	uint32 FramebufferSize;

	uint32 nVidTop;
	uint32 nVRAM;

	double minClock, maxClock;

	/* Here are all the Options */
	double LCDClock;
	int PanelX;				/* panel width */
	int PanelY;				/* panel height */

	wait_type eWaitQueue;
	wait_type eWaitIdle;
	wait_type eWaitIdleEmpty;

	/* Command Overflow Buffer (COB) */
	uint32 cobIndex;		/* size index */
	uint32 cobSize;			/* size in bytes */
	uint32 cobOffset;		/* offset in frame buffer */
	uint32 bciThresholdHi;	/* low and high thresholds for */
	uint32 bciThresholdLo;	/* shadow status update (32bit words) */
	uint32 bciUsedMask;		/* BCI entries used mask */
	uint16 eventStatusReg;	/* Status register index that holds event counter 0. */

	int	CursorKByte;
	int	endfb;

    int ShadowCounter;

	/* Overlays */
    int	dwBCIWait2DIdle;
	uint32 lastKnownPitch;
    unsigned int blendBase;

	SavageMonitorType eDisplayType;

	struct screen_detials
	{
		int crtPosH;
    	int crtSizeH;
    	int crtPosV;
    	int crtSizeV;

		int Width;
		int Height;
		int Bpp;
		int primStreamBpp;

		float RefreshRate;

		int lDelta;

		int XFactor;	/* overlay X factor */
		int YFactor;	/* overlay Y factor */
		int displayXoffset;	/* overlay X offset */
		int displayYoffset;	/* overlay Y offset */
		int XExp1;		/* expansion ratio in x */
		int XExp2;
		int YExp1;		/* expansion ratio in x */
		int YExp2;
		int cxScreen;
	} scrn;
	uint32 scrnBytes;

	/* Bitmap Descriptors for BCI */
	BMPDESC GlobalBD;

} savage_s;

class SavageDriver : public VesaDriver
{
	public:
		SavageDriver( int nFd );
		virtual ~SavageDriver();

		bool IsInited(){return( m_bIsInited );};

		virtual area_id Open();
		virtual void Close();

		virtual int GetFramebufferOffset(){return( 0 );}

		virtual int GetScreenModeCount();
		virtual bool GetScreenModeDesc( int nIndex, os::screen_mode *psMode );
		virtual int SetScreenMode( os::screen_mode sMode );

		virtual void LockBitmap( SrvBitmap* pcDstBitmap, SrvBitmap* pcSrcBitmap, os::IRect cSrcRect, os::IRect cDstRect );

		virtual bool DrawLine( SrvBitmap *pcBitmap, const os::IRect &cClipRect, const os::IPoint &cPnt1, const os::IPoint &cPnt2, const os::Color32_s &sColor, int nMode );
		virtual bool FillRect( SrvBitmap *pcBitmap, const os::IRect &cRect, const os::Color32_s &sColor, int nMode );
		virtual bool BltBitmap( SrvBitmap *pcDstBitmap, SrvBitmap *pcSrcBitmap, os::IRect cSrcRect, os::IRect cDstRect, int nMode, int nAlpha );

		virtual bool CreateVideoOverlay( const os::IPoint& cSize, const os::IRect& cDst, os::color_space eFormat, os::Color32_s sColorKey, area_id *phArea );
		virtual bool RecreateVideoOverlay( const os::IPoint& cSize, const os::IRect& cDst, os::color_space eFormat, area_id *phArea );
		virtual void DeleteVideoOverlay( area_id *phArea );

	private:
		void SavageInitStatus( savage_s *psCard );
		void SavageGetPanelInfo( savage_s *psCard );

		void SavageInitialize2DEngine( savage_s *psCard );
		void SavageSetGBD( savage_s *psCard );
		void SavageSetGBD_3D( savage_s *psCard );
		void SavageSetGBD_M7( savage_s *psCard );
		void SavageSetGBD_Twister( savage_s *psCard );
		void SavageSetGBD_PM( savage_s *psCard );
		void SavageSetGBD_2000( savage_s *psCard );
		unsigned int SavageSetBD( SrvBitmap *pcBitmap );

		bool SavageOverlayNew( const os::IPoint& cSize, const os::IRect& cDst, area_id *phArea );
		void SavageSetColorKeyNew( savage_s *psCard );
		bool SavageOverlayOld( const os::IPoint& cSize, const os::IRect& cDst, area_id *phArea );
		void SavageSetColorKeyOld( savage_s *psCard );

		void SavageStreamsOn( savage_s *psCard );
		void SavageStreamsOff( savage_s *psCard );
		void SavageInitStreamsNew( savage_s *psCard );
		void SavageInitStreamsOld( savage_s *psCard );

		/* All of the Savage wait handlers all called indirectly via. these three. */
		int WaitQueue( savage_s *psCard, int v );
		int WaitIdle( savage_s *psCard );
		int WaitIdleEmpty( savage_s *psCard );

		int WaitQueue3D( savage_s *psCard, int v );
		int WaitQueue4( savage_s *psCard, int v );
		int WaitQueue2K( savage_s *psCard, int v );

		int WaitIdleEmpty3D( savage_s *psCard );
		int WaitIdleEmpty4( savage_s *psCard );
		int WaitIdleEmpty2K( savage_s *psCard );

		int WaitIdle3D( savage_s *psCard );
		int WaitIdle4( savage_s *psCard );
		int WaitIdle2K( savage_s *psCard );

		void ResetBCI2K( savage_s *psCard );
		bool ShadowWait( savage_s *psCard );
		bool ShadowWaitQueue( savage_s *psCard, int v );

		os::Locker m_cGELock;
		bool m_bEngineDirty;
		savage_s *m_psCard;

		/* Physical addresses */
		uint32 m_nFramebufferAddr;
		uint32 m_nRegisterAddr;

		/* Areas for the above phys addresses */
		area_id m_hRegisterArea;
		area_id m_hFramebufferArea;

		bool m_bIsInited;
		
		os::Color32_s m_sColorKey;
		uint32 m_nVideoOffset;
		bool m_bVideoOverlayUsed;

		std::vector<os::screen_mode> m_cModes;
		os::screen_mode m_sCurrentMode;

		/* Control variables */
		bool m_bDisableCOB;		/* Enable or disable COB for Savage4 & ProSavage */
};

/* Replacement inline functions for the X-provided vgaHW functions, adapted from the  GeForceFX driver */
#define readCrtc( index ) DoReadCrtc( psCard, index )
static inline uint8 DoReadCrtc( savage_s *psCard, uint8 index )
{
	uint8 res;
	VGAOUT8( 0x3d4, index );
	res = VGAIN8( 0x3d5 );
	return res;
}

#define writeCrtc( index, val ) DoWriteCrtc( psCard, index, val )
static inline void DoWriteCrtc( savage_s *psCard, uint8 index, uint8 val)
{
	VGAOUT8( 0x3d4, index );
	VGAOUT8( 0x3d5, val );
}

#define readSeq( index ) DoReadSeq( psCard, index )
static inline uint8 DoReadSeq( savage_s *psCard, uint8 index )
{
	uint8 res;
	VGAOUT8( 0x3c4, index );
	res = VGAIN8( 0x3c5 );
	return res;
}

#define readST01() DoReadST01( psCard )
static inline uint8 DoReadST01( savage_s *psCard )
{
	return VGAIN8( VGA_IN_STAT_1_OFFSET );
}

/* Read/write to the DAC via MMIO */
#define inCRReg(reg) DoReadCrtc( psCard, reg )
#define outCRReg(reg, val) DoWriteCrtc( psCard, reg, val )

/* 
 * certain HW cursor operations seem 
 * to require a delay to prevent lockups.
 */
#define waitHSync(n)	{ \
							int num = n; \
							while (num--) \
							{ \
								while ((inStatus1()) & 0x01){};\
								while (!(inStatus1()) & 0x01){};\
							} \
						}

#endif

