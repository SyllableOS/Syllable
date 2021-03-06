/*
 *  Syllable driver for VIA vt82xx Audio Codecs
 *
 *  Syllable specific code is
 *  Copyright (C) 2003	Arno Klenke
 *  
 *  Other code is derived from the oss and alsa project
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
 */
 
 
#include <posix/errno.h>
#include <posix/ioctls.h>
#include <posix/fcntl.h>
#include <posix/termios.h>

#include <kernel/malloc.h>
#include <kernel/string.h>
#include <kernel/kdebug.h>
#include <kernel/areas.h>
#include <kernel/types.h>
#include <kernel/pci.h>
#include <kernel/device.h>
#include <kernel/semaphore.h>
#include <kernel/irq.h>
#include <kernel/isa_io.h>
#include <kernel/dma.h>
#include <kernel/udelay.h>
#include <kernel/spinlock.h>
#include <kernel/bitops.h>
#include <kernel/linux_compat.h>
#include <kernel/signal.h>
#include <syllable/soundcard.h>
#include <macros.h>

#include "ac97_codec.h"
#include "vt82xx.h"

/* number of cards, used for assigning unique numbers to cards */
static unsigned via_num_cards;

static void *via_driver_data;
static PCI_Info_s *via_pdev;

/****************************************************************
 *
 * Interrupt-related code
 *
 */
 
/**
 *	via_intr_channel - handle an interrupt for a single channel
 *	@chan: handle interrupt for this channel
 *
 *	This is the "meat" of the interrupt handler,
 *	containing the actions taken each time an interrupt
 *	occurs.  All communication and coordination with
 *	userspace takes place here.
 *
 *	Locking: inside card->lock
 */

void via_intr_channel (struct via_channel *chan)
{
	u8 status;
	int n;

	/* check pertinent bits of status register for action bits */
	status = inb (chan->iobase) & (VIA_SGD_FLAG | VIA_SGD_EOL | VIA_SGD_STOPPED);
	if (!status)
		return;

	/* acknowledge any flagged bits ASAP */
	outb (status, chan->iobase);

	if (!chan->sgtable) /* XXX: temporary solution */
		return;

	/* grab current h/w ptr value */
	n = atomic_read (&chan->hw_ptr);

	/* sanity check: make sure our h/w ptr doesn't have a weird value */
	assert (n >= 0);
	assert (n < chan->frag_number);

	/* reset SGD data structure in memory to reflect a full buffer,
	 * and advance the h/w ptr, wrapping around to zero if needed
	 */
	if (n == (chan->frag_number - 1)) {
		chan->sgtable[n].count = (chan->frag_size | VIA_EOL);
		atomic_set (&chan->hw_ptr, 0);
	} else {
		chan->sgtable[n].count = (chan->frag_size | VIA_FLAG);
		atomic_inc (&chan->hw_ptr);
	}

	/* accounting crap for SNDCTL_DSP_GETxPTR */
	chan->n_irqs++;
	chan->bytes += chan->frag_size;
	if (chan->bytes < 0) /* handle overflow of 31-bit value */
		chan->bytes = chan->frag_size;

	/* wake up anyone listening to see when interrupts occur */
		wakeup_sem(chan->wait, true);

	DPRINTK ("%s intr, status=0x%02X, hwptr=0x%lX, chan->hw_ptr=%d\n",
		 chan->name, status, (long) inl (chan->iobase + 0x04),
		 atomic_read (&chan->hw_ptr));

	/* If we are recording, then n_frags represents the number
	 * of fragments waiting to be handled by userspace.
	 * If we are playback, then n_frags represents the number
	 * of fragments remaining to be filled by userspace.
	 * We increment here.  If we reach max number of fragments,
	 * this indicates an underrun/overrun.  For this case under OSS,
	 * we stop the record/playback process.
	 */
	if (atomic_read (&chan->n_frags) < chan->frag_number)
		atomic_inc (&chan->n_frags);
	assert (atomic_read (&chan->n_frags) <= chan->frag_number);

	if (atomic_read (&chan->n_frags) == chan->frag_number) {
		chan->is_active = 0;
		via_chan_stop (chan->iobase);
	}

	DPRINTK ("%s intr, channel n_frags == %d\n", chan->name,
		 atomic_read (&chan->n_frags));
}


int via_interrupt(int irq, void *dev_id, SysCallRegs_s *regs )
{
	struct via_info *card = dev_id;
	/* synchronize interrupt handling under SMP.  this spinlock
	 * goes away completely on UP
	 */
	spin_lock (&card->lock);

	via_intr_channel (&card->ch_out);
	via_intr_channel (&card->ch_in);

	spin_unlock (&card->lock);
	return 0;
}

/**
 *	via_interrupt_disable - Disable all interrupt-generating sources
 *	@card: Private info for specified board
 *
 *	Disables all interrupt-generation flags in the Via
 *	audio hardware registers.
 */
