
/*
 *  Graphics driver for Voodoo Banshee, Voodoo 3 & Voodoo 5 chipsets
 *  (appserver driver),
 *
 *  based on the Linux framebuffer driver (tdfxfb.c),
 *    Copyright (C) 1999 Hannu Mallat
 *
 *  and Arno Klenke's port of the geforcefx driver,
 *    Copyright (C) 2003 Arno Klenke
 *
 *  Copyright (C) 2004 Jan L. Hauffa
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
 *
 */


#ifndef __TDFX_H__
#define __TDFX_H__

#include <atheos/isa_io.h>

#include "../../../server/ddriver.h"


#define MAX_CURS 64
#define APPSERVER_DRIVER "tdfx"

/* membase0 register offsets */
#define STATUS          0x00
#define PCIINIT0        0x04
#define SIPMONITOR      0x08
#define LFBMEMORYCONFIG 0x0c
#define MISCINIT0       0x10
#define MISCINIT1       0x14
#define DRAMINIT0       0x18
#define DRAMINIT1       0x1c
#define AGPINIT         0x20
#define TMUGBEINIT      0x24
#define VGAINIT0        0x28
#define VGAINIT1        0x2c
#define DRAMCOMMAND     0x30
#define DRAMDATA        0x34
#define PLLCTRL0        0x40
#define PLLCTRL1        0x44
#define PLLCTRL2        0x48
#define DACMODE         0x4c
#define DACADDR         0x50
#define DACDATA         0x54
#define RGBMAXDELTA     0x58
#define VIDPROCCFG      0x5c
#define HWCURPATADDR    0x60
#define HWCURLOC        0x64
#define HWCURC0         0x68
#define HWCURC1         0x6c
#define VIDINFORMAT     0x70
#define VIDINSTATUS     0x74
#define VIDSERPARPORT   0x78
#define VIDINXDELTA     0x7c
#define VIDININITERR    0x80
#define VIDINYDELTA     0x84
#define VIDPIXBUFTHOLD  0x88
#define VIDCHRMIN       0x8c
#define VIDCHRMAX       0x90
#define VIDCURLIN       0x94
#define VIDSCREENSIZE   0x98
#define VIDOVRSTARTCRD  0x9c
#define VIDOVRENDCRD    0xa0
#define VIDOVRDUDX      0xa4
#define VIDOVRDUDXOFF   0xa8
#define VIDOVRDVDY      0xac
#define VIDOVRDVDYOFF   0xe0
#define VIDDESKSTART    0xe4
#define VIDDESKSTRIDE   0xe8
#define VIDINADDR0      0xec
#define VIDINADDR1      0xf0
#define VIDINADDR2      0xf4
#define VIDINSTRIDE     0xf8
#define VIDCUROVRSTART  0xfc

#define INTCTRL         (0x00100000 + 0x04)
#define CLIP0MIN        (0x00100000 + 0x08)
#define CLIP0MAX        (0x00100000 + 0x0c)
#define DSTBASE         (0x00100000 + 0x10)
#define DSTFORMAT       (0x00100000 + 0x14)
#define SRCBASE         (0x00100000 + 0x34)
#define COMMANDEXTRA_2D (0x00100000 + 0x38)
#define LINESTIPPLE     (0x00100000 + 0x3c)
#define LINESTYLE       (0x00100000 + 0x40)
#define CLIP1MIN        (0x00100000 + 0x4c)
#define CLIP1MAX        (0x00100000 + 0x50)
#define SRCFORMAT       (0x00100000 + 0x54)
#define SRCSIZE         (0x00100000 + 0x58)
#define SRCXY           (0x00100000 + 0x5c)
#define COLORBACK       (0x00100000 + 0x60)
#define COLORFORE       (0x00100000 + 0x64)
#define DSTSIZE         (0x00100000 + 0x68)
#define DSTXY           (0x00100000 + 0x6c)
#define COMMAND_2D      (0x00100000 + 0x70)
#define LAUNCH_2D       (0x00100000 + 0x80)
#define COMMAND_3D      (0x00200000 + 0x120)

#define BIT(x) (1UL << (x))

/* COMMAND_2D reg. values */
#define TDFX_ROP_SRCCOPY     0xcc	// src
#define TDFX_ROP_DSTINVERT   0x55	// NOT dst
#define TDFX_ROP_SRCINVERT   0x66	// src XOR dst
#define TDFX_ROP_SRCERASE    0x44	// src AND (NOT dst)
#define TDFX_ROP_SRCPAINT    0xee	// src OR dst

#define AUTOINC_DSTX                    BIT(10)
#define AUTOINC_DSTY                    BIT(11)
#define CLIP_SEL1                       BIT(23)

#define COMMAND_2D_FILLRECT             0x05
#define COMMAND_2D_LINE                 0x06
#define COMMAND_2D_S2S_BITBLT           0x01	// screen to screen
#define COMMAND_2D_H2S_BITBLT           0x03	// host to screen

