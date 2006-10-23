/*
 **********************************************************************
 *     audio.c -- /dev/dsp interface for emu10k1 driver
 *     Copyright 1999, 2000 Creative Labs, Inc.
 *
 **********************************************************************
 *
 *     Date                 Author          Summary of changes
 *     ----                 ------          ------------------
 *     October 20, 1999     Bertrand Lee    base code release
 *     November 2, 1999	    Alan Cox        cleaned up types/leaks
 *
 **********************************************************************
 *
 *     This program is free software; you can redistribute it and/or
 *     modify it under the terms of the GNU General Public License as
 *     published by the Free Software Foundation; either version 2 of
 *     the License, or (at your option) any later version.
 *
 *     This program is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU General Public License for more details.
 *
 *     You should have received a copy of the GNU General Public
 *     License along with this program; if not, write to the Free
 *     Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139,
 *     USA.
 *
 **********************************************************************
 */

#define __NO_VERSION__
#include <atheos/kernel.h>
#include <atheos/atomic.h>
#include <atheos/isa_io.h>
#include <atheos/spinlock.h>
#include <posix/errno.h>
#include <atheos/device.h>

#include "hwaccess.h"
#include "cardwo.h"
#include "irqmgr.h"
#include "audio.h"
#include "8010.h"

static struct emu10k1_wavedevice *bh_wave_dev;

static void calculate_ofrag(struct woinst *);

/* Audio file operations */
int emu10k1_audio_write(void *node, void *cookie, off_t ppos, const void *buffer, size_t count)
{
	struct emu10k1_wavedevice *wave_dev = (struct emu10k1_wavedevice *)cookie;
	struct woinst *woinst = wave_dev->woinst;
	ssize_t ret;
	unsigned long flags;

	DPD(3, "emu10k1_audio_write(), buffer=%p, count=%d\n", buffer, (uint32) count);

	spinlock_cli(&woinst->lock, flags);

	if (woinst->state == WAVE_STATE_CLOSED)
	{
		calculate_ofrag(woinst);

		while (emu10k1_waveout_open(wave_dev) < 0)
		{
			spinunlock_restore(&woinst->lock, flags);
			sleep_on_sem(wave_dev->card->open_wait,INFINITE_TIMEOUT);
			spinlock_cli(&woinst->lock, flags);
		}
	}

	spinunlock_restore(&woinst->lock, flags);

	ret = 0;
	if (count % woinst->format.bytespersample)
                return -EINVAL;

	count /= woinst->num_voices;

	while (count > 0)
	{
		uint32 bytestocopy;

		spinlock_cli(&woinst->lock, flags);
		emu10k1_waveout_update(woinst);
		emu10k1_waveout_getxfersize(woinst, &bytestocopy);
		spinunlock_restore(&woinst->lock, flags);

		DPD(3, "bytestocopy --> %d\n", bytestocopy);

		if ((bytestocopy >= woinst->buffer.fragment_size) || (bytestocopy >= count))
		{
			bytestocopy = min_t(uint32, bytestocopy, count);

			emu10k1_waveout_xferdata(woinst, (uint8 *) buffer, &bytestocopy);

			count -= bytestocopy;
			buffer += bytestocopy * woinst->num_voices;
			ret += bytestocopy * woinst->num_voices;

			spinlock_cli(&woinst->lock, flags);
			woinst->total_copied += bytestocopy;

			if (!(woinst->state & WAVE_STATE_STARTED)
			    && (wave_dev->enablebits & PCM_ENABLE_OUTPUT)
			    && (woinst->total_copied >= woinst->buffer.fragment_size))
				emu10k1_waveout_start(wave_dev);

			spinunlock_restore(&woinst->lock, flags);
		}

		if (count > 0)
		{
			if ((!(wave_dev->enablebits & PCM_ENABLE_OUTPUT)))
				return (ret ? ret : -EAGAIN);

			sleep_on_sem(woinst->wait_queue,INFINITE_TIMEOUT);
		}
	}

	DPD(3, "bytes copied -> %d\n", (uint32) ret);

	return ret;
}

