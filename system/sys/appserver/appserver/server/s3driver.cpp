/*
 *  The AtheOS application server
 *  Copyright (C) 1999  Kurt Skauen
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

#include <atheos/types.h>
#include <sys/types.h>
#include <atheos/isa_io.h>

#include "ddriver.h"

// Standard glue.
#ifndef _S3_H
#define _S3_H

// Number of different chip set define. This is related to the size of every
// tables, just to be sure you're not forgetting to extend one of them.
#define  CHIP_COUNT     6

// Internal chip set id.
#define  s3_964         0
#define  s3_864         1
#define  s3_trio32      2
#define  s3_trio64      3
#define  s3_trio64vp    4
#define  s3_virge       5

// Temporary id for both trio32 and trio64
#define  s3_trio00      99

// Internal DAC id.
#define  tvp3025_DAC    1
#define  att21c498_DAC  2
#define  s3trio_DAC     3

// Internal resolution id.
#define vga640x480		1		// resolutions
#define vga800x600		2
#define vga1024x768		3
#define vga1152x900     4
#define vga1280x1024    5
#define vga1600x1200    6
#define vga_specific    7

// Macro use to access an specified adress in the io-register space.
#define ISA_ADDRESS(x)  (isa_IO+(x))

// Standard macro.
#ifdef abs
#undef abs
#endif
#define abs(a) (((a)>0)?(a):(-(a)))





#if 1

// S3 graphic engine registers and masks definition.
// (see any standard s3 databook for more informations).
#define	SUBSYS_STAT		0x42e8
#define	SUBSYS_CNTL		0x42e8
#define	ADVFUNC_CNTL	0x4ae8
#define	CUR_Y					0x82e8
#define	CUR_X					0x86e8
#define	DESTY_AXSTP		0x8ae8
#define	DESTX_DIASTP	0x8ee8
#define	ERR_TERM			0x92e8
#define	MAJ_AXIS_PCNT	0x96e8
#define	GP_STAT				0x9ae8
#define	CMD						0x9ae8
#define	SHORT_STROKE	0x9ee8
#define	BKGD_COLOR		0xa2e8
#define	FRGD_COLOR		0xa6e8
#define	WRT_MASK			0xaae8
#define	RD_MASK				0xaee8
#define	COLOR_CMP			0xb2e8
#define	BKGD_MIX			0xb6e8
#define	FRGD_MIX			0xbae8
#define	RD_REG_DT			0xbee8
#define	PIX_TRANS			0xe2e8
#define	PIX_TRANS_EXT	0xe2ea

#define	DATA_REG			0xbee8
#define	MIN_AXIS_PCNT	0x0000
#define	SCISSORS_T		0x1000
#define	SCISSORS_L		0x2000
#define	SCISSORS_B		0x3000
#define	SCISSORS_R		0x4000
#define	PIX_CNTL			0xa000
#define	MULT_MISC2		0xd000
#define	MULT_MISC			0xe000
#define	READ_SEL			0xf000


// Direct standard VGA registers, special index and data registers
  // with CR55 low bit == 0
#define TI_WRITE_ADDR		0x3C8
#define TI_RAMDAC_DATA	0x3C9
#define TI_PIXEL_MASK		0x3C6
#define TI_READ_ADDR		0x3C7
  // with CR55 low bit == 1
#define TI_INDEX_REG		0x3C6
#define TI_DATA_REG			0x3C7


// TVP 3025 indirect indexed registers and masks definition
// (see TVP 3025 databook for more information).
#define TI_CURS_X_LOW			0x00
#define TI_CURS_X_HIGH			0x01
#define TI_CURS_Y_LOW			0x02
#define TI_CURS_Y_HIGH			0x03
#define TI_SPRITE_ORIGIN_X		0x04
#define TI_SPRITE_ORIGIN_Y		0x05
#define TI_CURS_CONTROL			0x06
#define TI_PLANAR_ACCESS		0x80
#define TI_CURS_SPRITE_ENABLE 	0x40
#define TI_CURS_X_WINDOW_MODE 	0x10
#define TI_CURS_CTRL_MASK     	(TI_CURS_SPRITE_ENABLE | TI_CURS_X_WINDOW_MODE)
#define TI_CURS_RAM_ADDR_LOW	0x08
#define TI_CURS_RAM_ADDR_HIGH	0x09
#define TI_CURS_RAM_DATA		0x0A
#define TI_TRUE_COLOR_CONTROL	0x0E
#define TI_TC_BTMODE			0x04
#define TI_TC_NONVGAMODE		0x02
#define TI_TC_8BIT				0x01
#define TI_VGA_SWITCH_CONTROL	0x0F
#define TI_WINDOW_START_X_LOW	0x10
#define TI_WINDOW_START_X_HIGH	0x11
#define TI_WINDOW_STOP_X_LOW	0x12
#define TI_WINDOW_STOP_X_HIGH	0x13
#define TI_WINDOW_START_Y_LOW	0x14
#define TI_WINDOW_START_Y_HIGH	0x15
#define TI_WINDOW_STOP_Y_LOW	0x16
#define TI_WINDOW_STOP_Y_HIGH	0x17
#define TI_MUX_CONTROL_1		0x18
#define TI_MUX1_PSEUDO_COLOR	0x80
#define TI_MUX1_DIRECT_888		0x06
#define TI_MUX1_DIRECT_565		0x05
#define TI_MUX1_DIRECT_555		0x04
#define TI_MUX1_DIRECT_664		0x03
#define TI_MUX1_WEIRD_MODE_1	0xC6
#define TI_MUX1_WEIRD_MODE_2	0x4D
#define TI_MUX1_WEIRD_MODE_3	0x4E
#define TI_MUX_CONTROL_2		0x19
#define TI_MUX2_BUS_VGA			0x98
#define TI_MUX2_BUS_PC_D8P64	0x1C
#define TI_MUX2_BUS_DC_D24P64	0x1C
#define TI_MUX2_BUS_DC_D16P64	0x04
#define TI_MUX2_BUS_DC_D15P64	0x04
#define TI_MUX2_BUS_TC_D24P64	0x04
#define TI_MUX2_BUS_TC_D16P64	0x04
#define TI_MUX2_BUS_TC_D15P64	0x04
#define TI_MUX2_WEIRD_MODE_2	0x14
#define TI_MUX2_WEIRD_MODE_3	0x04
#define TI_INPUT_CLOCK_SELECT	0x1A
#define TI_ICLK_CLK0			0x00
#define TI_ICLK_CLK0_DOUBLE		0x10
#define TI_ICLK_CLK1			0x01
#define TI_ICLK_CLK1_DOUBLE		0x11
#define TI_ICLK_CLK2			0x02
#define TI_ICLK_CLK2_DOUBLE		0x12
#define TI_ICLK_CLK2_I			0x03
#define TI_ICLK_CLK2_I_DOUBLE	0x13
#define TI_ICLK_CLK2_E			0x04
#define TI_ICLK_CLK2_E_DOUBLE	0x14
#define TI_ICLK_PLL				0x05
#define TI_OUTPUT_CLOCK_SELECT	0x1B
#define TI_OCLK_VGA				0x3E
#define TI_OCLK_S_V1_R8			0x43
#define TI_OCLK_S_V2_R8			0x4B
#define TI_OCLK_S_V4_R8			0x53
#define TI_OCLK_S_V8_R8			0x5B
#define TI_OCLK_S_V2_R4			0x4A
#define TI_OCLK_S_V4_R4			0x52
#define TI_OCLK_S_V1_R2			0x41
#define TI_OCLK_S_V2_R2			0x49
#define TI_OCLK_NS_V1_R1		0x80
#define TI_OCLK_NS_V2_R2		0x89
#define TI_OCLK_NS_V4_R4		0x92
#define TI_PALETTE_PAGE			0x1C
#define TI_GENERAL_CONTROL		0x1D
#define TI_MISC_CONTROL			0x1E
#define TI_MC_POWER_DOWN		0x01
#define TI_MC_DOTCLK_DISABLE	0x02
#define TI_MC_INT_6_8_CONTROL	0x04
#define TI_MC_8_BPP				0x08
#define TI_MC_VCLK_POLARITY		0x20
#define TI_MC_LCLK_LATCH		0x40
#define TI_MC_LOOP_PLL_RCLK		0x80
#define TI_OVERSCAN_COLOR_RED	0x20
#define TI_OVERSCAN_COLOR_GREEN	0x21
#define TI_OVERSCAN_COLOR_BLUE	0x22
#define TI_CURSOR_COLOR_0_RED	0x23
#define TI_CURSOR_COLOR_0_GREEN	0x24
#define TI_CURSOR_COLOR_0_BLUE	0x25
#define TI_CURSOR_COLOR_1_RED	0x26
#define TI_CURSOR_COLOR_1_GREEN	0x27
#define TI_CURSOR_COLOR_1_BLUE	0x28
#define TI_AUXILLARY_CONTROL	0x29
#define TI_AUX_SELF_CLOCK		0x08
#define TI_AUX_W_CMPL			0x01
#define TI_GENERAL_IO_CONTROL	0x2A
#define TI_GIC_ALL_BITS			0x1F
#define TI_GENERAL_IO_DATA		0x2B
#define TI_GID_W2000_6BIT     	0x00
#define TI_GID_N9_964			0x01
#define TI_GID_W2000_8BIT     	0x08
#define TI_GID_S3_DAC_6BIT		0x1C
#define TI_GID_S3_DAC_8BIT		0x1E
#define TI_GID_TI_DAC_6BIT		0x1D
#define TI_GID_TI_DAC_8BIT		0x1F
#define TI_PLL_CONTROL			0x2C
#define TI_PIXEL_CLOCK_PLL_DATA	0x2D
#define TI_PLL_ENABLE			0x08
#define TI_MCLK_PLL_DATA		0x2E
#define TI_LOOP_CLOCK_PLL_DATA	0x2F
#define TI_COLOR_KEY_OLVGA_LOW	0x30
#define TI_COLOR_KEY_OLVGA_HIGH	0x31
#define TI_COLOR_KEY_RED_LOW	0x32
#define TI_COLOR_KEY_RED_HIGH	0x33
#define TI_COLOR_KEY_GREEN_LOW	0x34
#define TI_COLOR_KEY_GREEN_HIGH	0x35
#define TI_COLOR_KEY_BLUE_LOW	0x36
#define TI_COLOR_KEY_BLUE_HIGH	0x37
#define TI_COLOR_KEY_CONTROL	0x38
#define TI_SENSE_TEST			0x3A
#define TI_TEST_DATA			0x3B
#define TI_CRC_LOW				0x3C
#define TI_CRC_HIGH				0x3D
#define TI_CRC_CONTROL			0x3E
#define TI_ID					0x3F
#define TI_VIEWPOINT20_ID		0x20
#define TI_VIEWPOINT25_ID		0x25
#define TI_MODE_85_CONTROL		0xD5

// Standard reference frequency used by almost all the graphic card.
#define TI_REF_FREQ		14.318


#else // TRIO
// S3 graphic engine registers and masks definition.
// (see any standard s3 databook for more informations).
#define MM8180			0x8180
#define MM81C0			0x81c0
#define MM81C4			0x81c4
#define MM81C8			0x81c8
#define MM81F0			0x81f0
#define MM81F4			0x81f4
#define MM8200			0x8200

#define	SUBSYS_STAT		0x8504
#define	SUBSYS_CNTL		0x8504
#define	ADVFUNC_CNTL	0x850c
#define	CUR_Y			0x82e8
#define	CUR_X			0x86e8
#define	DESTY_AXSTP		0x8ae8
#define	DESTX_DIASTP	0x8ee8
#define	ERR_TERM		0x92e8
#define	MAJ_AXIS_PCNT	0x96e8
#define	GP_STAT			0x9ae8
#define	CMD				0x9ae8
#define	SHORT_STROKE	0x9ee8
#define	BKGD_COLOR		0xa2e8
#define	FRGD_COLOR		0xa6e8
#define	WRT_MASK		0xaae8
#define	RD_MASK			0xaee8
#define	COLOR_CMP		0xb2e8
#define	BKGD_MIX		0xb6e8
#define	FRGD_MIX		0xbae8
#define	RD_REG_DT		0xbee8
#define	PIX_TRANS		0xe2e8
#define	PIX_TRANS_EXT	0xe2ea

#define	DATA_REG		0xbee8
#define	MIN_AXIS_PCNT	0x0000
#define	SCISSORS_T		0x1000
#define	SCISSORS_L		0x2000
#define	SCISSORS_B		0x3000
#define	SCISSORS_R		0x4000
#define	PIX_CNTL		0xa000
#define	MULT_MISC2		0xd000
#define	MULT_MISC		0xe000
#define	READ_SEL		0xf000

// sequencer address and data registers
#define SEQ_INDEX		0x83c4
#define SEQ_DATA		0x83c5
#define SEQ_CLK_MODE	0x01

#define SCREEN_OFF		outb(SEQ_INDEX, 0x01);outb(SEQ_DATA, 0x21)
#define SCREEN_ON		outb(SEQ_INDEX, 0x01);outb(SEQ_DATA, 0x01)

// CRTC address and data registers
#define CRTC_INDEX		0x83d4
#define CRTC_DATA		0x83d5
#define INPUT_STATUS_1	0x83da
// Attribute controler
#define ATTR_REG		0x83c0
#define MISC_OUT_W		0x83c2
#define VGA_ENABLE		0x83c3
#define DAC_ADR_MASK	0x83c6
#define DAC_RD_INDEX	0x83c7
#define DAC_WR_INDEX	0x83c8
#define DAC_DATA		0x83c9
#define GCR_INDEX		0x83ce
#define GCR_DATA		0x83cf
#define MISC_OUT_R		0x83cc
// Standard reference frequency used by almost all the graphic card.
#define TI_REF_FREQ		14.318

// #include "/boot/atheos/s3_virge/virge.h"
#endif

//----------------------------------------------------------
// 864 table
//----------------------------------------------------------
#if 0
static uint16	vga_864_table[] =
{
	0x3c4,	0x0d,	0x3c5,	0x00,	// Screen saving

	0x3d4,	0x31,	0x3d5,	0x8c,	// Memory config
	0x3d4,	0x32,	0x3d5,	0x00,	// Backward Compatibility 1
	0x3d4,	0x33,	0x3d5,	0x02,	// Backward Compatibility 2
	0x3d4,	0x34,	0x3d5,	0x10,	// Backward compatibility 3
	0x3d4,	0x35,	0x3d5,	0x00,	// CRT Register Lock
	0x3d4,	0x37,	0x3d5,	0xfb,	// Configuration 2
	0x3d4,	0x3a,	0x3d5,	0x95,	// Miscellaneous 1
	0x3d4,	0x3c,	0x3d5,	0x40,	// Interlace Retrace Start

	0x3d4,	0x40,	0x3d5,	0xd1,	// (d1)System Configuration (?)
	0x3d4,	0x42,	0x3d5,	0x03,	// (02)Mode Control
	0x3d4,	0x43,	0x3d5,	0x00,	// Extended Mode
	0x3d4,	0x45,	0x3d5,	0x04,	// (00)Hardware Graphics Cursor Mode

	0x3d4,	0x52,	0x3d5,	0x00,	// Extended BIOS Flag 1
	0x3d4,	0x53,	0x3d5,	0x00,	// Extended Memory Cont 1
	0x3d4,	0x55,	0x3d5,	0x40,	// (00)Extended DAC Control
	0x3d4,	0x56,	0x3d5,	0x10,	// (00)External Sync Cont 1
	0x3d4,	0x57,	0x3d5,	0x00,	// External Sync Cont 2
	0x3d4,	0x5b,	0x3d5,	0x88, 	// (00)Extended BIOS Flag 2
	0x3d4,	0x5c,	0x3d5,	0x03, 	// General Out Port
	0x3d4,	0x5f,	0x3d5,	0x00,	// Bus Grant Termination Position
	0x3d4,	0x63,	0x3d5,	0x00,	// External Sync Delay Adjust High
	0x3d4,	0x64,	0x3d5,	0x00,	// Genlocking Adjustment
	0x3d4,	0x65,	0x3d5,	0x02, 	// (00)Extended Miscellaneous Control
	0x3d4,	0x66,	0x3d5,	0x00, 	// Extended Miscellaneous Control 1
	0x3d4,	0x6a,	0x3d5,	0x00,	// Extended System Control 4
	0x3d4,	0x6b,	0x3d5,	0x00,	// Extended BIOS flags 3
	0x3d4,	0x6c,	0x3d5,	0x00,	// Extended BIOS flags 4
	0x3d4,	0x6d,	0x3d5,	0x02,	// Extended Miscellaneous Control
	0x000,	0x00	           		// End-of-table
};
#endif
static uint16	vga_virge_table[] =
{
	0, 0
#if 0	
//	SEQ_INDEX,  0x09,   SEQ_DATA,  0x80,   // IO map disable
	SEQ_INDEX,  0x0a,   SEQ_DATA,  0xe0, //0x40,   // External bus request (was 0x80/0xc0)
	SEQ_INDEX,  0x0d,   SEQ_DATA,  0x00,   // Extended sequencer 1
	SEQ_INDEX,  0x14,   SEQ_DATA,  0x00,   // CLKSYN
	SEQ_INDEX,  0x18,   SEQ_DATA,  0x00,   // RAMDAC/CLKSYN

	CRTC_INDEX,	0x11,   CRTC_DATA,	0x0c,	// CRTC regs 0-7 unlocked, v_ret_end = 0x0c
	CRTC_INDEX,	0x31,	CRTC_DATA,	0x0c, //0x08,//0x8c,	// Memory config (0x08)
	CRTC_INDEX,	0x32,	CRTC_DATA,	0x00,	// all interrupt disenabled
	CRTC_INDEX,	0x33,	CRTC_DATA,	0x22,	// Backward Compatibility 2
	CRTC_INDEX,	0x34,	CRTC_DATA,	0x00,	// Backward compatibility 3
	CRTC_INDEX,	0x35,	CRTC_DATA,	0x00,	// CRT Register Lock

	CRTC_INDEX,	0x37,	CRTC_DATA,	0xff, /* want 0x19, bios has 0xff : bit 1 is magic :-(
											 Trio32/64 databook says bit 1 is 0 for test, 1 for
											 normal operation.  Must be a documentation error? */
	CRTC_INDEX,	0x66,	CRTC_DATA,	0x01, 	// Extended Miscellaneous Control 1 (enable)
	//?? 0x66 first?
	CRTC_INDEX,	0x3a,	CRTC_DATA,	0x15, //0x95,	// Miscellaneous 1 //95
	//CRTC_INDEX,	0x3c,	CRTC_DATA,	0x40,	// Interlace Retrace Start

	CRTC_INDEX,	0x42,	CRTC_DATA,	0x00,	// Mode Control (non interlace)
	CRTC_INDEX,	0x43,	CRTC_DATA,	0x00,	// Extended Mode
	CRTC_INDEX,	0x45,	CRTC_DATA,	0x00,	// Hardware Graphics Cursor Mode

	//CRTC_INDEX,	0x52,	CRTC_DATA,	0x00,	// Extended BIOS Flag 1
	CRTC_INDEX,	0x53,	CRTC_DATA,	0x08,	// Extended Memory Cont 1
	CRTC_INDEX,	0x54,	CRTC_DATA,	0x02,	// Extended Memory Cont 2
	CRTC_INDEX,	0x55,	CRTC_DATA,	0x00,	// Extended DAC Control
	CRTC_INDEX,	0x56,	CRTC_DATA,	0x00,	// External Sync Cont 1
	//CRTC_INDEX,	0x57,	CRTC_DATA,	0x00,	// External Sync Cont 2
	CRTC_INDEX,	0x5c,	CRTC_DATA,	0x00, 	// General Out Port
	CRTC_INDEX,	0x61,	CRTC_DATA,	0x00,	// Extended Memory Cont 4 pg 18-23
	CRTC_INDEX,	0x63,	CRTC_DATA,	0x00,	// External Sync Delay Adjust High
	CRTC_INDEX,	0x65,	CRTC_DATA,	0x00, 	// Extended Miscellaneous Control 0x24
	CRTC_INDEX,	0x6a,	CRTC_DATA,	0x00,	// Extended System Control 4
	CRTC_INDEX,	0x6b,	CRTC_DATA,	0x00,	// Extended BIOS flags 3
	CRTC_INDEX,	0x6c,	CRTC_DATA,	0x00,	// Extended BIOS flags 4
	0x000,	0x00			        // End-of-table
