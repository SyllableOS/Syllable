/*
 *	Intel i810 and friends ICH driver for Linux
 *	Alan Cox <alan@redhat.com>
 *
 *  Built from:
 *	Low level code:  Zach Brown (original nonworking i810 OSS driver)
 *			 Jaroslav Kysela <perex@suse.cz> (working ALSA driver)
 *
 *	Framework: Thomas Sailer <sailer@ife.ee.ethz.ch>
 *	Extended by: Zach Brown <zab@redhat.com>  
 *	             and others ...
 *	Port to Syllable: Michael Krueger <invenies@web.de>
 *
 *  Hardware Provided By:
 *	Analog Devices (A major AC97 codec maker)
 *	Intel Corp  (you've probably heard of them already)
 *
 * AC97 clues and assistance provided by
 *	Analog Devices
 *	Zach 'Fufu' Brown
 *	Jeff Garzik
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program; if not, write to the Free Software
 *	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *
 *	Intel 810 theory of operation
 *
 *	The chipset provides three DMA channels that talk to an AC97
 *	CODEC (AC97 is a digital/analog mixer standard). At its simplest
 *	you get 48Khz audio with basic volume and mixer controls. At the
 *	best you get rate adaption in the codec. We set the card up so
 *	that we never take completion interrupts but instead keep the card
 *	chasing its tail around a ring buffer. This is needed for mmap
 *	mode audio and happens to work rather well for non-mmap modes too.
 *
 *	The board has one output channel for PCM audio (supported) and
 *	a stereo line in and mono microphone input. Again these are normally
 *	locked to 48Khz only. Right now recording is not finished.
 *
 *	There is no midi support, no synth support. Use timidity. To get
 *	esd working you need to use esd -r 48000 as it won't probe 48KHz
 *	by default. mpg123 can't handle 48Khz only audio so use xmms.
 *
 *	Fix The Sound On Dell
 *
 *	Not everyone uses 48KHz. We know of no way to detect this reliably
 *	and certainly not to get the right data. If your i810 audio sounds
 *	stupid you may need to investigate other speeds. According to Analog
 *	they tend to use a 14.318MHz clock which gives you a base rate of
 *	41194Hz.
 *
 *	If you need to force a specific rate set the clocking= option
 */
 
 
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
#include <atheos/timer.h>
#include <macros.h>

#include "bitops.h"
#include "linuxcomp.h"
#include "ac97_codec.h"

/* Syllable 0.5.2 and higher uses a new device node structure */
#define ENABLE_DEVNODE_052

#ifndef PCI_DEVICE_ID_INTEL_82801
#define PCI_DEVICE_ID_INTEL_82801	0x2415
#endif
#ifndef PCI_DEVICE_ID_INTEL_82901
#define PCI_DEVICE_ID_INTEL_82901	0x2425
#endif
#ifndef PCI_DEVICE_ID_INTEL_ICH2
#define PCI_DEVICE_ID_INTEL_ICH2	0x2445
#endif
#ifndef PCI_DEVICE_ID_INTEL_ICH3
#define PCI_DEVICE_ID_INTEL_ICH3	0x2485
#endif
#ifndef PCI_DEVICE_ID_INTEL_ICH4
#define PCI_DEVICE_ID_INTEL_ICH4	0x24c5
#endif
#ifndef PCI_DEVICE_ID_INTEL_ICH5
#define PCI_DEVICE_ID_INTEL_ICH5	0x24d5
#endif
#ifndef PCI_DEVICE_ID_INTEL_440MX
#define PCI_DEVICE_ID_INTEL_440MX	0x7195
#endif
#ifndef PCI_DEVICE_ID_SI_7012
#define PCI_DEVICE_ID_SI_7012		0x7012
#endif
#ifndef PCI_DEVICE_ID_NVIDIA_MCP1_AUDIO
#define PCI_DEVICE_ID_NVIDIA_MCP1_AUDIO	0x01b1
#endif
#ifndef PCI_DEVICE_ID_NVIDIA_MCP2_AUDIO
#define PCI_DEVICE_ID_NVIDIA_MCP2_AUDIO	0x006a
#endif
#ifndef PCI_DEVICE_ID_NVIDIA_MCP3_AUDIO
#define PCI_DEVICE_ID_NVIDIA_MCP3_AUDIO	0x00da
#endif
#ifndef PCI_DEVICE_ID_AMD_768_AUDIO
#define PCI_DEVICE_ID_AMD_768_AUDIO	0x7445
#endif
#ifndef PCI_DEVICE_ID_AMD_8111_AC97
#define PCI_DEVICE_ID_AMD_8111_AC97	0x746d
#endif

#define MODULOP2(a, b) ((a) & ((b) - 1))
#define MASKP2(a, b) ((a) & ~((b) - 1))

static int ftsodell;
static int strict_clocking;
static unsigned int clocking;
static int spdif_locked;

#define ADC_RUNNING	1
#define DAC_RUNNING	2

//#define DEBUG
//#define DEBUG2
//#define DEBUG_INTERRUPTS
//#define DEBUG_MMAP
//#define DEBUG_MMIO

#define I810_FMT_16BIT	1		/* These are fixed in fact */
#define I810_FMT_STEREO	2
#define I810_FMT_MASK	3

#define SPDIF_ON	0x0004
#define SURR_ON		0x0010
#define CENTER_LFE_ON	0x0020
#define VOL_MUTED	0x8000

/* the 810's array of pointers to data buffers */

struct sg_item {
#define BUSADDR_MASK	0xFFFFFFFE
	u32 busaddr;	
#define CON_IOC 	0x80000000 /* interrupt on completion */
#define CON_BUFPAD	0x40000000 /* pad underrun with last sample, else 0 */
#define CON_BUFLEN_MASK	0x0000ffff /* buffer length in samples */
	u32 control;
};

/* an instance of the i810 channel */
#define SG_LEN 32
struct i810_channel 
{
	/* these sg guys should probably be allocated
	   seperately as nocache. Must be 8 byte aligned */
	struct sg_item sg[SG_LEN];	/* 32*8 */
	u32 offset;			/* 4 */
	u32 port;			/* 4 */
	u32 used;			/* 4 */
	u32 num;			/* 4 */
};

/*
 * we have 3 seperate dma engines.  pcm in, pcm out, and mic.
 * each dma engine has controlling registers.  These goofy
 * names are from the datasheet, but make it easy to write
 * code while leafing through it.
 *
 * ICH4 has 6 dma engines, pcm in, pcm out, mic, pcm in 2, 
 * mic in 2, s/pdif.   Of special interest is the fact that
 * the upper 3 DMA engines on the ICH4 *must* be accessed
 * via mmio access instead of pio access.
 */

#define ENUM_ENGINE(PRE,DIG) \
enum { \
	PRE##_BASE =	0x##DIG##0,		/* Base Address */ \
	PRE##_BDBAR =	0x##DIG##0,		/* Buffer Descriptor list Base Address */	\
	PRE##_CIV =	0x##DIG##4,		/* Current Index Value */			\
	PRE##_LVI =	0x##DIG##5,		/* Last Valid Index */				\
	PRE##_SR =	0x##DIG##6,		/* Status Register */				\
	PRE##_PICB =	0x##DIG##8,		/* Position In Current Buffer */		\
	PRE##_PIV =	0x##DIG##a,		/* Prefetched Index Value */			\
	PRE##_CR =	0x##DIG##b		/* Control Register */				\
}


ENUM_ENGINE(OFF,0);	/* Offsets */
ENUM_ENGINE(PI,0);	/* PCM In */
ENUM_ENGINE(PO,1);	/* PCM Out */
ENUM_ENGINE(MC,2);	/* Mic In */

enum {
	GLOB_CNT =	0x2c,			/* Global Control */
	GLOB_STA = 	0x30,			/* Global Status */
	CAS	 = 	0x34			/* Codec Write Semaphore Register */
};

ENUM_ENGINE(MC2,4);     /* Mic In 2 */
ENUM_ENGINE(PI2,5);     /* PCM In 2 */
ENUM_ENGINE(SP,6);      /* S/PDIF */

enum {
	SDM =           0x80                    /* SDATA_IN Map Register */
};

/* interrupts for a dma engine */
#define DMA_INT_FIFO		(1<<4)  /* fifo under/over flow */
#define DMA_INT_COMPLETE	(1<<3)  /* buffer read/write complete and ioc set */
#define DMA_INT_LVI		(1<<2)  /* last valid done */
#define DMA_INT_CELV		(1<<1)  /* last valid is current */
#define DMA_INT_DCH		(1)	/* DMA Controller Halted (happens on LVI interrupts) */
#define DMA_INT_MASK (DMA_INT_FIFO|DMA_INT_COMPLETE|DMA_INT_LVI)

/* interrupts for the whole chip */
#define INT_SEC		(1<<11)
#define INT_PRI		(1<<10)
#define INT_MC		(1<<7)
#define INT_PO		(1<<6)
#define INT_PI		(1<<5)
#define INT_MO		(1<<2)
#define INT_NI		(1<<1)
#define INT_GPI		(1<<0)
#define INT_MASK (INT_SEC|INT_PRI|INT_MC|INT_PO|INT_PI|INT_MO|INT_NI|INT_GPI)


#define DRIVER_VERSION "0.3" /* based on Linux driver version 0.25 */

/* magic numbers to protect our data structures */
#define I810_CARD_MAGIC		0x5072696E /* "Prin" */
#define I810_STATE_MAGIC	0x63657373 /* "cess" */
#define I810_DMA_MASK		0xffffffff /* DMA buffer mask for pci_alloc_consistent */
#define NR_HW_CH		2

/* maxinum number of AC97 codecs connected, AC97 2.0 defined 4 */
#define NR_AC97		2

/* minor number of /dev/dspW */
#define SND_DEV_DSP8	1

/* minor number of /dev/dspW */
#define SND_DEV_DSP16	1 

/* AtheOS additions */
static void *i810_card_data;
static void *i810_state_data;

/* Please note that an 8bit mono stream is not valid on this card, you must have a 16bit */
/* stream at a minimum for this card to be happy */
static const unsigned sample_size[] = { 1, 2, 2, 4 };
/* Samples are 16bit values, so we are shifting to a word, not to a byte, hence shift */
/* values are one less than might be expected */
static const unsigned sample_shift[] = { -1, 0, 0, 1 };

enum {
	ICH82801AA = 0,
	ICH82901AB,
	INTEL440MX,
	INTELICH2,
	INTELICH3,
	INTELICH4,
	INTELICH5,
	SI7012,
	NVIDIA_NFORCE,
	AMD768,
	AMD8111
};

struct pci_audio_info {
	u16 vendor;
	u16 device;
	u16 driver_data;
	char *name;
};

static struct pci_audio_info pci_audio_devices[] = {
	{PCI_VENDOR_ID_INTEL, PCI_DEVICE_ID_INTEL_82801, ICH82801AA, "Intel ICH 82801AA"},
	{PCI_VENDOR_ID_INTEL, PCI_DEVICE_ID_INTEL_82901, ICH82901AB, "Intel ICH 82901AB"},
	{PCI_VENDOR_ID_INTEL, PCI_DEVICE_ID_INTEL_440MX, INTEL440MX, "Intel 440MX"},
	{PCI_VENDOR_ID_INTEL, PCI_DEVICE_ID_INTEL_ICH2, INTELICH2, "Intel ICH2"},
	{PCI_VENDOR_ID_INTEL, PCI_DEVICE_ID_INTEL_ICH3, INTELICH3, "Intel ICH3"},
	{PCI_VENDOR_ID_INTEL, PCI_DEVICE_ID_INTEL_ICH4, INTELICH4, "Intel ICH4"},
	{PCI_VENDOR_ID_INTEL, PCI_DEVICE_ID_INTEL_ICH5, INTELICH5, "Intel ICH5"},
	{PCI_VENDOR_ID_SI, PCI_DEVICE_ID_SI_7012, SI7012,"SiS 7012"},
	{PCI_VENDOR_ID_NVIDIA, PCI_DEVICE_ID_NVIDIA_MCP1_AUDIO, NVIDIA_NFORCE, "NVIDIA nForce Audio"},
	{PCI_VENDOR_ID_NVIDIA, PCI_DEVICE_ID_NVIDIA_MCP2_AUDIO, NVIDIA_NFORCE, "NVIDIA nForce Audio"},
	{PCI_VENDOR_ID_NVIDIA, PCI_DEVICE_ID_NVIDIA_MCP3_AUDIO, NVIDIA_NFORCE, "NVIDIA nForce Audio"},
	{PCI_VENDOR_ID_AMD, PCI_DEVICE_ID_AMD_768_AUDIO, AMD768, "AMD 768"},	
	{PCI_VENDOR_ID_AMD, PCI_DEVICE_ID_AMD_8111_AC97, AMD8111, "AMD-8111 IoHub"},
};