#define COMMAND_3D_NOP                  0x00

#define STATUS_RETRACE                  BIT(6)
#define STATUS_BUSY                     BIT(9)
#define MISCINIT1_CLUT_INV              BIT(0)
#define MISCINIT1_2DBLOCK_DIS           BIT(15)
#define DRAMINIT0_SGRAM_NUM             BIT(26)
#define DRAMINIT0_SGRAM_TYPE            BIT(27)
#define DRAMINIT1_MEM_SDRAM             BIT(30)
#define VGAINIT0_VGA_DISABLE            BIT(0)
#define VGAINIT0_EXT_TIMING             BIT(1)
#define VGAINIT0_8BIT_DAC               BIT(2)
#define VGAINIT0_EXT_ENABLE             BIT(6)
#define VGAINIT0_WAKEUP_3C3             BIT(8)
#define VGAINIT0_LEGACY_DISABLE         BIT(9)
#define VGAINIT0_ALT_READBACK           BIT(10)
#define VGAINIT0_FAST_BLINK             BIT(11)
#define VGAINIT0_EXTSHIFTOUT            BIT(12)
#define VGAINIT0_DECODE_3C6             BIT(13)
#define VGAINIT0_SGRAM_HBLANK_DISABLE   BIT(22)
#define VGAINIT1_MASK                   0x1fffff
#define DACMODE_2X                      BIT(0)

#define VIDCFG_VIDPROC_ENABLE           BIT(0)
#define VIDCFG_CURS_X11                 BIT(1)
#define VIDCFG_HALF_MODE                BIT(4)
#define VIDCFG_DESK_ENABLE              BIT(7)
#define VIDCFG_DESK_CLUT_BYPASS         BIT(10)
#define VIDCFG_OVL_CLUT_BYPASS          BIT(11)
#define VIDCFG_2X                       BIT(26)
#define VIDCFG_HWCURSOR_ENABLE          BIT(27)
#define VIDCFG_PIXFMT_SHIFT             18

#define VIDCFG_FMT_RGB565               (1 << 21)
#define VIDCFG_FMT_YUV411               (4 << 21)
#define VIDCFG_FMT_YUYV422              (5 << 21)
#define VIDCFG_FMT_UYVY422              (6 << 21)
#define VIDCFG_FMT_RGB565D              (7 << 21)

#define VIDCFG_OVERLAY_MASK             0x5d1c1493
#define VIDCFG_OVERLAY_ENABLE           0x00000320


#define MISC_W  0x3c2
#define MISC_R  0x3cc
#define SEQ_I   0x3c4
#define SEQ_D   0x3c5
#define CRT_I   0x3d4
#define CRT_D   0x3d5
#define ATT_IW  0x3c0
#define IS1_R   0x3da
#define GRA_I   0x3ce
#define GRA_D   0x3cf


struct tdfx_reg
{
	/* VGA */
	unsigned char att[21];
	unsigned char crt[25];
	unsigned char gra[9];
	unsigned char misc[1];
	unsigned char seq[5];

	/* Banshee extensions */
	unsigned char ext[2];
	unsigned long vidcfg;
	unsigned long vidpll;
	unsigned long mempll;
	unsigned long gfxpll;
	unsigned long dacmode;
	unsigned long vgainit0;
	unsigned long vgainit1;
	unsigned long screensize;
	unsigned long stride;
	unsigned long cursloc;
	unsigned long curspataddr;
	unsigned long cursc0;
	unsigned long cursc1;
	unsigned long startaddr;
	unsigned long clip0min;
	unsigned long clip0max;
	unsigned long clip1min;
	unsigned long clip1max;
	unsigned long srcbase;
	unsigned long dstbase;
	unsigned long miscinit0;
};


#define ID_3DFX_BANSHEE 0x121a0003
#define ID_3DFX_VOODOO3 0x121a0005
#define ID_3DFX_VOODOO5 0x121a0009

#define NUM_SUPPORTED 3

struct chip_info
{
	uint32 nId;
	int nMaxPixclock;
};

const struct chip_info asSupportedChips[NUM_SUPPORTED] =
{
    {ID_3DFX_BANSHEE, 270000},
    {ID_3DFX_VOODOO3, 300000},
    {ID_3DFX_VOODOO5, 350000}
};


const int bytesPerPixel[] =
{
	-1, 4, -1, -1, 2, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
};

const int lineRop[] =
{
	TDFX_ROP_SRCCOPY, TDFX_ROP_SRCCOPY, TDFX_ROP_DSTINVERT,	-1, -1, -1, -1, -1,
    -1, -1
};



class TDFX:public DisplayDriver
{
public:
	TDFX( int nFd );
	 ~TDFX();

	virtual area_id Open();
	virtual void Close();