#endif	
};

// internal struct used to clone the add-on from server space to client space
/*
typedef struct {
    int	    theVGA;
    int     theDAC;
    int     theMem;
    int	    scrnRowByte;
    int	    scrnWidth;
    int	    scrnHeight;
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
    ulong   scrnResCode;
    int     scrnResNum;
    uchar   *scrnBufBase;
    long	scrnRes;
    ulong   available_spaces;
    int     hotpt_h;
    int     hotpt_v;
    short   lastCrtHT;
    short   lastCrtVT;
    uchar   *isa_IO;
    int     CursorMode;
} clone_info;
*/
#endif



#define	outpb( R, D )	outb_p( (D), (R) )
#define	outpw( R, D )	outw_p( (D), (R) )
#define	outpl( R, D )	outl_p( (D), (R) )







































void	EnableS3( void )
{
	outb_p( 0x38,0x3d4 );
	outb_p( 0x48,0x3d5 );

	outb_p( 0x39,0x3d4 );
	outb_p( 0xa5,0x3d5 );

	outb_p( 0x40,0x3d4 );
	outb_p( inb_p( 0x3d5 ) | 0x01 ,0x3d5 );



	outw_p( 0x0013, ADVFUNC_CNTL );

}

// All the other table settings are done with that very simple function.
static void	set_table(uint16 *ptr)
{
	uint16	p1;
	uint16	p2;

	while (TRUE) {
		p1 = *ptr++;
		p2 = *ptr++;
		if (p1 == 0 && p2 == 0) {
			return;
		}
		outpb(p1, p2);
	}
}