/* "software" or virtual channel, an instance of opened /dev/dsp */
struct i810_state {
	unsigned int magic;
	struct i810_card *card;	/* Card info */

	/* single open lock mechanism, only used for recording */
	sem_id open_sem;
	sem_id open_wait;

	/* file mode */
	mode_t open_mode;

	/* virtual channel number */
	int virt;

	struct dmabuf {
		/* wave sample stuff */
		unsigned int rate;
		unsigned char fmt, enable, trigger;

		/* hardware channel */
		struct i810_channel *read_channel;
		struct i810_channel *write_channel;

		/* OSS buffer management stuff */
		void *rawbuf;
		dma_addr_t dma_handle;
		void* real;
		unsigned buforder;
		unsigned numfrag;
		unsigned fragshift;

		/* our buffer acts like a circular ring */
		unsigned hwptr;		/* where dma last started, updated by update_ptr */
		unsigned swptr;		/* where driver last clear/filled, updated by read/write */
		int count;		/* bytes to be consumed or been generated by dma machine */
		unsigned total_bytes;	/* total bytes dmaed by hardware */

		unsigned error;		/* number of over/underruns */
		sem_id wait;	/* put process on wait queue when no more space in buffer */

		/* redundant, but makes calculations easier */
		/* what the hardware uses */
		unsigned dmasize;
		unsigned fragsize;
		unsigned fragsamples;

		/* what we tell the user to expect */
		unsigned userfrags;
		unsigned userfragsize;

		/* OSS stuff */
		unsigned mapped:1;
		unsigned ready:1;
		unsigned update_flag;
		unsigned ossfragsize;
		unsigned ossmaxfrags;
		unsigned subdivision;
	} dmabuf;
};

struct i810_card {
	unsigned int magic;

	/* We keep i810 cards in a linked list */
	struct i810_card *next;

	/* The i810 has a certain amount of cross channel interaction
	   so we use a single per card lock */
	SpinLock_s lock;

	/* PCI device stuff */
	PCI_Info_s * pci_dev;
	struct pci_audio_info *pci_info;
	u16 pci_id;
	u16 pci_id_internal; /* used to access card_cap[] */
	int nDeviceID; /* for AtheOS */
	
	/* soundcore stuff */
	int dev_audio;

	/* structures for abstraction of hardware facilities, codecs, banks and channels*/
	u16    ac97_id_map[NR_AC97];
	struct ac97_codec *ac97_codec[NR_AC97];
	struct i810_state *states[NR_HW_CH];
	struct i810_channel *channel;	/* 1:1 to states[] but diff. lifetime */
	dma_addr_t chandma;
	void* chanreal;

	u16 ac97_features;
	u16 ac97_status;
	u16 channels;
	
	/* hardware resources */
	unsigned long ac97base;
	unsigned long iobase;
	u32 irq;
	int irq_handle;
	
	/* Function support */
	struct i810_channel *(*alloc_pcm_channel)(struct i810_card *);
	struct i810_channel *(*alloc_rec_pcm_channel)(struct i810_card *);
	struct i810_channel *(*alloc_rec_mic_channel)(struct i810_card *);
	void (*free_pcm_channel)(struct i810_card *, int chan);

	/* We have a *very* long init time possibly, so use this to block */
	/* attempts to open our devices before we are ready (stops oops'es) */
	int initializing;
};

/* extract register offset from codec struct */
#define IO_REG_OFF(codec) (((struct i810_card *) codec->private_data)->ac97_id_map[codec->id])
#define GET_CIV(port) MODULOP2(inb((port) + OFF_CIV), SG_LEN)
#define GET_LVI(port) MODULOP2(inb((port) + OFF_LVI), SG_LEN)

/* set LVI from CIV */
#define CIV_TO_LVI(port, off) \
	outb(MODULOP2(GET_CIV((port)) + (off), SG_LEN), (port) + OFF_LVI)
 
static PCI_bus_s *g_psBus;

static struct i810_card *devs = NULL;

static int i810_dsp_read(void *node, void *cookie, off_t ppos, void *buffer, size_t count);
static int i810_dsp_write(void *node, void *cookie, off_t ppos, const void *buffer, size_t count);
static int i810_dsp_ioctl (void *node, void *cookie, uint32 cmd, void *args, bool b_fromkernel);
static status_t i810_dsp_open (void* pNode, uint32 nFlags, void **pCookie );
static status_t i810_dsp_release( void* pNode, void* pCookie);

static int i810_mixer_ioctl (void *node, void *cookie, uint32 cmd, void *args, bool b_fromkernel);
static status_t i810_mixer_open (void* pNode, uint32 nFlags, void **pCookie );
static status_t i810_mixer_release( void* pNode, void* pCookie);

static u16 i810_ac97_get(struct ac97_codec *dev, u8 reg);
static void i810_ac97_set(struct ac97_codec *dev, u8 reg, u16 data);

static struct i810_channel *i810_alloc_pcm_channel(struct i810_card *card)
{
	if(card->channel == NULL)
	{
		printk("STOPPP!!!\n");
		return NULL;
	}
	if(card->channel[1].used==1)
		return NULL;
	card->channel[1].used=1;
	return &card->channel[1];
}

static struct i810_channel *i810_alloc_rec_pcm_channel(struct i810_card *card)
{
	if(card->channel[0].used==1)
		return NULL;
	card->channel[0].used=1;
	return &card->channel[0];
}

static struct i810_channel *i810_alloc_rec_mic_channel(struct i810_card *card)
{
	if(card->channel[2].used==1)
		return NULL;
	card->channel[2].used=1;
	return &card->channel[2];
}

static void i810_free_pcm_channel(struct i810_card *card, int channel)
{
	card->channel[channel].used=0;
}

static int i810_valid_spdif_rate ( struct ac97_codec *codec, int rate )
{
	unsigned long id = 0L;

	id = (i810_ac97_get(codec, AC97_VENDOR_ID1) << 16);
	id |= i810_ac97_get(codec, AC97_VENDOR_ID2) & 0xffff;
#ifdef DEBUG
	printk ( "i810_audio: codec = %s, codec_id = 0x%08lx\n", codec->name, id);
#endif
	switch ( id ) {
		case 0x41445361: /* AD1886 */
			if (rate == 48000) {
				return 1;
			}
			break;
		default: /* all other codecs, until we know otherwiae */
			if (rate == 48000 || rate == 44100 || rate == 32000) {
				return 1;
			}
			break;
	}
	return (0);
}

/* i810_set_dac_channels
 *
 *  Configure the codec's multi-channel DACs
 *
 *  The logic is backwards. Setting the bit to 1 turns off the DAC. 
 *
 *  What about the ICH? We currently configure it using the
 *  SNDCTL_DSP_CHANNELS ioctl.  If we're turnning on the DAC, 
 *  does that imply that we want the ICH set to support
 *  these channels?
 *  
 *  TODO:
 *    vailidate that the codec really supports these DACs
 *    before turning them on. 
 */
static void i810_set_dac_channels(struct i810_state *state, int channel)
{
	int	aud_reg;
	struct ac97_codec *codec = state->card->ac97_codec[0];
	
	/* No codec, no setup */
	
	if(codec == NULL)
		return;
		
	aud_reg = i810_ac97_get(codec, AC97_EXTENDED_STATUS);
	aud_reg |= AC97_EA_PRI | AC97_EA_PRJ | AC97_EA_PRK;
	state->card->ac97_status &= ~(SURR_ON | CENTER_LFE_ON);

	switch ( channel ) {
		case 2: /* always enabled */
			break;
		case 4:
			aud_reg &= ~AC97_EA_PRJ;
			state->card->ac97_status |= SURR_ON;
			break;
		case 6:
			aud_reg &= ~(AC97_EA_PRJ | AC97_EA_PRI | AC97_EA_PRK);
			state->card->ac97_status |= SURR_ON | CENTER_LFE_ON;
			break;
		default:
			break;
	}
	i810_ac97_set(codec, AC97_EXTENDED_STATUS, aud_reg);
}


/* set playback sample rate */
static unsigned int i810_set_dac_rate(struct i810_state * state, unsigned int rate)
{	
	struct dmabuf *dmabuf = &state->dmabuf;
	u32 new_rate;
	struct ac97_codec *codec=state->card->ac97_codec[0];
	
	if(!(state->card->ac97_features&0x0001))
	{
		dmabuf->rate = clocking;
#ifdef DEBUG
		printk("Asked for %d Hz, but ac97_features says we only do %dHz.  Sorry!\n",
		       rate,clocking);
#endif		       
		return clocking;
	}
			
	if (rate > 48000)
		rate = 48000;
	if (rate < 8000)
		rate = 8000;
	dmabuf->rate = rate;
		
	/*
	 *	Adjust for misclocked crap
	 */
	rate = ( rate * clocking)/48000;
	if(strict_clocking && rate < 8000) {
		rate = 8000;
		dmabuf->rate = (rate * 48000)/clocking;
	}

        new_rate=ac97_set_dac_rate(codec, rate);
	if(new_rate != rate) {
		dmabuf->rate = (new_rate * 48000)/clocking;
	}
#ifdef DEBUG
	printk("i810_audio: called i810_set_dac_rate : asked for %d, got %d\n", rate, dmabuf->rate);
#endif
	rate = new_rate;
	return dmabuf->rate;
}

/* set recording sample rate */
static unsigned int i810_set_adc_rate(struct i810_state * state, unsigned int rate)
{
	struct dmabuf *dmabuf = &state->dmabuf;
	u32 new_rate;
	struct ac97_codec *codec=state->card->ac97_codec[0];
	
	if(!(state->card->ac97_features&0x0001))
	{
		dmabuf->rate = clocking;
		return clocking;
	}
			
	if (rate > 48000)
		rate = 48000;
	if (rate < 8000)
		rate = 8000;
	dmabuf->rate = rate;

	/*
	 *	Adjust for misclocked crap
	 */
	 
	rate = ( rate * clocking)/48000;
	if(strict_clocking && rate < 8000) {
		rate = 8000;
		dmabuf->rate = (rate * 48000)/clocking;
	}

	new_rate = ac97_set_adc_rate(codec, rate);
	
	if(new_rate != rate) {
		dmabuf->rate = (new_rate * 48000)/clocking;
		rate = new_rate;
	}
#ifdef DEBUG
	printk("i810_audio: called i810_set_adc_rate : rate = %d/%d\n", dmabuf->rate, rate);
#endif
	return dmabuf->rate;
}


/* get current playback/recording dma buffer pointer (byte offset from LBA),
   called with spinlock held! */
   
static inline unsigned i810_get_dma_addr(struct i810_state *state, int rec)
{
	struct dmabuf *dmabuf = &state->dmabuf;
	unsigned int civ, offset, port, port_picb, bytes = 2;
	
	if (!dmabuf->enable)
		return 0;

	if (rec)
		port = state->card->iobase + dmabuf->read_channel->port;
	else
		port = state->card->iobase + dmabuf->write_channel->port;

	if(state->card->pci_id == PCI_DEVICE_ID_SI_7012) {
		port_picb = port + OFF_SR;
		bytes = 1;
	} else
		port_picb = port + OFF_PICB;

	do {
		civ = GET_CIV(port);
		offset = inw(port_picb);
		/* Must have a delay here! */ 
		if(offset == 0)
			udelay(1);
		/* Reread both registers and make sure that that total
		 * offset from the first reading to the second is 0.
		 * There is an issue with SiS hardware where it will count
		 * picb down to 0, then update civ to the next value,
		 * then set the new picb to fragsize bytes.  We can catch
		 * it between the civ update and the picb update, making
		 * it look as though we are 1 fragsize ahead of where we
		 * are.  The next to we get the address though, it will
		 * be back in the right place, and we will suddenly think
		 * we just went forward dmasize - fragsize bytes, causing
		 * totally stupid *huge* dma overrun messages.  We are
		 * assuming that the 1us delay is more than long enough
		 * that we won't have to worry about the chip still being
		 * out of sync with reality ;-)
		 */
	} while (civ != GET_CIV(port) || offset != inw(port_picb));
		 
	return (((civ + 1) * dmabuf->fragsize - (bytes * offset))
		% dmabuf->dmasize);
}

