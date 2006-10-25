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
#include <ymfpci.h>

#include <atheos/kernel.h>
#include <atheos/kdebug.h>
#include <atheos/types.h>
#include <atheos/device.h>
#include <atheos/pci.h>
#include <atheos/soundcard.h>
#include <atheos/time.h>
#include <atheos/udelay.h>
#include <atheos/schedule.h>
#define NO_DEBUG_STUBS 1
#include <atheos/linux_compat.h>
#include <posix/signal.h>
#include <posix/errno.h>

#include "ac97_codec.h"

static int ymfpci_interrupt( int nIRQ, void *pData, SysCallRegs_s *psRegs );
static void ymfpci_aclink_reset( PCI_Info_s *psPCIDevice, PCI_bus_s *psBus );
static void ymfpci_download_image(struct YMFDevice *device);
static void ymf_memload(struct YMFDevice *device);
static void ymfpci_disable_dsp(struct YMFDevice *device);

static struct ymf_state *ymf_state_alloc(struct YMFDevice *device);
static int ymf_capture_alloc(struct YMFDevice *device, int *pbank);
static int ymf_playback_prepare(struct ymf_state *state);
static int ymf_capture_prepare(struct ymf_state *state);
static void ymfpci_hw_start(struct YMFDevice *device);
static void ymfpci_hw_stop(struct YMFDevice *device);
static void ymfpci_voice_free(struct YMFDevice *device, ymfpci_voice_t *pvoice);
static int ymf_playback_trigger(struct YMFDevice *device, struct ymf_pcm *ypcm, int cmd);
static void ymf_capture_trigger(struct YMFDevice *devicet, struct ymf_pcm *ypcm, int cmd);

/*
 *  common I/O routines
 */

static inline uint8 ymfpci_readb(struct YMFDevice *device, uint32 offset)
{
	return (*(volatile uint8*)(device->reg_addr + offset));
}

static inline void ymfpci_writeb(struct YMFDevice *device, uint32 offset, uint8 val)
{
	(*(volatile uint8*)(device->reg_addr + offset)) = val;
}

static inline uint16 ymfpci_readw(struct YMFDevice *device, uint32 offset)
{
	return (*(volatile uint16*)(device->reg_addr + offset));
}

static inline void ymfpci_writew(struct YMFDevice *device, uint32 offset, uint16 val)
{
	(*(volatile uint16*)(device->reg_addr + offset)) = val;
}

static inline uint32 ymfpci_readl(struct YMFDevice *device, uint32 offset)
{
	return (*(volatile uint32*)(device->reg_addr + offset));
}

static inline void ymfpci_writel(struct YMFDevice *device, uint32 offset, uint32 val)
{
	(*(volatile uint32*)(device->reg_addr + offset)) = val;
}

/* Driver routines */
static uint32 ymfpci_calc_delta(uint32 rate)
{
	switch (rate) {
	case 8000:	return 0x02aaab00;
	case 11025:	return 0x03accd00;
	case 16000:	return 0x05555500;
	case 22050:	return 0x07599a00;
	case 32000:	return 0x0aaaab00;
	case 44100:	return 0x0eb33300;
	default:	return ((rate << 16) / 48000) << 12;
	}
}

static uint32 def_rate[8] = {
	100, 2000, 8000, 11025, 16000, 22050, 32000, 48000
};

static uint32 ymfpci_calc_lpfK(uint32 rate)
{
	uint32 i;
	static uint32 val[8] = {
		0x00570000, 0x06AA0000, 0x18B20000, 0x20930000,
		0x2B9A0000, 0x35A10000, 0x3EAA0000, 0x40000000
	};
	
	if (rate == 44100)
		return 0x40000000;	/* FIXME: What's the right value? */
	for (i = 0; i < 8; i++)
		if (rate <= def_rate[i])
			return val[i];
	return val[0];
}

static uint32 ymfpci_calc_lpfQ(uint32 rate)
{
	uint32 i;
	static u32 val[8] = {
		0x35280000, 0x34A70000, 0x32020000, 0x31770000,
		0x31390000, 0x31C90000, 0x33D00000, 0x40000000
	};
	
	if (rate == 44100)
		return 0x370A0000;
	for (i = 0; i < 8; i++)
		if (rate <= def_rate[i])
			return val[i];
	return val[0];
}

static uint32 ymf_calc_lend(uint32 rate)
{
	return (rate * YMF_SAMPF) / 48000;
}

static int ymf_pcm_format_width(int format)
{
	static int mask16 = AFMT_S16_LE|AFMT_S16_BE|AFMT_U16_LE|AFMT_U16_BE;

	if ((format & (format-1)) != 0) {
		kerndbg(KERN_DEBUG,"format 0x%x is not a power of 2\n", format);
		return 8;
	}

	if (format == AFMT_IMA_ADPCM) return 4;
	if ((format & mask16) != 0) return 16;
	return 8;
}

static void ymf_pcm_update_shift(struct ymf_pcm_format *f)
{
	f->shift = 0;
	if (f->voices == 2)
		f->shift++;
	if (ymf_pcm_format_width(f->format) == 16)
		f->shift++;
}

static void ymfpci_aclink_reset(PCI_Info_s *psPCIDevice, PCI_bus_s *psBus)
{
	uint8 cmd;

	/*
	 * In the 744, 754 only 0x01 exists, 0x02 is undefined.
	 * It does not seem to hurt to trip both regardless of revision.
	 */
	cmd = psBus->read_pci_config( psPCIDevice->nBus, psPCIDevice->nDevice, psPCIDevice->nFunction, PCIR_DSXGCTRL, 1 );
	psBus->write_pci_config( psPCIDevice->nBus, psPCIDevice->nDevice, psPCIDevice->nFunction, PCIR_DSXGCTRL, 1, cmd & 0xfc );
	psBus->write_pci_config( psPCIDevice->nBus, psPCIDevice->nDevice, psPCIDevice->nFunction, PCIR_DSXGCTRL, 1, cmd | 0x03 );
	psBus->write_pci_config( psPCIDevice->nBus, psPCIDevice->nDevice, psPCIDevice->nFunction, PCIR_DSXGCTRL, 1, cmd & 0xfc );

	psBus->write_pci_config( psPCIDevice->nBus, psPCIDevice->nDevice, psPCIDevice->nFunction, PCIR_DSXPWRCTRL1, 2, 0 );
	psBus->write_pci_config( psPCIDevice->nBus, psPCIDevice->nDevice, psPCIDevice->nFunction, PCIR_DSXPWRCTRL2, 2, 0 );
}

static void ymfpci_enable_dsp(struct YMFDevice *device)
{
	ymfpci_writel(device, YDSXGR_CONFIG, 0x00000001);
}

static void ymfpci_disable_dsp(struct YMFDevice *device)
{
	uint32 val;
	int timeout = 1000;

	val = ymfpci_readl(device, YDSXGR_CONFIG);
	if (val)
		ymfpci_writel(device, YDSXGR_CONFIG, 0x00000000);
	while (timeout-- > 0) {
		val = ymfpci_readl(device, YDSXGR_STATUS);
		if ((val & 0x00000002) == 0)
			break;
	}
}

static int ymfpci_codec_ready(struct YMFDevice *device, int secondary)
{
	bigtime_t end_time;
	uint32 reg = secondary ? YDSXGR_SECSTATUSADR : YDSXGR_PRISTATUSADR;

	/* The original Linux code was end_time = jiffies + 3 * (HZ / 4);
	   We can treat jiffies as a function of get_system_time() and get away with it */

	end_time = ((get_system_time()/1000) + 3)*25;
	do {
		if ((ymfpci_readw(device, reg) & 0x8000) == 0)
			return 0;
	} while (end_time - (get_system_time()/1000) >= 0);
	kerndbg( KERN_DEBUG, "Codec %i is not ready [0x%x]\n", secondary, ymfpci_readw(device,reg));
	return -EBUSY;
}

static void ymfpci_codec_write(struct ac97_codec *codec, uint8 reg, uint16 val)
{
	struct YMFDevice *device = codec->private_data;
	uint32 cmd;

	spinlock(&device->ac97_lock);
	/* XXX Do make use of dev->id */
	ymfpci_codec_ready(device, 0);
	cmd = ((YDSXG_AC97WRITECMD | reg) << 16) | val;
	ymfpci_writel(device, YDSXGR_AC97CMDDATA, cmd);
	spinunlock(&device->ac97_lock);
}

static uint16 _ymfpci_codec_read(struct YMFDevice *device, uint8 reg)
{
	int i;

	if (ymfpci_codec_ready(device, 0))
		return ~0;
	ymfpci_writew(device, YDSXGR_AC97CMDADR, YDSXG_AC97READCMD | reg);
	if (ymfpci_codec_ready(device, 0))
		return ~0;
	if (device->pcidev->nDeviceID == PCI_DEVICE_ID_YAMAHA_744 && device->rev < 2) {
		for (i = 0; i < 600; i++)
			ymfpci_readw(device, YDSXGR_PRISTATUSDATA);
	}
	return ymfpci_readw(device, YDSXGR_PRISTATUSDATA);
}

static uint16 ymfpci_codec_read(struct ac97_codec *codec, uint8 reg)
{
	struct YMFDevice *device = codec->private_data;
	uint16 ret;
	
	spinlock(&device->ac97_lock);
	ret = _ymfpci_codec_read(device, reg);
	spinunlock(&device->ac97_lock);
	
	return ret;
}