void	InitS3( void )
{
#if 0
	set_table(vga_864_table);
#else
	set_table( vga_virge_table );
#endif
}



void wait_vga_hw(void)
{
	while (inw_p(GP_STAT) & 0x200)
		/*** EMPTY ***/;

//		Delay(40);
}

long S3DrawLine_32( long   x1, long x2, long y1, long y2, uint color, bool useClip, short clipLeft, short clipTop, short clipRight, short clipBottom );

// Stroke a line in 32 bits mode. (see documentation for more informations)
// NB: this call will probably be used in future version for 16 bits line too.
long S3DrawLine_32( long   x1,         // Coordinates of the two extremities
			 long   y1,         //
			 long   x2,         //
			 long   y2,         //
			 uint color,      	// RGB color
			 bool   useClip,    // Clipping enabling
			 short  clipLeft,   // Definition of the cliping rectangle if
			 short  clipTop,    // cliping enabled
			 short  clipRight,  //
			 short  clipBottom) //
{
	short		command;
	short		min1, max1;
	long    abs_dx, abs_dy;

// This call is using the graphic engine and access video memory, so we need to
// get the lock graphic-engine benaphore.

//	lock_ge();

// Refer to any s3 chip databook (864, 964, trio32, trio64 and more) to get
// more information about specific graphic engine operation.
// The cliping rectangle is discribed with 12 bits values.
	if (useClip) {
		outpw(DATA_REG, SCISSORS_T | (clipTop & 0xfff));
		outpw(DATA_REG, SCISSORS_L | (clipLeft & 0xfff));
		outpw(DATA_REG, SCISSORS_B | (clipBottom & 0xfff));
		outpw(DATA_REG, SCISSORS_R | (clipRight & 0xfff));
	}
	else {
		outpw(DATA_REG, SCISSORS_T | 0);
		outpw(DATA_REG, SCISSORS_L | 0);
		outpw(DATA_REG, SCISSORS_B | 0x7ff);
		outpw(DATA_REG, SCISSORS_R | 0x7ff);
	}

// Standard comand for x and y negative directions, and x major axis.
	command = 0x2413;

// Check for positive y direction.
	if (y1 < y2)
		command |= 0x80;

// Calculate vector coordinates...
	abs_dx = abs(x2-x1);
	abs_dy = abs(y2-y1);

// ...and check orientation.
	if (abs_dx > abs_dy) {
		max1 = abs_dx;
		min1 = abs_dy;
	}
	else {
		max1 = abs_dy;
		min1 = abs_dx;
  // Command correction when y is the major axis.
		command |= 0x40;
	}

// Wait for 3 empty FIFO slots.
	if (inw_p(CMD) & 0x20)
		do {
		  //		Delay(40);
		} while (inw_p(CMD) & 0x20);

// Select mode code (copy), color and reset pixel control.
//
// NB : Be careful about color definition : the four channels (Blue, Gree, Red
// and Alpha) are put in memory in the order described in the rgba_order in
// graphics_card_info, and so the ulong you receive is ordered so that writting
// it into memory would put the bytes in the good order. So the order of the
// channels in the long depends of the endianness of the processor.
// For example, on the PPC Bebox, if rgba_order is "rgba", then the red channel
// is coded in the highest byte of the ulong (0x##000000), as we're using
// big-endian.
	outpw(FRGD_MIX, 0x27);
	outpl(FRGD_COLOR, color);
	outpw(DATA_REG, PIX_CNTL);

// Wait for 7 empty FIFO slots.
	if (inw_p(CMD) & 0x02)
		do {
		  //			Delay(40);
		} while (inw_p(CMD) & 0x02);

// Send geometric description of the line in graphic engine format
	outpw( CUR_X, x1 );
	outpw( CUR_Y, y1 );
	outpw( MAJ_AXIS_PCNT, max1 );
	outpw( DESTX_DIASTP, 2 * (min1 - max1));
	outpw( DESTY_AXSTP, 2 * min1);
	if ((x2 - x1) > 0) {
		outpw(ERR_TERM, 2 * (min1 - max1) );
  // Command correction when x direction is positive.
		command |= 0x20;
	}
	else
		outpw( ERR_TERM, 2 * (min1 - max1) - 1 );

// Run the operation
	outpw(CMD, command);

// Wait for terminaison (this operation is synchronous in the current
// implementation).
	wait_vga_hw();

// Release the benaphore of the graphic engine
//	unlock_ge();
//	return B_NO_ERROR;
	return( 0 );
}