/* Stop recording (lock held) */
static inline void __stop_adc(struct i810_state *state)
{
	struct dmabuf *dmabuf = &state->dmabuf;
	struct i810_card *card = state->card;

	dmabuf->enable &= ~ADC_RUNNING;
	outb(0, card->iobase + PI_CR);
	// wait for the card to acknowledge shutdown
	while( inb(card->iobase + PI_CR) != 0 ) ;
	// now clear any latent interrupt bits (like the halt bit)
	if(card->pci_id == PCI_DEVICE_ID_SI_7012)
		outb( inb(card->iobase + PI_PICB), card->iobase + PI_PICB );
	else
		outb( inb(card->iobase + PI_SR), card->iobase + PI_SR );
	outl( inl(card->iobase + GLOB_STA) & INT_PI, card->iobase + GLOB_STA);
}

static void stop_adc(struct i810_state *state)
{
	struct i810_card *card = state->card;
	unsigned long flags;

	spin_lock_irqsave(&card->lock, flags);
	__stop_adc(state);
	spin_unlock_irqrestore(&card->lock, flags);
}

static inline void __start_adc(struct i810_state *state)
{
	struct dmabuf *dmabuf = &state->dmabuf;

	if (dmabuf->count < dmabuf->dmasize && dmabuf->ready && !dmabuf->enable &&
	    (dmabuf->trigger & PCM_ENABLE_INPUT)) {
		dmabuf->enable |= ADC_RUNNING;
		// Interrupt enable, LVI enable, DMA enable
		outb(0x10 | 0x04 | 0x01, state->card->iobase + PI_CR);
	}
}
static void start_adc(struct i810_state *state)
{
	struct i810_card *card = state->card;
	unsigned long flags;

	spin_lock_irqsave(&card->lock, flags);
	__start_adc(state);
	spin_unlock_irqrestore(&card->lock, flags);
}

/* stop playback (lock held) */
static inline void __stop_dac(struct i810_state *state)
{
	struct dmabuf *dmabuf = &state->dmabuf;
	struct i810_card *card = state->card;

	dmabuf->enable &= ~DAC_RUNNING;
	outb(0, card->iobase + PO_CR);
	// wait for the card to acknowledge shutdown
	while( inb(card->iobase + PO_CR) != 0 ) ;
	// now clear any latent interrupt bits (like the halt bit)
	if(card->pci_id == PCI_DEVICE_ID_SI_7012)
		outb( inb(card->iobase + PO_PICB), card->iobase + PO_PICB );
	else
		outb( inb(card->iobase + PO_SR), card->iobase + PO_SR );
	outl( inl(card->iobase + GLOB_STA) & INT_PO, card->iobase + GLOB_STA);
}

static void stop_dac(struct i810_state *state)
{
	struct i810_card *card = state->card;
	unsigned long flags;

	spin_lock_irqsave(&card->lock, flags);
	__stop_dac(state);
	spin_unlock_irqrestore(&card->lock, flags);
}	

static inline void __start_dac(struct i810_state *state)
{
	struct dmabuf *dmabuf = &state->dmabuf;

	if (dmabuf->count > 0 && dmabuf->ready && !dmabuf->enable &&
	    (dmabuf->trigger & PCM_ENABLE_OUTPUT)) {
		dmabuf->enable |= DAC_RUNNING;
		// Interrupt enable, LVI enable, DMA enable
		outb(0x10 | 0x04 | 0x01, state->card->iobase + PO_CR);
	}
}
static void start_dac(struct i810_state *state)
{
	struct i810_card *card = state->card;
	unsigned long flags;

	spin_lock_irqsave(&card->lock, flags);
	__start_dac(state);
	spin_unlock_irqrestore(&card->lock, flags);
}
	
#define DMABUF_DEFAULTORDER (15-PAGE_SHIFT)
#define DMABUF_MINORDER 1

/* allocate DMA buffer, playback and recording buffer should be allocated seperately */
static int alloc_dmabuf(struct i810_state *state)
{
	struct dmabuf *dmabuf = &state->dmabuf;
	void *rawbuf= NULL;
	void* real = NULL;
	int order, size;

	/* If we don't have any oss frag params, then use our default ones */
	if(dmabuf->ossmaxfrags == 0)
		dmabuf->ossmaxfrags = 4;
	if(dmabuf->ossfragsize == 0)
		dmabuf->ossfragsize = (PAGE_SIZE<<DMABUF_DEFAULTORDER)/dmabuf->ossmaxfrags;
	size = dmabuf->ossfragsize * dmabuf->ossmaxfrags;

	if(dmabuf->rawbuf && (PAGE_SIZE << dmabuf->buforder) == size)
		return 0;
	/* alloc enough to satisfy the oss params */
	for (order = DMABUF_DEFAULTORDER; order >= DMABUF_MINORDER; order--) {
		printk(" %d", order);
		if ( (PAGE_SIZE<<order) > size )
			continue;
		
		real = kmalloc( ( PAGE_SIZE << order ) + PAGE_SIZE, MEMF_KERNEL );
		rawbuf = (void*)PAGE_ALIGN( (uint32)real );
		dmabuf->dma_handle = (uint32)rawbuf;
		
		if( real )
			break;
		
		#if 0
		if ((rawbuf = pci_alloc_consistent(state->card->pci_dev,
						   PAGE_SIZE << order,
						   &dmabuf->dma_handle)))
			break;
		#endif
	}
	if (!rawbuf)
		return -ENOMEM;


#ifdef DEBUG
	printk("i810_audio: allocated %ld (order = %d) bytes at %p\n",
	       PAGE_SIZE << order, order, rawbuf);
#endif

	dmabuf->ready  = dmabuf->mapped = 0;
	dmabuf->rawbuf = rawbuf;
	dmabuf->real = real;
	dmabuf->buforder = order;

	return 0;
}

/* free DMA buffer */
static void dealloc_dmabuf(struct i810_state *state)
{
	struct dmabuf *dmabuf = &state->dmabuf;

	if (dmabuf->real) {
		kfree( dmabuf->real );
		//pci_free_consistent(state->card->pci_dev, PAGE_SIZE << dmabuf->buforder,
			//	    dmabuf->rawbuf, dmabuf->dma_handle);
	}
	dmabuf->rawbuf = NULL;
	dmabuf->real = NULL;
	dmabuf->mapped = dmabuf->ready = 0;
}

static int prog_dmabuf(struct i810_state *state, unsigned rec)
{
	struct dmabuf *dmabuf = &state->dmabuf;
	struct i810_channel *c;
	struct sg_item *sg;
	unsigned long flags;
	int ret;
	unsigned fragint;
	int i;

	spin_lock_irqsave(&state->card->lock, flags);
	if(dmabuf->enable & DAC_RUNNING)
		__stop_dac(state);
	if(dmabuf->enable & ADC_RUNNING)
		__stop_adc(state);
	dmabuf->total_bytes = 0;
	dmabuf->count = dmabuf->error = 0;
	dmabuf->swptr = dmabuf->hwptr = 0;
	spin_unlock_irqrestore(&state->card->lock, flags);

	/* allocate DMA buffer, let alloc_dmabuf determine if we are already
	 * allocated well enough or if we should replace the current buffer
	 * (assuming one is already allocated, if it isn't, then allocate it).
	 */
	if ((ret = alloc_dmabuf(state)))
		return ret;

	/* FIXME: figure out all this OSS fragment stuff */
	/* I did, it now does what it should according to the OSS API.  DL */
	/* We may not have realloced our dmabuf, but the fragment size to
	 * fragment number ratio may have changed, so go ahead and reprogram
	 * things
	 */
	dmabuf->dmasize = PAGE_SIZE << dmabuf->buforder;
	dmabuf->numfrag = SG_LEN;
	dmabuf->fragsize = dmabuf->dmasize/dmabuf->numfrag;
	dmabuf->fragsamples = dmabuf->fragsize >> 1;
	dmabuf->fragshift = ffs(dmabuf->fragsize) - 1;
	dmabuf->userfragsize = dmabuf->ossfragsize;
	dmabuf->userfrags = dmabuf->dmasize/dmabuf->ossfragsize;

	memset(dmabuf->rawbuf, 0, dmabuf->dmasize);

	if(dmabuf->ossmaxfrags == 4) {
		fragint = 8;
	} else if (dmabuf->ossmaxfrags == 8) {
		fragint = 4;
	} else if (dmabuf->ossmaxfrags == 16) {
		fragint = 2;
	} else {
		fragint = 1;
	}
	/*
	 *	Now set up the ring 
	 */
	if(dmabuf->read_channel)
		c = dmabuf->read_channel;
	else
		c = dmabuf->write_channel;
	while(c != NULL) {
		sg=&c->sg[0];
		/*
		 *	Load up 32 sg entries and take an interrupt at half
		 *	way (we might want more interrupts later..) 
		 */
	  
		for(i=0;i<dmabuf->numfrag;i++)
		{
			sg->busaddr=(u32)dmabuf->dma_handle+dmabuf->fragsize*i;
			// the card will always be doing 16bit stereo
			sg->control=dmabuf->fragsamples;
			if(state->card->pci_id == PCI_DEVICE_ID_SI_7012)
				sg->control <<= 1;
			sg->control|=CON_BUFPAD;
			// set us up to get IOC interrupts as often as needed to
			// satisfy numfrag requirements, no more
			if( ((i+1) % fragint) == 0) {
				sg->control|=CON_IOC;
			}
			sg++;
		}
		spin_lock_irqsave(&state->card->lock, flags);
		outb(2, state->card->iobase+c->port+OFF_CR);   /* reset DMA machine */
		while( inb(state->card->iobase+c->port+OFF_CR) & 0x02 ) ;
		outl((u32)state->card->chandma +
		    c->num*sizeof(struct i810_channel),
		    state->card->iobase+c->port+OFF_BDBAR);
		CIV_TO_LVI(state->card->iobase+c->port, 0);

		spin_unlock_irqrestore(&state->card->lock, flags);

		if(c != dmabuf->write_channel)
			c = dmabuf->write_channel;
		else
			c = NULL;
	}
	
	/* set the ready flag for the dma buffer */
	dmabuf->ready = 1;

#ifdef DEBUG
	printk("i810_audio: prog_dmabuf, sample rate = %d, format = %d,\n\tnumfrag = %d, "
	       "fragsize = %d dmasize = %d\n",
	       dmabuf->rate, dmabuf->fmt, dmabuf->numfrag,
	       dmabuf->fragsize, dmabuf->dmasize);
#endif

	return 0;
}

static void __i810_update_lvi(struct i810_state *state, int rec)
{
	struct dmabuf *dmabuf = &state->dmabuf;
	int x, port;
	int trigger;
	int count, fragsize;
	void (*start)(struct i810_state *);
	
	count = dmabuf->count;
	port = state->card->iobase;
	if (rec) {
		port += dmabuf->read_channel->port;
		trigger = PCM_ENABLE_INPUT;
		start = __start_adc;
		count = dmabuf->dmasize - count;
	} else {
		port += dmabuf->write_channel->port;
		trigger = PCM_ENABLE_OUTPUT;
		start = __start_dac;
	}
	
	/* Do not process partial fragments */
	fragsize = dmabuf->fragsize;
	if (count < fragsize)
		return;

	/* if we are currently stopped, then our CIV is actually set to our
	 * *last* sg segment and we are ready to wrap to the next.  However,
	 * if we set our LVI to the last sg segment, then it won't wrap to
	 * the next sg segment, it won't even get a start.  So, instead, when
	 * we are stopped, we set both the LVI value and also we increment
	 * the CIV value to the next sg segment to be played so that when
	 * we call start_{dac,adc}, things will operate properly
	 */
	if (!dmabuf->enable && dmabuf->ready) {
		if (!(dmabuf->trigger & trigger))
			return;
		start(state);
		while (!(inb(port + OFF_CR) & ((1<<4) | (1<<2))))
			;
	}

	/* MASKP2(swptr, fragsize) - 1 is the tail of our transfer */
	x = MODULOP2(MASKP2(dmabuf->swptr, fragsize) - 1, dmabuf->dmasize);
	x >>= dmabuf->fragshift;
	outb(x, port + OFF_LVI);
}

