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

#ifndef __F_MGA_REGS_H__
#define __F_MGA_REGS_H__

#include <atheos/types.h>

//#define M2_DEVCTRL					0x0004
#define M2_DWGCTL						0x1c00
#define M2_MACCESS					0x1c04
#define M2_PLNWT						0x1c1c
#define M2_BCOL						0x1c20
#define M2_FCOL						0x1c24
#define M2_XYSTRT						0x1c40
#define M2_XYEND						0x1c44
#define M2_SGN						0x1c58
#define M2_AR0						0x1c60
#define M2_AR3						0x1c6c
#define M2_AR5						0x1c74
#define M2_CXBNDRY					0x1c80	// Left/right clipping boundary
#define M2_FXBNDRY					0x1c84
#define M2_YDSTLEN					0x1c88
#define M2_PITCH						0x1c8c
#define M2_YDSTORG					0x1c94
#define M2_YTOP						0x1c98
#define M2_YBOT						0x1c9c

#define M2_STATUS						0x1e14

// Masks and values for the M2_DWGCTL register
#define M2_OPCODE_LINE_OPEN			0x00
#define M2_OPCODE_AUTOLINE_OPEN		0x01
#define M2_OPCODE_LINE_CLOSE		0x02
#define M2_OPCODE_AUTOLINE_CLOSE	0x03
#define M2_OPCODE_TRAP				0x04
#define M2_OPCODE_TRAP_LOAD			0x05
#define M2_OPCODE_BITBLT				0x08
#define M2_OPCODE_FBITBLT			0x0c
#define M2_OPCODE_ILOAD				0x09
#define M2_OPCODE_ILOAD_SCALE		0x0d
#define M2_OPCODE_ILOAD_FILTER		0x0f
#define M2_OPCODE_IDUMP				0x0a
#define M2_OPCODE_ILOAD_HIQH		0x07
#define M2_OPCODE_ILOAD_HIQHV		0x0e

#define M2_ATYPE_RPL					0x000	// Write (replace)
#define M2_ATYPE_RSTR				0x010	// Read-modify-write (raster)
#define M2_ATYPE_ZI					0x030	// Depth mode with Gouraud
#define M2_ATYPE_BLK					0x040	// Block write mode
#define M2_ATYPE_I					0x090	// Gouraud (with depth compare)

#define M2_ZMODE_NOZCMP				0x0000
#define M2_ZMODE_ZE					0x0200
#define M2_ZMODE_ZNE					0x0300
#define M2_ZMODE_ZLT					0x0400
#define M2_ZMODE_ZLTE				0x0500
#define M2_ZMODE_ZGT					0x0600
#define M2_ZMODE_ZGTE				0x0700

#define M2_SOLID						0x0800
#define M2_ARZERO						0x1000
#define M2_SGNZERO					0x2000
#define M2_SHFTZERO					0x4000

#define M2_BOP_MASK					0x0f0000
#define M2_TRANS_MASK				0xf00000

#define M2_BLTMOD_BMONOLEF			0x00000000
#define M2_BLTMOD_BMONOWF			0x80000000
#define M2_BLTMOD_BPLAN				0x02000000
#define M2_BLTMOD_BFCOL				0x04000000
#define M2_BLTMOD_BUYUV				0x1a000000
#define M2_BLTMOD_BU32BGR			0x06000000
#define M2_BLTMOD_BU32RGB			0x0e000000
#define M2_BLTMOD_BU24BGR			0x16000000
#define M2_BLTMOD_BU24RGB			0x1e000000

#define M2_PATTERN					0x20000000
#define M2_TRANSC						0x40000000

// Masks and values for the M2_SGN register:
#define M2_SGN_SCANLEFT				0x0001
#define M2_SGN_SDXL					0x0002
#define M2_SGN_SDY					0x0004
#define M2_SGN_SDXR					0x0010

// Bit masks for M2_STATUS
#define M2_PICKPEN					0x00004
#define M2_VSYNCSTS					0x00008
#define M2_VSYNCPEN					0x00010
#define M2_VLINEPEN					0x00020
#define M2_EXTPEN						0x00040
#define M2_DWGENGSTS					0x10000

// CRTC registers
#define MGA_MISC						0x1fc2
#define MGA_SEQI						0x1fc4
#define MGA_SEQD						0x1fc5
#define MGA_GCTLI						0x1fce
#define MGA_GCTLD						0x1fcf
#define MGA_CRTCI						0x1fd4
#define MGA_CRTCD						0x1fd5
#define MGA_INSTS1					0x1fda
#define MGA_CRTCEXTI					0x1fde
#define MGA_CRTCEXTD					0X1fdf

// Gx00 DAC registers
#define MGA_PALWTADD					0x00
#define MGA_PALDATA					0x01
#define MGA_XDATA						0x0a
#define MGA_MISCCTRL					0x1e
#define MGA_MULCTRL					0x19

// TI TVP3026 DAC registers
#define TVP3026_INDEX				0x00
#define TVP3026_WADR_PAL				0x00
#define TVP3026_CUR_COL_ADDR		0x04
#define TVP3026_CUR_COL_DATA		0x05
#define TVP3026_CURSOR_CTL			0x06
#define TVP3026_DATA					0x0a
#define TVP3026_CUR_XLOW				0x0c
#define TVP3026_CUR_RAM				0x0b
#define TVP3026_CUR_XHI				0x0d
#define TVP3026_CUR_YLOW				0x0e
#define TVP3026_CUR_YHI				0x0f	// Hmmm,
#define TVP3026_LATCHCTRL			0x0f	// are these correct?
#define TVP3026_CLK_SEL				0x1a
#define TVP3026_MISCCTRL				0x1e
#define TVP3026_TCOLCTRL				0x18
#define TVP3026_MULCTRL				0x19
#define TVP3026_PLL_ADDR				0x2c
#define TVP3026_PIX_CLK_DATA		0x2d
#define TVP3026_MEM_CLK_DATA		0x2e
#define TVP3026_LOAD_CLK_DATA		0x2f
#define TVP3026_MCLK_CTL				0x39

enum MGADACType
{
	NONE,
	TVP3026,
	MGAGx00,
	MGAGx50
};

struct MGAChip_s
{
	uint16 nDeviceID;
	char *zName;
	MGADACType eDAC;
};

#define MAXCHIPS	19

#define MGA_REG_WINDOW	1024 * 16	// 16k memory area for the MGA registers

#endif	// __F_MGA_REGS_H__
