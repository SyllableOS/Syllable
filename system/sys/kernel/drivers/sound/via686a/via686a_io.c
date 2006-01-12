/*
 *  via686a_io.c: Support for VIA 82Cxxx Audio Codecs
 *
 *  Copyright (C) 2001	Arno Klenke
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *  Acknowledgements
 *  ----------------
 *  This driver makes extensive use of the via82cxxx driver code
 *  for Linux written by Jeff Garzik
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
#include <atheos/linux_compat.h>
#include <macros.h>

#include "ac97_codec.h"
#include "via686a.h"



/****************************************************************
 *
 * Low-level base 0 register read/write helpers
 *
 *
 */

/**
 *	via_chan_stop - Terminate DMA on specified PCM channel
 *	@iobase: PCI base address for SGD channel registers
 *
 *	Terminate scatter-gather DMA operation for given
 *	channel (derived from @iobase), if DMA is active.
 *
 *	Note that @iobase is not the PCI base address,
 *	but the PCI base address plus an offset to
 *	one of three PCM channels supported by the chip.
 *
 */
 
void via_chan_stop (int iobase)
{
	if (inb (iobase + VIA_PCM_STATUS) & VIA_SGD_ACTIVE)
		outb (VIA_SGD_TERMINATE, iobase + VIA_PCM_CONTROL);
}


/**
 *	via_chan_status_clear - Clear status flags on specified DMA channel
 *	@iobase: PCI base address for SGD channel registers
 *
 *	Clear any pending status flags for the given
 *	DMA channel (derived from @iobase), if any
 *	flags are asserted.
 *
 *	Note that @iobase is not the PCI base address,
 *	but the PCI base address plus an offset to
 *	one of three PCM channels supported by the chip.
 *
 */
 
inline void via_chan_status_clear (int iobase)
{
	u8 tmp = inb (iobase + VIA_PCM_STATUS);
	
	if (tmp != 0)
		outb (tmp, iobase + VIA_PCM_STATUS);
}


/**
 *	sg_begin - Begin recording or playback on a PCM channel
 *	@chan: Channel for which DMA operation shall begin
 *
 *	Start scatter-gather DMA for the given channel.
 *
 */
 
inline void sg_begin (struct via_channel *chan)
{
	outb (VIA_SGD_START, chan->iobase + VIA_PCM_CONTROL);
}

/****************************************************************
 *
 * Miscellaneous debris
 *
 *
 */


/**
 *	via_syscall_down - down the card-specific syscell semaphore
 *	@card: Private info for specified board
 *	@nonblock: boolean, non-zero if O_NONBLOCK is set
 *
 *	Encapsulates standard method of acquiring the syscall sem.
 *
 *	Returns negative errno on error, or zero for success.
 */

int via_syscall_down (struct via_info *card, int nonblock)
{
	if (nonblock) {
		if (is_semaphore_locked(card->syscall_sem))
			return -EAGAIN;
		else
			LOCK(card->syscall_sem);
	} else 
		LOCK(card->syscall_sem);

	return 0;
}


/**
 *	via_stop_everything - Stop all audio operations
 *	@card: Private info for specified board
 *
 *	Stops all DMA operations and interrupts, and clear
 *	any pending status bits resulting from those operations.
 */

void via_stop_everything (struct via_info *card)
{
	DPRINTK ("ENTER\n");

	assert (card != NULL);

	/*
	 * terminate any existing operations on audio read/write channels
	 */
	via_chan_stop (card->baseaddr + VIA_BASE0_PCM_OUT_CHAN);
	via_chan_stop (card->baseaddr + VIA_BASE0_PCM_IN_CHAN);
	via_chan_stop (card->baseaddr + VIA_BASE0_FM_OUT_CHAN);

	/*
	 * clear any existing stops / flags (sanity check mainly)
	 */
	via_chan_status_clear (card->baseaddr + VIA_BASE0_PCM_OUT_CHAN);
	via_chan_status_clear (card->baseaddr + VIA_BASE0_PCM_IN_CHAN);
	via_chan_status_clear (card->baseaddr + VIA_BASE0_FM_OUT_CHAN);

	/*
	 * clear any enabled interrupt bits, reset to 8-bit mono PCM mode
	 */
	outb (0, card->baseaddr + VIA_BASE0_PCM_OUT_CHAN_TYPE);
	outb (0, card->baseaddr + VIA_BASE0_PCM_IN_CHAN_TYPE);
	outb (0, card->baseaddr + VIA_BASE0_FM_OUT_CHAN_TYPE);
	DPRINTK ("EXIT\n");
}

