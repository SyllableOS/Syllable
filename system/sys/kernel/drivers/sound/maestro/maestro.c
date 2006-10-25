/*****************************************************************************
 *
 *      ESS Maestro/Maestro-2/Maestro-2E driver for Linux 2.[23].x
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
 *	(c) Copyright 1999	 Alan Cox <alan.cox@linux.org>
 *
 *	Based heavily on SonicVibes.c:
 *      Copyright (C) 1998-1999  Thomas Sailer (sailer@ife.ee.ethz.ch)
 *
 *	Heavily modified by Zach Brown <zab@zabbo.net> based on lunch
 *	with ESS engineers.  Many thanks to Howard Kim for providing 
 *	contacts and hardware.  Honorable mention goes to Eric 
 *	Brombaugh for all sorts of things.  Best regards to the 
 *	proprietors of Hack Central for fine lodging.
 *
 *  Supported devices:
 *  /dev/dsp0-3    standard /dev/dsp device, (mostly) OSS compatible
 *  /dev/mixer  standard /dev/mixer device, (mostly) OSS compatible
 *
 *  Hardware Description
 *
 *	A working Maestro setup contains the Maestro chip wired to a 
 *	codec or 2.  In the Maestro we have the APUs, the ASSP, and the
 *	Wavecache.  The APUs can be though of as virtual audio routing
 *	channels.  They can take data from a number of sources and perform
 *	basic encodings of the data.  The wavecache is a storehouse for
 *	PCM data.  Typically it deals with PCI and interracts with the
 *	APUs.  The ASSP is a wacky DSP like device that ESS is loth
 *	to release docs on.  Thankfully it isn't required on the Maestro
 *	until you start doing insane things like FM emulation and surround
 *	encoding.  The codecs are almost always AC-97 compliant codecs, 
 *	but it appears that early Maestros may have had PT101 (an ESS
 *	part?) wired to them.  The only real difference in the Maestro
 *	families is external goop like docking capability, memory for
 *	the ASSP, and initialization differences.
 *
 *  Driver Operation
 *
 *	We only drive the APU/Wavecache as typical DACs and drive the
 *	mixers in the codecs.  There are 64 APUs.  We assign 6 to each
 *	/dev/dsp? device.  2 channels for output, and 4 channels for
 *	input.
 *
 *	Each APU can do a number of things, but we only really use
 *	3 basic functions.  For playback we use them to convert PCM
 *	data fetched over PCI by the wavecahche into analog data that
 *	is handed to the codec.  One APU for mono, and a pair for stereo.
 *	When in stereo, the combination of smarts in the APU and Wavecache
 *	decide which wavecache gets the left or right channel.
 *
 *	For record we still use the old overly mono system.  For each in
 *	coming channel the data comes in from the codec, through a 'input'
 *	APU, through another rate converter APU, and then into memory via
 *	the wavecache and PCI.  If its stereo, we mash it back into LRLR in
 *	software.  The pass between the 2 APUs is supposedly what requires us
 *	to have a 512 byte buffer sitting around in wavecache/memory.
 *
 *	The wavecache makes our life even more fun.  First off, it can
 *	only address the first 28 bits of PCI address space, making it
 *	useless on quite a few architectures.  Secondly, its insane.
 *	It claims to fetch from 4 regions of PCI space, each 4 meg in length.
 *	But that doesn't really work.  You can only use 1 region.  So all our
 *	allocations have to be in 4meg of each other.  Booo.  Hiss.
 *	So we have a module parameter, dsps_order, that is the order of
 *	the number of dsps to provide.  All their buffer space is allocated
 *	on open time.  The sonicvibes OSS routines we inherited really want
 *	power of 2 buffers, so we have all those next to each other, then
 *	512 byte regions for the recording wavecaches.  This ends up
 *	wasting quite a bit of memory.  The only fixes I can see would be 
 *	getting a kernel allocator that could work in zones, or figuring out
 *	just how to coerce the WP into doing what we want.
 *
 *	The indirection of the various registers means we have to spinlock
 *	nearly all register accesses.  We have the main register indirection
 *	like the wave cache, maestro registers, etc.  Then we have beasts
 *	like the APU interface that is indirect registers gotten at through
 *	the main maestro indirection.  Ouch.  We spinlock around the actual
 *	ports on a per card basis.  This means spinlock activity at each IO
 *	operation, but the only IO operation clusters are in non critical 
 *	paths and it makes the code far easier to follow.  Interrupts are
 *	blocked while holding the locks because the int handler has to
 *	get at some of them :(.  The mixer interface doesn't, however.
 *	We also have an OSS state lock that is thrown around in a few
 *	places.
 *
 *	This driver has brute force APM suspend support.  We catch suspend
 *	notifications and stop all work being done on the chip.  Any people
 *	that try between this shutdown and the real suspend operation will
 *	be put to sleep.  When we resume we restore our software state on
 *	the chip and wake up the people that were using it.  The code thats
 *	being used now is quite dirty and assumes we're on a uni-processor
 *	machine.  Much of it will need to be cleaned up for SMP ACPI or 
 *	similar.
 *
 *	We also pay attention to PCI power management now.  The driver
 *	will power down units of the chip that it knows aren't needed.
 *	The WaveProcessor and company are only powered on when people
 *	have /dev/dsp*s open.  On removal the driver will
 *	power down the maestro entirely.  There could still be
 *	trouble with BIOSen that magically change power states 
 *	themselves, but we'll see.  
 *	
 * History
 *  v0.15 - May 21 2001 - Marcus Meissner <mm@caldera.de>
 *      Ported to Linux 2.4 PCI API. Some clean ups, global devs list
 *      removed (now using pci device driver data).
 *      PM needs to be polished still. Bumped version.
 *  (still kind of v0.14) May 13 2001 - Ben Pfaff <pfaffben@msu.edu>
 *      Add support for 978 docking and basic hardware volume control
 *  (still kind of v0.14) Nov 23 - Alan Cox <alan@redhat.com>
 *	Add clocking= for people with seriously warped hardware
 *  (still v0.14) Nov 10 2000 - Bartlomiej Zolnierkiewicz <bkz@linux-ide.org>
 *	add __init to maestro_ac97_init() and maestro_install()
 *  (still based on v0.14) Mar 29 2000 - Zach Brown <zab@redhat.com>
 *	move to 2.3 power management interface, which
 *		required hacking some suspend/resume/check paths 
 *	make static compilation work
 *  v0.14 - Jan 28 2000 - Zach Brown <zab@redhat.com>
 *	add PCI power management through ACPI regs.
 *	we now shut down on machine reboot/halt
 *	leave scary PCI config items alone (isa stuff, mostly)
 *	enable 1921s, it seems only mine was broke.
 *	fix swapped left/right pcm dac.  har har.
 *	up bob freq, increase buffers, fix pointers at underflow
 *	silly compilation problems
 *  v0.13 - Nov 18 1999 - Zach Brown <zab@redhat.com>
 *	fix nec Versas?  man would that be cool.
 *  v0.12 - Nov 12 1999 - Zach Brown <zab@redhat.com>
 *	brown bag volume max fix..
 *  v0.11 - Nov 11 1999 - Zach Brown <zab@redhat.com>
 *	use proper stereo apu decoding, mmap/write should work.
 *	make volume sliders more useful, tweak rate calculation.
 *	fix lame 8bit format reporting bug.  duh. apm apu saving buglet also
 *	fix maestro 1 clock freq "bug", remove pt101 support
 *  v0.10 - Oct 28 1999 - Zach Brown <zab@redhat.com>
 *	aha, so, sometimes the WP writes a status word to offset 0
 *	  from one of the PCMBARs.  rearrange allocation accordingly..
 *	  cheers again to Eric for being a good hacker in investigating this.
 *	Jeroen Hoogervorst submits 7500 fix out of nowhere.  yay.  :)
 *  v0.09 - Oct 23 1999 - Zach Brown <zab@redhat.com>
 *	added APM support.
 *	re-order something such that some 2Es now work.  Magic!
 *	new codec reset routine.  made some codecs come to life.
 *	fix clear_advance, sync some control with ESS.
 *	now write to all base regs to be paranoid.
 *  v0.08 - Oct 20 1999 - Zach Brown <zab@redhat.com>
 *	Fix initial buflen bug.  I am so smart.  also smp compiling..
 *	I owe Eric yet another beer: fixed recmask, igain, 
 *	  muting, and adc sync consistency.  Go Team.
 *  v0.07 - Oct 4 1999 - Zach Brown <zab@redhat.com>
 *	tweak adc/dac, formating, and stuff to allow full duplex
 *	allocate dsps memory at open() so we can fit in the wavecache window
 *	fix wavecache braindamage.  again.  no more scribbling?
 *	fix ess 1921 codec bug on some laptops.
 *	fix dumb pci scanning bug
 *	started 2.3 cleanup, redid spinlocks, little cleanups
 *  v0.06 - Sep 20 1999 - Zach Brown <zab@redhat.com>
 *	fix wavecache thinkos.  limit to 1 /dev/dsp.
 *	eric is wearing his thinking toque this week.
 *		spotted apu mode bugs and gain ramping problem
 *	don't touch weird mixer regs, make recmask optional
 *	fixed igain inversion, defaults for mixers, clean up rec_start
 *	make mono recording work.
 *	report subsystem stuff, please send reports.
 *	littles: parallel out, amp now
 *  v0.05 - Sep 17 1999 - Zach Brown <zab@redhat.com>
 *	merged and fixed up Eric's initial recording code
 *	munged format handling to catch misuse, needs rewrite.
 *	revert ring bus init, fixup shared int, add pci busmaster setting
 *	fix mixer oss interface, fix mic mute and recmask
 *	mask off unsupported mixers, reset with all 1s, modularize defaults
 *	make sure bob is running while we need it
 *	got rid of device limit, initial minimal apm hooks
 *	pull out dead code/includes, only allow multimedia/audio maestros
 *  v0.04 - Sep 01 1999 - Zach Brown <zab@redhat.com>
 *	copied memory leak fix from sonicvibes driver
 *	different ac97 reset, play with 2.0 ac97, simplify ring bus setup
 *	bob freq code, region sanity, jitter sync fix; all from Eric 
 *
 * TODO
 *	fix bob frequency
 *	endianness
 *	do smart things with ac97 2.0 bits.
 *	dual codecs
 *	leave 54->61 open
 *
 *	it also would be fun to have a mode that would not use pci dma at all
 *	but would copy into the wavecache on board memory and use that 
 *	on architectures that don't like the maestro's pci dma ickiness.
 */

/* XXXKV
 *
 * I ported this driver in a hurry (~2days), so a lot of the original code
 * has remained as is; poor formatting, indenting, dodgy variable names and
 * all.  Reading this driver is enough to make your eyes bleed, but it works
 * (At least, noise comes out the speakers on my Maestro 2E).  There's plenty
 * wrong with this driver, not least of which include the horrible magic
 * numbers used everywhere, the inbuilt AC'97 code and consequently the horrible
 * stuttering when you use the mixer.  If anyone cares enough you could rip
 * out the inbuilt AC'97 and replace it with a proper set of routines and
 * ac97_codec.c  If you're really dedicated you could use the ALSA es1968
 * driver to replace the magic numbers with real macros.
 */

#include <atheos/kernel.h>
#include <atheos/kdebug.h>
#include <atheos/types.h>
#include <atheos/device.h>
#include <atheos/pci.h>
#include <atheos/irq.h>
#include <atheos/spinlock.h>
#include <atheos/semaphore.h>
#include <atheos/udelay.h>
#include <atheos/soundcard.h>
#define NO_DEBUG_STUBS 1
#include <atheos/linux_compat.h>

#include <posix/errno.h>
#include <posix/signal.h>

#include <maestro.h>

/* we try to setup 2^(dsps_order) /dev/dsp devices */
static const int dsps_order = 0;

/* clocking for broken hardware - a few laptops seem to use a 50Khz clock */
static const int clocking=48000;

#define MAX_DSP_ORDER	2
#define MAX_DSPS	(1<<MAX_DSP_ORDER)
#define NR_DSPS		(1<<dsps_order)
#define NR_IDRS		32

#define NR_APUS		64
#define NR_APU_REGS	16

struct maestro_state {
	unsigned int magic;
	/* FIXME: we probably want submixers in here, but only one record pair */
	uint8 apu[6];			/* l/r output, l/r intput converters, l/r input apus */
	uint8 apu_mode[6];		/* Running mode for this APU */
	uint8 apu_pan[6];		/* Panning setup for this APU */
	uint32 apu_base[6];		/* base address for this apu */
	struct maestro_card *card;	/* Card info */
	/* wave stuff */
	unsigned int rateadc, ratedac;
	unsigned char fmt, enable;

	int index;

	/* this locks around the oss state in the driver */
	SpinLock_s lock;
	/* only let 1 be opening at a time */
	sem_id open_sem;
	int open_mode;

	struct dmabuf {
		void *rawbuf;
		unsigned buforder;
		unsigned numfrag;
		unsigned fragshift;
		/* XXX zab - swptr only in here so that it can be referenced by
			clear_advance, as far as I can tell :( */
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
		unsigned ready:1;	/* our oss buffers are ready to go */
		unsigned endcleared:1;
		unsigned ossfragshift;
		int ossmaxfrags;
		unsigned subdivision;
		uint16 base;		/* Offset for ptr */
	} dma_dac, dma_adc;

	/* pointer to each dsp?s piece of the apu->src buffer page */
	void *mixbuf;

};

