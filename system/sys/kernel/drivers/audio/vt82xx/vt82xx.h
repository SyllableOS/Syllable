/*
 *	VIA audio driver
 *	Copyright (C) 2006 Arno Klenke
 * 
 *	based on:
 *   ALSA driver for VIA VT82xx (South Bridge)
 *
 *   VT82C686A/B/C, VT8233A/C, VT8235
 *
 *	Copyright (c) 2000 Jaroslav Kysela <perex@suse.cz>
 *	                   Tjeerd.Mulder <Tjeerd.Mulder@fujitsu-siemens.com>
 *                    2002 Takashi Iwai <tiwai@suse.de>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 */

 
#include "ac97audio.h"
#include "audio.h"

/* size of DMA buffers */
#define VIA_MAX_BUFFER_DMA_PAGES	32

/* buffering default values in ms */
#define VIA_DEFAULT_FRAG_TIME		20
#define VIA_DEFAULT_BUFFER_TIME		500

#define VIA_MAX_FRAG_SIZE		PAGE_SIZE
#define VIA_MIN_FRAG_SIZE		64

#define VIA_MAX_FRAGS			32

/*
 * controller base 0 (scatter-gather) registers
 *
 */

#define VIA_BASE0_PCM_OUT_CHAN	0x40 /* output PCM to user */
#define VIA_BASE0_PCM_OUT_CHAN_STATUS 0x40
#define VIA_BASE0_PCM_OUT_CHAN_CTRL	0x41
#define VIA_BASE0_PCM_OUT_CHAN_TYPE	0x42
#define VIA_BASE0_PCM_OUT_BLOCK_COUNT	0x4C

#define VIA_BASE0_PCM_IN_CHAN		0x60 /* input PCM from user */
#define VIA_BASE0_PCM_IN_CHAN_STATUS	0x60
#define VIA_BASE0_PCM_IN_CHAN_CTRL	0x61
#define VIA_BASE0_PCM_IN_CHAN_TYPE	0x62

/* offsets from base */
#define VIA_PCM_STATUS			0x00
#define VIA_PCM_CONTROL			0x01
#define VIA_PCM_TYPE			0x02
#define VIA_PCM_CAPTURE_FIFO	0x02
#define VIA_PCM_TABLE_ADDR		0x04
#define VIA_PCM_STOP			0x08
#define VIA_PCM_BLOCK_COUNT		0x0C


#define VIA_AC97_CTRL		0x80

/* VIA_BASE0_AUDIO_xxx_CHAN_TYPE bits */
#define VIA_IRQ_ON_FLAG			(1<<0)	/* int on each flagged scatter block */
#define VIA_IRQ_ON_EOL			(1<<1)	/* int at end of scatter list */
#define VIA_INT_SEL_PCI_LAST_LINE_READ	(0)	/* int at PCI read of last line */
#define VIA_INT_SEL_LAST_SAMPLE_SENT	(1<<2)	/* int at last sample sent */
#define VIA_INT_SEL_ONE_LINE_LEFT	(1<<3)	/* int at less than one line to send */
#define VIA_PCM_FMT_STEREO		(1<<4)	/* PCM stereo format (bit clear == mono) */
#define VIA_PCM_FMT_16BIT		(1<<5)	/* PCM 16-bit format (bit clear == 8-bit) */
#define VIA_PCM_REC_FIFO			(1<<6)	/* enable FIFO?  documented as "reserved" */
#define VIA_PCM_FMT_VT_16BIT		(1<<7) /* PCM 16-bit format */
#define VIA_PCM_FMT_VT_MONO			(1<<4)
#define VIA_PCM_FMT_VT_STEREO		(1<<5)
#define VIA_RESTART_SGD_ON_EOL		(1<<7)	/* restart scatter-gather at EOL */

/* VIA_BASE0_AUDIO_xxx_CHAN_STOP bits */
#define VIA_PCM_STOP_16BIT		0x00200000
#define VIA_PCM_STOP_STEREO		0x00100000
#define VIA_PCM_STOP_NEVER		0xff000000
#define VIA_PCM_STOP_VT_MONO		( 1 << 0 ) | ( 1 << 4 )
#define VIA_PCM_STOP_VT_STEREO		( 1 << 0 ) | ( 2 << 4 )

/* VIA_BASE0_AUDIO_xxx_CAPTURE_FIFO bits */
#define VIA_PCM_CAPTURE_FIFO_ENABLE	0x40

/* PCI configuration register bits and masks */

#define VIA_ACLINK_STAT		0x40
#define  VIA_ACLINK_C11_READY	0x20
#define  VIA_ACLINK_C10_READY	0x10
#define  VIA_ACLINK_C01_READY	0x04 /* secondary codec ready */
#define  VIA_ACLINK_LOWPOWER	0x02 /* low-power state */
#define  VIA_ACLINK_C00_READY	0x01 /* primary codec ready */

