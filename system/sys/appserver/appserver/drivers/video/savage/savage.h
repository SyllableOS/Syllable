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

  int     lDelta;

  uint32   endfb;

  /* Bitmap Descriptors for BCI */
  BMPDESC  GlobalBD;
  BMPDESC  PrimaryBD;
  BMPDESC  SecondBD;

  // volatile unsigned int *bci_ptr;

  int loop;
  int slots;
} clone_info;

extern clone_info sCardInfo;

/*
 * unprotect CRTC[0-7]              
 * CR11_7 = 0: Writing to all CRT Controller registers enabled      
 *        = 1: Writing to all bits of CR0~CR7 except CR7_4 disabled 
 */                                                                 
#define UnProtectCRTC()                 \
do {                                    \
    uchar byte;                          \
    OUTREG8(CRT_ADDRESS_REG,0x11);      \
    byte = INREG8(CRT_DATA_REG) & 0X7F; \
    OUTREG16(CRT_ADDRESS_REG,byte << 8 | 0x11); \
} while (0)

/*                                  
 * unlock extended regs                     
 * CR38:unlock CR20~CR3F            
 * CR39:unlock CR40~CRFF            
 * SR08:unlock SR09~SRFF            
 */                                 
#define UnLockExtRegs()                 \
do {                                    \
    OUTREG16(CRT_ADDRESS_REG,0X4838);   \
    OUTREG16(CRT_ADDRESS_REG,0XA039);   \
    OUTREG16(SEQ_ADDRESS_REG,0X0608);   \
} while (0)


#define VerticalRetraceWait()           \
do {                                    \
    uint8 temp; \
	temp = INREG8(CRT_ADDRESS_REG);            \
	OUTREG8(CRT_ADDRESS_REG, 0x17);     \
	if (INREG8(CRT_DATA_REG) & 0x80) {  \
		int i = 0x10000;                \
		while ((INREG8(SYSTEM_CONTROL_REG) & 0x08) == 0x08 && i--) ; \
		i = 0x10000;                                                  \
		while ((INREG8(SYSTEM_CONTROL_REG) & 0x08) == 0x00 && i--) ; \
	} \
} while (0)

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

#define BCI_CMD_LINE                 0x5C000000
#define BCI_CMD_LINE_LAST_PIXEL      0x58000000

#define BCI_BD_SET_BPP(bd, bpp) ((bd) |= (((bpp) & 0xFF) << 16))
#define BCI_BD_SET_STRIDE(bd, st) ((bd) |= ((st) & 0xFFFF))

#define BCI_SEND( dw ) *bci_ptr++ = (unsigned int) (dw)

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

/*
 * CR/SR registers MMIO offset
 * MISC Output Register(W:0x3c2,R:0x3cc) controls CR is 0X83Cx or 0X83Bx
 * but do we need to set MISC Output Register ???
 * (Note that CRT_ADDRESS_REG and CRT_DATA_REG are assumed to be COLOR)???
 */
#define MMIO_BASE_OF_VGA3C0             0X83C0
#define MMIO_BASE_OF_VGA3D0             0X83D0

#define ATTR_ADDRESS_REG        \
    (MMIO_BASE_OF_VGA3C0 + (0x03C0 - 0x03C0))
#define ATTR_DATA_WRITE_REG     \
    (MMIO_BASE_OF_VGA3C0 + (0x03C0 - 0x03C0))
#define ATTR_DATA_READ_REG      \
    (MMIO_BASE_OF_VGA3C0 + (0x03C1 - 0x03C0))
#define VIDEO_SUBSYSTEM_ENABLE  \
    (MMIO_BASE_OF_VGA3C0 + (0x03C3 - 0x03C0))
#define SEQ_ADDRESS_REG         \
    (MMIO_BASE_OF_VGA3C0 + (0x03C4 - 0x03C0))
#define SEQ_DATA_REG            \
    (MMIO_BASE_OF_VGA3C0 + (0x03C5 - 0x03C0))
#define DAC_PIXEL_MASK_REG      \
    (MMIO_BASE_OF_VGA3C0 + (0x03C6 - 0x03C0))
#define DAC_PEL_MASK            \
    (MMIO_BASE_OF_VGA3C0 + (0x03C6 - 0x03C0))
#define DAC_STATUS_REG          \
    (MMIO_BASE_OF_VGA3C0 + (0x03C7 - 0x03C0))
#define DAC_ADDRESS_READ_REG    \
    (MMIO_BASE_OF_VGA3C0 + (0x03C7 - 0x03C0))
#define DAC_ADDRESS_WRITE_REG   \
    (MMIO_BASE_OF_VGA3C0 + (0x03C8 - 0x03C0))
#define DAC_DATA_REG            \
    (MMIO_BASE_OF_VGA3C0 + (0x03C9 - 0x03C0))
#define DAC_DATA_REG_PORT       \
    (MMIO_BASE_OF_VGA3C0 + (0x03C9 - 0x03C0))
