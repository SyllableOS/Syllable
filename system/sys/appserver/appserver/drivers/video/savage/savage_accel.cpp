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

#include <savage_driver.h>
#include <savage_bci.h>
#include <savage_streams.h>

#include <gui/bitmap.h>

#include "../../../server/bitmap.h"

using namespace os;
using namespace std;

void SavageDriver::SavageInitialize2DEngine( savage_s *psCard )
{
	int vgaIOBase, vgaCRIndex, vgaCRReg;

	/* XXXKV: Need to know value of vgaIOBase; probably 0x3d0 (!) */
	vgaIOBase = 0x3d0;
	vgaCRIndex = vgaIOBase + 4;
	vgaCRReg = vgaIOBase + 5;

	VGAOUT16(vgaCRIndex, 0x0140);
	VGAOUT8(vgaCRIndex, 0x31);
	VGAOUT8(vgaCRReg, 0x0c);

	/* Setup plane masks */
	OUTREG(0x8128, ~0); /* enable all write planes */
	OUTREG(0x812C, ~0); /* enable all read planes */
	OUTREG16(0x8134, 0x27);
	OUTREG16(0x8136, 0x07);

	switch( psCard->eChip )
	{
		case S3_SAVAGE3D:
		case S3_SAVAGE_MX:
		{
			/* Disable BCI */
			OUTREG(0x48C18, INREG(0x48C18) & 0x3FF0);
			/* Setup BCI command overflow buffer */
			OUTREG(0x48C14, (psCard->cobOffset >> 11) | (psCard->cobIndex << 29)); /* tim */
			/*OUTREG(S3_OVERFLOW_BUFFER, psCard->cobOffset >> 11 | 0xE0000000);*/ /* S3 */
			/* Program shadow status update. */
			{
				unsigned long thresholds = ((psCard->bciThresholdLo & 0xffff) << 16) | (psCard->bciThresholdHi & 0xffff);
				OUTREG(0x48C10, thresholds);
				/* used to be 0x78207220 */
			}

			OUTREG(0x48C0C, 0);
			/* Enable BCI and command overflow buffer */
			OUTREG(0x48C18, INREG(0x48C18) | 0x0C);

			break;
		}

		case S3_SAVAGE4:
		case S3_TWISTER:
		case S3_PROSAVAGE:
		case S3_PROSAVAGEDDR:
		case S3_SUPERSAVAGE:
		{
			/* Disable BCI */
			OUTREG(0x48C18, INREG(0x48C18) & 0x3FF0);
			if( m_bDisableCOB == false )
			{
				/* Setup BCI command overflow buffer */
				OUTREG(0x48C14, (psCard->cobOffset >> 11) | (psCard->cobIndex << 29));
			}
			/* Program shadow status update */ /* AGD: what should this be? */
			{
				unsigned long thresholds = ((psCard->bciThresholdLo & 0x1fffe0) << 11) | ((psCard->bciThresholdHi & 0x1fffe0) >> 5);
				OUTREG(0x48C10, thresholds);
			}

			/*OUTREG(0x48C10, 0x00700040);*/ /* tim */
			/*OUTREG(0x48C10, 0x0e440f04L);*/ /* S3 */

			OUTREG(0x48C0C, 0);
			if( m_bDisableCOB )
				OUTREG(0x48C18, INREG(0x48C18) | 0x08);	/* Enable BCI without the COB */
			else
				OUTREG32(0x48C18, INREG32(0x48C18) | 0x0C);

			break;
		}

		case S3_SAVAGE2000:
		{
			/* Disable BCI */
			OUTREG(0x48C18, 0);
			/* Setup BCI command overflow buffer */
			OUTREG(0x48C18, (psCard->cobOffset >> 7) | (psCard->cobIndex));

			/* Disable shadow status update */
			OUTREG(0x48A30, 0);
			/* Enable BCI and command overflow buffer */
			OUTREG(0x48C18, INREG(0x48C18) | 0x00280000 );

			break;
		}

		default:
			break;
	}

 	/* Use and set global bitmap descriptor. */

	/* For reasons I do not fully understand yet, on the Savage4, the
	 * write to the GBD register, MM816C, does not "take" at this time.
	 * Only the low-order byte is acknowledged, resulting in an incorrect
	 * stride.  Writing the register later, after the mode switch, works
	 * correctly.  This needs to get resolved. */

	SavageSetGBD( psCard );
}

void SavageDriver::SavageSetGBD( savage_s *psCard )
{
	UnProtectCRTC();
	UnLockExtRegs();
	VerticalRetraceWait();

	psCard->scrn.lDelta = psCard->scrn.Width * (psCard->scrn.Bpp >> 3);

	switch( psCard->eChip )
	{
		case S3_SAVAGE3D:
		{
			SavageSetGBD_3D( psCard );
			break;
		}
		case S3_SAVAGE_MX:
		{
			SavageSetGBD_M7( psCard );            
			break;
		}
		case S3_SAVAGE4:
		case S3_TWISTER:
		case S3_PROSAVAGE:            
		case S3_PROSAVAGEDDR:
		{
			SavageSetGBD_Twister( psCard );
			break;
		}
		case S3_SUPERSAVAGE:
		{
			SavageSetGBD_PM( psCard );
			break;
		}
		case S3_SAVAGE2000:
		{
			SavageSetGBD_2000( psCard );
			break;
		}
		default:
			break;
	}
}