status_t emu10k1_audio_ioctl(void *node, void *cookie, uint32 cmd, void *arg, bool bFromKernel)
{
	struct emu10k1_wavedevice *wave_dev = (struct emu10k1_wavedevice *)cookie;
	struct woinst *woinst = NULL;
	int val = 0;
	unsigned long flags;

	DPF(4, "emu10k1_audio_ioctl()\n");

	woinst = wave_dev->woinst;

	switch (cmd)
	{
		case SNDCTL_DSP_SPEED:
		{
			DPF(2, "SNDCTL_DSP_SPEED:\n");

			if(memcpy_from_user(&val,(unsigned long*)arg,sizeof(val))!=0)
				return -EFAULT;

			DPD(2, "val is %d\n", val);

			if (val > 0)
			{
				struct wave_format format;

				spinlock_cli(&woinst->lock, flags);

				format = woinst->format;
				format.samplingrate = val;

				if (emu10k1_waveout_setformat(wave_dev, &format) < 0)
				{
					spinunlock_restore(&woinst->lock, flags);
					return -EINVAL;
				}

				val = woinst->format.samplingrate;

				spinunlock_restore(&woinst->lock, flags);

				DPD(2, "set playback sampling rate -> %d\n", val);

				if(memcpy_to_user((unsigned long *)arg,&val,sizeof(val))!=0)
					return -EFAULT;

				return 0;
			}
			else
			{
				val = woinst->format.samplingrate;
				if(memcpy_to_user((unsigned long *)arg,&val,sizeof(val))!=0)
					return -EFAULT;

				return 0;
			}

			break;
		}

		case SNDCTL_DSP_STEREO:
		{
			struct wave_format format;

			DPF(2, "SNDCTL_DSP_STEREO:\n");

			if(memcpy_from_user(&val,(unsigned long*)arg,sizeof(val))!=0)
				return -EFAULT;

			DPD(2, " val is %d\n", val);

			spinlock_cli(&woinst->lock, flags);

			format = woinst->format;
			format.channels = val ? 2 : 1;

			if (emu10k1_waveout_setformat(wave_dev, &format) < 0) {
				spinunlock_restore(&woinst->lock, flags);
				return -EINVAL;
			}

			val = woinst->format.channels - 1;
			spinunlock_restore(&woinst->lock, flags);

			DPD(2, "set playback stereo -> %d\n", val);

			if(memcpy_to_user((unsigned long *)arg,&val,sizeof(val))!=0)
				return -EFAULT;

			return 0;

			break;
		}

		case SNDCTL_DSP_GETFMTS:
		{
			DPF(2, "SNDCTL_DSP_GETFMTS:\n");

			val = AFMT_S16_LE | AFMT_U8;
			if(memcpy_to_user((unsigned long *)arg,&val,sizeof(val))!=0)
				return -EFAULT;

			return 0;

			break;
		}

		case SNDCTL_DSP_SETFMT:
		{
			DPF(2, "SNDCTL_DSP_SETFMT:\n");

			if(memcpy_from_user(&val,(unsigned long*)arg,sizeof(val))!=0)
				return -EFAULT;

			DPD(2, " val is %d\n", val);

			if (val != AFMT_QUERY)
			{
				struct wave_format format;

				spinlock_cli(&woinst->lock, flags);

				format = woinst->format;
				format.id = val;

				if (emu10k1_waveout_setformat(wave_dev, &format) < 0) {
					spinunlock_restore(&woinst->lock, flags);
					return -EINVAL;
				}

				val = woinst->format.id;

				spinunlock_restore(&woinst->lock, flags);
				DPD(2, "set playback format -> %d\n", val);

				if(memcpy_to_user((unsigned long *)arg,&val,sizeof(val))!=0)
					return -EFAULT;

				return 0;
			}
			else
			{
				val = woinst->format.id;
				if(memcpy_to_user((unsigned long *)arg,&val,sizeof(val))!=0)
					return -EFAULT;

				return 0;
			}

			break;
		}

		case SNDCTL_DSP_POST:
		{
			spinlock_cli(&woinst->lock, flags);

			if (!(woinst->state & WAVE_STATE_STARTED)
			&& (wave_dev->enablebits & PCM_ENABLE_OUTPUT)
			&& (woinst->total_copied > 0))
				emu10k1_waveout_start(wave_dev);

			spinunlock_restore(&woinst->lock, flags);

			break;
		}

		case SNDCTL_DSP_SETFRAGMENT:
		{
			DPF(2, "SNDCTL_DSP_SETFRAGMENT:\n");

			if(memcpy_from_user(&val,(unsigned long*)arg,sizeof(val))!=0)
				return -EFAULT;

			DPD(2, "val is %#x\n", val);

			if (val == 0)
				return -EIO;

			/* digital pass-through fragment count and size are fixed values */
			if (woinst->state & WAVE_STATE_OPEN)
				return -EINVAL;	/* too late to change */

			woinst->buffer.ossfragshift = val & 0xffff;
			woinst->buffer.numfrags = (val >> 16) & 0xffff;

			break;
		}

		case SNDCTL_DSP_RESET:
		{
			DPF(2, "SNDCTL_DSP_RESET:\n");
			wave_dev->enablebits = PCM_ENABLE_OUTPUT | PCM_ENABLE_INPUT;

			spinlock_cli(&woinst->lock, flags);

			if (woinst->state & WAVE_STATE_OPEN)
			{
				emu10k1_waveout_close(wave_dev);
			}

			woinst->mmapped = 0;
			woinst->total_copied = 0;
			woinst->total_played = 0;
			woinst->blocks = 0;

			spinunlock_restore(&woinst->lock, flags);

			break;
		}

		case SNDCTL_DSP_GETBLKSIZE:
		{
			DPF(2, "SNDCTL_DSP_GETBLKSIZE:\n");

			spinlock_cli(&woinst->lock, flags);

			calculate_ofrag(woinst);
			val = woinst->buffer.fragment_size * woinst->num_voices;

			spinunlock_restore(&woinst->lock, flags);

			if(memcpy_to_user((unsigned long *)arg,&val,sizeof(val))!=0)
				return -EFAULT;

			return 0;

			break;
		}


		case SNDCTL_DSP_GETOSPACE:
		{
			audio_buf_info info;
			uint32 bytestocopy;

			DPF(4, "SNDCTL_DSP_GETOSPACE:\n");

			spinlock_cli(&woinst->lock, flags);

			if (woinst->state & WAVE_STATE_OPEN)
			{
				emu10k1_waveout_update(woinst);
				emu10k1_waveout_getxfersize(woinst, &bytestocopy);

				info.bytes = bytestocopy;
			}
			else
			{
				calculate_ofrag(woinst);
				info.bytes = woinst->buffer.size;
			}
			spinunlock_restore(&woinst->lock, flags);

			info.bytes *= woinst->num_voices;
			info.fragsize = woinst->buffer.fragment_size * woinst->num_voices;
			info.fragstotal = woinst->buffer.numfrags * woinst->num_voices;
			info.fragments = info.bytes / info.fragsize;

			if (memcpy_to_user((int *) arg, &info, sizeof(info)))
				return -EFAULT;

			break;
		}

		case SNDCTL_DSP_GETODELAY:
		{
			uint32 bytestocopy;

			DPF(4, "SNDCTL_DSP_GETODELAY:\n");

			spinlock_cli(&woinst->lock, flags);
			if (woinst->state & WAVE_STATE_OPEN) {
				emu10k1_waveout_update(woinst);
				emu10k1_waveout_getxfersize(woinst, &bytestocopy);

				val = woinst->buffer.size - bytestocopy;
			} else
				val = 0;

			val *= woinst->num_voices;
			spinunlock_restore(&woinst->lock, flags);

			if(memcpy_to_user((unsigned long *)arg,&val,sizeof(val))!=0)
				return -EFAULT;

			return 0;
		}

		case IOCTL_GET_USERSPACE_DRIVER:
		{
			memcpy_to_user( arg, "oss.so", strlen( "oss.so" ) );
			break;
		}

		default:		/* Default is unrecognized command */
		{
			debug_ioctl(cmd);
			return -EINVAL;
		}

	}

	return 0;
}