void via_interrupt_disable (struct via_info *card)
{
	//u8 tmp8;
	unsigned long flags;

	DPRINTK ("ENTER\n");

	assert (card != NULL);
	
	spin_lock_irqsave (&card->lock, flags);
	outb (inb (card->baseaddr + VIA_BASE0_PCM_OUT_CHAN_TYPE) &
	      VIA_INT_DISABLE_MASK,
	      card->baseaddr + VIA_BASE0_PCM_OUT_CHAN_TYPE);
	outb (inb (card->baseaddr + VIA_BASE0_PCM_IN_CHAN_TYPE) &
	      VIA_INT_DISABLE_MASK,
	      card->baseaddr + VIA_BASE0_PCM_IN_CHAN_TYPE);

	spin_unlock_irqrestore (&card->lock, flags);
	
	DPRINTK ("EXIT\n");
}

/**
 *	via_interrupt_init - Initialize interrupt handling
 *	@card: Private info for specified board
 *
 *	Obtain and reserve IRQ for using in handling audio events.
 *	Also, disable any IRQ-generating resources, to make sure
 *	we don't get interrupts before we want them.
 */

int via_interrupt_init (struct via_info *card)
{
	DPRINTK ("ENTER\n");

	assert (card != NULL);
	assert (card->pdev != NULL);
	
	/* check for sane IRQ number. can this ever happen? */
	if (card->pdev->u.h0.nInterruptLine < 2) {
		printk (KERN_ERR PFX "insane IRQ %d, aborting\n",
			card->pdev->u.h0.nInterruptLine);
		DPRINTK ("EXIT, returning -EIO\n");
		return -EIO;
	}
	
	if (request_irq (card->pdev->u.h0.nInterruptLine, via_interrupt, NULL, SA_SHIRQ, VIA_MODULE_NAME, card)<0) {
		printk (KERN_ERR PFX "unable to obtain IRQ %d, aborting\n",
			card->pdev->u.h0.nInterruptLine);
		DPRINTK ("EXIT, returning -EBUSY\n");
		return -EBUSY;
	}

	/* we don't want interrupts until we're opened */
	via_interrupt_disable (card);

	DPRINTK ("EXIT, returning 0\n");
	return 0;
}

/**
 *	via_interrupt_cleanup - Shutdown driver interrupt handling
 *	@card: Private info for specified board
 *
 *	Disable any potential interrupt sources in the Via audio
 *	hardware, and then release (un-reserve) the IRQ line
 *	in the kernel core.
 */
void via_interrupt_cleanup (struct via_info *card)
{
	DPRINTK ("ENTER\n");

	assert (card != NULL);
	assert (card->pdev != NULL);

	via_interrupt_disable (card);


	DPRINTK ("EXIT\n");
}


/****************************************************************
 *
 * OSS MIXER device
 *
 */
 

status_t via_mixer_open (void* pNode, uint32 nFlags, void **pCookie)
{
	struct via_info *card;

	DPRINTK ("ENTER\n");

	assert (via_driver_data != NULL);
	card = via_driver_data;

	DPRINTK ("EXIT, returning 0\n");
	return 0;
}

status_t via_mixer_release(void* pNode, void* pCookie)
{
	return 0;
}

status_t via_mixer_ioctl (void *node, void *cookie, uint32 cmd, void *args, bool kernel)
{
	struct ac97_codec *codec;
	struct via_info *card = node;
	int nonblock = 0;
	int rc;
	unsigned long arg = (unsigned long)args;

	DPRINTK ("ENTER\n");

	assert (card != NULL);
	
	codec = &card->ac97;
	
	assert (codec != NULL);
	

	rc = via_syscall_down (card, nonblock);
	if (rc) goto out;

	rc = codec->mixer_ioctl(codec, cmd, arg);

	UNLOCK (card->syscall_sem);

out:
	DPRINTK ("EXIT, returning %d\n", rc);
	return rc;
}

DeviceOperations_s via_mixer_fops = {
		via_mixer_open,
		via_mixer_release,
		via_mixer_ioctl,
		NULL,
		NULL
};


/****************************************************************
 *
 * OSS DSP device
 *
 */
 
DeviceOperations_s via_dsp_fops = {
		via_dsp_open,
		via_dsp_release,
		via_dsp_ioctl,
		via_dsp_read,
		via_dsp_write
};


static int via_dsp_init(struct via_info *card)
{
//	u8 tmp8;
	
	DPRINTK ("ENTER\n");

	assert (card != NULL);

	via_stop_everything (card);

	DPRINTK ("EXIT, returning 0\n");
	return 0;
}


void via_dsp_cleanup (struct via_info *card)
{
	DPRINTK ("ENTER\n");

	assert (card != NULL);
	assert (card->dev_dsp >= 0);

	via_stop_everything (card);

	DPRINTK ("EXIT\n");
}