void SavageDriver::SavageSetGBD_3D( savage_s *psCard )
{
	unsigned long ulTmp;
	unsigned char byte;
	int bci_enable, tile16, tile32;

	bci_enable = BCI_ENABLE;
	tile16 = TILE_FORMAT_16BPP;
	tile32 = TILE_FORMAT_32BPP;

	/* MM81C0 and 81C4 are used to control primary stream. */
	OUTREG32(PSTREAM_FBADDR0_REG,0x00000000);
	OUTREG32(PSTREAM_FBADDR1_REG,0x00000000);

	/*
	 *  Program Primary Stream Stride Register.
	 *
	 *  Tell engine if tiling on or off, set primary stream stride, and
	 *  if tiling, set tiling bits/pixel and primary stream tile offset.
	 *  Note that tile offset (bits 16 - 29) must be scanline width in
	 *  bytes/128bytespertile * 256 Qwords/tile.  This is equivalent to
	 *  lDelta * 2.  Remember that if tiling, lDelta is screenwidth in
	 *  bytes padded up to an even number of tilewidths.
	 */
	OUTREG32(PSTREAM_STRIDE_REG,
			(((psCard->scrn.lDelta * 2) << 16) & 0x3FFFE000) |
			(psCard->scrn.lDelta & 0x00001fff));

	/*
	 *  CR69, bit 7 = 1
	 *  to use MM streams processor registers to control primary stream.
	 */
	OUTREG8(CRT_ADDRESS_REG,0x69);
	byte = INREG8(CRT_DATA_REG) | 0x80;
	OUTREG8(CRT_DATA_REG,byte);

	OUTREG32(0x8128, 0xFFFFFFFFL);
	OUTREG32(0x812C, 0xFFFFFFFFL);

	OUTREG32(S3_BCI_GLB_BD_HIGH, bci_enable | S3_LITTLE_ENDIAN | S3_BD64);

	/* CR50, bit 7,6,0 = 111, Use GBD.*/
	OUTREG8(CRT_ADDRESS_REG,0x50);
	byte = INREG8(CRT_DATA_REG) | 0xC1;
	OUTREG8(CRT_DATA_REG, byte);

	/*
	 * if MS1NB style linear tiling mode.
	 * bit MM850C[15] = 0 select NB linear tile mode.
	 * bit MM850C[15] = 1 select MS-1 128-bit non-linear tile mode.
	 */
	ulTmp = INREG32(ADVANCED_FUNC_CTRL) | 0x8000; /* use MS-s style tile mode*/
	OUTREG32(ADVANCED_FUNC_CTRL,ulTmp);

	/*
	 * Tiled Surface 0 Registers MM48C40:
	 *  bit 0~23: tile surface 0 frame buffer offset
	 *  bit 24~29:tile surface 0 width
	 *  bit 30~31:tile surface 0 bits/pixel
	 *            00: reserved
	 *            01, 8 bits
	 *            10, 16 Bits.
	 *            11, 32 Bits.
	 */
	/*
	 * Global Bitmap Descriptor Register MM816C
	 *   bit 24~25: tile format
	 *          00: linear 
	 *          01: reserved
	 *          10: 16 bpp tiles
	 *          11: 32 bpp tiles
	 *   bit 28: block write disable/enable
	 *          0: enable
	 *          1: disable
	 */

	/*
	 *  Do not enable block_write even for non-tiling modes, because
	 *  the driver cannot determine if the memory type is the certain
	 *  type of SGRAM for which block_write can be used.
	 */
	psCard->GlobalBD.bd1.HighPart.ResBWTile = TILE_FORMAT_LINEAR;/* linear */
        
	ulTmp = ( ((psCard->scrn.Width + 0x1F) & 0x0000FFE0) >> 5) << 24;
	OUTREG32(TILED_SURFACE_REGISTER_0,ulTmp | TILED_SURF_BPP32);

	psCard->GlobalBD.bd1.HighPart.ResBWTile |= 0x10;/* disable block write - was 0 */
	/* HW uses width */
	psCard->GlobalBD.bd1.HighPart.Stride = (unsigned short) psCard->scrn.lDelta / (psCard->scrn.Bpp >> 3);
	psCard->GlobalBD.bd1.HighPart.Bpp = (unsigned char) (psCard->scrn.Bpp);
	psCard->GlobalBD.bd1.Offset = 0;

	/*
	 * CR88, bit 4 - Block write enabled/disabled.
	 *
	 *      Note: Block write must be disabled when writing to tiled
	 *            memory.  Even when writing to non-tiled memory, block
	 *            write should only be enabled for certain types of SGRAM.
	 */
	OUTREG8(CRT_ADDRESS_REG,0x88);
	byte = INREG8(CRT_DATA_REG) | DISABLE_BLOCK_WRITE_2D;
	OUTREG8(CRT_DATA_REG,byte);

	/*
	 * CR31, bit 0 = 0, Disable address offset bits(CR6A_6-0).
	 *       bit 0 = 1, Enable 8 Mbytes of display memory thru 64K window
	 *                  at A000:0.
	 */
	OUTREG8(CRT_ADDRESS_REG,MEMORY_CONFIG_REG); /* cr31 */
	byte = INREG8(CRT_DATA_REG) & (~(ENABLE_CPUA_BASE_A0000));
	OUTREG8(CRT_DATA_REG,byte); /* perhaps this should be 0x0c */

	/* turn on screen */
	OUTREG8(SEQ_ADDRESS_REG,0x01);
	byte = INREG8(SEQ_DATA_REG) & ~0x20;
	OUTREG8(SEQ_DATA_REG,byte);
    
	/* program the GBD and SBD's */
	OUTREG32(S3_GLB_BD_LOW,psCard->GlobalBD.bd2.LoPart);
	OUTREG32(S3_GLB_BD_HIGH,psCard->GlobalBD.bd2.HiPart | bci_enable | S3_LITTLE_ENDIAN | S3_BD64);
	OUTREG32(S3_PRI_BD_LOW,psCard->GlobalBD.bd2.LoPart);
	OUTREG32(S3_PRI_BD_HIGH,psCard->GlobalBD.bd2.HiPart);
	OUTREG32(S3_SEC_BD_LOW,psCard->GlobalBD.bd2.LoPart);
	OUTREG32(S3_SEC_BD_HIGH,psCard->GlobalBD.bd2.HiPart);
}

