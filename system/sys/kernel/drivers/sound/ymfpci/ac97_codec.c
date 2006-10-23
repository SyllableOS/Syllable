/*
 *  Syllable generic AC97 mixer/modem module
 *
 *  Syllable specific code is
 *  Copyright (c) 2001 Arno Klenke
 *  Notes: extended to work with Linux 2.4 based i810 audio driver
 * 
 *  Other code is 
 *  Copyright (c) Silicon Integrated System Corporation
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
 **************************************************************************
 *
 * The Intel Audio Codec '97 specification is available at the Intel
 * audio homepage: http://developer.intel.com/ial/scalableplatforms/audio/
 *
 * The specification itself is currently available at:
 * ftp://download.intel.com/ial/scalableplatforms/ac97r22.pdf
 *
 **************************************************************************
 *
 * 
 */

#include <posix/errno.h>
#include <posix/ioctls.h>
#include <posix/fcntl.h>
#include <posix/termios.h>
#include <posix/signal.h>

#include <atheos/kernel.h>
#include <atheos/kdebug.h>
#include <atheos/areas.h>
#include <atheos/types.h>
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
#include <atheos/bitops.h>
#define NO_DEBUG_STUBS 1
#include <atheos/linux_compat.h>
#include <macros.h>

#include "ac97_codec.h"

#undef DEBUG

static int ac97_read_mixer(struct ac97_codec *codec, int oss_channel);
static void ac97_write_mixer(struct ac97_codec *codec, int oss_channel, 
			     unsigned int left, unsigned int right);
static void ac97_set_mixer(struct ac97_codec *codec, unsigned int oss_mixer, unsigned int val );
static int ac97_recmask_io(struct ac97_codec *codec, int rw, int mask);
static int ac97_mixer_ioctl(struct ac97_codec *codec, unsigned int cmd, unsigned long arg);

static int ac97_init_mixer(struct ac97_codec *codec);

static int wolfson_init(struct ac97_codec * codec);
static int tritech_init(struct ac97_codec * codec);
static int tritech_maestro_init(struct ac97_codec * codec);
static int sigmatel_9708_init(struct ac97_codec *codec);
static int sigmatel_9721_init(struct ac97_codec *codec);
static int sigmatel_9744_init(struct ac97_codec *codec);
static int enable_eapd(struct ac97_codec *codec);


#define arraysize(x)   (sizeof(x)/sizeof((x)[0]))

