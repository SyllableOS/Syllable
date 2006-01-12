#ifndef _VIA686A_H_
#define _VIA686A_H_

#define VIA_VERSION	"0.1 (based on linux driver 1.1.15)"

#define PCI_DEVICE_ID_VIA_82C686_5 0x3058

//#define DPRINTK(fmt...) printk( __FUNCTION__": " fmt )
#define DPRINTK(fmt...)
       
#define arraysize(x)            (sizeof(x)/sizeof(*(x)))

#define MAX_CARDS	1

#define VIA_CARD_NAME	"VIA 686a Audio driver " VIA_VERSION
#define VIA_MODULE_NAME "via686a"
#define PFX		VIA_MODULE_NAME ": "

#define VIA_COUNTER_LIMIT	100000

/* size of DMA buffers */
#define VIA_MAX_BUFFER_DMA_PAGES	32

/* buffering default values in ms */
#define VIA_DEFAULT_FRAG_TIME		20
#define VIA_DEFAULT_BUFFER_TIME		500

#define VIA_MAX_FRAG_SIZE		PAGE_SIZE
#define VIA_MIN_FRAG_SIZE		64

#define VIA_MIN_FRAG_NUMBER		2	

#ifndef AC97_PCM_LR_ADC_RATE
#  define AC97_PCM_LR_ADC_RATE AC97_PCM_LR_DAC_RATE
#endif

/* 82C686 function 5 (audio codec) PCI configuration registers */
#define VIA_ACLINK_CTRL		0x41
#define VIA_FUNC_ENABLE		0x42
#define VIA_PNP_CONTROL		0x43
#define VIA_FM_NMI_CTRL		0x48

/*
 * controller base 0 (scatter-gather) registers
 *
 * NOTE: Via datasheet lists first channel as "read"
 * channel and second channel as "write" channel.
 * I changed the naming of the constants to be more
 * clear than I felt the datasheet to be.
 */

#define VIA_BASE0_PCM_OUT_CHAN	0x00 /* output PCM to user */
#define VIA_BASE0_PCM_OUT_CHAN_STATUS 0x00
#define VIA_BASE0_PCM_OUT_CHAN_CTRL	0x01
#define VIA_BASE0_PCM_OUT_CHAN_TYPE	0x02
#define VIA_BASE0_PCM_OUT_BLOCK_COUNT	0x0C

#define VIA_BASE0_PCM_IN_CHAN		0x10 /* input PCM from user */
#define VIA_BASE0_PCM_IN_CHAN_STATUS	0x10
#define VIA_BASE0_PCM_IN_CHAN_CTRL	0x11
#define VIA_BASE0_PCM_IN_CHAN_TYPE	0x12

/* offsets from base */
#define VIA_PCM_STATUS			0x00
#define VIA_PCM_CONTROL			0x01
#define VIA_PCM_TYPE			0x02
#define VIA_PCM_TABLE_ADDR		0x04
#define VIA_PCM_BLOCK_COUNT		0x0C

/* XXX unused DMA channel for FM PCM data */
#define VIA_BASE0_FM_OUT_CHAN		0x20
#define VIA_BASE0_FM_OUT_CHAN_STATUS	0x20
#define VIA_BASE0_FM_OUT_CHAN_CTRL	0x21
#define VIA_BASE0_FM_OUT_CHAN_TYPE	0x22

#define VIA_BASE0_AC97_CTRL		0x80
#define VIA_BASE0_SGD_STATUS_SHADOW	0x84
#define VIA_BASE0_GPI_INT_ENABLE	0x8C
#define VIA_INTR_OUT			((1<<0) |  (1<<4) |  (1<<8))
#define VIA_INTR_IN			((1<<1) |  (1<<5) |  (1<<9))
#define VIA_INTR_FM			((1<<2) |  (1<<6) | (1<<10))
#define VIA_INTR_MASK		(VIA_INTR_OUT | VIA_INTR_IN | VIA_INTR_FM)