static void i810_update_lvi(struct i810_state *state, int rec)
{
	struct dmabuf *dmabuf = &state->dmabuf;
	unsigned long flags;

	if(!dmabuf->ready)
		return;
	spin_lock_irqsave(&state->card->lock, flags);
	__i810_update_lvi(state, rec);
	spin_unlock_irqrestore(&state->card->lock, flags);
}

/* update buffer manangement pointers, especially, dmabuf->count and dmabuf->hwptr */
static void i810_update_ptr(struct i810_state *state)
{
	struct dmabuf *dmabuf = &state->dmabuf;
	unsigned hwptr;
	unsigned fragmask, dmamask;
	int diff;

	fragmask = MASKP2(~0, dmabuf->fragsize);
	dmamask = MODULOP2(~0, dmabuf->dmasize);

	/* error handling and process wake up for DAC */
	if (dmabuf->enable == ADC_RUNNING) {
		/* update hardware pointer */
		hwptr = i810_get_dma_addr(state, 1) & fragmask;
		diff = (dmabuf->dmasize + hwptr) & dmamask;
#if defined(DEBUG_INTERRUPTS) || defined(DEBUG_MMAP)
		printk("ADC HWP %d,%d,%d\n", hwptr, dmabuf->hwptr, diff);
#endif
		dmabuf->hwptr = hwptr;
		dmabuf->total_bytes += diff;
		dmabuf->count += diff;
		if (dmabuf->count > dmabuf->dmasize) {
			/* buffer underrun or buffer overrun */
			/* this is normal for the end of a read */
			/* only give an error if we went past the */
			/* last valid sg entry */
			if (GET_CIV(state->card->iobase + PO_BASE) !=
				GET_LVI(state->card->iobase + PO_BASE)) {
				printk(KERN_WARNING "i810_audio: DMA overrun on read\n");
				dmabuf->error++;
			}
		}
		if (diff)
			wakeup_sem(dmabuf->wait, true);
	}
	/* error handling and process wake up for DAC */
	if (dmabuf->enable == DAC_RUNNING) {
		/* update hardware pointer */
		hwptr = i810_get_dma_addr(state, 0);
		diff = (dmabuf->dmasize + hwptr - dmabuf->hwptr) % dmabuf->dmasize;
#if defined(DEBUG_INTERRUPTS) || defined(DEBUG_MMAP)
		printk("DAC HWP %d,%d,%d\n", hwptr, dmabuf->hwptr, diff);
#endif
		dmabuf->hwptr = hwptr;
		dmabuf->total_bytes += diff;
		dmabuf->count -= diff;
		if (dmabuf->count < 0) {
			/* buffer underrun or buffer overrun */
			/* this is normal for the end of a write */
			/* only give an error if we went past the */
			/* last valid sg entry */
			if((inb(state->card->iobase + PO_CIV) & 31) !=
			   (inb(state->card->iobase + PO_LVI) & 31)) {
				printk(KERN_WARNING "i810_audio: DMA overrun on write\n");
				printk("i810_audio: CIV %d, LVI %d, hwptr %x, "
					"count %d\n",
					GET_CIV(state->card->iobase + PO_BASE),
					GET_LVI(state->card->iobase + PO_BASE),
					dmabuf->hwptr, dmabuf->count);
				dmabuf->error++;
			}
		}
		if (dmabuf->count < (dmabuf->dmasize-dmabuf->userfragsize))
		{
			wakeup_sem(dmabuf->wait, true);
		}
	}
}

static inline int i810_get_free_write_space(struct i810_state *state)
{
	struct dmabuf *dmabuf = &state->dmabuf;
	int free;

	i810_update_ptr(state);
	// catch underruns during playback
	if (dmabuf->count < 0) {
		dmabuf->count = 0;
		dmabuf->swptr = dmabuf->hwptr;
	}
	free = dmabuf->dmasize - dmabuf->count;
	if(free < 0)
		return(0);
	return(free);
}

static inline int i810_get_available_read_data(struct i810_state *state)
{
	struct dmabuf *dmabuf = &state->dmabuf;
	int avail;

	i810_update_ptr(state);
	// catch overruns during record
	if (dmabuf->count > dmabuf->dmasize) {
		dmabuf->count = dmabuf->dmasize;
		dmabuf->swptr = dmabuf->hwptr;
	}
	avail = dmabuf->count;
	if(avail < 0)
		return(0);
	return(avail);
}

static inline void fill_partial_frag(struct dmabuf *dmabuf)
{
	unsigned fragsize;
	unsigned swptr, len;

	fragsize = dmabuf->fragsize;
	swptr = dmabuf->swptr;
	len = fragsize - MODULOP2(dmabuf->swptr, fragsize);
	if (len == fragsize)
		return;

	memset(dmabuf->rawbuf + swptr, '\0', len);
	dmabuf->swptr = MODULOP2(swptr + len, dmabuf->dmasize);
	dmabuf->count += len;
}

static int drain_dac(struct i810_state *state, int signals_allowed)
{
	struct dmabuf *dmabuf = &state->dmabuf;
	unsigned long flags;
	unsigned long tmo;
	int count;

	if (!dmabuf->ready)
		return 0;
	if(dmabuf->mapped) {
		stop_dac(state);
		return 0;
	}

	spin_lock_irqsave(&state->card->lock, flags);

	fill_partial_frag(dmabuf);
	
	/* 
	 * This will make sure that our LVI is correct, that our
	 * pointer is updated, and that the DAC is running.  We
	 * have to force the setting of dmabuf->trigger to avoid
	 * any possible deadlocks.
	 */
	dmabuf->trigger = PCM_ENABLE_OUTPUT;
	i810_update_lvi(state,0);

	spin_unlock_irqrestore(&state->card->lock, flags);

	for (;;) {

		spin_lock_irqsave(&state->card->lock, flags);
		i810_update_ptr(state);
		count = dmabuf->count;
		spin_unlock_irqrestore(&state->card->lock, flags);

		if (count <= 0)
			break;

		/*
		 * set the timeout to significantly longer than it *should*
		 * take for the DAC to drain the DMA buffer
		 */
		tmo = (count * HZ) / (dmabuf->rate);
	}

	stop_dac(state);
	return 0;
}

static void i810_channel_interrupt(struct i810_card *card)
{
	int i, count;
	
#ifdef DEBUG_INTERRUPTS
	printk("CHANNEL ");
#endif
	for(i=0;i<NR_HW_CH;i++)
	{
		struct i810_state *state = card->states[i];
		struct i810_channel *c;
		struct dmabuf *dmabuf;
		unsigned long port = card->iobase;
		u16 status;
		
		if(!state)
			continue;
		if(!state->dmabuf.ready)
			continue;
		dmabuf = &state->dmabuf;
		if(dmabuf->enable & DAC_RUNNING) {
			c=dmabuf->write_channel;
		} else if(dmabuf->enable & ADC_RUNNING) {
			c=dmabuf->read_channel;
		} else	/* This can occur going from R/W to close */
			continue;
		
		port+=c->port;

		if(card->pci_id == PCI_DEVICE_ID_SI_7012)
			status = inw(port + OFF_PICB);
		else
			status = inw(port + OFF_SR);

#ifdef DEBUG_INTERRUPTS
		printk("NUM %d PORT %X IRQ ( ST%d ", c->num, c->port, status);
#endif
		if(status & DMA_INT_COMPLETE)
		{
			/* only wake_up() waiters if this interrupt signals
			 * us being beyond a userfragsize of data open or
			 * available, and i810_update_ptr() does that for
			 * us
			 */
			i810_update_ptr(state);
#ifdef DEBUG_INTERRUPTS
			printk("COMP %d ", dmabuf->hwptr /
					dmabuf->fragsize);
#endif
		}
		if(status & (DMA_INT_LVI | DMA_INT_DCH))
		{
			/* wake_up() unconditionally on LVI and DCH */
			i810_update_ptr(state);
			wakeup_sem(dmabuf->wait, true);
#ifdef DEBUG_INTERRUPTS
			if(status & DMA_INT_LVI)
				printk("LVI ");
			if(status & DMA_INT_DCH)
				printk("DCH -");
#endif
			count = dmabuf->count;
			if(dmabuf->enable & ADC_RUNNING)
				count = dmabuf->dmasize - count;
			if(count > (int)dmabuf->fragsize) {
				outb(inb(port+OFF_CR) | 1, port+OFF_CR);
#ifdef DEBUG_INTERRUPTS
				printk(" CONTINUE ");
#endif
			} else {
				if (dmabuf->enable & DAC_RUNNING)
					__stop_dac(state);
				if (dmabuf->enable & ADC_RUNNING)
					__stop_adc(state);
				dmabuf->enable = 0;
#ifdef DEBUG_INTERRUPTS
				printk(" STOP ");
#endif
			}
		}
		if(card->pci_id == PCI_DEVICE_ID_SI_7012)
			outw(status & DMA_INT_MASK, port + OFF_PICB);
		else
			outw(status & DMA_INT_MASK, port + OFF_SR);
	}
#ifdef DEBUG_INTERRUPTS
	printk(")\n");
#endif
}

static int i810_interrupt(int irq, void *dev_id, SysCallRegs_s *regs)
{
	struct i810_card *card = (struct i810_card *)dev_id;
	u32 status;

	spin_lock(&card->lock);

	status = inl(card->iobase + GLOB_STA);

	if(!(status & INT_MASK)) 
	{
		spin_unlock(&card->lock);
		return 0;  /* not for us */
	}

	if(status & (INT_PO|INT_PI|INT_MC))
		i810_channel_interrupt(card);

 	/* clear 'em */
	outl(status & INT_MASK, card->iobase + GLOB_STA);
	spin_unlock(&card->lock);
	return 0;
}


/* AtheOS additions */
DeviceOperations_s i810_mixer_fops = {
		i810_mixer_open,
		i810_mixer_release,
		i810_mixer_ioctl,
		NULL,
		NULL
};

DeviceOperations_s i810_dsp_fops = {
		i810_dsp_open,
		i810_dsp_release,
		i810_dsp_ioctl,
		i810_dsp_read,
		i810_dsp_write
};


/* in this loop, dmabuf.count signifies the amount of data that is
   waiting to be copied to the user's buffer.  It is filled by the dma
   machine and drained by this loop. */