/**
 *	via_set_rate - Set PCM rate for given channel
 *	@card: Private info for specified board
 *	@rate: Desired PCM sample rate, in Khz
 *	@inhale_deeply: Boolean.  If non-zero (true), the recording sample rate
 *			is set.  If zero (false), the playback sample rate
 *			is set.
 *
 *	Sets the PCM sample rate for a channel.
 *
 *	Values for @rate are clamped to a range of 4000 Khz through 48000 Khz,
 *	due to hardware constraints.
 */

int via_set_rate (struct ac97_codec *ac97,
			 struct via_channel *chan, unsigned rate)
{
	struct via_info *card = ac97->private_data;
	int rate_reg;

	DPRINTK ("ENTER, rate = %d\n", rate);

	if (card->locked_rate) {
		chan->rate = 48000;
		goto out;
	}

	if (rate > 48000)		rate = 48000;
	if (rate < 4000) 		rate = 4000;

	rate_reg = chan->is_record ? AC97_PCM_LR_ADC_RATE :
			    AC97_PCM_FRONT_DAC_RATE;

	via_ac97_write_reg (ac97, AC97_POWER_CONTROL,
		(via_ac97_read_reg (ac97, AC97_POWER_CONTROL) & ~0x0200) |
		0x0200);

	via_ac97_write_reg (ac97, rate_reg, rate);

	via_ac97_write_reg (ac97, AC97_POWER_CONTROL,
		via_ac97_read_reg (ac97, AC97_POWER_CONTROL) & ~0x0200);

	udelay (10);

	/* the hardware might return a value different than what we
	 * passed to it, so read the rate value back from hardware
	 * to see what we came up with
	 */
	chan->rate = via_ac97_read_reg (ac97, rate_reg);

	if (chan->rate == 0) {
		card->locked_rate = 1;
		chan->rate = 48000;
		printk (KERN_WARNING PFX "Codec rate locked at 48Khz\n");
	}

out:
	DPRINTK ("EXIT, returning rate %d Hz\n", chan->rate);
	return chan->rate;
}

/****************************************************************
 *
 * Channel-specific operations
 *
 *
 */


/**
 *	via_chan_init_defaults - Initialize a struct via_channel
 *	@card: Private audio chip info
 *	@chan: Channel to be initialized
 *
 *	Zero @chan, and then set all static defaults for the structure.
 */

void via_chan_init_defaults (struct via_info *card, struct via_channel *chan)
{
	memset (chan, 0, sizeof (*chan));

	if (chan == &card->ch_out) {
		chan->name = "PCM-OUT";
		chan->iobase = card->baseaddr + VIA_BASE0_PCM_OUT_CHAN;
	} else if (chan == &card->ch_in) {
		chan->name = "PCM-IN";
		chan->iobase = card->baseaddr + VIA_BASE0_PCM_IN_CHAN;
		chan->is_record = 1;
	} else if (chan == &card->ch_fm) {
		chan->name = "PCM-OUT-FM";
		chan->iobase = card->baseaddr + VIA_BASE0_FM_OUT_CHAN;
	} else {
		return;
	}

	//init_waitqueue_head (&chan->wait);
	chan->wait = create_semaphore("via_chan_wait", 0, 0);

	chan->pcm_fmt = VIA_PCM_FMT_MASK;
	chan->is_enabled = 1;

	chan->frag_number = 0;
        chan->frag_size = 0;
	atomic_set(&chan->n_frags, 0);
	atomic_set (&chan->hw_ptr, 0);
}
 
/**
 *      via_chan_init - Initialize PCM channel
 *      @card: Private audio chip info
 *      @chan: Channel to be initialized
 *
 *      Performs some of the preparations necessary to begin
 *      using a PCM channel.
 *
 *      Currently the preparations consist in 
 *      setting the
 *      PCM channel to a known state.
 */

void via_chan_init (struct via_info *card, struct via_channel *chan)
{

        DPRINTK ("ENTER\n");

	/* bzero channel structure, and init members to defaults */
        via_chan_init_defaults (card, chan);

        /* stop any existing channel output */
        via_chan_clear (card, chan);
        via_chan_status_clear (chan->iobase);
        via_chan_pcm_fmt (chan, 1);

	DPRINTK ("EXIT\n");
}