	virtual int GetScreenModeCount();
	virtual bool GetScreenModeDesc( int nIndex, os::screen_mode * psMode );
	virtual int SetScreenMode( os::screen_mode );
	virtual os::screen_mode GetCurrentScreenMode();

	
	virtual bool DrawLine( SrvBitmap * psBitMap, const os::IRect & cClipRect,
        const os::IPoint & cPnt1, const os::IPoint & cPnt2,
        const os::Color32_s & sColor, int nMode );
	virtual bool FillRect( SrvBitmap * psBitMap, const os::IRect & cRect,
        const os::Color32_s & sColor, int nMode );
	virtual bool BltBitmap( SrvBitmap * pcDstBitMap, SrvBitmap * pcSrcBitMap,
        os::IRect cSrcRect, os::IRect cDstRect, int nMode, int nAlpha );

	virtual bool CreateVideoOverlay( const os::IPoint & cSize,
        const os::IRect & cDst, os::color_space eFormat,
        os::Color32_s sColorKey, area_id *pBuffer );
	virtual bool RecreateVideoOverlay( const os::IPoint & cSize,
        const os::IRect & cDst, os::color_space eFormat, area_id *pBuffer );
	virtual void DeleteVideoOverlay( area_id *pBuffer );
	bool CreateOverlayCommon( const os::IPoint & cSize, const os::IRect & cDst,
        area_id *pBuffer );

	bool IsInitiated() const
	{
		return m_bIsInitiated;
	}


private:
	bool m_bIsInitiated;
	os::Locker m_cGELock;
	uint32 m_nChipsetId, m_nFbSize;
	int m_nMaxPixclock;

	vuint8 *m_pRegBase, *m_pFrameBufferBase;
	uint32 m_nFbBasePhys;
	area_id m_hRegArea, m_hFrameBufferArea;

	std::vector < os::screen_mode > m_cScreenModeList;
	os::screen_mode m_sCurrentMode;

	bool m_bVideoOverlayUsed;
	uint32 m_nColorKey;

	inline uint8 vga_inb( uint32 reg )
	{
		return inb( reg );
	}
	inline uint16 vga_inw( uint32 reg )
	{
		return inw( reg );
	}
	inline uint32 vga_inl( uint32 reg )
	{
		return inl( reg );
	}

	// DANGER! Reverse order of parameters!
	inline void vga_outb( uint32 reg, uint8 val )
	{
		outb( val, reg );
	}
	inline void vga_outw( uint32 reg, uint16 val )
	{
		outw( val, reg );
	}
	inline void vga_outl( uint32 reg, uint32 val )
	{
		outl( val, reg );
	}

	inline void gra_outb( uint32 idx, uint8 val )
	{
		vga_outb( GRA_I, idx );
		vga_outb( GRA_D, val );
	}
	inline void seq_outb( uint32 idx, uint8 val )
	{
		vga_outb( SEQ_I, idx );
		vga_outb( SEQ_D, val );
	}
	inline uint8 seq_inb( uint32 idx )
	{
		vga_outb( SEQ_I, idx );
		return vga_inb( SEQ_D );
	}
	inline void crt_outb( uint32 idx, uint8 val )
	{
		vga_outb( CRT_I, idx );
		vga_outb( CRT_D, val );
	}
	inline uint8 crt_inb( uint32 idx )
	{
		vga_outb( CRT_I, idx );
		return vga_inb( CRT_D );
	}

	inline void att_outb( uint32 idx, uint8 val )
	{
		unsigned char tmp;

		tmp = vga_inb( IS1_R );
		vga_outb( ATT_IW, idx );
		vga_outb( ATT_IW, val );
	}

	inline void vga_disable_video()
	{
		unsigned char s;

		s = seq_inb( 0x01 ) | 0x20;
		seq_outb( 0x00, 0x01 );
		seq_outb( 0x01, s );
		seq_outb( 0x00, 0x03 );
	}

	inline void vga_enable_video()
	{
		unsigned char s;

		s = seq_inb( 0x01 ) & 0xdf;
		seq_outb( 0x00, 0x01 );
		seq_outb( 0x01, s );
		seq_outb( 0x00, 0x03 );
	}

	inline void vga_enable_palette()
	{
		vga_inb( IS1_R );
		vga_outb( ATT_IW, 0x20 );
	}

	inline uint32 tdfx_inl( unsigned int reg ) const
	{
		return *( ( vuint32 * )( m_pRegBase + reg ) );
	}
	inline void tdfx_outl( unsigned int reg, uint32 val ) const
	{
		*( ( vuint32 * )( m_pRegBase + reg ) ) = val;
	}

	inline uint32 pci_size( uint32 base, uint32 mask );
	uint32 get_pci_memory_size( int nFd, const PCI_Info_s * pcPCIInfo,
        int nResource );
	inline void fifo_make_room( unsigned int size );
	void wait_idle();
	uint32 calc_pll( int freq, int *freq_out );
	void write_regs( struct tdfx_reg *reg );
	uint32 get_lfb_size();
};


#endif