/* VIA_BASE0_AUDIO_xxx_CHAN_TYPE bits */
#define VIA_IRQ_ON_FLAG			(1<<0)	/* int on each flagged scatter block */
#define VIA_IRQ_ON_EOL			(1<<1)	/* int at end of scatter list */
#define VIA_INT_SEL_PCI_LAST_LINE_READ	(0)	/* int at PCI read of last line */
#define VIA_INT_SEL_LAST_SAMPLE_SENT	(1<<2)	/* int at last sample sent */
#define VIA_INT_SEL_ONE_LINE_LEFT	(1<<3)	/* int at less than one line to send */
#define VIA_PCM_FMT_STEREO		(1<<4)	/* PCM stereo format (bit clear == mono) */
#define VIA_PCM_FMT_16BIT		(1<<5)	/* PCM 16-bit format (bit clear == 8-bit) */
#define VIA_PCM_REC_FIFO			(1<<6)	/* enable FIFO?  documented as "reserved" */
#define VIA_RESTART_SGD_ON_EOL		(1<<7)	/* restart scatter-gather at EOL */
#define VIA_PCM_FMT_MASK		(VIA_PCM_FMT_STEREO|VIA_PCM_FMT_16BIT)
#define VIA_CHAN_TYPE_MASK		(VIA_RESTART_SGD_ON_EOL | \
					 VIA_IRQ_ON_FLAG | \
					 VIA_IRQ_ON_EOL)
#define VIA_CHAN_TYPE_INT_SELECT	(VIA_INT_SEL_LAST_SAMPLE_SENT)

/* PCI configuration register bits and masks */
#define VIA_CR40_AC97_READY	0x01
#define VIA_CR40_AC97_LOW_POWER	0x02
#define VIA_CR40_SECONDARY_READY 0x04

#define VIA_CR41_AC97_ENABLE	0x80 /* enable AC97 codec */
#define VIA_CR41_AC97_RESET	0x40 /* clear bit to reset AC97 */
#define VIA_CR41_AC97_WAKEUP	0x20 /* wake up from power-down mode */
#define VIA_CR41_AC97_SDO	0x10 /* force Serial Data Out (SDO) high */
#define VIA_CR41_VRA		0x08 /* enable variable sample rate */
#define VIA_CR41_PCM_ENABLE	0x04 /* AC Link SGD Read Channel PCM Data Output */
#define VIA_CR41_FM_PCM_ENABLE	0x02 /* AC Link FM Channel PCM Data Out */
#define VIA_CR41_SB_PCM_ENABLE	0x01 /* AC Link SB PCM Data Output */
#define VIA_CR41_BOOT_MASK	(VIA_CR41_AC97_ENABLE | \
				 VIA_CR41_AC97_WAKEUP | \
				 VIA_CR41_AC97_SDO)
#define VIA_CR41_RUN_MASK	(VIA_CR41_AC97_ENABLE | \
				 VIA_CR41_AC97_RESET | \
				 VIA_CR41_VRA | \
				 VIA_CR41_PCM_ENABLE)

#define VIA_CR42_SB_ENABLE	0x01
#define VIA_CR42_MIDI_ENABLE	0x02
#define VIA_CR42_FM_ENABLE	0x04
#define VIA_CR42_GAME_ENABLE	0x08
#define VIA_CR42_MIDI_IRQMASK   0x40
#define VIA_CR42_MIDI_PNP	0x80

#define VIA_CR44_SECOND_CODEC_SUPPORT	(1 << 6)
#define VIA_CR44_AC_LINK_ACCESS		(1 << 7)

#define VIA_CR48_FM_TRAP_TO_NMI		(1 << 2)

/* controller base 0 register bitmasks */
#define VIA_INT_DISABLE_MASK		(~(0x01|0x02))
#define VIA_SGD_STOPPED			(1 << 2)
#define VIA_SGD_ACTIVE			(1 << 7)
#define VIA_SGD_TERMINATE		(1 << 6)
#define VIA_SGD_FLAG			(1 << 0)
#define VIA_SGD_EOL			(1 << 1)
#define VIA_SGD_START			(1 << 7)

#define VIA_CR80_FIRST_CODEC		0
#define VIA_CR80_SECOND_CODEC		(1 << 30)
#define VIA_CR80_FIRST_CODEC_VALID	(1 << 25)
#define VIA_CR80_VALID			(1 << 25)
#define VIA_CR80_SECOND_CODEC_VALID	(1 << 27)
#define VIA_CR80_BUSY			(1 << 24)
#define VIA_CR83_BUSY			(1)
#define VIA_CR83_FIRST_CODEC_VALID	(1 << 1)
#define VIA_CR80_READ			(1 << 23)
#define VIA_CR80_WRITE_MODE		0
#define VIA_CR80_REG_IDX(idx)		((((idx) & 0xFF) >> 1) << 16)