/**
 *	via_chan_buffer_init - Initialize PCM channel buffer
 *	@card: Private audio chip info
 *	@chan: Channel to be initialized
 *
 *	Performs some of the preparations necessary to begin
 *	using a PCM channel.
 *
 *	Currently the preparations include allocating the
 *	scatter-gather DMA table and buffers,
 *	and passing the
 *	address of the DMA table to the hardware.
 *
 *	Note that special care is taken when passing the
 *	DMA table address to hardware, because it was found
 *	during driver development that the hardware did not
 *	always "take" the address.
 */

int via_chan_buffer_init (struct via_info *card, struct via_channel *chan)
{
	int page, offset;
	int i;

	DPRINTK ("ENTER\n");

	if (chan->sgtable != NULL) {
		DPRINTK ("EXIT\n");
		return 0;
	}

	/* alloc DMA-able memory for scatter-gather table */
	chan->sgtable = pci_alloc_consistent (card->pdev,
		(sizeof (struct via_sgd_table) * chan->frag_number),
		&chan->sgt_handle);
	if (!chan->sgtable) {
		printk (KERN_ERR PFX "DMA table alloc fail, aborting\n");
		DPRINTK ("EXIT\n");
		return -ENOMEM;
	}

	memset ((void*)chan->sgtable, 0,
		(sizeof (struct via_sgd_table) * chan->frag_number));

	/* alloc DMA-able memory for scatter-gather buffers */

	chan->page_number = (chan->frag_number * chan->frag_size) / PAGE_SIZE + 
			    (((chan->frag_number * chan->frag_size) % PAGE_SIZE) ? 1 : 0);

	for (i = 0; i < chan->page_number; i++) {
		chan->pgtbl[i].cpuaddr = pci_alloc_consistent (card->pdev, PAGE_SIZE,
					      &chan->pgtbl[i].handle);

		if (!chan->pgtbl[i].cpuaddr) {
			chan->page_number = i;
			goto err_out_nomem;
		}

        memset (chan->pgtbl[i].cpuaddr, 0xBC, chan->frag_size);

                DPRINTK ("dmabuf_pg #%d (h=%lx, v2p=%lx, a=%p)\n",
			i, (long)chan->pgtbl[i].handle,
			virt_to_phys(chan->pgtbl[i].cpuaddr),
			chan->pgtbl[i].cpuaddr);
	}

	for (i = 0; i < chan->frag_number; i++) {

		page = i / (PAGE_SIZE / chan->frag_size);
		offset = (i % (PAGE_SIZE / chan->frag_size)) * chan->frag_size;

		chan->sgtable[i].count = cpu_to_le32 (chan->frag_size | VIA_FLAG);
		chan->sgtable[i].addr = cpu_to_le32 (chan->pgtbl[page].handle + offset);

		DPRINTK ("dmabuf #%d (32(h)=%lx)\n",
			 i,
			 (long)chan->sgtable[i].addr);
	}	

	/* overwrite the last buffer information */
	chan->sgtable[chan->frag_number - 1].count = cpu_to_le32 (chan->frag_size | VIA_EOL);

	/* set location of DMA-able scatter-gather info table */
	DPRINTK ("outl (0x%X, 0x%04lX)\n",
		cpu_to_le32 (chan->sgt_handle),
		chan->iobase + VIA_PCM_TABLE_ADDR);

	via_ac97_wait_idle (card);
	outl (cpu_to_le32 (chan->sgt_handle),
	      chan->iobase + VIA_PCM_TABLE_ADDR);
	udelay (20);
	via_ac97_wait_idle (card);

	DPRINTK ("inl (0x%lX) = %x\n",
		chan->iobase + VIA_PCM_TABLE_ADDR,
		inl(chan->iobase + VIA_PCM_TABLE_ADDR));

	DPRINTK ("EXIT\n");
	return 0;

err_out_nomem:
	printk (KERN_ERR PFX "DMA buffer alloc fail, aborting\n");
	via_chan_buffer_free (card, chan);
	DPRINTK ("EXIT\n");
	return -ENOMEM;
}


/**
 *	via_chan_free - Release a PCM channel
 *	@card: Private audio chip info
 *	@chan: Channel to be released
 *
 *	Performs all the functions necessary to clean up
 *	an initialized channel.
 *
 *	Currently these functions include disabled any
 *	active DMA operations, setting the PCM channel
 *	back to a known state, and releasing any allocated
 *	sound buffers.
 */
 