static int ymfpci_memalloc(struct YMFDevice *device)
{
	unsigned int playback_ctrl_size;
	unsigned int bank_size_playback;
	unsigned int bank_size_capture;
	unsigned int bank_size_effect;
	unsigned int size;
	unsigned int off;
	char *ptr;
	uint32 pba;
	int voice, bank;

	playback_ctrl_size = 4 + 4 * YDSXG_PLAYBACK_VOICES;
	bank_size_playback = ymfpci_readl(device, YDSXGR_PLAYCTRLSIZE) << 2;
	bank_size_capture = ymfpci_readl(device, YDSXGR_RECCTRLSIZE) << 2;
	bank_size_effect = ymfpci_readl(device, YDSXGR_EFFCTRLSIZE) << 2;
	device->work_size = YDSXG_DEFAULT_WORK_SIZE;

	size = ((playback_ctrl_size + 0x00ff) & ~0x00ff) +
	    ((bank_size_playback * 2 * YDSXG_PLAYBACK_VOICES + 0xff) & ~0xff) +
	    ((bank_size_capture * 2 * YDSXG_CAPTURE_VOICES + 0xff) & ~0xff) +
	    ((bank_size_effect * 2 * YDSXG_EFFECT_VOICES + 0xff) & ~0xff) +
	    device->work_size;

	ptr = pci_alloc_consistent(device->pcidev, size + 0xff, &pba);
	if (ptr == NULL)
		return -ENOMEM;
	device->dma_area_va = ptr;
	device->dma_area_ba = pba;
	device->dma_area_size = size + 0xff;

	if ((off = ((uint) ptr) & 0xff) != 0) {
		ptr += 0x100 - off;
		pba += 0x100 - off;
	}

	/*
	 * Hardware requires only ptr[playback_ctrl_size] zeroed,
	 * but in our judgement it is a wrong kind of savings, so clear it all.
	 */
	memset(ptr, 0, size);

	device->ctrl_playback = (uint32 *)ptr;
	device->ctrl_playback_ba = pba;
	device->ctrl_playback[0] = YDSXG_PLAYBACK_VOICES;
	ptr += (playback_ctrl_size + 0x00ff) & ~0x00ff;
	pba += (playback_ctrl_size + 0x00ff) & ~0x00ff;

	off = 0;
	for (voice = 0; voice < YDSXG_PLAYBACK_VOICES; voice++) {
		device->voices[voice].number = voice;
		device->voices[voice].bank =
		    (ymfpci_playback_bank_t *) (ptr + off);
		device->voices[voice].bank_ba = pba + off;
		off += 2 * bank_size_playback;		/* 2 banks */
	}
	off = (off + 0xff) & ~0xff;
	ptr += off;
	pba += off;

	off = 0;
	device->bank_base_capture = pba;
	for (voice = 0; voice < YDSXG_CAPTURE_VOICES; voice++)
		for (bank = 0; bank < 2; bank++) {
			device->bank_capture[voice][bank] =
			    (ymfpci_capture_bank_t *) (ptr + off);
			off += bank_size_capture;
		}
	off = (off + 0xff) & ~0xff;
	ptr += off;
	pba += off;

	off = 0;
	device->bank_base_effect = pba;
	for (voice = 0; voice < YDSXG_EFFECT_VOICES; voice++)
		for (bank = 0; bank < 2; bank++) {
			device->bank_effect[voice][bank] =
			    (ymfpci_effect_bank_t *) (ptr + off);
			off += bank_size_effect;
		}
	off = (off + 0xff) & ~0xff;
	ptr += off;
	pba += off;

	device->work_base = pba;

	return 0;
}

static void ymf_memload(struct YMFDevice *device)
{
	ymfpci_writel(device, YDSXGR_PLAYCTRLBASE, device->ctrl_playback_ba);
	ymfpci_writel(device, YDSXGR_RECCTRLBASE, device->bank_base_capture);
	ymfpci_writel(device, YDSXGR_EFFCTRLBASE, device->bank_base_effect);
	ymfpci_writel(device, YDSXGR_WORKBASE, device->work_base);
	ymfpci_writel(device, YDSXGR_WORKSIZE, device->work_size >> 2);

	/* S/PDIF output initialization */
	ymfpci_writew(device, YDSXGR_SPDIFOUTCTRL, 0);
	ymfpci_writew(device, YDSXGR_SPDIFOUTSTATUS,
		SND_PCM_AES0_CON_EMPHASIS_NONE |
		(SND_PCM_AES1_CON_ORIGINAL << 8) |
		(SND_PCM_AES1_CON_PCM_CODER << 8));

	/* S/PDIF input initialization */
	ymfpci_writew(device, YDSXGR_SPDIFINCTRL, 0);

	/* move this volume setup to mixer */
	ymfpci_writel(device, YDSXGR_NATIVEDACOUTVOL, 0x3fff3fff);
	ymfpci_writel(device, YDSXGR_BUF441OUTVOL, 0);
	ymfpci_writel(device, YDSXGR_NATIVEADCINVOL, 0x3fff3fff);
	ymfpci_writel(device, YDSXGR_NATIVEDACINVOL, 0x3fff3fff);
}

static void ymfpci_memfree(struct YMFDevice *device)
{
	ymfpci_writel(device, YDSXGR_PLAYCTRLBASE, 0);
	ymfpci_writel(device, YDSXGR_RECCTRLBASE, 0);
	ymfpci_writel(device, YDSXGR_EFFCTRLBASE, 0);
	ymfpci_writel(device, YDSXGR_WORKBASE, 0);
	ymfpci_writel(device, YDSXGR_WORKSIZE, 0);
	kfree(device->dma_area_va);
}

#include "ymfpci_image.h"

static void ymfpci_download_image(struct YMFDevice *device)
{
	int i, ver_1e;
	uint16 ctrl;

	ymfpci_writel(device, YDSXGR_NATIVEDACOUTVOL, 0x00000000);
	ymfpci_disable_dsp(device);
	ymfpci_writel(device, YDSXGR_MODE, 0x00010000);
	ymfpci_writel(device, YDSXGR_MODE, 0x00000000);
	ymfpci_writel(device, YDSXGR_MAPOFREC, 0x00000000);
	ymfpci_writel(device, YDSXGR_MAPOFEFFECT, 0x00000000);
	ymfpci_writel(device, YDSXGR_PLAYCTRLBASE, 0x00000000);
	ymfpci_writel(device, YDSXGR_RECCTRLBASE, 0x00000000);
	ymfpci_writel(device, YDSXGR_EFFCTRLBASE, 0x00000000);
	ctrl = ymfpci_readw(device, YDSXGR_GLOBALCTRL);
	ymfpci_writew(device, YDSXGR_GLOBALCTRL, ctrl & ~0x0007);

	/* setup DSP instruction code */
	for (i = 0; i < YDSXG_DSPLENGTH / 4; i++)
		ymfpci_writel(device, YDSXGR_DSPINSTRAM + (i << 2), DspInst[i]);

	switch (device->pcidev->nDeviceID) {
	case PCI_DEVICE_ID_YAMAHA_724F:
	case PCI_DEVICE_ID_YAMAHA_740C:
	case PCI_DEVICE_ID_YAMAHA_744:
	case PCI_DEVICE_ID_YAMAHA_754:
		ver_1e = 1;
		break;
	default:
		ver_1e = 0;
	}

	if (ver_1e) {
		/* setup control instruction code */
		for (i = 0; i < YDSXG_CTRLLENGTH / 4; i++)
			ymfpci_writel(device, YDSXGR_CTRLINSTRAM + (i << 2), CntrlInst1E[i]);
	} else {
		for (i = 0; i < YDSXG_CTRLLENGTH / 4; i++)
			ymfpci_writel(device, YDSXGR_CTRLINSTRAM + (i << 2), CntrlInst[i]);
	}

	ymfpci_enable_dsp(device);

	/* 0.02s sounds not too bad, we may do schedule_timeout() later. */
	udelay(200); /* seems we need some delay after downloading image.. */
}

static int ymf_ac97_init(struct YMFDevice *device, int num_ac97)
{
	struct ac97_codec *codec;
	uint16 eid;

	if ((codec = ac97_alloc_codec()) == NULL)
		return -ENOMEM;

	/* initialize some basic codec information, other fields will be filled
	   in ac97_probe_codec */
	codec->private_data = device;
	codec->id = num_ac97;

	codec->codec_read = ymfpci_codec_read;
	codec->codec_write = ymfpci_codec_write;

	if (ac97_probe_codec(codec) == 0) {
		kerndbg(KERN_WARNING,"ac97_probe_codec failed\n");
		goto out_kfree;
	}

	eid = ymfpci_codec_read(codec, AC97_EXTENDED_ID);
	if (eid==0xFFFF) {
		kerndbg(KERN_WARNING,"No AC97 codec attached?\n");
		goto out_kfree;
	}

	device->ac97_features = eid;
	device->ac97_codec[num_ac97] = codec;

	return 0;
 out_kfree:
	ac97_release_codec(codec);
	return -ENODEV;
}