#define VIA_DSP_CAP (DSP_CAP_REVISION | DSP_CAP_DUPLEX | \
		     DSP_CAP_TRIGGER | DSP_CAP_REALTIME)


/* scatter-gather DMA table entry, exactly as passed to hardware */
struct via_sgd_table {
	u32 addr;
	u32 count;	/* includes additional bits also */
};
#define VIA_EOL (1 << 31)
#define VIA_FLAG (1 << 30)
#define VIA_STOP (1 << 29)

enum via_channel_states {
	sgd_stopped = 0,
	sgd_in_progress = 1,
};

struct via_buffer_pgtbl {
	dma_addr_t handle;
	void *cpuaddr;
};


struct via_channel {
	atomic_t n_frags;
	atomic_t hw_ptr;
	sem_id wait;

	unsigned int sw_ptr;
	unsigned int slop_len;
	unsigned int n_irqs;
	int bytes;

	unsigned is_active : 1;
	unsigned is_record : 1;
	unsigned is_mapped : 1;
	unsigned is_enabled : 1;
	u8 pcm_fmt;		/* VIA_PCM_FMT_xxx */

	unsigned rate;		/* sample rate */
	unsigned int frag_size;
	unsigned int frag_number;

	volatile struct via_sgd_table *sgtable;
	dma_addr_t sgt_handle;

	unsigned int page_number;
	struct via_buffer_pgtbl pgtbl[VIA_MAX_BUFFER_DMA_PAGES];

	long iobase;

	const char *name;
};

/* data stored for each chip */
struct via_info {
	PCI_Info_s *pdev;
	long baseaddr;
	
	struct ac97_codec ac97;
	SpinLock_s lock;
	int card_num;		/* unique card number, from 0 */

	int dev_dsp;		/* /dev/dsp index from register_sound_dsp() */
	
	unsigned rev_h : 1;
	
	int locked_rate : 1;

	sem_id syscall_sem;
	sem_id open_sem;
	
	struct via_channel ch_in;
	struct via_channel ch_out;
	struct via_channel ch_fm;
};


 
/****************************************************************
 *
 * prototypes
 *
 *
 */


void via_chan_stop (int iobase);
void via_chan_status_clear (int iobase);
void sg_begin (struct via_channel *chan);
int via_syscall_down (struct via_info *card, int nonblock);
void via_stop_everything (struct via_info *card);
int via_set_rate (struct ac97_codec *ac97, struct via_channel *chan, unsigned rate);
void via_chan_init_defaults (struct via_info *card, struct via_channel *chan);
void via_chan_init (struct via_info *card, struct via_channel *chan);
int via_chan_buffer_init (struct via_info *card, struct via_channel *chan);
void via_chan_free (struct via_info *card, struct via_channel *chan);
void via_chan_buffer_free (struct via_info *card, struct via_channel *chan);
void via_chan_clear (struct via_info *card,  struct via_channel *chan);
void via_chan_pcm_fmt (struct via_channel *chan, int reset);
int via_chan_set_speed (struct via_info *card,
			       struct via_channel *chan, int val);
int via_chan_set_fmt (struct via_info *card,
			     struct via_channel *chan, int val);
int via_chan_set_stereo (struct via_info *card,
			        struct via_channel *chan, int val);
int via_chan_set_buffering (struct via_info *card,
                                struct via_channel *chan, int val);
void via_chan_flush_frag (struct via_channel *chan);
void via_chan_maybe_start (struct via_channel *chan);



int via_init_one (PCI_Info_s *dev);
void via_remove_one (PCI_Info_s *pdev);

int via_dsp_read(void *node, void *cookie, off_t ppos, void *buffer, size_t count);
int via_dsp_write(void *node, void *cookie, off_t ppos, const void *buffer, size_t count);
status_t via_dsp_ioctl (void *node, void *cookie, uint32 cmd, void *args, bool bFromKernel);
status_t via_dsp_open (void* pNode, uint32 nFlags, void **pCookie );
status_t via_dsp_release( void* pNode, void* pCookie);

u16 via_ac97_read_reg (struct ac97_codec *codec, u8 reg);
void via_ac97_write_reg (struct ac97_codec *codec, u8 reg, u16 value);
int via_ac97_reset (struct via_info *card);
u8 via_ac97_wait_idle (struct via_info *card);
int via_ac97_init (struct via_info *card);
void via_ac97_codec_wait (struct ac97_codec *codec);
void via_ac97_cleanup (struct via_info *card);


#endif