void via_chan_free (struct via_info *card, struct via_channel *chan)
{
	
	DPRINTK ("ENTER\n");
	
	//synchronize_irq();
	spin_lock_irq (&card->lock);

	/* stop any existing channel output */
	via_chan_stop (chan->iobase);
	via_chan_status_clear (chan->iobase);
	via_chan_pcm_fmt (chan, 1);

	spin_unlock_irq (&card->lock);

	DPRINTK ("EXIT\n");
}

void via_chan_buffer_free (struct via_info *card, struct via_channel *chan)
{
	int i;

	DPRINTK ("ENTER\n");

	/* zero location of DMA-able scatter-gather info table */
	via_ac97_wait_idle(card);
	outl (0, chan->iobase + VIA_PCM_TABLE_ADDR);

	for (i = 0; i < chan->page_number; i++)
		if (chan->pgtbl[i].cpuaddr) {
			pci_free_consistent (card->pdev, PAGE_SIZE,
					     chan->pgtbl[i].cpuaddr,
					     chan->pgtbl[i].handle);
			chan->pgtbl[i].cpuaddr = NULL;
			chan->pgtbl[i].handle = 0;
		}

	chan->page_number = 0;

	if (chan->sgtable) {
		pci_free_consistent (card->pdev,
			(sizeof (struct via_sgd_table) * chan->frag_number),
			(void*)chan->sgtable, chan->sgt_handle);
		chan->sgtable = NULL;
	}

	DPRINTK ("EXIT\n");
}

/**
 *	via_chan_pcm_fmt - Update PCM channel settings
 *	@card: Private audio chip info
 *	@chan: Channel to be updated
 *	@reset: Boolean.  If non-zero, channel will be reset
 *		to 8-bit mono mode.
 *
 *	Stores the settings of the current PCM format,
 *	8-bit or 16-bit, and mono/stereo, into the
 *	hardware settings for the specified channel.
 *	If @reset is non-zero, the channel is reset
 *	to 8-bit mono mode.  Otherwise, the channel
 *	is set to the values stored in the channel
 *	information struct @chan.
 */
 
 void via_chan_pcm_fmt (struct via_channel *chan, int reset)
{
	DPRINTK ("ENTER, pcm_fmt=0x%02X, reset=%s\n",
		 chan->pcm_fmt, reset ? "yes" : "no");

	assert (chan != NULL);

	if (reset)
		/* reset to 8-bit mono mode */
		chan->pcm_fmt = 0;
	
	/* enable interrupts on FLAG and EOL */
	chan->pcm_fmt |= VIA_CHAN_TYPE_MASK;

	/* if we are recording, enable recording fifo bit */
	if (chan->is_record)
		chan->pcm_fmt |= VIA_PCM_REC_FIFO;
	/* set interrupt select bits where applicable (PCM & FM out channels) */
	if (!chan->is_record)
		chan->pcm_fmt |= VIA_CHAN_TYPE_INT_SELECT;

	outb (chan->pcm_fmt, chan->iobase + VIA_PCM_TYPE);

	DPRINTK ("EXIT, pcm_fmt = 0x%02X, reg = 0x%02X\n",
		 chan->pcm_fmt,
		 inb (chan->iobase + VIA_PCM_TYPE));
}


/**
 *	via_chan_clear - Stop DMA channel operation, and reset pointers
 *	@chan: Channel to be cleared
 *
 *	Call via_chan_stop to halt DMA operations, and then resets
 *	all software pointers which track DMA operation.
 */

void via_chan_clear (struct via_info *card, struct via_channel *chan)
{
	DPRINTK ("ENTER\n");
	via_chan_stop (chan->iobase);
	via_chan_buffer_free(card, chan);
	chan->is_active = 0;
	chan->is_mapped = 0;
	chan->is_enabled = 1;
	chan->slop_len = 0;
	chan->sw_ptr = 0;
	chan->n_irqs = 0;
	atomic_set (&chan->hw_ptr, 0);
	DPRINTK ("EXIT\n");
}


/**
 *	via_chan_set_speed - Set PCM sample rate for given channel
 *	@card: Private info for specified board
 *	@chan: Channel whose sample rate will be adjusted
 *	@val: New sample rate, in Khz
 *
 *	Helper function for the %SNDCTL_DSP_SPEED ioctl.  OSS semantics
 *	demand that all audio operations halt (if they are not already
 *	halted) when the %SNDCTL_DSP_SPEED is given.
 *
 *	This function halts all audio operations for the given channel
 *	@chan, and then calls via_set_rate to set the audio hardware
 *	to the new rate.
 */