/* sorted by vendor/device id */
static const struct {
	u32 id;
	char *name;
	int  (*init)  (struct ac97_codec *codec);
} ac97_codec_ids[] = {
	{0x41445303, "Analog Devices AD1819",	NULL},
	{0x41445340, "Analog Devices AD1881",	NULL},
	{0x41445348, "Analog Devices AD1881A",	NULL},
	{0x41445460, "Analog Devices AD1885",	enable_eapd},
	{0x414B4D00, "Asahi Kasei AK4540",	NULL},
	{0x414B4D01, "Asahi Kasei AK4542",	NULL},
	{0x414B4D02, "Asahi Kasei AK4543",	NULL},
	{0x414C4710, "ALC200/200P",		NULL},
	{0x414C4720, "ALC650",			NULL},
	{0x43525900, "Cirrus Logic CS4297",	enable_eapd},
	{0x43525903, "Cirrus Logic CS4297",	enable_eapd},
	{0x43525913, "Cirrus Logic CS4297A rev A", enable_eapd},
	{0x43525914, "Cirrus Logic CS4297A rev B", NULL},
	{0x43525923, "Cirrus Logic CS4298",	NULL},
	{0x4352592B, "Cirrus Logic CS4294",	NULL},
	{0x4352592D, "Cirrus Logic CS4294",	NULL},
	{0x43525931, "Cirrus Logic CS4299 rev A", NULL},
	{0x43525933, "Cirrus Logic CS4299 rev C", NULL},
	{0x43525934, "Cirrus Logic CS4299 rev D", NULL},
	{0x45838308, "ESS Allegro ES1988",	NULL},
//	{0x49434511, "ICE1232",			NULL}, /* I hope --jk */
	{0x49434511, "Via AC97",			NULL},
	{0x4e534331, "National Semiconductor LM4549", NULL},
	{0x53494c22, "Silicon Laboratory Si3036", NULL},
	{0x53494c23, "Silicon Laboratory Si3038", NULL},
	{0x545200FF, "TriTech TR?????",		tritech_maestro_init},
	{0x54524102, "TriTech TR28022",		NULL},
	{0x54524103, "TriTech TR28023",		NULL},
	{0x54524106, "TriTech TR28026",		NULL},
	{0x54524108, "TriTech TR28028",		tritech_init},
	{0x54524123, "TriTech TR?????",		NULL},
	{0x56494161, "VIA vt82xx AC97",			NULL},
	{0x574D4C00, "Wolfson WM9704",		wolfson_init},
	{0x574D4C03, "Wolfson WM9703/9704",	wolfson_init},
	{0x574D4C04, "Wolfson WM9704 (quad)",	wolfson_init},
	{0x83847600, "SigmaTel STAC????",	NULL},
	{0x83847604, "SigmaTel STAC9701/3/4/5", NULL},
	{0x83847605, "SigmaTel STAC9704",	NULL},
	{0x83847608, "SigmaTel STAC9708",	sigmatel_9708_init},
	{0x83847609, "SigmaTel STAC9721/23",	sigmatel_9721_init},
	{0x83847644, "SigmaTel STAC9744/45",	sigmatel_9744_init},
	{0x83847656, "SigmaTel STAC9756/57",	sigmatel_9744_init},
	{0x83847684, "SigmaTel STAC9783/84?",	NULL},
	{0,}
};
#if 0
static const char *ac97_stereo_enhancements[] =
{
	/*   0 */ "No 3D Stereo Enhancement",
	/*   1 */ "Analog Devices Phat Stereo",
	/*   2 */ "Creative Stereo Enhancement",
	/*   3 */ "National Semi 3D Stereo Enhancement",
	/*   4 */ "YAMAHA Ymersion",
	/*   5 */ "BBE 3D Stereo Enhancement",
	/*   6 */ "Crystal Semi 3D Stereo Enhancement",
	/*   7 */ "Qsound QXpander",
	/*   8 */ "Spatializer 3D Stereo Enhancement",
	/*   9 */ "SRS 3D Stereo Enhancement",
	/*  10 */ "Platform Tech 3D Stereo Enhancement",
	/*  11 */ "AKM 3D Audio",
	/*  12 */ "Aureal Stereo Enhancement",
	/*  13 */ "Aztech 3D Enhancement",
	/*  14 */ "Binaura 3D Audio Enhancement",
	/*  15 */ "ESS Technology Stereo Enhancement",
	/*  16 */ "Harman International VMAx",
	/*  17 */ "Nvidea 3D Stereo Enhancement",
	/*  18 */ "Philips Incredible Sound",
	/*  19 */ "Texas Instruments 3D Stereo Enhancement",
	/*  20 */ "VLSI Technology 3D Stereo Enhancement",
	/*  21 */ "TriTech 3D Stereo Enhancement",
	/*  22 */ "Realtek 3D Stereo Enhancement",
	/*  23 */ "Samsung 3D Stereo Enhancement",
	/*  24 */ "Wolfson Microelectronics 3D Enhancement",
	/*  25 */ "Delta Integration 3D Enhancement",
	/*  26 */ "SigmaTel 3D Enhancement",
	/*  27 */ "Reserved 27",
	/*  28 */ "Rockwell 3D Stereo Enhancement",
	/*  29 */ "Reserved 29",
	/*  30 */ "Reserved 30",
	/*  31 */ "Reserved 31"
};
#endif
/* this table has default mixer values for all OSS mixers. */
struct mixer_defaults_struct {
	int mixer;
	unsigned int value;
};


static struct mixer_defaults_struct mixer_defaults[SOUND_MIXER_NRDEVICES] = {
	/* all values 0 -> 100 in bytes */
	{SOUND_MIXER_VOLUME,	0x4343},
	{SOUND_MIXER_BASS,	0x4343},
	{SOUND_MIXER_TREBLE,	0x4343},
	{SOUND_MIXER_PCM,	0x4343},
	{SOUND_MIXER_SPEAKER,	0x4343},
	{SOUND_MIXER_LINE,	0x4343},
	{SOUND_MIXER_MIC,	0x1111},	/* P3 - audio loop */
	{SOUND_MIXER_CD,	0x4343},
	{SOUND_MIXER_ALTPCM,	0x4343},
	{SOUND_MIXER_IGAIN,	0x4343},
	{SOUND_MIXER_LINE1,	0x4343},
	{SOUND_MIXER_PHONEIN,	0x4343},
	{SOUND_MIXER_PHONEOUT,	0x4343},
	{SOUND_MIXER_VIDEO,	0x4343},
	{-1,0}
};

/* table to scale scale from OSS mixer value to AC97 mixer register value */	
struct ac97_mixer_hw {
	unsigned char offset;
	int scale;
};


static struct ac97_mixer_hw ac97_hw[SOUND_MIXER_NRDEVICES]= {
	[SOUND_MIXER_VOLUME]	=	{AC97_MASTER_VOL_STEREO,64},
	[SOUND_MIXER_BASS]	=	{AC97_MASTER_TONE,	16},
	[SOUND_MIXER_TREBLE]	=	{AC97_MASTER_TONE,	16},
	[SOUND_MIXER_PCM]	=	{AC97_PCMOUT_VOL,	32},
	[SOUND_MIXER_SPEAKER]	=	{AC97_PCBEEP_VOL,	16},
	[SOUND_MIXER_LINE]	=	{AC97_LINEIN_VOL,	32},
	[SOUND_MIXER_MIC]	=	{AC97_MIC_VOL,		32},
	[SOUND_MIXER_CD]	=	{AC97_CD_VOL,		32},
	[SOUND_MIXER_ALTPCM]	=	{AC97_HEADPHONE_VOL,	64},
	[SOUND_MIXER_IGAIN]	=	{AC97_RECORD_GAIN,	16},
	[SOUND_MIXER_LINE1]	=	{AC97_AUX_VOL,		32},
	[SOUND_MIXER_PHONEIN]	= 	{AC97_PHONE_VOL,	32},
	[SOUND_MIXER_PHONEOUT]	= 	{AC97_MASTER_VOL_MONO,	64},
	[SOUND_MIXER_VIDEO]	=	{AC97_VIDEO_VOL,	32},
};