static void ymf_start_dac(struct ymf_state *state)
{
	ymf_playback_trigger(state->device, &state->wpcm, 1);
}

/*
 * Wait until output is drained.
 * This does not kill the hardware for the sake of ioctls.
 */
static void ymf_wait_dac(struct ymf_state *state)
{
	struct YMFDevice *device = state->device;
	struct ymf_pcm *ypcm = &state->wpcm;
	unsigned long flags;

	spinlock_cli(&device->reg_lock, flags);
	if (ypcm->dmabuf.count != 0 && !ypcm->running) {
		ymf_playback_trigger(device, ypcm, 1);
	}

	while (ypcm->running) {
		spinunlock_restore(&device->reg_lock, flags);
		Schedule();
		spinlock_cli(&device->reg_lock, flags);
	}

	spinunlock_restore(&device->reg_lock, flags);

	/*
	 * This function may take up to 4 seconds to reach this point
	 * (32K circular buffer, 8000 Hz). User notices.
	 */
}

/* Can just stop, without wait. Or can we? */
static void ymf_stop_adc(struct ymf_state *state)
{
	struct YMFDevice *device = state->device;
	unsigned long flags;

	spinlock_cli(&device->reg_lock, flags);
	ymf_capture_trigger(device, &state->rpcm, 0);
	spinunlock_restore(&device->reg_lock, flags);
}

static int ymf_playback_trigger(struct YMFDevice *device, struct ymf_pcm *ypcm, int cmd)
{

	if (ypcm->voices[0] == NULL) {
		return -EINVAL;
	}
	if (cmd != 0) {
		device->ctrl_playback[ypcm->voices[0]->number + 1] =
		    cpu_to_le32(ypcm->voices[0]->bank_ba);
		if (ypcm->voices[1] != NULL)
			device->ctrl_playback[ypcm->voices[1]->number + 1] =
			    cpu_to_le32(ypcm->voices[1]->bank_ba);
		ypcm->running = 1;
	} else {
		device->ctrl_playback[ypcm->voices[0]->number + 1] = 0;
		if (ypcm->voices[1] != NULL)
			device->ctrl_playback[ypcm->voices[1]->number + 1] = 0;
		ypcm->running = 0;
	}
	return 0;
}

static void ymf_capture_trigger(struct YMFDevice *device, struct ymf_pcm *ypcm, int cmd)
{
	uint32 tmp;

	if (cmd != 0) {
		tmp = ymfpci_readl(device, YDSXGR_MAPOFREC) | (1 << ypcm->capture_bank_number);
		ymfpci_writel(device, YDSXGR_MAPOFREC, tmp);
		ypcm->running = 1;
	} else {
		tmp = ymfpci_readl(device, YDSXGR_MAPOFREC) & ~(1 << ypcm->capture_bank_number);
		ymfpci_writel(device, YDSXGR_MAPOFREC, tmp);
		ypcm->running = 0;
	}
}

/*
 *  Playback voice management
 */

static int voice_alloc(struct YMFDevice *device, ymfpci_voice_type_t type, int pair, ymfpci_voice_t *rvoice[])
{
	ymfpci_voice_t *voice, *voice2;
	int idx;

	for (idx = 0; idx < YDSXG_PLAYBACK_VOICES; idx += pair ? 2 : 1) {
		voice = &device->voices[idx];
		voice2 = pair ? &device->voices[idx+1] : NULL;
		if (voice->use || (voice2 && voice2->use))
			continue;
		voice->use = 1;
		if (voice2)
			voice2->use = 1;
		switch (type) {
		case YMFPCI_PCM:
			voice->pcm = 1;
			if (voice2)
				voice2->pcm = 1;
			break;
		case YMFPCI_SYNTH:
			voice->synth = 1;
			break;
		case YMFPCI_MIDI:
			voice->midi = 1;
			break;
		}

		ymfpci_hw_start(device);
		rvoice[0] = voice;
		if (voice2) {
			ymfpci_hw_start(device);
			rvoice[1] = voice2;
		}
		return 0;
	}
	return -EBUSY;	/* Your audio channel is open by someone else. */
}

static int ymfpci_pcm_voice_alloc(struct ymf_pcm *ypcm, int voices)
{
	struct YMFDevice *device;
	int err;

	device = ypcm->state->device;
	if (ypcm->voices[1] != NULL && voices < 2) {
		ymfpci_voice_free(device, ypcm->voices[1]);
		ypcm->voices[1] = NULL;
	}

	if (voices == 1 && ypcm->voices[0] != NULL)
		return 0;		/* already allocated */
	if (voices == 2 && ypcm->voices[0] != NULL && ypcm->voices[1] != NULL)
		return 0;		/* already allocated */
	if (voices > 1) {
		if (ypcm->voices[0] != NULL && ypcm->voices[1] == NULL) {
			ymfpci_voice_free(device, ypcm->voices[0]);
			ypcm->voices[0] = NULL;
		}

		if ((err = voice_alloc(device, YMFPCI_PCM, 1, ypcm->voices)) < 0)
			return err;
		ypcm->voices[0]->ypcm = ypcm;
		ypcm->voices[1]->ypcm = ypcm;

	} else {
		if ((err = voice_alloc(device, YMFPCI_PCM, 0, ypcm->voices)) < 0)
			return err;
		ypcm->voices[0]->ypcm = ypcm;
	}

	return 0;
}

static void ymfpci_voice_free(struct YMFDevice *device, ymfpci_voice_t *pvoice)
{
	ymfpci_hw_stop(device);
	pvoice->use = pvoice->pcm = pvoice->synth = pvoice->midi = 0;
	pvoice->ypcm = NULL;
}

static void ymf_pcm_init_voice(ymfpci_voice_t *voice, int stereo,
    int rate, int w_16, unsigned long addr, unsigned int end, int spdif)
{
	uint32 format;
	uint32 delta = ymfpci_calc_delta(rate);
	uint32 lpfQ = ymfpci_calc_lpfQ(rate);
	uint32 lpfK = ymfpci_calc_lpfK(rate);
	ymfpci_playback_bank_t *bank;
	int nbank;

	/*
	 * The gain is a floating point number. According to the manual,
	 * bit 31 indicates a sign bit, bit 30 indicates an integer part,
	 * and bits [29:15] indicate a decimal fraction part. Thus,
	 * for a gain of 1.0 the constant of 0x40000000 is loaded.
	 */
	unsigned default_gain = cpu_to_le32(0x40000000);

	format = (stereo ? 0x00010000 : 0) | (w_16 ? 0 : 0x80000000);
	if (stereo)
		end >>= 1;
	if (w_16)
		end >>= 1;
	for (nbank = 0; nbank < 2; nbank++) {
		bank = &voice->bank[nbank];
		bank->format = cpu_to_le32(format);
		bank->loop_default = 0;	/* 0-loops forever, otherwise count */
		bank->base = cpu_to_le32(addr);
		bank->loop_start = 0;
		bank->loop_end = cpu_to_le32(end);
		bank->loop_frac = 0;
		bank->eg_gain_end = default_gain;
		bank->lpfQ = cpu_to_le32(lpfQ);
		bank->status = 0;
		bank->num_of_frames = 0;
		bank->loop_count = 0;
		bank->start = 0;
		bank->start_frac = 0;
		bank->delta =
		bank->delta_end = cpu_to_le32(delta);
		bank->lpfK =
		bank->lpfK_end = cpu_to_le32(lpfK);
		bank->eg_gain = default_gain;
		bank->lpfD1 =
		bank->lpfD2 = 0;

		bank->left_gain = 
		bank->right_gain =
		bank->left_gain_end =
		bank->right_gain_end =
		bank->eff1_gain =
		bank->eff2_gain =
		bank->eff3_gain =
		bank->eff1_gain_end =
		bank->eff2_gain_end =
		bank->eff3_gain_end = 0;

		if (!stereo) {
			if (!spdif) {
				bank->left_gain = 
				bank->right_gain =
				bank->left_gain_end =
				bank->right_gain_end = default_gain;
			} else {
				bank->eff2_gain =
				bank->eff2_gain_end =
				bank->eff3_gain =
				bank->eff3_gain_end = default_gain;
			}
		} else {
			if (!spdif) {
				if ((voice->number & 1) == 0) {
					bank->left_gain =
					bank->left_gain_end = default_gain;
				} else {
					bank->format |= cpu_to_le32(1);
					bank->right_gain =
					bank->right_gain_end = default_gain;
				}
			} else {
				if ((voice->number & 1) == 0) {
					bank->eff2_gain =
					bank->eff2_gain_end = default_gain;
				} else {
					bank->format |= cpu_to_le32(1);
					bank->eff3_gain =
					bank->eff3_gain_end = default_gain;
				}
			}
		}
	}
}

static void ymf_pcm_free_substream(struct ymf_pcm *ypcm)
{
	unsigned long flags;
	struct YMFDevice *device;

	device = ypcm->state->device;

	if (ypcm->type == PLAYBACK_VOICE) {
		spinlock_cli(&device->voice_lock, flags);
		if (ypcm->voices[1])
			ymfpci_voice_free(device, ypcm->voices[1]);
		if (ypcm->voices[0])
			ymfpci_voice_free(device, ypcm->voices[0]);
		spinunlock_restore(&device->voice_lock, flags);
	} else {
		if (ypcm->capture_bank_number != -1) {
			device->capture[ypcm->capture_bank_number].use = 0;
			ypcm->capture_bank_number = -1;
			ymfpci_hw_stop(device);
		}
	}
}