struct maestro_card {
	unsigned int magic;

	int card_type;

	/* as most of this is static,
		perhaps it should be a pointer to a global struct */
	struct mixer_goo {
		int modcnt;
		int supported_mixers;
		int stereo_mixers;
		int record_sources;
		/* the caller must guarantee arg sanity before calling these */
		void (*write_mixer)(struct maestro_card *card,int mixer, unsigned int left,unsigned int right);
		int (*recmask_io)(struct maestro_card *card,int rw,int mask);
		unsigned int mixer_state[SOUND_MIXER_NRDEVICES];
	} mix;

	struct maestro_state channels[MAX_DSPS];
	uint16 maestro_map[NR_IDRS];	/* Register map */
	/* we have to store this junk so that we can come back from a
		suspend */
	uint16 apu_map[NR_APUS][NR_APU_REGS];	/* contents of apu regs */

	/* this locks around the physical registers on the card */
	SpinLock_s lock;

	/* memory for this card.. wavecache limited :(*/
	void *dmapages;
	int dmaorder;

	/* hardware resources */
	PCI_Info_s *pcidev;
	area_id ioarea;
	uint32 iobase;
	int irq;

	int bob_freq;
	char dsps_open;

	int dock_mute_vol;
};

enum card_types_t {
	TYPE_MAESTRO,
	TYPE_MAESTRO2,
	TYPE_MAESTRO2E
};

static const char *card_names[]={
	[TYPE_MAESTRO] = "ESS Maestro",
	[TYPE_MAESTRO2] = "ESS Maestro 2",
	[TYPE_MAESTRO2E] = "ESS Maestro 2E"
};

static int clock_freq[]={
	[TYPE_MAESTRO] = (49152000L / 1024L),
	[TYPE_MAESTRO2] = (50000000L / 1024L),
	[TYPE_MAESTRO2E] = (50000000L / 1024L)
};

struct MaestroDevice
{
	int nDeviceID;
	int nCardType;
};

static int devices[] = {
	[TYPE_MAESTRO] = PCI_DEVICE_ID_ESS_ESS0100,
	[TYPE_MAESTRO2] = PCI_DEVICE_ID_ESS_ESS1968,
	[TYPE_MAESTRO2E] = PCI_DEVICE_ID_ESS_ESS1978
};

static const unsigned sample_size[] = { 1, 2, 2, 4 };
static const unsigned sample_shift[] = { 0, 1, 1, 2 };

static PCI_bus_s *g_psBus = NULL;

static void set_mixer( struct maestro_card *card, unsigned int mixer, unsigned int val );

static unsigned 
ld2(unsigned int x)
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

/* AC'97 */

/*
 *	ESS Maestro AC97 codec programming interface.
 */
	 
static void maestro_ac97_set( struct maestro_card *card, uint8 cmd, uint16 val )
{
	int io = card->iobase;
	int i;
	/*
	 *	Wait for the codec bus to be free 
	 */
	 
	for(i=0;i<10000;i++)
	{
		if(!(inb(io+ESS_AC97_INDEX)&1)) 
			break;
	}
	/*
	 *	Write the bus
	 */ 
	outw(val, io+ESS_AC97_DATA);
	udelay(100);
	outb(cmd, io+ESS_AC97_INDEX);
	udelay(100);
}

static uint16 maestro_ac97_get( struct maestro_card *card, uint8 cmd )
{
	int io = card->iobase;
	int sanity=10000;
	uint16 data;
	int i;
	
	/*
	 *	Wait for the codec bus to be free 
	 */
	 
	for(i=0;i<10000;i++)
	{
		if(!(inb(io+ESS_AC97_INDEX)&1))
			break;
	}

	outb(cmd|0x80, io+ESS_AC97_INDEX);
	udelay(100);

	while(inb(io+ESS_AC97_INDEX)&1)
	{
		sanity--;
		if(!sanity)
		{
			kerndbg( KERN_WARNING, "maestro: ac97 codec timeout reading 0x%x.\n",cmd);
			return 0;
		}
	}
	data=inw(io+ESS_AC97_DATA);
	udelay(100);
	return data;
}

/* OSS interface to the ac97s.. */

#define AC97_STEREO_MASK (SOUND_MASK_VOLUME|\
	SOUND_MASK_PCM|SOUND_MASK_LINE|SOUND_MASK_CD|\
	SOUND_MASK_VIDEO|SOUND_MASK_LINE1|SOUND_MASK_IGAIN)

#define AC97_SUPPORTED_MASK (AC97_STEREO_MASK | \
	SOUND_MASK_BASS|SOUND_MASK_TREBLE|SOUND_MASK_MIC|\
	SOUND_MASK_SPEAKER)

#define AC97_RECORD_MASK (SOUND_MASK_MIC|\
	SOUND_MASK_CD| SOUND_MASK_VIDEO| SOUND_MASK_LINE1| SOUND_MASK_LINE|\
	SOUND_MASK_PHONEIN)

#define supported_mixer(CARD,FOO) ( CARD->mix.supported_mixers & (1<<FOO) )

/* this table has default mixer values for all OSS mixers.
	be sure to fill it in if you add oss mixers
	to anyone's supported mixer defines */

static unsigned int mixer_defaults[SOUND_MIXER_NRDEVICES] = {
	[SOUND_MIXER_VOLUME] =          0x3232,
	[SOUND_MIXER_BASS] =            0x3232,
	[SOUND_MIXER_TREBLE] =          0x3232,
	[SOUND_MIXER_SPEAKER] =         0x3232,
	[SOUND_MIXER_MIC] =     0x8000, /* annoying */
	[SOUND_MIXER_LINE] =    0x3232,
	[SOUND_MIXER_CD] =      0x3232,
	[SOUND_MIXER_VIDEO] =   0x3232,
	[SOUND_MIXER_LINE1] =   0x3232,
	[SOUND_MIXER_PCM] =             0x3232,
	[SOUND_MIXER_IGAIN] =           0x3232
};

static struct ac97_mixer_hw {
	unsigned char offset;
	int scale;
} ac97_hw[SOUND_MIXER_NRDEVICES]= {
	[SOUND_MIXER_VOLUME]	=	{0x02,63},
	[SOUND_MIXER_BASS]	=	{0x08,15},
	[SOUND_MIXER_TREBLE]	=	{0x08,15},
	[SOUND_MIXER_SPEAKER]	=	{0x0a,15},
	[SOUND_MIXER_MIC]	=	{0x0e,31},
	[SOUND_MIXER_LINE]	=	{0x10,31},
	[SOUND_MIXER_CD]	=	{0x12,31},
	[SOUND_MIXER_VIDEO]	=	{0x14,31},
	[SOUND_MIXER_LINE1]	=	{0x16,31},
	[SOUND_MIXER_PCM]	=	{0x18,31},
	[SOUND_MIXER_IGAIN]	=	{0x1c,15}
};

/* write the OSS encoded volume to the given OSS encoded mixer,
	again caller's job to make sure all is well in arg land,
	call with spinlock held */
	
/* linear scale -> log */
static unsigned char lin2log[101] = 
{
0, 0 , 15 , 23 , 30 , 34 , 38 , 42 , 45 , 47 ,
50 , 52 , 53 , 55 , 57 , 58 , 60 , 61 , 62 ,
63 , 65 , 66 , 67 , 68 , 69 , 69 , 70 , 71 ,
72 , 73 , 73 , 74 , 75 , 75 , 76 , 77 , 77 ,
78 , 78 , 79 , 80 , 80 , 81 , 81 , 82 , 82 ,
83 , 83 , 84 , 84 , 84 , 85 , 85 , 86 , 86 ,
87 , 87 , 87 , 88 , 88 , 88 , 89 , 89 , 89 ,
90 , 90 , 90 , 91 , 91 , 91 , 92 , 92 , 92 ,
93 , 93 , 93 , 94 , 94 , 94 , 94 , 95 , 95 ,
95 , 95 , 96 , 96 , 96 , 96 , 97 , 97 , 97 ,
97 , 98 , 98 , 98 , 98 , 99 , 99 , 99 , 99 , 99 
};

static void ac97_write_mixer( struct maestro_card *card, int mixer, unsigned int left, unsigned int right )
{
	uint16 val=0;
	struct ac97_mixer_hw *mh = &ac97_hw[mixer];

	kerndbg( KERN_DEBUG, "wrote mixer %d (0x%x) %d,%d",mixer,mh->offset,left,right);

	if(AC97_STEREO_MASK & (1<<mixer)) {
		/* stereo mixers, mute them if we can */

		if (mixer == SOUND_MIXER_IGAIN) {
			/* igain's slider is reversed.. */
			right = (right * mh->scale) / 100;
			left = (left * mh->scale) / 100;
			if ((left == 0) && (right == 0))
				val |= 0x8000;
		} else if (mixer == SOUND_MIXER_PCM || mixer == SOUND_MIXER_CD) {
			/* log conversion seems bad for them */
			if ((left == 0) && (right == 0))
				val = 0x8000;
			right = ((100 - right) * mh->scale) / 100;
			left = ((100 - left) * mh->scale) / 100;
		} else {
			/* log conversion for the stereo controls */
			if((left == 0) && (right == 0))
				val = 0x8000;
			right = ((100 - lin2log[right]) * mh->scale) / 100;
			left = ((100 - lin2log[left]) * mh->scale) / 100;
		}

		val |= (left << 8) | right;

	} else if (mixer == SOUND_MIXER_SPEAKER) {
		val = (((100 - left) * mh->scale) / 100) << 1;
	} else if (mixer == SOUND_MIXER_MIC) {
		val = maestro_ac97_get(card, mh->offset) & ~0x801f;
		val |= (((100 - left) * mh->scale) / 100);
	/*  the low bit is optional in the tone sliders and masking
		it lets is avoid the 0xf 'bypass'.. */
	} else if (mixer == SOUND_MIXER_BASS) {
		val = maestro_ac97_get(card , mh->offset) & ~0x0f00;
		val |= ((((100 - left) * mh->scale) / 100) << 8) & 0x0e00;
	} else if (mixer == SOUND_MIXER_TREBLE)  {
		val = maestro_ac97_get(card , mh->offset) & ~0x000f;
		val |= (((100 - left) * mh->scale) / 100) & 0x000e;
	}

	maestro_ac97_set(card , mh->offset, val);
	
	kerndbg( KERN_DEBUG, " -> %x\n",val);
}

/* the following tables allow us to go from 
	OSS <-> ac97 quickly. */

enum ac97_recsettings {
	AC97_REC_MIC=0,
	AC97_REC_CD,
	AC97_REC_VIDEO,
	AC97_REC_AUX,
	AC97_REC_LINE,
	AC97_REC_STEREO, /* combination of all enabled outputs..  */
	AC97_REC_MONO,        /*.. or the mono equivalent */
	AC97_REC_PHONE        
};

static unsigned int ac97_oss_mask[] = {
	[AC97_REC_MIC] = SOUND_MASK_MIC, 
	[AC97_REC_CD] = SOUND_MASK_CD, 
	[AC97_REC_VIDEO] = SOUND_MASK_VIDEO, 
	[AC97_REC_AUX] = SOUND_MASK_LINE1, 
	[AC97_REC_LINE] = SOUND_MASK_LINE, 
	[AC97_REC_PHONE] = SOUND_MASK_PHONEIN
};

/* indexed by bit position */
static unsigned int ac97_oss_rm[] = {
	[SOUND_MIXER_MIC] = AC97_REC_MIC,
	[SOUND_MIXER_CD] = AC97_REC_CD,
	[SOUND_MIXER_VIDEO] = AC97_REC_VIDEO,
	[SOUND_MIXER_LINE1] = AC97_REC_AUX,
	[SOUND_MIXER_LINE] = AC97_REC_LINE,
	[SOUND_MIXER_PHONEIN] = AC97_REC_PHONE
};

/* read or write the recmask 
	the ac97 can really have left and right recording
	inputs independently set, but OSS doesn't seem to 
	want us to express that to the user. 
	the caller guarantees that we have a supported bit set,
	and they must be holding the card's spinlock */
static int ac97_recmask_io( struct maestro_card *card, int read, int mask ) 
{
	unsigned int val = ac97_oss_mask[ maestro_ac97_get(card, 0x1a) & 0x7 ];

	if (read) return val;

	/* oss can have many inputs, maestro can't.  try
		to pick the 'new' one */

	if (mask != val) mask &= ~val;

	val = ffs(mask) - 1; 
	val = ac97_oss_rm[val];
	val |= val << 8;  /* set both channels */

	kerndbg( KERN_DEBUG, "maestro: setting ac97 recmask to 0x%x\n",val);

	maestro_ac97_set(card,0x1a,val);

	return 0;
};

/*
 *	The Maestro can be wired to a standard AC97 compliant codec
 *	(see www.intel.com for the pdf's on this), or to a PT101 codec
 *	which appears to be the ES1918 (data sheet on the esstech.com.tw site)
 *
 *	The PT101 setup is untested.
 */
 