static size_t via_dsp_do_read (struct via_info *card,
				char *userbuf, size_t count,
				int nonblock)
{
	const char *orig_userbuf = userbuf;
	struct via_channel *chan = &card->ch_in;
	size_t size;
	int n, tmp;

	/* if SGD has not yet been started, start it */
	via_chan_maybe_start (chan);

handle_one_block:

	/* grab current channel software pointer.  In the case of
	 * recording, this is pointing to the next buffer that
	 * will receive data from the audio hardware.
	 */
	n = chan->sw_ptr;

	/* n_frags represents the number of fragments waiting
	 * to be copied to userland.  sleep until at least
	 * one buffer has been read from the audio hardware.
	 */
	tmp = atomic_read (&chan->n_frags);
	assert (tmp >= 0);
	assert (tmp <= chan->frag_number);
	while (tmp == 0) {
		if (nonblock || !chan->is_active)
			return -EAGAIN;

		DPRINTK ("Sleeping on block %d\n", n);
		sleep_on_sem(chan->wait, INFINITE_TIMEOUT);


		tmp = atomic_read (&chan->n_frags);
	}

	/* Now that we have a buffer we can read from, send
	 * as much as sample data possible to userspace.
	 */
	while ((count > 0) && (chan->slop_len < chan->frag_size)) {
		size_t slop_left = chan->frag_size - chan->slop_len;

		size = (count < slop_left) ? count : slop_left;
		if (copy_to_user (userbuf,
				  chan->pgtbl[n / (PAGE_SIZE / chan->frag_size)].cpuaddr + n % (PAGE_SIZE / chan->frag_size) + chan->slop_len,
				  size))
			return -EFAULT;

		count -= size;
		chan->slop_len += size;
		userbuf += size;
	}

	/* If we didn't copy the buffer completely to userspace,
	 * stop now.
	 */
	if (chan->slop_len < chan->frag_size)
		goto out;

	/*
	 * If we get to this point, we copied one buffer completely
	 * to userspace, give the buffer back to the hardware.
	 */

	/* advance channel software pointer to point to
	 * the next buffer from which we will copy
	 */
	if (chan->sw_ptr == (chan->frag_number - 1))
		chan->sw_ptr = 0;
	else
		chan->sw_ptr++;

	/* mark one less buffer waiting to be processed */
	assert (atomic_read (&chan->n_frags) > 0);
	atomic_dec (&chan->n_frags);

	/* we are at a block boundary, there is no fragment data */
	chan->slop_len = 0;

	DPRINTK ("Flushed block %u, sw_ptr now %u, n_frags now %d\n",
		n, chan->sw_ptr, atomic_read (&chan->n_frags));

	DPRINTK ("regs==%02X %02X %02X %08X %08X %08X %08X\n",
		 inb (card->baseaddr + 0x00),
		 inb (card->baseaddr + 0x01),
		 inb (card->baseaddr + 0x02),
		 inl (card->baseaddr + 0x04),
		 inl (card->baseaddr + 0x0C),
		 inl (card->baseaddr + 0x80),
		 inl (card->baseaddr + 0x84));

	if (count > 0)
		goto handle_one_block;

out:
	return userbuf - orig_userbuf;
}



int via_dsp_read(void *node, void *cookie, off_t ppos, void *buffer, size_t count)
{
	struct via_info *card;
	int nonblock = 0;
	int rc;

	//DPRINTK ("ENTER, buffer=%p, count=%u, ppos=%lu\n",
		// buffer, count, ppos ? ((unsigned long)*ppos) : 0);

	assert (buffer != NULL);
	card = node;
	assert (card != NULL);


	rc = via_syscall_down (card, nonblock);
	if (rc) goto out;

	via_chan_set_buffering(card, &card->ch_in, -1);
        rc = via_chan_buffer_init (card, &card->ch_in);

	if (rc)
		goto out_up;

	rc = via_dsp_do_read (card,(char *) buffer, count, nonblock);

out_up:
	UNLOCK(card->syscall_sem);
	
out:
	DPRINTK ("EXIT, returning %ld\n",(long) rc);
	return rc;
}




static size_t via_dsp_do_write (struct via_info *card,
				 const char *userbuf, size_t count,
				 int nonblock)
{
	const char *orig_userbuf = userbuf;
	struct via_channel *chan = &card->ch_out;
	volatile struct via_sgd_table *sgtable = chan->sgtable;
	size_t size;
	int n, tmp;

handle_one_block:
	/* grab current channel fragment pointer.  In the case of
	 * playback, this is pointing to the next fragment that
	 * should receive data from userland.
	 */
	n = chan->sw_ptr;

	/* n_frags represents the number of fragments remaining
	 * to be filled by userspace.  Sleep until
	 * at least one fragment is available for our use.
	 */
	tmp = atomic_read (&chan->n_frags);
	assert (tmp >= 0);
	assert (tmp <= chan->frag_number);
	while (tmp == 0) {
		if (nonblock || !chan->is_enabled)
			return -EAGAIN;

		DPRINTK ("Sleeping on page %d, tmp==%d, ir==%d\n", n, tmp, chan->is_record);
		sleep_on_sem(chan->wait, INFINITE_TIMEOUT);

		tmp = atomic_read (&chan->n_frags);
	}

	/* Now that we have at least one fragment we can write to, fill the buffer
	 * as much as possible with data from userspace.
	 */
	while ((count > 0) && (chan->slop_len < chan->frag_size)) {
		size_t slop_left = chan->frag_size - chan->slop_len;

		size = (count < slop_left) ? count : slop_left;
		if (copy_from_user (chan->pgtbl[n / (PAGE_SIZE / chan->frag_size)].cpuaddr + (n % (PAGE_SIZE / chan->frag_size)) * chan->frag_size + chan->slop_len,
				    userbuf, size))
			return -EFAULT;

		count -= size;
		chan->slop_len += size;
		userbuf += size;
	}

	/* If we didn't fill up the buffer with data, stop now.
         * Put a 'stop' marker in the DMA table too, to tell the
         * audio hardware to stop if it gets here.
         */
	if (chan->slop_len < chan->frag_size) {
		sgtable[n].count = cpu_to_le32 (chan->slop_len | VIA_EOL | VIA_STOP);
		goto out;
	}

	/*
         * If we get to this point, we have filled a buffer with
         * audio data, flush the buffer to audio hardware.
         */

	/* Record the true size for the audio hardware to notice */
        if (n == (chan->frag_number - 1))
                sgtable[n].count = cpu_to_le32 (chan->frag_size | VIA_EOL);
        else
                sgtable[n].count = cpu_to_le32 (chan->frag_size | VIA_FLAG);

	/* advance channel software pointer to point to
	 * the next buffer we will fill with data
	 */
	if (chan->sw_ptr == (chan->frag_number - 1))
		chan->sw_ptr = 0;
	else
		chan->sw_ptr++;

	/* mark one less buffer as being available for userspace consumption */
	assert (atomic_read (&chan->n_frags) > 0);
	atomic_dec (&chan->n_frags);

	/* we are at a block boundary, there is no fragment data */
	chan->slop_len = 0;

	/* if SGD has not yet been started, start it */
	via_chan_maybe_start (chan);

	DPRINTK ("Flushed block %u, sw_ptr now %u, n_frags now %d\n",
		n, chan->sw_ptr, atomic_read (&chan->n_frags));

	DPRINTK ("regs==%02X %02X %02X %08X %08X %08X %08X\n",
		 inb (card->baseaddr + 0x00),
		 inb (card->baseaddr + 0x01),
		 inb (card->baseaddr + 0x02),
		 inl (card->baseaddr + 0x04),
		 inl (card->baseaddr + 0x0C),
		 inl (card->baseaddr + 0x80),
		 inl (card->baseaddr + 0x84));

	if (count > 0)
		goto handle_one_block;

out:
	return userbuf - orig_userbuf;
}