// Fill a rect in 32 bits (see documentation for more informations)
void S3FillRect_32(long  x1,    // The rect to fill. Call will always respect
			 long  y1,    // x1 <= x2, y1 <= y2 and the rect will be
			 long  x2,    // completly in the current screen space (no
			 long  y2,    // cliping needed).
			 uint color) // Rgb color.
{
// This call is using the graphic engine and access video memory, so we need to
// get the lock graphic-engine benaphore.

//	lock_ge();

// Refer to any s3 chip databook (864, 964, trio32, trio64 and more) to get
// more information about specific graphic engine operation.
// Disable the cliping rectangle.
	outpw(DATA_REG, SCISSORS_T | 0);
	outpw(DATA_REG, SCISSORS_L | 0);
	outpw(DATA_REG, SCISSORS_B | 0x7ff);
	outpw(DATA_REG, SCISSORS_R | 0x7ff);

// Wait for 3 empty FIFO slots.
	if (inw_p(CMD) & 0x20)
		do {
		  //			Delay(40);
		} while (inw_p(CMD) & 0x20);

// Select mode code (copy), color and reset pixel control.
//
// NB : Be careful about color definition : the four channels (Blue, Gree, Red
// and Alpha) are put in memory in the order described in the rgba_order in
// graphics_card_info, and so the ulong you receive is ordered so that writting
// it into memory would put the bytes in the good order. So the order of the
// channels in the long depend of the endianness of the processor.
// For example, on the PPC Bebox, if rgba_order is "rgba", then the red channel
// is coded in the highest byte of the ulong (0x##000000), as we're using
// big-endian.
	outpw(FRGD_MIX, 0x27);
	outpl(FRGD_COLOR, color);
	outpw(DATA_REG, PIX_CNTL);

// Wait for 5 empty FIFO slots.
	if (inw_p(CMD) & 0x08)
		do {
		  //			Delay(40);
		} while (inw_p(CMD) & 0x08);

// Send geometric description of the rect in graphic engine format
	outpw(CUR_X, x1);
	outpw(CUR_Y, y1);
	outpw(MAJ_AXIS_PCNT, x2 - x1);
	outpw(DATA_REG, MIN_AXIS_PCNT | (y2 - y1));

// Run the operation
	outpw(CMD, 0x40b3);

// Wait for terminaison (this operation is synchronous in the current
// implementation).
	wait_vga_hw();

// Release the benaphore of the graphic engine
//	unlock_ge();
//	return B_NO_ERROR;
}