void SavageDriver::SavageSetGBD_M7( savage_s *psCard )
{
	int vgaIOBase, vgaCRIndex, vgaCRReg;

	/* XXXKV: Need to know value of vgaIOBase; probably 0x3d0 (!) */
	vgaIOBase = 0x3d0;
	vgaCRIndex = vgaIOBase + 4;
	vgaCRReg = vgaIOBase + 5;

	/* Called EnableMode_M7 in the old driver */
	unsigned char byte;
	int bci_enable, tile16, tile32;

	bci_enable = BCI_ENABLE;
	tile16 = TILE_FORMAT_16BPP;
	tile32 = TILE_FORMAT_32BPP;

    /* following is the enable case */

	/* SR01:turn off screen */
	OUTREG8 (SEQ_ADDRESS_REG,0x01);
	byte = INREG8(SEQ_DATA_REG) | 0x20;
	OUTREG8(SEQ_DATA_REG,byte);

	/*
	 * CR67_3:
	 *  = 1  stream processor MMIO address and stride register
	 *       are used to control the primary stream
	 *  = 0  standard VGA address and stride registers
	 *       are used to control the primary streams
	 */
	OUTREG8(CRT_ADDRESS_REG,0x67); 
	byte =  INREG8(CRT_DATA_REG) | 0x08;
	OUTREG8(CRT_DATA_REG,byte);

	/* IGA 2 */
	OUTREG16(SEQ_ADDRESS_REG,SELECT_IGA2_READS_WRITES);
	OUTREG8(CRT_ADDRESS_REG,0x67); 
	byte =  INREG8(CRT_DATA_REG) | 0x08;
	OUTREG8(CRT_DATA_REG,byte);             
	OUTREG16(SEQ_ADDRESS_REG,SELECT_IGA1);

	/* Set primary stream to bank 0 */
	OUTREG8(CRT_ADDRESS_REG, MEMORY_CTRL0_REG);/* CRCA */
	byte =  INREG8(CRT_DATA_REG) & ~(MEM_PS1 + MEM_PS2) ;
	OUTREG8(CRT_DATA_REG,byte);

	/* MM81C0 and 81C4 are used to control primary stream. */
	OUTREG32(PRI_STREAM_FBUF_ADDR0, 0x0 );
	OUTREG32(PRI_STREAM_FBUF_ADDR1, 0x0 );
	OUTREG32(PRI_STREAM2_FBUF_ADDR0, 0x0 );
	OUTREG32(PRI_STREAM2_FBUF_ADDR1, 0x0 );
 
	/*
	 *  Program Primary Stream Stride Register.
	 *
	 *  Tell engine if tiling on or off, set primary stream stride, and
	 *  if tiling, set tiling bits/pixel and primary stream tile offset.
	 *  Note that tile offset (bits 16 - 29) must be scanline width in
	 *  bytes/128bytespertile * 256 Qwords/tile.  This is equivalent to
	 *  lDelta * 2.  Remember that if tiling, lDelta is screenwidth in
	 *  bytes padded up to an even number of tilewidths.
	 */

	OUTREG32(PRI_STREAM_STRIDE,
			(((psCard->scrn.lDelta * 2) << 16) & 0x3FFF0000) |
			(psCard->scrn.lDelta & 0x00003fff));
	OUTREG32(PRI_STREAM2_STRIDE,
			(((psCard->scrn.lDelta * 2) << 16) & 0x3FFF0000) |
			(psCard->scrn.lDelta & 0x00003fff));

	OUTREG32(0x8128, 0xFFFFFFFFL);
	OUTREG32(0x812C, 0xFFFFFFFFL);

	OUTREG32(S3_BCI_GLB_BD_HIGH, bci_enable | S3_LITTLE_ENDIAN | S3_BD64);

	/* CR50, bit 7,6,0 = 111, Use GBD.*/
	OUTREG8(CRT_ADDRESS_REG,0x50);
	byte = INREG8(CRT_DATA_REG) | 0xC1;
	OUTREG8(CRT_DATA_REG, byte);

	/*
	 * CR78, bit 3  - Block write enabled(1)/disabled(0).
	 *       bit 2  - Block write cycle time(0:2 cycles,1: 1 cycle)
	 *      Note: Block write must be disabled when writing to tiled
	 *            memory.  Even when writing to non-tiled memory, block
	 *            write should only be enabled for certain types of SGRAM.
	 */
	OUTREG8(CRT_ADDRESS_REG,0x78);
	byte = INREG8(CRT_DATA_REG) | 0xfb;
	OUTREG8(CRT_DATA_REG,byte);

	/*
	 * Tiled Surface 0 Registers MM48C40:
	 *  bit 0~23: tile surface 0 frame buffer offset
	 *  bit 24~29:tile surface 0 width
	 *  bit 30~31:tile surface 0 bits/pixel
	 *            00: reserved
	 *            01, 8 bits
	 *            10, 16 Bits.
	 *            11, 32 Bits.
	 */
	/*
	 * Global Bitmap Descriptor Register MM816C
	 *   bit 24~25: tile format
	 *          00: linear
	 *          01: reserved
	 *          10: 16 bit
	 *          11: 32 bit
	 *   bit 28: block write disble/enable
	 *          0: enable
	 *          1: disable
	 */

	/*
	 *  Do not enable block_write even for non-tiling modes, because
	 *  the driver cannot determine if the memory type is the certain
	 *  type of SGRAM for which block_write can be used.
	 */
	psCard->GlobalBD.bd1.HighPart.ResBWTile = TILE_FORMAT_LINEAR;/* linear */

	psCard->GlobalBD.bd1.HighPart.ResBWTile |= 0x10;/* disable block write */
	/* HW uses width */
	psCard->GlobalBD.bd1.HighPart.Stride = (unsigned short)(psCard->scrn.lDelta / (psCard->scrn.Bpp >> 3));
	psCard->GlobalBD.bd1.HighPart.Bpp = (unsigned char)(psCard->scrn.Bpp);
	psCard->GlobalBD.bd1.Offset = 0;    

	/*
	 * CR31, bit 0 = 0, Disable address offset bits(CR6A_6-0).
	 *       bit 0 = 1, Enable 8 Mbytes of display memory thru 64K window
	 *                  at A000:0.
	 */
	OUTREG8(CRT_ADDRESS_REG,MEMORY_CONFIG_REG); /* cr31 */
	byte = (INREG8(CRT_DATA_REG) | 0x04) & 0xFE;
	OUTREG8(CRT_DATA_REG,byte);

	/* program the GBD and SBD's */
	OUTREG32(S3_GLB_BD_LOW,psCard->GlobalBD.bd2.LoPart );
	/* 8: bci enable */
	OUTREG32(S3_GLB_BD_HIGH,(psCard->GlobalBD.bd2.HiPart
                             | bci_enable | S3_LITTLE_ENDIAN | S3_BD64));
	OUTREG32(S3_PRI_BD_LOW,psCard->GlobalBD.bd2.LoPart);
	OUTREG32(S3_PRI_BD_HIGH,psCard->GlobalBD.bd2.HiPart);
	OUTREG32(S3_SEC_BD_LOW,psCard->GlobalBD.bd2.LoPart);
	OUTREG32(S3_SEC_BD_HIGH,psCard->GlobalBD.bd2.HiPart);

	/* turn on screen */
	OUTREG8(SEQ_ADDRESS_REG,0x01);
	byte = INREG8(SEQ_DATA_REG) & ~0X20;
	OUTREG8(SEQ_DATA_REG,byte);
}

