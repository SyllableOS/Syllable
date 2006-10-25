/*****************************************************************************/
/*
 *      es1371.c  --  Creative Ensoniq ES1371.
 *
 *      Copyright (C) 1998-2001  Thomas Sailer (t.sailer@alumni.ethz.ch)
 *                               and others.
 *               Port to AtheOS  Sergio Lopez (xatann@gmx.net) 
 *
 *      Default setting for playback changed from 8 KHz mono to 44.1 KHz stereo.
 *          29 August 2003, Kaj de Vos
 *
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; either version 2 of the License, or
 *      (at your option) any later version.
 *
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *
 *      You should have received a copy of the GNU General Public License
 *      along with this program; if not, write to the Free Software
 *      Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * Special thanks to Ensoniq
 */
/*****************************************************************************/

#include <posix/errno.h>
#include <posix/ioctls.h>
#include <posix/fcntl.h>
#include <posix/termios.h>
#include <posix/signal.h>

#include <atheos/kernel.h>
#include <atheos/kdebug.h>
#include <atheos/areas.h>
#include <atheos/pci.h>
#include <atheos/device.h>
#include <atheos/semaphore.h>
#include <atheos/irq.h>
#include <atheos/isa_io.h>
#include <atheos/dma.h>
#include <atheos/udelay.h>
#include <atheos/areas.h>
#include <atheos/soundcard.h>
#include <atheos/spinlock.h>
#include <atheos/linux_compat.h>
#include <macros.h>

#include "ac97_codec.h"

#ifndef PCI_VENDOR_ID_ENSONIQ
#define PCI_VENDOR_ID_ENSONIQ        0x1274    
#endif

#ifndef PCI_VENDOR_ID_ECTIVA
#define PCI_VENDOR_ID_ECTIVA         0x1102
#endif

#ifndef PCI_DEVICE_ID_ENSONIQ_ES1371
#define PCI_DEVICE_ID_ENSONIQ_ES1371 0x1371
#endif

#ifndef PCI_DEVICE_ID_ENSONIQ_CT5880
#define PCI_DEVICE_ID_ENSONIQ_CT5880 0x5880
#endif

#ifndef PCI_DEVICE_ID_ECTIVA_EV1938
#define PCI_DEVICE_ID_ECTIVA_EV1938 0x8938
#endif

/* ES1371 chip ID */
/* This is a little confusing because all ES1371 compatible chips have the
   same DEVICE_ID, the only thing differentiating them is the REV_ID field.
   This is only significant if you want to enable features on the later parts.
   Yes, I know it's stupid and why didn't we use the sub IDs?
*/
#define ES1371REV_ES1373_A  0x04
#define ES1371REV_ES1373_B  0x06
#define ES1371REV_CT5880_A  0x07
#define CT5880REV_CT5880_C  0x02
#define CT5880REV_CT5880_D  0x03
#define ES1371REV_ES1371_B  0x09
#define EV1938REV_EV1938_A  0x00
#define ES1371REV_ES1373_8  0x08

#define ES1371_MAGIC  ((PCI_VENDOR_ID_ENSONIQ<<16)|PCI_DEVICE_ID_ENSONIQ_ES1371)

#define ES1371_EXTENT             0x40
#define JOY_EXTENT                8

#define ES1371_REG_CONTROL        0x00
#define ES1371_REG_STATUS         0x04 /* on the 5880 it is control/status */
#define ES1371_REG_UART_DATA      0x08
#define ES1371_REG_UART_STATUS    0x09
#define ES1371_REG_UART_CONTROL   0x09
#define ES1371_REG_UART_TEST      0x0a
#define ES1371_REG_MEMPAGE        0x0c
#define ES1371_REG_SRCONV         0x10
#define ES1371_REG_CODEC          0x14
#define ES1371_REG_LEGACY         0x18
#define ES1371_REG_SERIAL_CONTROL 0x20
#define ES1371_REG_DAC1_SCOUNT    0x24
#define ES1371_REG_DAC2_SCOUNT    0x28
#define ES1371_REG_ADC_SCOUNT     0x2c

#define ES1371_REG_DAC1_FRAMEADR  0xc30
#define ES1371_REG_DAC1_FRAMECNT  0xc34
#define ES1371_REG_DAC2_FRAMEADR  0xc38
#define ES1371_REG_DAC2_FRAMECNT  0xc3c
#define ES1371_REG_ADC_FRAMEADR   0xd30
#define ES1371_REG_ADC_FRAMECNT   0xd34

#define ES1371_FMT_U8_MONO     0
#define ES1371_FMT_U8_STEREO   1
#define ES1371_FMT_S16_MONO    2
#define ES1371_FMT_S16_STEREO  3
#define ES1371_FMT_STEREO      1
#define ES1371_FMT_S16         2
#define ES1371_FMT_MASK        3

static const unsigned sample_size[] = { 1, 2, 2, 4 };
static const unsigned sample_shift[] = { 0, 1, 1, 2 };

#define CTRL_RECEN_B    0x08000000  /* 1 = don't mix analog in to digital out */
#define CTRL_SPDIFEN_B  0x04000000
#define CTRL_JOY_SHIFT  24
#define CTRL_JOY_MASK   3
#define CTRL_JOY_200    0x00000000  /* joystick base address */
#define CTRL_JOY_208    0x01000000
#define CTRL_JOY_210    0x02000000
#define CTRL_JOY_218    0x03000000
#define CTRL_GPIO_IN0   0x00100000  /* general purpose inputs/outputs */
#define CTRL_GPIO_IN1   0x00200000
#define CTRL_GPIO_IN2   0x00400000
#define CTRL_GPIO_IN3   0x00800000
#define CTRL_GPIO_OUT0  0x00010000
#define CTRL_GPIO_OUT1  0x00020000
#define CTRL_GPIO_OUT2  0x00040000
#define CTRL_GPIO_OUT3  0x00080000
#define CTRL_MSFMTSEL   0x00008000  /* MPEG serial data fmt: 0 = Sony, 1 = I2S */
#define CTRL_SYNCRES    0x00004000  /* AC97 warm reset */
#define CTRL_ADCSTOP    0x00002000  /* stop ADC transfers */
#define CTRL_PWR_INTRM  0x00001000  /* 1 = power level ints enabled */
#define CTRL_M_CB       0x00000800  /* recording source: 0 = ADC, 1 = MPEG */
#define CTRL_CCB_INTRM  0x00000400  /* 1 = CCB "voice" ints enabled */
#define CTRL_PDLEV0     0x00000000  /* power down level */
#define CTRL_PDLEV1     0x00000100
#define CTRL_PDLEV2     0x00000200
#define CTRL_PDLEV3     0x00000300
#define CTRL_BREQ       0x00000080  /* 1 = test mode (internal mem test) */
#define CTRL_DAC1_EN    0x00000040  /* enable DAC1 */
#define CTRL_DAC2_EN    0x00000020  /* enable DAC2 */
#define CTRL_ADC_EN     0x00000010  /* enable ADC */
#define CTRL_UART_EN    0x00000008  /* enable MIDI uart */
#define CTRL_JYSTK_EN   0x00000004  /* enable Joystick port */
#define CTRL_XTALCLKDIS 0x00000002  /* 1 = disable crystal clock input */
#define CTRL_PCICLKDIS  0x00000001  /* 1 = disable PCI clock distribution */


#define STAT_INTR       0x80000000  /* wired or of all interrupt bits */
#define CSTAT_5880_AC97_RST 0x20000000 /* CT5880 Reset bit */
#define STAT_EN_SPDIF   0x00040000  /* enable S/PDIF circuitry */
#define STAT_TS_SPDIF   0x00020000  /* test S/PDIF circuitry */
#define STAT_TESTMODE   0x00010000  /* test ASIC */
#define STAT_SYNC_ERR   0x00000100  /* 1 = codec sync error */
#define STAT_VC         0x000000c0  /* CCB int source, 0=DAC1, 1=DAC2, 2=ADC, 3=undef */
#define STAT_SH_VC      6
#define STAT_MPWR       0x00000020  /* power level interrupt */
#define STAT_MCCB       0x00000010  /* CCB int pending */
#define STAT_UART       0x00000008  /* UART int pending */
#define STAT_DAC1       0x00000004  /* DAC1 int pending */
#define STAT_DAC2       0x00000002  /* DAC2 int pending */
#define STAT_ADC        0x00000001  /* ADC int pending */