//static ssize_t i810_read(struct file *file, char *buffer, size_t count, loff_t *ppos)
static int i810_dsp_read(void *node, void *cookie, off_t ppos, void *buffer, size_t count)
{
	struct i810_state *state = (struct i810_state *)i810_state_data;
	struct i810_card *card=state ? state->card : 0;
	struct dmabuf *dmabuf = &state->dmabuf;
	ssize_t ret;
	unsigned long flags;
	unsigned int swptr;
	int cnt;
	int pending;
        //DECLARE_WAITQUEUE(waita, current);

#ifdef DEBUG2
	printk("i810_audio: i810_read called, count = %d\n", count);
#endif

	if (dmabuf->mapped)
		return -ENXIO;
	if (dmabuf->enable & DAC_RUNNING)
		return -ENODEV;
	if (!dmabuf->read_channel) {
		dmabuf->ready = 0;
		dmabuf->read_channel = card->alloc_rec_pcm_channel(card);
		if (!dmabuf->read_channel) {
			return -EBUSY;
		}
	}
	if (!dmabuf->ready && (ret = prog_dmabuf(state, 1)))
		return ret;
	ret = 0;
	pending = 0;

	while (count > 0) {
		spin_lock_irqsave(&card->lock, flags);

		cnt = i810_get_available_read_data(state);
		swptr = dmabuf->swptr;
		// this is to make the copy_to_user simpler below
		if(cnt > (dmabuf->dmasize - swptr))
			cnt = dmabuf->dmasize - swptr;
		spin_unlock_irqrestore(&card->lock, flags);

		if (cnt > count)
			cnt = count;
		/* Lop off the last two bits to force the code to always
		 * write in full samples.  This keeps software that sets
		 * O_NONBLOCK but doesn't check the return value of the
		 * write call from getting things out of state where they
		 * think a full 4 byte sample was written when really only
		 * a portion was, resulting in odd sound and stereo
		 * hysteresis.
		 */
		//cnt &= ~0x3;
		if (cnt <= 0) {
			unsigned long tmo;
			/*
			 * Don't let us deadlock.  The ADC won't start if
			 * dmabuf->trigger isn't set.  A call to SETTRIGGER
			 * could have turned it off after we set it to on
			 * previously.
			 */
			dmabuf->trigger = PCM_ENABLE_INPUT;
			/*
			 * This does three things.  Updates LVI to be correct,
			 * makes sure the ADC is running, and updates the
			 * hwptr.
			 */
			i810_update_lvi(state,1);
			/* Set the timeout to how long it would take to fill
			 * two of our buffers.  If we haven't been woke up
			 * by then, then we know something is wrong.
			 */
			tmo = (dmabuf->dmasize * HZ * 2) / (dmabuf->rate * 4);
			/* There are two situations when sleep_on_timeout returns, one is when
			   the interrupt is serviced correctly and the process is waked up by
			   ISR ON TIME. Another is when timeout is expired, which means that
			   either interrupt is NOT serviced correctly (pending interrupt) or it
			   is TOO LATE for the process to be scheduled to run (scheduler latency)
			   which results in a (potential) buffer overrun. And worse, there is
			   NOTHING we can do to prevent it. */
			if (!sleep_on_sem(dmabuf->wait, tmo)) {
#ifdef DEBUG
				printk(KERN_ERR "i810_audio: recording schedule timeout, "
				       "dmasz %u fragsz %u count %i hwptr %u swptr %u\n",
				       dmabuf->dmasize, dmabuf->fragsize, dmabuf->count,
				       dmabuf->hwptr, dmabuf->swptr);
#endif
				/* a buffer overrun, we delay the recovery until next time the
				   while loop begin and we REALLY have space to record */
			}
			continue;
		}

		if (copy_to_user(buffer, dmabuf->rawbuf + swptr, cnt)) {
			if (!ret) ret = -EFAULT;
			goto done;
		}

		swptr = MODULOP2(swptr + cnt, dmabuf->dmasize);

		spin_lock_irqsave(&card->lock, flags);

		dmabuf->swptr = swptr;
		pending = dmabuf->count -= cnt;
		spin_unlock_irqrestore(&card->lock, flags);

		count -= cnt;
		buffer += cnt;
		ret += cnt;
	}
 done:
	pending = dmabuf->dmasize - pending;
	if (dmabuf->enable || pending >= dmabuf->userfragsize)
		i810_update_lvi(state, 1);
	return ret;
}

/* in this loop, dmabuf.count signifies the amount of data that is waiting to be dma to
   the soundcard.  it is drained by the dma machine and filled by this loop. */
//static ssize_t i810_write(struct file *file, const char *buffer, size_t count, loff_t *ppos)
int i810_dsp_write(void *node, void *cookie, off_t ppos, const void *buffer, size_t count)
{
	struct i810_state *state = (struct i810_state *) i810_state_data;
	struct i810_card *card=state ? state->card : 0;
	struct dmabuf *dmabuf = &state->dmabuf;
	ssize_t ret;
	unsigned long flags;
	unsigned int swptr = 0;
	int cnt;
	int pending;
        //DECLARE_WAITQUEUE(waita, current);

#ifdef DEBUG2
	printk("i810_audio: i810_write called, count = %d\n", count);
#endif

	if (dmabuf->mapped)
		return -ENXIO;
	if (dmabuf->enable & ADC_RUNNING)
		return -ENODEV;
	if (!dmabuf->write_channel) {
		dmabuf->ready = 0;
		dmabuf->write_channel = card->alloc_pcm_channel(card);
		if(!dmabuf->write_channel)
			return -EBUSY;
	}
	if (!dmabuf->ready && (ret = prog_dmabuf(state, 0)))
		return ret;
	ret = 0;
	pending = 0;

	while (count > 0) {
		spin_lock_irqsave(&state->card->lock, flags);

		cnt = i810_get_free_write_space(state);
		swptr = dmabuf->swptr;
		/* Bound the maximum size to how much we can copy to the
		 * dma buffer before we hit the end.  If we have more to
		 * copy then it will get done in a second pass of this
		 * loop starting from the beginning of the buffer.
		 */
		if(cnt > (dmabuf->dmasize - swptr))
			cnt = dmabuf->dmasize - swptr;
		spin_unlock_irqrestore(&state->card->lock, flags);

#ifdef DEBUG2
		printk(KERN_INFO "i810_audio: i810_write: %d bytes available space\n", cnt);
#endif
		if (cnt > count)
			cnt = count;
		/* Lop off the last two bits to force the code to always
		 * write in full samples.  This keeps software that sets
		 * O_NONBLOCK but doesn't check the return value of the
		 * write call from getting things out of state where they
		 * think a full 4 byte sample was written when really only
		 * a portion was, resulting in odd sound and stereo
		 * hysteresis.
		 */
		//cnt &= ~0x3;
		if (cnt <= 0) {
			unsigned long tmo;
			// There is data waiting to be played
			/*
			 * Force the trigger setting since we would
			 * deadlock with it set any other way
			 */
			dmabuf->trigger = PCM_ENABLE_OUTPUT;
			i810_update_lvi(state,0);

			/* Not strictly correct but works */
			tmo = (dmabuf->dmasize * HZ * 2) / (dmabuf->rate * 4);
			/* There are two situations when sleep_on_timeout returns, one is when
			   the interrupt is serviced correctly and the process is waked up by
			   ISR ON TIME. Another is when timeout is expired, which means that
			   either interrupt is NOT serviced correctly (pending interrupt) or it
			   is TOO LATE for the process to be scheduled to run (scheduler latency)
			   which results in a (potential) buffer underrun. And worse, there is
			   NOTHING we can do to prevent it. */
			if (sleep_on_sem(dmabuf->wait, tmo * 1000) < 0) {
#ifdef DEBUG
				printk(KERN_ERR "i810_audio: playback schedule timeout, "
				       "dmasz %u fragsz %u count %i hwptr %u swptr %u\n",
				       dmabuf->dmasize, dmabuf->fragsize, dmabuf->count,
				       dmabuf->hwptr, dmabuf->swptr);
#endif
				/* a buffer underrun, we delay the recovery until next time the
				   while loop begin and we REALLY have data to play */
			}
			continue;
		}
		if (copy_from_user(dmabuf->rawbuf+swptr,buffer,cnt)) {
			if (!ret) ret = -EFAULT;
			goto ret;
		}

		swptr = MODULOP2(swptr + cnt, dmabuf->dmasize);

		spin_lock_irqsave(&state->card->lock, flags);

		dmabuf->swptr = swptr;
		pending = dmabuf->count += cnt;

		count -= cnt;
		buffer += cnt;
		ret += cnt;
		spin_unlock_irqrestore(&state->card->lock, flags);
	}
ret:
	if (dmabuf->enable || pending >= dmabuf->userfragsize)
		i810_update_lvi(state, 0);

	return ret;
}

