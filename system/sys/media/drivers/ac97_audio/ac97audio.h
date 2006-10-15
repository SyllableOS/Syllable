/*  Syllable Audio Interface (AC97 Codec)
 *  Copyright (C) 2006 Arno Klenke
 *
 *  Copyright 2000 Silicon Integrated System Corporation
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of version 2 of the GNU Library
 *  General Public License as published by the Free Software
 *  Foundation.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 *  MA 02111-1307, USA
 */

#ifndef _AC97_AUDIO_H_
#define _AC97_AUDIO_H_


/* AC97 1.0 */
#define  AC97_RESET               0x0000      //
#define  AC97_MASTER_VOL_STEREO   0x0002      // Line Out
#define  AC97_HEADPHONE_VOL       0x0004      // 
#define  AC97_MASTER_VOL_MONO     0x0006      // TAD Output
#define  AC97_MASTER_TONE         0x0008      //
#define  AC97_PCBEEP_VOL          0x000a      // none
#define  AC97_PHONE_VOL           0x000c      // TAD Input (mono)
#define  AC97_MIC_VOL             0x000e      // MIC Input (mono)
#define  AC97_LINEIN_VOL          0x0010      // Line Input (stereo)
#define  AC97_CD_VOL              0x0012      // CD Input (stereo)
#define  AC97_VIDEO_VOL           0x0014      // none
#define  AC97_AUX_VOL             0x0016      // Aux Input (stereo)
#define  AC97_PCMOUT_VOL          0x0018      // Wave Output (stereo)
#define  AC97_RECORD_SELECT       0x001a      //
#define  AC97_RECORD_GAIN         0x001c
#define  AC97_RECORD_GAIN_MIC     0x001e
#define  AC97_GENERAL_PURPOSE     0x0020
#define  AC97_3D_CONTROL          0x0022
#define  AC97_MODEM_RATE          0x0024
#define  AC97_POWER_CONTROL       0x0026

/* AC'97 2.0 */
#define AC97_EXTENDED_ID          0x0028       /* Extended Audio ID */
#define AC97_EXTENDED_STATUS      0x002A       /* Extended Audio Status */
#define AC97_PCM_FRONT_DAC_RATE   0x002C       /* PCM Front DAC Rate */
#define AC97_PCM_SURR_DAC_RATE    0x002E       /* PCM Surround DAC Rate */
#define AC97_PCM_LFE_DAC_RATE     0x0030       /* PCM LFE DAC Rate */
#define AC97_PCM_LR_ADC_RATE      0x0032       /* PCM LR ADC Rate */
#define AC97_PCM_MIC_ADC_RATE     0x0034       /* PCM MIC ADC Rate */
#define AC97_CENTER_LFE_MASTER    0x0036       /* Center + LFE Master Volume */
#define AC97_SURROUND_MASTER      0x0038       /* Surround (Rear) Master Volume */
#define AC97_RESERVED_3A          0x003A       /* Reserved in AC '97 < 2.2 */

/* AC'97 2.2 */
#define AC97_SPDIF_CONTROL        0x003A       /* S/PDIF Control */

/* range 0x3c-0x58 - MODEM */
#define AC97_EXTENDED_MODEM_ID    0x003C
#define AC97_EXTEND_MODEM_STAT    0x003E
#define AC97_LINE1_RATE           0x0040
#define AC97_LINE2_RATE           0x0042
#define AC97_HANDSET_RATE         0x0044
#define AC97_LINE1_LEVEL          0x0046
#define AC97_LINE2_LEVEL          0x0048
#define AC97_HANDSET_LEVEL        0x004A
#define AC97_GPIO_CONFIG          0x004C
#define AC97_GPIO_POLARITY        0x004E
#define AC97_GPIO_STICKY          0x0050
#define AC97_GPIO_WAKE_UP         0x0052
#define AC97_GPIO_STATUS          0x0054
#define AC97_MISC_MODEM_STAT      0x0056
#define AC97_RESERVED_58          0x0058