static uint16 maestro_ac97_init( struct maestro_card *card )
{
	uint16 vend1, vend2, caps;

	card->mix.supported_mixers = AC97_SUPPORTED_MASK;
	card->mix.stereo_mixers = AC97_STEREO_MASK;
	card->mix.record_sources = AC97_RECORD_MASK;
	card->mix.write_mixer = ac97_write_mixer;
	card->mix.recmask_io = ac97_recmask_io;

	vend1 = maestro_ac97_get(card, 0x7c);
	vend2 = maestro_ac97_get(card, 0x7e);

	caps = maestro_ac97_get(card, 0x00);

	kerndbg( KERN_INFO, "AC97 Codec detected: v: 0x%2x%2x caps: 0x%x pwr: 0x%x\n",
		vend1,vend2,caps,maestro_ac97_get(card,0x26) & 0xf);

	if (! (caps & 0x4) ) {
		/* no bass/treble nobs */
		card->mix.supported_mixers &= ~(SOUND_MASK_BASS|SOUND_MASK_TREBLE);
	}

	/* XXX endianness, dork head. */
	/* vendor specifc bits.. */
	switch ((long)(vend1 << 16) | vend2) {
	case 0x545200ff:	/* TriTech */
		/* no idea what this does */
		maestro_ac97_set(card,0x2a,0x0001);
		maestro_ac97_set(card,0x2c,0x0000);
		maestro_ac97_set(card,0x2c,0xffff);
		break;
#if 0	/* i thought the problems I was seeing were with
	the 1921, but apparently they were with the pci board
	it was on, so this code is commented out.
	 lets see if this holds true. */
	case 0x83847609:	/* ESS 1921 */
		/* writing to 0xe (mic) or 0x1a (recmask) seems
			to hang this codec */
		card->mix.supported_mixers &= ~(SOUND_MASK_MIC);
		card->mix.record_sources = 0;
		card->mix.recmask_io = NULL;
#if 0	/* don't ask.  I have yet to see what these actually do. */
		maestro_ac97_set(card,0x76,0xABBA); /* o/~ Take a chance on me o/~ */
		udelay(20);
		maestro_ac97_set(card,0x78,0x3002);
		udelay(20);
		maestro_ac97_set(card,0x78,0x3802);
		udelay(20);
#endif
		break;
#endif
	default: break;
	}

	maestro_ac97_set(card, 0x1E, 0x0404);
	/* null misc stuff */
	maestro_ac97_set(card, 0x20, 0x0000);

	return 0;
}

/* this is very magic, and very slow.. */
static void maestro_ac97_reset( int ioaddr, PCI_Info_s *pcidev )
{
	uint16 save_68;
	uint16 w;
	uint32 vend;

	outw( inw(ioaddr + 0x38) & 0xfffc, ioaddr + 0x38);
	outw( inw(ioaddr + 0x3a) & 0xfffc, ioaddr + 0x3a);
	outw( inw(ioaddr + 0x3c) & 0xfffc, ioaddr + 0x3c);

	/* reset the first codec */
	outw(0x0000,  ioaddr+0x36);
	save_68 = inw(ioaddr+0x68);
	w = g_psBus->read_pci_config( pcidev->nBus, pcidev->nDevice, pcidev->nFunction, 0x58, 2 );
	vend = g_psBus->read_pci_config( pcidev->nBus, pcidev->nDevice, pcidev->nFunction, PCI_SUBSYSTEM_VENDOR_ID, 4 );
	if( w & 0x1)
		save_68 |= 0x10;
	outw(0xfffe, ioaddr + 0x64);	/* tickly gpio 0.. */
	outw(0x0001, ioaddr + 0x68);
	outw(0x0000, ioaddr + 0x60);
	udelay(20);
	outw(0x0001, ioaddr + 0x60);
	udelay(2000);

	outw(save_68 | 0x1, ioaddr + 0x68);	/* now restore .. */
	outw( (inw(ioaddr + 0x38) & 0xfffc)|0x1, ioaddr + 0x38);
	outw( (inw(ioaddr + 0x3a) & 0xfffc)|0x1, ioaddr + 0x3a);
	outw( (inw(ioaddr + 0x3c) & 0xfffc)|0x1, ioaddr + 0x3c);

	/* now the second codec */
	outw(0x0000,  ioaddr+0x36);
	outw(0xfff7, ioaddr + 0x64);
	save_68 = inw(ioaddr+0x68);
	outw(0x0009, ioaddr + 0x68);
	outw(0x0001, ioaddr + 0x60);
	udelay(20);
	outw(0x0009, ioaddr + 0x60);
	udelay(50000);	/* .. ouch.. */
	outw( inw(ioaddr + 0x38) & 0xfffc, ioaddr + 0x38);
	outw( inw(ioaddr + 0x3a) & 0xfffc, ioaddr + 0x3a);
	outw( inw(ioaddr + 0x3c) & 0xfffc, ioaddr + 0x3c);

#if 0 /* the loop here needs to be much better if we want it.. */
	kerndbg( KERN_DEBUG, "trying software reset\n");
	/* try and do a software reset */
	outb(0x80|0x7c, ioaddr + 0x30);
	for (w=0; ; w++) {
		if ((inw(ioaddr+ 0x30) & 1) == 0) {
			if(inb(ioaddr + 0x32) !=0) break;

			outb(0x80|0x7d, ioaddr + 0x30);
			if (((inw(ioaddr+ 0x30) & 1) == 0) && (inb(ioaddr + 0x32) !=0)) break;
			outb(0x80|0x7f, ioaddr + 0x30);
			if (((inw(ioaddr+ 0x30) & 1) == 0) && (inb(ioaddr + 0x32) !=0)) break;
		}

		if( w > 10000) {
			outb( inb(ioaddr + 0x37) | 0x08, ioaddr + 0x37);  /* do a software reset */
			//mdelay(500); /* oh my.. */
			udelay(50000); /* oh my.. */
			outb( inb(ioaddr + 0x37) & ~0x08, ioaddr + 0x37);  
			udelay(1);
			outw( 0x80, ioaddr+0x30);
			for(w = 0 ; w < 10000; w++) {
				if((inw(ioaddr + 0x30) & 1) ==0) break;
			}
		}
	}
#endif
	if ( vend == NEC_VERSA_SUBID1 || vend == NEC_VERSA_SUBID2) {
		/* turn on external amp? */
		outw(0xf9ff, ioaddr + 0x64);
		outw(inw(ioaddr+0x68) | 0x600, ioaddr + 0x68);
		outw(0x0209, ioaddr + 0x60);
	}

	/* Turn on the 978 docking chip.
	   First frob the "master output enable" bit,
	   then set most of the playback volume control registers to max. */
	outb(inb(ioaddr+0xc0)|(1<<5), ioaddr+0xc0);
	outb(0xff, ioaddr+0xc3);
	outb(0xff, ioaddr+0xc4);
	outb(0xff, ioaddr+0xc6);
	outb(0xff, ioaddr+0xc8);
	outb(0x3f, ioaddr+0xcf);
	outb(0x3f, ioaddr+0xd0);
}

/*
 *	Indirect register access. Not all registers are readable so we
 *	need to keep register state ourselves
 */
 
#define WRITEABLE_MAP	0xEFFFFF
#define READABLE_MAP	0x64003F

/*
 *	The Maestro engineers were a little indirection happy. These indirected
 *	registers themselves include indirect registers at another layer
 */

static void __maestro_write( struct maestro_card *card, uint16 reg, uint16 data )
{
	long ioaddr = card->iobase;

	outw(reg, ioaddr+0x02);
	outw(data, ioaddr+0x00);
	if( reg >= NR_IDRS) printk("maestro: IDR %d out of bounds!\n",reg);
	else card->maestro_map[reg]=data;

}
 
static void maestro_write( struct maestro_state *s, uint16 reg, uint16 data )
{
	unsigned long flags;

	spinlock_cli( &s->card->lock, flags );
	__maestro_write(s->card,reg,data);
	spinunlock_restore(&s->card->lock,flags);
}

static uint16 __maestro_read( struct maestro_card *card, uint16 reg )
{
	long ioaddr = card->iobase;

	outw(reg, ioaddr+0x02);
	return card->maestro_map[reg]=inw(ioaddr+0x00);
}

static uint16 maestro_read( struct maestro_state *s, uint16 reg )
{
	if( READABLE_MAP & (1<<reg) )
	{
		unsigned long flags;

		spinlock_cli( &s->card->lock, flags );
		__maestro_read(s->card,reg);
		spinunlock_restore(&s->card->lock,flags);
	}
	return s->card->maestro_map[reg];
}

/*
 *	These routines handle accessing the second level indirections to the
 *	wave ram.
 */

/*
 *	The register names are the ones ESS uses (see 104T31.ZIP)
 */
 
#define IDR0_DATA_PORT		0x00
#define IDR1_CRAM_POINTER	0x01
#define IDR2_CRAM_DATA		0x02
#define IDR3_WAVE_DATA		0x03
#define IDR4_WAVE_PTR_LOW	0x04
#define IDR5_WAVE_PTR_HI	0x05
#define IDR6_TIMER_CTRL		0x06
#define IDR7_WAVE_ROMRAM	0x07

static void apu_index_set( struct maestro_card *card, uint16 index )
{
	int i;
	__maestro_write(card, IDR1_CRAM_POINTER, index);
	for(i=0;i<1000;i++)
		if(__maestro_read(card, IDR1_CRAM_POINTER)==index)
			return;
	kerndbg( KERN_WARNING, "maestro: APU register select failed.\n");
}

static void apu_data_set( struct maestro_card *card, uint16 data )
{
	int i;
	for(i=0;i<1000;i++)
	{
		if(__maestro_read(card, IDR0_DATA_PORT)==data)
			return;
		__maestro_write(card, IDR0_DATA_PORT, data);
	}
}

/*
 *	This is the public interface for APU manipulation. It handles the
 *	interlock to avoid two APU writes in parallel etc. Don't diddle
 *	directly with the stuff above.
 */

static void apu_set_register( struct maestro_state *s, uint16 channel, uint8 reg, uint16 data )
{
	unsigned long flags;

	if(channel&ESS_CHAN_HARD)
		channel&=~ESS_CHAN_HARD;
	else
	{
		if(channel>5)
			printk("BAD CHANNEL %d.\n",channel);
		else
			channel = s->apu[channel];
		/* store based on real hardware apu/reg */
		s->card->apu_map[channel][reg]=data;
	}
	reg|=(channel<<4);
	
	/* hooray for double indirection!! */
	spinlock_cli(&s->card->lock,flags);

	apu_index_set(s->card, reg);
	apu_data_set(s->card, data);

	spinunlock_restore(&s->card->lock,flags);
}

static uint16 apu_get_register( struct maestro_state *s, uint16 channel, uint8 reg )
{
	unsigned long flags;
	uint16 v;

	if(channel&ESS_CHAN_HARD)
		channel&=~ESS_CHAN_HARD;
	else
		channel = s->apu[channel];

	reg|=(channel<<4);
	
	spinlock_cli(&s->card->lock,flags);

	apu_index_set(s->card, reg);
	v=__maestro_read(s->card, IDR0_DATA_PORT);

	spinunlock_restore(&s->card->lock,flags);
	return v;
}

/*
 *	The wavecache buffers between the APUs and
 *	pci bus mastering
 */
 
static void wave_set_register( struct maestro_state *s, uint16 reg, uint16 value )
{
	long ioaddr = s->card->iobase;
	unsigned long flags;
	
	spinlock_cli(&s->card->lock,flags);

	outw(reg, ioaddr+0x10);
	outw(value, ioaddr+0x12);

	spinunlock_restore(&s->card->lock,flags);
}

static uint16 wave_get_register( struct maestro_state *s, uint16 reg )
{
	long ioaddr = s->card->iobase;
	unsigned long flags;
	u16 value;
	
	spinlock_cli(&s->card->lock,flags);
	outw(reg, ioaddr+0x10);
	value=inw(ioaddr+0x12);
	spinunlock_restore(&s->card->lock,flags);
	
	return value;
}

static void sound_reset(int ioaddr)
{
	outw(0x2000, 0x18+ioaddr);
	udelay(1);
	outw(0x0000, 0x18+ioaddr);
	udelay(1);
}

/* sets the play formats of these apus, should be passed the already shifted format */
static void set_apu_fmt( struct maestro_state *s, int apu, int mode )
{
	int apu_fmt = 0x10;

	if(!(mode&ESS_FMT_16BIT)) apu_fmt+=0x20; 
	if((mode&ESS_FMT_STEREO)) apu_fmt+=0x10; 
	s->apu_mode[apu]   = apu_fmt;
	s->apu_mode[apu+1] = apu_fmt;
}

/* this only fixes the output apu mode to be later set by start_dac and
	company.  output apu modes are set in ess_rec_setup */
static void set_fmt( struct maestro_state *s, unsigned char mask, unsigned char data )
{
	s->fmt = (s->fmt & mask) | data;
	set_apu_fmt(s, 0, (s->fmt >> ESS_DAC_SHIFT) & ESS_FMT_MASK);
}

/* this is off by a little bit.. */
static uint32 compute_rate( struct maestro_state *s, uint32 freq )
{
	uint32 clock = clock_freq[s->card->card_type];     

	freq = (freq * clocking)/48000;

	if (freq == 48000) 
		return 0x10000;

	return ((freq / clock) <<16 )+  
		(((freq % clock) << 16) / clock);
}

