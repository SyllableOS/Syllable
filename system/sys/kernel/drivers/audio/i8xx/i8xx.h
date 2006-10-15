/*
 *	Intel ICH audio driver
 *	Copyright (C) 2006 Arno Klenke
 *  based on:
 *  ALSA driver for Intel ICH (i8x0) chipsets
 *
 *	Copyright (c) 2000 Jaroslav Kysela <perex@suse.cz>
 *
 *
 *   This code also contains alpha support for SiS 735 chipsets provided
 *   by Mike Pieper <mptei@users.sourceforge.net>. We have no datasheet
 *   for SiS735, so the code is not fully functional.
 *
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

/* Register blocks */
#define ICH_REG_BASE_PCMOUT	0x10
#define ICH_REG_BASE_MC		0x20
#define ICH_REG_BASE_SPDIF	0x60
#define ICH_REG_BASE_NV_SPDIF 0x70

/* Registers for every DMA engine */
#define ICH_REG_BDBAR	0x00 /* dword - buffer descriptor list base address */

#define ICH_REG_CIV		0x04 /* byte - current index value */

#define ICH_REG_LVI		0x05 /* byte - last valid index */
#define 	ICH_REG_LVI_MASK		0x1f

#define ICH_REG_SR		0x06 /* byte - status register */
#define 	ICH_FIFOE			0x10	/* FIFO error */
#define 	ICH_BCIS			0x08	/* buffer completion interrupt status */
#define 	ICH_LVBCI			0x04	/* last valid buffer completion interrupt */
#define 	ICH_CELV			0x02	/* current equals last valid */
#define 	ICH_DCH				0x01	/* DMA controller halted */

#define ICH_REG_PICB	0x08 /* word - position in current buffer */

#define ICH_REG_PIV		0x0a /* byte - prefetched index value */
#define		ICH_REG_PIV_MASK	0x1f	/* mask */

#define ICH_REG_CR		0x0b /* byte - control register */
#define 	ICH_IOCE			0x10	/* interrupt on completion enable */
#define 	ICH_FEIE			0x08	/* fifo error interrupt enable */
#define 	ICH_LVBIE			0x04	/* last valid buffer interrupt enable */
#define 	ICH_RESETREGS		0x02	/* reset busmaster registers */
#define 	ICH_STARTBM			0x01	/* start busmaster operation */