/* registers 0x005a - 0x007a are vendor reserved */

#define AC97_VENDOR_ID1           0x007c
#define AC97_VENDOR_ID2           0x007e

/* basic capabilities (reset register) */
#define AC97_BC_DEDICATED_MIC	0x0001	/* Dedicated Mic PCM In Channel */
#define AC97_BC_RESERVED1	0x0002	/* Reserved (was Modem Line Codec support) */
#define AC97_BC_BASS_TREBLE	0x0004	/* Bass & Treble Control */
#define AC97_BC_SIM_STEREO	0x0008	/* Simulated stereo */
#define AC97_BC_HEADPHONE	0x0010	/* Headphone Out Support */
#define AC97_BC_LOUDNESS	0x0020	/* Loudness (bass boost) Support */
#define AC97_BC_16BIT_DAC	0x0000	/* 16-bit DAC resolution */
#define AC97_BC_18BIT_DAC	0x0040	/* 18-bit DAC resolution */
#define AC97_BC_20BIT_DAC	0x0080	/* 20-bit DAC resolution */
#define AC97_BC_DAC_MASK	0x00c0
#define AC97_BC_16BIT_ADC	0x0000	/* 16-bit ADC resolution */
#define AC97_BC_18BIT_ADC	0x0100	/* 18-bit ADC resolution */
#define AC97_BC_20BIT_ADC	0x0200	/* 20-bit ADC resolution */
#define AC97_BC_ADC_MASK	0x0300


/* extended audio ID bit defines */
#define AC97_EI_VRA		0x0001	/* Variable bit rate supported */
#define AC97_EI_DRA		0x0002	/* Double rate supported */
#define AC97_EI_SPDIF		0x0004	/* S/PDIF out supported */
#define AC97_EI_VRM		0x0008	/* Variable bit rate supported for MIC */
#define AC97_EI_DACS_SLOT_MASK	0x0030	/* DACs slot assignment */
#define AC97_EI_DACS_SLOT_SHIFT	4
#define AC97_EI_CDAC		0x0040	/* PCM Center DAC available */
#define AC97_EI_SDAC		0x0080	/* PCM Surround DACs available */
#define AC97_EI_LDAC		0x0100	/* PCM LFE DAC available */
#define AC97_EI_AMAP		0x0200	/* indicates optional slot/DAC mapping based on codec ID */
#define AC97_EI_REV_MASK	0x0c00	/* AC'97 revision mask */
#define AC97_EI_REV_22		0x0400	/* AC'97 revision 2.2 */
#define AC97_EI_REV_23		0x0800	/* AC'97 revision 2.3 */
#define AC97_EI_REV_SHIFT	10
#define AC97_EI_ADDR_MASK	0xc000	/* physical codec ID (address) */
#define AC97_EI_ADDR_SHIFT	14


/* extended audio status and control bit defines */
#define AC97_EA_VRA		0x0001	/* Variable bit rate enable bit */
#define AC97_EA_DRA		0x0002	/* Double-rate audio enable bit */
#define AC97_EA_SPDIF		0x0004	/* S/PDIF out enable bit */
#define AC97_EA_VRM		0x0008	/* Variable bit rate for MIC enable bit */
#define AC97_EA_SPSA_SLOT_MASK	0x0030	/* Mask for slot assignment bits */
#define AC97_EA_SPSA_SLOT_SHIFT 4
#define AC97_EA_SPSA_3_4	0x0000	/* Slot assigned to 3 & 4 */
#define AC97_EA_SPSA_7_8	0x0010	/* Slot assigned to 7 & 8 */
#define AC97_EA_SPSA_6_9	0x0020	/* Slot assigned to 6 & 9 */
#define AC97_EA_SPSA_10_11	0x0030	/* Slot assigned to 10 & 11 */
#define AC97_EA_CDAC		0x0040	/* PCM Center DAC is ready (Read only) */
#define AC97_EA_SDAC		0x0080	/* PCM Surround DACs are ready (Read only) */
#define AC97_EA_LDAC		0x0100	/* PCM LFE DAC is ready (Read only) */
#define AC97_EA_MDAC		0x0200	/* MIC ADC is ready (Read only) */
#define AC97_EA_SPCV		0x0400	/* S/PDIF configuration valid (Read only) */
#define AC97_EA_PRI		0x0800	/* Turns the PCM Center DAC off */
#define AC97_EA_PRJ		0x1000	/* Turns the PCM Surround DACs off */
#define AC97_EA_PRK		0x2000	/* Turns the PCM LFE DAC off */
#define AC97_EA_PRL		0x4000	/* Turns the MIC ADC off */


