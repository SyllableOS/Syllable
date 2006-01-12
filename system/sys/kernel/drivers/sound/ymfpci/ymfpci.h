/*
 *  Copyright 1999 Jaroslav Kysela <perex@suse.cz>
 *  Copyright 2000 Alan Cox <alan@redhat.com>
 *  Copyright 2001 Kai Germaschewski <kai@tp1.ruhr-uni-bochum.de>
 *  Copyright 2002 Pete Zaitcev <zaitcev@yahoo.com>
 *  Copyright 2004 Kristian Van Der Vliet <vanders@liqwyd.com>
 *
 *  Yamaha YMF7xx driver.
 *
 *  This code is a result of high-speed collision
 *  between ymfpci.c of ALSA and cs46xx.c of Linux.
 *  -- Pete Zaitcev <zaitcev@yahoo.com>; 2000/09/18
 *
 *  Then I ported it to Syllable.
 *  -- Kristian van Der Vliet <vanders@liqwyd.com>; 2004/03/15
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
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef __YMFPCI_H__
#define __YMFPCI_H__ 1

#include <atheos/types.h>
#include <atheos/pci.h>
#include <atheos/semaphore.h>
#include <atheos/spinlock.h>
#include <atheos/areas.h>
#include <atheos/soundcard.h>
#include <atheos/list.h>
#define NO_DEBUG_STUBS 1
#include <atheos/linux_compat.h>

#include "ac97_codec.h"

#define PCI_VENDOR_ID_YAMAHA            0x1073

#define PCI_DEVICE_ID_YAMAHA_724        0x0004
#define PCI_DEVICE_ID_YAMAHA_724F       0x000d
#define PCI_DEVICE_ID_YAMAHA_740        0x000a
#define PCI_DEVICE_ID_YAMAHA_740C       0x000c
#define PCI_DEVICE_ID_YAMAHA_744        0x0010
#define PCI_DEVICE_ID_YAMAHA_754        0x0012

/* Registers */