//static int i810_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
int i810_dsp_ioctl(void *node, void *cookie, uint32 cmd, void *args, bool b_fromkernel)
{
	struct i810_state *state = (struct i810_state *) i810_state_data;
	struct i810_channel *c = NULL;
	struct dmabuf *dmabuf = &state->dmabuf;
	unsigned long flags;
	unsigned long arg = (unsigned long)args;
	audio_buf_info abinfo;
	count_info cinfo;
	int val = 0, ret;

#ifdef DEBUG
	printk("i810_audio: i810_ioctl, arg=0x%x, cmd=", arg ? *(int *)arg : 0);
#endif

	switch (cmd) 
	{
	case OSS_GETVERSION:
#ifdef DEBUG
		printk("OSS_GETVERSION\n");
#endif
		put_user(SOUND_VERSION, (int *)arg);
		return 0;

	case SNDCTL_DSP_RESET:
#ifdef DEBUG
		printk("SNDCTL_DSP_RESET\n");
#endif
		spin_lock_irqsave(&state->card->lock, flags);
		if (dmabuf->enable == DAC_RUNNING) {
			c = dmabuf->write_channel;
			__stop_dac(state);
		}
		if (dmabuf->enable == ADC_RUNNING) {
			c = dmabuf->read_channel;
			__stop_adc(state);
		}
		if (c != NULL) {
			outb(2, state->card->iobase+c->port+OFF_CR);   /* reset DMA machine */
			while ( inb(state->card->iobase+c->port+OFF_CR) & 2 );
			outl((u32)state->card->chandma +
			    c->num*sizeof(struct i810_channel),
			    state->card->iobase+c->port+OFF_BDBAR);
			CIV_TO_LVI(state->card->iobase+c->port, 0);
		}

		spin_unlock_irqrestore(&state->card->lock, flags);
		dmabuf->ready = 0;
		dmabuf->swptr = dmabuf->hwptr = 0;
		dmabuf->count = dmabuf->total_bytes = 0;
		return 0;

	case SNDCTL_DSP_SYNC:
#ifdef DEBUG
		printk("SNDCTL_DSP_SYNC\n");
#endif
		if (dmabuf->enable != DAC_RUNNING)
			return 0;
		if((val = drain_dac(state, 1)))
			return val;
		dmabuf->total_bytes = 0;
		return 0;

	case SNDCTL_DSP_SPEED: /* set sample rate */
#ifdef DEBUG
		printk("SNDCTL_DSP_SPEED\n");
#endif
		get_user(val, (int *)arg);
		if (val >= 0) {
				stop_dac(state);
				dmabuf->ready = 0;
				spin_lock_irqsave(&state->card->lock, flags);
				i810_set_dac_rate(state, val);
				spin_unlock_irqrestore(&state->card->lock, flags);
		}
		put_user(dmabuf->rate, (int *)arg);
		return 0;

	case SNDCTL_DSP_STEREO: /* set stereo or mono channel */
#ifdef DEBUG
		printk("SNDCTL_DSP_STEREO\n");
#endif
		if (dmabuf->enable & DAC_RUNNING) {
			stop_dac(state);
		}
		if (dmabuf->enable & ADC_RUNNING) {
			stop_adc(state);
		}
		put_user(1, (int *)arg);
		return 0;

	case SNDCTL_DSP_GETBLKSIZE:
		if (!dmabuf->ready && (val = prog_dmabuf(state, 0)))
			return val;
#ifdef DEBUG
		printk("SNDCTL_DSP_GETBLKSIZE %d\n", dmabuf->userfragsize);
#endif
		put_user(dmabuf->userfragsize, (int *)arg);
		return 0;

	case SNDCTL_DSP_GETFMTS: /* Returns a mask of supported sample format*/
#ifdef DEBUG
		printk("SNDCTL_DSP_GETFMTS\n");
#endif
		put_user(AFMT_S16_LE, (int *)arg);
		return 0;

	case SNDCTL_DSP_SETFMT: /* Select sample format */
#ifdef DEBUG
		printk("SNDCTL_DSP_SETFMT\n");
#endif
		put_user(AFMT_S16_LE, (int *)arg);
		return 0;

	case SNDCTL_DSP_CHANNELS:
#ifdef DEBUG
		printk("SNDCTL_DSP_CHANNELS\n");
#endif
		get_user(val, (int *)arg);

		if (val > 0) {
			if (dmabuf->enable & DAC_RUNNING) {
				stop_dac(state);
			}
			if (dmabuf->enable & ADC_RUNNING) {
				stop_adc(state);
			}
		} else {
			put_user(state->card->channels, (int *)arg);
			return 0;
		}
		
		outl(inl(state->card->iobase + GLOB_CNT) & 0xffcfffff,
				     state->card->iobase + GLOB_CNT);

		put_user(2, (int *)arg);
		return 0;
		/* No multi-channel support for Syllable */

	case SNDCTL_DSP_POST: /* the user has sent all data and is notifying us */
		/* we update the swptr to the end of the last sg segment then return */
#ifdef DEBUG
		printk("SNDCTL_DSP_POST\n");
#endif
		if(!dmabuf->ready || (dmabuf->enable != DAC_RUNNING))
			return 0;
		if((dmabuf->swptr % dmabuf->fragsize) != 0) {
			val = dmabuf->fragsize - (dmabuf->swptr % dmabuf->fragsize);
			dmabuf->swptr += val;
			dmabuf->count += val;
		}
		return 0;

	case SNDCTL_DSP_SUBDIVIDE:
		if (dmabuf->subdivision)
			return -EINVAL;
		get_user(val, (int *)arg);
		if (val != 1 && val != 2 && val != 4)
			return -EINVAL;
#ifdef DEBUG
		printk("SNDCTL_DSP_SUBDIVIDE %d\n", val);
#endif
		dmabuf->subdivision = val;
		dmabuf->ready = 0;
		return 0;

	case SNDCTL_DSP_SETFRAGMENT:
		get_user(val, (int *)arg);

		dmabuf->ossfragsize = 1<<(val & 0xffff);
		dmabuf->ossmaxfrags = (val >> 16) & 0xffff;
		if (!dmabuf->ossfragsize || !dmabuf->ossmaxfrags)
			return -EINVAL;
		/*
		 * Bound the frag size into our allowed range of 256 - 4096
		 */
		if (dmabuf->ossfragsize < 256)
			dmabuf->ossfragsize = 256;
		else if (dmabuf->ossfragsize > 4096)
			dmabuf->ossfragsize = 4096;
		/*
		 * The numfrags could be something reasonable, or it could
		 * be 0xffff meaning "Give me as much as possible".  So,
		 * we check the numfrags * fragsize doesn't exceed our
		 * 64k buffer limit, nor is it less than our 8k minimum.
		 * If it fails either one of these checks, then adjust the
		 * number of fragments, not the size of them.  It's OK if
		 * our number of fragments doesn't equal 32 or anything
		 * like our hardware based number now since we are using
		 * a different frag count for the hardware.  Before we get
		 * into this though, bound the maxfrags to avoid overflow
		 * issues.  A reasonable bound would be 64k / 256 since our
		 * maximum buffer size is 64k and our minimum frag size is
		 * 256.  On the other end, our minimum buffer size is 8k and
		 * our maximum frag size is 4k, so the lower bound should
		 * be 2.
		 */

		if(dmabuf->ossmaxfrags > 256)
			dmabuf->ossmaxfrags = 256;
		else if (dmabuf->ossmaxfrags < 2)
			dmabuf->ossmaxfrags = 2;

		val = dmabuf->ossfragsize * dmabuf->ossmaxfrags;
		while (val < 8192) {
		    val <<= 1;
		    dmabuf->ossmaxfrags <<= 1;
		}
		while (val > 65536) {
		    val >>= 1;
		    dmabuf->ossmaxfrags >>= 1;
		}
		dmabuf->ready = 0;
#ifdef DEBUG
		printk("SNDCTL_DSP_SETFRAGMENT 0x%x, %d, %d\n", val,
			dmabuf->ossfragsize, dmabuf->ossmaxfrags);
#endif

		return 0;

	case SNDCTL_DSP_GETOSPACE:
		if (!dmabuf->ready && (val = prog_dmabuf(state, 0)) != 0)
			return val;
		spin_lock_irqsave(&state->card->lock, flags);
		i810_update_ptr(state);
		abinfo.fragsize = dmabuf->userfragsize;
		abinfo.fragstotal = dmabuf->userfrags;
		if (dmabuf->mapped)
 			abinfo.bytes = dmabuf->dmasize;
  		else
 			abinfo.bytes = i810_get_free_write_space(state);
		abinfo.fragments = abinfo.bytes / dmabuf->userfragsize;
		spin_unlock_irqrestore(&state->card->lock, flags);
#if defined(DEBUG) || defined(DEBUG_MMAP)
		printk("SNDCTL_DSP_GETOSPACE %d, %d, %d, %d\n", abinfo.bytes,
			abinfo.fragsize, abinfo.fragments, abinfo.fragstotal);
#endif
		return copy_to_user((void *)arg, &abinfo, sizeof(abinfo)) ? -EFAULT : 0;

	case SNDCTL_DSP_GETOPTR:
		if (!dmabuf->ready && (val = prog_dmabuf(state, 0)) != 0)
			return val;
		spin_lock_irqsave(&state->card->lock, flags);
		val = i810_get_free_write_space(state);
		cinfo.bytes = dmabuf->total_bytes;
		cinfo.ptr = dmabuf->hwptr;
		cinfo.blocks = val/dmabuf->userfragsize;
		if (dmabuf->mapped && (dmabuf->trigger & PCM_ENABLE_OUTPUT)) {
			dmabuf->count += val;
			dmabuf->swptr = (dmabuf->swptr + val) % dmabuf->dmasize;
			__i810_update_lvi(state, 0);
		}
		spin_unlock_irqrestore(&state->card->lock, flags);
#if defined(DEBUG) || defined(DEBUG_MMAP)
		printk("SNDCTL_DSP_GETOPTR %d, %d, %d, %d\n", cinfo.bytes,
			cinfo.blocks, cinfo.ptr, dmabuf->count);
#endif
		return copy_to_user((void *)arg, &cinfo, sizeof(cinfo)) ? -EFAULT : 0;

	/* No recording support for Syllable */
	case SNDCTL_DSP_GETISPACE:
	case SNDCTL_DSP_GETIPTR:
		return -EINVAL;

	case SNDCTL_DSP_NONBLOCK:
#ifdef DEBUG
		printk("SNDCTL_DSP_NONBLOCK\n");
#endif
		//file->f_flags |= O_NONBLOCK;
		return 0;

	case SNDCTL_DSP_GETCAPS:
#ifdef DEBUG
		printk("SNDCTL_DSP_GETCAPS\n");
#endif
	    put_user(DSP_CAP_REALTIME|DSP_CAP_TRIGGER|DSP_CAP_MMAP|DSP_CAP_BIND, (int *)arg);
		return 0;

	case SNDCTL_DSP_GETTRIGGER:
		val = 0;
#ifdef DEBUG
		printk("SNDCTL_DSP_GETTRIGGER 0x%x\n", dmabuf->trigger);
#endif
		put_user(dmabuf->trigger, (int *)arg);
		return 0;

	case SNDCTL_DSP_SETTRIGGER:
		get_user(val, (int *)arg);
#if defined(DEBUG) || defined(DEBUG_MMAP)
		printk("SNDCTL_DSP_SETTRIGGER 0x%x\n", val);
#endif
		stop_dac(state);
		dmabuf->trigger = val;
		if((val & PCM_ENABLE_OUTPUT) && !(dmabuf->enable & DAC_RUNNING)) {
			if (!dmabuf->write_channel) {
				dmabuf->ready = 0;
				dmabuf->write_channel = state->card->alloc_pcm_channel(state->card);
				if (!dmabuf->write_channel)
					return -EBUSY;
			}
			if (!dmabuf->ready && (ret = prog_dmabuf(state, 0)))
				return ret;
			if (dmabuf->mapped) {
				spin_lock_irqsave(&state->card->lock, flags);
				i810_update_ptr(state);
				dmabuf->count = 0;
				dmabuf->swptr = dmabuf->hwptr;
				dmabuf->count = i810_get_free_write_space(state);
				dmabuf->swptr = (dmabuf->swptr + dmabuf->count) % dmabuf->dmasize;
				__i810_update_lvi(state, 0);
				spin_unlock_irqrestore(&state->card->lock, flags);
			}
			i810_update_lvi(state, 0);
			start_dac(state);
		}
		return 0;

	case SNDCTL_DSP_SETDUPLEX:
#ifdef DEBUG
		printk("SNDCTL_DSP_SETDUPLEX\n");
#endif
		return -EINVAL;

	case SNDCTL_DSP_GETODELAY:
		spin_lock_irqsave(&state->card->lock, flags);
		i810_update_ptr(state);
		val = dmabuf->count;
		spin_unlock_irqrestore(&state->card->lock, flags);
#ifdef DEBUG
		printk("SNDCTL_DSP_GETODELAY %d\n", dmabuf->count);
#endif
		put_user(val, (int *)arg);
		return 0;

	case SOUND_PCM_READ_RATE:
#ifdef DEBUG
		printk("SOUND_PCM_READ_RATE %d\n", dmabuf->rate);
#endif
		put_user(dmabuf->rate, (int *)arg);
		return 0;

	case SOUND_PCM_READ_CHANNELS:
#ifdef DEBUG
		printk("SOUND_PCM_READ_CHANNELS\n");
#endif
		put_user(2, (int *)arg);
		return 0;

	case SOUND_PCM_READ_BITS:
#ifdef DEBUG
		printk("SOUND_PCM_READ_BITS\n");
#endif
		put_user(AFMT_S16_LE, (int *)arg);
		return 0;

	/* No S/PDIF support for Syllable */
	case SNDCTL_DSP_SETSPDIF: /* Set S/PDIF Control register */
	case SNDCTL_DSP_GETSPDIF: /* Get S/PDIF Control register */
		printk("i810_audio: S/PDIF support disabled.\n");
		return -EINVAL;

	/* No surround channels to bind for Syllable */   			

	case SNDCTL_DSP_MAPINBUF:
	case SNDCTL_DSP_MAPOUTBUF:
	case SNDCTL_DSP_SETSYNCRO:
	case SOUND_PCM_WRITE_FILTER:
	case SOUND_PCM_READ_FILTER:
#ifdef DEBUG
		printk("SNDCTL_* -EINVAL\n");
#endif
		return -EINVAL;
	}
	return -EINVAL;
}


//static int i810_open(struct inode *inode, struct file *file)
static status_t i810_dsp_open (void* pNode, uint32 nFlags, void **pCookie )
{
	int i = 0;
	struct i810_card *card = devs;
	struct i810_state *state = NULL;
	struct dmabuf *dmabuf = NULL;
	
	//printk( "i810_dps_open\n" );

	/* find an available virtual channel (instance of /dev/dsp) */
	while (card != NULL) {
		/*
		 * If we are initializing and then fail, card could go
		 * away unuexpectedly while we are in the for() loop.
		 * So, check for card on each iteration before we check
		 * for card->initializing to avoid a possible oops.
		 * This usually only matters for times when the driver is
		 * autoloaded by kmod.
		 */
		for (i = 0; i < NR_HW_CH && card && !card->initializing; i++) {
			if (card->states[i] == NULL) {
				state = card->states[i] = (struct i810_state *)
					kmalloc(sizeof(struct i810_state), MEMF_KERNEL);
				if (state == NULL)
					return -ENOMEM;
				memset(state, 0, sizeof(struct i810_state));
				dmabuf = &state->dmabuf;
				goto found_virt;
			}
		}
		card = card->next;
	}
	/* no more virtual channel avaiable */
	if (!state)
		return -ENODEV;

found_virt:
	/* initialize the virtual channel */
	state->virt = i;
	state->card = card;
	state->magic = I810_STATE_MAGIC;
	state->open_sem = create_semaphore("i810_open_sem", 1, 0); 
	i810_state_data = state; /* Therefore only one card is supported! */
	dmabuf->wait = create_semaphore("i810_wait", 0, 0);
	dmabuf->trigger = 0;

	/* allocate hardware channels */
	if((dmabuf->write_channel = card->alloc_pcm_channel(card)) == NULL) {
		/* make sure we free the record channel allocated above */
		kfree (card->states[i]);
		card->states[i] = NULL;
		return -EBUSY;
	}
	/* Initialize to 8kHz?  What if we don't support 8kHz? */
	/*  Let's change this to check for S/PDIF stuff */
	
	dmabuf->trigger |= PCM_ENABLE_OUTPUT;
	i810_set_dac_rate(state, 44100); /* required for Syllable */
	/* Put the ACLink in 2 channel mode by default */
	i = inl(card->iobase + GLOB_CNT);
	outl(i & 0xffcfffff, card->iobase + GLOB_CNT);

	/* set default sample format. According to OSS Programmer's Guide  /dev/dsp
	   should be default to unsigned 8-bits, mono, with sample rate 8kHz and
	   /dev/dspW will accept 16-bits sample, but we don't support those so we
	   set it immediately to stereo and 16bit, which is all we do support */
	dmabuf->fmt |= I810_FMT_16BIT | I810_FMT_STEREO;
	dmabuf->ossfragsize = 0;
	dmabuf->ossmaxfrags  = 0;
	dmabuf->subdivision  = 0;

	state->open_mode = FMODE_WRITE;
	
	//printk( "Device opened\n" );

	return 0;
}

