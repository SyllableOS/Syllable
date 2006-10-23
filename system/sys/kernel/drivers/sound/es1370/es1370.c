/*****************************************************************************/
/*
 *      es1370.c  --  Ensoniq ES1370/Asahi Kasei AK4531 audio driver.
 *
 *      Copyright (C) 1998-2001  Thomas Sailer (t.sailer@alumni.ethz.ch)
 *               Port to AtheOS  Henrik Isaksson (henrik@boing.nu)
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
 * Special thanks to David C. Niemi
 *
 * some important things missing in Ensoniq documentation:
 *
 * Experimental PCLKDIV results:  play the same waveforms on both DAC1 and DAC2
 * and vary PCLKDIV to obtain zero beat.
 *  5512sps:  254
 * 44100sps:   30
 * seems to be fs = 1411200/(PCLKDIV+2)
 *
 * should find out when curr_sample_ct is cleared and
 * where exactly the CCB fetches data
 *
 * The card uses a 22.5792 MHz crystal.
 * The LINEIN jack may be converted to an AOUT jack by
 * setting pin 47 (XCTL0) of the ES1370 to high.
 * Pin 48 (XCTL1) of the ES1370 sets the +5V bias for an electretmic
 * 
 *
 */

/*****************************************************************************/

#include "es1370.h"

/* --------------------------------------------------------------------- */

#undef OSS_DOCUMENTED_MIXER_SEMANTICS
#define DBG(x) {}
/*#define DBG(x) {x}*/

/* --------------------------------------------------------------------- */


static const unsigned sample_size[] = { 1, 2, 2, 4 };
static const unsigned sample_shift[] = { 0, 1, 1, 2 };

static const unsigned dac1_samplerate[] = { 5512, 11025, 22050, 44100 };

/*
 * The following buffer is used to point the phantom write channel to,
 * so that it cannot wreak havoc. The attribute makes sure it doesn't
 * cross a page boundary and ensures dword alignment for the DMA engine
 */
//static unsigned char bugbuf[16] __attribute__ ((aligned (16)));

/* --------------------------------------------------------------------- */

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

/* --------------------------------------------------------------------- */

void wrcodec(struct es1370_state *s, unsigned char idx, unsigned char data)
{
	int j = 0;

	do {
		j ++;
		//printk("%lx %lx\n", inl(s->io+ES1370_REG_STATUS), s->io );
		if (!(inl(s->io+ES1370_REG_STATUS) & STAT_CSTAT)) {
			outw((((unsigned short)idx)<<8)|data, s->io+ES1370_REG_CODEC);
			return;
		}
		//udelay(2);
		snooze(1000);
	} while(j < 100);

	printk(KERN_ERR "es1370: write to codec register timeout\n");
}

/* --------------------------------------------------------------------- */

static inline void stop_adc(struct es1370_state *s)
{
	unsigned long flags;

	spin_lock_irqsave(&s->lock, flags);
	s->ctrl &= ~CTRL_ADC_EN;
	outl(s->ctrl, s->io+ES1370_REG_CONTROL);
	spin_unlock_irqrestore(&s->lock, flags);
}	

static inline void stop_dac1(struct es1370_state *s)
{
	unsigned long flags;

	spin_lock_irqsave(&s->lock, flags);
	s->ctrl &= ~CTRL_DAC1_EN;
	outl(s->ctrl, s->io+ES1370_REG_CONTROL);
	spin_unlock_irqrestore(&s->lock, flags);
}	

static inline void stop_dac2(struct es1370_state *s)
{
	unsigned long flags;

	spin_lock_irqsave(&s->lock, flags);
	s->ctrl &= ~CTRL_DAC2_EN;
	outl(s->ctrl, s->io+ES1370_REG_CONTROL);
	spin_unlock_irqrestore(&s->lock, flags);
}	