#define	YDSXGR_INTFLAG				0x0004
#define	YDSXGR_ACTIVITY				0x0006
#define	YDSXGR_GLOBALCTRL			0x0008
#define	YDSXGR_ZVCTRL				0x000A
#define	YDSXGR_TIMERCTRL			0x0010
#define	YDSXGR_TIMERCTRL_TEN		0x0001
#define	YDSXGR_TIMERCTRL_TIEN		0x0002
#define	YDSXGR_TIMERCOUNT			0x0012
#define	YDSXGR_SPDIFOUTCTRL			0x0018
#define	YDSXGR_SPDIFOUTSTATUS		0x001C
#define	YDSXGR_EEPROMCTRL			0x0020
#define	YDSXGR_SPDIFINCTRL			0x0034
#define	YDSXGR_SPDIFINSTATUS		0x0038
#define	YDSXGR_DSPPROGRAMDL			0x0048
#define	YDSXGR_DLCNTRL				0x004C
#define	YDSXGR_GPIOININTFLAG		0x0050
#define	YDSXGR_GPIOININTENABLE		0x0052
#define	YDSXGR_GPIOINSTATUS			0x0054
#define	YDSXGR_GPIOOUTCTRL			0x0056
#define	YDSXGR_GPIOFUNCENABLE		0x0058
#define	YDSXGR_GPIOTYPECONFIG		0x005A
#define	YDSXGR_AC97CMDDATA			0x0060
#define	YDSXGR_AC97CMDADR			0x0062
#define	YDSXGR_PRISTATUSDATA		0x0064
#define	YDSXGR_PRISTATUSADR			0x0066
#define	YDSXGR_SECSTATUSDATA		0x0068
#define	YDSXGR_SECSTATUSADR			0x006A
#define	YDSXGR_SECCONFIG			0x0070
#define	YDSXGR_LEGACYOUTVOL			0x0080
#define	YDSXGR_LEGACYOUTVOLL		0x0080
#define	YDSXGR_LEGACYOUTVOLR		0x0082
#define	YDSXGR_NATIVEDACOUTVOL		0x0084
#define	YDSXGR_NATIVEDACOUTVOLL		0x0084
#define	YDSXGR_NATIVEDACOUTVOLR		0x0086
#define	YDSXGR_SPDIFOUTVOL			0x0088
#define	YDSXGR_SPDIFOUTVOLL			0x0088
#define	YDSXGR_SPDIFOUTVOLR			0x008A
#define	YDSXGR_AC3OUTVOL			0x008C
#define	YDSXGR_AC3OUTVOLL			0x008C
#define	YDSXGR_AC3OUTVOLR			0x008E
#define	YDSXGR_PRIADCOUTVOL			0x0090
#define	YDSXGR_PRIADCOUTVOLL		0x0090
#define	YDSXGR_PRIADCOUTVOLR		0x0092
#define	YDSXGR_LEGACYLOOPVOL		0x0094
#define	YDSXGR_LEGACYLOOPVOLL		0x0094
#define	YDSXGR_LEGACYLOOPVOLR		0x0096
#define	YDSXGR_NATIVEDACLOOPVOL		0x0098
#define	YDSXGR_NATIVEDACLOOPVOLL	0x0098
#define	YDSXGR_NATIVEDACLOOPVOLR	0x009A
#define	YDSXGR_SPDIFLOOPVOL			0x009C
#define	YDSXGR_SPDIFLOOPVOLL		0x009E
#define	YDSXGR_SPDIFLOOPVOLR		0x009E
#define	YDSXGR_AC3LOOPVOL			0x00A0
#define	YDSXGR_AC3LOOPVOLL			0x00A0
#define	YDSXGR_AC3LOOPVOLR			0x00A2
#define	YDSXGR_PRIADCLOOPVOL		0x00A4
#define	YDSXGR_PRIADCLOOPVOLL		0x00A4
#define	YDSXGR_PRIADCLOOPVOLR		0x00A6
#define	YDSXGR_NATIVEADCINVOL		0x00A8
#define	YDSXGR_NATIVEADCINVOLL		0x00A8
#define	YDSXGR_NATIVEADCINVOLR		0x00AA
#define	YDSXGR_NATIVEDACINVOL		0x00AC
#define	YDSXGR_NATIVEDACINVOLL		0x00AC
#define	YDSXGR_NATIVEDACINVOLR		0x00AE
#define	YDSXGR_BUF441OUTVOL			0x00B0
#define	YDSXGR_BUF441OUTVOLL		0x00B0
#define	YDSXGR_BUF441OUTVOLR		0x00B2
#define	YDSXGR_BUF441LOOPVOL		0x00B4
#define	YDSXGR_BUF441LOOPVOLL		0x00B4
#define	YDSXGR_BUF441LOOPVOLR		0x00B6
#define	YDSXGR_SPDIFOUTVOL2			0x00B8
#define	YDSXGR_SPDIFOUTVOL2L		0x00B8
#define	YDSXGR_SPDIFOUTVOL2R		0x00BA
#define	YDSXGR_SPDIFLOOPVOL2		0x00BC
#define	YDSXGR_SPDIFLOOPVOL2L		0x00BC
#define	YDSXGR_SPDIFLOOPVOL2R		0x00BE
#define	YDSXGR_ADCSLOTSR			0x00C0
#define	YDSXGR_RECSLOTSR			0x00C4
#define	YDSXGR_ADCFORMAT			0x00C8
#define	YDSXGR_RECFORMAT			0x00CC
#define	YDSXGR_P44SLOTSR			0x00D0
#define	YDSXGR_STATUS				0x0100
#define	YDSXGR_CTRLSELECT			0x0104
#define	YDSXGR_MODE					0x0108
#define	YDSXGR_SAMPLECOUNT			0x010C
#define	YDSXGR_NUMOFSAMPLES			0x0110
#define	YDSXGR_CONFIG				0x0114
#define	YDSXGR_PLAYCTRLSIZE			0x0140
#define	YDSXGR_RECCTRLSIZE			0x0144
#define	YDSXGR_EFFCTRLSIZE			0x0148
#define	YDSXGR_WORKSIZE				0x014C
#define	YDSXGR_MAPOFREC				0x0150
#define	YDSXGR_MAPOFEFFECT			0x0154
#define	YDSXGR_PLAYCTRLBASE			0x0158
#define	YDSXGR_RECCTRLBASE			0x015C
#define	YDSXGR_EFFCTRLBASE			0x0160
#define	YDSXGR_WORKBASE				0x0164
#define	YDSXGR_DSPINSTRAM			0x1000
#define	YDSXGR_CTRLINSTRAM			0x4000