#define USTAT_RXINT     0x80        /* UART rx int pending */
#define USTAT_TXINT     0x04        /* UART tx int pending */
#define USTAT_TXRDY     0x02        /* UART tx ready */
#define USTAT_RXRDY     0x01        /* UART rx ready */

#define UCTRL_RXINTEN   0x80        /* 1 = enable RX ints */
#define UCTRL_TXINTEN   0x60        /* TX int enable field mask */
#define UCTRL_ENA_TXINT 0x20        /* enable TX int */
#define UCTRL_CNTRL     0x03        /* control field */
#define UCTRL_CNTRL_SWR 0x03        /* software reset command */

/* sample rate converter */
#define SRC_OKSTATE        1

#define SRC_RAMADDR_MASK   0xfe000000
#define SRC_RAMADDR_SHIFT  25
#define SRC_DAC1FREEZE     (1UL << 21)
#define SRC_DAC2FREEZE      (1UL << 20)
#define SRC_ADCFREEZE      (1UL << 19)


#define SRC_WE             0x01000000  /* read/write control for SRC RAM */
#define SRC_BUSY           0x00800000  /* SRC busy */
#define SRC_DIS            0x00400000  /* 1 = disable SRC */
#define SRC_DDAC1          0x00200000  /* 1 = disable accum update for DAC1 */
#define SRC_DDAC2          0x00100000  /* 1 = disable accum update for DAC2 */
#define SRC_DADC           0x00080000  /* 1 = disable accum update for ADC2 */
#define SRC_CTLMASK        0x00780000
#define SRC_RAMDATA_MASK   0x0000ffff
#define SRC_RAMDATA_SHIFT  0

#define SRCREG_ADC      0x78
#define SRCREG_DAC1     0x70
#define SRCREG_DAC2     0x74
#define SRCREG_VOL_ADC  0x6c
#define SRCREG_VOL_DAC1 0x7c
#define SRCREG_VOL_DAC2 0x7e

#define SRCREG_TRUNC_N     0x00
#define SRCREG_INT_REGS    0x01
#define SRCREG_ACCUM_FRAC  0x02
#define SRCREG_VFREQ_FRAC  0x03

#define CODEC_PIRD        0x00800000  /* 0 = write AC97 register */
#define CODEC_PIADD_MASK  0x007f0000
#define CODEC_PIADD_SHIFT 16
#define CODEC_PIDAT_MASK  0x0000ffff
#define CODEC_PIDAT_SHIFT 0

#define CODEC_RDY         0x80000000  /* AC97 read data valid */
#define CODEC_WIP         0x40000000  /* AC97 write in progress */
#define CODEC_PORD        0x00800000  /* 0 = write AC97 register */
#define CODEC_POADD_MASK  0x007f0000
#define CODEC_POADD_SHIFT 16
#define CODEC_PODAT_MASK  0x0000ffff
#define CODEC_PODAT_SHIFT 0


#define LEGACY_JFAST      0x80000000  /* fast joystick timing */
#define LEGACY_FIRQ       0x01000000  /* force IRQ */

#define SCTRL_DACTEST     0x00400000  /* 1 = DAC test, test vector generation purposes */
#define SCTRL_P2ENDINC    0x00380000  /*  */
#define SCTRL_SH_P2ENDINC 19
#define SCTRL_P2STINC     0x00070000  /*  */
#define SCTRL_SH_P2STINC  16
#define SCTRL_R1LOOPSEL   0x00008000  /* 0 = loop mode */
#define SCTRL_P2LOOPSEL   0x00004000  /* 0 = loop mode */
#define SCTRL_P1LOOPSEL   0x00002000  /* 0 = loop mode */
#define SCTRL_P2PAUSE     0x00001000  /* 1 = pause mode */
#define SCTRL_P1PAUSE     0x00000800  /* 1 = pause mode */
#define SCTRL_R1INTEN     0x00000400  /* enable interrupt */
#define SCTRL_P2INTEN     0x00000200  /* enable interrupt */
#define SCTRL_P1INTEN     0x00000100  /* enable interrupt */
#define SCTRL_P1SCTRLD    0x00000080  /* reload sample count register for DAC1 */
#define SCTRL_P2DACSEN    0x00000040  /* 1 = DAC2 play back last sample when disabled */
#define SCTRL_R1SEB       0x00000020  /* 1 = 16bit */
#define SCTRL_R1SMB       0x00000010  /* 1 = stereo */
#define SCTRL_R1FMT       0x00000030  /* format mask */
#define SCTRL_SH_R1FMT    4
#define SCTRL_P2SEB       0x00000008  /* 1 = 16bit */
#define SCTRL_P2SMB       0x00000004  /* 1 = stereo */
#define SCTRL_P2FMT       0x0000000c  /* format mask */
#define SCTRL_SH_P2FMT    2
#define SCTRL_P1SEB       0x00000002  /* 1 = 16bit */
#define SCTRL_P1SMB       0x00000001  /* 1 = stereo */
#define SCTRL_P1FMT       0x00000003  /* format mask */
#define SCTRL_SH_P1FMT    0


/* misc stuff */
#define POLL_COUNT   0x1000
#define FMODE_DAC         4           /* slight misuse of mode_t */

/* MIDI buffer sizes */

#define MIDIINBUF  256
#define MIDIOUTBUF 256

#define FMODE_MIDI_SHIFT 3
#define FMODE_MIDI_READ  (FMODE_READ << FMODE_MIDI_SHIFT)
#define FMODE_MIDI_WRITE (FMODE_WRITE << FMODE_MIDI_SHIFT)

#define ES1371_MODULE_NAME "es1371"
#define PFX ES1371_MODULE_NAME ": "

#define DMABUF_DEFAULTORDER (17-PAGE_SHIFT)
#define DMABUF_MINORDER 1


/* --------------------------------------------------------------------- */

/* AtheOS Adds */

static void *es1371_driver_data;

/* --------------------------------------------------------------------- */

struct es1371_state {
	/* magic */
	unsigned int magic;
   
	/* the corresponding pci_dev structure */
        PCI_Info_s *dev;

	/* soundcore stuff */
	int dev_audio;
	int dev_dac;
	int dev_midi;
	
	/* hardware resources */
	unsigned long io; /* long for SPARC */
	unsigned int irq;

	/* PCI ID's */
	u16 vendor;
	u16 device;
        u8 rev; /* the chip revision */

	/* options */
	int spdif_volume; /* S/PDIF output is enabled if != -1 */

	struct ac97_codec codec;
        bool open;

	/* wave stuff */
	unsigned ctrl;
	unsigned sctrl;
	unsigned dac1rate, dac2rate, adcrate;

	SpinLock_s lock;
	sem_id open_sem;
   
	struct dmabuf {
		void *rawbuf;
		dma_addr_t dmaaddr;
		unsigned buforder;
		unsigned numfrag;
		unsigned fragshift;
		unsigned hwptr, swptr;
		unsigned total_bytes;
		int count;
		unsigned error; /* over/underrun */
		sem_id wait;
		/* redundant, but makes calculations easier */
		unsigned fragsize;
		unsigned dmasize;
		unsigned fragsamples;
		/* OSS stuff */
		unsigned mapped:1;
		unsigned ready:1;
		unsigned endcleared:1;
		unsigned enabled:1;
		unsigned ossfragshift;
		int ossmaxfrags;
		unsigned subdivision;
	} dma_dac1, dma_dac2, dma_adc;

    	sem_id sem;
};

/* --------------------------------------------------------------------- */

struct initvol {
	int mixch;
	int vol;
} initvol[] = {
	{ SOUND_MIXER_WRITE_LINE, 0x4040 },
	{ SOUND_MIXER_WRITE_CD, 0x4040 },
	{ MIXER_WRITE(SOUND_MIXER_VIDEO), 0x4040 },
	{ SOUND_MIXER_WRITE_LINE1, 0x4040 },
	{ SOUND_MIXER_WRITE_PCM, 0x4040 },
	{ SOUND_MIXER_WRITE_VOLUME, 0x4040 },
	{ MIXER_WRITE(SOUND_MIXER_PHONEOUT), 0x4040 },
	{ SOUND_MIXER_WRITE_OGAIN, 0x4040 },
	{ MIXER_WRITE(SOUND_MIXER_PHONEIN), 0x4040 },
	{ SOUND_MIXER_WRITE_SPEAKER, 0x4040 },
	{ SOUND_MIXER_WRITE_MIC, 0x4040 },
	{ SOUND_MIXER_WRITE_RECLEV, 0x4040 },
	{ SOUND_MIXER_WRITE_IGAIN, 0x4040 }
};