/* the following tables allow us to go from OSS <-> ac97 quickly. */
enum ac97_recsettings {
	AC97_REC_MIC=0,
	AC97_REC_CD,
	AC97_REC_VIDEO,
	AC97_REC_AUX,
	AC97_REC_LINE,
	AC97_REC_STEREO, /* combination of all enabled outputs..  */
	AC97_REC_MONO,	      /*.. or the mono equivalent */
	AC97_REC_PHONE	      
};

static unsigned int ac97_rm2oss[] = {
	[AC97_REC_MIC] 	 = SOUND_MIXER_MIC,
	[AC97_REC_CD] 	 = SOUND_MIXER_CD,
	[AC97_REC_VIDEO] = SOUND_MIXER_VIDEO,
	[AC97_REC_AUX] 	 = SOUND_MIXER_LINE1,
	[AC97_REC_LINE]  = SOUND_MIXER_LINE,
	[AC97_REC_STEREO]= SOUND_MIXER_IGAIN,
	[AC97_REC_PHONE] = SOUND_MIXER_PHONEIN
};

/* indexed by bit position */
static unsigned int ac97_oss_rm[] = {
	[SOUND_MIXER_MIC] 	= AC97_REC_MIC,
	[SOUND_MIXER_CD] 	= AC97_REC_CD,
	[SOUND_MIXER_VIDEO] 	= AC97_REC_VIDEO,
	[SOUND_MIXER_LINE1] 	= AC97_REC_AUX,
	[SOUND_MIXER_LINE] 	= AC97_REC_LINE,
	[SOUND_MIXER_IGAIN]	= AC97_REC_STEREO,
	[SOUND_MIXER_PHONEIN] 	= AC97_REC_PHONE
};

/* reads the given OSS mixer from the ac97 the caller must have insured that the ac97 knows
   about that given mixer, and should be holding a spinlock for the card */
static int ac97_read_mixer(struct ac97_codec *codec, int oss_channel) 
{
	u16 val;
	int ret = 0;
	int scale;
	struct ac97_mixer_hw *mh = &ac97_hw[oss_channel];

	val = codec->codec_read(codec , mh->offset);

	if (val & AC97_MUTE) {
		ret = 0;
	} else if (AC97_STEREO_MASK & (1 << oss_channel)) {
		/* nice stereo mixers .. */
		int left,right;

		left = (val >> 8)  & 0x7f;
		right = val  & 0x7f;

		if (oss_channel == SOUND_MIXER_IGAIN) {
			right = (right * 100) / mh->scale;
			left = (left * 100) / mh->scale;
		} else {
			/* these may have 5 or 6 bit resolution */
			if(oss_channel == SOUND_MIXER_VOLUME || oss_channel == SOUND_MIXER_ALTPCM)
				scale = (1 << codec->bit_resolution);
			else
				scale = mh->scale;

			right = 100 - ((right * 100) / scale);
			left = 100 - ((left * 100) / scale);
		}
		ret = left | (right << 8);
	} else if (oss_channel == SOUND_MIXER_SPEAKER) {
		ret = 100 - ((((val & 0x1e)>>1) * 100) / mh->scale);
	} else if (oss_channel == SOUND_MIXER_PHONEIN) {
		ret = 100 - (((val & 0x1f) * 100) / mh->scale);
	} else if (oss_channel == SOUND_MIXER_PHONEOUT) {
		scale = (1 << codec->bit_resolution);
		ret = 100 - (((val & 0x1f) * 100) / scale);
	} else if (oss_channel == SOUND_MIXER_MIC) {
		ret = 100 - (((val & 0x1f) * 100) / mh->scale);
		/*  the low bit is optional in the tone sliders and masking
		    it lets us avoid the 0xf 'bypass'.. */
	} else if (oss_channel == SOUND_MIXER_BASS) {
		ret = 100 - ((((val >> 8) & 0xe) * 100) / mh->scale);
	} else if (oss_channel == SOUND_MIXER_TREBLE) {
		ret = 100 - (((val & 0xe) * 100) / mh->scale);
	}

#ifdef DEBUG
	printk("ac97_codec: read OSS mixer %2d (%s ac97 register 0x%02x), "
	       "0x%04x -> 0x%04x\n",
	       oss_channel, codec->id ? "Secondary" : "Primary",
	       mh->offset, val, ret);
#endif

	return ret;
}