// Blit a rect from screen to screen (see documentation for more informations)
int S3Blit( int  x1,     // top-left point of the source
		  int  y1,     //
		  int  x2,     // top-left point of the destination
		  int  y2,     //
		  int  width,  // size of the rect to move (from border included to
		  int  height) // opposite border included).
{
	short	command;
	short	srcx, srcy, destx, desty;

// Check degenerated blit (source == destination)
	if ((x1 == x2) && (y1 == y2)) return( 0 );

// This call is using the graphic engine and access video memory, so we need to
// get the lock graphic-engine benaphore.
//	lock_ge();

// Refer to any s3 chip databook (864, 964, trio32, trio64 and more) to get
// more information about specific graphic engine operation.
// Standard command for negative x, negative y
	command = 0xc013;

// Convert application server width and height (as for BRect object) into
// real width and height

//	width += 1;
//	height += 1;

// Check if source and destination are not linked
	if ((x2 > (x1 + width)) ||
		((x2 + width) < x1) ||
		(y2 > (y1 + height) ||
		 (y2 + height) < y1)) {
		srcx = x1;
		srcy = y1;
		destx = x2;
		desty = y2;
  // Command correction when positive x and positive y.
		command |= 0xa0;
	}
// In the other case, copy in the correct order
	else {
		if (x1 > x2) {
  // Command correction when positive x.
			command |= 0x20;
			srcx = x1;
			destx = x2;
		}
		else {
			srcx = x1 + width - 1;
			destx = x2 + width - 1;
		}
		if (y1 > y2) {
  // Command correction when positive y.
			command |= 0x80;
			srcy = y1;
			desty = y2;
		}
		else {
			srcy = y1 + height - 1;
			desty = y2 + height - 1;
		}
	}

// Disable the cliping rectangle.
	outpw(DATA_REG, SCISSORS_T | 0);
	outpw(DATA_REG, SCISSORS_L | 0);
	outpw(DATA_REG, SCISSORS_B | 0x7ff);
	outpw(DATA_REG, SCISSORS_R | 0x7ff);

// Wait for 2 empty FIFO slots.

	int	c = 100000;
	if (inw_p( CMD ) & 0x40)
		do {
			if ( c-- < 0 ) break;
		  //			Delay( 40 );
		} while ( inw_p( CMD ) & 0x40);

// Select mode code (display memory source) and reset pixel control.
	outpw(FRGD_MIX, 0x67);
	outpw(DATA_REG, PIX_CNTL);

// Wait for 7 empty FIFO slots.
	c = 100000;
	if (inw_p(CMD) & 0x02)
		do {
		  //			Delay( 40 );
			if ( c-- < 0 ) break;
		} while (inw_p(CMD) & 0x02);

// Send geometric description of the blit in graphic engine format

	outpw( CUR_X, srcx );
	outpw( CUR_Y, srcy );
	outpw( DESTX_DIASTP, destx );
	outpw( DESTY_AXSTP, desty );
	outpw( MAJ_AXIS_PCNT, width - 1 );
	outpw( DATA_REG, MIN_AXIS_PCNT | ((height - 1) & 0xfff ) );

// Run the operation
	outpw( CMD, command );

// Wait for terminaison (this operation is synchronous in the current
// implementation). For big blit, the delay function is calling snooze to
// free some CPU time.

/*
	if ((width*height) > 25000)
		wait_vga_hw_big();
	else
		wait_vga_hw();
*/


	
//...........................	wait_vga_hw();




	

// Release the benaphore of the graphic engine
//	unlock_ge();
	return( 0 );
}