void SavageDriver::SavageSetGBD_Twister( savage_s *psCard )
{
    unsigned long ulTmp;
    unsigned char byte;
    int bci_enable, tile16, tile32;

	if(psCard->eChip == S3_SAVAGE4)
	{
		bci_enable = BCI_ENABLE;
		tile16 = TILE_FORMAT_16BPP;
		tile32 = TILE_FORMAT_32BPP;
	}
	else
	{
		bci_enable = BCI_ENABLE_TWISTER;
		tile16 = TILE_DESTINATION;
		tile32 = TILE_DESTINATION;
	}
    
	/* MM81C0 and 81C4 are used to control primary stream. */
	OUTREG32(PSTREAM_FBADDR0_REG,0x00000000);
	OUTREG32(PSTREAM_FBADDR1_REG,0x00000000);

	/*
	 *  Program Primary Stream Stride Register.
	 *
	 *  Tell engine if tiling on or off, set primary stream stride, and
	 *  if tiling, set tiling bits/pixel and primary stream tile offset.
	 *  Note that tile offset (bits 16 - 29) must be scanline width in
	 *  bytes/128bytespertile * 256 Qwords/tile.  This is equivalent to
	 *  lDelta * 2.  Remember that if tiling, lDelta is screenwidth in
	 *  bytes padded up to an even number of tilewidths.
	 */
	OUTREG32(PSTREAM_STRIDE_REG,
			(((psCard->scrn.lDelta * 2) << 16) & 0x3FFFE000) |
			(psCard->scrn.lDelta & 0x00001fff));

	/*
	 *  CR69, bit 7 = 1
	 *  to use MM streams processor registers to control primary stream.
	 */
	OUTREG8(CRT_ADDRESS_REG,0x69);
	byte = INREG8(CRT_DATA_REG) | 0x80;
	OUTREG8(CRT_DATA_REG,byte);

	OUTREG32(0x8128, 0xFFFFFFFFL);
	OUTREG32(0x812C, 0xFFFFFFFFL);

	OUTREG32(S3_BCI_GLB_BD_HIGH, bci_enable | S3_LITTLE_ENDIAN | S3_BD64);

	/* CR50, bit 7,6,0 = 111, Use GBD.*/
	OUTREG8(CRT_ADDRESS_REG,0x50);
	byte = INREG8(CRT_DATA_REG) | 0xC1;
	OUTREG8(CRT_DATA_REG, byte);

	/*
	 * if MS1NB style linear tiling mode.
	 * bit MM850C[15] = 0 select NB linear tile mode.
	 * bit MM850C[15] = 1 select MS-1 128-bit non-linear tile mode.
	 */
	ulTmp = INREG32(ADVANCED_FUNC_CTRL) | 0x8000; /* use MS-s style tile mode*/
	OUTREG32(ADVANCED_FUNC_CTRL,ulTmp);

	/*
	 * Set up Tiled Surface Registers
	 *  Bit 25:20 - Surface width in tiles.
	 *  Bit 29 - Y Range Flag.
	 *  Bit 31:30 = 00, 4 bpp.
	 *            = 01, 8 bpp.
	 *            = 10, 16 bpp.
	 *            = 11, 32 bpp.
	 */
	/*
	 * Global Bitmap Descriptor Register MM816C - twister/prosavage
	 *   bit 24~25: tile format
	 *          00: linear 
	 *          01: destination tiling format
	 *          10: texture tiling format
	 *          11: reserved
	 *   bit 28: block write disble/enable
	 *          0: disable
	 *          1: enable
	 */
	/*
	 * Global Bitmap Descriptor Register MM816C - savage4
	 *   bit 24~25: tile format
	 *          00: linear 
	 *          01: reserved
	 *          10: 16 bpp tiles
	 *          11: 32 bpp tiles
	 *   bit 28: block write disable/enable
	 *          0: enable
	 *          1: disable
	 */
	/*
	 *  Do not enable block_write even for non-tiling modes, because
	 *  the driver cannot determine if the memory type is the certain
	 *  type of SGRAM for which block_write can be used.
	 */
	psCard->GlobalBD.bd1.HighPart.ResBWTile = TILE_FORMAT_LINEAR;/* linear */

	psCard->GlobalBD.bd1.HighPart.ResBWTile |= 0x10;/* disable block write - was 0 */
	/* HW uses width */
	psCard->GlobalBD.bd1.HighPart.Stride = (unsigned short) psCard->scrn.lDelta / (psCard->scrn.Bpp >> 3);
	psCard->GlobalBD.bd1.HighPart.Bpp = (unsigned char) (psCard->scrn.Bpp);
	psCard->GlobalBD.bd1.Offset = 0;

	/*
	 * CR88, bit 4 - Block write enabled/disabled.
	 *
	 *      Note: Block write must be disabled when writing to tiled
	 *            memory.  Even when writing to non-tiled memory, block
	 *            write should only be enabled for certain types of SGRAM.
	 */
	OUTREG8(CRT_ADDRESS_REG,0x88);
	byte = INREG8(CRT_DATA_REG) | DISABLE_BLOCK_WRITE_2D;
	OUTREG8(CRT_DATA_REG,byte);

	/*
	 * CR31, bit 0 = 0, Disable address offset bits(CR6A_6-0).
	 *       bit 0 = 1, Enable 8 Mbytes of display memory thru 64K window
	 *                  at A000:0.
	 */
	OUTREG8(CRT_ADDRESS_REG,MEMORY_CONFIG_REG); /* cr31 */
	byte = INREG8(CRT_DATA_REG) & (~(ENABLE_CPUA_BASE_A0000));
	OUTREG8(CRT_DATA_REG,byte); /* perhaps this should be 0x0c */

	/* turn on screen */
	OUTREG8(SEQ_ADDRESS_REG,0x01);
	byte = INREG8(SEQ_DATA_REG) & ~0x20;
	OUTREG8(SEQ_DATA_REG,byte);

	/* program the GBD and SBD's */
	OUTREG32(S3_GLB_BD_LOW,psCard->GlobalBD.bd2.LoPart);
	OUTREG32(S3_GLB_BD_HIGH,psCard->GlobalBD.bd2.HiPart | bci_enable | S3_LITTLE_ENDIAN | S3_BD64);
	OUTREG32(S3_PRI_BD_LOW,psCard->GlobalBD.bd2.LoPart);
	OUTREG32(S3_PRI_BD_HIGH,psCard->GlobalBD.bd2.HiPart);
	OUTREG32(S3_SEC_BD_LOW,psCard->GlobalBD.bd2.LoPart);
	OUTREG32(S3_SEC_BD_HIGH,psCard->GlobalBD.bd2.HiPart);
}