int via_chan_set_speed (struct via_info *card,
			       struct via_channel *chan, int val)
{
	DPRINTK ("ENTER, requested rate = %d\n", val);

	via_chan_clear (card, chan);

	val = via_set_rate (&card->ac97, chan, val);	
	
	DPRINTK ("EXIT, returning %d\n", val);
	return val;
}


/**
 *	via_chan_set_fmt - Set PCM sample size for given channel
 *	@card: Private info for specified board
 *	@chan: Channel whose sample size will be adjusted
 *	@val: New sample size, use the %AFMT_xxx constants
 *
 *	Helper function for the %SNDCTL_DSP_SETFMT ioctl.  OSS semantics
 *	demand that all audio operations halt (if they are not already
 *	halted) when the %SNDCTL_DSP_SETFMT is given.
 *
 *	This function halts all audio operations for the given channel
 *	@chan, and then calls via_chan_pcm_fmt to set the audio hardware
 *	to the new sample size, either 8-bit or 16-bit.
 */

 int via_chan_set_fmt (struct via_info *card,
			     struct via_channel *chan, int val)
{
	DPRINTK ("ENTER, val=%s\n",
		 val == AFMT_U8 ? "AFMT_U8" :
	 	 val == AFMT_S16_LE ? "AFMT_S16_LE" :
		 "unknown");

	via_chan_clear (card, chan);
	
	assert (val != AFMT_QUERY); /* this case is handled elsewhere */
	
	switch (val) {
	case AFMT_S16_LE:
		if ((chan->pcm_fmt & VIA_PCM_FMT_16BIT) == 0) {
			chan->pcm_fmt |= VIA_PCM_FMT_16BIT;
			via_chan_pcm_fmt (chan, 0);
		}
		break;

	case AFMT_U8:
		if (chan->pcm_fmt & VIA_PCM_FMT_16BIT) {
			chan->pcm_fmt &= ~VIA_PCM_FMT_16BIT;
			via_chan_pcm_fmt (chan, 0);
		}
		break;

	default:
		DPRINTK ("unknown AFMT: 0x%X\n", val);
		val = AFMT_S16_LE;
	}
	
	DPRINTK ("EXIT\n");
	return val;
}


/**
 *	via_chan_set_stereo - Enable or disable stereo for a DMA channel
 *	@card: Private info for specified board
 *	@chan: Channel whose stereo setting will be adjusted
 *	@val: New sample size, use the %AFMT_xxx constants
 *
 *	Helper function for the %SNDCTL_DSP_CHANNELS and %SNDCTL_DSP_STEREO ioctls.  OSS semantics
 *	demand that all audio operations halt (if they are not already
 *	halted) when %SNDCTL_DSP_CHANNELS or SNDCTL_DSP_STEREO is given.
 *
 *	This function halts all audio operations for the given channel
 *	@chan, and then calls via_chan_pcm_fmt to set the audio hardware
 *	to enable or disable stereo.
 */

 int via_chan_set_stereo (struct via_info *card,
			        struct via_channel *chan, int val)
{
	DPRINTK ("ENTER, channels = %d\n", val);

	via_chan_clear (card, chan);
	
	switch (val) {

	/* mono */
	case 1:
		chan->pcm_fmt &= ~VIA_PCM_FMT_STEREO;
		via_chan_pcm_fmt (chan, 0);
		break;

	/* stereo */
	case 2:
		chan->pcm_fmt |= VIA_PCM_FMT_STEREO;
		via_chan_pcm_fmt (chan, 0);
		break;

	/* unknown */
	default:
		printk (KERN_WARNING PFX "unknown number of channels\n");
		val = -EINVAL;
		break;
	}
	
	DPRINTK ("EXIT, returning %d\n", val);
	return val;
}

int via_chan_set_buffering (struct via_info *card,
                                struct via_channel *chan, int val)
{
	int shift;

    DPRINTK ("ENTER\n");

	/* in both cases the buffer cannot be changed */
	if (chan->is_active || chan->is_mapped) { 
		DPRINTK ("EXIT\n");
		return -EINVAL;
	}