static struct ymf_state *ymf_state_alloc(struct YMFDevice *device)
{
	struct ymf_pcm *ypcm;
	struct ymf_state *state;

	if ((state = kmalloc(sizeof(struct ymf_state), MEMF_KERNEL)) == NULL) {
		goto out0;
	}
	memset(state, 0, sizeof(struct ymf_state));

	ypcm = &state->wpcm;
	ypcm->state = state;
	ypcm->type = PLAYBACK_VOICE;
	ypcm->capture_bank_number = -1;
//	init_waitqueue_head(&ypcm->dmabuf.wait);
	ypcm->dmabuf.wait = create_semaphore("ymfpci_wait", 1, 0);

	ypcm = &state->rpcm;
	ypcm->state = state;
	ypcm->type = CAPTURE_AC97;
	ypcm->capture_bank_number = -1;
//	init_waitqueue_head(&ypcm->dmabuf.wait);
	ypcm->dmabuf.wait = create_semaphore("ymfpci_wait", 1, 0);

	state->device = device;

#if 0
	state->format.format = AFMT_U8;
	state->format.rate = 8000;
	state->format.voices = 1;
#else
	state->format.format = AFMT_S16_LE;
	state->format.rate = 44100;
	state->format.voices = 2;
#endif
	ymf_pcm_update_shift(&state->format);

	return state;

out0:
	return NULL;
}

/*
 * XXX Capture channel allocation is entirely fake at the moment.
 * We use only one channel and mark it busy as required.
 */
static int ymf_capture_alloc(struct YMFDevice *device, int *pbank)
{
	struct ymf_capture *cap;
	int cbank;

	cbank = 1;		/* Only ADC slot is used for now. */
	cap = &device->capture[cbank];
	if (cap->use)
		return -EBUSY;
	cap->use = 1;
	*pbank = cbank;
	return 0;
}

static int ymf_playback_prepare(struct ymf_state *state)
{
	struct ymf_pcm *ypcm = &state->wpcm;
	int err, nvoice;

	if ((err = ymfpci_pcm_voice_alloc(ypcm, state->format.voices)) < 0) {
		/* Somebody started 32 mpg123's in parallel? */
		kerndbg(KERN_INFO, "Cannot allocate voice\n");
		return err;
	}

	for (nvoice = 0; nvoice < state->format.voices; nvoice++) {
		ymf_pcm_init_voice(ypcm->voices[nvoice],
		    state->format.voices == 2, state->format.rate,
		    ymf_pcm_format_width(state->format.format) == 16,
		    ypcm->dmabuf.dma_addr, ypcm->dmabuf.dmasize,
		    ypcm->spdif);
	}

	return 0;
}

static int ymf_capture_prepare(struct ymf_state *state)
{
	struct YMFDevice *device = state->device;
	struct ymf_pcm *ypcm = &state->rpcm;
	ymfpci_capture_bank_t * bank;
	/* XXX This is confusing, gotta rename one of them banks... */
	int nbank;		/* flip-flop bank */
	int cbank;		/* input [super-]bank */
	struct ymf_capture *cap;
	uint32 rate, format;

	if (ypcm->capture_bank_number == -1) {
		if (ymf_capture_alloc(device, &cbank) != 0)
			return -EBUSY;

		ypcm->capture_bank_number = cbank;

		cap = &device->capture[cbank];
		cap->bank = device->bank_capture[cbank][0];
		cap->ypcm = ypcm;
		ymfpci_hw_start(device);
	}

	// ypcm->frag_size = snd_pcm_lib_transfer_fragment(substream);
	// frag_size is replaced with nonfragged byte-aligned rolling buffer
	rate = ((48000 * 4096) / state->format.rate) - 1;
	format = 0;
	if (state->format.voices == 2)
		format |= 2;
	if (ymf_pcm_format_width(state->format.format) == 8)
		format |= 1;
	switch (ypcm->capture_bank_number) {
	case 0:
		ymfpci_writel(device, YDSXGR_RECFORMAT, format);
		ymfpci_writel(device, YDSXGR_RECSLOTSR, rate);
		break;
	case 1:
		ymfpci_writel(device, YDSXGR_ADCFORMAT, format);
		ymfpci_writel(device, YDSXGR_ADCSLOTSR, rate);
		break;
	}
	for (nbank = 0; nbank < 2; nbank++) {
		bank = device->bank_capture[ypcm->capture_bank_number][nbank];
		bank->base = cpu_to_le32(ypcm->dmabuf.dma_addr);
		// bank->loop_end = ypcm->dmabuf.dmasize >> state->format.shift;
		bank->loop_end = cpu_to_le32(ypcm->dmabuf.dmasize);
		bank->start = 0;
		bank->num_of_loops = 0;
	}
#if 0 /* s/pdif */
	if (state->digital.dig_valid)
		/*state->digital.type == SND_PCM_DIG_AES_IEC958*/
		ymfpci_writew(codec, YDSXGR_SPDIFOUTSTATUS,
		    state->digital.dig_status[0] | (state->digital.dig_status[1] << 8));
#endif
	return 0;
}

/* Are you sure 32K is not too much? See if mpg123 skips on loaded systems. */
#define DMABUF_DEFAULTORDER (15-PAGE_SHIFT)
#define DMABUF_MINORDER 1

/*
 * Allocate DMA buffer
 */
static int alloc_dmabuf(struct YMFDevice *device, struct ymf_dmabuf *dmabuf)
{
	void *rawbuf = NULL;
	uint32 dma_addr;
	int order;

	/* alloc as big a chunk as we can */
	for (order = DMABUF_DEFAULTORDER; order >= DMABUF_MINORDER; order--) {
		rawbuf = pci_alloc_consistent(device->pcidev, PAGE_SIZE << order, &dma_addr);
		if (rawbuf)
			break;
	}
	if (!rawbuf)
		return -ENOMEM;

#if 0
	printk(KERN_DEBUG "ymfpci: allocated %ld (order = %d) bytes at %p\n",
	       PAGE_SIZE << order, order, rawbuf);
#endif

	dmabuf->ready  = dmabuf->mapped = 0;
	dmabuf->rawbuf = rawbuf;
	dmabuf->dma_addr = dma_addr;
	dmabuf->buforder = order;

	return 0;
}

/*
 * Free DMA buffer
 */
static void dealloc_dmabuf(struct YMFDevice *device, struct ymf_dmabuf *dmabuf)
{
	if (dmabuf->rawbuf) {
		pci_free_consistent(device->pcidev, PAGE_SIZE << dmabuf->buforder,
		    dmabuf->rawbuf, dmabuf->dma_addr);
	}
	dmabuf->rawbuf = NULL;
	dmabuf->mapped = dmabuf->ready = 0;
}

static int prog_dmabuf(struct ymf_state *state, int rec)
{
	struct ymf_dmabuf *dmabuf;
	int w_16;
	unsigned bufsize;
	unsigned long flags;
	int redzone, redfrags;
	int ret;

	w_16 = ymf_pcm_format_width(state->format.format) == 16;
	dmabuf = rec ? &state->rpcm.dmabuf : &state->wpcm.dmabuf;

	spinlock_cli(&state->device->reg_lock, flags);
	dmabuf->hwptr = dmabuf->swptr = 0;
	dmabuf->total_bytes = 0;
	dmabuf->count = 0;
	spinunlock_restore(&state->device->reg_lock, flags);

	/* allocate DMA buffer if not allocated yet */
	if (!dmabuf->rawbuf)
	{
		if ((ret = alloc_dmabuf(state->device, dmabuf)))
			return ret;
	}

	/*
	 * Create fake fragment sizes and numbers for OSS ioctls.
	 * Import what Doom might have set with SNDCTL_DSP_SETFRAGMENT.
	 */
	bufsize = PAGE_SIZE << dmabuf->buforder;
	/* By default we give 4 big buffers. */
	dmabuf->fragshift = (dmabuf->buforder + PAGE_SHIFT - 2);
	if (dmabuf->ossfragshift > 3 &&
	    dmabuf->ossfragshift < dmabuf->fragshift) {
		/* If OSS set smaller fragments, give more smaller buffers. */
		dmabuf->fragshift = dmabuf->ossfragshift;
	}
	dmabuf->fragsize = 1 << dmabuf->fragshift;

	dmabuf->numfrag = bufsize >> dmabuf->fragshift;
	dmabuf->dmasize = dmabuf->numfrag << dmabuf->fragshift;

	if (dmabuf->ossmaxfrags >= 2) {
		redzone = ymf_calc_lend(state->format.rate);
		redzone <<= state->format.shift;
		redzone *= 3;
		redfrags = (redzone + dmabuf->fragsize-1) >> dmabuf->fragshift;

		if (dmabuf->ossmaxfrags + redfrags < dmabuf->numfrag) {
			dmabuf->numfrag = dmabuf->ossmaxfrags + redfrags;
			dmabuf->dmasize = dmabuf->numfrag << dmabuf->fragshift;
		}
	}

	memset(dmabuf->rawbuf, w_16 ? 0 : 0x80, dmabuf->dmasize);

	/*
	 *	Now set up the ring 
	 */

	/* XXX   ret = rec? cap_pre(): pbk_pre();  */
	spinlock_cli(&state->device->voice_lock, flags);
	if (rec) {
		if ((ret = ymf_capture_prepare(state)) != 0) {
			spinunlock_restore(&state->device->voice_lock, flags);
			return ret;
		}
	} else {
		if ((ret = ymf_playback_prepare(state)) != 0) {
			spinunlock_restore(&state->device->voice_lock, flags);
			return ret;
		}
	}
	spinunlock_restore(&state->device->voice_lock, flags);

	/* set the ready flag for the dma buffer (this comment is not stupid) */
	dmabuf->ready = 1;

#if 0
	printk(KERN_DEBUG "prog_dmabuf: rate %d format 0x%x,"
	    " numfrag %d fragsize %d dmasize %d\n",
	       state->format.rate, state->format.format, dmabuf->numfrag,
	       dmabuf->fragsize, dmabuf->dmasize);
#endif

	return 0;
}