/* write the OSS encoded volume to the given OSS encoded mixer, again caller's job to
   make sure all is well in arg land, call with spinlock held */
static void ac97_write_mixer(struct ac97_codec *codec, int oss_channel,
		      unsigned int left, unsigned int right)
{
	u16 val = 0;
	int scale;
	struct ac97_mixer_hw *mh = &ac97_hw[oss_channel];

#ifdef DEBUG
	printk("ac97_codec: wrote OSS mixer %2d (%s ac97 register 0x%02x), "
	       "left vol:%2d, right vol:%2d:",
	       oss_channel, codec->id ? "Secondary" : "Primary",
	       mh->offset, left, right);
#endif

	if (AC97_STEREO_MASK & (1 << oss_channel)) {
		/* stereo mixers */
		if (left == 0 && right == 0) {
			val = AC97_MUTE;
		} else {
			if (oss_channel == SOUND_MIXER_IGAIN) {
				right = (right * mh->scale) / 100;
				left = (left * mh->scale) / 100;
				if (right >= mh->scale)
					right = mh->scale-1;
				if (left >= mh->scale)
					left = mh->scale-1;
			} else {
				/* these may have 5 or 6 bit resolution */
				if (oss_channel == SOUND_MIXER_VOLUME ||
				    oss_channel == SOUND_MIXER_ALTPCM)
					scale = (1 << codec->bit_resolution);
				else
					scale = mh->scale;

				right = ((100 - right) * scale) / 100;
				left = ((100 - left) * scale) / 100;
				if (right >= scale)
					right = scale-1;
				if (left >= scale)
					left = scale-1;
			}
			val = (left << 8) | right;
		}
	} else if (oss_channel == SOUND_MIXER_BASS) {
		val = codec->codec_read(codec , mh->offset) & ~0x0f00;
		left = ((100 - left) * mh->scale) / 100;
		if (left >= mh->scale)
			left = mh->scale-1;
		val |= (left << 8) & 0x0e00;
	} else if (oss_channel == SOUND_MIXER_TREBLE) {
		val = codec->codec_read(codec , mh->offset) & ~0x000f;
		left = ((100 - left) * mh->scale) / 100;
		if (left >= mh->scale)
			left = mh->scale-1;
		val |= left & 0x000e;
	} else if(left == 0) {
		val = AC97_MUTE;
	} else if (oss_channel == SOUND_MIXER_SPEAKER) {
		left = ((100 - left) * mh->scale) / 100;
		if (left >= mh->scale)
			left = mh->scale-1;
		val = left << 1;
	} else if (oss_channel == SOUND_MIXER_PHONEIN) {
		left = ((100 - left) * mh->scale) / 100;
		if (left >= mh->scale)
			left = mh->scale-1;
		val = left;
	} else if (oss_channel == SOUND_MIXER_PHONEOUT) {
		scale = (1 << codec->bit_resolution);
		left = ((100 - left) * scale) / 100;
		if (left >= mh->scale)
			left = mh->scale-1;
		val = left;
	} else if (oss_channel == SOUND_MIXER_MIC) {
		val = codec->codec_read(codec , mh->offset) & ~0x801f;
		left = ((100 - left) * mh->scale) / 100;
		if (left >= mh->scale)
			left = mh->scale-1;
		val |= left;
		/*  the low bit is optional in the tone sliders and masking
		    it lets us avoid the 0xf 'bypass'.. */
	}
#ifdef DEBUG
	printk(" 0x%04x", val);
#endif

	codec->codec_write(codec, mh->offset, val);

#ifdef DEBUG
	val = codec->codec_read(codec, mh->offset);
	printk(" -> 0x%04x\n", val);
#endif
}

/* a thin wrapper for write_mixer */
static void ac97_set_mixer(struct ac97_codec *codec, unsigned int oss_mixer, unsigned int val ) 
{
	unsigned int left,right;

	/* cleanse input a little */
	right = ((val >> 8)  & 0xff) ;
	left = (val  & 0xff) ;

	if (right > 100) right = 100;
	if (left > 100) left = 100;

	codec->mixer_state[oss_mixer] = (right << 8) | left;
	codec->write_mixer(codec, oss_mixer, left, right);
}

/* read or write the recmask, the ac97 can really have left and right recording
   inputs independantly set, but OSS doesn't seem to want us to express that to
   the user. the caller guarantees that we have a supported bit set, and they
   must be holding the card's spinlock */
static int ac97_recmask_io(struct ac97_codec *codec, int rw, int mask) 
{
	unsigned int val;

	if (rw) {
		/* read it from the card */
		val = codec->codec_read(codec, AC97_RECORD_SELECT);
#ifdef DEBUG
		printk("ac97_codec: ac97 recmask to set to 0x%04x\n", val);
#endif
		return (1 << ac97_rm2oss[val & 0x07]);
	}

	/* else, write the first set in the mask as the
	   output */	
	/* clear out current set value first (AC97 supports only 1 input!) */
	val = (1 << ac97_rm2oss[codec->codec_read(codec, AC97_RECORD_SELECT) & 0x07]);
	if (mask != val)
	    mask &= ~val;
       
	val = ffs(mask); 
	val = ac97_oss_rm[val-1];
	val |= val << 8;  /* set both channels */

#ifdef DEBUG
	printk("ac97_codec: setting ac97 recmask to 0x%04x\n", val);
#endif

	codec->codec_write(codec, AC97_RECORD_SELECT, val);

	return 0;
};