void SavageDriver::SavageSetGBD_PM( savage_s *psCard )
{
	unsigned char byte;
	int bci_enable, tile16, tile32;

	bci_enable = BCI_ENABLE_TWISTER;
	tile16 = TILE_DESTINATION;
	tile32 = TILE_DESTINATION;

	/* following is the enable case */

	/* SR01:turn off screen */
	OUTREG8 (SEQ_ADDRESS_REG,0x01);
	byte = INREG8(SEQ_DATA_REG) | 0x20;
	OUTREG8(SEQ_DATA_REG,byte);

	/*
	 * CR67_3:
	 *  = 1  stream processor MMIO address and stride register
	 *       are used to control the primary stream
	 *  = 0  standard VGA address and stride registers
	 *       are used to control the primary streams
	 */
	OUTREG8(CRT_ADDRESS_REG,0x67); 
	byte =  INREG8(CRT_DATA_REG) | 0x08;
	OUTREG8(CRT_DATA_REG,byte);
	/* IGA 2 */
	OUTREG16(SEQ_ADDRESS_REG,SELECT_IGA2_READS_WRITES);

	OUTREG8(CRT_ADDRESS_REG,0x67); 
	byte =  INREG8(CRT_DATA_REG) | 0x08;
	OUTREG8(CRT_DATA_REG,byte);

	OUTREG16(SEQ_ADDRESS_REG,SELECT_IGA1);

	/*
	 * load ps1 active registers as determined by MM81C0/81C4
	 * load ps2 active registers as determined by MM81B0/81B4
	 */
	OUTREG8(CRT_ADDRESS_REG,0x65); 
	byte =  INREG8(CRT_DATA_REG) | 0x03;
	OUTREG8(CRT_DATA_REG,byte);

	/*
	 *  Program Primary Stream Stride Register.
	 *
	 *  Tell engine if tiling on or off, set primary stream stride, and
	 *  if tiling, set tiling bits/pixel and primary stream tile offset.
	 *  Note that tile offset (bits 16 - 29) must be scanline width in
	 *  bytes/128bytespertile * 256 Qwords/tile.  This is equivalent to
	 *  lDelta * 2.  Remember that if tiling, lDelta is screenwidth in
	 *  bytes padded up to an even number of tilewidths.
	 */
	OUTREG32(PRI_STREAM_STRIDE,
			(((psCard->scrn.lDelta * 2) << 16) & 0x3FFF0000) |
			(psCard->scrn.lDelta & 0x00001fff));
	OUTREG32(PRI_STREAM2_STRIDE,
			(((psCard->scrn.lDelta * 2) << 16) & 0x3FFF0000) |
			(psCard->scrn.lDelta & 0x00001fff));

	/* MM81C0 and 81C4 are used to control primary stream. */
	OUTREG32(PRI_STREAM_FBUF_ADDR0,0);
	OUTREG32(PRI_STREAM_FBUF_ADDR1,0x80000000);
	OUTREG32(PRI_STREAM2_FBUF_ADDR0,0x80000000);
	OUTREG32(PRI_STREAM2_FBUF_ADDR1,0);

	OUTREG32(0x8128, 0xFFFFFFFFL);
	OUTREG32(0x812C, 0xFFFFFFFFL);

	/* bit 28:block write disable */
	OUTREG32(S3_GLB_BD_HIGH, bci_enable | S3_BD64 | 0x10000000); 

	/* CR50, bit 7,6,0 = 111, Use GBD.*/
	OUTREG8(CRT_ADDRESS_REG,0x50);
	byte = INREG8(CRT_DATA_REG) | 0xC1;
	OUTREG8(CRT_DATA_REG, byte);

	/*
	 *  Do not enable block_write even for non-tiling modes, because
	 *  the driver cannot determine if the memory type is the certain
	 *  type of SGRAM for which block_write can be used.
	 */
	psCard->GlobalBD.bd1.HighPart.ResBWTile = TILE_FORMAT_LINEAR;/* linear */

	psCard->GlobalBD.bd1.HighPart.ResBWTile |= 0x10;/* disable block write */
	/* HW uses width */
	psCard->GlobalBD.bd1.HighPart.Stride = (unsigned short)(psCard->scrn.lDelta / (psCard->scrn.Bpp >> 3));
	psCard->GlobalBD.bd1.HighPart.Bpp = (unsigned char) (psCard->scrn.Bpp);
	psCard->GlobalBD.bd1.Offset = 0;

	/*
	 * CR31, bit 0 = 0, Disable address offset bits(CR6A_6-0).
	 *       bit 0 = 1, Enable 8 Mbytes of display memory thru 64K window
	 *                  at A000:0.
	 */
	OUTREG8(CRT_ADDRESS_REG,MEMORY_CONFIG_REG);
	byte = INREG8(CRT_DATA_REG) & (~(ENABLE_CPUA_BASE_A0000));
	OUTREG8(CRT_DATA_REG,byte);

	/* program the GBD and SBDs */
	OUTREG32(S3_GLB_BD_LOW,psCard->GlobalBD.bd2.LoPart );
	OUTREG32(S3_GLB_BD_HIGH,(psCard->GlobalBD.bd2.HiPart | bci_enable | S3_LITTLE_ENDIAN | 0x10000000 | S3_BD64));
	OUTREG32(S3_PRI_BD_LOW,psCard->GlobalBD.bd2.LoPart);
	OUTREG32(S3_PRI_BD_HIGH,psCard->GlobalBD.bd2.HiPart);
	OUTREG32(S3_SEC_BD_LOW,psCard->GlobalBD.bd2.LoPart);
	OUTREG32(S3_SEC_BD_HIGH,psCard->GlobalBD.bd2.HiPart);

	/* turn on screen */
	OUTREG8(SEQ_ADDRESS_REG,0x01);
	byte = INREG8(SEQ_DATA_REG) & ~0x20;
	OUTREG8(SEQ_DATA_REG,byte);
}