int via_dsp_write(void *node, void *cookie, off_t ppos, const void *buffer, size_t count)
{
	struct via_info *card;
	ssize_t rc;
	int nonblock = 0;

//	DPRINTK ("ENTER, buffer=%p, count=%u, ppos=%lu\n",
	//	 buffer, count, ppos ? ((unsigned long)*ppos) : 0);

	assert (buffer != NULL);
	card = node;
	assert (card != NULL);


	rc = via_syscall_down (card, nonblock);
	if (rc) goto out;

	via_chan_set_buffering(card, &card->ch_out, -1);
	rc = via_chan_buffer_init (card, &card->ch_out);

	if (rc)
		goto out_up;

	rc = via_dsp_do_write (card, buffer, count, nonblock);

out_up:
	UNLOCK(card->syscall_sem);
out:
	DPRINTK ("EXIT, returning %ld\n",(long) rc);
	return rc;
}

/**
 *	via_dsp_drain_playback - sleep until all playback samples are flushed
 *	@card: Private info for specified board
 *	@chan: Channel to drain
 *	@nonblock: boolean, non-zero if O_NONBLOCK is set
 *
 *	Sleeps until all playback has been flushed to the audio
 *	hardware.
 *
 *	Locking: inside card->syscall_sem
 */

int via_dsp_drain_playback (struct via_info *card, struct via_channel *chan, 
												int nonblock)
{
	DPRINTK ("ENTER, nonblock = %d\n", nonblock);

	if (chan->slop_len > 0)
		via_chan_flush_frag (chan);

	if (atomic_read (&chan->n_frags) == chan->frag_number)
		goto out;

	via_chan_maybe_start (chan);

	while (atomic_read (&chan->n_frags) < chan->frag_number) {
		if (nonblock) {
			DPRINTK ("EXIT, returning -EAGAIN\n");
			return -EAGAIN;
		}

		DPRINTK ("sleeping, nbufs=%d\n", atomic_read (&chan->n_frags));
		sleep_on_sem(chan->wait, INFINITE_TIMEOUT);
	}

out:
	DPRINTK ("EXIT, returning 0\n");
	return 0;
}


/**
 *	via_dsp_ioctl_space - get information about channel buffering
 *	@card: Private info for specified board
 *	@chan: pointer to channel-specific info
 *	@arg: user buffer for returned information
 *
 *	Handles SNDCTL_DSP_GETISPACE and SNDCTL_DSP_GETOSPACE.
 *
 *	Locking: inside card->syscall_sem
 */

static int via_dsp_ioctl_space (struct via_info *card,
				struct via_channel *chan,
				void *arg)
{
	audio_buf_info info;

	via_chan_set_buffering(card, chan, -1);

	info.fragstotal = chan->frag_number;
	info.fragsize = chan->frag_size;

	/* number of full fragments we can read/write without blocking */
	info.fragments = atomic_read (&chan->n_frags);

	if ((chan->slop_len % chan->frag_size > 0) && (info.fragments > 0))
		info.fragments--;

	/* number of bytes that can be read or written immediately
	 * without blocking.
	 */
	info.bytes = (info.fragments * chan->frag_size);
	if (chan->slop_len % chan->frag_size > 0)
		info.bytes += chan->frag_size - (chan->slop_len % chan->frag_size);

	DPRINTK ("EXIT, returning fragstotal=%d, fragsize=%d, fragments=%d, bytes=%d\n",
		info.fragstotal,
		info.fragsize,
		info.fragments,
		info.bytes);

	return copy_to_user (arg, &info, sizeof (info));
}