/* S/PDIF control bit defines */
#define AC97_SC_PRO		0x0001	/* Professional status */
#define AC97_SC_NAUDIO		0x0002	/* Non audio stream */
#define AC97_SC_COPY		0x0004	/* Copyright status */
#define AC97_SC_PRE		0x0008	/* Preemphasis status */
#define AC97_SC_CC_MASK		0x07f0	/* Category Code mask */
#define AC97_SC_CC_SHIFT	4
#define AC97_SC_L		0x0800	/* Generation Level status */
#define AC97_SC_SPSR_MASK	0x3000	/* S/PDIF Sample Rate bits */
#define AC97_SC_SPSR_SHIFT	12
#define AC97_SC_SPSR_44K	0x0000	/* Use 44.1kHz Sample rate */
#define AC97_SC_SPSR_48K	0x2000	/* Use 48kHz Sample rate */
#define AC97_SC_SPSR_32K	0x3000	/* Use 32kHz Sample rate */
#define AC97_SC_DRS		0x4000	/* Double Rate S/PDIF */
#define AC97_SC_V		0x8000	/* Validity status */

/* volume control bit defines */
#define AC97_MUTE                 0x8000
#define AC97_MICBOOST             0x0040
#define AC97_LEFTVOL              0x3f00
#define AC97_RIGHTVOL             0x003f

/* record mux defines */
#define AC97_RECMUX_MIC           0x0000
#define AC97_RECMUX_CD            0x0101
#define AC97_RECMUX_VIDEO         0x0202
#define AC97_RECMUX_AUX           0x0303
#define AC97_RECMUX_LINE          0x0404
#define AC97_RECMUX_STEREO_MIX    0x0505
#define AC97_RECMUX_MONO_MIX      0x0606
#define AC97_RECMUX_PHONE         0x0707

/* general purpose register bit defines */
#define AC97_GP_LPBK              0x0080       /* Loopback mode */
#define AC97_GP_MS                0x0100       /* Mic Select 0=Mic1, 1=Mic2 */
#define AC97_GP_MIX               0x0200       /* Mono output select 0=Mix, 1=Mic */
#define AC97_GP_RLBK              0x0400       /* Remote Loopback - Modem line codec */
#define AC97_GP_LLBK              0x0800       /* Local Loopback - Modem Line codec */
#define AC97_GP_LD                0x1000       /* Loudness 1=on */
#define AC97_GP_3D                0x2000       /* 3D Enhancement 1=on */
#define AC97_GP_ST                0x4000       /* Stereo Enhancement 1=on */
#define AC97_GP_POP               0x8000       /* Pcm Out Path, 0=pre 3D, 1=post 3D */

/* powerdown control and status bit defines */

/* status */
#define AC97_PWR_MDM              0x0010       /* Modem section ready */
#define AC97_PWR_REF              0x0008       /* Vref nominal */
#define AC97_PWR_ANL              0x0004       /* Analog section ready */
#define AC97_PWR_DAC              0x0002       /* DAC section ready */
#define AC97_PWR_ADC              0x0001       /* ADC section ready */

