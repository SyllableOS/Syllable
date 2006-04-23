/*

The Syllable application server
Intel i855 graphics driver  
Copyright 1998-1999 Precision Insight, Inc., Cedar Park, Texas.
Copyright 2002 by David Dawes.
Copyright 2004 Arno Klenke

All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sub license, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice (including the
next paragraph) shall be included in all copies or substantial portions
of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
IN NO EVENT SHALL THE COPYRIGHT HOLDERS AND/OR THEIR SUPPLIERS BE LIABLE FOR
ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef	__I855_H__
#define	__I855_H__

#include <atheos/kernel.h>
#include <atheos/vesa_gfx.h>
#include <atheos/pci.h>

#include "../../../server/ddriver.h"
#include "i855_regs.h"
#include "i855_video.h"

#define INREG(addr)         *(volatile uint32 *)( (uint32)m_pRegisterBase + (addr))
#define OUTREG(addr, val)   *(volatile uint32 *)( (uint32)m_pRegisterBase + (addr)) = (val)

#define BEGIN_RING( n )	\
	if( n & 1 )	\
		dbprintf( "i855:: Invalid command!\n" ); \
	uint32 nHead = INREG( LP_RING + RING_HEAD ); \
	uint32 nTail = INREG( LP_RING + RING_TAIL ); \
	if( n > 2 ) \
	{ \
		if( nTail > nHead ) { \
			if( ( m_nRingSize - ( nTail - nHead ) - 16 < n ) || ( nTail - nHead ) > 100 ) \
				WaitForIdle(); \
		} else if( nHead > nTail ) { \
			if( ( ( nHead - nTail ) - 16 < n ) || ( nHead - nTail ) > 100 ) \
				WaitForIdle(); \
		} \
	}
		

#define OUTRING( n ) \
	*(volatile unsigned int *)( m_pRingBase + m_nRingPos ) = n; \
	m_nRingPos += 4;	\
	m_nRingPos &= m_nRingMask;
	
#define FLUSH_RING() \
	OUTREG( LP_RING + RING_TAIL, m_nRingPos );

struct i855Mode : public os::screen_mode
{
	i855Mode( int w, int h, int bbl, os::color_space cs, float rf, int mode, uint32 fb ) : os::screen_mode( w,h,bbl,cs, rf ) { m_nVesaMode = mode; m_nFrameBuffer = fb; }
	int	   m_nVesaMode;
	uint32 m_nFrameBuffer;
};

class i855 : public DisplayDriver
{
public:

    i855( int nFd );
    ~i855();
    
    area_id	Open();
    void	Close();

    virtual int	 GetScreenModeCount();
    virtual bool GetScreenModeDesc( int nIndex, os::screen_mode* psMode );
  
    int		SetScreenMode( os::screen_mode sMode );
    os::screen_mode GetCurrentScreenMode();
   
    virtual int		    GetFramebufferOffset() { return( 0 ); }

    virtual void	LockBitmap( SrvBitmap* pcDstBitmap, SrvBitmap* pcSrcBitmap, os::IRect cSrc, os::IRect cDst );
	
	virtual bool	DrawLine( SrvBitmap* psBitMap, const os::IRect& cClipRect,
				  const os::IPoint& cPnt1, const os::IPoint& cPnt2, const os::Color32_s& sColor, int nMode );
	virtual bool	FillRect( SrvBitmap* psBitMap, const os::IRect& cRect, const os::Color32_s& sColor, int nMode );
	virtual bool	BltBitmap( SrvBitmap* pcDstBitMap, SrvBitmap* pcSrcBitMap, os::IRect cSrcRect, os::IRect cDstRect, int nMode, int nAlpha );

	virtual bool	CreateVideoOverlay( const os::IPoint& cSize, const os::IRect& cDst, os::color_space eFormat, os::Color32_s sColorKey, area_id *pBuffer );
	virtual bool	RecreateVideoOverlay( const os::IPoint& cSize, const os::IRect& cDst, os::color_space eFormat, area_id *pBuffer );
	virtual void	DeleteVideoOverlay( area_id *pBuffer );
	
	bool		IsInitiated() const { return( m_bIsInitiated ); }
private:
    void		TweakMemorySize( int nFd );
    bool		InitModes();
    bool		SetVesaMode( uint32 nMode, int nWidth );
    void		WaitForIdle();
    
    void		UpdateVideo();
    void		VideoOff();
    void		ResetVideo();
	bool		SetCoeffRegs(double *coeff, int mantSize, coeffPtr pCoeff, int pos);
	void		UpdateCoeff(int taps, double fCutoff, bool isHoriz, bool isY, coeffPtr pCoeff);
    bool		SetupVideo( const os::IPoint & cSize, const os::IRect & cDst, os::color_space eFormat, area_id *pBuffer );
    void		SetupVideoOneLine();
    
    bool		m_bIsInitiated;
    PCI_Info_s	m_cPCIInfo;
    os::Locker	m_cGELock;
    bool		m_bEngineDirty;
    
	uint32		m_nBIOSVersion;
	bool		m_bTwoPipes;
    int			m_nDisplayPipe;
    int			m_nDisplayInfo;
    
    vuint8*		m_pRomBase;
	area_id		m_hRomArea;
    
    vuint8*		m_pRegisterBase;
	area_id		m_hRegisterArea;
	
	vuint8*		m_pRingBase;
	area_id		m_hRingArea;
	int			m_nRingPos;
	int			m_nRingSize;
	int			m_nRingSpace;
	int			m_nRingMask;

	vuint8*		m_pFrameBufferBase;
	area_id		m_hFrameBufferArea;
	int			m_nFrameBufferOffset;

	vuint8*		m_pCursorBase;
	uint32		m_nCursorAddress;
	int			m_nCursorOffset;
	
	bool		m_bVideoOverlayUsed;
	uint32		m_nVideoAddress;
	uint32		m_nVideoEnd;
	i855VideoRegs_s* m_psVidRegs;
	bool		m_bVideoIsOn;
	uint32		m_nVideoColorKey;
	os::IPoint	m_cVideoSize;		
	uint32		m_nVideoOffset;
	uint32		m_nVideoGAMC0;
	uint32		m_nVideoGAMC1;
	uint32		m_nVideoGAMC2;
	uint32		m_nVideoGAMC3;
	uint32		m_nVideoGAMC4;
	uint32		m_nVideoGAMC5;
	bool		m_bVideoOneLine;
	int			m_nVideoScale;

    int	   m_nCurrentMode;
    std::vector<i855Mode> m_cModeList;
    
  
};

#endif // __F_VESADRV_H__