status_t emu10k1_audio_open(void *node, uint32 flags, void **cookie)
{
	struct emu10k1_card *card = NULL;
	struct emu10k1_wavedevice *wave_dev;
	struct woinst *woinst;
	int i;

	DPF(2, "emu10k1_audio_open()\n");
	//printk("emu10k1_audio_open()\n");

	card = (struct emu10k1_card *)node;

	wave_dev = (struct emu10k1_wavedevice *) kmalloc(sizeof(struct emu10k1_wavedevice), MEMF_KERNEL);

	if (wave_dev == NULL)
	{ 
		ERROR();
		return -ENOMEM;
	}

	wave_dev->card = card;
	wave_dev->woinst = NULL;
	wave_dev->enablebits = PCM_ENABLE_OUTPUT | PCM_ENABLE_INPUT;	/* Default */

	bh_wave_dev = wave_dev;
	*cookie = wave_dev;

	if ((woinst = (struct woinst *) kmalloc(sizeof(struct woinst), MEMF_KERNEL)) == NULL)
	{
		ERROR();
		return -ENODEV;
	}

/*
	woinst->format.id = AFMT_U8;
	woinst->format.samplingrate = 8000;
	woinst->format.bitsperchannel = 8;
	woinst->format.channels = 1;
*/
	woinst->format.id = AFMT_S16_LE;
	woinst->format.samplingrate = 44100;
	woinst->format.bitsperchannel = 16;
	woinst->format.channels = 2;

	woinst->state = WAVE_STATE_CLOSED;

	woinst->buffer.fragment_size = 0;
	woinst->buffer.ossfragshift = 0;
	woinst->buffer.numfrags = 0;
	woinst->timer.state = TIMER_STATE_UNINSTALLED;
	woinst->num_voices = 1;

	for (i = 0; i < WAVEOUT_MAXVOICES; i++)
	{
		woinst->voice[i].usage = VOICE_USAGE_FREE;
		woinst->voice[i].mem.emupageindex = -1;
	}

	woinst->wait_queue=create_semaphore("waveout_wait_queue",1,SEM_RECURSIVE);

	woinst->mmapped = 0;
	woinst->total_copied = 0;
	woinst->total_played = 0;
	woinst->blocks = 0;
	spinlock_init(&woinst->lock,"waveout_lock");

	wave_dev->woinst = woinst;
	emu10k1_waveout_setformat(wave_dev, &woinst->format);

	return 0;
}