	/* called outside SETFRAGMENT */
	/* set defaults or do nothing */
	if (val < 0) {

		if (chan->frag_size && chan->frag_number)
			goto out;

		DPRINTK ("\n");

		chan->frag_size = (VIA_DEFAULT_FRAG_TIME * chan->rate *
				   ((chan->pcm_fmt & VIA_PCM_FMT_STEREO) ? 2 : 1) *
				   ((chan->pcm_fmt & VIA_PCM_FMT_16BIT) ? 2 : 1)) / 1000 - 1;

		shift = 0;
		while (chan->frag_size) {
			chan->frag_size >>= 1;
			shift++;
		}
		chan->frag_size = 1 << shift;

		chan->frag_number = (VIA_DEFAULT_BUFFER_TIME / VIA_DEFAULT_FRAG_TIME);

		DPRINTK ("setting default values %d %d\n", chan->frag_size, chan->frag_number);
	} else {
		chan->frag_size = 1 << (val & 0xFFFF);
		chan->frag_number = (val >> 16) & 0xFFFF;

		DPRINTK ("using user values %d %d\n", chan->frag_size, chan->frag_number);
	}

	/* quake3 wants frag_number to be a power of two */
	shift = 0;
	while (chan->frag_number) {
		chan->frag_number >>= 1;
		shift++;
	}
	chan->frag_number = 1 << shift;

	if (chan->frag_size > VIA_MAX_FRAG_SIZE)
		chan->frag_size = VIA_MAX_FRAG_SIZE;
	else if (chan->frag_size < VIA_MIN_FRAG_SIZE)
		chan->frag_size = VIA_MIN_FRAG_SIZE;

	if (chan->frag_number < VIA_MIN_FRAG_NUMBER)
                chan->frag_number = VIA_MIN_FRAG_NUMBER;

	if ((chan->frag_number * chan->frag_size) / PAGE_SIZE > VIA_MAX_BUFFER_DMA_PAGES)
		chan->frag_number = (VIA_MAX_BUFFER_DMA_PAGES * PAGE_SIZE) / chan->frag_size;

out:
	if (chan->is_record)
		atomic_set (&chan->n_frags, 0);
	else
		atomic_set (&chan->n_frags, chan->frag_number);

	DPRINTK ("EXIT\n");

	return 0;
}

/**
 *	via_chan_flush_frag - Flush partially-full playback buffer to hardware
 *	@chan: Channel whose DMA table will be displayed
 *
 *	Flushes partially-full playback buffer to hardware.
 */

void via_chan_flush_frag (struct via_channel *chan)
{
	DPRINTK ("ENTER\n");

	assert (chan->slop_len > 0);

	if (chan->sw_ptr == (chan->frag_number - 1))
		chan->sw_ptr = 0;
	else
		chan->sw_ptr++;

	chan->slop_len = 0;

	assert (atomic_read (&chan->n_frags) > 0);
	atomic_dec (&chan->n_frags);

	DPRINTK ("EXIT\n");
}



/**
 *	via_chan_maybe_start - Initiate audio hardware DMA operation
 *	@chan: Channel whose DMA is to be started
 *
 *	Initiate DMA operation, if the DMA engine for the given
 *	channel @chan is not already active.
 */

void via_chan_maybe_start (struct via_channel *chan)
{
	if (!chan->is_active && chan->is_enabled) {
		chan->is_active = 1;
		sg_begin (chan);
		DPRINTK ("starting channel %s\n", chan->name);
	}
}


/****************************************************************
 *
 * Interface to ac97-codec module
 *
 *
 */
 
/**
 *	via_ac97_wait_idle - Wait until AC97 codec is not busy
 *	@card: Private info for specified board
 *
 *	Sleep until the AC97 codec is no longer busy.
 *	Returns the final value read from the SGD
 *	register being polled.
 */

 u8 via_ac97_wait_idle (struct via_info *card)
{
	u8 tmp8;
	int counter = VIA_COUNTER_LIMIT;
	
	DPRINTK ("ENTER/EXIT\n");

	assert (card != NULL);
	assert (card->pdev != NULL);
	
	do {
		udelay (15);

		tmp8 = inb (card->baseaddr + 0x83);
	} while ((tmp8 & VIA_CR83_BUSY) && (counter-- > 0));

	if (tmp8 & VIA_CR83_BUSY)
		printk (KERN_WARNING PFX "timeout waiting on AC97 codec\n");
	return tmp8;
}