/**
 *	via_dsp_ioctl_ptr - get information about hardware buffer ptr
 *	@card: Private info for specified board
 *	@chan: pointer to channel-specific info
 *	@arg: user buffer for returned information
 *
 *	Handles SNDCTL_DSP_GETIPTR and SNDCTL_DSP_GETOPTR.
 *
 *	Locking: inside card->syscall_sem
 */

int via_dsp_ioctl_ptr (struct via_info *card,
				struct via_channel *chan,
				void *arg)
{
	count_info info;

	spin_lock_irq (&card->lock);

	info.bytes = chan->bytes;
	info.blocks = chan->n_irqs;
	chan->n_irqs = 0;

	spin_unlock_irq (&card->lock);

	if (chan->is_active) {
		unsigned long extra;
		info.ptr = atomic_read (&chan->hw_ptr) * chan->frag_size;
		extra = chan->frag_size - inl (chan->iobase + VIA_PCM_BLOCK_COUNT);
		info.ptr += extra;
		info.bytes += extra;
	} else {
		info.ptr = 0;
	}

	DPRINTK ("EXIT, returning bytes=%d, blocks=%d, ptr=%d\n",
		info.bytes,
		info.blocks,
		info.ptr);

	return copy_to_user (arg, &info, sizeof (info));
}


int via_dsp_ioctl_trigger (struct via_channel *chan, int val)
{
	int enable, do_something;

	if (chan->is_record)
		enable = (val & PCM_ENABLE_INPUT);
	else
		enable = (val & PCM_ENABLE_OUTPUT);

	if (!chan->is_enabled && enable) {
		do_something = 1;
	} else if (chan->is_enabled && !enable) {
		do_something = -1;
	} else {
		do_something = 0;
	}

	DPRINTK ("enable=%d, do_something=%d\n",
		 enable, do_something);

	if (chan->is_active && do_something)
		return -EINVAL;

	if (do_something == 1) {
		chan->is_enabled = 1;
		via_chan_maybe_start (chan);
		DPRINTK ("Triggering input\n");
	}

	else if (do_something == -1) {
		chan->is_enabled = 0;
		DPRINTK ("Setup input trigger\n");
	}

	return 0;
}