/* --------------------------------------------------------------------- */

#define NR_DEVICE 5

static int joystick[NR_DEVICE] = { 0, };
static int spdif[NR_DEVICE] = { 0, };
static int nomix[NR_DEVICE] = { 0, };

static unsigned int devindex = 0;
//static int amplifier = 0;

static const char invalid_magic[] = "invalid magic value\n";

#define VALIDATE_STATE(s)                         \
({                                                \
	if (!(s) || (s)->magic != ES1371_MAGIC) { \
		printk(invalid_magic);            \
		return -ENXIO;                    \
	}                                         \
})

/* --------------------------------------------------------------------- */


static inline void dealloc_dmabuf(struct es1371_state *s, struct dmabuf *db)
{
	if (db->rawbuf) {
		pci_free_consistent(s->dev, PAGE_SIZE << db->buforder, db->rawbuf, db->dmaaddr);
	}
	db->rawbuf = NULL;
	db->mapped = db->ready = 0;
}


static inline unsigned ld2(unsigned int x)
{
	unsigned r = 0;
	
	if (x >= 0x10000) {
		x >>= 16;
		r += 16;
	}
	if (x >= 0x100) {
		x >>= 8;
		r += 8;
	}
	if (x >= 0x10) {
		x >>= 4;
		r += 4;
	}
	if (x >= 4) {
		x >>= 2;
		r += 2;
	}
	if (x >= 2)
		r++;
	return r;
}

static unsigned wait_src_ready(struct es1371_state *s)
{
	unsigned int t, r;

	for (t = 0; t < POLL_COUNT; t++) {
		if (!((r = inl(s->io + ES1371_REG_SRCONV)) & SRC_BUSY))
			return r;
		udelay(1);
	}
	printk("sample rate converter timeout r = 0x%08x\n", r);
	return r;
}

static unsigned src_read(struct es1371_state *s, unsigned reg)
{
        unsigned int temp,i,orig;

        /* wait for ready */
        temp = wait_src_ready (s);

        /* we can only access the SRC at certain times, make sure
           we're allowed to before we read */
           
        orig = temp;
        /* expose the SRC state bits */
        outl ( (temp & SRC_CTLMASK) | (reg << SRC_RAMADDR_SHIFT) | 0x10000UL,
               s->io + ES1371_REG_SRCONV);

        /* now, wait for busy and the correct time to read */
        temp = wait_src_ready (s);

        if ( (temp & 0x00870000UL ) != ( SRC_OKSTATE << 16 )){
                /* wait for the right state */
                for (i=0; i<POLL_COUNT; i++){
                        temp = inl (s->io + ES1371_REG_SRCONV);
                        if ( (temp & 0x00870000UL ) == ( SRC_OKSTATE << 16 ))
                                break;
                }
        }

        /* hide the state bits */
        outl ((orig & SRC_CTLMASK) | (reg << SRC_RAMADDR_SHIFT), s->io + ES1371_REG_SRCONV);
        return temp;
                        
                
}


static void src_write(struct es1371_state *s, unsigned reg, unsigned data)
{
      
	unsigned int r;

	r = wait_src_ready(s) & (SRC_DIS | SRC_DDAC1 | SRC_DDAC2 | SRC_DADC);
	r |= (reg << SRC_RAMADDR_SHIFT) & SRC_RAMADDR_MASK;
	r |= (data << SRC_RAMDATA_SHIFT) & SRC_RAMDATA_MASK;
	outl(r | SRC_WE, s->io + ES1371_REG_SRCONV);

}

/* --------------------------------------------------------------------- */

/* most of the following here is black magic */
static void set_adc_rate(struct es1371_state *s, unsigned rate)
{
	unsigned long flags;
	unsigned int n, truncm, freq;

	if (rate > 48000)
		rate = 48000;
	if (rate < 4000)
		rate = 4000;
	n = rate / 3000;
	if ((1 << n) & ((1 << 15) | (1 << 13) | (1 << 11) | (1 << 9)))
		n--;
	truncm = (21 * n - 1) | 1;
        freq = ((48000UL << 15) / rate) * n;
	s->adcrate = (48000UL << 15) / (freq / n);
	spin_lock_irqsave(&s->lock, flags);
	if (rate >= 24000) {
		if (truncm > 239)
			truncm = 239;
		src_write(s, SRCREG_ADC+SRCREG_TRUNC_N, 
			  (((239 - truncm) >> 1) << 9) | (n << 4));
	} else {
		if (truncm > 119)
			truncm = 119;
		src_write(s, SRCREG_ADC+SRCREG_TRUNC_N, 
			  0x8000 | (((119 - truncm) >> 1) << 9) | (n << 4));
	}		
	src_write(s, SRCREG_ADC+SRCREG_INT_REGS, 
		  (src_read(s, SRCREG_ADC+SRCREG_INT_REGS) & 0x00ff) |
		  ((freq >> 5) & 0xfc00));
	src_write(s, SRCREG_ADC+SRCREG_VFREQ_FRAC, freq & 0x7fff);
	src_write(s, SRCREG_VOL_ADC, n << 8);
	src_write(s, SRCREG_VOL_ADC+1, n << 8);
	spin_unlock_irqrestore(&s->lock, flags);
}


static void set_dac1_rate(struct es1371_state *s, unsigned rate)
{
	unsigned long flags;
	unsigned int freq, r;

	if (rate > 48000)
		rate = 48000;
	if (rate < 4000)
		rate = 4000;
        freq = ((rate << 15) + 1500) / 3000;
	s->dac1rate = (freq * 3000 + 16384) >> 15;
	spin_lock_irqsave(&s->lock, flags);
	r = (wait_src_ready(s) & (SRC_DIS | SRC_DDAC2 | SRC_DADC)) | SRC_DDAC1;
	outl(r, s->io + ES1371_REG_SRCONV);
	src_write(s, SRCREG_DAC1+SRCREG_INT_REGS, 
		  (src_read(s, SRCREG_DAC1+SRCREG_INT_REGS) & 0x00ff) |
		  ((freq >> 5) & 0xfc00));
	src_write(s, SRCREG_DAC1+SRCREG_VFREQ_FRAC, freq & 0x7fff);
	r = (wait_src_ready(s) & (SRC_DIS | SRC_DDAC2 | SRC_DADC));
	outl(r, s->io + ES1371_REG_SRCONV);
	spin_unlock_irqrestore(&s->lock, flags);
}

static void set_dac2_rate(struct es1371_state *s, unsigned rate)
{
	unsigned long flags;
	unsigned int freq, r;

	if (rate > 48000)
		rate = 48000;
	if (rate < 4000)
		rate = 4000;
        freq = ((rate << 15) + 1500) / 3000;
	s->dac2rate = (freq * 3000 + 16384) >> 15;
	spin_lock_irqsave(&s->lock, flags);
	r = (wait_src_ready(s) & (SRC_DIS | SRC_DDAC1 | SRC_DADC)) | SRC_DDAC2;
	outl(r, s->io + ES1371_REG_SRCONV);
	src_write(s, SRCREG_DAC2+SRCREG_INT_REGS, 
		  (src_read(s, SRCREG_DAC2+SRCREG_INT_REGS) & 0x00ff) |
		  ((freq >> 5) & 0xfc00));
	src_write(s, SRCREG_DAC2+SRCREG_VFREQ_FRAC, freq & 0x7fff);
	r = (wait_src_ready(s) & (SRC_DIS | SRC_DDAC1 | SRC_DADC));
	outl(r, s->io + ES1371_REG_SRCONV);
	spin_unlock_irqrestore(&s->lock, flags);
}