status_t emu10k1_audio_release(void *node, void *cookie)
{
	struct emu10k1_wavedevice *wave_dev = (struct emu10k1_wavedevice *)cookie;
	struct emu10k1_card *card = (struct emu10k1_card *)node;
	unsigned long flags;
	struct woinst *woinst = wave_dev->woinst;
	
	wave_dev->card=card;

	DPF(2, "emu10k1_audio_release()\n");
	printk("emu10k1_audio_release()\n");

	spinlock_cli(&woinst->lock, flags);

	if (woinst->state & WAVE_STATE_OPEN)
	{
		if (woinst->state & WAVE_STATE_STARTED)
		{
			while (woinst->total_played < woinst->total_copied)
			{
				DPF(4, "Buffer hasn't been totally played, sleep....\n");
				spinunlock_restore(&woinst->lock, flags);
				sleep_on_sem(woinst->wait_queue,INFINITE_TIMEOUT);
				spinlock_cli(&woinst->lock, flags);
			}
		}

		emu10k1_waveout_close(wave_dev);
	}

	spinunlock_restore(&woinst->lock, flags);

	kfree(wave_dev->woinst);
	kfree(wave_dev);

	wakeup_sem(card->open_wait,true);

	return 0;
}

static void calculate_ofrag(struct woinst *woinst)
{
	struct waveout_buffer *buffer = &woinst->buffer;
	uint32 fragsize;

	if (buffer->fragment_size)
		return;

	if (!buffer->ossfragshift)
	{
		fragsize = (woinst->format.bytespervoicesample * woinst->format.samplingrate * WAVEOUT_DEFAULTFRAGLEN) / 1000 - 1;

		while (fragsize)
		{
			fragsize >>= 1;
			buffer->ossfragshift++;
		}
	}

	if (buffer->ossfragshift < WAVEOUT_MINFRAGSHIFT)
		buffer->ossfragshift = WAVEOUT_MINFRAGSHIFT;

	buffer->fragment_size = 1 << buffer->ossfragshift;

	while (buffer->fragment_size * WAVEOUT_MINFRAGS > WAVEOUT_MAXBUFSIZE)
		buffer->fragment_size >>= 1;

	/* now we are sure that:
	 (2^WAVEOUT_MINFRAGSHIFT) <= (fragment_size = 2^n) <= (WAVEOUT_MAXBUFSIZE / WAVEOUT_MINFRAGS)
	*/

	if (!buffer->numfrags)
	{
		uint32 numfrags;

		numfrags = (woinst->format.bytespervoicesample * woinst->format.samplingrate * WAVEOUT_DEFAULTBUFLEN) /
			   (buffer->fragment_size * 1000) - 1;

		buffer->numfrags = 1;

		while (numfrags)
		{
			numfrags >>= 1;
			buffer->numfrags <<= 1;
		}
	}

	if (buffer->numfrags < WAVEOUT_MINFRAGS)
		buffer->numfrags = WAVEOUT_MINFRAGS;

	if (buffer->numfrags * buffer->fragment_size > WAVEOUT_MAXBUFSIZE)
		buffer->numfrags = WAVEOUT_MAXBUFSIZE / buffer->fragment_size;

	if (buffer->numfrags < WAVEOUT_MINFRAGS)
		printk("BUG in the Audigy/SBLive driver :o(\n");

	buffer->size = buffer->fragment_size * buffer->numfrags;
	buffer->pages = buffer->size / PAGE_SIZE + ((buffer->size % PAGE_SIZE) ? 1 : 0);

	DPD(2, " calculated playback fragment_size -> %d\n", buffer->fragment_size);
	DPD(2, " calculated playback numfrags -> %d\n", buffer->numfrags);

	return;
}

void emu10k1_waveout_bh(void)
{
	struct emu10k1_wavedevice *wave_dev=bh_wave_dev;
	struct woinst *woinst = wave_dev->woinst;
	uint32 bytestocopy;
	unsigned long flags;

	if (!woinst)
		return;

	spinlock_cli(&woinst->lock, flags);

	if (!(woinst->state & WAVE_STATE_STARTED))
	{
		spinunlock_restore(&woinst->lock, flags);
		return;
	}

	emu10k1_waveout_update(woinst);
	emu10k1_waveout_getxfersize(woinst, &bytestocopy);

	if (woinst->buffer.fill_silence)
	{
		spinunlock_restore(&woinst->lock, flags);
		emu10k1_waveout_fillsilence(woinst);
	}
	else
		spinunlock_restore(&woinst->lock, flags);

	if (bytestocopy >= woinst->buffer.fragment_size)
		wakeup_sem(woinst->wait_queue,true);
	else
		DPD(3, "Not enough transfer size -> %d\n", bytestocopy);

	return;
}

DeviceOperations_s emu10k1_audio_fops = {
		emu10k1_audio_open,
		emu10k1_audio_release,
		emu10k1_audio_ioctl,
		NULL,
		emu10k1_audio_write
};