#if 0
static void start_dac1(struct es1370_state *s)
{
	unsigned long flags;
	unsigned fragremain, fshift;

	spin_lock_irqsave(&s->lock, flags);
	if (!(s->ctrl & CTRL_DAC1_EN) && (s->dma_dac1.mapped || s->dma_dac1.count > 0)
	    && s->dma_dac1.ready) {
		s->ctrl |= CTRL_DAC1_EN;
		s->sctrl = (s->sctrl & ~(SCTRL_P1LOOPSEL | SCTRL_P1PAUSE | SCTRL_P1SCTRLD)) | SCTRL_P1INTEN;
		outl(s->sctrl, s->io+ES1370_REG_SERIAL_CONTROL);
		fragremain = ((- s->dma_dac1.hwptr) & (s->dma_dac1.fragsize-1));
		fshift = sample_shift[(s->sctrl & SCTRL_P1FMT) >> SCTRL_SH_P1FMT];
		if (fragremain < 2*fshift)
			fragremain = s->dma_dac1.fragsize;
		outl((fragremain >> fshift) - 1, s->io+ES1370_REG_DAC1_SCOUNT);
		outl(s->ctrl, s->io+ES1370_REG_CONTROL);
		outl((s->dma_dac1.fragsize >> fshift) - 1, s->io+ES1370_REG_DAC1_SCOUNT);
	}
	spin_unlock_irqrestore(&s->lock, flags);
}	
#endif

static void start_dac2(struct es1370_state *s)
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
		outl(s->sctrl, s->io+ES1370_REG_SERIAL_CONTROL);
		fragremain = ((- s->dma_dac2.hwptr) & (s->dma_dac2.fragsize-1));
		fshift = sample_shift[(s->sctrl & SCTRL_P2FMT) >> SCTRL_SH_P2FMT];
		if (fragremain < 2*fshift)
			fragremain = s->dma_dac2.fragsize;
		outl((fragremain >> fshift) - 1, s->io+ES1370_REG_DAC2_SCOUNT);
		outl(s->ctrl, s->io+ES1370_REG_CONTROL);
		outl((s->dma_dac2.fragsize >> fshift) - 1, s->io+ES1370_REG_DAC2_SCOUNT);
	}
	spin_unlock_irqrestore(&s->lock, flags);
}	

#if 0
static void start_adc(struct es1370_state *s)
{
	unsigned long flags;
	unsigned fragremain, fshift;

	spin_lock_irqsave(&s->lock, flags);
	if (!(s->ctrl & CTRL_ADC_EN) && (s->dma_adc.mapped || s->dma_adc.count < (signed)(s->dma_adc.dmasize - 2*s->dma_adc.fragsize))
	    && s->dma_adc.ready) {
		s->ctrl |= CTRL_ADC_EN;
		s->sctrl = (s->sctrl & ~SCTRL_R1LOOPSEL) | SCTRL_R1INTEN;
		outl(s->sctrl, s->io+ES1370_REG_SERIAL_CONTROL);
		fragremain = ((- s->dma_adc.hwptr) & (s->dma_adc.fragsize-1));
		fshift = sample_shift[(s->sctrl & SCTRL_R1FMT) >> SCTRL_SH_R1FMT];
		if (fragremain < 2*fshift)
			fragremain = s->dma_adc.fragsize;
		outl((fragremain >> fshift) - 1, s->io+ES1370_REG_ADC_SCOUNT);
		outl(s->ctrl, s->io+ES1370_REG_CONTROL);
		outl((s->dma_adc.fragsize >> fshift) - 1, s->io+ES1370_REG_ADC_SCOUNT);
	}
	spin_unlock_irqrestore(&s->lock, flags);
}	
#endif

/* --------------------------------------------------------------------- */

#define DMABUF_DEFAULTORDER (17-PAGE_SHIFT)
#define DMABUF_MINORDER 1

static inline void dealloc_dmabuf(struct es1370_state *s, struct dmabuf *db)
{
	if (db->rawbuf) {
		pci_free_consistent(s->dev, PAGE_SIZE << db->buforder, db->rawbuf, db->dmaaddr);
	}
	db->rawbuf = NULL;
	db->mapped = db->ready = 0;
}

static int prog_dmabuf(struct es1370_state *s, struct dmabuf *db, unsigned rate, unsigned fmt, unsigned reg)
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
	fmt &= ES1370_FMT_MASK;
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
	memset(db->rawbuf, (fmt & ES1370_FMT_S16) ? 0 : 0x80, db->dmasize);
	outl((reg >> 8) & 15, s->io+ES1370_REG_MEMPAGE);
	outl(db->dmaaddr, s->io+(reg & 0xff));
	outl((db->dmasize >> 2)-1, s->io+((reg + 4) & 0xff));
	db->enabled = 1;
	db->ready = 1;
	return 0;
}