static status_t ac97_mixer_ioctl(struct ac97_codec *codec, unsigned int cmd, unsigned long arg)
{
	int i, val = 0;

	if (cmd == SOUND_MIXER_INFO) {
		mixer_info info;
		strncpy(info.id, codec->name, sizeof(info.id));
		strncpy(info.name, codec->name, sizeof(info.name));
		info.modify_counter = codec->modcnt;
		if (copy_to_user((void *)arg, &info, sizeof(info)))
			return -EFAULT;
		return 0;
	}
	if (cmd == SOUND_OLD_MIXER_INFO) {
		_old_mixer_info info;
		strncpy(info.id, codec->name, sizeof(info.id));
		strncpy(info.name, codec->name, sizeof(info.name));
		if (copy_to_user((void *)arg, &info, sizeof(info)))
			return -EFAULT;
		return 0;
	}

	if (cmd == OSS_GETVERSION) {
		
	   put_user(SOUND_VERSION, (int *)arg);
	   return 0;
	}
	if (_SIOC_DIR(cmd) == _SIOC_READ) {
		switch (_IOC_NR(cmd)) {
		case SOUND_MIXER_RECSRC: /* give them the current record source */
			if (!codec->recmask_io) {
				val = 0;
			} else {
				val = codec->recmask_io(codec, 1, 0);
			}
			break;

		case SOUND_MIXER_DEVMASK: /* give them the supported mixers */
			val = codec->supported_mixers;
			break;

		case SOUND_MIXER_RECMASK: /* Arg contains a bit for each supported recording source */
			val = codec->record_sources;
			break;

		case SOUND_MIXER_STEREODEVS: /* Mixer channels supporting stereo */
			val = codec->stereo_mixers;
			break;

		case SOUND_MIXER_CAPS:
			val = SOUND_CAP_EXCL_INPUT;
			break;

		default: /* read a specific mixer */
			i = _IOC_NR(cmd);

			if (!supported_mixer(codec, i)) 
				return -EINVAL;

			/* do we ever want to touch the hardware? */
		        /* val = codec->read_mixer(codec, i); */
			val = codec->mixer_state[i];
 			break;
		}
		put_user(val, (int *)arg);
	        return 0;
	}

	if (_SIOC_DIR(cmd) == (_SIOC_WRITE|_SIOC_READ)) {
		codec->modcnt++;
		get_user(val, (int *)arg);

		switch (_IOC_NR(cmd)) {
		case SOUND_MIXER_RECSRC: /* Arg contains a bit for each recording source */
			if (!codec->recmask_io) return -EINVAL;
			if (!val) return 0;
			if (!(val &= codec->record_sources)) return -EINVAL;

			codec->recmask_io(codec, 0, val);

			return 0;
		default: /* write a specific mixer */
			i = _IOC_NR(cmd);

			if (!supported_mixer(codec, i)) 
				return -EINVAL;

			ac97_set_mixer(codec, i, val);

			return 0;
		}
	}
	return -EINVAL;
}

/**
 *	ac97_probe_codec - Initialize and setup AC97-compatible codec
 *	@codec: (in/out) Kernel info for a single AC97 codec
 *
 *	Reset the AC97 codec, then initialize the mixer and
 *	the rest of the @codec structure.
 *
 *	The codec_read and codec_write fields of @codec are
 *	required to be setup and working when this function
 *	is called.  All other fields are set by this function.
 *
 *	codec_wait field of @codec can optionally be provided
 *	when calling this function.  If codec_wait is not %NULL,
 *	this function will call codec_wait any time it is
 *	necessary to wait for the audio chip to reach the
 *	codec-ready state.  If codec_wait is %NULL, then
 *	the default behavior is to call schedule_timeout.
 *	Currently codec_wait is used to wait for AC97 codec
 *	reset to complete. 
 *
 *	Returns 1 (true) on success, or 0 (false) on failure.
 */
 