#define MISC_OUTPUT_REG_WRITE   \
    (MMIO_BASE_OF_VGA3C0 + (0x03C2 - 0x03C0))
#define MISC_OUTPUT_REG_READ    \
    (MMIO_BASE_OF_VGA3C0 + (0x03CC - 0x03C0))
#define GR_ADDRESS_REG          \
    (MMIO_BASE_OF_VGA3C0 + (0x03CE - 0x03C0))
#define GR_DATA_REG             \
    (MMIO_BASE_OF_VGA3C0 + (0x03CF - 0x03C0))
#define WAKEUP_REG              \
    (MMIO_BASE_OF_VGA3C0 + (0x0510 - 0x03C0))

#define CRT_ADDRESS_REG         \
    (MMIO_BASE_OF_VGA3D0 + (0x03D4 - 0x03D0))
#define CRT_DATA_REG            \
    (MMIO_BASE_OF_VGA3D0 + (0x03D5 - 0x03D0))
#define SYSTEM_CONTROL_REG      \
    (MMIO_BASE_OF_VGA3D0 + (0x03DA - 0x03D0))

#define   BCI_ENABLE                    0

#define   S3_BIG_ENDIAN                    4
#define   S3_LITTLE_ENDIAN                 0
#define   S3_BD64                          1

#define MEM_PS1                     0x10    /*CRCA_4 :Primary stream 1*/
#define MEM_PS2                     0x20    /*CRCA_5 :Primary stream 2*/
#define MEM_SS1                     0x40    /*CRCA_6 :Secondary stream 1*/
#define MEM_SS2                     0x80    /*CRCA_7 :Secondary stream 2*/

/* Stream Processor 1 */

/* Primary Stream 1 Frame Buffer Address 0 */
#define PRI_STREAM_FBUF_ADDR0           0x81c0
/* Primary Stream 1 Frame Buffer Address 0 */
#define PRI_STREAM_FBUF_ADDR1           0x81c4
/* Primary Stream 1 Stride */
#define PRI_STREAM_STRIDE               0x81c8
/* Primary Stream 1 Frame Buffer Size */
#define PRI_STREAM_BUFFERSIZE           0x8214

/* Secondary stream 1 Color/Chroma Key Control */
#define SEC_STREAM_CKEY_LOW             0x8184
/* Secondary stream 1 Chroma Key Upper Bound */
#define SEC_STREAM_CKEY_UPPER           0x8194
/* Blend Control of Secondary Stream 1 & 2 */
#define BLEND_CONTROL                   0x8190
/* Secondary Stream 1 Color conversion/Adjustment 1 */
#define SEC_STREAM_COLOR_CONVERT1       0x8198
/* Secondary Stream 1 Color conversion/Adjustment 2 */
#define SEC_STREAM_COLOR_CONVERT2       0x819c
/* Secondary Stream 1 Color conversion/Adjustment 3 */
#define SEC_STREAM_COLOR_CONVERT3       0x81e4
/* Secondary Stream 1 Horizontal Scaling */
#define SEC_STREAM_HSCALING             0x81a0
/* Secondary Stream 1 Frame Buffer Size */
#define SEC_STREAM_BUFFERSIZE           0x81a8
/* Secondary Stream 1 Horizontal Scaling Normalization (2K only) */
#define SEC_STREAM_HSCALE_NORMALIZE	0x81ac
/* Secondary Stream 1 Horizontal Scaling */
#define SEC_STREAM_VSCALING             0x81e8
/* Secondary Stream 1 Frame Buffer Address 0 */
#define SEC_STREAM_FBUF_ADDR0           0x81d0
/* Secondary Stream 1 Frame Buffer Address 1 */
#define SEC_STREAM_FBUF_ADDR1           0x81d4
/* Secondary Stream 1 Frame Buffer Address 2 */
#define SEC_STREAM_FBUF_ADDR2           0x81ec
/* Secondary Stream 1 Stride */
#define SEC_STREAM_STRIDE               0x81d8
/* Secondary Stream 1 Window Start Coordinates */
#define SEC_STREAM_WINDOW_START         0x81f8
/* Secondary Stream 1 Window Size */
#define SEC_STREAM_WINDOW_SZ            0x81fc
/* Secondary Streams Tile Offset */
#define SEC_STREAM_TILE_OFF             0x821c
/* Secondary Stream 1 Opaque Overlay Control */
#define SEC_STREAM_OPAQUE_OVERLAY       0x81dc


/* Stream Processor 2 */

/* Primary Stream 2 Frame Buffer Address 0 */
#define PRI_STREAM2_FBUF_ADDR0          0x81b0
/* Primary Stream 2 Frame Buffer Address 1 */
#define PRI_STREAM2_FBUF_ADDR1          0x81b4
/* Primary Stream 2 Stride */
#define PRI_STREAM2_STRIDE              0x81b8
/* Primary Stream 2 Frame Buffer Size */
#define PRI_STREAM2_BUFFERSIZE          0x8218