#define YDSXG_AC97READCMD			0x8000
#define YDSXG_AC97WRITECMD			0x0000

#define PCIR_LEGCTRL				0x40
#define PCIR_ELEGCTRL				0x42
#define PCIR_DSXGCTRL				0x48
#define PCIR_DSXPWRCTRL1			0x4a
#define PCIR_DSXPWRCTRL2			0x4e
#define PCIR_OPLADR					0x60
#define PCIR_SBADR					0x62
#define PCIR_MPUADR					0x64

#define YDSXG_DSPLENGTH				0x0080
#define YDSXG_CTRLLENGTH			0x3000

#define YDSXG_DEFAULT_WORK_SIZE		0x0400

#define YDSXG_PLAYBACK_VOICES		64
#define YDSXG_CAPTURE_VOICES		2
#define YDSXG_EFFECT_VOICES			5

/* AES/IEC958 channel status bits */
#define SND_PCM_AES0_PROFESSIONAL	(1<<0)	/* 0 = consumer, 1 = professional */
#define SND_PCM_AES0_NONAUDIO		(1<<1)	/* 0 = audio, 1 = non-audio */
#define SND_PCM_AES0_PRO_EMPHASIS	(7<<2)	/* mask - emphasis */
#define SND_PCM_AES0_PRO_EMPHASIS_NOTID	(0<<2)	/* emphasis not indicated */
#define SND_PCM_AES0_PRO_EMPHASIS_NONE	(1<<2)	/* none emphasis */
#define SND_PCM_AES0_PRO_EMPHASIS_5015	(3<<2)	/* 50/15us emphasis */
#define SND_PCM_AES0_PRO_EMPHASIS_CCITT	(7<<2)	/* CCITT J.17 emphasis */
#define SND_PCM_AES0_PRO_FREQ_UNLOCKED	(1<<5)	/* source sample frequency: 0 = locked, 1 = unlocked */
#define SND_PCM_AES0_PRO_FS		(3<<6)	/* mask - sample frequency */
#define SND_PCM_AES0_PRO_FS_NOTID	(0<<6)	/* fs not indicated */
#define SND_PCM_AES0_PRO_FS_44100	(1<<6)	/* 44.1kHz */
#define SND_PCM_AES0_PRO_FS_48000	(2<<6)	/* 48kHz */
#define SND_PCM_AES0_PRO_FS_32000	(3<<6)	/* 32kHz */
#define SND_PCM_AES0_CON_NOT_COPYRIGHT	(1<<2)	/* 0 = copyright, 1 = not copyright */
#define SND_PCM_AES0_CON_EMPHASIS	(7<<3)	/* mask - emphasis */
#define SND_PCM_AES0_CON_EMPHASIS_NONE	(0<<3)	/* none emphasis */
#define SND_PCM_AES0_CON_EMPHASIS_5015	(1<<3)	/* 50/15us emphasis */
#define SND_PCM_AES0_CON_MODE		(3<<6)	/* mask - mode */
#define SND_PCM_AES1_PRO_MODE		(15<<0)	/* mask - channel mode */
#define SND_PCM_AES1_PRO_MODE_NOTID	(0<<0)	/* not indicated */
#define SND_PCM_AES1_PRO_MODE_STEREOPHONIC (2<<0) /* stereophonic - ch A is left */
#define SND_PCM_AES1_PRO_MODE_SINGLE	(4<<0)	/* single channel */
#define SND_PCM_AES1_PRO_MODE_TWO	(8<<0)	/* two channels */
#define SND_PCM_AES1_PRO_MODE_PRIMARY	(12<<0)	/* primary/secondary */
#define SND_PCM_AES1_PRO_MODE_BYTE3	(15<<0)	/* vector to byte 3 */
#define SND_PCM_AES1_PRO_USERBITS	(15<<4)	/* mask - user bits */
#define SND_PCM_AES1_PRO_USERBITS_NOTID	(0<<4)	/* not indicated */
#define SND_PCM_AES1_PRO_USERBITS_192	(8<<4)	/* 192-bit structure */
#define SND_PCM_AES1_PRO_USERBITS_UDEF	(12<<4)	/* user defined application */
#define SND_PCM_AES1_CON_CATEGORY	0x7f
#define SND_PCM_AES1_CON_GENERAL	0x00
#define SND_PCM_AES1_CON_EXPERIMENTAL	0x40
#define SND_PCM_AES1_CON_SOLIDMEM_MASK	0x0f
#define SND_PCM_AES1_CON_SOLIDMEM_ID	0x08
#define SND_PCM_AES1_CON_BROADCAST1_MASK 0x07
#define SND_PCM_AES1_CON_BROADCAST1_ID	0x04
#define SND_PCM_AES1_CON_DIGDIGCONV_MASK 0x07
#define SND_PCM_AES1_CON_DIGDIGCONV_ID	0x02
#define SND_PCM_AES1_CON_ADC_COPYRIGHT_MASK 0x1f
#define SND_PCM_AES1_CON_ADC_COPYRIGHT_ID 0x06
#define SND_PCM_AES1_CON_ADC_MASK	0x1f
#define SND_PCM_AES1_CON_ADC_ID		0x16
#define SND_PCM_AES1_CON_BROADCAST2_MASK 0x0f
#define SND_PCM_AES1_CON_BROADCAST2_ID	0x0e
#define SND_PCM_AES1_CON_LASEROPT_MASK	0x07
#define SND_PCM_AES1_CON_LASEROPT_ID	0x01
#define SND_PCM_AES1_CON_MUSICAL_MASK	0x07
#define SND_PCM_AES1_CON_MUSICAL_ID	0x05
#define SND_PCM_AES1_CON_MAGNETIC_MASK	0x07
#define SND_PCM_AES1_CON_MAGNETIC_ID	0x03
#define SND_PCM_AES1_CON_IEC908_CD	(SND_PCM_AES1_CON_LASEROPT_ID|0x00)
#define SND_PCM_AES1_CON_NON_IEC908_CD	(SND_PCM_AES1_CON_LASEROPT_ID|0x08)
#define SND_PCM_AES1_CON_PCM_CODER	(SND_PCM_AES1_CON_DIGDIGCONV_ID|0x00)
#define SND_PCM_AES1_CON_SAMPLER	(SND_PCM_AES1_CON_DIGDIGCONV_ID|0x20)
#define SND_PCM_AES1_CON_MIXER		(SND_PCM_AES1_CON_DIGDIGCONV_ID|0x10)
#define SND_PCM_AES1_CON_RATE_CONVERTER	(SND_PCM_AES1_CON_DIGDIGCONV_ID|0x18)
#define SND_PCM_AES1_CON_SYNTHESIZER	(SND_PCM_AES1_CON_MUSICAL_ID|0x00)
#define SND_PCM_AES1_CON_MICROPHONE	(SND_PCM_AES1_CON_MUSICAL_ID|0x08)
#define SND_PCM_AES1_CON_DAT		(SND_PCM_AES1_CON_MAGNETIC_ID|0x00)
#define SND_PCM_AES1_CON_VCR		(SND_PCM_AES1_CON_MAGNETIC_ID|0x08)
#define SND_PCM_AES1_CON_ORIGINAL	(1<<7)	/* this bits depends on the category code */
#define SND_PCM_AES2_PRO_SBITS		(7<<0)	/* mask - sample bits */
#define SND_PCM_AES2_PRO_SBITS_20	(2<<0)	/* 20-bit - coordination */
#define SND_PCM_AES2_PRO_SBITS_24	(4<<0)	/* 24-bit - main audio */
#define SND_PCM_AES2_PRO_SBITS_UDEF	(6<<0)	/* user defined application */
#define SND_PCM_AES2_PRO_WORDLEN	(7<<3)	/* mask - source word length */
#define SND_PCM_AES2_PRO_WORDLEN_NOTID	(0<<3)	/* not indicated */
#define SND_PCM_AES2_PRO_WORDLEN_22_18	(2<<3)	/* 22-bit or 18-bit */
#define SND_PCM_AES2_PRO_WORDLEN_23_19	(4<<3)	/* 23-bit or 19-bit */
#define SND_PCM_AES2_PRO_WORDLEN_24_20	(5<<3)	/* 24-bit or 20-bit */
#define SND_PCM_AES2_PRO_WORDLEN_20_16	(6<<3)	/* 20-bit or 16-bit */
#define SND_PCM_AES2_CON_SOURCE		(15<<0)	/* mask - source number */
#define SND_PCM_AES2_CON_SOURCE_UNSPEC	(0<<0)	/* unspecified */
#define SND_PCM_AES2_CON_CHANNEL	(15<<4)	/* mask - channel number */
#define SND_PCM_AES2_CON_CHANNEL_UNSPEC	(0<<4)	/* unspecified */
#define SND_PCM_AES3_CON_FS		(15<<0)	/* mask - sample frequency */
#define SND_PCM_AES3_CON_FS_44100	(0<<0)	/* 44.1kHz */
#define SND_PCM_AES3_CON_FS_48000	(2<<0)	/* 48kHz */
#define SND_PCM_AES3_CON_FS_32000	(3<<0)	/* 32kHz */
#define SND_PCM_AES3_CON_CLOCK		(3<<4)	/* mask - clock accuracy */
#define SND_PCM_AES3_CON_CLOCK_1000PPM	(0<<4)	/* 1000 ppm */
#define SND_PCM_AES3_CON_CLOCK_50PPM	(1<<4)	/* 50 ppm */
#define SND_PCM_AES3_CON_CLOCK_VARIABLE	(2<<4)	/* variable pitch */