int ac97_probe_codec(struct ac97_codec *codec)
{
	u16 id1, id2;
	u16 audio, modem;
	int i;

	/* probing AC97 codec, AC97 2.0 says that bit 15 of register 0x00 (reset) should 
	   be read zero. Probing of AC97 in this way is not reliable, it is not even SAFE !! */
	codec->codec_write(codec, AC97_RESET, 0L);
	
	/* also according to spec, we wait for codec-ready state */	
	if (codec->codec_wait)
		codec->codec_wait(codec);
	else
		udelay(10);
	
	if ((audio = codec->codec_read(codec, AC97_RESET)) & 0x8000) {
		kerndbg(KERN_WARNING, "ac97_codec: %s ac97 codec not present\n",
		       codec->id ? "Secondary" : "Primary");
		return 0;
	}

	/* probe for Modem Codec */
	codec->codec_write(codec, AC97_EXTENDED_MODEM_ID, 0L);
	modem = codec->codec_read(codec, AC97_EXTENDED_MODEM_ID);

	codec->name = NULL;
	codec->codec_init = NULL;

	id1 = codec->codec_read(codec, AC97_VENDOR_ID1);
	id2 = codec->codec_read(codec, AC97_VENDOR_ID2);
	for (i = 0; i < arraysize(ac97_codec_ids); i++) {
		if (ac97_codec_ids[i].id == ((id1 << 16) | id2)) {
			codec->type = ac97_codec_ids[i].id;
			codec->name = ac97_codec_ids[i].name;
			codec->codec_init = ac97_codec_ids[i].init;
			break;
		}
	}
	if (codec->name == NULL)
		codec->name = "Unknown";
	kerndbg(KERN_INFO, "ac97_codec: AC97 %s codec, vendor id1: 0x%04x, "
	       "id2: 0x%04x (%s)\n", audio ? "Audio" : (modem ? "Modem" : ""),
	       id1, id2, codec->name);

	return ac97_init_mixer(codec);
}

static int ac97_init_mixer(struct ac97_codec *codec)
{
	u16 cap;
	int i;

	cap = codec->codec_read(codec, AC97_RESET);

	/* mixer masks */
	codec->supported_mixers = AC97_SUPPORTED_MASK;
	codec->stereo_mixers = AC97_STEREO_MASK;
	codec->record_sources = AC97_RECORD_MASK;
	if (!(cap & 0x04))
		codec->supported_mixers &= ~(SOUND_MASK_BASS|SOUND_MASK_TREBLE);
	if (!(cap & 0x10))
		codec->supported_mixers &= ~SOUND_MASK_ALTPCM;

	/* detect bit resolution */
	codec->codec_write(codec, AC97_MASTER_VOL_STEREO, 0x2020);
	if(codec->codec_read(codec, AC97_MASTER_VOL_STEREO) == 0x1f1f)
		codec->bit_resolution = 5;
	else
		codec->bit_resolution = 6;

	/* generic OSS to AC97 wrapper */
	codec->read_mixer = ac97_read_mixer;
	codec->write_mixer = ac97_write_mixer;
	codec->recmask_io = ac97_recmask_io;
	codec->mixer_ioctl = ac97_mixer_ioctl;

	/* codec specific initialization for 4-6 channel output or secondary codec stuff */
	if (codec->codec_init != NULL) {
		codec->codec_init(codec);
	}

	/* initilize mixer channel volumes */
	for (i = 0; i < SOUND_MIXER_NRDEVICES; i++) {
		struct mixer_defaults_struct *md = &mixer_defaults[i];
		if (md->mixer == -1) 
			break;
		if (!supported_mixer(codec, md->mixer)) 
			continue;
		ac97_set_mixer(codec, md->mixer, md->value);
	}

	return 1;
}

#define AC97_SIGMATEL_ANALOG    0x6c	/* Analog Special */
#define AC97_SIGMATEL_DAC2INVERT 0x6e
#define AC97_SIGMATEL_BIAS1     0x70
#define AC97_SIGMATEL_BIAS2     0x72
#define AC97_SIGMATEL_MULTICHN  0x74	/* Multi-Channel programming */
#define AC97_SIGMATEL_CIC1      0x76
#define AC97_SIGMATEL_CIC2      0x78


static int sigmatel_9708_init(struct ac97_codec * codec)
{
	u16 codec72, codec6c;

	codec72 = codec->codec_read(codec, AC97_SIGMATEL_BIAS2) & 0x8000;
	codec6c = codec->codec_read(codec, AC97_SIGMATEL_ANALOG);

	if ((codec72==0) && (codec6c==0)) {
		codec->codec_write(codec, AC97_SIGMATEL_CIC1, 0xabba);
		codec->codec_write(codec, AC97_SIGMATEL_CIC2, 0x1000);
		codec->codec_write(codec, AC97_SIGMATEL_BIAS1, 0xabba);
		codec->codec_write(codec, AC97_SIGMATEL_BIAS2, 0x0007);
	} else if ((codec72==0x8000) && (codec6c==0)) {
		codec->codec_write(codec, AC97_SIGMATEL_CIC1, 0xabba);
		codec->codec_write(codec, AC97_SIGMATEL_CIC2, 0x1001);
		codec->codec_write(codec, AC97_SIGMATEL_DAC2INVERT, 0x0008);
	} else if ((codec72==0x8000) && (codec6c==0x0080)) {
		/* nothing */
	}
	codec->codec_write(codec, AC97_SIGMATEL_MULTICHN, 0x0000);
	return 0;
}