#define VIA_ACLINK_CTRL		0x41
#define VIA_CR41_AC97_ENABLE	0x80 /* enable AC97 codec */
#define VIA_CR41_AC97_RESET	0x40 /* clear bit to reset AC97 */
#define VIA_CR41_AC97_WAKEUP	0x20 /* wake up from power-down mode */
#define VIA_CR41_AC97_SDO	0x10 /* force Serial Data Out (SDO) high */
#define VIA_CR41_VRA		0x08 /* enable variable sample rate */
#define VIA_CR41_PCM_ENABLE	0x04 /* AC Link SGD Read Channel PCM Data Output */
#define VIA_CR41_FM_PCM_ENABLE	0x02 /* AC Link FM Channel PCM Data Out */
#define VIA_CR41_SB_PCM_ENABLE	0x01 /* AC Link SB PCM Data Output */
#define VIA_CR41_BOOT_MASK	(VIA_CR41_AC97_ENABLE | \
				 VIA_CR41_AC97_WAKEUP | \
				 VIA_CR41_AC97_SDO)
#define VIA_CR41_RUN_MASK	(VIA_CR41_AC97_ENABLE | \
				 VIA_CR41_AC97_RESET | \
				 VIA_CR41_VRA | \
				 VIA_CR41_PCM_ENABLE)


#define VIA_SPDIF_CTRL	0x49
#define  VIA_SPDIF_DX3	0x08
#define  VIA_SPDIF_SLOT_MASK	0x03
#define  VIA_SPDIF_SLOT_1011	0x00
#define  VIA_SPDIF_SLOT_34		0x01
#define  VIA_SPDIF_SLOT_78		0x02
#define  VIA_SPDIF_SLOT_69		0x03


/* controller base 0 register bitmasks */
#define VIA_INT_DISABLE_MASK		(~(0x01|0x02))
#define VIA_SGD_STOPPED			(1 << 2)
#define VIA_SGD_ACTIVE			(1 << 7)
#define VIA_SGD_TERMINATE		(1 << 6)
#define VIA_SGD_FLAG			(1 << 0)
#define VIA_SGD_EOL				(1 << 1)
#define VIA_SGD_STOP			(1 << 2)
#define VIA_SGD_AUTOSTART		(1 << 5)
#define VIA_SGD_START			(1 << 7)

#define VIA_CR80_FIRST_CODEC		0
#define VIA_CR80_SECOND_CODEC		(1 << 30)
#define VIA_CR80_FIRST_CODEC_VALID	(1 << 25)
#define VIA_CR80_VALID			(1 << 25)
#define VIA_CR80_SECOND_CODEC_VALID	(1 << 27)
#define VIA_CR80_BUSY			(1 << 24)
#define VIA_CR83_BUSY			(1)
#define VIA_CR83_FIRST_CODEC_VALID	(1 << 1)
#define VIA_CR80_READ			(1 << 23)
#define VIA_CR80_WRITE_MODE		0
#define VIA_CR80_REG_IDX(idx)		((((idx) & 0xFF) >> 1) << 16)

#define VIA_DSP_CAP (DSP_CAP_REVISION | DSP_CAP_DUPLEX | \
		     DSP_CAP_TRIGGER | DSP_CAP_REALTIME)


/* scatter-gather DMA table entry, exactly as passed to hardware */
struct via_sgd_table {
	uint32 addr;
	uint32 count;	/* includes additional bits also */
};
#define VIA_EOL (1 << 31)
#define VIA_FLAG (1 << 30)
#define VIA_STOP (1 << 29)


#define VIA_MAX_STREAM 2

struct _VIAAudioDriver;

typedef struct 
{
	struct _VIAAudioDriver* psDriver;
	char zName[AUDIO_NAME_LENGTH];
	bool bIsLocked;
	bool bIsActive;
	bool bIsRecord;
	bool bIsSPDIF;
	uint32 nCnt;
	int nSampleRate;
	int nChannels;
	uint32 nFragSize;
	uint32 nFragNumber;
	area_id hSgdArea;
	uint32 nSgdPhysAddr;
	volatile struct via_sgd_table* pasSgTable;
	uint32 nPageNumber;
	area_id hBufArea;
	uint8* pBuffer;
	uint32 nPcmFmt;
	uint32 nPcmStop;
	uint32 nBaseAddr;
	uint32 nIoBase;
	uint32 nIRQMask;
	
	GenericStream_s sGeneric;
} VIAStream_s;


typedef struct _VIAAudioDriver
{
	int nDeviceID;
	char zName[AUDIO_NAME_LENGTH];
	sem_id hLock;
	PCI_Info_s sPCI;
	int nOpenCount;
	uint32 nBaseAddr;
	int nIRQHandle;
	AC97AudioDriver_s sAC97;
	int nNumStreams;
	VIAStream_s sStream[VIA_MAX_STREAM];
} VIAAudioDriver_s;