void SavageDriver::SavageSetGBD_2000( savage_s *psCard )
{
	unsigned long ulYRange;
	unsigned char byte;
	int bci_enable, tile16, tile32;

	bci_enable = BCI_ENABLE_TWISTER;
	tile16 = TILE_DESTINATION;
	tile32 = TILE_DESTINATION;

	if (psCard->scrn.Width > 1024)
		ulYRange = 0x40000000;
	else
		ulYRange = 0x20000000;

    /* following is the enable case */

	/* SR01:turn off screen */
	OUTREG8 (SEQ_ADDRESS_REG,0x01);
	byte = INREG8(SEQ_DATA_REG) | 0x20;
	OUTREG8(SEQ_DATA_REG,byte);

	/* MM81C0 and 81B0 are used to control primary stream. */
	OUTREG32(PRI_STREAM_FBUF_ADDR0, 0);
	OUTREG32(PRI_STREAM2_FBUF_ADDR0, 0);

	/*
	 *  Program Primary Stream Stride Register.
	 *
	 *  Tell engine if tiling on or off, set primary stream stride, and
	 *  if tiling, set tiling bits/pixel and primary stream tile offset.
	 *  Note that tile offset (bits 16 - 29) must be scanline width in
	 *  bytes/128bytespertile * 256 Qwords/tile.  This is equivalent to
	 *  lDelta * 2.  Remember that if tiling, lDelta is screenwidth in
	 *  bytes padded up to an even number of tilewidths.
	 */
	OUTREG32(PRI_STREAM_STRIDE, ((psCard->scrn.lDelta << 4) & 0x7ff0));
	OUTREG32(PRI_STREAM2_STRIDE, ((psCard->scrn.lDelta << 4) & 0x7ff0));

	/*
	 * CR67_3:
	 *  = 1  stream processor MMIO address and stride register
	 *       are used to control the primary stream
	 *  = 0  standard VGA address and stride registers
	 *       are used to control the primary streams
	 */

	OUTREG8(CRT_ADDRESS_REG,0x67); 
	byte =  INREG8(CRT_DATA_REG) | 0x08;
	OUTREG8(CRT_DATA_REG,byte);

	OUTREG32(0x8128, 0xFFFFFFFFL);
	OUTREG32(0x812C, 0xFFFFFFFFL);

	/* bit 28:block write disable */
	OUTREG32(S3_GLB_BD_HIGH, bci_enable | S3_BD64 | 0x10000000); 

	/* CR50, bit 7,6,0 = 111, Use GBD.*/
	OUTREG8(CRT_ADDRESS_REG,0x50);
	byte = INREG8(CRT_DATA_REG) | 0xC1;
	OUTREG8(CRT_DATA_REG, byte);

	/* CR73 bit 5 = 0 block write disable */
	OUTREG8(CRT_ADDRESS_REG,0x73);
	byte = INREG8(CRT_DATA_REG) & ~0x20;
	OUTREG8(CRT_DATA_REG, byte);

	/*
	 *  Do not enable block_write even for non-tiling modes, because
	 *  the driver cannot determine if the memory type is the certain
	 *  type of SGRAM for which block_write can be used.
	 */
	psCard->GlobalBD.bd1.HighPart.ResBWTile = TILE_FORMAT_LINEAR;/* linear */

	psCard->GlobalBD.bd1.HighPart.ResBWTile |= 0x10;/* disable block write */
	/* HW uses width */
	psCard->GlobalBD.bd1.HighPart.Stride = (unsigned short)(psCard->scrn.lDelta / (psCard->scrn.Bpp >> 3));
	psCard->GlobalBD.bd1.HighPart.Bpp = (unsigned char) (psCard->scrn.Bpp);
	psCard->GlobalBD.bd1.Offset = 0;    

	/*
	 * CR31, bit 0 = 0, Disable address offset bits(CR6A_6-0).
	 *       bit 0 = 1, Enable 8 Mbytes of display memory thru 64K window
	 *                  at A000:0.
	 */
	OUTREG8(CRT_ADDRESS_REG,MEMORY_CONFIG_REG);
	byte = INREG8(CRT_DATA_REG) & (~(ENABLE_CPUA_BASE_A0000));
	OUTREG8(CRT_DATA_REG,byte);

	/* program the GBD and SBDs */
	OUTREG32(S3_GLB_BD_LOW,psCard->GlobalBD.bd2.LoPart );
	OUTREG32(S3_GLB_BD_HIGH,(psCard->GlobalBD.bd2.HiPart | bci_enable | S3_LITTLE_ENDIAN | 0x10000000 | S3_BD64));
	OUTREG32(S3_PRI_BD_LOW,psCard->GlobalBD.bd2.LoPart);
	OUTREG32(S3_PRI_BD_HIGH,psCard->GlobalBD.bd2.HiPart);
	OUTREG32(S3_SEC_BD_LOW,psCard->GlobalBD.bd2.LoPart);
	OUTREG32(S3_SEC_BD_HIGH,psCard->GlobalBD.bd2.HiPart);

	/* turn on screen */
	OUTREG8(SEQ_ADDRESS_REG,0x01);
	byte = INREG8(SEQ_DATA_REG) & ~0x20;
	OUTREG8(SEQ_DATA_REG,byte);
}