static int sigmatel_9721_init(struct ac97_codec * codec)
{
	/* Only set up secondary codec */
	if (codec->id == 0)
		return 0;

	codec->codec_write(codec, AC97_SURROUND_MASTER, 0L);

	/* initialize SigmaTel STAC9721/23 as secondary codec, decoding AC link
	   sloc 3,4 = 0x01, slot 7,8 = 0x00, */
	codec->codec_write(codec, AC97_SIGMATEL_MULTICHN, 0x00);

	/* we don't have the crystal when we are on an AMR card, so use
	   BIT_CLK as our clock source. Write the magic word ABBA and read
	   back to enable register 0x78 */
	codec->codec_write(codec, AC97_SIGMATEL_CIC1, 0xabba);
	codec->codec_read(codec, AC97_SIGMATEL_CIC1);

	/* sync all the clocks*/
	codec->codec_write(codec, AC97_SIGMATEL_CIC2, 0x3802);

	return 0;
}


static int sigmatel_9744_init(struct ac97_codec * codec)
{
	// patch for SigmaTel
	codec->codec_write(codec, AC97_SIGMATEL_CIC1, 0xabba);
	codec->codec_write(codec, AC97_SIGMATEL_CIC2, 0x0000); // is this correct? --jk
	codec->codec_write(codec, AC97_SIGMATEL_BIAS1, 0xabba);
	codec->codec_write(codec, AC97_SIGMATEL_BIAS2, 0x0002);
	codec->codec_write(codec, AC97_SIGMATEL_MULTICHN, 0x0000);
	return 0;
}


static int wolfson_init(struct ac97_codec * codec)
{
	codec->codec_write(codec, 0x72, 0x0808);
	codec->codec_write(codec, 0x74, 0x0808);

	// patch for DVD noise
	codec->codec_write(codec, 0x5a, 0x0200);

	// init vol as PCM vol
	codec->codec_write(codec, 0x70,
		codec->codec_read(codec, AC97_PCMOUT_VOL));

	codec->codec_write(codec, AC97_SURROUND_MASTER, 0x0000);
	return 0;
}


static int tritech_init(struct ac97_codec * codec)
{
	codec->codec_write(codec, 0x26, 0x0300);
	codec->codec_write(codec, 0x26, 0x0000);
	codec->codec_write(codec, AC97_SURROUND_MASTER, 0x0000);
	codec->codec_write(codec, AC97_RESERVED_3A, 0x0000);
	return 0;
}


/* copied from drivers/sound/maestro.c */
static int tritech_maestro_init(struct ac97_codec * codec)
{
	/* no idea what this does */
	codec->codec_write(codec, 0x2A, 0x0001);
	codec->codec_write(codec, 0x2C, 0x0000);
	codec->codec_write(codec, 0x2C, 0XFFFF);
	return 0;
}


/*
 *	External AMP management for EAPD using codecs
 *	(CS4279A, AD1885, ...)
 */

static int enable_eapd(struct ac97_codec * codec)
{
	codec->codec_write(codec, AC97_POWER_CONTROL,
		codec->codec_read(codec, AC97_POWER_CONTROL)|0x8000);
	return 0;
}


/* copied from drivers/sound/maestro.c */
#if 0  /* there has been 1 person on the planet with a pt101 that we
        know of.  If they care, they can put this back in :) */
static int pt101_init(struct ac97_codec * codec)
{
	printk(KERN_INFO "ac97_codec: PT101 Codec detected, initializing but _not_ installing mixer device.\n");
	/* who knows.. */
	codec->codec_write(codec, 0x2A, 0x0001);
	codec->codec_write(codec, 0x2C, 0x0000);
	codec->codec_write(codec, 0x2C, 0xFFFF);
	codec->codec_write(codec, 0x10, 0x9F1F);
	codec->codec_write(codec, 0x12, 0x0808);
	codec->codec_write(codec, 0x14, 0x9F1F);
	codec->codec_write(codec, 0x16, 0x9F1F);
	codec->codec_write(codec, 0x18, 0x0404);
	codec->codec_write(codec, 0x1A, 0x0000);
	codec->codec_write(codec, 0x1C, 0x0000);
	codec->codec_write(codec, 0x02, 0x0404);
	codec->codec_write(codec, 0x04, 0x0808);
	codec->codec_write(codec, 0x0C, 0x801F);
	codec->codec_write(codec, 0x0E, 0x801F);
	return 0;
}
#endif
	
#if 0
static int sigmatel_init(struct ac97_codec * codec)
{
	if(codec->id == 0)
		return 0;
		
	/* Only set up secondary codec */
	
	codec->codec_write(codec, AC97_SURROUND_MASTER, 0L);

	/* initialize SigmaTel STAC9721/23 as secondary codec, decoding AC link
	   sloc 3,4 = 0x01, slot 7,8 = 0x00, */
	codec->codec_write(codec, 0x74, 0x00);

	/* we don't have the crystal when we are on an AMR card, so use
	   BIT_CLK as our clock source. Write the magic word ABBA and read
	   back to enable register 0x78 */
	codec->codec_write(codec, 0x76, 0xabba);
	codec->codec_read(codec, 0x76);

	/* sync all the clocks*/
	codec->codec_write(codec, 0x78, 0x3802);

	return 1;
}
#endif