//static int i810_release(struct inode *inode, struct file *file)
static status_t i810_dsp_release( void *pNode, void *pCookie )
{
	struct i810_state *state = (struct i810_state *) i810_state_data;
	struct i810_card *card = state->card;
	struct dmabuf *dmabuf = &state->dmabuf;
	unsigned long flags;
	
	//printk( "i810_dps_release\n" );

	/* stop DMA state machine and free DMA buffers/channels */
	if(dmabuf->trigger & PCM_ENABLE_OUTPUT) {
		drain_dac(state, 0);
	}
	if(dmabuf->trigger & PCM_ENABLE_INPUT) {
		stop_adc(state);
	}
	spin_lock_irqsave(&card->lock, flags);
	dealloc_dmabuf(state);

	state->card->free_pcm_channel(state->card, dmabuf->write_channel->num);

	state->card->states[state->virt] = NULL;
	kfree(state);
	spin_unlock_irqrestore(&card->lock, flags);
	
	//printk( "Device closed\n" );

	return 0;
}

/* Write AC97 codec registers */
static u16 i810_ac97_get(struct ac97_codec *dev, u8 reg)
{
	struct i810_card *card = dev->private_data;
	int count = 100;
	u16 reg_set = IO_REG_OFF(dev) | (reg&0x7f);
	
	while(count-- && (inb(card->iobase + CAS) & 1)) 
		udelay(1);
	
	return inw(card->ac97base + reg_set);
}

static void i810_ac97_set(struct ac97_codec *dev, u8 reg, u16 data)
{
	struct i810_card *card = dev->private_data;
	int count = 100;
	u16 reg_set = IO_REG_OFF(dev) | (reg&0x7f);
	
	while(count-- && (inb(card->iobase + CAS) & 1)) 
		udelay(1);
	
        outw(data, card->ac97base + reg_set);
}

/* OSS /dev/mixer file operation methods */

//static int i810_open_mixdev(struct inode *inode, struct file *file)
static status_t i810_mixer_open(void* pNode, uint32 nFlags, void **pCookie)
{
	int i;
	struct i810_card *card = devs;
	
	//printk( "i810_mixer_open\n" );

	for (card = devs; card != NULL; card = card->next) {
		for (i = 0; i < NR_AC97 && card; i++) 
			if (card->ac97_codec[i] != NULL) {
				pNode = card->ac97_codec[i];
				return 0;
			}
	}
	return -ENODEV;
}

static status_t i810_mixer_release(void *pNode, void *pCookie)
{
	return 0;
}

//static int i810_ioctl_mixdev(struct inode *inode, struct file *file, unsigned int cmd,
//				unsigned long arg)
static status_t i810_mixer_ioctl (void *node, void *cookie, uint32 cmd, void *args, bool b_fromkernel)
{
	unsigned long arg = (unsigned long)args;
	struct ac97_codec *codec = (struct ac97_codec *) node;

	return codec->mixer_ioctl(codec, cmd, arg);
}

/* AC97 codec initialisation.  These small functions exist so we don't
   duplicate code between module init and apm resume */

static inline int i810_ac97_exists(struct i810_card *card, int ac97_number)
{
	u32 reg = inl(card->iobase + GLOB_STA);
	switch (ac97_number) {
	case 0:
		return reg & (1<<8);
	case 1: 
		return reg & (1<<9);
	case 2:
		return reg & (1<<28);
	}
	return 0;
}

static inline int i810_ac97_enable_variable_rate(struct ac97_codec *codec)
{
	i810_ac97_set(codec, AC97_EXTENDED_STATUS, 9);
	i810_ac97_set(codec,AC97_EXTENDED_STATUS,
		      i810_ac97_get(codec, AC97_EXTENDED_STATUS)|0xE800);
	
	return (i810_ac97_get(codec, AC97_EXTENDED_STATUS)&1);
}


static int i810_ac97_probe_and_powerup(struct i810_card *card,struct ac97_codec *codec)
{
	/* Returns 0 on failure */
	int i;

	if (ac97_probe_codec(codec) == 0) return 0;
	
	/* power it all up */
	i810_ac97_set(codec, AC97_POWER_CONTROL,
		      i810_ac97_get(codec, AC97_POWER_CONTROL) & ~0x7f00);

	/* wait for analog ready */
	for (i=100; i && ((i810_ac97_get(codec, AC97_POWER_CONTROL) & 0xf) != 0xf); i--)
	{
		snooze( 50000 );
		//set_current_state(TASK_UNINTERRUPTIBLE);
		//schedule_timeout(HZ/20);
	} 
	return i;
}

/**
 *	i810_ac97_power_up_bus	-	bring up AC97 link
 *	@card : ICH audio device to power up
 *
 *	Bring up the ACLink AC97 codec bus
 */
 
static int i810_ac97_power_up_bus(struct i810_card *card)
{	
	u32 reg = inl(card->iobase + GLOB_CNT);
	int i;
	int primary_codec_id = 0;

	if((reg&2)==0)	/* Cold required */
		reg|=2;
	else
		reg|=4;	/* Warm */
		
	reg&=~8;	/* ACLink on */
	
	/* At this point we deassert AC_RESET # */
	outl(reg , card->iobase + GLOB_CNT);

	/* We must now allow time for the Codec initialisation.
	   600mS is the specified time */
	   	
	for(i=0;i<10;i++)
	{
		if((inl(card->iobase+GLOB_CNT)&4)==0)
			break;

		
		snooze( 50000 );
		//set_current_state(TASK_UNINTERRUPTIBLE);
		//schedule_timeout(HZ/20);
	}
	if(i==10)
	{
		printk("i810_audio: AC'97 reset failed.\n");
		return 0;
	}
	
	snooze( 500000 );

	//set_current_state(TASK_UNINTERRUPTIBLE);
	//schedule_timeout(HZ/2);

	/*
	 *	See if the primary codec comes ready. This must happen
	 *	before we start doing DMA stuff
	 */	
	/* see i810_ac97_init for the next 7 lines (jsaw) */
	inw(card->ac97base);

	if(! i810_ac97_exists(card, primary_codec_id))
	{
		printk("i810_audio: Codec not ready.. wait..\n");
		snooze( 1000000 );
		//set_current_state(TASK_UNINTERRUPTIBLE);
		//schedule_timeout(HZ);	/* actually 600mS by the spec */

		if(i810_ac97_exists(card, primary_codec_id))
			printk("OK\n");
		else 
			printk("no response.\n");
	}
	inw(card->ac97base);
	return 1;
}