static void set_dac_rate( struct maestro_state *s, unsigned int rate )
{
	uint32 freq;
	int fmt = (s->fmt >> ESS_DAC_SHIFT) & ESS_FMT_MASK;

	if (rate > 48000)
		rate = 48000;
	if (rate < 4000)
		rate = 4000;

	s->ratedac = rate;

	if(! (fmt & ESS_FMT_16BIT) && !(fmt & ESS_FMT_STEREO))
		rate >>= 1;

	kerndbg( KERN_DEBUG_LOW, "computing dac rate %d with mode %d\n",rate,s->fmt);

	freq = compute_rate(s, rate);
	
	/* Load the frequency, turn on 6dB */
	apu_set_register(s, 0, 2,(apu_get_register(s, 0, 2)&0x00FF)|
		( ((freq&0xFF)<<8)|0x10 ));
	apu_set_register(s, 0, 3, freq>>8);
	apu_set_register(s, 1, 2,(apu_get_register(s, 1, 2)&0x00FF)|
		( ((freq&0xFF)<<8)|0x10 ));
	apu_set_register(s, 1, 3, freq>>8);
}

static void set_adc_rate( struct maestro_state *s, unsigned rate )
{
	uint32 freq;

	/* Sample Rate conversion APUs don't like 0x10000 for their rate */
	if (rate > 47999)
		rate = 47999;
	if (rate < 4000)
		rate = 4000;

	s->rateadc = rate;

	freq = compute_rate(s, rate);
	
	/* Load the frequency, turn on 6dB */
	apu_set_register(s, 2, 2,(apu_get_register(s, 2, 2)&0x00FF)|
		( ((freq&0xFF)<<8)|0x10 ));
	apu_set_register(s, 2, 3, freq>>8);
	apu_set_register(s, 3, 2,(apu_get_register(s, 3, 2)&0x00FF)|
		( ((freq&0xFF)<<8)|0x10 ));
	apu_set_register(s, 3, 3, freq>>8);

	/* fix mixer rate at 48khz.  and its _must_ be 0x10000. */
	freq = 0x10000;

	apu_set_register(s, 4, 2,(apu_get_register(s, 4, 2)&0x00FF)|
		( ((freq&0xFF)<<8)|0x10 ));
	apu_set_register(s, 4, 3, freq>>8);
	apu_set_register(s, 5, 2,(apu_get_register(s, 5, 2)&0x00FF)|
		( ((freq&0xFF)<<8)|0x10 ));
	apu_set_register(s, 5, 3, freq>>8);
}

/* Stop our host of recording apus */
static inline void stop_adc( struct maestro_state *s )
{
	/* XXX lets hope we don't have to lock around this */
	if (! (s->enable & ADC_RUNNING)) return;

	s->enable &= ~ADC_RUNNING;
	apu_set_register(s, 2, 0, apu_get_register(s, 2, 0)&0xFF0F);
	apu_set_register(s, 3, 0, apu_get_register(s, 3, 0)&0xFF0F);
	apu_set_register(s, 4, 0, apu_get_register(s, 2, 0)&0xFF0F);
	apu_set_register(s, 5, 0, apu_get_register(s, 3, 0)&0xFF0F);
}	

/* stop output apus */
static void stop_dac( struct maestro_state *s )
{
	/* XXX have to lock around this? */
	if (! (s->enable & DAC_RUNNING)) return;

	s->enable &= ~DAC_RUNNING;
	apu_set_register(s, 0, 0, apu_get_register(s, 0, 0)&0xFF0F);
	apu_set_register(s, 1, 0, apu_get_register(s, 1, 0)&0xFF0F);
}

static void start_dac( struct maestro_state *s )
{
	/* XXX locks? */
	if ( ( s->dma_dac.mapped || s->dma_dac.count > 0 ) && 
		s->dma_dac.ready &&
		(! (s->enable & DAC_RUNNING)) ) {

		s->enable |= DAC_RUNNING;

		apu_set_register(s, 0, 0, 
			(apu_get_register(s, 0, 0)&0xFF0F)|s->apu_mode[0]);

		if((s->fmt >> ESS_DAC_SHIFT)  & ESS_FMT_STEREO) 
			apu_set_register(s, 1, 0, 
				(apu_get_register(s, 1, 0)&0xFF0F)|s->apu_mode[1]);
	}
}	

static void start_adc( struct maestro_state *s )
{
	/* XXX locks? */
	if ((s->dma_adc.mapped || s->dma_adc.count < (signed)(s->dma_adc.dmasize - 2*s->dma_adc.fragsize)) 
	    && s->dma_adc.ready && (! (s->enable & ADC_RUNNING)) ) {

		s->enable |= ADC_RUNNING;
		apu_set_register(s, 2, 0, 
			(apu_get_register(s, 2, 0)&0xFF0F)|s->apu_mode[2]);
		apu_set_register(s, 4, 0, 
			(apu_get_register(s, 4, 0)&0xFF0F)|s->apu_mode[4]);

		if( s->fmt & (ESS_FMT_STEREO << ESS_ADC_SHIFT)) {
			apu_set_register(s, 3, 0, 
				(apu_get_register(s, 3, 0)&0xFF0F)|s->apu_mode[3]);
			apu_set_register(s, 5, 0, 
				(apu_get_register(s, 5, 0)&0xFF0F)|s->apu_mode[5]);
		}
			
	}
}

/*
 *	Native play back driver 
 */

/* the mode passed should be already shifted and masked */
static void maestro_play_setup( struct maestro_state *state, int mode, uint32 rate, void *buffer, int size )
{
	uint32 pa;
	uint32 tmpval;
	int high_apu = 0;
	int channel;

	kerndbg( KERN_DEBUG, "mode=%d rate=%d buf=%p len=%d.\n", mode, rate, buffer, size);

	/* all maestro sizes are in 16bit words */
	size >>=1;

	if(mode&ESS_FMT_STEREO) {
		high_apu++;
		/* only 16/stereo gets size divided */
		if(mode&ESS_FMT_16BIT)
			size>>=1;
	}

	for(channel=0; channel <= high_apu; channel++)
	{
		pa = virt_to_bus(buffer);

		/* set the wavecache control reg */
		tmpval = (pa - 0x10) & 0xFFF8;
		if(!(mode & ESS_FMT_16BIT)) tmpval |= 4;
		if(mode & ESS_FMT_STEREO) tmpval |= 2;
		state->apu_base[channel]=tmpval;
		wave_set_register(state, state->apu[channel]<<3, tmpval);
		
		pa -= virt_to_bus(state->card->dmapages);
		pa>>=1; /* words */
		
		/* base offset of dma calcs when reading the pointer
			on the left one */
		if(!channel) state->dma_dac.base = pa&0xFFFF;
		
		pa|=0x00400000;			/* System RAM */

		/* XXX the 16bit here might not be needed.. */
		if((mode & ESS_FMT_STEREO) && (mode & ESS_FMT_16BIT)) {
			if(channel) 
				pa|=0x00800000;			/* Stereo */
			pa>>=1;
		}

/* XXX think about endianess when writing these registers */
		kerndbg( KERN_DEBUG, "maestro: maestro_play_setup: APU[%d] pa = 0x%x\n", state->apu[channel], pa);
		/* start of sample */
		apu_set_register(state, channel, 4, ((pa>>16)&0xFF)<<8);
		apu_set_register(state, channel, 5, pa&0xFFFF);
		/* sample end */
		apu_set_register(state, channel, 6, (pa+size)&0xFFFF);
		/* setting loop len == sample len */
		apu_set_register(state, channel, 7, size);
		
		/* clear effects/env.. */
		apu_set_register(state, channel, 8, 0x0000);
		/* set amp now to 0xd0 (?), low byte is 'amplitude dest'? */
		apu_set_register(state, channel, 9, 0xD000);

		/* clear routing stuff */
		apu_set_register(state, channel, 11, 0x0000);
		/* dma on, no envelopes, filter to all 1s) */
		apu_set_register(state, channel, 0, 0x400F);
		
		if(mode&ESS_FMT_16BIT)
			state->apu_mode[channel]=0x10;
		else
			state->apu_mode[channel]=0x30;

		if(mode&ESS_FMT_STEREO) {
			/* set panning: left or right */
			apu_set_register(state, channel, 10, 0x8F00 | (channel ? 0 : 0x10));
			state->apu_mode[channel] += 0x10;
		} else
			apu_set_register(state, channel, 10, 0x8F08);
	}

	/* clear WP interrupts */
	outw(1, state->card->iobase+0x04);
	/* enable WP ints */
	outw(inw(state->card->iobase+0x18)|4, state->card->iobase+0x18);

	/* go team! */
	set_dac_rate(state,rate);
	start_dac(state);
}

/* again, passed mode is alrady shifted/masked */
static void maestro_rec_setup( struct maestro_state *state, int mode, uint32 rate, void *buffer, int size )
{
	/* XXXKV: Todo */
	kerndbg( KERN_WARNING, "maestro_rec_setup() called!\n" );
}

/* Playback pointer */
static inline unsigned get_dmaa( struct maestro_state *s )
{
	int offset;

	offset = apu_get_register(s,0,5);
	kerndbg( KERN_DEBUG_LOW, "dmaa: offset: %d, base: %d\n",offset,s->dma_dac.base);
	offset-=s->dma_dac.base;
	return (offset&0xFFFE)<<1; /* hardware is in words */
}

/* Record pointer */
static inline unsigned get_dmac( struct maestro_state *s )
{
	int offset;

	offset = apu_get_register(s,2,5);
	kerndbg( KERN_DEBUG_LOW, "dmac: offset: %d, base: %d\n",offset,s->dma_adc.base);
	/* The offset is an address not a position relative to base */
	offset-=s->dma_adc.base;
	return (offset&0xFFFE)<<1; /* hardware is in words */
}

/*
 *	Meet Bob, the timer...
 */

static void stop_bob( struct maestro_state *s )
{
	/* Mask IDR 11,17 */
	maestro_write(s,  0x11, maestro_read(s, 0x11)&~1);
	maestro_write(s,  0x17, maestro_read(s, 0x17)&~1);
}

/* eventually we could be clever and limit bob ints
	to the frequency at which our smallest duration
	chunks may expire */
#define ESS_SYSCLK	50000000
static void start_bob( struct maestro_state *s )
{
	int prescale;
	int divide;
	
	/* XXX make freq selector much smarter, see calc_bob_rate */
	int freq = 200; 
	
	/* compute ideal interrupt frequency for buffer size & play rate */
	/* first, find best prescaler value to match freq */
	for(prescale=5;prescale<12;prescale++)
		if(freq > (ESS_SYSCLK>>(prescale+9)))
			break;
			
	/* next, back off prescaler whilst getting divider into optimum range */
	divide=1;
	while((prescale > 5) && (divide<32))
	{
		prescale--;
		divide <<=1;
	}
	divide>>=1;
	
	/* now fine-tune the divider for best match */
	for(;divide<31;divide++)
		if(freq >= ((ESS_SYSCLK>>(prescale+9))/(divide+1)))
			break;
	
	/* divide = 0 is illegal, but don't let prescale = 4! */
	if(divide == 0)
	{
		divide++;
		if(prescale>5)
			prescale--;
	}

	maestro_write(s, 6, 0x9000 | (prescale<<5) | divide); /* set reg */
	
	/* Now set IDR 11/17 */
	maestro_write(s, 0x11, maestro_read(s, 0x11)|1);
	maestro_write(s, 0x17, maestro_read(s, 0x17)|1);
}

/* this quickly calculates the frequency needed for bob
	and sets it if its different than what bob is
	currently running at.  its called often so 
	needs to be fairly quick. */
#define BOB_MIN 50
#define BOB_MAX 400
static void calc_bob_rate( struct maestro_state *s )
{
/* this thing tries to set the frequency of bob such that
	there are 2 interrupts / buffer walked by the dac/adc.  That
	is probably very wrong for people who actually care about 
	mid buffer positioning.  it should be calculated as bytes/interrupt
	and that needs to be decided :)  so for now just use the static 150
	in start_bob.*/
}

static int prog_dmabuf( struct maestro_state *s, unsigned rec )
{
	struct dmabuf *db = rec ? &s->dma_adc : &s->dma_dac;
	unsigned rate = rec ? s->rateadc : s->ratedac;
	unsigned bytepersec;
	unsigned bufs;
	unsigned char fmt;
	unsigned long flags;

	spinlock_cli(&s->lock, flags);
	fmt = s->fmt;
	if (rec) {
		stop_adc(s);
		fmt >>= ESS_ADC_SHIFT;
	} else {
		stop_dac(s);
		fmt >>= ESS_DAC_SHIFT;
	}
	spinunlock_restore(&s->lock, flags);
	fmt &= ESS_FMT_MASK;

	db->hwptr = db->swptr = db->total_bytes = db->count = db->error = db->endcleared = 0;

	/* this algorithm is a little nuts.. where did /1000 come from? */
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

	kerndbg( KERN_DEBUG, "maestro: setup oss: numfrag: %d fragsize: %d dmasize: %d\n",db->numfrag,db->fragsize,db->dmasize);

	memset(db->rawbuf, (fmt & ESS_FMT_16BIT) ? 0 : 0x80, db->dmasize);

	spinlock_cli(&s->lock, flags);
	if (rec) 
		maestro_rec_setup(s, fmt, s->rateadc, db->rawbuf, db->dmasize);
	else 
		maestro_play_setup(s, fmt, s->ratedac, db->rawbuf, db->dmasize);

	spinunlock_restore(&s->lock, flags);
	db->ready = 1;

	return 0;
}