int via_dsp_ioctl (void *node, void *cookie, uint32 cmd, void *args, bool kernel)
{
	int rc, rd=0, wr=0, val=0;
	struct via_info *card;
	struct via_channel *chan;
	unsigned long arg = (unsigned long)args;
	int nonblock = 1;

	DPRINTK ("ENTER, cmd = 0x%08X\n", (uint)cmd);

	card = node;
	assert (card != NULL);

	wr = 1;
	rd = 1;

	rc = via_syscall_down (card, nonblock);
	if (rc)
		return rc;
	rc = -EINVAL;
	
	switch (cmd) {

	/* OSS API version.  XXX unverified */		
	case OSS_GETVERSION:
		DPRINTK("EXIT, returning SOUND_VERSION\n");
		rc = 0;
		put_user (SOUND_VERSION, (int *)arg);
		break;

	/* list of supported PCM data formats */
	case SNDCTL_DSP_GETFMTS:
		DPRINTK("EXIT, returning AFMT U8|S16_LE\n");
		rc = 0;
		put_user (AFMT_U8 | AFMT_S16_LE, (int *)arg);
		break;

	/* query or set current channel's PCM data format */
	case SNDCTL_DSP_SETFMT:
		get_user(val, (int *)arg);
		if (val != AFMT_QUERY) {
			rc = 0;

			if (rd)
				rc = via_chan_set_fmt (card, &card->ch_in, val);

			if (rc >= 0 && wr)
				rc = via_chan_set_fmt (card, &card->ch_out, val);

			if (rc < 0)
				break;

			val = rc;
		} else {
			if ((rd && (card->ch_in.pcm_stop & VIA_PCM_STOP_16BIT)) ||
			    (wr && (card->ch_out.pcm_fmt & VIA_PCM_FMT_VT_16BIT)))
				val = AFMT_S16_LE;
			else
				val = AFMT_U8;
		}
		DPRINTK("SETFMT EXIT, returning %d\n", val);
		rc = 0;
		put_user (val, (int *)arg);
		break;

	/* query or set number of channels (1=mono, 2=stereo) */
        case SNDCTL_DSP_CHANNELS:
		get_user(val, (int *)arg);
		if (val != 0) {
			rc = 0;

			if (rd)
				rc = via_chan_set_stereo (card, &card->ch_in, val);

			if (rc >= 0 && wr)
				rc = via_chan_set_stereo (card, &card->ch_out, val);

			if (rc < 0)
				break;

			val = rc;
		} else {
			if ((rd && (card->ch_in.pcm_stop & VIA_PCM_STOP_STEREO)) ||
			    (wr && (card->ch_out.pcm_fmt & VIA_PCM_FMT_VT_STEREO)))
				val = 2;
			else
				val = 1;
		}
		DPRINTK("CHANNELS EXIT, returning %d\n", val);
		rc = 0;
		put_user (val, (int *)arg);
		break;
	
	/* enable (val is not zero) or disable (val == 0) stereo */
        case SNDCTL_DSP_STEREO:
        get_user(val, (int *)arg);
		DPRINTK ("DSP_STEREO, val==%d\n", val);
		rc = 0;

		if (rd)
			rc = via_chan_set_stereo (card, &card->ch_in, val ? 2 : 1);
		if (rc >= 0 && wr)
			rc = via_chan_set_stereo (card, &card->ch_out, val ? 2 : 1);

		if (rc < 0)
			break;

		val = rc - 1;
		rc = 0;
		DPRINTK ("STEREO EXIT, returning %d\n", val);
		put_user(val, (int *) arg);
        break;
        
	/* query or set sampling rate */
        case SNDCTL_DSP_SPEED:
		get_user(val, (int *)arg);
		DPRINTK ("DSP_SPEED, val==%d\n", val);
		if (val < 0) {
			rc = -EINVAL;
			break;
		}
		if (val > 0) {
			rc = 0;

			if (rd)
				rc = via_chan_set_speed (card, &card->ch_in, val);
			if (rc >= 0 && wr)
				rc = via_chan_set_speed (card, &card->ch_out, val);

			if (rc < 0)
				break;

			val = rc;
		} else {
			if (rd)
				val = card->ch_in.rate;
			else if (wr)
				val = card->ch_out.rate;
			else
				val = 0;
		}
		rc = 0;
		DPRINTK ("SPEED EXIT, returning %d\n", val);
        put_user (val, (int *)arg);
		break;
	
	/* wait until all buffers have been played, and then stop device */
	case SNDCTL_DSP_SYNC:
		if (wr) {
			DPRINTK("SYNC EXIT (after calling via_dsp_drain_dac)\n");
			rc = via_dsp_drain_playback (card, &card->ch_out, 0);
		}
		break;

	/* stop recording/playback immediately */
        case SNDCTL_DSP_RESET:
		DPRINTK ("DSP_RESET\n");
		if (rd) {
			via_chan_clear (card, &card->ch_in);
			via_chan_pcm_fmt (&card->ch_in, 1);
			card->ch_in.frag_number = 0;
			card->ch_in.frag_size = 0;
			atomic_set(&card->ch_in.n_frags, 0);
		}

		if (wr) {
			via_chan_clear (card, &card->ch_out);
			via_chan_pcm_fmt (&card->ch_out, 1);
			card->ch_out.frag_number = 0;
			card->ch_out.frag_size = 0;
			atomic_set(&card->ch_out.n_frags, 0);
		}
		
		rc = 0;
		break;

	/* obtain bitmask of device capabilities, such as mmap, full duplex, etc. */
	case SNDCTL_DSP_GETCAPS:
		DPRINTK("GETCAPS EXIT\n");
		rc = 0;
		put_user(VIA_DSP_CAP, (int *)arg);
		break;
		
	/* obtain bitmask of device capabilities, such as mmap, full duplex, etc. */
	case SNDCTL_DSP_GETBLKSIZE:
		DPRINTK ("DSP_GETBLKSIZE\n");

		if (wr) {
			via_chan_set_buffering(card, &card->ch_in, -1);
			put_user(card->ch_in.frag_size, (int *)arg);
		} else if (rd) {
			via_chan_set_buffering(card, &card->ch_out, -1);
			put_user(card->ch_out.frag_size, (int *)arg);
		}
		rc = 0;
		break;
		
	/* obtain information about input buffering */
	case SNDCTL_DSP_GETISPACE:
		DPRINTK("GETISPACE EXIT\n");
		if (rd)
			rc = via_dsp_ioctl_space (card, &card->ch_in, (void*) arg);
		break;
	/* obtain information about output buffering */
	case SNDCTL_DSP_GETOSPACE:
		DPRINTK("GETOSPACE EXIT\n");
		if (wr)
			rc = via_dsp_ioctl_space (card, &card->ch_out, (void*) arg);
		break;
		
	/* obtain information about input hardware pointer */
	case SNDCTL_DSP_GETIPTR:
		DPRINTK ("DSP_GETIPTR\n");
		if (rd)
			rc = via_dsp_ioctl_ptr (card, &card->ch_in, (void*) arg);
		break;

	/* obtain information about output hardware pointer */
	case SNDCTL_DSP_GETOPTR:
		DPRINTK ("DSP_GETOPTR\n");
		if (wr)
			rc = via_dsp_ioctl_ptr (card, &card->ch_out, (void*) arg);
		break;

	/* return number of bytes remaining to be played by DMA engine */
	case SNDCTL_DSP_GETODELAY:
		{
		DPRINTK ("DSP_GETODELAY\n");

		chan = &card->ch_out;

		if (!wr)
			break;

		if (chan->is_active) {

			val = chan->frag_number - atomic_read (&chan->n_frags);

			if (val > 0) {
				val *= chan->frag_size;
				val -= chan->frag_size -
				       ( inl (chan->iobase + VIA_PCM_BLOCK_COUNT) & 0xffffff );
			}
			val += chan->slop_len % chan->frag_size;
		} else
			val = 0;

		assert (val <= (chan->frag_size * chan->frag_number));
		
		rc = 0;

		DPRINTK ("GETODELAY EXIT, val = %d bytes\n", val);
        put_user (val, (int *)arg);
		break;
		}
		
	/* handle the quick-start of a channel,
	 * or the notification that a quick-start will
	 * occur in the future
	 */
	case SNDCTL_DSP_SETTRIGGER:
		get_user(val, (int *)arg);
		DPRINTK ("DSP_SETTRIGGER, rd=%d, wr=%d, act=%d/%d, en=%d/%d\n",
			rd, wr, card->ch_in.is_active, card->ch_out.is_active,
			card->ch_in.is_enabled, card->ch_out.is_enabled);

		rc = 0;

		if (rd)
			rc = via_dsp_ioctl_trigger (&card->ch_in, val);

		if (!rc && wr)
			rc = via_dsp_ioctl_trigger (&card->ch_out, val);

		break;

	/* Enable full duplex.  Since we do this as soon as we are opened
	 * with O_RDWR, this is mainly a no-op that always returns success.
	 */
	case SNDCTL_DSP_SETDUPLEX:
		DPRINTK ("DSP_SETDUPLEX\n");
		if (!rd || !wr)
			break;
		rc = 0;
		break;

	/* set fragment size.  implemented as a successful no-op for now */
	case SNDCTL_DSP_SETFRAGMENT:
		get_user(val, (int *)arg);
		DPRINTK ("DSP_SETFRAGMENT, val==%d\n", val);

		if (rd)
			rc = via_chan_set_buffering(card, &card->ch_in, val);

		if (wr)
			rc = via_chan_set_buffering(card, &card->ch_out, val);	

		DPRINTK ("SNDCTL_DSP_SETFRAGMENT (fragshift==0x%04X (%d), maxfrags==0x%04X (%d))\n",
			 val & 0xFFFF,
			 val & 0xFFFF,
			 (val >> 16) & 0xFFFF,
			 (val >> 16) & 0xFFFF);

		rc = 0;
		break;;

	/* inform device of an upcoming pause in input (or output).  not implemented */
	case SNDCTL_DSP_POST:
		DPRINTK ("DSP_POST\n");
		if (wr) {
			if (card->ch_out.slop_len > 0)
				via_chan_flush_frag (&card->ch_out);
			via_chan_maybe_start (&card->ch_out);
		}

		rc = 0;
		break;

	/* not implemented */
	default:
		DPRINTK ("unhandled ioctl\n");
		break;
	}
		
	UNLOCK(card->syscall_sem);
	DPRINTK("EXIT, returning %d\n", rc);
	return rc;
}