/* maxinum number of AC97 codecs connected, AC97 2.0 defined 4 */
#define NR_AC97		2

#define YMF_SAMPF			256	/* Samples per frame @48000 */

/* Device information */
typedef struct stru_ymfpci_playback_bank {
	uint32 format;
	uint32 loop_default;
	uint32 base;			/* 32-bit address */
	uint32 loop_start;			/* 32-bit offset */
	uint32 loop_end;			/* 32-bit offset */
	uint32 loop_frac;			/* 8-bit fraction - loop_start */
	uint32 delta_end;			/* pitch delta end */
	uint32 lpfK_end;
	uint32 eg_gain_end;
	uint32 left_gain_end;
	uint32 right_gain_end;
	uint32 eff1_gain_end;
	uint32 eff2_gain_end;
	uint32 eff3_gain_end;
	uint32 lpfQ;
	uint32 status;		/* P3: Always 0 for some reason. */
	uint32 num_of_frames;
	uint32 loop_count;
	uint32 start;		/* P3: J. reads this to know where chip is. */
	uint32 start_frac;
	uint32 delta;
	uint32 lpfK;
	uint32 eg_gain;
	uint32 left_gain;
	uint32 right_gain;
	uint32 eff1_gain;
	uint32 eff2_gain;
	uint32 eff3_gain;
	uint32 lpfD1;
	uint32 lpfD2;
} ymfpci_playback_bank_t;