static void ymfpci_hw_start(struct YMFDevice *device)
{
	unsigned long flags;

	spinlock_cli(&device->reg_lock, flags);
	if (device->start_count++ == 0) {
		ymfpci_writel(device, YDSXGR_MODE,
		    ymfpci_readl(device, YDSXGR_MODE) | 3);
		device->active_bank = ymfpci_readl(device, YDSXGR_CTRLSELECT) & 1;
	}
	spinunlock_restore(&device->reg_lock, flags);
}

static void ymfpci_hw_stop(struct YMFDevice *device)
{
	unsigned long flags;
	long timeout = 1000;

	spinlock_cli(&device->reg_lock, flags);
	if (--device->start_count == 0) {
		ymfpci_writel(device, YDSXGR_MODE,
		    ymfpci_readl(device, YDSXGR_MODE) & ~3);
		while (timeout-- > 0) {
			if ((ymfpci_readl(device, YDSXGR_STATUS) & 2) == 0)
				break;
		}
	}
	spinunlock_restore(&device->reg_lock, flags);
}

static void ymf_pcm_interrupt(struct YMFDevice *device, ymfpci_voice_t *voice)
{
	struct ymf_pcm *ypcm;
	int redzone;
	int pos, delta, swptr;
	int played, distance;
	struct ymf_state *state;
	struct ymf_dmabuf *dmabuf;
	char silence;

	if ((ypcm = voice->ypcm) == NULL) {
		return;
	}
	if ((state = ypcm->state) == NULL) {
		ypcm->running = 0;	// lock it
		return;
	}
	dmabuf = &ypcm->dmabuf;
	spin_lock(&device->reg_lock);
	if (ypcm->running) {
		kerndbg(KERN_DEBUG,"ymfpci: %d, intr bank %ld count %d start 0x%lx:%lx\n",
		   voice->number, device->active_bank, dmabuf->count,
		   le32_to_cpu(voice->bank[0].start),
		   le32_to_cpu(voice->bank[1].start));
		silence = (ymf_pcm_format_width(state->format.format) == 16) ?
		    0 : 0x80;
		/* We need actual left-hand-side redzone size here. */
		redzone = ymf_calc_lend(state->format.rate);
		redzone <<= (state->format.shift + 1);
		swptr = dmabuf->swptr;

		pos = le32_to_cpu(voice->bank[device->active_bank].start);
		pos <<= state->format.shift;
		if (pos < 0 || pos >= dmabuf->dmasize) {	/* ucode bug */
			kerndbg(KERN_DEBUG, "ymfpci: runaway voice %d: hwptr %d=>%d dmasize %d\n",
			    voice->number, dmabuf->hwptr, pos, dmabuf->dmasize);
			pos = 0;
		}
		if (pos < dmabuf->hwptr) {
			delta = dmabuf->dmasize - dmabuf->hwptr;
			memset(dmabuf->rawbuf + dmabuf->hwptr, silence, delta);
			delta += pos;
			memset(dmabuf->rawbuf, silence, pos);
		} else {
			delta = pos - dmabuf->hwptr;
			memset(dmabuf->rawbuf + dmabuf->hwptr, silence, delta);
		}
		dmabuf->hwptr = pos;

		if (dmabuf->count == 0) {
			kerndbg(KERN_DEBUG, "ymfpci: %d: strain: hwptr %d\n",
			    voice->number, dmabuf->hwptr);
			ymf_playback_trigger(device, ypcm, 0);
		}

		if (swptr <= pos) {
			distance = pos - swptr;
		} else {
			distance = dmabuf->dmasize - (swptr - pos);
		}
		if (distance < redzone) {
			/*
			 * hwptr inside redzone => DMA ran out of samples.
			 */
			if (delta < dmabuf->count) {
				/*
				 * Lost interrupt or other screwage.
				 */
				kerndbg(KERN_DEBUG, "ymfpci: %d: lost: delta %d"
				    " hwptr %d swptr %d distance %d count %d\n",
				    voice->number, delta,
				    dmabuf->hwptr, swptr, distance, dmabuf->count);
			} else {
				/*
				 * Normal end of DMA.
				 */
				kerndbg(KERN_DEBUG,"ymfpci: %d: done: delta %d"
				    " hwptr %d swptr %d distance %d count %d\n",
				    voice->number, delta,
				    dmabuf->hwptr, swptr, distance, dmabuf->count);
			}
			played = dmabuf->count;
			if (ypcm->running) {
				ymf_playback_trigger(device, ypcm, 0);
			}
		} else {
			/*
			 * hwptr is chipping away towards a remote swptr.
			 * Calculate other distance and apply it to count.
			 */
			if (swptr >= pos) {
				distance = swptr - pos;
			} else {
				distance = dmabuf->dmasize - (pos - swptr);
			}
			if (distance < dmabuf->count) {
				played = dmabuf->count - distance;
			} else {
				played = 0;
			}
		}

		dmabuf->total_bytes += played;
		dmabuf->count -= played;
		if (dmabuf->count < dmabuf->dmasize / 2) {
			wakeup_sem(dmabuf->wait, true);
		}
	}
	spin_unlock(&device->reg_lock);
}

static void ymf_cap_interrupt(struct YMFDevice *device, struct ymf_capture *cap)
{
	struct ymf_pcm *ypcm;
	int redzone;
	struct ymf_state *state;
	struct ymf_dmabuf *dmabuf;
	int pos, delta;
	int cnt;

	if ((ypcm = cap->ypcm) == NULL) {
		return;
	}
	if ((state = ypcm->state) == NULL) {
		ypcm->running = 0;	// lock it
		return;
	}
	dmabuf = &ypcm->dmabuf;
	spinlock(&device->reg_lock);
	if (ypcm->running) {
		redzone = ymf_calc_lend(state->format.rate);
		redzone <<= (state->format.shift + 1);

		pos = le32_to_cpu(cap->bank[device->active_bank].start);
		// pos <<= state->format.shift;
		if (pos < 0 || pos >= dmabuf->dmasize) {	/* ucode bug */
			kerndbg(KERN_DEBUG, "ymfpci: runaway capture %d: hwptr %d=>%d dmasize %d\n",
			    ypcm->capture_bank_number, dmabuf->hwptr, pos, dmabuf->dmasize);
			pos = 0;
		}
		if (pos < dmabuf->hwptr) {
			delta = dmabuf->dmasize - dmabuf->hwptr;
			delta += pos;
		} else {
			delta = pos - dmabuf->hwptr;
		}
		dmabuf->hwptr = pos;

		cnt = dmabuf->count;
		cnt += delta;
		if (cnt + redzone > dmabuf->dmasize) {
			/* Overflow - bump swptr */
			dmabuf->count = dmabuf->dmasize - redzone;
			dmabuf->swptr = dmabuf->hwptr + redzone;
			if (dmabuf->swptr >= dmabuf->dmasize) {
				dmabuf->swptr -= dmabuf->dmasize;
			}
		} else {
			dmabuf->count = cnt;
		}

		dmabuf->total_bytes += delta;
		if (dmabuf->count) {		/* && is_sleeping  XXX */
			wakeup_sem(dmabuf->wait,true);
		}
	}
	spinunlock(&device->reg_lock);
}

static int ymfpci_interrupt( int nIRQ, void *pData, SysCallRegs_s *psRegs )
{
	struct YMFDevice *device = (struct YMFDevice*)pData;
	uint32 status, nvoice, mode;
	struct ymf_voice *voice;
	struct ymf_capture *cap;

	status = ymfpci_readl(device, YDSXGR_STATUS);
	if (status & 0x80000000) {
		device->active_bank = ymfpci_readl(device, YDSXGR_CTRLSELECT) & 1;
		spin_lock(&device->voice_lock);
		for (nvoice = 0; nvoice < YDSXG_PLAYBACK_VOICES; nvoice++) {
			voice = &device->voices[nvoice];
			if (voice->use)
				ymf_pcm_interrupt(device, voice);
		}
		for (nvoice = 0; nvoice < YDSXG_CAPTURE_VOICES; nvoice++) {
			cap = &device->capture[nvoice];
			if (cap->use)
				ymf_cap_interrupt(device, cap);
		}
		spin_unlock(&device->voice_lock);
		spin_lock(&device->reg_lock);
		ymfpci_writel(device, YDSXGR_STATUS, 0x80000000);
		mode = ymfpci_readl(device, YDSXGR_MODE) | 2;
		ymfpci_writel(device, YDSXGR_MODE, mode);
		spin_unlock(&device->reg_lock);
	}

	status = ymfpci_readl(device, YDSXGR_INTFLAG);
	if (status & 1) {
		/* timer handler */
		ymfpci_writel(device, YDSXGR_INTFLAG, ~0);
	}

	return 0;
}