status_t via_dsp_open (void* pNode, uint32 nFlags, void **pCookie)
{
	struct via_info *card;
	struct via_channel *chan;
	int nonblock = 0;

	DPRINTK ("ENTER\n");

	card = NULL;
	assert (via_driver_data != NULL);
			
	card = via_driver_data;	
	
	if (nonblock) {
		if (is_semaphore_locked(card->open_sem)) {
			DPRINTK ("EXIT, returning -EAGAIN\n");
			return -EAGAIN;
		} else
			LOCK(card->open_sem);
	} else {
		LOCK(card->open_sem);
	}

	
	/* enable full duplex mode */
	
	chan = &card->ch_in;

	via_chan_init (card, chan);
	
	chan->pcm_stop = VIA_PCM_STOP_16BIT | VIA_PCM_STOP_STEREO;
	via_chan_pcm_fmt (chan, 0);
	via_set_rate (&card->ac97, chan, 44100);
	chan = &card->ch_out;
	via_chan_init (card, chan);
	// if in duplex mode make the recording and playback channels
	//  have the same settings 
	chan->pcm_fmt = VIA_PCM_FMT_VT_16BIT | VIA_PCM_FMT_VT_STEREO;
	chan->pcm_stop = VIA_PCM_STOP_VT_STEREO;
	via_chan_pcm_fmt (chan, 0);
    via_set_rate (&card->ac97, chan, 44100);

	DPRINTK ("EXIT, returning 0\n");
	return 0;
}


status_t via_dsp_release(void* pNode, void* pCookie)
{
	struct via_info *card;
	int nonblock = 0;
	int rc;

	DPRINTK ("ENTER\n");

	card = pNode;
	assert (card != NULL);

	rc = via_syscall_down (card, nonblock);
	if (rc) {
		DPRINTK ("EXIT (syscall_down error), rc=%d\n", rc);
		return rc;
	}

		rc = via_dsp_drain_playback (card, &card->ch_out, nonblock);
		if (rc)
			printk (KERN_DEBUG "via_audio: ignoring drain playback error %d\n", rc);

		via_chan_free (card, &card->ch_out);
		via_chan_buffer_free(card, &card->ch_out);
		via_chan_free (card, &card->ch_in);
		via_chan_buffer_free (card, &card->ch_in);
	
	UNLOCK(card->syscall_sem);
	UNLOCK(card->open_sem);

	DPRINTK ("EXIT, returning 0\n");
	return 0;
}


/****************************************************************
 *
 * Chip setup and kernel registration
 *
 *
 */