static inline int prog_dmabuf_adc(struct es1370_state *s)
{
	stop_adc(s);
	return prog_dmabuf(s, &s->dma_adc, DAC2_DIVTOSR((s->ctrl & CTRL_PCLKDIV) >> CTRL_SH_PCLKDIV),
			   (s->sctrl >> SCTRL_SH_R1FMT) & ES1370_FMT_MASK, ES1370_REG_ADC_FRAMEADR);
}

static inline int prog_dmabuf_dac2(struct es1370_state *s)
{
	stop_dac2(s);
	return prog_dmabuf(s, &s->dma_dac2, DAC2_DIVTOSR((s->ctrl & CTRL_PCLKDIV) >> CTRL_SH_PCLKDIV),
			   (s->sctrl >> SCTRL_SH_P2FMT) & ES1370_FMT_MASK, ES1370_REG_DAC2_FRAMEADR);
}

static inline int prog_dmabuf_dac1(struct es1370_state *s)
{
	stop_dac1(s);
	return prog_dmabuf(s, &s->dma_dac1, dac1_samplerate[(s->ctrl & CTRL_WTSRSEL) >> CTRL_SH_WTSRSEL],
			   (s->sctrl >> SCTRL_SH_P1FMT) & ES1370_FMT_MASK, ES1370_REG_DAC1_FRAMEADR);
}