/* DSP interface */

status_t ymfpci_open( void* pNode, uint32 nFlags, void **ppCookie )
{
	struct YMFDevice *device = (struct YMFDevice*)pNode;
	struct ymf_state *state;

	if( NULL == device )
		return -ENXIO;

	lock_semaphore(device->open_sem, 0, INFINITE_TIMEOUT);

	if ((state = ymf_state_alloc(device)) == NULL) {
		unlock_semaphore(device->open_sem);
		return -ENOMEM;
	}
	list_add_tail(&state->chain, &device->states);

	/* Store the state data for use by read(), write() & ioctl() */
	*ppCookie = state;

	/*
	 * The Linux OSS driver setup the DMA buffers here, but we've moved
	 * the calls to prog_dmabuf() into ymfpci_read() and ymfpci_write()
	 */
#if 0 /* test if interrupts work */
	ymfpci_writew(device, YDSXGR_TIMERCOUNT, 0xfffe);	/* ~ 680ms */
	ymfpci_writeb(device, YDSXGR_TIMERCTRL,
	    (YDSXGR_TIMERCTRL_TEN|YDSXGR_TIMERCTRL_TIEN));
#endif
	unlock_semaphore(device->open_sem);

	return 0;
}

status_t ymfpci_close( void* pNode, void* pCookie )
{
	struct YMFDevice *device = (struct YMFDevice*)pNode;
	struct ymf_state *state = (struct ymf_state *)pCookie;

#if 0 /* test if interrupts work */
	ymfpci_writeb(unit, YDSXGR_TIMERCTRL, 0);
#endif

	lock_semaphore(device->open_sem, 0, INFINITE_TIMEOUT);

	/*
	 * XXX Solve the case of O_NONBLOCK close - don't deallocate here.
	 * Deallocate when unloading the driver and we can wait.
	 */
	ymf_wait_dac(state);
	ymf_stop_adc(state);		/* fortunately, it's immediate */
	dealloc_dmabuf(device, &state->wpcm.dmabuf);
	dealloc_dmabuf(device, &state->rpcm.dmabuf);
	ymf_pcm_free_substream(&state->wpcm);
	ymf_pcm_free_substream(&state->rpcm);

	list_del(&state->chain);
	kfree(state);

	unlock_semaphore(device->open_sem);

	return 0;
}

int ymfpci_read( void *pNode, void *pCookie, off_t nPos, void *pBuffer, size_t nCount )
{
	struct ymf_state *state = (struct ymf_state *)pCookie;
	struct ymf_dmabuf *dmabuf = &state->rpcm.dmabuf;
	struct YMFDevice *device = state->device;
	ssize_t ret;
	unsigned long flags;
	unsigned int swptr;
	int cnt;			/* This many to go in this revolution */

	if (!dmabuf->ready)
	{
		ret = prog_dmabuf(state, 1);
		if( ret )
			return ret;
	}
	ret = 0;

	state->direction = 1;	/* read */

	while (nCount > 0) {
		spinlock_cli(&device->reg_lock, flags);
		swptr = dmabuf->swptr;
		cnt = dmabuf->dmasize - swptr;
		if (dmabuf->count < cnt)
			cnt = dmabuf->count;
		spinunlock_restore(&device->reg_lock, flags);

		if (cnt > nCount)
			cnt = nCount;
		if (cnt <= 0) {
			unsigned long tmo;
			/* buffer is empty, start the dma machine and wait for data to be
			   recorded */
			spinlock_cli(&state->device->reg_lock, flags);
			if (!state->rpcm.running) {
				ymf_capture_trigger(state->device, &state->rpcm, 1);
			}
			spinunlock_restore(&state->device->reg_lock, flags);
			/* This isnt strictly right for the 810  but it'll do */
			tmo = (dmabuf->dmasize * HZ) / (state->format.rate * 2);
			tmo >>= state->format.shift;
			/* There are two situations when sleep_on_timeout returns, one is when
			   the interrupt is serviced correctly and the process is waked up by
			   ISR ON TIME. Another is when timeout is expired, which means that
			   either interrupt is NOT serviced correctly (pending interrupt) or it
			   is TOO LATE for the process to be scheduled to run (scheduler latency)
			   which results in a (potential) buffer overrun. And worse, there is
			   NOTHING we can do to prevent it. */
			sleep_on_sem(dmabuf->wait, tmo);

			if (is_signal_pending()) {
				if (!ret) ret = -ERESTARTSYS;
				break;
			}
			continue;
		}

		if (copy_to_user(pBuffer, dmabuf->rawbuf + swptr, cnt)) {
			if (!ret) ret = -EFAULT;
			break;
		}

		swptr = (swptr + cnt) % dmabuf->dmasize;

		spinlock_cli(&device->reg_lock, flags);

		dmabuf->swptr = swptr;
		dmabuf->count -= cnt;

		nCount -= cnt;
		pBuffer += cnt;
		ret += cnt;
		if (!state->rpcm.running) {
			ymf_capture_trigger(device, &state->rpcm, 1);
		}
		spinunlock_restore(&device->reg_lock, flags);
	}

	return ret;
}

int ymfpci_write( void *pNode, void *pCookie, off_t nPos, const void *pBuffer, size_t nCount )
{
	struct YMFDevice *device = (struct YMFDevice*)pNode;
	struct ymf_state *state = (struct ymf_state *)pCookie;
	struct ymf_dmabuf *dmabuf = &state->wpcm.dmabuf;
	status_t ret = -ENXIO;
	unsigned long flags;
	unsigned int swptr;
	int cnt;			/* This many to go in this revolution */
	int redzone;
	int delay;

	if (!dmabuf->ready)
	{
		ret = prog_dmabuf(state, 0);
		if( ret )
			return ret;
	}
	ret = 0;

	state->direction = 0; /* write */

	/*
	 * Alan's cs46xx works without a red zone - marvel of ingenuity.
	 * We are not so brilliant... Red zone does two things:
	 *  1. allows for safe start after a pause as we have no way
	 *     to know what the actual, relentlessly advancing, hwptr is.
	 *  2. makes computations in ymf_pcm_interrupt simpler.
	 */
	redzone = ymf_calc_lend(state->format.rate) << state->format.shift;
	redzone *= 3;	/* 2 redzone + 1 possible uncertainty reserve. */

	while (nCount > 0) {
		spinlock_cli(&device->reg_lock, flags);
		if (dmabuf->count < 0) {
			kerndbg(KERN_WARNING,"ymfpci_write: count %d, was legal in cs46xx\n", dmabuf->count);
			dmabuf->count = 0;
		}
		if (dmabuf->count == 0) {
			swptr = dmabuf->hwptr;
			if (state->wpcm.running) {
				/*
				 * Add uncertainty reserve.
				 */
				cnt = ymf_calc_lend(state->format.rate);
				cnt <<= state->format.shift;
				if ((swptr += cnt) >= dmabuf->dmasize) {
					swptr -= dmabuf->dmasize;
				}
			}
			dmabuf->swptr = swptr;
		} else {
			/*
			 * XXX This is not right if dmabuf->count is small -
			 * about 2*x frame size or less. We cannot count on
			 * on appending and not causing an artefact.
			 * Should use a variation of the count==0 case above.
			 */
			swptr = dmabuf->swptr;
		}

		cnt = dmabuf->dmasize - swptr;
		if (dmabuf->count + cnt > dmabuf->dmasize - redzone)
			cnt = (dmabuf->dmasize - redzone) - dmabuf->count;
		spinunlock_restore(&device->reg_lock, flags);

		if (cnt > nCount)
			cnt = nCount;
		if (cnt <= 0) {
			/*
			 * buffer is full, start the dma machine and
			 * wait for data to be played
			 */
			spinlock_cli(&device->reg_lock, flags);
			if (!state->wpcm.running) {
				ymf_playback_trigger(device, &state->wpcm, 1);
			}
			spinunlock_restore(&device->reg_lock, flags);
			sleep_on_sem(dmabuf->wait, INFINITE_TIMEOUT);
			if (is_signal_pending()) {
				if (!ret) ret = -ERESTARTSYS;
				break;
			}
			continue;
		}
		if (copy_from_user(dmabuf->rawbuf + swptr, pBuffer, cnt)) {
			if (!ret) ret = -EFAULT;
			break;
		}

		if ((swptr += cnt) >= dmabuf->dmasize) {
			swptr -= dmabuf->dmasize;
		}

		spinlock_cli(&device->reg_lock, flags);
		dmabuf->swptr = swptr;
		dmabuf->count += cnt;

		/*
		 * Start here is a bad idea - may cause startup click
		 * in /bin/play when dmabuf is not full yet.
		 * However, some broken applications do not make
		 * any use of SNDCTL_DSP_SYNC (Doom is the worst).
		 * One frame is about 5.3ms, Doom write size is 46ms.
		 */
		delay = state->format.rate / 20;	/* 50ms */
		delay <<= state->format.shift;
		if (dmabuf->count >= delay && !state->wpcm.running) {
			ymf_playback_trigger(device, &state->wpcm, 1);
		}

		spinunlock_restore(&device->reg_lock, flags);

		nCount -= cnt;
		pBuffer += cnt;
		ret += cnt;
	}

	return ret;
}