static __inline__ void 
clear_advance( struct maestro_state *s )
{
	unsigned char c = ((s->fmt >> ESS_DAC_SHIFT) & ESS_FMT_16BIT) ? 0 : 0x80;
	
	unsigned char *buf = s->dma_dac.rawbuf;
	unsigned bsize = s->dma_dac.dmasize;
	unsigned bptr = s->dma_dac.swptr;
	unsigned len = s->dma_dac.fragsize;
	
	if (bptr + len > bsize) {
		unsigned x = bsize - bptr;
		memset(buf + bptr, c, x);
		/* account for wrapping? */
		bptr = 0;
		len -= x;
	}
	memset(buf + bptr, c, len);
}

/* Hardware management */
/* call with spinlock held! */
static void maestro_update_ptr( struct maestro_state *s )
{
	unsigned hwptr;
	int diff;

	/* update ADC pointer */
	if (s->dma_adc.ready) {
		/* oh boy should this all be re-written.  everything in the current code paths think
		   that the various counters/pointers are expressed in bytes to the user but we have
		   two apus doing stereo stuff so we fix it up here.. it propagates to all the various
		   counters from here.  */
		if ( s->fmt & (ESS_FMT_STEREO << ESS_ADC_SHIFT)) {
			hwptr = (get_dmac(s)*2) % s->dma_adc.dmasize;
		} else {
			hwptr = get_dmac(s) % s->dma_adc.dmasize;
		}
		diff = (s->dma_adc.dmasize + hwptr - s->dma_adc.hwptr) % s->dma_adc.dmasize;
		s->dma_adc.hwptr = hwptr;
		s->dma_adc.total_bytes += diff;
		s->dma_adc.count += diff;
		if (s->dma_adc.count >= (signed)s->dma_adc.fragsize) 
			wakeup_sem(s->dma_adc.wait, true);
		if (!s->dma_adc.mapped) {
			if (s->dma_adc.count > (signed)(s->dma_adc.dmasize - ((3 * s->dma_adc.fragsize) >> 1))) {
				/* FILL ME 
				wrindir(s, SV_CIENABLE, s->enable); */
				stop_adc(s); 
				/* brute force everyone back in sync, sigh */
				s->dma_adc.count = 0;
				s->dma_adc.swptr = 0;
				s->dma_adc.hwptr = 0;
				s->dma_adc.error++;
			}
		}
	}
	/* update DAC pointer */
	if (s->dma_dac.ready) {
		hwptr = get_dmaa(s) % s->dma_dac.dmasize; 
		/* the apu only reports the length it has seen, not the
			length of the memory that has been used (the WP
			knows that) */
		if ( ((s->fmt >> ESS_DAC_SHIFT) & ESS_FMT_MASK) == (ESS_FMT_STEREO|ESS_FMT_16BIT))
			hwptr<<=1;

		diff = (s->dma_dac.dmasize + hwptr - s->dma_dac.hwptr) % s->dma_dac.dmasize;
		kerndbg( KERN_DEBUG_LOW, "updating dac: hwptr: %d diff: %d\n",hwptr,diff);
		s->dma_dac.hwptr = hwptr;
		s->dma_dac.total_bytes += diff;
		if (s->dma_dac.mapped) {
			s->dma_dac.count += diff;
			if (s->dma_dac.count >= (signed)s->dma_dac.fragsize) {
				wakeup_sem(s->dma_dac.wait, true);
			}
		} else {
			s->dma_dac.count -= diff;
			kerndbg( KERN_DEBUG_LOW, "maestro: ess_update_ptr: diff: %d, count: %d\n", diff, s->dma_dac.count);
			if (s->dma_dac.count <= 0) {
				kerndbg( KERN_DEBUG, "underflow! diff: %d count: %d hw: %d sw: %d\n", diff, s->dma_dac.count, 
					hwptr, s->dma_dac.swptr);
				/* FILL ME 
				wrindir(s, SV_CIENABLE, s->enable); */
				/* XXX how on earth can calling this with the lock held work.. */
				stop_dac(s);
				/* brute force everyone back in sync, sigh */
				s->dma_dac.count = 0;
				s->dma_dac.swptr = hwptr;
				s->dma_dac.error++;
			} else if (s->dma_dac.count <= (signed)s->dma_dac.fragsize && !s->dma_dac.endcleared) {
				clear_advance(s);
				s->dma_dac.endcleared = 1;
			}
			if (s->dma_dac.count + (signed)s->dma_dac.fragsize <= (signed)s->dma_dac.dmasize) {
				wakeup_sem(s->dma_dac.wait, true);
				kerndbg( KERN_DEBUG, "waking up DAC count: %d sw: %d hw: %d\n",s->dma_dac.count, s->dma_dac.swptr, hwptr);
			}
		}
	}
}

static int maestro_interrupt( int nIRQ, void *pData, SysCallRegs_s *psRegs )
{
	struct maestro_state *s;
	struct maestro_card *c = (struct maestro_card *)pData;
	int i;
	uint32 event;

	if ( ! (event = inb(c->iobase+0x1A)) )
		return 0;

	outw(inw(c->iobase+4)&1, c->iobase+4);
	kerndbg( KERN_DEBUG_LOW, "maestro int: %x\n",event);

	if(event&(1<<6))
	{
		int x;
		enum {UP_EVT, DOWN_EVT, MUTE_EVT} vol_evt;
		int volume;

		/* Figure out which volume control button was pushed,
		   based on differences from the default register
		   values. */
		x = inb(c->iobase+0x1c);
		if (x&1) vol_evt = MUTE_EVT;
		else if (((x>>1)&7) > 4) vol_evt = UP_EVT;
		else vol_evt = DOWN_EVT;

		/* Reset the volume control registers. */
		outb(0x88, c->iobase+0x1c);
		outb(0x88, c->iobase+0x1d);
		outb(0x88, c->iobase+0x1e);
		outb(0x88, c->iobase+0x1f);

		/* Deal with the button press in a hammer-handed
		   manner by adjusting the master mixer volume. */
		volume = c->mix.mixer_state[0] & 0xff;
		if (vol_evt == UP_EVT) {
			volume += 5;
			if (volume > 100)
				volume = 100;
		}
		else if (vol_evt == DOWN_EVT) {
			volume -= 5;
			if (volume < 0)
				volume = 0;
		} else {
			/* vol_evt == MUTE_EVT */
			if (volume == 0)
				volume = c->dock_mute_vol;
			else {
				c->dock_mute_vol = volume;
				volume = 0;
			}
		}
		set_mixer (c, 0, (volume << 8) | volume);
	}

	/* Ack all the interrupts. */
	outb(0xFF, c->iobase+0x1A);

	if( event & 0x04 )
	{
		/*
		 *	Update the pointers for all APU's we are running.
		 */
		for( i=0; i<NR_DSPS; i++ )
		{
			s=&c->channels[i];
			spinlock(&s->lock);
			maestro_update_ptr(s);
			spinunlock(&s->lock);
		}
	}

	return 0;
}

/* DSP interface */
static const char invalid_magic[] = "maestro: invalid magic value in %s\n";

#define VALIDATE_MAGIC(FOO,MAG)                         \
({                                                \
	if (!(FOO) || (FOO)->magic != MAG) { \
		kerndbg( KERN_WARNING, invalid_magic,__FUNCTION__);            \
		return -ENXIO;                    \
	}                                         \
})

#define VALIDATE_STATE(a) VALIDATE_MAGIC(a,ESS_STATE_MAGIC)
#define VALIDATE_CARD(a) VALIDATE_MAGIC(a,ESS_CARD_MAGIC)

static void set_base_registers(struct maestro_state *s,void *vaddr )
{
	unsigned long packed_phys = virt_to_bus(vaddr)>>12;
	wave_set_register(s, 0x01FC , packed_phys);
	wave_set_register(s, 0x01FD , packed_phys);
	wave_set_register(s, 0x01FE , packed_phys);
	wave_set_register(s, 0x01FF , packed_phys);
}

/* we allocate a large power of two for all our memory.
	this is cut up into (not to scale :):
	|silly fifo word	| 512byte mixbuf per adc	| dac/adc * channels |
*/
static int allocate_buffers( struct maestro_state *s )
{
	void *rawbuf=NULL;
	int order,i;
	struct page *page, *pend;

	/* alloc as big a chunk as we can */
	for (order = (dsps_order + (16-PAGE_SHIFT) + 1); order >= (dsps_order + 2 + 1); order--)
	{
		uint32 addr = kmalloc((PAGE_SIZE<<order)+PAGE_SIZE, MEMF_KERNEL|MEMF_CLEAR);
		rawbuf = PAGE_ALIGN(addr);
		if( rawbuf )
			break;
	}

	if (!rawbuf)
		return 1;

	kerndbg( KERN_DEBUG, "maestro: allocated %ld (%d) bytes at %p\n",PAGE_SIZE<<order,order, rawbuf);

	s->card->dmapages = rawbuf;
	s->card->dmaorder = order;

	for(i=0;i<NR_DSPS;i++) {
		struct maestro_state *state = &s->card->channels[i];

		state->dma_dac.ready = s->dma_dac.mapped = 0;
		state->dma_adc.ready = s->dma_adc.mapped = 0;
		state->dma_adc.buforder = state->dma_dac.buforder = order - 1 - dsps_order - 1;

		/* offset dac and adc buffers starting half way through and then at each [da][ad]c's
			order's intervals.. */
		state->dma_dac.rawbuf = rawbuf + (PAGE_SIZE<<(order-1)) + (i * ( PAGE_SIZE << (state->dma_dac.buforder + 1 )));
		state->dma_adc.rawbuf = state->dma_dac.rawbuf + ( PAGE_SIZE << state->dma_dac.buforder);
		/* offset mixbuf by a mixbuf so that the lame status fifo can
			happily scribble away.. */ 
		state->mixbuf = rawbuf + (512 * (i+1));

		kerndbg( KERN_DEBUG, "maestro: setup apu %d: dac: %p adc: %p mix: %p\n",i,state->dma_dac.rawbuf,
			state->dma_adc.rawbuf, state->mixbuf);
	}

	return 0;
}

static void free_buffers( struct maestro_state *s )
{
	s->dma_dac.rawbuf = s->dma_adc.rawbuf = NULL;
	s->dma_dac.mapped = s->dma_adc.mapped = 0;
	s->dma_dac.ready = s->dma_adc.ready = 0;

	kerndbg( KERN_DEBUG, "maestro: freeing %p\n",s->card->dmapages);

	free_pages((unsigned long)s->card->dmapages,s->card->dmaorder);
	s->card->dmapages = NULL;
}

static int drain_dac(struct maestro_state *s, int nonblock )
{
	unsigned long flags;
	int count;
	signed long tmo;

	if (s->dma_dac.mapped || !s->dma_dac.ready)
		return 0;
	for (;;) {
		/* XXX uhm.. questionable locking*/
		spinlock_cli(&s->lock, flags);
		count = s->dma_dac.count;
		spinunlock_restore(&s->lock, flags);
		if (count <= 0)
			break;
		if (is_signal_pending())
			break;
		tmo = (count * HZ) / s->ratedac;
		tmo >>= sample_shift[(s->fmt >> ESS_DAC_SHIFT) & ESS_FMT_MASK];
		/* XXX this is just broken.  someone is waking us up alot, or schedule_timeout is broken.
		   or something.  who cares. - zach */
		sleep_on_sem( s->dma_dac.wait, tmo );
	}

	if (is_signal_pending())
		return -ERESTARTSYS;
	return 0;
}

status_t maestro_dsp_open( void* pNode, uint32 nFlags, void **ppCookie )
{
	struct maestro_card *c = (struct maestro_card *)pNode;
	struct maestro_state *s = &c->channels[0];

	VALIDATE_STATE(s);
	*ppCookie = s;

	kerndbg( KERN_DEBUG, "maestro: open\n" );

	/* wait for device to become free */
	LOCK( s->open_sem );

	kerndbg( KERN_DEBUG, "maestro: allocate_buffers\n" );

	/* under semaphore.. */
	if ((s->card->dmapages==NULL) && allocate_buffers(s)) {
		UNLOCK(s->open_sem);
		return -ENOMEM;
	}

	kerndbg( KERN_DEBUG, "maestro: start_bob\n" );

	/* we're covered by the open_sem */
	if( ! s->card->dsps_open )  {
		start_bob(s);
	}
	s->card->dsps_open++;
	kerndbg( KERN_DEBUG, "maestro: open, %d bobs now\n",s->card->dsps_open);

	/* ok, lets write WC base regs now that we've 
		powered up the chip */
	kerndbg( KERN_DEBUG, "maestro: writing 0x%lx (bus 0x%lx) to the wp\n",virt_to_bus(s->card->dmapages),
		((virt_to_bus(s->card->dmapages))&0xFFE00000)>>12);
	set_base_registers(s,s->card->dmapages);

	/* The Linux OSS driver set the format & rate at this point.  This has been moved into
	   maestro_dsp_read() & maestro_dsp_write() */

//	s->open_mode = 0;
	s->open_mode = FMODE_WRITE;

	kerndbg( KERN_DEBUG, "maestro: open done\n" );

	return 0;
}

status_t maestro_dsp_close( void* pNode, void* pCookie )
{
	struct maestro_state *s = (struct maestro_state *)pCookie;

	VALIDATE_STATE(s);
	if (s->open_mode == FMODE_WRITE)
		drain_dac(s, 0);
	if (s->open_mode == FMODE_WRITE) {
		stop_dac(s);
	}
	if (s->open_mode == FMODE_READ) {
		stop_adc(s);
	}

	s->open_mode = 0;
	/* we're covered by the open_sem */
	kerndbg( KERN_DEBUG, "maestro: %d dsps now alive\n",s->card->dsps_open-1);
	if( --s->card->dsps_open <= 0) {
		s->card->dsps_open = 0;
		stop_bob(s);
		free_buffers(s);
	}
	UNLOCK(s->open_sem);
	return 0;
}