#if 0
//------------------ Virge -------------------------------------------------------------------------------------------

/*
#define v_inb(a)			inl_p( (a)  - 0x8000 )
#define v_outb(a, b)	outb_p( b, (a) - 0x8000 )
#define v_inw(a)			inw_p( (a)  - 0x8000 )			
#define v_outw(a, w)	outw_( w, (a)  - 0x8000 )
#define v_inl(a)			inl_p( (a)  - 0x8000 )
#define v_outl(a, l)	outl_p( l, (a)  - 0x8000 )
*/

#define v_inb(a)			inl_p( (a) )
#define v_outb(a, b)	outb_p( b, (a) )
#define v_inw(a)			inw_p( (a) )			
#define v_outw(a, w)	outw_( w, (a) )
#define v_inl(a)			inl_p( (a) )
#define v_outl(a, l)	outl_p( l, (a) )



void	EnableVirge()
{
#if 0	
//		set_pci(0x04, 4, 0x02000003); // enable ISA IO, enable MemMapped IO
//		isa_outb(VGA_ENABLE, 0x01); // pg 13-1
//		dprintf("VGA_ENABLE: 0x%02x\n", (int)isa_inb(VGA_ENABLE));
		v_outb(VGA_ENABLE, 0x01); // pg 13-1
//		dprintf("VGA_ENABLE: 0x%02x\n", (int)inb(VGA_ENABLE));
//		isa_outb(MISC_OUT_W, 0x0f);
//		dprintf("MISC_OUT_R: 0x%02x\n", (int)isa_inb(MISC_OUT_R));
		v_outb(MISC_OUT_W, 0x0f);
//		dprintf("MISC_OUT_R: 0x%02x\n", (int)inb(MISC_OUT_R));
//#if 1
//		uint8 t = isa_inb(MISC_OUT_R); // pg 13-1
		uint8 t = v_inb(MISC_OUT_R); // pg 13-1
//		isa_outb(DAC_ADR_MASK, 0xff);
		v_outb(DAC_ADR_MASK, 0xff);
//		isa_outb(SEQ_INDEX, 0x08); // pg 13-2 -- unlock extended Sequencer regs
//		isa_outb(SEQ_DATA,  0x06);
		v_outb(SEQ_INDEX, 0x08); // pg 13-2 -- unlock extended Sequencer regs
		v_outb(SEQ_DATA,  0x06);
//		isa_outb(CRTC_INDEX, 0x38); // pg 13-2 -- unlock extended CRTC 2D->3F
//		isa_outb(CRTC_DATA,  0x48);
		v_outb(CRTC_INDEX, 0x38); // pg 13-2 -- unlock extended CRTC 2D->3F
		v_outb(CRTC_DATA,  0x48);
//		isa_outb(CRTC_INDEX, 0x39); // pg 13-2 -- unlock extended CRTC 40->FF
//		isa_outb(CRTC_DATA,  0xa5);
		v_outb(CRTC_INDEX, 0x39); // pg 13-2 -- unlock extended CRTC 40->FF
		v_outb(CRTC_DATA,  0xa5);
//		isa_outb(CRTC_INDEX, 0x40); // pg 13-2 -- enable access to enhanced programming regs
//		t = isa_inb(CRTC_DATA);
//		isa_outb(CRTC_DATA,  t | 0x01);
		v_outb(CRTC_INDEX, 0x40); // pg 13-2 -- enable access to enhanced programming regs
		t = v_inb(CRTC_DATA);
		v_outb(CRTC_DATA,  t | 0x01);
//		isa_outb(CRTC_INDEX, 0x53);	// Extended memory control 1
//		isa_outb(CRTC_DATA,  0x08);
		v_outb(CRTC_INDEX, 0x53);	// Extended memory control 1
		v_outb(CRTC_DATA,  0x08);
#endif	
}



