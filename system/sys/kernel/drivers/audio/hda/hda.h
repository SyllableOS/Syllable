/*
 *
 *  hda.c - Intel HD Audio driver. Derived from the hda alsa driver
 * 
 *	Copyright(c) 2006 Arno Klenke
 *
 *  Copyright(c) 2004 Intel Corporation. All rights reserved.
 *
 *  Copyright (c) 2004 Takashi Iwai <tiwai@suse.de>
 *                     PeiSen Hou <pshou@realtek.com.tw>
 *
 *
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the Free
 *  Software Foundation; either version 2 of the License, or (at your option)
 *  any later version.
 *
 *  This program is distributed in the hope that it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 *  more details.
 *
 *  You should have received a copy of the GNU General Public License along with
 *  this program; if not, write to the Free Software Foundation, Inc., 59
 *  Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */
 
#ifndef _HDA_H_
#define _HDA_H_

#include "audio.h"

#include "hda_codec.h"

/*
 * registers
 */
#define HDA_REG_GCAP			0x00
#define HDA_REG_VMIN			0x02
#define HDA_REG_VMAJ			0x03
#define HDA_REG_OUTPAY			0x04
#define HDA_REG_INPAY			0x06
#define HDA_REG_GCTL			0x08
#define HDA_REG_WAKEEN			0x0c
#define HDA_REG_STATESTS		0x0e
#define HDA_REG_GSTS			0x10
#define HDA_REG_INTCTL			0x20
#define HDA_REG_INTSTS			0x24
#define HDA_REG_WALCLK			0x30
#define HDA_REG_SYNC			0x34	
#define HDA_REG_CORBLBASE		0x40
#define HDA_REG_CORBUBASE		0x44
#define HDA_REG_CORBWP			0x48
#define HDA_REG_CORBRP			0x4A
#define HDA_REG_CORBCTL		0x4c
#define HDA_REG_CORBSTS		0x4d
#define HDA_REG_CORBSIZE		0x4e

#define HDA_REG_RIRBLBASE		0x50
#define HDA_REG_RIRBUBASE		0x54
#define HDA_REG_RIRBWP			0x58
#define HDA_REG_RINTCNT		0x5a
#define HDA_REG_RIRBCTL		0x5c
#define HDA_REG_RIRBSTS		0x5d
#define HDA_REG_RIRBSIZE		0x5e

#define HDA_REG_IC			0x60
#define HDA_REG_IR			0x64
#define HDA_REG_IRS			0x68
#define   HDA_IRS_VALID	(1<<1)
#define   HDA_IRS_BUSY		(1<<0)

#define HDA_REG_DPLBASE		0x70
#define HDA_REG_DPUBASE		0x74
#define   HDA_DPLBASE_ENABLE	0x1	/* Enable position buffer */

/* SD offset: SDI0=0x80, SDI1=0xa0, ... SDO3=0x160 */
enum { SDI0, SDI1, SDI2, SDI3, SDO0, SDO1, SDO2, SDO3 };

/* stream register offsets from stream base */
#define HDA_REG_SD_CTL			0x00
#define HDA_REG_SD_STS			0x03
#define HDA_REG_SD_LPIB		0x04
#define HDA_REG_SD_CBL			0x08
#define HDA_REG_SD_LVI			0x0c
#define HDA_REG_SD_FIFOW		0x0e
#define HDA_REG_SD_FIFOSIZE		0x10
#define HDA_REG_SD_FORMAT		0x12
#define HDA_REG_SD_BDLPL		0x18
#define HDA_REG_SD_BDLPU		0x1c

/* PCI space */
#define HDA_PCIREG_TCSEL	0x44

/* RIRB int mask: overrun[2], response[0] */
#define RIRB_INT_RESPONSE	0x01
#define RIRB_INT_OVERRUN	0x04
#define RIRB_INT_MASK		0x05

/* STATESTS int mask: SD2,SD1,SD0 */
#define STATESTS_INT_MASK	0x07
#define AZX_MAX_CODECS		4

/* SD_CTL bits */
#define SD_CTL_STREAM_RESET	0x01	/* stream reset bit */
#define SD_CTL_DMA_START	0x02	/* stream DMA start bit */
#define SD_CTL_STREAM_TAG_MASK	(0xf << 20)
#define SD_CTL_STREAM_TAG_SHIFT	20