int maestro_dsp_read( void *pNode, void *pCookie, off_t nPos, void *pBuffer, size_t nCount )
{
	struct maestro_card *c = (struct maestro_card *)pNode;
	struct maestro_state *s = (struct maestro_state *)pCookie;

	if( s->open_mode == 0 )
	{
		unsigned char fmtm = ~0, fmts = 0;

		fmtm &= ~((ESS_FMT_STEREO|ESS_FMT_16BIT) << ESS_ADC_SHIFT);
		fmts = (ESS_FMT_STEREO|ESS_FMT_16BIT) << ESS_ADC_SHIFT;

		s->dma_adc.ossfragshift = s->dma_adc.ossmaxfrags = s->dma_adc.subdivision = 0;
		set_adc_rate(s, 44000);
		set_fmt(s, fmtm, fmts);

		s->open_mode = FMODE_READ;
	}

	/* XXXKV: Todo */
	kerndbg( KERN_WARNING, "maestro_dsp_read() not implemented!\n" );
	return -1;
}

int maestro_dsp_write( void *pNode, void *pCookie, off_t nPos, const void *pBuffer, size_t nCount )
{
	struct maestro_card *c = (struct maestro_card *)pNode;
	struct maestro_state *s = (struct maestro_state *)pCookie;
	ssize_t ret;
	unsigned long flags;
	unsigned swptr;
	int cnt;

	kerndbg( KERN_DEBUG, "maestro_dsp_write() enter\n" );

	if( s->open_mode == 0 )
	{
		unsigned char fmtm = ~0, fmts = 0;

		kerndbg( KERN_DEBUG, "Setting format\n" );

		fmtm = fmts = (ESS_FMT_STEREO|ESS_FMT_16BIT) << ESS_DAC_SHIFT;

		s->dma_dac.ossfragshift = s->dma_dac.ossmaxfrags = s->dma_dac.subdivision = 0;
		set_fmt(s, fmtm, fmts);
		set_dac_rate(s, 44100);

		s->open_mode = FMODE_WRITE;
	}

	kerndbg( KERN_DEBUG, "start write\n" );

	VALIDATE_STATE(s);
	if (s->dma_dac.mapped)
		return -ENXIO;
	if (!s->dma_dac.ready && (ret = prog_dmabuf(s, 0)))
		return ret;
	ret = 0;

	kerndbg( KERN_DEBUG, "calc_bob_rate\n" );

	calc_bob_rate(s);

	while (nCount > 0) {
		spinlock_cli(&s->lock, flags);

		if (s->dma_dac.count < 0) {
			s->dma_dac.count = 0;
			s->dma_dac.swptr = s->dma_dac.hwptr;
		}
		swptr = s->dma_dac.swptr;

		cnt = s->dma_dac.dmasize-swptr;

		if (s->dma_dac.count + cnt > s->dma_dac.dmasize)
			cnt = s->dma_dac.dmasize - s->dma_dac.count;

		spinunlock_restore(&s->lock, flags);

		if (cnt > nCount)
			cnt = nCount;

		kerndbg( KERN_DEBUG, "nCount=%d cnt=%d\n", nCount, cnt );

		if (cnt <= 0) {
			start_dac(s);
			if (sleep_on_sem(s->dma_dac.wait, INFINITE_TIMEOUT)) {
				kerndbg( KERN_DEBUG, "waited too long for DAC\n" );
				stop_dac(s);
				spinlock_cli(&s->lock, flags);
				/* program enhanced mode registers */
/*				wrindir(s, SV_CIDMAABASECOUNT1, (s->dma_dac.fragsamples-1) >> 8);
				wrindir(s, SV_CIDMAABASECOUNT0, s->dma_dac.fragsamples-1); */
				/* FILL ME */
				s->dma_dac.count = s->dma_dac.hwptr = s->dma_dac.swptr = 0;
				spinunlock_restore(&s->lock, flags);
			}
			if (is_signal_pending()) {
				if (!ret) ret = -ERESTARTSYS;
				goto return_free;
			}
			continue;
		}
		if (memcpy_from_user(s->dma_dac.rawbuf + swptr, pBuffer, cnt)) {
			if (!ret) ret = -EFAULT;
			goto return_free;
		}
		kerndbg( KERN_DEBUG, "wrote %d bytes at sw: %d cnt: %d while hw: %d\n",cnt, swptr, s->dma_dac.count, s->dma_dac.hwptr);

		swptr = (swptr + cnt) % s->dma_dac.dmasize;

		spinlock_cli(&s->lock, flags);
		s->dma_dac.swptr = swptr;
		s->dma_dac.count += cnt;
		s->dma_dac.endcleared = 0;
		spinunlock_restore(&s->lock, flags);
		nCount -= cnt;
		pBuffer += cnt;
		ret += cnt;

		kerndbg( KERN_DEBUG, "starting dac with sw: %d dac cnt: %d hw: %d\n", swptr, s->dma_dac.count, s->dma_dac.hwptr);

		start_dac(s);
	}

	kerndbg( KERN_DEBUG, "maestro_dsp_write() done, wrote %d\n", ret );

return_free:
	return ret;
}

status_t maestro_dsp_ioctl( void *pNode, void *pCookie, uint32 nCmd, void *pArg, bool bFromKernel )
{
	struct maestro_state *s = (struct maestro_state *)pCookie;
	unsigned long flags;
	audio_buf_info abinfo;
	count_info cinfo;
	int val, ret;
	unsigned char fmtm, fmtd;
	void *argp = (void *)pArg;
	int *p = argp;

	kerndbg( KERN_DEBUG, "maestro: maesto_dsp_ioctl: cmd %d\n", nCmd);
	VALIDATE_STATE(s);

	switch (nCmd) {
		case OSS_GETVERSION:
			kerndbg( KERN_DEBUG, "OSS_GETVERSION\n");
			put_user(SOUND_VERSION, p);
			return 0;

		case SNDCTL_DSP_SYNC:
			kerndbg( KERN_DEBUG, "SNDCTL_DSP_SYNC\n");
			if (s->open_mode & FMODE_WRITE)
				return drain_dac(s, 0);
			return 0;

		case SNDCTL_DSP_SETDUPLEX:
			kerndbg( KERN_DEBUG, "SNDCTL_DSP_SETDUPLEX\n");
			/* XXX fix */
			return 0;

		case SNDCTL_DSP_GETCAPS:
			kerndbg( KERN_DEBUG, "SNDCTL_DSP_GETCAPS\n");
			put_user(DSP_CAP_DUPLEX | DSP_CAP_REALTIME | DSP_CAP_TRIGGER | DSP_CAP_MMAP, p);
			return 0;

		case SNDCTL_DSP_RESET:
			kerndbg( KERN_DEBUG, "SNDCTL_DSP_RESET\n");
			if (s->open_mode & FMODE_WRITE) {
				stop_dac(s);
				s->dma_dac.swptr = s->dma_dac.hwptr = s->dma_dac.count = s->dma_dac.total_bytes = 0;
			}
			if (s->open_mode & FMODE_READ) {
				stop_adc(s);
				s->dma_adc.swptr = s->dma_adc.hwptr = s->dma_adc.count = s->dma_adc.total_bytes = 0;
			}
			return 0;

		case SNDCTL_DSP_SPEED:
			kerndbg( KERN_DEBUG, "SNDCTL_DSP_SPEED\n");
			get_user(val, p);
			if (val >= 0) {
				if (s->open_mode & FMODE_READ) {
					stop_adc(s);
					s->dma_adc.ready = 0;
					set_adc_rate(s, val);
				}
				if (s->open_mode & FMODE_WRITE) {
					stop_dac(s);
					s->dma_dac.ready = 0;
					set_dac_rate(s, val);
				}
			}
			put_user((s->open_mode & FMODE_READ) ? s->rateadc : s->ratedac, p);
			return 0;

		case SNDCTL_DSP_STEREO:
			kerndbg( KERN_DEBUG, "SNDCTL_DSP_STEREO\n");
			get_user(val, p);
			fmtd = 0;
			fmtm = ~0;
			if (s->open_mode & FMODE_READ) {
				stop_adc(s);
				s->dma_adc.ready = 0;
				if (val)
					fmtd |= ESS_FMT_STEREO << ESS_ADC_SHIFT;
				else
					fmtm &= ~(ESS_FMT_STEREO << ESS_ADC_SHIFT);
			}
			if (s->open_mode & FMODE_WRITE) {
				stop_dac(s);
				s->dma_dac.ready = 0;
				if (val)
					fmtd |= ESS_FMT_STEREO << ESS_DAC_SHIFT;
				else
					fmtm &= ~(ESS_FMT_STEREO << ESS_DAC_SHIFT);
			}
			set_fmt(s, fmtm, fmtd);
			return 0;

		case SNDCTL_DSP_CHANNELS:
			kerndbg( KERN_DEBUG, "SNDCTL_DSP_CHANNELS\n");
			get_user(val, p);
			if (val != 0) {
				fmtd = 0;
				fmtm = ~0;
				if (s->open_mode & FMODE_READ) {
					stop_adc(s);
					s->dma_adc.ready = 0;
					if (val >= 2)
						fmtd |= ESS_FMT_STEREO << ESS_ADC_SHIFT;
					else
						fmtm &= ~(ESS_FMT_STEREO << ESS_ADC_SHIFT);
				}
				if (s->open_mode & FMODE_WRITE) {
					stop_dac(s);
					s->dma_dac.ready = 0;
					if (val >= 2)
						fmtd |= ESS_FMT_STEREO << ESS_DAC_SHIFT;
					else
						fmtm &= ~(ESS_FMT_STEREO << ESS_DAC_SHIFT);
				}
				set_fmt(s, fmtm, fmtd);
			}
			put_user((s->fmt & ((s->open_mode & FMODE_READ) ? (ESS_FMT_STEREO << ESS_ADC_SHIFT) 
					   : (ESS_FMT_STEREO << ESS_DAC_SHIFT))) ? 2 : 1, p);
			return 0;

		case SNDCTL_DSP_GETFMTS: /* Returns a mask */
			kerndbg( KERN_DEBUG, "SNDCTL_DSP_GETFMTS\n");
			put_user(AFMT_U8|AFMT_S16_LE, p);
			return 0;
		
		case SNDCTL_DSP_SETFMT: /* Selects ONE fmt*/
			kerndbg( KERN_DEBUG, "SNDCTL_DSP_SETFMT\n");
			get_user(val, p);
			if (val != AFMT_QUERY) {
				fmtd = 0;
				fmtm = ~0;
				if (s->open_mode & FMODE_READ) {
					stop_adc(s);
					s->dma_adc.ready = 0;
					/* fixed at 16bit for now */
					fmtd |= ESS_FMT_16BIT << ESS_ADC_SHIFT;
#if 0
					if (val == AFMT_S16_LE)
						fmtd |= ESS_FMT_16BIT << ESS_ADC_SHIFT;
					else
						fmtm &= ~(ESS_FMT_16BIT << ESS_ADC_SHIFT);
#endif
				}
				if (s->open_mode & FMODE_WRITE) {
					stop_dac(s);
					s->dma_dac.ready = 0;
					if (val == AFMT_S16_LE)
						fmtd |= ESS_FMT_16BIT << ESS_DAC_SHIFT;
					else
						fmtm &= ~(ESS_FMT_16BIT << ESS_DAC_SHIFT);
				}
				set_fmt(s, fmtm, fmtd);
			}
			put_user((s->fmt & ((s->open_mode & FMODE_READ) ? (ESS_FMT_16BIT << ESS_ADC_SHIFT) : (ESS_FMT_16BIT << ESS_DAC_SHIFT))) ? AFMT_S16_LE : AFMT_U8, p);
			return 0;

		case SNDCTL_DSP_POST:
			kerndbg( KERN_DEBUG, "SNDCTL_DSP_POST\n");
			return 0;

		case SNDCTL_DSP_GETTRIGGER:
			kerndbg( KERN_DEBUG, "SNDCTL_DSP_GETTRIGGER\n");
			val = 0;
			if ((s->open_mode & FMODE_READ) && (s->enable & ADC_RUNNING))
				val |= PCM_ENABLE_INPUT;
			if ((s->open_mode & FMODE_WRITE) && (s->enable & DAC_RUNNING)) 
				val |= PCM_ENABLE_OUTPUT;
			put_user(val, p);
			return 0;

		case SNDCTL_DSP_SETTRIGGER:
			kerndbg( KERN_DEBUG, "SNDCTL_DSP_SETTRIGGER\n");
			get_user(val, p);
			if (s->open_mode & FMODE_READ) {
				if (val & PCM_ENABLE_INPUT) {
					if (!s->dma_adc.ready && (ret =  prog_dmabuf(s, 1)))
						return ret;
					start_adc(s);
				} else
					stop_adc(s);
			}
			if (s->open_mode & FMODE_WRITE) {
				if (val & PCM_ENABLE_OUTPUT) {
					if (!s->dma_dac.ready && (ret = prog_dmabuf(s, 0)))
						return ret;
					start_dac(s);
				} else
					stop_dac(s);
			}
			return 0;

		case SNDCTL_DSP_GETOSPACE:
			kerndbg( KERN_DEBUG, "SNDCTL_DSP_GETOSPACE\n");
#if 0
			if (!(s->open_mode & FMODE_WRITE))
				return -EINVAL;
#endif
			if (!s->dma_dac.ready && (ret = prog_dmabuf(s, 0)))
				return ret;
			spinlock_cli(&s->lock, flags);
			maestro_update_ptr(s);
			abinfo.fragsize = s->dma_dac.fragsize;
			abinfo.bytes = s->dma_dac.dmasize - s->dma_dac.count;
			abinfo.fragstotal = s->dma_dac.numfrag;
			abinfo.fragments = abinfo.bytes >> s->dma_dac.fragshift;      
			spinunlock_restore(&s->lock, flags);
			return memcpy_to_user(argp, &abinfo, sizeof(abinfo)) ? -EFAULT : 0;

		case SNDCTL_DSP_GETISPACE:
			kerndbg( KERN_DEBUG, "SNDCTL_DSP_GETISPACE\n");
			if (!(s->open_mode & FMODE_READ))
			return -EINVAL;
			if (!s->dma_adc.ready && (ret =  prog_dmabuf(s, 1)))
				return ret;
			spinlock_cli(&s->lock, flags);
			maestro_update_ptr(s);
			abinfo.fragsize = s->dma_adc.fragsize;
			abinfo.bytes = s->dma_adc.count;
			abinfo.fragstotal = s->dma_adc.numfrag;
			abinfo.fragments = abinfo.bytes >> s->dma_adc.fragshift;      
			spinunlock_restore(&s->lock, flags);
			return memcpy_to_user(argp, &abinfo, sizeof(abinfo)) ? -EFAULT : 0;

		case SNDCTL_DSP_GETODELAY:
			kerndbg( KERN_DEBUG, "SNDCTL_DSP_GETODELAY\n");
#if 0
			if (!(s->open_mode & FMODE_WRITE))
				return -EINVAL;
#endif
			if (!s->dma_dac.ready && (ret = prog_dmabuf(s, 0)))
				return ret;
			spinlock_cli(&s->lock, flags);
			maestro_update_ptr(s);
			val = s->dma_dac.count;
			spinunlock_restore(&s->lock, flags);
			put_user(val, p);
			return 0;

		case SNDCTL_DSP_GETIPTR:
			kerndbg( KERN_DEBUG, "SNDCTL_DSP_GETIPTR\n");
			if (!(s->open_mode & FMODE_READ))
				return -EINVAL;
			if (!s->dma_adc.ready && (ret =  prog_dmabuf(s, 1)))
				return ret;
			spinlock_cli(&s->lock, flags);
			maestro_update_ptr(s);
			cinfo.bytes = s->dma_adc.total_bytes;
			cinfo.blocks = s->dma_adc.count >> s->dma_adc.fragshift;
			cinfo.ptr = s->dma_adc.hwptr;
			if (s->dma_adc.mapped)
				s->dma_adc.count &= s->dma_adc.fragsize-1;
			spinunlock_restore(&s->lock, flags);
			return memcpy_to_user(argp, &cinfo, sizeof(cinfo));

		case SNDCTL_DSP_GETOPTR:
			kerndbg( KERN_DEBUG, "SNDCTL_DSP_GETOPTR\n");
			if (!(s->open_mode & FMODE_WRITE))
				return -EINVAL;
			if (!s->dma_dac.ready && (ret = prog_dmabuf(s, 0)))
				return ret;
			spinlock_cli(&s->lock, flags);
			maestro_update_ptr(s);
			cinfo.bytes = s->dma_dac.total_bytes;
			cinfo.blocks = s->dma_dac.count >> s->dma_dac.fragshift;
			cinfo.ptr = s->dma_dac.hwptr;
			if (s->dma_dac.mapped)
				s->dma_dac.count &= s->dma_dac.fragsize-1;
			spinunlock_restore(&s->lock, flags);
			return memcpy_to_user(argp, &cinfo, sizeof(cinfo));

		case SNDCTL_DSP_GETBLKSIZE:
			kerndbg( KERN_DEBUG, "SNDCTL_DSP_GETBLKSIZE\n");
			if (s->open_mode & FMODE_WRITE) {
				if ((val = prog_dmabuf(s, 0)))
					return val;
				put_user(s->dma_dac.fragsize, p);
				return 0;
			}
			if ((val = prog_dmabuf(s, 1)))
				return val;
			put_user(s->dma_adc.fragsize, p);
			return 0;

		case SNDCTL_DSP_SETFRAGMENT:
			kerndbg( KERN_DEBUG, "SNDCTL_DSP_SETFRAGMENT\n");
			get_user(val, p);
			if (s->open_mode & FMODE_READ) {
				s->dma_adc.ossfragshift = val & 0xffff;
				s->dma_adc.ossmaxfrags = (val >> 16) & 0xffff;
				if (s->dma_adc.ossfragshift < 4)
					s->dma_adc.ossfragshift = 4;
				if (s->dma_adc.ossfragshift > 15)
					s->dma_adc.ossfragshift = 15;
				if (s->dma_adc.ossmaxfrags < 4)
					s->dma_adc.ossmaxfrags = 4;
			}
			if (s->open_mode & FMODE_WRITE) {
				s->dma_dac.ossfragshift = val & 0xffff;
				s->dma_dac.ossmaxfrags = (val >> 16) & 0xffff;
				if (s->dma_dac.ossfragshift < 4)
					s->dma_dac.ossfragshift = 4;
				if (s->dma_dac.ossfragshift > 15)
					s->dma_dac.ossfragshift = 15;
				if (s->dma_dac.ossmaxfrags < 4)
					s->dma_dac.ossmaxfrags = 4;
			}
			return 0;

		case SNDCTL_DSP_SUBDIVIDE:
			kerndbg( KERN_DEBUG, "SNDCTL_DSP_SUBDIVIDE\n");
			if ((s->open_mode & FMODE_READ && s->dma_adc.subdivision) ||
			    (s->open_mode & FMODE_WRITE && s->dma_dac.subdivision))
				return -EINVAL;
			get_user(val, p);
			if (val != 1 && val != 2 && val != 4)
				return -EINVAL;
			if (s->open_mode & FMODE_READ)
				s->dma_adc.subdivision = val;
			if (s->open_mode & FMODE_WRITE)
				s->dma_dac.subdivision = val;
			return 0;

		case SOUND_PCM_READ_RATE:
			kerndbg( KERN_DEBUG, "SOUND_PCM_READ_RATE\n");
			put_user((s->open_mode & FMODE_READ) ? s->rateadc : s->ratedac, p);
			return 0;

		case SOUND_PCM_READ_CHANNELS:
			kerndbg( KERN_DEBUG, "SOUND_PCM_READ_CHANNELS\n");
			put_user((s->fmt & ((s->open_mode & FMODE_READ) ? (ESS_FMT_STEREO << ESS_ADC_SHIFT) 
					   : (ESS_FMT_STEREO << ESS_DAC_SHIFT))) ? 2 : 1, p);
			return 0;

		case SOUND_PCM_READ_BITS:
			kerndbg( KERN_DEBUG, "SOUND_PCM_READ_BITS\n");
			put_user((s->fmt & ((s->open_mode & FMODE_READ) ? (ESS_FMT_16BIT << ESS_ADC_SHIFT) 
					   : (ESS_FMT_16BIT << ESS_DAC_SHIFT))) ? 16 : 8, p);
			return 0;

		case IOCTL_GET_USERSPACE_DRIVER:
		{
			memcpy_to_user( p, "oss.so", strlen( "oss.so" ) );
			return( 0 );
		}

	}
	return -EINVAL;

}