/* Secondary Stream 2 Color/Chroma Key Control */
#define SEC_STREAM2_CKEY_LOW            0x8188
/* Secondary Stream 2 Chroma Key Upper Bound */
#define SEC_STREAM2_CKEY_UPPER          0x818c
/* Secondary Stream 2 Horizontal Scaling */
#define SEC_STREAM2_HSCALING            0x81a4
/* Secondary Stream 2 Horizontal Scaling */
#define SEC_STREAM2_VSCALING            0x8204
/* Secondary Stream 2 Frame Buffer Size */
#define SEC_STREAM2_BUFFERSIZE          0x81ac
/* Secondary Stream 2 Frame Buffer Address 0 */
#define SEC_STREAM2_FBUF_ADDR0          0x81bc
/* Secondary Stream 2 Frame Buffer Address 1 */
#define SEC_STREAM2_FBUF_ADDR1          0x81e0
/* Secondary Stream 2 Frame Buffer Address 2 */
#define SEC_STREAM2_FBUF_ADDR2          0x8208
/* Multiple Buffer/LPB and Secondary Stream 2 Stride */
#define SEC_STREAM2_STRIDE_LPB          0x81cc
/* Secondary Stream 2 Color conversion/Adjustment 1 */
#define SEC_STREAM2_COLOR_CONVERT1      0x81f0
/* Secondary Stream 2 Color conversion/Adjustment 2 */
#define SEC_STREAM2_COLOR_CONVERT2      0x81f4
/* Secondary Stream 2 Color conversion/Adjustment 3 */
#define SEC_STREAM2_COLOR_CONVERT3      0x8200
/* Secondary Stream 2 Window Start Coordinates */
#define SEC_STREAM2_WINDOW_START        0x820c
/* Secondary Stream 2 Window Size */
#define SEC_STREAM2_WINDOW_SZ           0x8210
/* Secondary Stream 2 Opaque Overlay Control */
#define SEC_STREAM2_OPAQUE_OVERLAY      0x8180

#define STATUS_WORD0              (INREG(0x48C00))

#define MAXFIFO                   0x7F00
#define MAXLOOP                   0xFFFFFF

#define SAVAGE_NEWMMIO_REGBASE_S3 0x1000000
#define SAVAGE_NEWMMIO_REGBASE_S4 0x0000000
#define SAVAGE_NEWMMIO_REGSIZE    0x0080000
#define BCI_BD_BW_DISABLE         0x10000000
#define VGA_MISC_OUT_R            0x3C2
#define VGA_MISC_OUT_W            0x3CC

#define SELECT_IGA1               0x4026
#define SELECT_IGA2_READS_WRITES  0x4f26

/* GX-3 Configuration/Status Registers */
#define S3_SHADOW_STATUS              0x48C0C
#define S3_BUFFER_THRESHOLD           0x48C10
#define S3_OVERFLOW_BUFFER            0x48C14
#define S3_OVERFLOW_BUFFER_PTR        0x48C18

#define ENABLE_BCI                        0x08   /* MM48C18_3 */
#define ENABLE_COMMAND_OVERFLOW_BUF       0x04   /* MM48C18_2 */
#define ENABLE_COMMAND_BUF_STATUS_UPDATE  0x02   /* MM48C18_1 */
#define ENABLE_SHADOW_STATUS_UPDATE       0x01   /* MM48C0C_0 */

#define MEMORY_CTRL0_REG            0xCA
#define MEMORY_CTRL1_REG            0xCB
#define MEMORY_CTRL2_REG            0xCC

#define MEMORY_CONFIG_REG           0x31

#define TILED_SURFACE_REGISTER_0        0x48c40
#define TILED_SURFACE_REGISTER_1        0x48c44
#define TILED_SURFACE_REGISTER_2        0x48c48
#define TILED_SURFACE_REGISTER_3        0x48c4c
#define TILED_SURFACE_REGISTER_4        0x48c50
                                                
#define TILED_SURF_BPP4    0x00000000  /* bits 31-30=00 for  4 bits/pixel */
#define TILED_SURF_BPP8    0x40000000  /* bits 31-30=01 for  8 bits/pixel */
#define TILED_SURF_BPP16   0x80000000	/* bits 31-30=10 for 16 bits/pixel */
#define TILED_SURF_BPP32   0xC0000000  /* bits 31-30=11 for 32 bits/pixel */

/* CR31[0] set = Enable 8MB display memory through 64K window at A0000H. */
#define ENABLE_CPUA_BASE_A0000      0x01  

/* bitmap descriptor register */
#define S3_GLB_BD_LOW                      0X8168
#define S3_GLB_BD_HIGH                     0X816C
#define S3_PRI_BD_LOW                      0X8170
#define S3_PRI_BD_HIGH                     0X8174
#define S3_SEC_BD_LOW                      0X8178
#define S3_SEC_BD_HIGH                     0X817c

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

    void            EnableMode(uint8 bEnable);
    void            EnableMode_M7(uint8 bEnable);

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