void SavageDriver::LockBitmap( SrvBitmap* pcDstBitmap, SrvBitmap* pcSrcBitmap, os::IRect cSrcRect, os::IRect cDstRect )
{
	if( ( pcDstBitmap->m_bVideoMem == false && ( pcSrcBitmap == NULL || pcSrcBitmap->m_bVideoMem == false ) ) || m_bEngineDirty == false )
		return;

	m_cGELock.Lock();
	WaitIdle( m_psCard );
	m_bEngineDirty = false;
	m_cGELock.Unlock();
}

unsigned int SavageDriver::SavageSetBD( SrvBitmap *pcBitmap )
{
	unsigned int bd = 0;

	/* HW uses width */
	if( pcBitmap->m_eColorSpc == CS_RGB32 )
	{
		BCI_BD_SET_BPP( bd, 32 );
		BCI_BD_SET_STRIDE( bd, pcBitmap->m_nBytesPerLine / 4 );
	}
	else
	{
		BCI_BD_SET_BPP( bd, 16 );
		BCI_BD_SET_STRIDE( bd, pcBitmap->m_nBytesPerLine / 2 );
	}
	bd |= BCI_BD_BW_DISABLE | BCI_BD_TILE_NONE;

	return bd;
}

bool SavageDriver::DrawLine( SrvBitmap *pcBitmap, const IRect &cClipRect, const IPoint &cPnt1, const IPoint &cPnt2, const Color32_s &sColor, int nMode )
{
	savage_s *psCard = m_psCard;
	bool bRet;

	if( pcBitmap->m_bVideoMem && nMode == DM_COPY &&
	    ( pcBitmap->m_eColorSpc == CS_RGB32 || pcBitmap->m_eColorSpc == CS_RGB16 ) )
	{
		int x1 = cPnt1.x;
		int y1 = cPnt1.y;
		int x2 = cPnt2.x;
		int y2 = cPnt2.y;

		if( DisplayDriver::ClipLine( cClipRect, &x1, &y1, &x2, &y2 ) == false )
			bRet = false;
		else
		{
			uint32 nCmd, nColor;
			int dx, dy;
			int min, max, xp, yp, ym;
			BCI_GET_PTR;

			if (pcBitmap->m_eColorSpc == CS_RGB32)
				nColor = COL_TO_RGB32(sColor);
			else
				nColor = COL_TO_RGB16(sColor);	/* We only support CS_RGB32 and CS_RGB16 */

			dx = x2 - x1;
			dy = y2 - y1;

			xp = (dx >= 0);
			if (!xp)
				dx = -dx;

			yp = (dy >= 0);
			if (!yp)
				dy = -dy;

			ym = (dy > dx );
			if( ym )
			{
				max = dy + 1;
				min = dx;
			}
			else
			{
				max = dx + 1;
				min = dy;
			}

			m_cGELock.Lock();

			nCmd = (BCI_CMD_LINE_LAST_PIXEL | BCI_CMD_RECT_XP |	BCI_CMD_RECT_YP |
					BCI_CMD_SEND_COLOR | BCI_CMD_DEST_PBD_NEW | BCI_CMD_SRC_SOLID);

			BCI_CMD_SET_ROP( nCmd, 0x00cc );	/* GXcopy */

			WaitQueue( psCard, 8 );

			BCI_SEND( nCmd );
			BCI_SEND( pcBitmap->m_nVideoMemOffset );
			BCI_SEND( SavageSetBD( pcBitmap ) );
			BCI_SEND( nColor );
			BCI_SEND( BCI_LINE_X_Y( x1, y1 ) );
			BCI_SEND( BCI_LINE_STEPS( 2 * (min - max), 2 * min ) );
			BCI_SEND( BCI_LINE_MISC( max, ym, xp, yp, 2 * min - max ) );

			m_bEngineDirty = true;
			m_cGELock.Unlock();
			bRet = true;
		}
	}
	else
		bRet = DisplayDriver::DrawLine( pcBitmap, cClipRect, cPnt1, cPnt2, sColor, nMode );

	return bRet;
}