DeviceOperations_s maestro_dsp_fops = {
	maestro_dsp_open,
	maestro_dsp_close,
	maestro_dsp_ioctl,
	maestro_dsp_read,
	maestro_dsp_write
};

/* Mixer interface */
static void set_mixer( struct maestro_card *card, unsigned int mixer, unsigned int val ) 
{
	unsigned int left,right;
	/* cleanse input a little */
	right = ((val >> 8)  & 0xff) ;
	left = (val  & 0xff) ;

	if(right > 100) right = 100;
	if(left > 100) left = 100;

	card->mix.mixer_state[mixer]=(right << 8) | left;
	card->mix.write_mixer( card, mixer, left, right );
}

static void mixer_push_state( struct maestro_card *card )
{
	int i;
	for(i = 0 ; i < SOUND_MIXER_NRDEVICES ; i++) {
		if( ! supported_mixer( card, i )) continue;

		set_mixer( card, i, card->mix.mixer_state[i] );
	}
}

static int mixer_ioctl(struct maestro_card *card, unsigned int cmd, void *argp, bool from_kernel)
{
	int i, val=0;
	unsigned long flags;
	int *p = argp;

	VALIDATE_CARD(card);

	if (cmd == SOUND_MIXER_INFO) {
		mixer_info info;
		memset(&info, 0, sizeof(info));
		strlcpy(info.id, card_names[card->card_type], sizeof(info.id));
		strlcpy(info.name, card_names[card->card_type], sizeof(info.name));
		info.modify_counter = card->mix.modcnt;
		if (memcpy_to_user(argp, &info, sizeof(info)))
			return -EFAULT;
		return 0;
	}
	if (cmd == SOUND_OLD_MIXER_INFO) {
		_old_mixer_info info;
		memset(&info, 0, sizeof(info));
		strlcpy(info.id, card_names[card->card_type], sizeof(info.id));
		strlcpy(info.name, card_names[card->card_type], sizeof(info.name));
		if (memcpy_to_user(argp, &info, sizeof(info)))
			return -EFAULT;
		return 0;
	}
	if (cmd == OSS_GETVERSION)
	{
		put_user(SOUND_VERSION, p);
		return 0;
	}

	if (_IOC_DIR(cmd) == _IOC_READ) {
		switch (_IOC_NR(cmd)) {
			case SOUND_MIXER_RECSRC: /* give them the current record source */
				if(!card->mix.recmask_io) {
					val = 0;
				} else {
					spinlock_cli(&card->lock, flags);
					val = card->mix.recmask_io(card,1,0);
					spinunlock_restore(&card->lock, flags);
				}
				break;
			
			case SOUND_MIXER_DEVMASK: /* give them the supported mixers */
				val = card->mix.supported_mixers;
				break;

			case SOUND_MIXER_RECMASK: /* Arg contains a bit for each supported recording source */
				val = card->mix.record_sources;
				break;
			
			case SOUND_MIXER_STEREODEVS: /* Mixer channels supporting stereo */
				val = card->mix.stereo_mixers;
				break;
			
			case SOUND_MIXER_CAPS:
				val = SOUND_CAP_EXCL_INPUT;
				break;

			default: /* read a specific mixer */
				i = _IOC_NR(cmd);

				if ( ! supported_mixer(card,i)) 
					return -EINVAL;

				val = card->mix.mixer_state[i];
				kerndbg( KERN_DEBUG, "returned 0x%x for mixer %d\n",val,i);

				break;
		}
		put_user(val, p);
		return 0;
	}
	
	if (_IOC_DIR(cmd) != (_IOC_WRITE|_IOC_READ))
		return -EINVAL;
	
	card->mix.modcnt++;

	get_user(val, p);

	switch (_IOC_NR(cmd)) {
		case SOUND_MIXER_RECSRC: /* Arg contains a bit for each recording source */

			if (!card->mix.recmask_io) return -EINVAL;
			if(!val) return 0;
			if(! (val &= card->mix.record_sources)) return -EINVAL;

			spinlock_cli(&card->lock, flags);
			card->mix.recmask_io(card,0,val);
			spinunlock_restore(&card->lock, flags);
			break;

		default:
			i = _IOC_NR(cmd);

			if ( ! supported_mixer(card,i)) 
				return -EINVAL;

			spinlock_cli(&card->lock, flags);
			set_mixer(card,i,val);
			spinunlock_restore(&card->lock, flags);
			break;
	}

	return 0;
}

status_t maestro_mixer_open( void* pNode, uint32 nFlags, void **pCookie )
{
	if( pNode == NULL )
		return -ENODEV;
	return EOK;
}

status_t maestro_mixer_close( void* pNode, void* pCookie )
{
	struct maestro_card *card = (struct maestro_card *)pNode;

	VALIDATE_CARD(card);
	return EOK;
}

status_t maestro_mixer_ioctl(void *pNode, void *pCookie, uint32 nCmd, void *pArgs, bool bFromKernel )
{
	struct maestro_card *card = (struct maestro_card *)pNode;

	VALIDATE_CARD(card);
	return mixer_ioctl(card, nCmd, pArgs, bFromKernel);
}

DeviceOperations_s maestro_mixer_fops = {
	maestro_mixer_open,
	maestro_mixer_close,
	maestro_mixer_ioctl,
	NULL,
	NULL
};