static void src_init(struct es1371_state *s)
{
        unsigned int i;

        /* before we enable or disable the SRC we need
           to wait for it to become ready */
        wait_src_ready(s);

        outl(SRC_DIS, s->io + ES1371_REG_SRCONV);

        for (i = 0; i < 0x80; i++)
                src_write(s, i, 0);

        src_write(s, SRCREG_DAC1+SRCREG_TRUNC_N, 16 << 4);
        src_write(s, SRCREG_DAC1+SRCREG_INT_REGS, 16 << 10);
        src_write(s, SRCREG_DAC2+SRCREG_TRUNC_N, 16 << 4);
        src_write(s, SRCREG_DAC2+SRCREG_INT_REGS, 16 << 10);
        src_write(s, SRCREG_VOL_ADC, 1 << 12);
        src_write(s, SRCREG_VOL_ADC+1, 1 << 12);
        src_write(s, SRCREG_VOL_DAC1, 1 << 12);
        src_write(s, SRCREG_VOL_DAC1+1, 1 << 12);
        src_write(s, SRCREG_VOL_DAC2, 1 << 12);
        src_write(s, SRCREG_VOL_DAC2+1, 1 << 12);
        set_adc_rate(s, 22050);
        set_dac1_rate(s, 22050);
        set_dac2_rate(s, 22050);

        /* WARNING:
         * enabling the sample rate converter without properly programming
         * its parameters causes the chip to lock up (the SRC busy bit will
         * be stuck high, and I've found no way to rectify this other than
         * power cycle)
         */
        wait_src_ready(s);
        outl(0, s->io+ES1371_REG_SRCONV);
}


static u16 rdcodec(struct ac97_codec *codec, u8 addr)
{
	struct es1371_state *s = (struct es1371_state *)codec->private_data;
	unsigned long flags;
	unsigned t, x;

        /* wait for WIP to go away */
	for (t = 0; t < 0x1000; t++)
		if (!(inl(s->io+ES1371_REG_CODEC) & CODEC_WIP))
			break;
	spin_lock_irqsave(&s->lock, flags);

	/* save the current state for later */
	x = (wait_src_ready(s) & (SRC_DIS | SRC_DDAC1 | SRC_DDAC2 | SRC_DADC));

	/* enable SRC state data in SRC mux */
	outl( x | 0x00010000,
              s->io+ES1371_REG_SRCONV);

        /* wait for not busy (state 0) first to avoid
           transition states */
        for (t=0; t<POLL_COUNT; t++){
                if((inl(s->io+ES1371_REG_SRCONV) & 0x00870000) ==0 )
                    break;
                udelay(1);
        }
        
        /* wait for a SAFE time to write addr/data and then do it, dammit */
        for (t=0; t<POLL_COUNT; t++){
                if((inl(s->io+ES1371_REG_SRCONV) & 0x00870000) ==0x00010000)
                    break;
                udelay(1);
        }

	outl(((addr << CODEC_POADD_SHIFT) & CODEC_POADD_MASK) | CODEC_PORD, s->io+ES1371_REG_CODEC);
	/* restore SRC reg */
	wait_src_ready(s);
	outl(x, s->io+ES1371_REG_SRCONV);
	spin_unlock_irqrestore(&s->lock, flags);

        /* wait for WIP again */
	for (t = 0; t < 0x1000; t++)
		if (!(inl(s->io+ES1371_REG_CODEC) & CODEC_WIP))
			break;
        
	/* now wait for the stinkin' data (RDY) */
	for (t = 0; t < POLL_COUNT; t++)
		if ((x = inl(s->io+ES1371_REG_CODEC)) & CODEC_RDY)
			break;
        
	return ((x & CODEC_PIDAT_MASK) >> CODEC_PIDAT_SHIFT);
}

static void wrcodec(struct ac97_codec *codec, u8 addr, u16 data)
{
	struct es1371_state *s = (struct es1371_state *)codec->private_data;
	unsigned long flags;
	unsigned t, x;
        
	for (t = 0; t < POLL_COUNT; t++)
		if (!(inl(s->io+ES1371_REG_CODEC) & CODEC_WIP))
			break;
	spin_lock_irqsave(&s->lock, flags);

        /* save the current state for later */
        x = wait_src_ready(s);

        /* enable SRC state data in SRC mux */
	outl((x & (SRC_DIS | SRC_DDAC1 | SRC_DDAC2 | SRC_DADC)) | 0x00010000,
	     s->io+ES1371_REG_SRCONV);

        /* wait for not busy (state 0) first to avoid
           transition states */
        for (t=0; t<POLL_COUNT; t++){
                if((inl(s->io+ES1371_REG_SRCONV) & 0x00870000) ==0 )
                    break;
                udelay(1);
        }
        
        /* wait for a SAFE time to write addr/data and then do it, dammit */
        for (t=0; t<POLL_COUNT; t++){
                if((inl(s->io+ES1371_REG_SRCONV) & 0x00870000) ==0x00010000)
                    break;
                udelay(1);
        }

	outl(((addr << CODEC_POADD_SHIFT) & CODEC_POADD_MASK) |
	     ((data << CODEC_PODAT_SHIFT) & CODEC_PODAT_MASK), s->io+ES1371_REG_CODEC);

	/* restore SRC reg */
	wait_src_ready(s);
	outl(x, s->io+ES1371_REG_SRCONV);
	spin_unlock_irqrestore(&s->lock, flags);
}

static inline void stop_adc(struct es1371_state *s)
{
	unsigned long flags;

	spin_lock_irqsave(&s->lock, flags);
	s->ctrl &= ~CTRL_ADC_EN;
	outl(s->ctrl, s->io+ES1371_REG_CONTROL);
	spin_unlock_irqrestore(&s->lock, flags);
}	

static inline void stop_dac1(struct es1371_state *s)
{
	unsigned long flags;

	spin_lock_irqsave(&s->lock, flags);
	s->ctrl &= ~CTRL_DAC1_EN;
	outl(s->ctrl, s->io+ES1371_REG_CONTROL);
	spin_unlock_irqrestore(&s->lock, flags);
}	

static inline void stop_dac2(struct es1371_state *s)
{
	unsigned long flags;

	spin_lock_irqsave(&s->lock, flags);
	s->ctrl &= ~CTRL_DAC2_EN;
	outl(s->ctrl, s->io+ES1371_REG_CONTROL);
	spin_unlock_irqrestore(&s->lock, flags);
}	

#if 0
static void start_dac1(struct es1371_state *s)
{
	unsigned long flags;
	unsigned fragremain, fshift;

	spin_lock_irqsave(&s->lock, flags);
	if (!(s->ctrl & CTRL_DAC1_EN) && (s->dma_dac1.mapped || s->dma_dac1.count > 0)
	    && s->dma_dac1.ready) {
		s->ctrl |= CTRL_DAC1_EN;
		s->sctrl = (s->sctrl & ~(SCTRL_P1LOOPSEL | SCTRL_P1PAUSE | SCTRL_P1SCTRLD)) | SCTRL_P1INTEN;
		outl(s->sctrl, s->io+ES1371_REG_SERIAL_CONTROL);
		fragremain = ((- s->dma_dac1.hwptr) & (s->dma_dac1.fragsize-1));
		fshift = sample_shift[(s->sctrl & SCTRL_P1FMT) >> SCTRL_SH_P1FMT];
		if (fragremain < 2*fshift)
			fragremain = s->dma_dac1.fragsize;
		outl((fragremain >> fshift) - 1, s->io+ES1371_REG_DAC1_SCOUNT);
		outl(s->ctrl, s->io+ES1371_REG_CONTROL);
		outl((s->dma_dac1.fragsize >> fshift) - 1, s->io+ES1371_REG_DAC1_SCOUNT);
	}
	spin_unlock_irqrestore(&s->lock, flags);
}	
#endif