bool SavageDriver::FillRect( SrvBitmap *pcBitmap, const IRect &cRect, const Color32_s &sColor, int nMode )
{
	savage_s *psCard = m_psCard;
	uint32 nColor;
	bool bRet;

	if ( pcBitmap->m_bVideoMem && cRect.Width() > 1 && cRect.Height() > 1 && 
		 ( pcBitmap->m_eColorSpc == CS_RGB32 || pcBitmap->m_eColorSpc == CS_RGB16 ) && nMode == DM_COPY )
	{
		BCI_GET_PTR;
		int nCmd;

		if (pcBitmap->m_eColorSpc == CS_RGB32)
			nColor = COL_TO_RGB32( sColor );
		else
		    nColor = COL_TO_RGB16( sColor );	 /* We only support CS_RGB32 and CS_RGB16 */

		m_cGELock.Lock();

		nCmd = BCI_CMD_RECT | BCI_CMD_RECT_XP | BCI_CMD_RECT_YP | BCI_CMD_DEST_PBD_NEW |
			   BCI_CMD_SRC_SOLID | BCI_CMD_SEND_COLOR;

		BCI_CMD_SET_ROP( nCmd, 0x00cc );	/* GXcopy */

		WaitQueue( psCard, 7 );
		BCI_SEND( nCmd );
		BCI_SEND( pcBitmap->m_nVideoMemOffset );
		BCI_SEND( SavageSetBD( pcBitmap ) );

		BCI_SEND( nColor );
		BCI_SEND( BCI_X_Y( cRect.left, cRect.top ) );
		BCI_SEND( BCI_W_H( cRect.Width() + 1, cRect.Height() + 1) );

		m_bEngineDirty = true;
		m_cGELock.Unlock();
		bRet = true;
	}
	else
		bRet = DisplayDriver::FillRect( pcBitmap, cRect, sColor, nMode );

	return bRet;
}

bool SavageDriver::BltBitmap( SrvBitmap *pcDstBitmap, SrvBitmap *pcSrcBitmap, IRect cSrcRect, IRect cDstRect, int nMode, int nAlpha )
{
	savage_s *psCard = m_psCard;
	bool bRet;

	if( pcSrcBitmap->m_bVideoMem == true && pcDstBitmap->m_bVideoMem == true &&
		nMode == DM_COPY &&
		cSrcRect.Size() == cDstRect.Size() )
	{
		/* Screen to screen copy */
		int nWidth  = cSrcRect.Width();
		int nHeight = cSrcRect.Height();
		int sx = cSrcRect.left;
		int sy = cSrcRect.top;
		int dx = cDstRect.left;
		int dy = cDstRect.top;

		BCI_GET_PTR;
		uint32 nCmd;

		//dbprintf( "VRAM->VRAM copy: w=%d h=%d sx=%d sy=%d dx=%d dy=%d\n", nWidth, nHeight, sx, sy, dx, dy );
		//dbprintf( "VRAM->VRAM copy: src=%d dst=%d\n", pcSrcBitmap->m_nVideoMemOffset, pcDstBitmap->m_nVideoMemOffset );

		m_cGELock.Lock();

		nCmd = (BCI_CMD_RECT | BCI_CMD_DEST_PBD_NEW | BCI_CMD_SRC_SBD_COLOR_NEW);
		BCI_CMD_SET_ROP( nCmd, 0x00cc );	/* GXcopy */

		if(dx < sx)
			nCmd |= BCI_CMD_RECT_XP;	/* Left to right */
		else
		{
			dx += nWidth;
			sx += nWidth;
		}

		if(dy < sy)
			nCmd |= BCI_CMD_RECT_YP;	/* Top to bottom */
		else
		{
			dy += nHeight;
			sy += nHeight;
		}

		WaitQueue( psCard, 9 );
		BCI_SEND( nCmd );

		/* Source */
		BCI_SEND( pcSrcBitmap->m_nVideoMemOffset );	/* Low */
		BCI_SEND( SavageSetBD( pcSrcBitmap ) );		/* High */

		/* Dest. */
		BCI_SEND( pcDstBitmap->m_nVideoMemOffset );	/* Low */
		BCI_SEND( SavageSetBD( pcDstBitmap ) );		/* High */

		BCI_SEND( BCI_X_Y( sx, sy ) );
		BCI_SEND( BCI_X_Y( dx, dy ) );
		BCI_SEND( BCI_W_H( nWidth + 1, nHeight + 1 ) );

		m_bEngineDirty = true;
		m_cGELock.Unlock();
		bRet = true;
	}
	else
		bRet = DisplayDriver::BltBitmap( pcDstBitmap, pcSrcBitmap, cSrcRect, cDstRect, nMode, nAlpha );

	return bRet;
}