/* control */
#define AC97_PWR_PR0              0x0100       /* ADC and Mux powerdown */
#define AC97_PWR_PR1              0x0200       /* DAC powerdown */
#define AC97_PWR_PR2              0x0400       /* Output mixer powerdown (Vref on) */
#define AC97_PWR_PR3              0x0800       /* Output mixer powerdown (Vref off) */
#define AC97_PWR_PR4              0x1000       /* AC-link powerdown */
#define AC97_PWR_PR5              0x2000       /* Internal Clk disable */
#define AC97_PWR_PR6              0x4000       /* HP amp powerdown */
#define AC97_PWR_PR7              0x8000       /* Modem off - if supported */


/* useful power states */
#define AC97_PWR_D0               0x0000      /* everything on */
#define AC97_PWR_D1              AC97_PWR_PR0|AC97_PWR_PR1|AC97_PWR_PR4
#define AC97_PWR_D2              AC97_PWR_PR0|AC97_PWR_PR1|AC97_PWR_PR2|AC97_PWR_PR3|AC97_PWR_PR4
#define AC97_PWR_D3              AC97_PWR_PR0|AC97_PWR_PR1|AC97_PWR_PR2|AC97_PWR_PR3|AC97_PWR_PR4
#define AC97_PWR_ANLOFF          AC97_PWR_PR2|AC97_PWR_PR3  /* analog section off */

/* Total number of defined registers.  */
#define AC97_REG_CNT 64

#define AC97_MAX_MIXER 16

#define AC97_NAME_LENGTH 255

typedef struct
{
	bool bStereo;
	bool bInvert;
	int nCodec;
	uint8 nReg;
	uint32 nScale;
	uint16 nValue;
} AC97Mixer_s;

typedef struct
{
	status_t (*pfWait)( void* pDriverData, int nCodec );
	status_t (*pfWrite)( void* pDriverData, int nCodec, uint8 nReg, uint16 nVal );
	uint16 (*pfRead)( void* pDriverData, int nCodec, uint8 nReg );
	void* pDriverData;
	
	char zID[AC97_NAME_LENGTH];
	int nCodecs;
	uint32 nID[4];
	uint16 nBasicID[4];
	uint16 nExtID[4];
	uint16 nExtStat[4];
	int nChannels[4];
	int nMaxChannels;
	AC97Mixer_s asMixer[AC97_MAX_MIXER];
	int nNumMixer;
} AC97AudioDriver_s;


typedef struct
{
	int nDeviceID;
	int nDeviceHandle;
	char zName[AC97_NAME_LENGTH];
	bool bSPDIF;
	bool bRecord;
	int nMaxChannels;
	int nMaxSampleRate;
	int nMaxResolution;
} AC97CardInfo_s;

typedef struct
{
	int nCodec;
	uint8 nReg;
	uint16 nVal;
} AC97RegOp_s;

typedef struct
{
	int nChannels;
	int nSampleRate;
	int nResolution;
} AC97Format_s;

enum
{
	AC97_GET_CARD_INFO = 10000,
	AC97_SET_FORMAT,
	AC97_GET_BUFFER_SIZE,
	AC97_GET_DELAY,
	AC97_CLEAR,
	
	AC97_GET_CODEC_INFO,
	AC97_WAIT,
	AC97_READ,
	AC97_WRITE
};

bool ac97_supports_vra( AC97AudioDriver_s* psDriver, int nCodec );

int ac97_supports_spdif( AC97AudioDriver_s* psDriver, int nCodec );

int ac97_get_max_channels( AC97AudioDriver_s* psDriver );

int ac97_get_channels( AC97AudioDriver_s* psDriver, int nCodec );
status_t ac97_enable_spdif( AC97AudioDriver_s* psDriver, int nCodec, bool bEnable );
status_t ac97_set_rate( AC97AudioDriver_s* psDriver, int nCodec, uint8 nReg, uint nRate );
status_t ac97_initialize( AC97AudioDriver_s* psDriver, char* pzID, int nNumCodecs );
status_t ac97_ioctl( AC97AudioDriver_s* psDriver, uint32 nCommand, void *pArgs, bool bFromKernel );
status_t ac97_suspend( AC97AudioDriver_s* psDriver );
status_t ac97_resume( AC97AudioDriver_s* psDriver );

#endif






