static void start_dac2(struct es1371_state *s)
{
	unsigned long flags;
	unsigned fragremain, fshift;

	spin_lock_irqsave(&s->lock, flags);
	if (!(s->ctrl & CTRL_DAC2_EN) && (s->dma_dac2.mapped || s->dma_dac2.count > 0)
	    && s->dma_dac2.ready) {
		s->ctrl |= CTRL_DAC2_EN;
		s->sctrl = (s->sctrl & ~(SCTRL_P2LOOPSEL | SCTRL_P2PAUSE | SCTRL_P2DACSEN | 
					 SCTRL_P2ENDINC | SCTRL_P2STINC)) | SCTRL_P2INTEN |
			(((s->sctrl & SCTRL_P2FMT) ? 2 : 1) << SCTRL_SH_P2ENDINC) | 
			(0 << SCTRL_SH_P2STINC);
		outl(s->sctrl, s->io+ES1371_REG_SERIAL_CONTROL);
		fragremain = ((- s->dma_dac2.hwptr) & (s->dma_dac2.fragsize-1));
		fshift = sample_shift[(s->sctrl & SCTRL_P2FMT) >> SCTRL_SH_P2FMT];
		if (fragremain < 2*fshift)
			fragremain = s->dma_dac2.fragsize;
		outl((fragremain >> fshift) - 1, s->io+ES1371_REG_DAC2_SCOUNT);
		outl(s->ctrl, s->io+ES1371_REG_CONTROL);
		outl((s->dma_dac2.fragsize >> fshift) - 1, s->io+ES1371_REG_DAC2_SCOUNT);
	}
	spin_unlock_irqrestore(&s->lock, flags);
}	

#if 0
static void start_adc(struct es1371_state *s)
{
	unsigned long flags;
	unsigned fragremain, fshift;

	spin_lock_irqsave(&s->lock, flags);
	if (!(s->ctrl & CTRL_ADC_EN) && (s->dma_adc.mapped || s->dma_adc.count < (signed)(s->dma_adc.dmasize - 2*s->dma_adc.fragsize))
	    && s->dma_adc.ready) {
		s->ctrl |= CTRL_ADC_EN;
		s->sctrl = (s->sctrl & ~SCTRL_R1LOOPSEL) | SCTRL_R1INTEN;
		outl(s->sctrl, s->io+ES1371_REG_SERIAL_CONTROL);
		fragremain = ((- s->dma_adc.hwptr) & (s->dma_adc.fragsize-1));
		fshift = sample_shift[(s->sctrl & SCTRL_R1FMT) >> SCTRL_SH_R1FMT];
		if (fragremain < 2*fshift)
			fragremain = s->dma_adc.fragsize;
		outl((fragremain >> fshift) - 1, s->io+ES1371_REG_ADC_SCOUNT);
		outl(s->ctrl, s->io+ES1371_REG_CONTROL);
		outl((s->dma_adc.fragsize >> fshift) - 1, s->io+ES1371_REG_ADC_SCOUNT);
	}
	spin_unlock_irqrestore(&s->lock, flags);
}	
#endif

static int prog_dmabuf(struct es1371_state *s, struct dmabuf *db, unsigned rate, unsigned fmt, unsigned reg)
{
	int order;
	unsigned bytepersec;
	unsigned bufs;

	db->hwptr = db->swptr = db->total_bytes = db->count = db->error = db->endcleared = 0;
	if (!db->rawbuf) {
		db->ready = db->mapped = 0;
		for (order = DMABUF_DEFAULTORDER; order >= DMABUF_MINORDER; order--)
			if ((db->rawbuf = pci_alloc_consistent(s->dev, PAGE_SIZE << order, &db->dmaaddr)))
				break;
		if (!db->rawbuf)
			return -ENOMEM;
		db->buforder = order;
	}
	fmt &= ES1371_FMT_MASK;
	bytepersec = rate << sample_shift[fmt];
	bufs = PAGE_SIZE << db->buforder;
	if (db->ossfragshift) {
		if ((1000 << db->ossfragshift) < bytepersec)
			db->fragshift = ld2(bytepersec/1000);
		else
			db->fragshift = db->ossfragshift;
	} else {
		db->fragshift = ld2(bytepersec/100/(db->subdivision ? db->subdivision : 1));
		if (db->fragshift < 3)
			db->fragshift = 3;
	}
	db->numfrag = bufs >> db->fragshift;
	while (db->numfrag < 4 && db->fragshift > 3) {
		db->fragshift--;
		db->numfrag = bufs >> db->fragshift;
	}
	db->fragsize = 1 << db->fragshift;
	if (db->ossmaxfrags >= 4 && db->ossmaxfrags < db->numfrag)
		db->numfrag = db->ossmaxfrags;
	db->fragsamples = db->fragsize >> sample_shift[fmt];
	db->dmasize = db->numfrag << db->fragshift;
	memset(db->rawbuf, (fmt & ES1371_FMT_S16) ? 0 : 0x80, db->dmasize);
	outl((reg >> 8) & 15, s->io+ES1371_REG_MEMPAGE);
	outl(db->dmaaddr, s->io+(reg & 0xff));
	outl((db->dmasize >> 2)-1, s->io+((reg + 4) & 0xff));
	db->enabled = 1;
	db->ready = 1;
	return 0;
}

static inline int prog_dmabuf_adc(struct es1371_state *s)
{
	stop_adc(s);
	return prog_dmabuf(s, &s->dma_adc, s->adcrate, (s->sctrl >> SCTRL_SH_R1FMT) & ES1371_FMT_MASK, 
			   ES1371_REG_ADC_FRAMEADR);
}

static inline int prog_dmabuf_dac2(struct es1371_state *s)
{
	stop_dac2(s);
	return prog_dmabuf(s, &s->dma_dac2, s->dac2rate, (s->sctrl >> SCTRL_SH_P2FMT) & ES1371_FMT_MASK, 
			   ES1371_REG_DAC2_FRAMEADR);
}

static inline int prog_dmabuf_dac1(struct es1371_state *s)
{
	stop_dac1(s);
	return prog_dmabuf(s, &s->dma_dac1, s->dac1rate, (s->sctrl >> SCTRL_SH_P1FMT) & ES1371_FMT_MASK,
			   ES1371_REG_DAC1_FRAMEADR);
}

static inline void clear_advance(void *buf, unsigned bsize, unsigned bptr, unsigned len, unsigned char c)
{
	if (bptr + len > bsize) {
		unsigned x = bsize - bptr;
		memset(((char *)buf) + bptr, c, x);
		bptr = 0;
		len -= x;
	}
	memset(((char *)buf) + bptr, c, len);
}

static inline unsigned get_hwptr(struct es1371_state *s, struct dmabuf *db, unsigned reg)
{
	unsigned hwptr, diff;

	outl((reg >> 8) & 15, s->io+ES1371_REG_MEMPAGE);
	hwptr = (inl(s->io+(reg & 0xff)) >> 14) & 0x3fffc;
	diff = (db->dmasize + hwptr - db->hwptr) % db->dmasize;
	db->hwptr = hwptr;
	return diff;
}