/* global block */
#define ICH_REG_GLOB_CNT		0x2c	/* dword - global control */
#define   ICH_PCM_SPDIF_MASK	0xc0000000	/* s/pdif pcm slot mask (ICH4) */
#define   ICH_PCM_SPDIF_NONE	0x00000000	/* reserved - undefined */
#define   ICH_PCM_SPDIF_78	0x40000000	/* s/pdif pcm on slots 7&8 */
#define   ICH_PCM_SPDIF_69	0x80000000	/* s/pdif pcm on slots 6&9 */
#define   ICH_PCM_SPDIF_1011	0xc0000000	/* s/pdif pcm on slots 10&11 */
#define   ICH_PCM_20BIT		0x00400000	/* 20-bit samples (ICH4) */
#define   ICH_PCM_246_MASK	0x00300000	/* 6 channels (not all chips) */
#define   ICH_PCM_6		0x00200000	/* 6 channels (not all chips) */
#define   ICH_PCM_4		0x00100000	/* 4 channels (not all chips) */
#define   ICH_PCM_2		0x00000000	/* 2 channels (stereo) */
#define   ICH_SIS_PCM_246_MASK	0x000000c0	/* 6 channels (SIS7012) */
#define   ICH_SIS_PCM_6		0x00000080	/* 6 channels (SIS7012) */
#define   ICH_SIS_PCM_4		0x00000040	/* 4 channels (SIS7012) */
#define   ICH_SIS_PCM_2		0x00000000	/* 2 channels (SIS7012) */
#define   ICH_TRIE		0x00000040	/* tertiary resume interrupt enable */
#define   ICH_SRIE		0x00000020	/* secondary resume interrupt enable */
#define   ICH_PRIE		0x00000010	/* primary resume interrupt enable */
#define   ICH_ACLINK		0x00000008	/* AClink shut off */
#define   ICH_AC97WARM		0x00000004	/* AC'97 warm reset */
#define   ICH_AC97COLD		0x00000002	/* AC'97 cold reset */
#define   ICH_GIE		0x00000001	/* GPI interrupt enable */
#define ICH_REG_GLOB_STA		0x30	/* dword - global status */
#define   ICH_TRI		0x20000000	/* ICH4: tertiary (AC_SDIN2) resume interrupt */
#define   ICH_TCR		0x10000000	/* ICH4: tertiary (AC_SDIN2) codec ready */
#define   ICH_BCS		0x08000000	/* ICH4: bit clock stopped */
#define   ICH_SPINT		0x04000000	/* ICH4: S/PDIF interrupt */
#define   ICH_P2INT		0x02000000	/* ICH4: PCM2-In interrupt */
#define   ICH_M2INT		0x01000000	/* ICH4: Mic2-In interrupt */
#define   ICH_SAMPLE_CAP	0x00c00000	/* ICH4: sample capability bits (RO) */
#define   ICH_SAMPLE_16_20	0x00400000	/* ICH4: 16- and 20-bit samples */
#define   ICH_MULTICHAN_CAP	0x00300000	/* ICH4: multi-channel capability bits (RO) */
#define   ICH_MD3		0x00020000	/* modem power down semaphore */
#define   ICH_AD3		0x00010000	/* audio power down semaphore */
#define   ICH_RCS		0x00008000	/* read completion status */
#define   ICH_BIT3		0x00004000	/* bit 3 slot 12 */
#define   ICH_BIT2		0x00002000	/* bit 2 slot 12 */
#define   ICH_BIT1		0x00001000	/* bit 1 slot 12 */
#define   ICH_SRI		0x00000800	/* secondary (AC_SDIN1) resume interrupt */
#define   ICH_PRI		0x00000400	/* primary (AC_SDIN0) resume interrupt */
#define   ICH_SCR		0x00000200	/* secondary (AC_SDIN1) codec ready */
#define   ICH_PCR		0x00000100	/* primary (AC_SDIN0) codec ready */
#define   ICH_MCINT		0x00000080	/* MIC capture interrupt */
#define   ICH_POINT		0x00000040	/* playback interrupt */
#define   ICH_PIINT		0x00000020	/* capture interrupt */
#define   ICH_NVSPINT		0x00000010	/* nforce spdif interrupt */
#define   ICH_MOINT		0x00000004	/* modem playback interrupt */
#define   ICH_MIINT		0x00000002	/* modem capture interrupt */
#define   ICH_GSCI		0x00000001	/* GPI status change interrupt */
#define ICH_REG_ACC_SEMA		0x34	/* byte - codec write semaphore */
#define   ICH_CAS		0x01		/* codec access semaphore */
#define ICH_REG_SDM		0x80
#define   ICH_DI2L_MASK		0x000000c0	/* PCM In 2, Mic In 2 data in line */
#define   ICH_DI2L_SHIFT	6
#define   ICH_DI1L_MASK		0x00000030	/* PCM In 1, Mic In 1 data in line */
#define   ICH_DI1L_SHIFT	4
#define   ICH_SE		0x00000008	/* steer enable */
#define   ICH_LDI_MASK		0x00000003	/* last codec read data input */

#define ICH_MAX_FRAGS		32		/* max hw frags */
 


#define ICH_MAX_FRAG_SIZE		PAGE_SIZE / 2
#define ICH_MAX_FRAGS		32		/* max hw frags */


/* scatter-gather DMA table entry, exactly as passed to hardware */
struct ich_sgd_table {
	uint32 addr;
	uint32 count;	/* includes additional bits also */
};

#define ICH_INT 0x80000000
#define ICH_MAX_STREAM 3

struct _ICHAudioDriver;

typedef struct 
{
	struct _ICHAudioDriver* psDriver;
	char zName[AUDIO_NAME_LENGTH];
	bool bIsLocked;
	bool bIsActive;
	bool bIsRecord;
	bool bIsSPDIF;
	uint32 nCnt;
	uint8 nLVI;
	uint8 nCIV;
	int nSampleRate;
	int nChannels;
	uint32 nFragSize;
	uint32 nFragNumber;
	area_id hSgdArea;
	uint32 nSgdPhysAddr;
	volatile struct ich_sgd_table* pasSgTable;
	uint32 nPageNumber;
	area_id hBufArea;
	uint8* pBuffer;
	
	uint32 nBaseAddr;
	uint32 nIoBase;
	uint32 nIRQMask;
	
	GenericStream_s sGeneric;
} ICHStream_s;


typedef struct _ICHAudioDriver
{
	int nDeviceID;
	char zName[AUDIO_NAME_LENGTH];
	sem_id hLock;
	PCI_Info_s sPCI;
	int nOpenCount;
	area_id hBaseArea;
	area_id hAC97Area;
	uint32 nBaseAddr;
	uint32 nAC97BaseAddr;
	int nIRQHandle;
	bool bSIS;
	bool bNVIDIA;
	bool bICH4;
	bool bMMIO;
	AC97AudioDriver_s sAC97;
	int nNumStreams;
	ICHStream_s sStream[ICH_MAX_STREAM];
} ICHAudioDriver_s;




















