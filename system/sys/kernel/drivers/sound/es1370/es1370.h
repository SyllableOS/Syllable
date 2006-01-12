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

#ifndef PCI_VENDOR_ID_ENSONIQ
#define PCI_VENDOR_ID_ENSONIQ        0x1274
#endif

#ifndef PCI_DEVICE_ID_ENSONIQ_ES1370
#define PCI_DEVICE_ID_ENSONIQ_ES1370 0x5000
#endif

#define ES1370_MAGIC  ((PCI_VENDOR_ID_ENSONIQ<<16)|PCI_DEVICE_ID_ENSONIQ_ES1370)


#define ES1370_EXTENT             0x40
#define JOY_EXTENT                8

#define ES1370_REG_CONTROL        0x00
#define ES1370_REG_STATUS         0x04
#define ES1370_REG_UART_DATA      0x08
#define ES1370_REG_UART_STATUS    0x09
#define ES1370_REG_UART_CONTROL   0x09
#define ES1370_REG_UART_TEST      0x0a
#define ES1370_REG_MEMPAGE        0x0c
#define ES1370_REG_CODEC          0x10
#define ES1370_REG_SERIAL_CONTROL 0x20
#define ES1370_REG_DAC1_SCOUNT    0x24
#define ES1370_REG_DAC2_SCOUNT    0x28
#define ES1370_REG_ADC_SCOUNT     0x2c

#define ES1370_REG_DAC1_FRAMEADR    0xc30
#define ES1370_REG_DAC1_FRAMECNT    0xc34
#define ES1370_REG_DAC2_FRAMEADR    0xc38
#define ES1370_REG_DAC2_FRAMECNT    0xc3c
#define ES1370_REG_ADC_FRAMEADR     0xd30
#define ES1370_REG_ADC_FRAMECNT     0xd34
#define ES1370_REG_PHANTOM_FRAMEADR 0xd38
#define ES1370_REG_PHANTOM_FRAMECNT 0xd3c

#define ES1370_FMT_U8_MONO     0
#define ES1370_FMT_U8_STEREO   1
#define ES1370_FMT_S16_MONO    2
#define ES1370_FMT_S16_STEREO  3
#define ES1370_FMT_STEREO      1
#define ES1370_FMT_S16         2
#define ES1370_FMT_MASK        3

#define CTRL_ADC_STOP   0x80000000  /* 1 = ADC stopped */
#define CTRL_XCTL1      0x40000000  /* electret mic bias */
#define CTRL_OPEN       0x20000000  /* no function, can be read and written */
#define CTRL_PCLKDIV    0x1fff0000  /* ADC/DAC2 clock divider */
#define CTRL_SH_PCLKDIV 16
#define CTRL_MSFMTSEL   0x00008000  /* MPEG serial data fmt: 0 = Sony, 1 = I2S */
#define CTRL_M_SBB      0x00004000  /* DAC2 clock: 0 = PCLKDIV, 1 = MPEG */
#define CTRL_WTSRSEL    0x00003000  /* DAC1 clock freq: 0=5512, 1=11025, 2=22050, 3=44100 */
#define CTRL_SH_WTSRSEL 12
#define CTRL_DAC_SYNC   0x00000800  /* 1 = DAC2 runs off DAC1 clock */
#define CTRL_CCB_INTRM  0x00000400  /* 1 = CCB "voice" ints enabled */
#define CTRL_M_CB       0x00000200  /* recording source: 0 = ADC, 1 = MPEG */
#define CTRL_XCTL0      0x00000100  /* 0 = Line in, 1 = Line out */
#define CTRL_BREQ       0x00000080  /* 1 = test mode (internal mem test) */
#define CTRL_DAC1_EN    0x00000040  /* enable DAC1 */
#define CTRL_DAC2_EN    0x00000020  /* enable DAC2 */
#define CTRL_ADC_EN     0x00000010  /* enable ADC */
#define CTRL_UART_EN    0x00000008  /* enable MIDI uart */
#define CTRL_JYSTK_EN   0x00000004  /* enable Joystick port (presumably at address 0x200) */
#define CTRL_CDC_EN     0x00000002  /* enable serial (CODEC) interface */
#define CTRL_SERR_DIS   0x00000001  /* 1 = disable PCI SERR signal */

#define STAT_INTR       0x80000000  /* wired or of all interrupt bits */
#define STAT_CSTAT      0x00000400  /* 1 = codec busy or codec write in progress */
#define STAT_CBUSY      0x00000200  /* 1 = codec busy */
#define STAT_CWRIP      0x00000100  /* 1 = codec write in progress */
#define STAT_VC         0x00000060  /* CCB int source, 0=DAC1, 1=DAC2, 2=ADC, 3=undef */
#define STAT_SH_VC      5
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

#define FMODE_DAC         4           /* slight misuse of mode_t */

/* MIDI buffer sizes */

#define MIDIINBUF  256
#define MIDIOUTBUF 256

#define FMODE_MIDI_SHIFT 3
#define FMODE_MIDI_READ  (FMODE_READ << FMODE_MIDI_SHIFT)
#define FMODE_MIDI_WRITE (FMODE_WRITE << FMODE_MIDI_SHIFT)

#define DAC2_SRTODIV(x) (((1411200+(x)/2)/(x))-2)
#define DAC2_DIVTOSR(x) (1411200/((x)+2))

/* --------------------------------------------------------------------- */

struct es1370_state {
	/* magic */
	unsigned int magic;

	/* list of es1370 devices */
//	struct list_head devs;

	/* the corresponding pci_dev structure */
	PCI_Info_s *dev;

	/* soundcore stuff */
	int dev_audio;
	int dev_mixer;
	int dev_dac;
	int dev_midi;
	
	/* hardware resources */
	unsigned long io; /* long for SPARC */
	unsigned int irq;

	/* mixer registers; there is no HW readback */
	struct {
		unsigned short vol[10];
		unsigned int recsrc;
		unsigned int modcnt;
		unsigned short micpreamp;
	        unsigned int imix;
	} mix;

	/* wave stuff */
	unsigned ctrl;
	unsigned sctrl;
	
	sem_id wait;

	SpinLock_s lock;
	sem_id open_sem;
	mode_t open_mode;

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

	/* midi stuff */
/*	struct {
		unsigned ird, iwr, icnt;
		unsigned ord, owr, ocnt;
		wait_queue_head_t iwait;
		wait_queue_head_t owait;
		unsigned char ibuf[MIDIINBUF];
		unsigned char obuf[MIDIOUTBUF];
	} midi;

	struct gameport gameport;*/
	sem_id sem;
};


void es1370_update_ptr(struct es1370_state *s);

status_t es1370_write(void *node, void *cookie, off_t ppos, const void *buffer, size_t count);
status_t es1370_open (void* pNode, uint32 nFlags, void **pCookie);
status_t es1370_release(void* pNode, void* pCookie);
status_t es1370_ioctl (void *node, void *cookie, uint32 cmd, void *arg, bool frkrnl);

void wrcodec(struct es1370_state *s, unsigned char idx, unsigned char data);
int es1370_interrupt(int irq, void *dev_id, SysCallRegs_s *regs);