static void es1371_update_ptr(struct es1371_state *s)
{
	int diff;

	/* update ADC pointer */
	if (s->ctrl & CTRL_ADC_EN) {
		diff = get_hwptr(s, &s->dma_adc, ES1371_REG_ADC_FRAMECNT);
		s->dma_adc.total_bytes += diff;
		s->dma_adc.count += diff;
		if (s->dma_adc.count >= (signed)s->dma_adc.fragsize) 
			wakeup_sem(s->dma_adc.wait, true);
		if (!s->dma_adc.mapped) {
			if (s->dma_adc.count > (signed)(s->dma_adc.dmasize - ((3 * s->dma_adc.fragsize) >> 1))) {
				s->ctrl &= ~CTRL_ADC_EN;
				outl(s->ctrl, s->io+ES1371_REG_CONTROL);
				s->dma_adc.error++;
			}
		}
	}
	/* update DAC1 pointer */
	if (s->ctrl & CTRL_DAC1_EN) {
		diff = get_hwptr(s, &s->dma_dac1, ES1371_REG_DAC1_FRAMECNT);
		s->dma_dac1.total_bytes += diff;
		if (s->dma_dac1.mapped) {
			s->dma_dac1.count += diff;
			if (s->dma_dac1.count >= (signed)s->dma_dac1.fragsize)
				wakeup_sem(s->dma_dac1.wait, true);
		} else {
			s->dma_dac1.count -= diff;
			if (s->dma_dac1.count <= 0) {
				s->ctrl &= ~CTRL_DAC1_EN;
				outl(s->ctrl, s->io+ES1371_REG_CONTROL);
				s->dma_dac1.error++;
			} else if (s->dma_dac1.count <= (signed)s->dma_dac1.fragsize && !s->dma_dac1.endcleared) {
				clear_advance(s->dma_dac1.rawbuf, s->dma_dac1.dmasize, s->dma_dac1.swptr, 
					      s->dma_dac1.fragsize, (s->sctrl & SCTRL_P1SEB) ? 0 : 0x80);
				s->dma_dac1.endcleared = 1;
			}
			if (s->dma_dac1.count + (signed)s->dma_dac1.fragsize <= (signed)s->dma_dac1.dmasize)
				wakeup_sem(s->dma_dac1.wait, true);
		}
	}
	/* update DAC2 pointer */
	if (s->ctrl & CTRL_DAC2_EN) {
		diff = get_hwptr(s, &s->dma_dac2, ES1371_REG_DAC2_FRAMECNT);
		s->dma_dac2.total_bytes += diff;
		if (s->dma_dac2.mapped) {
			s->dma_dac2.count += diff;
			if (s->dma_dac2.count >= (signed)s->dma_dac2.fragsize)
				wakeup_sem(s->dma_dac2.wait, true);
		} else {
			s->dma_dac2.count -= diff;
			if (s->dma_dac2.count <= 0) {
				s->ctrl &= ~CTRL_DAC2_EN;
				outl(s->ctrl, s->io+ES1371_REG_CONTROL);
				s->dma_dac2.error++;
			} else if (s->dma_dac2.count <= (signed)s->dma_dac2.fragsize && !s->dma_dac2.endcleared) {
				clear_advance(s->dma_dac2.rawbuf, s->dma_dac2.dmasize, s->dma_dac2.swptr, 
					      s->dma_dac2.fragsize, (s->sctrl & SCTRL_P2SEB) ? 0 : 0x80);
				s->dma_dac2.endcleared = 1;
			}
			if (s->dma_dac2.count + (signed)s->dma_dac2.fragsize <= (signed)s->dma_dac2.dmasize)
				wakeup_sem(s->dma_dac2.wait, true);
		}
	}
}


static int es1371_interrupt(int irq, void *dev_id, SysCallRegs_s *regs)
{
        struct es1371_state *s = (struct es1371_state *)dev_id;
	unsigned int intsrc, sctl;
	
	/* fastpath out, to ease interrupt sharing */
	intsrc = inl(s->io+ES1371_REG_STATUS);
	if (!(intsrc & 0x80000000))
		return 0;
	spin_lock(&s->lock);
	/* clear audio interrupts first */
	sctl = s->sctrl;
	if (intsrc & STAT_ADC)
		sctl &= ~SCTRL_R1INTEN;
	if (intsrc & STAT_DAC1)
		sctl &= ~SCTRL_P1INTEN;
	if (intsrc & STAT_DAC2)
		sctl &= ~SCTRL_P2INTEN;
	outl(sctl, s->io+ES1371_REG_SERIAL_CONTROL);
	outl(s->sctrl, s->io+ES1371_REG_SERIAL_CONTROL);
	es1371_update_ptr(s);
	spin_unlock(&s->lock);
        return 0;
}


static int mixdev_ioctl(struct ac97_codec *codec, unsigned int cmd, unsigned long arg)
{
	struct es1371_state *s = (struct es1371_state *)codec->private_data;

	VALIDATE_STATE(s);
	/* filter mixer ioctls to catch PCM and MASTER volume when in S/PDIF mode */
	if (s->spdif_volume == -1)
		return codec->mixer_ioctl(codec, cmd, arg);
	switch (cmd) {
	case SOUND_MIXER_WRITE_VOLUME:
		return 0;

	case SOUND_MIXER_WRITE_PCM:   /* use SRC for PCM volume */
	        /*get_user(val, (int *)arg);
		right = ((val >> 8)  & 0xff);
		left = (val  & 0xff);
		if (right > 100)
			right = 100;
		if (left > 100)
			left = 100;
		s->spdif_volume = (right << 8) | left;
		spin_lock_irqsave(&s->lock, flags);
		src_write(s, SRCREG_VOL_DAC2, DACVolTable[100 - left]);
		src_write(s, SRCREG_VOL_DAC2+1, DACVolTable[100 - right]);
		spin_unlock_irqrestore(&s->lock, flags);*/
		
		return 0;
	
	case SOUND_MIXER_READ_PCM:
		put_user(s->spdif_volume, (int *)arg);
		return 0;
	}
	return codec->mixer_ioctl(codec, cmd, arg);
}

status_t es1371_write(void *node, void *cookie, off_t ppos, const void *buffer, size_t count)
{
	struct es1371_state *s = node;
	size_t ret;
	unsigned long flags;
	unsigned swptr;
	int cnt;

//	printk("ES1371_write: ENTER\n");
        VALIDATE_STATE(s);
	if (s->dma_dac2.mapped)
		return -3;
        LOCK(s->sem);	
	if (!s->dma_dac2.ready && (ret = prog_dmabuf_dac2(s)))
		return -4;
	ret = 0;
	while (count > 0) {
	        spin_lock_irqsave(&s->lock, flags);
		if (s->dma_dac2.count < 0) {
			s->dma_dac2.count = 0;
			s->dma_dac2.swptr = s->dma_dac2.hwptr;
		}
		swptr = s->dma_dac2.swptr;
		cnt = s->dma_dac2.dmasize-swptr;
		if (s->dma_dac2.count + cnt > s->dma_dac2.dmasize)
			cnt = s->dma_dac2.dmasize - s->dma_dac2.count;
		spin_unlock_irqrestore(&s->lock, flags);
		if (cnt > count)
			cnt = count;
		if (cnt <= 0) {
			if (s->dma_dac2.enabled)
				start_dac2(s);
			UNLOCK(s->sem);
		        sleep_on_sem(s->dma_dac2.wait, INFINITE_TIMEOUT);
		        LOCK(s->sem);
			if (s->dma_dac2.mapped)
			{
				ret = -1;
				goto out;
			}
			continue;
		}
		if (copy_from_user(s->dma_dac2.rawbuf + swptr, buffer, cnt)) {
			if (!ret)
				ret = -2;
			goto out;
		}
		swptr = (swptr + cnt) % s->dma_dac2.dmasize;
		spin_lock_irqsave(&s->lock, flags);
		s->dma_dac2.swptr = swptr;
		s->dma_dac2.count += cnt;
		s->dma_dac2.endcleared = 0;
		spin_unlock_irqrestore(&s->lock, flags);
		count -= cnt;
		buffer += cnt;
		ret += cnt;
		if (s->dma_dac2.enabled)
			start_dac2(s);
	}
out:
	UNLOCK(s->sem);
        return ret;
}

status_t es1371_open (void* pNode, uint32 nFlags, void **pCookie)
{
	unsigned long flags;
	struct es1371_state *s = (struct es1371_state *) pNode;

       	VALIDATE_STATE(s);
        if (s->open == true) {
	           printk("es1371_open: Device already open\n");
	           return -1;
	}
	LOCK(s->sem);
	s->dma_dac2.ossfragshift = s->dma_dac2.ossmaxfrags = s->dma_dac2.subdivision = 0;
	s->dma_dac2.enabled = 1;
	set_dac2_rate(s, 44100);
	spin_lock_irqsave(&s->lock, flags);
	s->sctrl |= ES1371_FMT_S16_STEREO << SCTRL_SH_P2FMT;
	outl(s->sctrl, s->io+ES1371_REG_SERIAL_CONTROL);
	spin_unlock_irqrestore(&s->lock, flags);
	s->open = true;
        UNLOCK(s->sem);
//	printk("es1371_open: EXIT\n");
	return 0;
}


static int drain_dac2(struct es1371_state *s, int nonblock)
{
	unsigned long flags;
	int count, tmo;

	if (s->dma_dac2.mapped || !s->dma_dac2.ready)
		return 0;
        for (;;) {
                spin_lock_irqsave(&s->lock, flags);
		count = s->dma_dac2.count;
                spin_unlock_irqrestore(&s->lock, flags);
		if (count <= 0)
			break;
	        tmo = 3 * 100 * (count + s->dma_dac2.fragsize) / 2 / s->dac2rate;
		tmo >>= sample_shift[(s->sctrl & SCTRL_P2FMT) >> SCTRL_SH_P2FMT];
        }
        return 0;
}