/* SD_CTL and SD_STS */
#define SD_INT_DESC_ERR		0x10	/* descriptor error interrupt */
#define SD_INT_FIFO_ERR		0x08	/* FIFO error interrupt */
#define SD_INT_COMPLETE		0x04	/* completion interrupt */
#define SD_INT_MASK		(SD_INT_DESC_ERR|SD_INT_FIFO_ERR|SD_INT_COMPLETE)

/* SD_STS */
#define SD_STS_FIFO_READY	0x20	/* FIFO ready */

/* INTCTL and INTSTS */
#define HDA_INT_ALL_STREAM	0xff		/* all stream interrupts */
#define HDA_INT_CTRL_EN	0x40000000	/* controller interrupt enable bit */
#define HDA_INT_GLOBAL_EN	0x80000000	/* global interrupt enable bit */

/* GCTL unsolicited response enable bit */
#define HDA_GCTL_UREN		(1<<8)

/* GCTL reset bit */
#define HDA_GCTL_RESET		(1<<0)

/* CORB/RIRB control, read/write pointer */
#define HDA_RBCTL_DMA_EN	0x02	/* enable DMA */
#define HDA_RBCTL_IRQ_EN	0x01	/* enable IRQ */
#define HDA_RBRWP_CLR		0x8000	/* read/write pointer clear */
/* below are so far hardcoded - should read registers in future */
#define HDA_MAX_CORB_ENTRIES	256
#define HDA_MAX_RIRB_ENTRIES	256

#define HDA_RIRB_EX_UNSOL_EV	(1<<4)

#define HDA_MAX_FRAG_SIZE		PAGE_SIZE / 2
#define HDA_MAX_FRAGS		32		/* max hw frags */

#define HDA_LVI_MASK		0x1f


#define HDA_MAX_STREAM 1

/* Defines for ATI HD Audio support in SB450 south bridge */
#define ATI_SB450_HDAUDIO_MISC_CNTR2_ADDR   0x42
#define ATI_SB450_HDAUDIO_ENABLE_SNOOP      0x02

/* Defines for Nvidia HDA support */
#define NVIDIA_HDA_TRANSREG_ADDR      0x4e
#define NVIDIA_HDA_ENABLE_COHBITS     0x0f

/* scatter-gather DMA table entry, exactly as passed to hardware */
struct hda_sgd_table {
	uint32 addr;
	uint32 addr_high;
	uint32 count;
	uint32 flags;
};

typedef struct _HDABuffer
{
	uint32* pBuf;
	uint32 nRp;
	uint32 nWp;
	int nCmds;
	uint32 nResult;
} HDABuffer_s;



struct _HDAAudioDriver;

typedef struct 
{
	struct _HDAAudioDriver* psDriver;
	char zName[AUDIO_NAME_LENGTH];
	int nStreamTag;
	bool bIsLocked;
	bool bIsActive;
	bool bIsRecord;
	uint32 nCnt;
	uint8 nLVI;
	uint8 nCIV;
	int nSampleRate;
	int nChannels;
	uint32 nFragSize;
	uint32 nFragNumber;
	area_id hSgdArea;
	uint32 nSgdPhysAddr;
	volatile struct hda_sgd_table* pasSgTable;
	uint32 nPageNumber;
	area_id hBufArea;
	uint8* pBuffer;
	
	uint32 nBaseAddr;
	uint32 nIoBase;
	uint32 nIRQMask;
	
	GenericStream_s sGeneric;
} HDAStream_s;

typedef struct _HDAAudioDriver
{
	int nDeviceID;
	char zName[AUDIO_NAME_LENGTH];
	sem_id hLock;
	PCI_Info_s sPCI;
	int nOpenCount;
	int nIRQHandle;
	uint32 nCodecMask;
	uint32* pCmdBufAddr;
	area_id hCmdBufArea;
	uint32 nBaseAddr;
	area_id hBaseArea;
	HDABuffer_s sCorb;
	HDABuffer_s sRirb;
	
	int nFg;
	int nNodesStart;
	int nNodes;
	HDANode_s* pasNodes;
	int nOutputPaths;
	HDAOutputPath_s pasOutputPaths[HDA_MAX_OUTPUT_PATHS];

	int nNumStreams;
	HDAStream_s sStream[HDA_MAX_STREAM];

} HDAAudioDriver_s;


status_t hda_initialize_codec( HDAAudioDriver_s* psDriver, int nCodec );

#endif
