/*
 *	AC97 library support routines
 */	
 
/**
 *	ac97_set_dac_rate	-	set codec rate adaption
 *	@codec: ac97 code
 *	@rate: rate in hertz
 *
 *	Set the DAC rate. Assumes the codec supports VRA. The caller is
 *	expected to have checked this little detail.
 */
 
unsigned int ac97_set_dac_rate(struct ac97_codec *codec, unsigned int rate)
{
	unsigned int new_rate = rate;
	u32 dacp;
	u32 mast_vol, phone_vol, mono_vol, pcm_vol;
	u32 mute_vol = 0x8000;	/* The mute volume? */

	if(rate != codec->codec_read(codec, AC97_PCM_FRONT_DAC_RATE))
	{
		/* Mute several registers */
		mast_vol = codec->codec_read(codec, AC97_MASTER_VOL_STEREO);
		mono_vol = codec->codec_read(codec, AC97_MASTER_VOL_MONO);
		phone_vol = codec->codec_read(codec, AC97_HEADPHONE_VOL);
		pcm_vol = codec->codec_read(codec, AC97_PCMOUT_VOL);
		codec->codec_write(codec, AC97_MASTER_VOL_STEREO, mute_vol);
		codec->codec_write(codec, AC97_MASTER_VOL_MONO, mute_vol);
		codec->codec_write(codec, AC97_HEADPHONE_VOL, mute_vol);
		codec->codec_write(codec, AC97_PCMOUT_VOL, mute_vol);
		
		/* Power down the DAC */
		dacp=codec->codec_read(codec, AC97_POWER_CONTROL);
		codec->codec_write(codec, AC97_POWER_CONTROL, dacp|0x0200);
		/* Load the rate and read the effective rate */
		codec->codec_write(codec, AC97_PCM_FRONT_DAC_RATE, rate);
		new_rate=codec->codec_read(codec, AC97_PCM_FRONT_DAC_RATE);
		/* Power it back up */
		codec->codec_write(codec, AC97_POWER_CONTROL, dacp);

		/* Restore volumes */
		codec->codec_write(codec, AC97_MASTER_VOL_STEREO, mast_vol);
		codec->codec_write(codec, AC97_MASTER_VOL_MONO, mono_vol);
		codec->codec_write(codec, AC97_HEADPHONE_VOL, phone_vol);
		codec->codec_write(codec, AC97_PCMOUT_VOL, pcm_vol);
	}
	return new_rate;
}

//EXPORT_SYMBOL(ac97_set_dac_rate);

/**
 *	ac97_set_adc_rate	-	set codec rate adaption
 *	@codec: ac97 code
 *	@rate: rate in hertz
 *
 *	Set the ADC rate. Assumes the codec supports VRA. The caller is
 *	expected to have checked this little detail.
 */

unsigned int ac97_set_adc_rate(struct ac97_codec *codec, unsigned int rate)
{
	unsigned int new_rate = rate;
	u32 dacp;

	if(rate != codec->codec_read(codec, AC97_PCM_LR_DAC_RATE)) /* ugly hack */
	{
		/* Power down the ADC */
		dacp=codec->codec_read(codec, AC97_POWER_CONTROL);
		codec->codec_write(codec, AC97_POWER_CONTROL, dacp|0x0100);
		/* Load the rate and read the effective rate */
		codec->codec_write(codec, AC97_PCM_LR_DAC_RATE, rate);
		new_rate=codec->codec_read(codec, AC97_PCM_LR_DAC_RATE);
		/* Power it back up */
		codec->codec_write(codec, AC97_POWER_CONTROL, dacp);
	}
	return new_rate;
}

/**
 *	ac97_alloc_codec - Allocate an AC97 codec
 *
 *	Returns a new AC97 codec structure. AC97 codecs may become
 *	refcounted soon so this interface is needed. Returns with
 *	one reference taken.
 */
 
struct ac97_codec *ac97_alloc_codec(void)
{
	struct ac97_codec *codec = kmalloc(sizeof(struct ac97_codec), MEMF_KERNEL);
	if(!codec)
		return NULL;

	memset(codec, 0, sizeof(*codec));
#if 0
	spinlock_init(&codec->lock);
    INIT_LIST_HEAD(&codec->list);
#endif
	return codec;
}

/**
 *	ac97_release_codec -	Release an AC97 codec
 *	@codec: codec to release
 *
 *	Release an allocated AC97 codec. This will be refcounted in
 *	time but for the moment is trivial. Calls the unregister
 *	handler if the codec is now defunct.
 */
 
void ac97_release_codec(struct ac97_codec *codec)
{
#if 0
	/* Remove from the list first, we don't want to be
	   "rediscovered" */
	down(&codec_sem);
	list_del(&codec->list);
	up(&codec_sem);
	/*
	 *	The driver needs to deal with internal
	 *	locking to avoid accidents here. 
	 */
	if(codec->driver)
		codec->driver->remove(codec, codec->driver);
#endif
	kfree(codec);
}