typedef struct stru_ymfpci_capture_bank {
	uint32 base;			/* 32-bit address (aligned at 4) */
	uint32 loop_end;			/* size in BYTES (aligned at 4) */
	uint32 start;			/* 32-bit offset */
	uint32 num_of_loops;		/* counter */
} ymfpci_capture_bank_t;

typedef struct stru_ymfpci_effect_bank {
	uint32 base;			/* 32-bit address */
	uint32 loop_end;			/* 32-bit offset */
	uint32 start;			/* 32-bit offset */
	uint32 temp;
} ymfpci_effect_bank_t;

struct ymf_capture {
	int use;
	ymfpci_capture_bank_t *bank;
	struct ymf_pcm *ypcm;
};

typedef enum {
	YMFPCI_PCM,
	YMFPCI_SYNTH,
	YMFPCI_MIDI
} ymfpci_voice_type_t;

struct ymf_voice {
	int number;
	char use, pcm, synth, midi;	// bool
	ymfpci_playback_bank_t *bank;
	struct ymf_pcm *ypcm;
	uint32 bank_ba;
};
typedef struct ymf_voice ymfpci_voice_t;

struct ymf_dmabuf {
	uint32 dma_addr;
	void *rawbuf;
	unsigned buforder;

	/* OSS buffer management stuff */
	unsigned numfrag;
	unsigned fragshift;