status_t es1371_release(void* pNode, void* pCookie)
{
	struct es1371_state *s = (struct es1371_state *)pNode;

	VALIDATE_STATE(s);

	drain_dac2(s, O_WRONLY & O_NONBLOCK);
	LOCK(s->open_sem);
	stop_dac2(s);
   	dealloc_dmabuf(s, &s->dma_dac2);
	s->open = false;
        UNLOCK(s->open_sem);
	return 0;
}


status_t es1371_ioctl (void *node, void *cookie, uint32 cmd, void *arg, bool frkrnl)
{
	struct es1371_state *s = (struct es1371_state *)node;
	unsigned long flags;
	int val, ret;
   
//        printk("es1371_ioctl: ENTER, Command: %d\n", cmd);

	VALIDATE_STATE(s);
  	switch (cmd) {
	case OSS_GETVERSION:
	        put_user(SOUND_VERSION, (int *)arg);
	        return 0;

        case SNDCTL_DSP_SYNC:
		return drain_dac2(s, 0/*file->f_flags & O_NONBLOCK*/);
		
	case SNDCTL_DSP_SETDUPLEX:
		return 0;

	case SNDCTL_DSP_GETCAPS:
		put_user(DSP_CAP_DUPLEX | DSP_CAP_REALTIME | DSP_CAP_TRIGGER | DSP_CAP_MMAP, (int *)arg);
	        return 0;
		
        case SNDCTL_DSP_RESET:
	        stop_dac2(s);
		s->dma_dac2.swptr = s->dma_dac2.hwptr = s->dma_dac2.count = s->dma_dac2.total_bytes = 0;
		return 0;

        case SNDCTL_DSP_SPEED:
                get_user(val, (int *)arg);
		if (val >= 0) {
		         stop_dac2(s);
			 s->dma_dac2.ready = 0;
			 set_dac2_rate(s, val);
		}
                put_user(s->dac2rate, (int *)arg);
	        return 0;

        case SNDCTL_DSP_STEREO:
		get_user(val, (int *)arg);
		stop_dac2(s);
		s->dma_dac2.ready = 0;
		spin_lock_irqsave(&s->lock, flags);
		if (val)
			s->sctrl |= SCTRL_P2SMB;
		else
		       	s->sctrl &= ~SCTRL_P2SMB;
		outl(s->sctrl, s->io+ES1371_REG_SERIAL_CONTROL);
		spin_unlock_irqrestore(&s->lock, flags);
		return 0;

        case SNDCTL_DSP_CHANNELS:
                get_user(val, (int *)arg);
		if (val != 0) {
		       	stop_dac2(s);
	       		s->dma_dac2.ready = 0;
       			spin_lock_irqsave(&s->lock, flags);
		        if (val >= 2)
	       		      s->sctrl |= SCTRL_P2SMB;
	       	        else
		              s->sctrl &= ~SCTRL_P2SMB;
			outl(s->sctrl, s->io+ES1371_REG_SERIAL_CONTROL);
		       	spin_unlock_irqrestore(&s->lock, flags);
		}
		put_user( 1, (int *)arg);
	        return 0;
		
	case SNDCTL_DSP_GETFMTS: /* Returns a mask */
                put_user(AFMT_S16_LE|AFMT_U8, (int *)arg);
	        return 0;
		
	case SNDCTL_DSP_SETFMT: /* Selects ONE fmt*/
		get_user(val, (int *)arg);
		if (val != AFMT_QUERY) {
		        stop_dac2(s);
		       	s->dma_dac2.ready = 0;
	       		spin_lock_irqsave(&s->lock, flags);
       			if (val == AFMT_S16_LE)
		                s->sctrl |= SCTRL_P2SEB;
			else
		       		s->sctrl &= ~SCTRL_P2SEB;
       			outl(s->sctrl, s->io+ES1371_REG_SERIAL_CONTROL);
	       		spin_unlock_irqrestore(&s->lock, flags);
		}
		put_user(AFMT_S16_LE, (int *)arg);
	        return 0;
		
	case SNDCTL_DSP_POST:
                return 0;

        case SNDCTL_DSP_GETTRIGGER:
		val = 0;
		val |= PCM_ENABLE_OUTPUT;
		put_user(val, (int *)arg);
	        return 0;
		
	case SNDCTL_DSP_SETTRIGGER:
		get_user(val, (int *)arg);
		
	     	if (val & PCM_ENABLE_OUTPUT) {
       			if (!s->dma_dac2.ready && (ret = prog_dmabuf_dac2(s)))
	       			return ret;
	       		s->dma_dac2.enabled = 1;
	       		start_dac2(s);
	       	} else {
	       		s->dma_dac2.enabled = 0;
	       		stop_dac2(s);
	       	}
		return 0;

	case SNDCTL_DSP_GETOSPACE:
		printk("SNDCTL_DSP_GETOSPACE: No implemented\n");
	        return 0;
	
	case SNDCTL_DSP_GETISPACE:
	        printk("SNDCTL_DSP_GETISPACE: No implemented\n");
	        return 0;
	   
        case SNDCTL_DSP_NONBLOCK:
                printk("SNDCTL_DSP_NONBLOCK: No implemented\n");
	        return 0;

        case SNDCTL_DSP_GETODELAY:
		printk("SNDCTL_DSP_GETODELAY: No implemented\n");
	        return 0;

        case SNDCTL_DSP_GETIPTR:
		printk("SNDCTL_DSP_GETIPTR: No implemented\n");
	        return 0;

        case SNDCTL_DSP_GETOPTR:
		printk("SNDCTL_DSP_GETOPTR: No implemented\n");
	        return 0;
	   
        case SNDCTL_DSP_GETBLKSIZE:
	       	if ((val = prog_dmabuf_dac2(s))){
       		         printk("Error en getblksize\n");
       		         return val;
       		}
       	        put_user(s->dma_dac2.fragsize, (int *)arg);
                return 0;

        case SNDCTL_DSP_SETFRAGMENT:
                get_user(val, (int *)arg);
	       	s->dma_dac2.ossfragshift = val & 0xffff;
       		s->dma_dac2.ossmaxfrags = (val >> 16) & 0xffff;
       		if (s->dma_dac2.ossfragshift < 4)
       			s->dma_dac2.ossfragshift = 4;
       		if (s->dma_dac2.ossfragshift > 15)
       			s->dma_dac2.ossfragshift = 15;
       		if (s->dma_dac2.ossmaxfrags < 4)
       			s->dma_dac2.ossmaxfrags = 4;
		return 0;

        case SNDCTL_DSP_SUBDIVIDE:
	        if (s->dma_dac2.subdivision)
			return -EINVAL;
                get_user(val, (int *)arg);
		if (val != 1 && val != 2 && val != 4)
			return -EINVAL;
		s->dma_dac2.subdivision = val;
		return 0;

        case SOUND_PCM_READ_RATE:
		put_user( s->dac2rate, (int *)arg);
	        return 0;

        case SOUND_PCM_READ_CHANNELS:
	        put_user( 2, (int *)arg);
	        return 0;
		
        case SOUND_PCM_READ_BITS:
		put_user(16, (int *)arg);
	        return 0;

		case IOCTL_GET_USERSPACE_DRIVER:
		{
			memcpy_to_user( arg, "oss.so", strlen( "oss.so" ) );
			return( 0 );
		}

        case SOUND_PCM_WRITE_FILTER:
        case SNDCTL_DSP_SETSYNCRO:
        case SOUND_PCM_READ_FILTER:
                return -EINVAL;
		
	}
	return mixdev_ioctl(&s->codec, cmd, (unsigned long) &arg);
}

status_t es1371_mixer_open (void* pNode, uint32 nFlags, void **pCookie)
{
/*	int minor = MINOR(inode->i_rdev);
	struct list_head *list;*/
	struct es1371_state *s = (struct es1371_state *)pNode;

/*	for (list = devs.next; ; list = list->next) {
		if (list == &devs)
			return -ENODEV;
		s = list_entry(list, struct es1371_state, devs);
		if (s->codec.dev_mixer == minor)
			break;
	}*/
       	VALIDATE_STATE(s);
//	file->private_data = s;
	return 0;
}

status_t es1371_mixer_release(void* pNode, void* pCookie)
{
	return 0;
}


