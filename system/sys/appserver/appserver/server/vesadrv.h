/*
 *  The AtheOS application server
 *  Copyright (C) 1999 - 2001 Kurt Skauen
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

#ifndef	__F_VESADRV_H__
#define	__F_VESADRV_H__

#include "ddriver.h"

struct VesaMode : public ScreenMode
{
    VesaMode( int w, int h, int bbl, os::color_space cs, int mode, uint32 fb ) : ScreenMode( w,h,bbl,cs ) { m_nVesaMode = mode; m_nFrameBuffer = fb; }
    int	   m_nVesaMode;
    uint32 m_nFrameBuffer;
};

class VesaDriver : public DisplayDriver
{
public:

    VesaDriver();
    ~VesaDriver();

    area_id	Open();
    void	Close();

    virtual int	 GetScreenModeCount();
    virtual bool GetScreenModeDesc( int nIndex, ScreenMode* psMode );
  
    int		SetScreenMode( int nWidth, int nHeight, os::color_space eColorSpc,
			       int nPosH, int nPosV, int nSizeH, int nSizeV, float vRefreshRate );

    virtual int	            GetHorizontalRes();
    virtual int	            GetVerticalRes();
    virtual int	            GetBytesPerLine();
    virtual os::color_space GetColorSpace();
    virtual int		    GetFramebufferOffset() { return( m_nFrameBufferOffset ); }
  
    void		SetColor( int nIndex, const os::Color32_s&	sColor );

    bool		IntersectWithMouse( const os::IRect& cRect );

    bool		DrawLine( SrvBitmap* psBitMap, const os::IRect& cClipRect,
				  const os::IPoint& cPnt1, const os::IPoint& cPnt2, const os::Color32_s& sColor, int nMode );
    bool		FillRect( SrvBitmap* psBitMap, const os::IRect& cRect, const os::Color32_s& sColor );
    bool		BltBitmap( SrvBitmap* pcDstBitMap, SrvBitmap* pcSrcBitMap, os::IRect cSrcRect, os::IPoint cDstPos, int nMode );
private:
    bool		InitModes();
    bool		SetVesaMode( uint32 nMode );

    int	   m_nCurrentMode;
    uint32 m_nFrameBufferSize;
    int	   m_nFrameBufferOffset;
    std::vector<VesaMode> m_cModeList;
};

#endif // __F_VESADRV_H__