static int i810_ac97_init(struct i810_card *card)
{
	int num_ac97 = 0;
	int ac97_id;
	int total_channels = 0;
	//int nr_ac97_max = card_cap[card->pci_id_internal].nr_ac97;
	struct ac97_codec *codec;
	u16 eid;
	u32 reg;

	if(!i810_ac97_power_up_bus(card)) return 0;

	/* Number of channels supported */
	/* What about the codec?  Just because the ICH supports */
	/* multiple channels doesn't mean the codec does.       */
	/* we'll have to modify this in the codec section below */
	/* to reflect what the codec has.                       */
	/* ICH and ICH0 only support 2 channels so don't bother */
	/* to check....                                         */

	card->channels = 2;
	reg = inl(card->iobase + GLOB_STA);
	if ( reg & 0x0200000 )
		card->channels = 6;
	else if ( reg & 0x0100000 )
		card->channels = 4;
	printk("i810_audio: Audio Controller supports %d channels.\n", card->channels);
	printk("i810_audio: Defaulting to base 2 channel mode.\n");
	reg = inl(card->iobase + GLOB_CNT);
	outl(reg & 0xffcfffff, card->iobase + GLOB_CNT);
		
	for (num_ac97 = 0; num_ac97 < NR_AC97; num_ac97++) 
		card->ac97_codec[num_ac97] = NULL;

	/*@FIXME I don't know, if I'm playing to safe here... (jsaw) */
	//if ((nr_ac97_max > 2) && !card->use_mmio) nr_ac97_max = 2;

	for (num_ac97 = 0; num_ac97 < NR_AC97; num_ac97++) {
		/* codec reset */
		printk("i810_audio: Resetting connection %d\n", num_ac97);
		/*if (card->use_mmio) readw(card->ac97base_mmio + 0x80*num_ac97);
		else */
		inw(card->ac97base + 0x80*num_ac97);

		/* If we have the SDATA_IN Map Register, as on ICH4, we
		   do not loop thru all possible codec IDs but thru all 
		   possible IO channels. Bit 0:1 of SDM then holds the 
		   last codec ID spoken to. 
		*/
		#if 0
		if ((card->pci_id == PCI_DEVICE_ID_INTEL_ICH4 || card->pci_id == PCI_DEVICE_ID_INTEL_ICH5)
		    && (card->use_mmio)) {
			ac97_id = (int) readl(card->iobase_mmio + SDM) & 0x3;
			printk("i810_audio: Connection %d with codec id %d\n",
			       num_ac97, ac97_id);
		}
		else {
		#endif
			ac97_id = num_ac97;
		//}

		/* The ICH programmer's reference says you should   */
		/* check the ready status before probing. So we chk */
		/*   What do we do if it's not ready?  Wait and try */
		/*   again, or abort?                               */
		if (!i810_ac97_exists(card, ac97_id)) {
			if(num_ac97 == 0)
				printk("i810_audio: Primary codec not ready.\n");
		}
		
		if ((codec = ac97_alloc_codec()) == NULL)
			return -ENOMEM;

		/* initialize some basic codec information, other fields will be filled
		   in ac97_probe_codec */
		codec->private_data = card;
		codec->id = ac97_id;
		card->ac97_id_map[ac97_id] = num_ac97 * 0x80;

		#if 0
		if (card->use_mmio) {	
			codec->codec_read = i810_ac97_get_mmio;
			codec->codec_write = i810_ac97_set_mmio;
		}
		else {
		#endif
			codec->codec_read = i810_ac97_get;
			codec->codec_write = i810_ac97_set;
		//}
	
		if(!i810_ac97_probe_and_powerup(card,codec)) {
			printk("i810_audio: timed out waiting for codec %d analog ready.\n", ac97_id);
			ac97_release_codec(codec);
			break;	/* it didn't work */
		}
		/* Store state information about S/PDIF transmitter */
		card->ac97_status = 0;
		
		/* Don't attempt to get eid until powerup is complete */
		eid = i810_ac97_get(codec, AC97_EXTENDED_ID);

		if(eid==0xFFFF)
		{
			printk("i810_audio: no codec attached ?\n");
			ac97_release_codec(codec);
			break;
		}
		
		/* Check for an AC97 1.0 soft modem (ID1) */
		
		if(codec->modem)
		{
			printk("i810_audio: codec %d is a softmodem - skipping.\n", ac97_id);
			ac97_release_codec(codec);
			continue;
		}
		
		card->ac97_features = eid;

		/* Now check the codec for useful features to make up for
		   the dumbness of the 810 hardware engine */

		if(!(eid&0x0001))
			printk("i810_audio: only 48Khz playback available.\n");
		else
		{
			if(!i810_ac97_enable_variable_rate(codec)) {
				printk("i810_audio: Codec refused to allow VRA, using 48Khz only.\n");
				card->ac97_features&=~1;
			}			
		}
   		
		/* Turn on the amplifier */

		codec->codec_write(codec, AC97_POWER_CONTROL, 
			 codec->codec_read(codec, AC97_POWER_CONTROL) & ~0x8000);
				
		/* Determine how many channels the codec(s) support   */
		/*   - The primary codec always supports 2            */
		/*   - If the codec supports AMAP, surround DACs will */
		/*     automaticlly get assigned to slots.            */
		/*     * Check for surround DACs and increment if     */
		/*       found.                                       */
		/*   - Else check if the codec is revision 2.2        */
		/*     * If surround DACs exist, assign them to slots */
		/*       and increment channel count.                 */

		/* All of this only applies to ICH2 and above. ICH    */
		/* and ICH0 only support 2 channels.  ICH2 will only  */
		/* support multiple codecs in a "split audio" config. */
		/* as described above.                                */

		/* TODO: Remove all the debugging messages!           */

		if((eid & 0xc000) == 0) /* primary codec */
			total_channels += 2; 

		if(eid & 0x200) { /* GOOD, AMAP support */
			if (eid & 0x0080) /* L/R Surround channels */
				total_channels += 2;
			if (eid & 0x0140) /* LFE and Center channels */
				total_channels += 2;
			printk("i810_audio: AC'97 codec %d supports AMAP, total channels = %d\n", ac97_id, total_channels);
		} else if (eid & 0x0400) {  /* this only works on 2.2 compliant codecs */
			eid &= 0xffcf;
			if((eid & 0xc000) != 0)	{
				switch ( total_channels ) {
					case 2:
						/* Set dsa1, dsa0 to 01 */
						eid |= 0x0010;
						break;
					case 4:
						/* Set dsa1, dsa0 to 10 */
						eid |= 0x0020;
						break;
					case 6:
						/* Set dsa1, dsa0 to 11 */
						eid |= 0x0030;
						break;
				}
				total_channels += 2;
			}
			i810_ac97_set(codec, AC97_EXTENDED_ID, eid);
			eid = i810_ac97_get(codec, AC97_EXTENDED_ID);
			printk("i810_audio: AC'97 codec %d, new EID value = 0x%04x\n", ac97_id, eid);
			if (eid & 0x0080) /* L/R Surround channels */
				total_channels += 2;
			if (eid & 0x0140) /* LFE and Center channels */
				total_channels += 2;
			printk("i810_audio: AC'97 codec %d, DAC map configured, total channels = %d\n", ac97_id, total_channels);
		} else {
			printk("i810_audio: AC'97 codec %d Unable to map surround DAC's (or DAC's not present), total channels = %d\n", ac97_id, total_channels);
		}
		
		if( create_device_node( card->nDeviceID, card->pci_dev->nHandle, 
#ifdef ENABLE_DEVNODE_052
			"sound/i810/mixer/0", &i810_mixer_fops, codec ) < 0 ) {
#else
			"sound/mixer", &i810_mixer_fops, codec ) < 0 ) {
#endif
			printk( "i810_audio: failed to create mixer node \n");
			kfree(codec);
			break;
		}

		card->ac97_codec[num_ac97] = codec;
	}

	/* pick the minimum of channels supported by ICHx or codec(s) */
	card->channels = (card->channels > total_channels)?total_channels:card->channels;

	return num_ac97;
}

static void i810_configure_clocking (void)
{
	struct i810_card *card;
	struct i810_state *state;
	struct dmabuf *dmabuf;
	unsigned int i, offset, new_offset;
	int flags;

	card = devs;
	/* We could try to set the clocking for multiple cards, but can you even have
	 * more than one i810 in a machine?  Besides, clocking is global, so unless
	 * someone actually thinks more than one i810 in a machine is possible and
	 * decides to rewrite that little bit, setting the rate for more than one card
	 * is a waste of time.
	 */
	if(card != NULL) {
		state = card->states[0] = (struct i810_state *)
					kmalloc(sizeof(struct i810_state), MEMF_KERNEL|MEMF_CLEAR);
		if (state == NULL)
			return;
		dmabuf = &state->dmabuf;

		dmabuf->write_channel = card->alloc_pcm_channel(card);
		state->virt = 0;
		state->card = card;
		state->magic = I810_STATE_MAGIC;
		state->open_sem = create_semaphore("i810_open_sem", 1, 0); 
		dmabuf->wait = create_semaphore("i810_wait", 1, 0); 
		dmabuf->fmt = I810_FMT_STEREO | I810_FMT_16BIT;
		dmabuf->trigger = PCM_ENABLE_OUTPUT;
		i810_set_dac_channels(state, 2);
		i810_set_dac_rate(state, 48000);
		if(prog_dmabuf(state, 0) != 0) {
			goto config_out_nodmabuf;
		}
		if(dmabuf->dmasize < 16384) {
			goto config_out;
		}
		dmabuf->count = dmabuf->dmasize;
		CIV_TO_LVI(card->iobase+dmabuf->write_channel->port, -1);
		flags = get_cpu_flags();
		cli();
		start_dac(state);
		offset = i810_get_dma_addr(state, 0);
		udelay(50000);
		new_offset = i810_get_dma_addr(state, 0);
		stop_dac(state);
		put_cpu_flags(flags);
		i = new_offset - offset;
#ifdef DEBUG_INTERRUPTS
		printk("i810_audio: %d bytes in 50 milliseconds\n", i);
#endif
		if(i == 0)
			goto config_out;
		i = i / 4 * 20;
		if (i > 48500 || i < 47500) {
			clocking = clocking * clocking / i;
			printk("i810_audio: setting clocking to %d\n", clocking);
		}
config_out:
		dealloc_dmabuf(state);
config_out_nodmabuf:
		state->card->free_pcm_channel(state->card,state->dmabuf.write_channel->num);
		kfree(state);
		card->states[0] = NULL;
	}
}

/* install the driver, we do not allocate hardware channel nor DMA buffer now, they are defered 
   until "ACCESS" time (in prog_dmabuf called by open/read/write/ioctl/mmap) */
   
//static int i810_probe(struct pci_dev *pci_dev, const struct pci_device_id *pci_id)
static int i810_probe(PCI_Info_s *pci_dev, int pci_id, int nDeviceID, struct pci_audio_info *pci_info)
{
	struct i810_card *card;
	
	u16 pci_command, new_command;
	unsigned char pci_latency;

	g_psBus = get_busmanager( PCI_BUS_NAME, PCI_BUS_VERSION );

	if ((card = kmalloc(sizeof(struct i810_card),
	    MEMF_KERNEL | MEMF_CLEAR)) == NULL) {
		printk(KERN_ERR "i810_audio: out of memory\n");
		return -ENOMEM;
	}
	memset(card, 0, sizeof(*card));

	card->initializing = 1;
	card->nDeviceID = nDeviceID;
	card->pci_info = pci_info;
	card->pci_dev = pci_dev;
	card->pci_id = pci_dev->nDeviceID;
	card->pci_id_internal = pci_info->driver_data;
	card->ac97base = pci_dev->u.h0.nBase0 & PCI_ADDRESS_IO_MASK;
	card->iobase = pci_dev->u.h0.nBase1 & PCI_ADDRESS_IO_MASK;
	card->irq = pci_dev->u.h0.nInterruptLine;
	card->next = devs;
	card->magic = I810_CARD_MAGIC;
	spinlock_init(&card->lock, "i810_lock");
	devs = card;
	i810_card_data = card;

	/* Get and check the bus-master and latency values. */
   	pci_command = g_psBus->read_pci_config( pci_dev->nBus, pci_dev->nDevice,
                                  	pci_dev->nFunction, PCI_COMMAND, 2 );
   new_command = pci_command | PCI_COMMAND_MASTER | PCI_COMMAND_IO;
   if (pci_command != new_command)
   {
   		printk(KERN_INFO "i810_audio: The PCI BIOS hasn't enabled the device "
               	    "at %d/%d/%d!    Updating PCI command %4.4x->%4.4x.\n",
                   	pci_dev->nBus, pci_dev->nDevice, pci_dev->nFunction,
                   	pci_command, new_command);
				g_psBus->write_pci_config( pci_dev->nBus, pci_dev->nDevice, pci_dev->nFunction,
					PCI_COMMAND, 2, new_command);
   }

	pci_latency = g_psBus->read_pci_config( pci_dev->nBus, pci_dev->nDevice,
                                        	pci_dev->nFunction, PCI_LATENCY, 1 );
	if (pci_latency < 32)
   {
		printk("i810_audio: PCI latency timer (CFLT) is unreasonably low at %d."
					   "  Setting to 64 clocks.\n", pci_latency);
				g_psBus->write_pci_config( pci_dev->nBus, pci_dev->nDevice, pci_dev->nFunction,
					PCI_LATENCY, 1, 64);
	
	}

	printk(KERN_INFO "i810: %s found at IO 0x%04lx and 0x%04lx, "
	       "IRQ %d\n",
	       card->pci_info->name, card->iobase, card->ac97base, (int) card->irq);

	if( claim_device( nDeviceID, pci_dev->nHandle, card->pci_info->name, DEVICE_AUDIO ) < 0 )
		goto out_mem;

	card->alloc_pcm_channel = i810_alloc_pcm_channel;
	card->alloc_rec_pcm_channel = i810_alloc_rec_pcm_channel;
	card->alloc_rec_mic_channel = i810_alloc_rec_mic_channel;
	card->free_pcm_channel = i810_free_pcm_channel;

	card->chanreal = kmalloc( sizeof( struct i810_channel)*NR_HW_CH + PAGE_SIZE, MEMF_KERNEL );
	card->channel = (struct i810_channel*)PAGE_ALIGN( (uint32)card->chanreal );
	card->chandma = (uint32)card->channel;
	
	#if 0
	if ((card->channel = pci_alloc_consistent(pci_dev,
	    sizeof(struct i810_channel)*NR_HW_CH, &card->chandma)) == NULL) {
		printk(KERN_ERR "i810: cannot allocate channel DMA memory\n");
		goto out_mem;
	}
	#endif

	printk("##1##");
	{ /* We may dispose of this altogether some time soon, so... */
		struct i810_channel *cp = card->channel;

		cp[0].offset = 0;
		cp[0].port = 0x00;
		cp[0].num = 0;
		cp[1].offset = 0;
		cp[1].port = 0x10;
		cp[1].num = 1;
		cp[2].offset = 0;
		cp[2].port = 0x20;
		cp[2].num = 2;
	}

	if (request_irq(card->irq, i810_interrupt, NULL,
		    SA_SHIRQ, card->pci_info->name, card) < 0) {
		printk(KERN_ERR "i810_audio: unable to allocate irq %d\n",(int) card->irq);
		goto out_pio;
	}

	printk("##2##");

	/* initialize AC97 codec and register /dev/mixer */
	if (i810_ac97_init(card) <= 0) {
		release_irq(card->irq, 0);
		goto out_iospace;
	}

	if(clocking == 0) {
		clocking = 48000;
		i810_configure_clocking();
	}

	/* register /dev/dsp */
	//if ((card->dev_audio = register_sound_dsp(&i810_audio_fops, -1)) < 0) {
#ifdef ENABLE_DEVNODE_052
	if( create_device_node( nDeviceID, pci_dev->nHandle,"sound/i810/dsp/0",
#else
	if( create_device_node( nDeviceID, pci_dev->nHandle,"sound/dsp",
#endif
			&i810_dsp_fops, i810_card_data ) < 0 ) {
		int i;
		printk(KERN_ERR "i810_audio: couldn't register DSP node!\n");
		release_irq(card->irq, card->irq_handle);
		for (i = 0; i < NR_AC97; i++)
		if (card->ac97_codec[i] != NULL) {
			kfree (card->ac97_codec[i]);
		}
		goto out_iospace;
	}

	printk("i810_audio: Card initialization successful.\n");

	card->initializing = 0;
	return 1;

out_iospace:
out_pio:	
	kfree( card->chanreal );
	//pci_free_consistent(pci_dev, sizeof(struct i810_channel)*NR_HW_CH,
	 //   card->channel, card->chandma);
out_mem:
	kfree(card);
	return 0;
}

static void i810_remove(struct i810_card *card)
{
	int i;

	/* free hardware resources */
	release_irq(card->irq, 0);

	/* unregister audio devices */
	for (i = 0; i < NR_AC97; i++)
		if (card->ac97_codec[i] != NULL) {
			kfree (card->ac97_codec[i]);
			card->ac97_codec[i] = NULL;
		}
	kfree(card);
	return;
}


/* ------------------------------------------------------------------------- */

status_t device_init(int nDeviceID)
{
	PCI_Info_s *pcidev;
	int foundone = 0;
 	int i;
 
	printk(KERN_INFO "i810 ICHx/440MX/SiS 7012/NVIDIA nForce/AMD 768/8111 audio v"
	       DRIVER_VERSION "\n");
	       
	ac97_init();
 
	for (i = 0; i < sizeof (pci_audio_devices) / sizeof( struct pci_audio_info ); i++) {
		pcidev = NULL;
		if ((pcidev = pci_find_device(pci_audio_devices[i].vendor,
						 pci_audio_devices[i].device,
						 pcidev)) != NULL) {
			foundone += i810_probe(pcidev, pci_audio_devices[i].device,
				nDeviceID, pci_audio_devices + i);
 		}
	}
 
	if (!foundone)
	{
		printk("i810_audio: No cards found\n");
		disable_device( nDeviceID );
		return -ENODEV;
	}
	return 0;
}

status_t device_uninit( int nDeviceID)
{
	struct i810_card *card = devs;	
	
	/* remove ALL cards */
	while (card != NULL)
	{
		i810_remove (card);
		card = card->next;
	}
	printk("i810_audio: devices successfully unregistered\n");
	return (0);
}