int via_init_one ( int nDeviceID, PCI_Info_s *pdev)
{
	int rc;
	struct via_info *card;
	
	DPRINTK ("ENTER\n");


	card = kmalloc (sizeof (*card), MEMF_KERNEL);
	if (!card) {
		printk (KERN_ERR PFX "out of memory, aborting\n");
		rc = -ENOMEM;
		goto err_out_none;
	}

	via_driver_data = card;
	
	via_pdev = pdev;

	memset (card, 0, sizeof (*card));
	card->pdev = pdev;
	card->bus = ( PCI_bus_s* )get_busmanager( PCI_BUS_NAME, PCI_BUS_VERSION );
	card->baseaddr = pdev->u.h0.nBase0 & PCI_ADDRESS_MEMORY_32_MASK;
	card->card_num = via_num_cards++;
	spinlock_init (&card->lock, "via-lock");
	card->open_sem = create_semaphore("via_open_sem", 1, 0);
	card->syscall_sem = create_semaphore("via_syscall_sem", 2, 0);
	
	/* we must init these now, in case the intr handler needs them */
	via_chan_init_defaults (card, &card->ch_out);
	via_chan_init_defaults (card, &card->ch_in);

	if (pdev->u.h0.nInterruptLine < 1) {
		printk (KERN_ERR PFX "invalid PCI IRQ %d, aborting\n", pdev->u.h0.nInterruptLine);
		rc = -ENODEV;
		goto err_out_kfree;
	}

	/* 
	 * init AC97 mixer and codec
	 */
	rc = via_ac97_init (card);
	if (rc) {
		printk (KERN_ERR PFX "AC97 init failed, aborting\n");
		goto err_out_kfree;
	}

	/*
	 * init DSP device
	 */
	rc = via_dsp_init (card);
	if (rc) {
		printk (KERN_ERR PFX "DSP device init failed, aborting\n");
		goto err_out_have_mixer;
	}
	
	/*
	 * init and turn on interrupts, as the last thing we do
	 */
	rc = via_interrupt_init (card);
	if (rc) {
		printk (KERN_ERR PFX "interrupt init failed, aborting\n");
		goto err_out_have_dsp;
	}

	printk (KERN_INFO PFX "board #%d at 0x%04lX, IRQ %d\n",
		card->card_num + 1, card->baseaddr, pdev->u.h0.nInterruptLine);
	
	if( claim_device( nDeviceID, pdev->nHandle, "VIA VT82xx", DEVICE_AUDIO ) != 0 ) {
		rc = -ENODEV;
		goto err_out_have_dsp;
	}
	
	DPRINTK ("EXIT, returning 0\n");
	return 0;

err_out_have_dsp:
	via_dsp_cleanup (card);

err_out_have_mixer:
	via_ac97_cleanup (card);

err_out_kfree:
	kfree (card);

err_out_none:
	
	via_driver_data = NULL;
	DPRINTK ("EXIT - returning %d\n", rc);
	return rc;
}


void via_remove_one (PCI_Info_s *pdev)
{
	struct via_info *card;
	
	DPRINTK ("ENTER\n");
	
	assert (pdev != NULL);
	card = via_driver_data;
	assert (card != NULL);
	
	via_interrupt_cleanup (card);
	via_dsp_cleanup (card);
	via_ac97_cleanup (card);

	kfree (card);

	via_driver_data = NULL;
	


	DPRINTK ("EXIT\n");
	return;
}


/****************************************************************
 *
 * Driver initialization and cleanup
 *
 *
 */

status_t device_init( int nDeviceID )
{
	int i;
	bool found = false;
	PCI_Info_s pci;
	PCI_bus_s* psBus;
	
	DPRINTK("ENTER\n");
	
	printk (KERN_INFO "Via vt82xx audio driver " VIA_VERSION "\n");
	
	/* Get PCI busmanager */
	psBus = ( PCI_bus_s* )get_busmanager( PCI_BUS_NAME, PCI_BUS_VERSION );
	if( psBus == NULL )
		return( -EINVAL );
	
	/* scan all PCI devices */
    for(i = 0 ;  psBus->get_pci_info( &pci, i ) == 0 ; ++i) {
        if (pci.nVendorID == PCI_VENDOR_ID_VIA && pci.nDeviceID == PCI_DEVICE_ID_VIA_VT82XX) {		
			if(via_init_one( nDeviceID, &pci ) == 0) {
				found = true;
				break;
			}
        }
    }
    if(found) {
    	/* create DSP node */
		if( create_device_node( nDeviceID, pci.nHandle, "sound/vt82xx/dsp/0", &via_dsp_fops, via_driver_data ) < 0 ) {
			printk( "Failed to create 1 node \n");
			return ( -EINVAL );
		}
		/* create mixer node */
		if( create_device_node( nDeviceID, pci.nHandle, "sound/vt82xx/mixer/0", &via_mixer_fops, via_driver_data ) < 0 ) {
			printk( "Failed to create 1 node \n");
			return ( -EINVAL );
		}
	} else {
		printk( "No device found\n" );
		disable_device( nDeviceID );
		return ( -EINVAL );
	}
	
	return (0);	
	
	DPRINTK("EXIT\n");
}

 
status_t device_uninit( int nDeviceID)
{
	printk("device_uninit()\n" );
	DPRINTK("ENTER\n");
	
	release_device( via_pdev->nHandle );
	
	via_remove_one (via_pdev);
	
	via_pdev = via_driver_data = NULL;

	DPRINTK("EXIT\n");
	return( 0 );
}