/**
 *	via_ac97_read_reg - Read AC97 standard register
 *	@codec: Pointer to generic AC97 codec info
 *	@reg: Index of AC97 register to be read
 *
 *	Read the value of a single AC97 codec register,
 *	as defined by the Intel AC97 specification.
 *
 *	Defines the standard AC97 read-register operation
 *	required by the kernel's ac97_codec interface.
 *
 *	Returns the 16-bit value stored in the specified
 *	register.
 */

 u16 via_ac97_read_reg (struct ac97_codec *codec, u8 reg)
{
	u32 data;
	struct via_info *card;
	int counter;
	
	DPRINTK ("ENTER\n");

	assert (codec != NULL);
	assert (codec->private_data != NULL);

	card = codec->private_data;
	
	/* Every time we write to register 80 we cause a transaction.
	   The only safe way to clear the valid bit is to write it at
	   the same time as the command */
	data = (reg << 16) | VIA_CR80_READ | VIA_CR80_VALID;

	outl (data, card->baseaddr + VIA_BASE0_AC97_CTRL);
	for (counter = VIA_COUNTER_LIMIT; counter > 0; counter--) {
	        udelay (1);
		if ((((data = inl(card->baseaddr + 0x80)) &
			(VIA_CR80_VALID|VIA_CR80_BUSY)) == VIA_CR80_VALID))
			goto out;
	}

	printk (KERN_WARNING PFX "timeout while reading AC97 codec\n");
	goto err_out;

out:
	/* Once the valid bit has become set, we must wait a complete AC97
	   frame before the data has settled. */
        udelay(25);
	data = (unsigned long) inl (card->baseaddr + 0x80);

	if (((data & 0x007F0000) >> 16) == reg) {
		DPRINTK ("EXIT, success, data=0x%x, retval=0x%x\n",
			 (uint)data, (uint)data & 0x0000FFFF);
		return data & 0x0000FFFF;
	}

	DPRINTK ("WARNING: not our index: reg=0x%x, newreg=0x%x\n",
		 reg, (((uint)data & 0x007F0000) >> 16));

err_out:
	DPRINTK ("EXIT, returning 0\n");
	return 0;
}


/**
 *	via_ac97_write_reg - Write AC97 standard register
 *	@codec: Pointer to generic AC97 codec info
 *	@reg: Index of AC97 register to be written
 *	@value: Value to be written to AC97 register
 *
 *	Write the value of a single AC97 codec register,
 *	as defined by the Intel AC97 specification.
 *
 *	Defines the standard AC97 write-register operation
 *	required by the kernel's ac97_codec interface.
 */

 void via_ac97_write_reg (struct ac97_codec *codec, u8 reg, u16 value)
{
	u32 data;
	struct via_info *card;
	int counter;
	
	DPRINTK ("ENTER\n");

	assert (codec != NULL);
	assert (codec->private_data != NULL);

	card = codec->private_data;

	data = (reg << 16) + value;
	outl (data, card->baseaddr + VIA_BASE0_AC97_CTRL);
	udelay (20);

	for (counter = VIA_COUNTER_LIMIT; counter > 0; counter--) {
		if ((inb (card->baseaddr + 0x83) & VIA_CR83_BUSY) == 0)
			goto out;
		udelay(15);
	}
	
	printk (KERN_WARNING PFX "timeout after AC97 codec write\n");

out:
	DPRINTK ("EXIT\n");
}