static inline unsigned get_hwptr(struct es1370_state *s, struct dmabuf *db, unsigned reg)
{
	unsigned hwptr, diff;

	outl((reg >> 8) & 15, s->io+ES1370_REG_MEMPAGE);
	hwptr = (inl(s->io+(reg & 0xff)) >> 14) & 0x3fffc;
	diff = (db->dmasize + hwptr - db->hwptr) % db->dmasize;
	db->hwptr = hwptr;
	return diff;
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

/* call with spinlock held! */
void es1370_update_ptr(struct es1370_state *s)
{
	int diff;

	/* update ADC pointer */
	if (s->ctrl & CTRL_ADC_EN) {
		diff = get_hwptr(s, &s->dma_adc, ES1370_REG_ADC_FRAMECNT);
		s->dma_adc.total_bytes += diff;
		s->dma_adc.count += diff;
		if (s->dma_adc.count >= (signed)s->dma_adc.fragsize) 
			wakeup_sem(s->dma_adc.wait, true);
		if (!s->dma_adc.mapped) {
			if (s->dma_adc.count > (signed)(s->dma_adc.dmasize - ((3 * s->dma_adc.fragsize) >> 1))) {
				s->ctrl &= ~CTRL_ADC_EN;
				outl(s->ctrl, s->io+ES1370_REG_CONTROL);
				s->dma_adc.error++;
			}
		}
	}
	/* update DAC1 pointer */
	if (s->ctrl & CTRL_DAC1_EN) {
		diff = get_hwptr(s, &s->dma_dac1, ES1370_REG_DAC1_FRAMECNT);
		s->dma_dac1.total_bytes += diff;
		if (s->dma_dac1.mapped) {
			s->dma_dac1.count += diff;
			if (s->dma_dac1.count >= (signed)s->dma_dac1.fragsize)
				wakeup_sem(s->dma_dac1.wait, true);
		} else {
			s->dma_dac1.count -= diff;
			if (s->dma_dac1.count <= 0) {
				s->ctrl &= ~CTRL_DAC1_EN;
				outl(s->ctrl, s->io+ES1370_REG_CONTROL);
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
		diff = get_hwptr(s, &s->dma_dac2, ES1370_REG_DAC2_FRAMECNT);
		s->dma_dac2.total_bytes += diff;
		if (s->dma_dac2.mapped) {
			s->dma_dac2.count += diff;
			if (s->dma_dac2.count >= (signed)s->dma_dac2.fragsize)
				wakeup_sem(s->dma_dac2.wait, true);
		} else {
			s->dma_dac2.count -= diff;
			if (s->dma_dac2.count <= 0) {
				s->ctrl &= ~CTRL_DAC2_EN;
				outl(s->ctrl, s->io+ES1370_REG_CONTROL);
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

/* --------------------------------------------------------------------- */

status_t es1370_open_mixdev(void* pNode, uint32 nFlags, void **pCookie)
{
//	struct es1370_state *s = (struct es1370_state *)pNode;
       	return 0;
      /*
	int minor = MINOR(inode->i_rdev);
	struct list_head *list;
	struct es1370_state *s;

	for (list = devs.next; ; list = list->next) {
		if (list == &devs)
			return -ENODEV;
		s = list_entry(list, struct es1370_state, devs);
		if (s->dev_mixer == minor)
			break;
	}

	file->private_data = s;
	return 0;*/
}

status_t es1370_release_mixdev(void* pNode, uint32 nFlags, void **pCookie)
{
//	struct es1370_state *s = (struct es1370_state *)pNode;
	

	return 0;
}

status_t es1370_ioctl_mixdev(void *node, void *cookie, uint32 cmd, void *args, bool len)
{
	return 0;
}

/* --------------------------------------------------------------------- */

static int drain_dac2(struct es1370_state *s, int nonblock)
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
		tmo >>= sample_shift[(s->sctrl & SCTRL_P2FMT) >> SCTRL_SH_P2FMT];
	        tmo = 3 * 100 * (count + s->dma_dac2.fragsize) / 2 / ( DAC2_DIVTOSR((s->ctrl & CTRL_PCLKDIV) >> CTRL_SH_PCLKDIV) );
		tmo >>= sample_shift[(s->sctrl & SCTRL_P2FMT) >> SCTRL_SH_P2FMT];
        }
        return 0;
}

/* --------------------------------------------------------------------- */

status_t es1370_write(void *node, void *cookie, off_t ppos, const void *buffer, size_t count)
{
	struct es1370_state *s = node;
	ssize_t ret = 0;
	unsigned long flags;
	unsigned swptr;
	int cnt;

//	printk("ES1370_write: ENTER\n");

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
		        sleep_on_sem(s->dma_dac2.wait, 5000000);	// Almost infinite timeout (5 seconds) =o)
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

int es1370_interrupt(int irq, void *dev_id, SysCallRegs_s *regs)
{
        struct es1370_state *s = (struct es1370_state *)dev_id;
	unsigned int intsrc, sctl;

	/* fastpath out, to ease interrupt sharing */
	intsrc = inl(s->io+ES1370_REG_STATUS);
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
	outl(sctl, s->io+ES1370_REG_SERIAL_CONTROL);
	outl(s->sctrl, s->io+ES1370_REG_SERIAL_CONTROL);
	es1370_update_ptr(s);
	spin_unlock(&s->lock);
	return 0;
}

status_t es1370_open (void* pNode, uint32 nFlags, void **pCookie)
{
	//int minor = MINOR(inode->i_rdev);
	unsigned long flags;
	struct es1370_state *s = (struct es1370_state *) pNode;

	printk("es1370_open: %p\n", s);


/*        if (s->open == true) {
	           printk("es1370_open: Device already open\n");
	           return -1;
	}*/

	LOCK(s->sem);
	s->dma_dac2.ossfragshift = s->dma_dac2.ossmaxfrags = s->dma_dac2.subdivision = 0;
	s->dma_dac2.enabled = 1;
//	set_dac2_rate(s, 8000);
	spin_lock_irqsave(&s->lock, flags);
	s->sctrl |= ES1370_FMT_S16_STEREO << SCTRL_SH_P2FMT;
	outl(s->sctrl, s->io+ES1370_REG_SERIAL_CONTROL);
	spin_unlock_irqrestore(&s->lock, flags);
	//s->open = true;

	// Set up the mixer to wave playback, mute everything else.
	// (Hard-coded volume level!)

	wrcodec(s, 0x00, 0x01);	// Master Attenuation Level, Left
	wrcodec(s, 0x01, 0x01);	// Master Attenuation Level, Right
	wrcodec(s, 0x02, 0x01);	// Voice Attenuation Level, Left
	wrcodec(s, 0x03, 0x01);	// Voice Attenuation Level, Right
	wrcodec(s, 0x04, 0x80);	// FM Attenuation Level, Left
	wrcodec(s, 0x05, 0x80);	// FM Attenuation Level, Right
	wrcodec(s, 0x06, 0x80);	// CD Attenuation Level, Left
	wrcodec(s, 0x07, 0x80);	// CD Attenuation Level, Right
	wrcodec(s, 0x08, 0x80);	// Line Attenuation Level, Left
	wrcodec(s, 0x09, 0x80);	// Line Attenuation Level, Right
	wrcodec(s, 0x0A, 0x80);	// AUX Attenuation Level, Left
	wrcodec(s, 0x0B, 0x80);	// AUX Attenuation Level, Right
	wrcodec(s, 0x0C, 0x80);	// Mono1 Attenuation Level
	wrcodec(s, 0x0D, 0x80);	// Mono2 Attenuation Level
	wrcodec(s, 0x0E, 0x80);	// MIC Attenuation Level
	wrcodec(s, 0x0D, 0x80);	// Mono Out Attenuation Level
	wrcodec(s, 0x10, 0x00);	// Output Mixer SW 1
	wrcodec(s, 0x11, 0x0C);	// Output Mixer SW 2

        UNLOCK(s->sem);
	printk("es1370_open: EXIT\n");
	return 0;
}


status_t es1370_release (void* pNode, void *pCookie)
{
	struct es1370_state *s = (struct es1370_state *)pNode;

	printk("es1370_release\n");

	// Mute
	wrcodec(s, 0x00, 0x80);	// Master Attenuation Level, Left
	wrcodec(s, 0x01, 0x80);	// Master Attenuation Level, Right

/*
	lock_kernel();
	if (file->f_mode & FMODE_WRITE)
		drain_dac2(s, file->f_flags & O_NONBLOCK);
	down(&s->open_sem);
	if (file->f_mode & FMODE_WRITE) {
		stop_dac2(s);
		synchronize_irq();
		dealloc_dmabuf(s, &s->dma_dac2);
	}
	if (file->f_mode & FMODE_READ) {
		stop_adc(s);
		dealloc_dmabuf(s, &s->dma_adc);
	}
	s->open_mode &= (~file->f_mode) & (FMODE_READ|FMODE_WRITE);
	wake_up(&s->open_wait);
	up(&s->open_sem);
	unlock_kernel(); */
	return 0;
}


status_t es1370_ioctl (void *node, void *cookie, uint32 cmd, void *arg, bool frkrnl)
{
	struct es1370_state *s = (struct es1370_state *)node;
	unsigned long flags;
	int val, ret;
   
        printk("es1370_ioctl: ENTER, Command: %u\n", (unsigned int)cmd);

  	switch (cmd) {
	case OSS_GETVERSION:
	//        put_user(SOUND_VERSION, (int *)arg);
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
     /*           get_user(val, (int *)arg);
		if (val >= 0) {
		         stop_dac2(s);
			 s->dma_dac2.ready = 0;
			 set_dac2_rate(s, val);
		}
                put_user(s->dac2rate, (int *)arg);*/
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
		outl(s->sctrl, s->io+ES1370_REG_SERIAL_CONTROL);
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
			outl(s->sctrl, s->io+ES1370_REG_SERIAL_CONTROL);
		       	spin_unlock_irqrestore(&s->lock, flags);
		}
		put_user( 1, (int *)arg);
	        return 0;
		
	case SNDCTL_DSP_GETFMTS: // Returns a mask
                put_user(AFMT_S16_LE|AFMT_U8, (int *)arg);
	        return 0;
		
	case SNDCTL_DSP_SETFMT: // Selects ONE fmt
		get_user(val, (int *)arg);
		if (val != AFMT_QUERY) {
		        stop_dac2(s);
		       	s->dma_dac2.ready = 0;
	       		spin_lock_irqsave(&s->lock, flags);
       			if (val == AFMT_S16_LE)
		                s->sctrl |= SCTRL_P2SEB;
			else
		       		s->sctrl &= ~SCTRL_P2SEB;
       			outl(s->sctrl, s->io+ES1370_REG_SERIAL_CONTROL);
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
	//	put_user( s->dac2rate, (int *)arg);
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
			break;
		}

        case SOUND_PCM_WRITE_FILTER:
        case SNDCTL_DSP_SETSYNCRO:
        case SOUND_PCM_READ_FILTER:
                return -EINVAL;
		
	}
	//return mixdev_ioctl(&s->codec, cmd, (unsigned long) &arg);
	return 0;
}
