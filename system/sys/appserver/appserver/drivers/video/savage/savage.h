#ifndef _SAVAGE_H
#define _SAVAGE_H

/*
 *  SavageIX Driver for Syllable application server
 *  Copyright (C) 2003 Hilary Cheng (hilarycheng@yahoo.com)
 *
 *  Based on the S3 Savage XFre86 Driver and Savage DirectFB Driver
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

#include <atheos/types.h>
#include <atheos/pci.h>
#include <gui/guidefines.h>

#include "../../../server/ddriver.h"
#include "../../../server/vesadrv.h"

#define	__INTEL__

// internal struct used to clone the add-on from server space to client space
typedef struct {
  PCI_Info_s	pcii;
  vuchar	*base0;
  vuchar      *base1;
  vuchar      *BciMem;
#if !defined(__INTEL__)
  vuchar	*isa_io;
#endif
  int     theMem;
  int	    scrnRowByte;
  int	    scrnWidth;
  int	    scrnHeight;
  color_space	colorspace;
  int	    offscrnWidth;
  int	    offscrnHeight;
  int	    scrnPosH;
  int	    scrnPosV;
  int	    scrnColors;
  void    *scrnBase;
  float   scrnRate;
  short   crtPosH;
  short   crtSizeH;
  short   crtPosV;
  short   crtSizeV;
  //    ulong   scrnResCode;
  int     scrnResNum;
  uchar   *scrnBufBase;
  //    long    scrnRes;
  //    ulong   available_spaces;
  int     hotpt_h;
  int     hotpt_v;
  short   lastCrtHT;
  short   lastCrtVT;
  int     CursorMode;
  ulong	dot_clock;

  int     cobIndex;
  int     cobOffset;
  int     cobSize;
  int     CursorKByte;
} clone_info;

extern clone_info sCardInfo;

#define BCI_CMD_SEND_COLOR    0x00008000

#define BCI_CMD_CLIP_CURRENT  0x00002000
#define BCI_CMD_CLIP_NEW      0x00006000

#define BCI_CMD_RECT          0x48000000
#define BCI_CMD_RECT_XP       0x01000000
#define BCI_CMD_RECT_YP       0x02000000

#define BCI_CMD_DEST_GBD      0x00000000
#define BCI_CMD_SRC_GBD       0x00000020

#define BCI_CMD_SRC_PBD_COLOR 0x00000080 
#define BCI_CMD_SRC_SOLID     0x00000000
#define BCI_CMD_NOP           0x40000000

#define BCI_BD_SET_BPP(bd, bpp) ((bd) |= (((bpp) & 0xFF) << 16))
#define BCI_BD_SET_STRIDE(bd, st) ((bd) |= ((st) & 0xFFFF))

#define BCI_SEND( dw ) (*bci_ptr++ = (unsigned int) (dw))

#define BCI_CLIP_TL(t, l) ((((t) << 16) | (l)) & 0x0FFF0FFF)
#define BCI_CLIP_BR(b, r) ((((b) << 16) | (r)) & 0x0FFF0FFF)

#define BCI_X_Y(x, y) ((((y) << 16) | (x)) & 0x0FFF0FFF)
#define BCI_W_H(w, h) ((((h) << 16) | (w)) & 0x0FFF0FFF)

#define BCI_LINE_X_Y( x, y ) (((y) << 16) | ((x) & 0xFFFF))
#define BCI_LINE_STEPS(diag, axi) (((axi) << 16 | ((diag) & 0xFFFF)))
#define BCI_LINE_MISC(maj, ym, xp, yp, err) \
				   (((maj) & 0x1FFF) | \
				    ((ym) ? 1<<13 : 0) | \
				    ((xp) ? 1<<14 : 0) | \
				    ((yp) ? 1<<15 : 0) | \
				    ((err) << 16))

#define PRI_STREAM_STRIDE         0x81C8
#define SEC_STREAM_STRIDE         0x81D8

#define STATUS_WORD0              (INREG(0x48C00))

#define MAXFIFO                   0x7F00
#define MAXLOOP                   0xFFFFFF

#define SAVAGE_NEWMMIO_REGBASE_S3 0x1000000
#define SAVAGE_NEWMMIO_REGBASE_S4 0x0000000
#define SAVAGE_NEWMMIO_REGSIZE    0x0080000
#define BCI_BD_BW_DISABLE         0x10000000
#define VGA_MISC_OUT_R            0x3C2
#define VGA_MISC_OUT_W            0x3CC

class SavageDriver : public VesaDriver
{
public:

    SavageDriver( int nFd );
    ~SavageDriver();

    area_id	    Open();
    void	    Close();
    virtual int	    GetScreenModeCount();
    virtual bool    GetScreenModeDesc( int nIndex, os::screen_mode* psMode );
    int		    SetScreenMode( os::screen_mode sMode );
    os::screen_mode GetCurrentScreenMode();
   
    void	    SetCursorBitmap( os::mouse_ptr_mode eMode, const os::IPoint& cHotSpot,
				     const void* pRaster, int nWidth, int nHeight );
    void	    MouseOn();
    void	    MouseOff();
    void	    SetMousePos( os::IPoint cNewPos );
    bool	    IntersectWithMouse( const os::IRect& cRect );

    bool	    DrawLine( SrvBitmap* psBitMap, const IRect& cClipRect,
			      const IPoint& cPnt1, const IPoint& cPnt2,
			      const Color32_s& sColor, int nMode );
    bool	    FillRect( SrvBitmap* psBitMap, const IRect& cRect,
			      const Color32_s& sColor );
    bool	    BltBitmap( SrvBitmap* pcDstBitMap, SrvBitmap* pcSrcBitMap,
			       IRect cSrcRect, IPoint cDstPos, int nMode );

    void            EnableMMIO( void );
    void            Initialize2DEngine( void );
    void            SetGBD( void );
    void            WaitQueue( int v );
    void            WaitIdle( void );

private:
    os::Locker                    m_cGELock;
    std::vector<os::screen_mode>  m_cModes;
    os::screen_mode	          m_sCurrentMode;
    uint32                        m_nFrameBufferSize;
    uint8*                        m_pFrameBuffer;
    uint8*                        m_pMMIOBuffer;
    uint8*                        m_pBCIBuffer;
    uint                          s3chip;
    uint16                        vgaCRIndex, vgaCRReg;
};



#endif