int via_ac97_reset (struct via_info *card)
{
	PCI_Info_s* pdev = card->pdev;
	u16 tmp16;
	PCI_bus_s* psBus = get_busmanager( PCI_BUS_NAME, PCI_BUS_VERSION );
	
	
	DPRINTK ("ENTER\n");

	assert (pdev != NULL);
	

        /*
         * reset AC97 controller: enable, disable, enable
         * pause after each command for good luck
         */
      //  pci_write_config_byte (pdev, VIA_ACLINK_CTRL, VIA_CR41_AC97_ENABLE |
        //                       VIA_CR41_AC97_RESET | VIA_CR41_AC97_WAKEUP);
        psBus->write_pci_config(pdev->nBus, pdev->nDevice, pdev->nFunction, VIA_ACLINK_CTRL,
        	1, VIA_CR41_AC97_ENABLE | VIA_CR41_AC97_RESET | VIA_CR41_AC97_WAKEUP); 
        udelay (100);
 
       // pci_write_config_byte (pdev, VIA_ACLINK_CTRL, 0);
       psBus->write_pci_config(pdev->nBus, pdev->nDevice, pdev->nFunction, VIA_ACLINK_CTRL,
        	1, 0); 
        udelay (100);
 
       // pci_write_config_byte (pdev, VIA_ACLINK_CTRL,
		//	       VIA_CR41_AC97_ENABLE | VIA_CR41_PCM_ENABLE |
          //                     VIA_CR41_VRA | VIA_CR41_AC97_RESET);
       	psBus->write_pci_config(pdev->nBus, pdev->nDevice, pdev->nFunction, VIA_ACLINK_CTRL,
        	1, VIA_CR41_AC97_ENABLE | VIA_CR41_PCM_ENABLE | VIA_CR41_VRA | VIA_CR41_AC97_RESET); 
        udelay (100);


	/* route FM trap to IRQ, disable FM trap */
	//pci_write_config_byte (pdev, 0x48, 0x05);
	psBus->write_pci_config(pdev->nBus, pdev->nDevice, pdev->nFunction, 0x48, 1, 0x05); 
	udelay(10);
	
	/* disable all codec GPI interrupts */
	outl (0, (pdev->u.h0.nBase0 & PCI_ADDRESS_MEMORY_32_MASK) + 0x8C);

	/* WARNING: this line is magic.  Remove this
	 * and things break. */
	/* enable variable rate, variable rate MIC ADC */
 	/*
 	 * If we cannot enable VRA, we have a locked-rate codec.
 	 * We try again to enable VRA before assuming so, however.
 	 */
 	tmp16 = via_ac97_read_reg (&card->ac97, AC97_EXTENDED_STATUS);
 	if ((tmp16 & 1) == 0) {
 		via_ac97_write_reg (&card->ac97, AC97_EXTENDED_STATUS, tmp16 | 1);
 		tmp16 = via_ac97_read_reg (&card->ac97, AC97_EXTENDED_STATUS);
 		if ((tmp16 & 1) == 0) {
 			card->locked_rate = 1;
 			printk (KERN_WARNING PFX "Codec rate locked at 48Khz\n");
 		}
 	}

	DPRINTK ("EXIT, returning 0\n");
	return 0;
}

void via_ac97_codec_wait (struct ac97_codec *codec)
{
	assert (codec->private_data != NULL);
	via_ac97_wait_idle (codec->private_data);
}

int via_ac97_init (struct via_info *card)
{
	int rc;
	u16 tmp16;
	
	DPRINTK ("ENTER\n");

	assert (card != NULL);

	memset (&card->ac97, 0, sizeof (card->ac97));	
	card->ac97.private_data = card;
	card->ac97.codec_read = via_ac97_read_reg;
	card->ac97.codec_write = via_ac97_write_reg;
	card->ac97.codec_wait = via_ac97_codec_wait;
	
	/*card->ac97.dev_mixer = register_sound_mixer (&via_mixer_fops, -1);
	if (card->ac97.dev_mixer < 0) {
		printk (KERN_ERR PFX "unable to register AC97 mixer, aborting\n");
		DPRINTK("EXIT, returning -EIO\n");
		return -EIO;
	}*/

	rc = via_ac97_reset (card);
	if (rc) {
		printk (KERN_ERR PFX "unable to reset AC97 codec, aborting\n");
		goto err_out;
	}

	if (ac97_probe_codec (&card->ac97) == 0) {
		printk (KERN_ERR PFX "unable to probe AC97 codec, aborting\n");
		rc = -EIO;
		goto err_out;
	}

	/* enable variable rate, variable rate MIC ADC */
	tmp16 = via_ac97_read_reg (&card->ac97, AC97_EXTENDED_STATUS);
	via_ac97_write_reg (&card->ac97, AC97_EXTENDED_STATUS, tmp16 | 1);

	
	DPRINTK ("EXIT, returning 0\n");
	return 0;

err_out:
	//unregister_sound_mixer (card->ac97.dev_mixer);
	DPRINTK("EXIT, returning %d\n", rc);
	return rc;
}


void via_ac97_cleanup (struct via_info *card)
{
	DPRINTK("ENTER\n");
	
	assert (card != NULL);
	assert (card->ac97.dev_mixer >= 0);
	
	//unregister_sound_mixer (card->ac97.dev_mixer);

	DPRINTK("EXIT\n");
}