status_t es1371_mixer_ioctl (void *node, void *cookie, uint32 cmd, void *args, bool len)
{
	struct es1371_state *s = (struct es1371_state *)node;
	struct ac97_codec *codec = &s->codec;
        unsigned long arg = (unsigned long)args;
       
	return mixdev_ioctl(codec, cmd, arg);
}

DeviceOperations_s es1371_mixer_fops = {
		es1371_mixer_open,
		es1371_mixer_release,
		es1371_mixer_ioctl,
		NULL,
		NULL
};


int es1371_init_one(PCI_Info_s *pcidev)
{
	struct es1371_state *s;
	int i, val, res = -1;
	unsigned int cssr;
	
	if (!(s = kmalloc(sizeof(struct es1371_state), MEMF_KERNEL))) {
		printk(KERN_WARNING PFX "out of memory\n");
		return -ENOMEM;
	}
	memset(s, 0, sizeof(struct es1371_state));
	
	s->open_sem = create_semaphore("es1371_open_sem", 1, 0);
        s->sem = create_semaphore("es1371_sem", 1, 0);
	spinlock_init(&s->lock, "es1371_lock");
	s->magic = ES1371_MAGIC;
	s->dev = pcidev;
	s->io = pcidev->u.h0.nBase0 & PCI_ADDRESS_MEMORY_32_MASK;
	s->irq = pcidev->u.h0.nInterruptLine;
	s->vendor = pcidev->nVendorID;
	s->device = pcidev->nDeviceID;
	s->rev = pcidev->nRevisionID;
	s->codec.private_data = s;
	s->codec.id = 0;

        s->codec.codec_read = rdcodec;
	s->codec.codec_write = wrcodec;
        s->dma_dac1.wait = create_semaphore("es1371_dac1_sem", 1, 0);
        s->dma_dac2.wait = create_semaphore("es1371_dac2_sem", 1, 0);
        s->dma_adc.wait = create_semaphore("es1371_adc_sem", 1, 0);

	
	printk("found chip, vendor id 0x%04x device id 0x%04x revision 0x%02x\n",
	       s->vendor, s->device, s->rev);
	
        if (request_irq (s->dev->u.h0.nInterruptLine, es1371_interrupt, NULL, SA_SHIRQ, "es1371", s)<0) {
		printk ("unable to obtain IRQ %d, aborting\n",
			s->dev->u.h0.nInterruptLine);
		printk ("EXIT, returning -EBUSY\n");
		return -EBUSY;
	}
   
	printk("found es1371 rev %d at io %#lx irq %u\n features: joystick 0x%x\n", s->rev, s->io, s->irq, joystick[devindex]);
	
	/* initialize codec registers */
	s->ctrl = 0;
	
	s->sctrl = 0;
	cssr = 0;
	s->spdif_volume = -1;
	/* check to see if s/pdif mode is being requested */
	if (spdif[devindex]) {
		if (s->rev >= 4) {
			printk("enabling S/PDIF output\n");
			s->spdif_volume = 0;
			cssr |= STAT_EN_SPDIF;
			s->ctrl |= CTRL_SPDIFEN_B;
			if (nomix[devindex]) /* don't mix analog inputs to s/pdif output */
				s->ctrl |= CTRL_RECEN_B;
		} else {
			printk(KERN_ERR PFX "revision %d does not support S/PDIF\n", s->rev);
		}
	}
	/* initialize the chips */
	outl(s->ctrl, s->io+ES1371_REG_CONTROL);
	outl(s->sctrl, s->io+ES1371_REG_SERIAL_CONTROL);
	outl(0, s->io+ES1371_REG_LEGACY);
	/* if we are a 5880 turn on the AC97 */
	if (s->vendor == PCI_VENDOR_ID_ENSONIQ &&
	    ((s->device == PCI_DEVICE_ID_ENSONIQ_CT5880 && s->rev >= CT5880REV_CT5880_C) || 
	     (s->device == PCI_DEVICE_ID_ENSONIQ_ES1371 && s->rev == ES1371REV_CT5880_A) || 
	     (s->device == PCI_DEVICE_ID_ENSONIQ_ES1371 && s->rev == ES1371REV_ES1373_8))) {
		cssr |= CSTAT_5880_AC97_RST;
		outl(cssr, s->io+ES1371_REG_STATUS);
		/* need to delay around 20ms(bleech) to give
		   some CODECs enough time to wakeup */
		udelay(2);
		/*tmo = jiffies + (HZ / 50) + 1;
		for (;;) {
			tmo2 = tmo - jiffies;
			if (tmo2 <= 0)
				break;
			schedule_timeout(tmo2);
		}*/
	}
	/* AC97 warm reset to start the bitclk */
	outl(s->ctrl | CTRL_SYNCRES, s->io+ES1371_REG_CONTROL);
	udelay(2);
	outl(s->ctrl, s->io+ES1371_REG_CONTROL);
	/* init the sample rate converter */
	src_init(s);
	/* codec init */
        if (!ac97_probe_codec(&s->codec)) {
		res = -ENODEV;
		goto error;
	}
	/* set default values */

	val = SOUND_MASK_LINE;
	mixdev_ioctl(&s->codec, SOUND_MIXER_WRITE_RECSRC, (unsigned long)&val);
	for (i = 0; i < sizeof(initvol)/sizeof(initvol[0]); i++) {
		val = initvol[i].vol;
		mixdev_ioctl(&s->codec, initvol[i].mixch, (unsigned long)&val);
	}
	/* mute master and PCM when in S/PDIF mode */
	if (s->spdif_volume != -1) {
		val = 0x0000;
		s->codec.mixer_ioctl(&s->codec, SOUND_MIXER_WRITE_VOLUME, (unsigned long)&val);
		s->codec.mixer_ioctl(&s->codec, SOUND_MIXER_WRITE_PCM, (unsigned long)&val);
	}
	/* turn on S/PDIF output driver if requested */
	outl(cssr, s->io+ES1371_REG_STATUS);
	
        es1371_driver_data = s;
        s->open = false;
   
        if (devindex < NR_DEVICE-1)
		devindex++;
       	return 0;

 error:
	kfree(s);
	return res;
}

DeviceOperations_s es1371_dsp_fops = {
		es1371_open,
		es1371_release,
		es1371_ioctl,
		NULL,
		es1371_write
};

status_t device_init( int nDeviceID )
{
	int i;
	bool found = false;
	PCI_Info_s pci;
	PCI_bus_s* psBus = get_busmanager( PCI_BUS_NAME, PCI_BUS_VERSION );
	
	if( psBus == NULL )
		return( -ENODEV );
	
	printk ("Ensoniq 1371 driver\n");
	
	/* scan all PCI devices */
	for(i = 0 ;  psBus->get_pci_info( &pci, i ) == 0 && found != true ; ++i) {
		if ( (pci.nVendorID == PCI_VENDOR_ID_ENSONIQ && pci.nDeviceID == PCI_DEVICE_ID_ENSONIQ_ES1371) ||
		     (pci.nVendorID == PCI_VENDOR_ID_ENSONIQ && pci.nDeviceID == PCI_DEVICE_ID_ENSONIQ_CT5880) || 
		     (pci.nVendorID == PCI_VENDOR_ID_ECTIVA && pci.nDeviceID == PCI_DEVICE_ID_ECTIVA_EV1938) ) {
			if(claim_device( nDeviceID, pci.nHandle, "Ensoniq 1371", DEVICE_AUDIO ) == 0 && es1371_init_one(&pci) == 0)
				found = true;
        	}
	}
    
	if(found) {
    	/* create DSP node */
		if( create_device_node( nDeviceID, pci.nHandle, "audio/es1371", &es1371_dsp_fops, es1371_driver_data ) < 0 ) {
			printk( "Failed to create dsp node \n");
			return ( -EINVAL );
		}
	        /* create mixer node */
		if( create_device_node( nDeviceID, pci.nHandle, "audio/mixer/es1371", &es1371_mixer_fops, es1371_driver_data ) < 0 ) {
			printk( "Failed to create mixer node \n");
			return ( -EINVAL );
		}
		printk( "Found!\n" );
	} else {
		disable_device( nDeviceID );
		printk( "No device found\n" );
		return ( -EINVAL );
	}
	return (0);	
}