status_t ymfpci_ioctl( void *pNode, void *pCookie, uint32 nCmd, void *pArg, bool bFromKernel )
{
	struct ymf_state *state = (struct ymf_state *)pCookie;
	struct ymf_dmabuf *dmabuf;
	unsigned long flags;
	audio_buf_info abinfo;
	int redzone;
	int val;

	switch( nCmd )
	{
		case OSS_GETVERSION:
		{
			put_user(SOUND_VERSION, (int *)pArg);
			return 0;
		}

		case SNDCTL_DSP_SYNC:
		{
			if (state->direction == 0) {
				dmabuf = &state->wpcm.dmabuf;
				ymf_wait_dac(state);
			}
			/* XXX What does this do for reading? dmabuf->count=0; ? */
			return 0;
		}

		case SNDCTL_DSP_GETCAPS:
		{
			put_user(0, (int *)pArg);
			return 0;
		}

		case SNDCTL_DSP_RESET:
		{
			if (state->direction == 0) {
				ymf_wait_dac(state);
				dmabuf = &state->wpcm.dmabuf;
				spinlock_cli(&state->device->reg_lock, flags);
				dmabuf->ready = 0;
				dmabuf->swptr = dmabuf->hwptr;
				dmabuf->count = dmabuf->total_bytes = 0;
				spinunlock_restore(&state->device->reg_lock, flags);
			}
			if (state->direction == 1) {
				ymf_stop_adc(state);
				dmabuf = &state->rpcm.dmabuf;
				spinlock_cli(&state->device->reg_lock, flags);
				dmabuf->ready = 0;
				dmabuf->swptr = dmabuf->hwptr;
				dmabuf->count = dmabuf->total_bytes = 0;
				spinunlock_restore(&state->device->reg_lock, flags);
			}
			return 0;
		}

		case SNDCTL_DSP_SPEED:
		{
			get_user(val, (int *)pArg);
			if (val >= 8000 && val <= 48000) {
				if (state->direction == 0) {
					ymf_wait_dac(state);
					dmabuf = &state->wpcm.dmabuf;
					spinlock_cli(&state->device->reg_lock, flags);
					dmabuf->ready = 0;
					state->format.rate = val;
					ymf_pcm_update_shift(&state->format);
					spinunlock_restore(&state->device->reg_lock, flags);
				}
				if (state->direction == 1) {
					ymf_stop_adc(state);
					dmabuf = &state->rpcm.dmabuf;
					spinlock_cli(&state->device->reg_lock, flags);
					dmabuf->ready = 0;
					state->format.rate = val;
					ymf_pcm_update_shift(&state->format);
					spinunlock_restore(&state->device->reg_lock, flags);
				}
			}
			put_user(state->format.rate, (int *)pArg);
			return 0;
		}

		case SNDCTL_DSP_STEREO:
		{
			get_user(val, (int *)pArg);
			if (state->direction == 0) {
				ymf_wait_dac(state); 
				dmabuf = &state->wpcm.dmabuf;
				spinlock_cli(&state->device->reg_lock, flags);
				dmabuf->ready = 0;
				state->format.voices = val ? 2 : 1;
				ymf_pcm_update_shift(&state->format);
				spinunlock_restore(&state->device->reg_lock, flags);
			}
			if (state->direction == 1) {
				ymf_stop_adc(state);
				dmabuf = &state->rpcm.dmabuf;
				spinlock_cli(&state->device->reg_lock, flags);
				dmabuf->ready = 0;
				state->format.voices = val ? 2 : 1;
				ymf_pcm_update_shift(&state->format);
				spinunlock_restore(&state->device->reg_lock, flags);
			}
			return 0;
		}

		case SNDCTL_DSP_CHANNELS:
		{
			get_user(val, (int *)pArg);
			if (val != 0) {
				if (state->direction == 0) {
					ymf_wait_dac(state);
					if (val == 1 || val == 2) {
						spinlock_cli(&state->device->reg_lock, flags);
						dmabuf = &state->wpcm.dmabuf;
						dmabuf->ready = 0;
						state->format.voices = val;
						ymf_pcm_update_shift(&state->format);
						spinunlock_restore(&state->device->reg_lock, flags);
					}
				}
				if (state->direction == 1) {
					ymf_stop_adc(state);
					if (val == 1 || val == 2) {
						spinlock_cli(&state->device->reg_lock, flags);
						dmabuf = &state->rpcm.dmabuf;
						dmabuf->ready = 0;
						state->format.voices = val;
						ymf_pcm_update_shift(&state->format);
						spinunlock_restore(&state->device->reg_lock, flags);
					}
				}
			}
			put_user(state->format.voices, (int *)pArg);
			return 0;
		}

		case SNDCTL_DSP_GETFMTS:
		{
			put_user(AFMT_S16_LE|AFMT_U8, (int *)pArg);
			return 0;
		}

		case SNDCTL_DSP_SETFMT:
		{
			get_user(val, (int *)pArg);
			if (val == AFMT_S16_LE || val == AFMT_U8) {
				if (state->direction == 0) {
					ymf_wait_dac(state);
					dmabuf = &state->wpcm.dmabuf;
					spinlock_cli(&state->device->reg_lock, flags);
					dmabuf->ready = 0;
					state->format.format = val;
					ymf_pcm_update_shift(&state->format);
					spinunlock_restore(&state->device->reg_lock, flags);
				}
				if (state->direction == 1) {
					ymf_stop_adc(state);
					dmabuf = &state->rpcm.dmabuf;
					spinlock_cli(&state->device->reg_lock, flags);
					dmabuf->ready = 0;
					state->format.format = val;
					ymf_pcm_update_shift(&state->format);
					spinunlock_restore(&state->device->reg_lock, flags);
				}
			}
			put_user(state->format.format, (int *)pArg);
			return 0;
		}

		case SNDCTL_DSP_POST:
		{
			spinlock_cli(&state->device->reg_lock, flags);
			if (state->wpcm.dmabuf.count != 0 && !state->wpcm.running) {
				ymf_start_dac(state);
			}
			spinunlock_restore(&state->device->reg_lock, flags);
			return 0;
		}

		case SNDCTL_DSP_GETOSPACE:
		{
			if (state->direction != 0)
				return -EINVAL;
			dmabuf = &state->wpcm.dmabuf;
			if (!dmabuf->ready && (val = prog_dmabuf(state, 0)) != 0)
				return val;
			redzone = ymf_calc_lend(state->format.rate);
			redzone <<= state->format.shift;
			redzone *= 3;
			spinlock_cli(&state->device->reg_lock, flags);
			abinfo.fragsize = dmabuf->fragsize;
			abinfo.bytes = dmabuf->dmasize - dmabuf->count - redzone;
			abinfo.fragstotal = dmabuf->numfrag;
			abinfo.fragments = abinfo.bytes >> dmabuf->fragshift;
			spinunlock_restore(&state->device->reg_lock, flags);
			copy_to_user((void *)pArg, &abinfo, sizeof(abinfo));
			return 0;
		}

		case SNDCTL_DSP_GETISPACE:
		{
			if (state->direction != 1)
				return -EINVAL;
			dmabuf = &state->rpcm.dmabuf;
			if (!dmabuf->ready && (val = prog_dmabuf(state, 1)) != 0)
				return val;
			spinlock_cli(&state->device->reg_lock, flags);
			abinfo.fragsize = dmabuf->fragsize;
			abinfo.bytes = dmabuf->count;
			abinfo.fragstotal = dmabuf->numfrag;
			abinfo.fragments = abinfo.bytes >> dmabuf->fragshift;
			spinunlock_restore(&state->device->reg_lock, flags);
			copy_to_user((void *)pArg, &abinfo, sizeof(abinfo));
			return 0;
		}

		case SNDCTL_DSP_GETBLKSIZE:
		{
			if (state->direction == 0) {
				if ((val = prog_dmabuf(state, 0)))
					return val;
				val = state->wpcm.dmabuf.fragsize;
				put_user(val, (int *)pArg);
				return 0;
			}
			if (state->direction == 1) {
				if ((val = prog_dmabuf(state, 1)))
					return val;
				val = state->rpcm.dmabuf.fragsize;
				put_user(val, (int *)pArg);
				return 0;
			}
			return -EINVAL;
		}

		case SNDCTL_DSP_SETFRAGMENT:
		{
			get_user(val, (int *)pArg);
			dmabuf = &state->wpcm.dmabuf;
			dmabuf->ossfragshift = val & 0xffff;
			dmabuf->ossmaxfrags = (val >> 16) & 0xffff;
			if (dmabuf->ossfragshift < 4)
				dmabuf->ossfragshift = 4;
			if (dmabuf->ossfragshift > 15)
				dmabuf->ossfragshift = 15;
			return 0;
		}

		case SOUND_PCM_READ_RATE:
		{
			put_user(state->format.rate, (int *)pArg);
			return 0;
		}

		case SOUND_PCM_READ_CHANNELS:
		{
			put_user(state->format.voices, (int *)pArg);
			return 0;
		}

		case SOUND_PCM_READ_BITS:
		{
			put_user(AFMT_S16_LE, (int *)pArg);
			return 0;
		}

		case SNDCTL_DSP_GETODELAY:
		{
			put_user(0, (int *)pArg);
			return 0;
		}

		case IOCTL_GET_USERSPACE_DRIVER:
		{
			memcpy_to_user( pArg, "oss.so", strlen( "oss.so" ) );
			return( 0 );
		}

		default:
			return -EINVAL;
	}
}