	/* our buffer acts like a circular ring */
	unsigned hwptr;		/* where dma last started */
	unsigned swptr;		/* where driver last clear/filled */
	int count;		/* fill count */
	unsigned total_bytes;	/* total bytes dmaed by hardware */

//	wait_queue_head_t wait;	/* put process on wait queue when no more space in buffer */
	sem_id wait;

	/* redundant, but makes calculations easier */
	unsigned fragsize;
	unsigned dmasize;	/* Total rawbuf[] size */

	/* OSS stuff */
	unsigned mapped:1;
	unsigned ready:1;
	unsigned ossfragshift;
	int ossmaxfrags;
	unsigned subdivision;
};

struct ymf_pcm_format {
	int format;			/* OSS format */
	int rate;			/* rate in Hz */
	int voices;			/* number of voices */
	int shift;			/* redundant, computed from the above */
};

typedef enum {
	PLAYBACK_VOICE,
	CAPTURE_REC,
	CAPTURE_AC97,
	EFFECT_DRY_LEFT,
	EFFECT_DRY_RIGHT,
	EFFECT_EFF1,
	EFFECT_EFF2,
	EFFECT_EFF3
} ymfpci_pcm_type_t;

/* This is variant record, but we hate unions. Little waste on pointers []. */
struct ymf_pcm {
	ymfpci_pcm_type_t type;
	struct ymf_state *state;

	ymfpci_voice_t *voices[2];
	int capture_bank_number;

	struct ymf_dmabuf dmabuf;
	int running;
	int spdif;
};

/*
 * "Software" or virtual channel, an instance of opened /dev/dsp.
 * It may have two physical channels (pcms) for duplex operations.
 */

struct ymf_state {
	struct list_head chain;
	struct YMFDevice *device;			/* backpointer, was "struct ymf_unit *unit;" */
	struct ymf_pcm rpcm, wpcm;
	struct ymf_pcm_format format;
	int direction;	/* 0 = write, 1 = read */
};

struct YMFDevice
{
	SpinLock_s reg_lock;
	SpinLock_s voice_lock;
	SpinLock_s ac97_lock;
	sem_id open_sem;

	PCI_Info_s *pcidev;
	uint8 rev;
	int irq;
	int irq_handle;
	char *name;

	unsigned long base;
	area_id reg_area;
	void *reg_addr;

	struct ac97_codec *ac97_codec[NR_AC97];
	uint16 ac97_features;

	void* dma_area_va;
	uint32 dma_area_ba;
	unsigned int dma_area_size;

	uint32* ctrl_playback;
	uint32 ctrl_playback_ba;

	int start_count;
	int suspended;

	uint32 active_bank;
	struct ymf_voice voices[YDSXG_PLAYBACK_VOICES];
	struct ymf_capture capture[YDSXG_CAPTURE_VOICES];

	uint32 bank_base_capture;
	uint32 bank_base_effect;
	uint32 work_base;
	unsigned int work_size;

	ymfpci_playback_bank_t *bank_playback[YDSXG_PLAYBACK_VOICES][2];
	ymfpci_capture_bank_t *bank_capture[YDSXG_CAPTURE_VOICES][2];
	ymfpci_effect_bank_t *bank_effect[YDSXG_EFFECT_VOICES][2];

	struct list_head states;	/* List of states for this unit */
};

status_t ymfpci_init_one( PCI_bus_s *psBus, PCI_Info_s *psPCIDevice, struct YMFDevice **psInstanceData );

#endif	/* __YMFPCI_H__ */