/* Driver management */
static int maestro_config( struct maestro_card *card ) 
{
	PCI_Info_s *pcidev = card->pcidev;
	struct maestro_state *state = &card->channels[0];
	int apu, iobase = card->iobase;
	uint16 w;
	uint32 n;

	/* We used to muck around with pci config space that
	 * we had no business messing with.  We don't know enough
	 * about the machine to know which DMA mode is appropriate, 
	 * etc.  We were guessing wrong on some machines and making
	 * them unhappy.  We now trust in the BIOS to do things right,
	 * which almost certainly means a new host of problems will
	 * arise with broken BIOS implementations.  screw 'em. 
	 * We're already intolerant of machines that don't assign
	 * IRQs.
	 */

	/* XXXKV: This code is packed with magic numbers for both
	   the PCI config & I/O access. */

	w = g_psBus->read_pci_config( pcidev->nBus, pcidev->nDevice, pcidev->nFunction, 0x50, 2 );

	w&=~(1<<5);			/* Don't swap left/right (undoc)*/
	
	g_psBus->write_pci_config( pcidev->nBus, pcidev->nDevice, pcidev->nFunction, 0x50, 2, w );
	
	w = g_psBus->read_pci_config( pcidev->nBus, pcidev->nDevice, pcidev->nFunction, 0x52, 2 );
	w&=~(1<<15);		/* Turn off internal clock multiplier */
	/* XXX how do we know which to use? */
	w&=~(1<<14);		/* External clock */
	
	w|= (1<<7);		/* Hardware volume control on */
	w|= (1<<6);		/* Debounce off: easier to push the HWV buttons. */
	w&=~(1<<5);		/* GPIO 4:5 */
	w|= (1<<4);             /* Disconnect from the CHI.  Enabling this made a dell 7500 work. */
	w&=~(1<<2);		/* MIDI fix off (undoc) */
	w&=~(1<<1);		/* reserved, always write 0 */
	g_psBus->write_pci_config( pcidev->nBus, pcidev->nDevice, pcidev->nFunction, 0x52, 2, w );

	/*
	 *	Legacy mode
	 */

	w = g_psBus->read_pci_config( pcidev->nBus, pcidev->nDevice, pcidev->nFunction, 0x40, 2 );
	w|=(1<<15);	/* legacy decode off */
	w&=~(1<<14);	/* Disable SIRQ */
	w&=~(0x1f);	/* disable mpu irq/io, game port, fm, SB */
	 
	g_psBus->write_pci_config( pcidev->nBus, pcidev->nDevice, pcidev->nFunction, 0x40, 2, w );

	/* Set up 978 docking control chip. */
	w = g_psBus->read_pci_config( pcidev->nBus, pcidev->nDevice, pcidev->nFunction, 0x58, 2 );
	w|=1<<2;	/* Enable 978. */
	w|=1<<3;	/* Turn on 978 hardware volume control. */
	w&=~(1<<11);	/* Turn on 978 mixer volume control. */
	g_psBus->write_pci_config( pcidev->nBus, pcidev->nDevice, pcidev->nFunction, 0x58, 2, w );

	sound_reset( iobase );

	/*
	 *	Ring Bus Setup
	 */

	/* setup usual 0x34 stuff.. 0x36 may be chip specific */
	outw( 0xC090, iobase+0x34 ); /* direct sound, stereo */
	udelay( 20 );
	outw( 0x3000, iobase+0x36 ); /* direct sound, stereo */
	udelay( 20 );

	/*
	 *	Reset the CODEC
	 */

	maestro_ac97_reset(iobase,pcidev);

	/*
	 *	Ring Bus Setup
	 */

	n=inl(iobase+0x34);
	n&=~0xF000;
	n|=12<<12;		/* Direct Sound, Stereo */
	outl(n, iobase+0x34);

	n=inl(iobase+0x34);
	n&=~0x0F00;		/* Modem off */
	outl(n, iobase+0x34);

	n=inl(iobase+0x34);
	n&=~0x00F0;
	n|=9<<4;		/* DAC, Stereo */
	outl(n, iobase+0x34);
	
	n=inl(iobase+0x34);
	n&=~0x000F;		/* ASSP off */
	outl(n, iobase+0x34);
	
	n=inl(iobase+0x34);
	n|=(1<<29);		/* Enable ring bus */
	outl(n, iobase+0x34);
	
	n=inl(iobase+0x34);
	n|=(1<<28);		/* Enable serial bus */
	outl(n, iobase+0x34);
	
	n=inl(iobase+0x34);
	n&=~0x00F00000;		/* MIC off */
	outl(n, iobase+0x34);
	
	n=inl(iobase+0x34);
	n&=~0x000F0000;		/* I2S off */
	outl(n, iobase+0x34);

	w=inw(iobase+0x18);
	w&=~(1<<7);		/* ClkRun off */
	outw(w, iobase+0x18);

	w=inw(iobase+0x18);
	w&=~(1<<6);		/* Hardware volume control interrupt off... for now. */
	outw(w, iobase+0x18);
	
	w=inw(iobase+0x18);
	w&=~(1<<4);		/* ASSP irq off */
	outw(w, iobase+0x18);
	
	w=inw(iobase+0x18);
	w&=~(1<<3);		/* ISDN irq off */
	outw(w, iobase+0x18);
	
	w=inw(iobase+0x18);
	w|=(1<<2);		/* Direct Sound IRQ on */
	outw(w, iobase+0x18);

	w=inw(iobase+0x18);
	w&=~(1<<1);		/* MPU401 IRQ off */
	outw(w, iobase+0x18);

	w=inw(iobase+0x18);
	w|=(1<<0);		/* SB IRQ on */
	outw(w, iobase+0x18);

	/* Set hardware volume control registers to midpoints.
	   We can tell which button was pushed based on how they change. */
	outb(0x88, iobase+0x1c);
	outb(0x88, iobase+0x1d);
	outb(0x88, iobase+0x1e);
	outb(0x88, iobase+0x1f);

	/* it appears some maestros (dell 7500) only work if these are set,
		regardless of whether we use the assp or not. */

	outb(0, iobase+0xA4); 
	outb(3, iobase+0xA2); 
	outb(0, iobase+0xA6);

	for(apu=0;apu<16;apu++)
	{
		/* Write 0 into the buffer area 0x1E0->1EF */
		outw(0x01E0+apu, 0x10+iobase);
		outw(0x0000, 0x12+iobase);
	
		/*
		 * The 1.10 test program seem to write 0 into the buffer area
		 * 0x1D0-0x1DF too.
		 */
		outw(0x01D0+apu, 0x10+iobase);
		outw(0x0000, 0x12+iobase);
	}

	wave_set_register( state, IDR7_WAVE_ROMRAM, 
		(wave_get_register( state, IDR7_WAVE_ROMRAM)&0xFF00));
	wave_set_register( state, IDR7_WAVE_ROMRAM,
		wave_get_register( state, IDR7_WAVE_ROMRAM)|0x100);
	wave_set_register( state, IDR7_WAVE_ROMRAM,
		wave_get_register( state, IDR7_WAVE_ROMRAM)&~0x200);
	wave_set_register( state, IDR7_WAVE_ROMRAM,
		wave_get_register( state, IDR7_WAVE_ROMRAM)|~0x400);

	maestro_write( state, IDR2_CRAM_DATA, 0x0000 );
	maestro_write( state, 0x08, 0xB004 );
	/* Now back to the DirectSound stuff */
	maestro_write( state, 0x09, 0x001B );
	maestro_write( state, 0x0A, 0x8000 );
	maestro_write( state, 0x0B, 0x3F37 );
	maestro_write( state, 0x0C, 0x0098 );

	/* parallel out ?? */
	maestro_write( state, 0x0C, 
		( maestro_read( state, 0x0C ) &~0xF000)|0x8000 ); 
	/* parallel in, has something to do with recording :) */
	maestro_write( state, 0x0C, 
		( maestro_read( state, 0x0C ) &~0x0F00)|0x0500 );

	maestro_write( state, 0x0D, 0x7632 );

	/* Wave cache control on - test off, sg off, 
		enable, enable extra chans 1Mb */

	outw(inw(0x14+iobase)|(1<<8),0x14+iobase);
	outw(inw(0x14+iobase)&0xFE03,0x14+iobase);
	outw((inw(0x14+iobase)&0xFFFC), 0x14+iobase);
	outw(inw(0x14+iobase)|(1<<7),0x14+iobase);

	outw(0xA1A0, 0x14+iobase);      /* 0300 ? */

	/* Now clear the APU control ram */	
	for( apu=0; apu<NR_APUS; apu++ )
		for( w=0; w<NR_APU_REGS; w++)
			apu_set_register( state, apu|ESS_CHAN_HARD, w, 0 );

	return 0;
}

static status_t maestro_init_one( PCI_Info_s *psPCIDevice, struct maestro_card **ppsCard, int card_type )
{
	uint32 n;
	int i, ret;
	struct maestro_card *card;
	struct maestro_state *state;
	int num = 0;

#if 0
	/* XXXKV: Need to fit this to Syllable, which has three seperate byte fields for Class, ClassSub & ClassAPI */

	/* don't pick up weird modem maestros */
	if( psPCIDevice->nClassSub != PCI_CLASS_MULTIMEDIA_AUDIO )
		return -ENODEV;
#endif

	if( ( card = kmalloc( sizeof( struct maestro_card ), MEMF_KERNEL ) ) == NULL )
	{
		kerndbg(KERN_WARNING, "Out of memory\n");
		goto out_delete_area;
	}
	memset(card, 0, sizeof( *card ) );

	card->iobase = psPCIDevice->u.h0.nBase0 & PCI_ADDRESS_MEMORY_32_MASK;

	/* just to be sure */
	g_psBus->enable_pci_master( psPCIDevice->nBus, psPCIDevice->nDevice, psPCIDevice->nFunction );

	card->pcidev = psPCIDevice;
	card->card_type = card_type;
	card->irq = psPCIDevice->u.h0.nInterruptLine;
	card->magic = ESS_CARD_MAGIC;
	spinlock_init(&card->lock, "maestro");

	card->dock_mute_vol = 50;

	/* init our groups of 6 apus */
	for( i=0; i<NR_DSPS; i++ )
	{
		struct maestro_state *s=&card->channels[i];

		s->index = i;

		s->card = card;

		s->dma_adc.wait = create_semaphore( "maestro_adc_wait", 1, 0 );
		s->dma_dac.wait = create_semaphore( "maestro_dac_wait", 1, 0 );

		spinlock_init( &s->lock, "maestro_channel" );
		s->open_sem = create_semaphore( "maestro_open", 1, 0 );

		s->magic = ESS_STATE_MAGIC;
		
		s->apu[0] = 6*i;
		s->apu[1] = (6*i)+1;
		s->apu[2] = (6*i)+2;
		s->apu[3] = (6*i)+3;
		s->apu[4] = (6*i)+4;
		s->apu[5] = (6*i)+5;
		
		if( s->dma_adc.ready || s->dma_dac.ready || s->dma_adc.rawbuf )
			kerndbg( KERN_DEBUG_LOW, "maestro: BOTCH!\n");
	}
	num = i;
	state = &card->channels[0];

	/*
	 *	Ok card ready. Begin setup proper
	 */

	maestro_config( card );
	maestro_ac97_init( card );

	memcpy( card->mix.mixer_state, mixer_defaults, sizeof( card->mix.mixer_state ) );
	mixer_push_state( card );

	if( !request_irq( card->irq, maestro_interrupt, NULL, SA_SHIRQ, card_names[card_type], card ) )
	{
		kerndbg( KERN_WARNING, "maestro: unable to allocate irq %d,\n", card->irq);
		goto out_delete_area;
	}

	/* Turn on hardware volume control interrupt.
	   This has to come after we grab the IRQ above,
	   or a crash will result on installation if a button has been pressed,
	   because in that case we'll get an immediate interrupt. */
	n = inw( card->iobase+0x18 );
	n|=(1<<6);
	outw(n, card->iobase+0x18 );

	*ppsCard = card;

	return EOK;

out_delete_area:
	delete_area( card->ioarea );
out_free_card:
	kfree( card );
	return -ENODEV;
}

status_t device_init( int nDeviceID )
{
	PCI_Info_s sDevice;
	struct maestro_card *psCard;
	bool bFound = false;
	int i,j;

	g_psBus = get_busmanager( PCI_BUS_NAME, PCI_BUS_VERSION );
	if( NULL == g_psBus )
		return -ENODEV;		/* No PCI bus means no PCI devices */

	/* Check each PCI device for ESS */
	for( i = 0; g_psBus->get_pci_info( &sDevice, i ) == 0; ++i )
	{
		if( sDevice.nVendorID == PCI_VENDOR_ID_ESS || sDevice.nVendorID == PCI_VENDOR_ESS_OLD )
		{
			/* See if this is a supported device */
			for( j = 0; j < 3; ++j )
			{
				if( sDevice.nDeviceID == devices[j] )
				{
					/* We have an ESS Maestro device */
					if( maestro_init_one( &sDevice, &psCard, j ) < 0 )
						break;

					/* Claim our now functional device from the PCI bus manager */
					if( claim_device( nDeviceID, sDevice.nHandle, card_names[j], DEVICE_AUDIO ) < 0 )
						break;

					/* Create device nodes */
					if( create_device_node( nDeviceID, sDevice.nHandle, "audio/maestro", &maestro_dsp_fops, (void*)psCard ) < 0 )
					{
						kerndbg( KERN_WARNING, "Failed to create device node audio/maestro\n" );
						break;
					}

					if( create_device_node( nDeviceID, sDevice.nHandle, "audio/mixer/maestro", &maestro_mixer_fops, (void*)psCard ) < 0 )
					{
						kerndbg( KERN_WARNING, "Failed to create device node audio/mixer/maestro\n" );
						break;
					}

					/* Log what we've got */
					kerndbg( KERN_INFO, "%s found at 0x%4lx\n", card_names[j], sDevice.u.h0.nBase0 & PCI_ADDRESS_MEMORY_32_MASK );
					bFound = true;
					break;
				}
			}
		}
	}

	if( false == bFound )
	{
		disable_device( nDeviceID );
		return ENODEV;
	}

	return 0;
}