DeviceOperations_s ymfpci_dsp_fops = {
	ymfpci_open,
	ymfpci_close,
	ymfpci_ioctl,
	ymfpci_read,
	ymfpci_write
};

/* Mixer stuff */

status_t ymfpci_mixer_open( void* pNode, uint32 nFlags, void **pCookie )
{
	struct YMFDevice *device = (struct YMFDevice*)pNode;
	*pCookie = device->ac97_codec[0];

	return 0;
}

status_t ymfpci_mixer_close( void* pNode, void* pCookie )
{
	return 0;
}

status_t ymfpci_mixer_ioctl(void *pNode, void *pCookie, uint32 nCmd, void *pArgs, bool bFromKernel )
{
	struct ac97_codec *codec = (struct ac97_codec *)pCookie;
	struct YMFDevice *device = (struct YMFDevice*)pNode;
	unsigned long arg = (unsigned long)pArgs;
	if (nCmd == SOUND_MIXER_INFO) {
		mixer_info info;
		strncpy(info.name, device->name, sizeof(info.name));
		if (copy_to_user((void *)arg, &info, sizeof(info)))
			return -EFAULT;
		return 0;
	}
	return codec->mixer_ioctl(codec, nCmd, arg);
}

DeviceOperations_s ymfpci_mixer_fops = {
	ymfpci_mixer_open,
	ymfpci_mixer_close,
	ymfpci_mixer_ioctl,
	NULL,
	NULL
};

/* Driver management */

struct YMFDeviceID
{
	int nDeviceID;
	char zName[OS_NAME_LENGTH];
};

static struct YMFDeviceID aYMFDevices[] = {
	{ PCI_DEVICE_ID_YAMAHA_724, "Yamaha YMF724" },
	{ PCI_DEVICE_ID_YAMAHA_724F, "Yamaha YMF724F" },
	{ PCI_DEVICE_ID_YAMAHA_740, "Yamaha YMF740" },
	{ PCI_DEVICE_ID_YAMAHA_740C, "Yamaha YMF740C" },
	{ PCI_DEVICE_ID_YAMAHA_744, "Yamaha YMF744" },
	{ PCI_DEVICE_ID_YAMAHA_754, "Yamaha YMF754" },
	{ 0, "" },
};

status_t ymfpci_init_one( PCI_bus_s *psBus, PCI_Info_s *psPCIDevice, struct YMFDevice **psInstanceData )
{
	uint16 ctrl, nCmd;
	struct YMFDevice *device = NULL;
	int err;

	if ((device = kmalloc(sizeof(struct YMFDevice), MEMF_KERNEL)) == NULL) {
		kerndbg(KERN_WARNING, "Out of memory\n");
		return -ENOMEM;
	}
	memset(device, 0, sizeof(struct YMFDevice));

	device->base = psPCIDevice->u.h0.nBase0 & PCI_ADDRESS_MEMORY_32_MASK;

	spinlock_init(&device->reg_lock, "ymfpci_registers");
	spinlock_init(&device->voice_lock, "ymfpci_voice");
	spinlock_init(&device->ac97_lock, "ymfpci_ac97");
	device->open_sem = create_semaphore( "ymfpci_sem", 1, 0 );
	INIT_LIST_HEAD(&device->states);
	device->pcidev = psPCIDevice;

	device->rev = psBus->read_pci_config( psPCIDevice->nBus,
										  psPCIDevice->nDevice,
										  psPCIDevice->nFunction,
										  PCI_REVISION,
										  1 );

	device->reg_area = create_area("ymfpci", (void**)&device->reg_addr, 0x8000, 0x8000, AREA_ANY_ADDRESS | AREA_KERNEL, AREA_FULL_LOCK);
	if( device->reg_area < 0 )
	{
		kerndbg( KERN_WARNING, "Unable to create register area\n");
		goto out_free;
	}

	if( remap_area( device->reg_area, (void*)device->base ) < 0 )
	{
		kerndbg( KERN_WARNING, "Unable to remap register area\n");
		goto out_release_area;
	}

	/* Enable MMIO */
	nCmd = psBus->read_pci_config( psPCIDevice->nBus,
								   psPCIDevice->nDevice,
								   psPCIDevice->nFunction,
								   PCI_COMMAND,
								   2 );
	if( nCmd != ( nCmd | PCI_COMMAND_MEMORY ) )
	{
		kerndbg( KERN_INFO, "Enabling MMIO\n");
		psBus->write_pci_config( psPCIDevice->nBus,
								 psPCIDevice->nDevice,
								 psPCIDevice->nFunction,
								 PCI_COMMAND,
								 2,
								 nCmd | PCI_COMMAND_MEMORY );
	}
	psBus->enable_pci_master( psPCIDevice->nBus, psPCIDevice->nDevice, psPCIDevice->nFunction );

	/* Reset and intialise device */
	ymfpci_aclink_reset( psPCIDevice, psBus );
	if (ymfpci_codec_ready(device, 0) < 0)
		goto out_release_area;

	ymfpci_download_image(device);

	if (ymfpci_memalloc(device) < 0)
		goto out_disable_dsp;
	ymf_memload(device);

	device->irq = psPCIDevice->u.h0.nInterruptLine;
	device->irq_handle = request_irq( device->irq, ymfpci_interrupt, NULL, SA_SHIRQ, "ymfpci", (void*)device );
	if( device->irq_handle < 0 )
	{
		kerndbg( KERN_WARNING, "Unable to claim IRQ %i\n", device->irq );
		goto out_memfree;
	}
	enable_irq(device->irq);

	/*
	 * Poke just the primary for the moment.
	 */
	if ((err = ymf_ac97_init(device, 0)) != 0)
		kerndbg(KERN_WARNING, "Failed to intialise DSP\n");

	*psInstanceData = device;
	return 0;

 out_memfree:
	ymfpci_memfree(device);
 out_disable_dsp:
	ymfpci_disable_dsp(device);
	ctrl = ymfpci_readw(device, YDSXGR_GLOBALCTRL);
	ymfpci_writew(device, YDSXGR_GLOBALCTRL, ctrl & ~0x0007);
	ymfpci_writel(device, YDSXGR_STATUS, ~0);
 out_release_area:
	delete_area( device->reg_area );
 out_free:
	if (device->ac97_codec[0])
		ac97_release_codec(device->ac97_codec[0]);
	return -ENODEV;
}

status_t device_init( int nDeviceID )
{
	int i,j;
	bool bFound = false;
	PCI_Info_s sDevice;
	PCI_bus_s *psBus = get_busmanager( PCI_BUS_NAME, PCI_BUS_VERSION );
	struct YMFDevice *psInstanceData;

	if( NULL == psBus )
		return -ENODEV;		/* No PCI bus means no PCI devices */

	/* Check each PCI device for Yamaha */
	for( i = 0; psBus->get_pci_info( &sDevice, i ) == 0; ++i )
	{
		if( sDevice.nVendorID == PCI_VENDOR_ID_YAMAHA )
		{
			/* See if this is a supported device */
			for( j = 0;; ++j )
			{
				if( aYMFDevices[j].nDeviceID == 0 )
					break;

				if( sDevice.nDeviceID == aYMFDevices[j].nDeviceID )
				{
					/* We have a Yahama 7xx device */
					if( ymfpci_init_one( psBus, &sDevice, &psInstanceData ) < 0 )
						break;

					/* Claim our now functional device from the PCI bus manager */
					if( claim_device( nDeviceID, sDevice.nHandle, aYMFDevices[j].zName, DEVICE_AUDIO ) < 0 )
						break;

					/* Create device nodes */
					if( create_device_node( nDeviceID, sDevice.nHandle, "audio/ymfpci", &ymfpci_dsp_fops, (void*)psInstanceData ) < 0 )
					{
						kerndbg( KERN_WARNING, "Failed to create device node audio/ymfpci\n" );
						break;
					}

					if( create_device_node( nDeviceID, sDevice.nHandle, "audio/mixer/ymfpci", &ymfpci_mixer_fops, (void*)psInstanceData ) < 0 )
					{
						kerndbg( KERN_WARNING, "Failed to create device node audio/mixer/ymfpci\n" );
						break;
					}

					psInstanceData->name = aYMFDevices[j].zName;

					/* Log what we've got */
					kerndbg( KERN_INFO, "%s found at 0x%4lx\n", aYMFDevices[j].zName,
						sDevice.u.h0.nBase0 & PCI_ADDRESS_MEMORY_32_MASK );
					bFound = true;
					break;
				}
			}
		}
	}

	if( !bFound )
	{
		/* Well, no Yamaha YMF7xx devices on this computer */
		disable_device( nDeviceID );
		return -EINVAL;
	}

	return (0);	
}