// Wait for n*0x100 empty FIFO slots.
static void wait_for_fifo(long value) {
//	if ((v_inl(SUBSYS_STAT) & 0x1f00) < value)
	{
			//do delay(40);

		int r;
		do {
			r = v_inl(SUBSYS_STAT );
			dbprintf( "SUBSYS_STAT = %04x\n", r );
		} while ((r & 0x1f00) < value);
		
//		while ((v_inl(SUBSYS_STAT) & 0x1f00) < value);
	}
}

static void wait_for_sync() {
	if ((v_inl(SUBSYS_STAT) & 0x2000) == 0) {
		//do delay(40);
		while ((v_inl(SUBSYS_STAT) & 0x2000) == 0);
	}
}


// Blit a rect from screen to screen (see documentation for more informations)
long v_blit(long  x1,     // top-left point of the source
		  long  y1,     //
		  long  x2,     // top-left point of the destination
		  long  y2,     //
		  long  width,  // size of the rect to move (from border included to
		  long  height) // opposite border included).
{
	uint32	command;

// check for zero height/width blits :-(
	//if ((height == 0) || (width == 0)) return B_NO_ERROR;

// Check degenerated blit (source == destination)
	if ((x1 == x2) && (y1 == y2)) return 0;

	//dprintf("blit: from %d,%d to %d,%d size %d,%d depth %d\n", x1,y1, x2,y2, width, height, ci.scrnColors);
// This call is using the graphic engine and access video memory, so we need to
// get the lock graphic-engine benaphore.
//...	lock_ge();

	command = 0x07980020;

	if (x1 < x2) {
		x1 += width;// - 1;
		x2 += width;// - 1;
		command &= ~0x02000000;
	}
	if (y1 < y2) {
		y1 += height;// - 1;
		y2 += height;// - 1;
		command &= ~0x04000000;
	}
	// 16 bit mode
#if 0	
	if (ci.scrnColors == 16) command |= 0x00000004;
#else
	command |= 0x00000004;
#endif
	
	wait_for_fifo(0x700);
	v_outl(0xa500, 0x78000000); // turn off auto-execute
	v_outl(0xa4d4, 0);	// source base addr
	v_outl(0xa4d8, 0);	// dest base addr
	
//	v_outl(0xa4e4, ci.scrnRowByte | (ci.scrnRowByte << 16));	// source/dest bytes per row
	v_outl(0xa4e4, 512 | (512 << 16));	// source/dest bytes per row
	
	v_outl(0xa504, ((width)<<16)|(height+1));
	v_outl(0xa508, (x1<<16)|y1);
	v_outl(0xa50c, (x2<<16)|y2);
	v_outl(0xa500, command);
	//dprintf("command was: 0x%08x\n", command);
	wait_for_sync();
	
// Release the benaphore of the graphic engine
//...	unlock_ge();
	return 0;
}
#endif
