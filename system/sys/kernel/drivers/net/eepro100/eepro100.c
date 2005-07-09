/* drivers/net/eepro100.c: An Intel i82557-559 Ethernet driver for Linux. */
/*
   NOTICE: this version of the driver is supposed to work with 2.2 kernels.
	Written 1996-1999 by Donald Becker.

	This software may be used and distributed according to the terms
	of the GNU Public License, incorporated herein by reference.

	This driver is for the Intel EtherExpress Pro100 (Speedo3) design.
	It should work with all i82557/558/559 boards.

	To use as a module, use the compile-command at the end of the file.

	The author may be reached as becker@CESDIS.usra.edu, or C/O
	Center of Excellence in Space Data and Information Sciences
	   Code 930.5, NASA Goddard Space Flight Center, Greenbelt MD 20771
	For updates see
		http://cesdis.gsfc.nasa.gov/linux/drivers/eepro100.html
	For installation instructions
		http://cesdis.gsfc.nasa.gov/linux/misc/modules.html
	There is a Majordomo mailing list based at
		linux-eepro100@cesdis.gsfc.nasa.gov
	
	The driver also contains updates by different kernel developers.
	This driver clone is maintained by Andrey V. Savochkin <saw@saw.sw.com.sg>.
	Please use this email address and linux-kernel mailing list for bug reports.
	
	Modification history:
	2000 Mar 24  Dragan Stancevic <visitor@valinux.com>
		Disabled FC and ER, to avoid lockups when when we get FCP interrupts.
	2000 May 27  Andrey Moruga <moruga@sw.com.sg>
		Code duplication for 82559ER support was removed.
		Accurate handling of all supported chips was implemented.
		Some fixes in 2.3 clone of the driver were ported.
	2000 May 30  Dragan Stancevic <visitor@valinux.com> and
				 Andrey Moruga <moruga@sw.com.sg>
		Honor PortReset timing specification.
	2000 Jul 25  Dragan Stancevic <visitor@valinux.com>
		Changed to MMIO, resized FIFOs, resized rings, changed ISR timeout
		Problem reported by:
		Marc MERLIN <merlin@valinux.com>
	2000 Nov 15  Dragan Stancevic <visitor@valinux.com>
		Changed command completion time and added debug info as to which
		CMD timed out. Problem reported by:
		"Ulrich Windl" <Ulrich.Windl@rz.uni-regensburg.de>
	2002 Jan 21  Neale Banks <neale@lowendale.com.au>
		Gracefully handle the case where init_etherdev() returns NULL
	2003 Aug 18  Michael Krueger <invenies@web.de>
		Ported the driver to Syllable/AtheOS
*/

/* AtheOS includes */
#include <atheos/kernel.h>
#include <atheos/kdebug.h>
#include <atheos/irq.h>
#include <atheos/isa_io.h>
#include <atheos/udelay.h>
#include <atheos/time.h>
#include <atheos/timer.h>
#include <atheos/pci.h>
#include <atheos/semaphore.h>
#include <atheos/spinlock.h>
#include <atheos/ctype.h>
#include <atheos/device.h>

#include <posix/unistd.h>
#include <posix/errno.h>
#include <posix/ioctls.h>
#include <posix/fcntl.h>
#include <posix/signal.h>
#include <net/net.h>
#include <net/ip.h>
#include <net/sockios.h>

#include "bitops.h"
#include "linuxcomp.h"

#define USE_IO 1 /* most important: No MMIO! This is really complicated in AtheOS!*/

//static const char *version =
//"eepro100.c:v1.09j-t 9/29/99 Donald Becker http://cesdis.gsfc.nasa.gov/linux/drivers/eepro100.html\n"
//"eepro100.c: $Revision$ 2000/05/31 Modified by Andrey V. Savochkin <saw@saw.sw.com.sg> and others\n"
//"eepro100.c: VA Linux custom, Dragan Stancevic <visitor@valinux.com> 2000/11/15\n";
static const char *version =
"eepro100: Intel EtherExpress Pro 10/100 NIC driver v0.2\n"
"based on Linux driver by Donald Becker et al., port by Michael Krueger\n";

/* A few user-configurable values that apply to all boards.
   First set is undocumented and spelled per Intel recommendations. */

static int congenb = 0;		/* Enable congestion control in the DP83840. */
static int txfifo = 0;		/* Tx FIFO threshold in 4 byte units, 0-15 */
static int rxfifo = 0xF;		/* Rx FIFO threshold, default 32 bytes. */
/* Tx/Rx DMA burst length, 0-127, 0 == no preemption, tx==128 -> disabled. */
static int txdmacount = 128;
static int rxdmacount = 0;

/* Set the copy breakpoint for the copy-only-tiny-buffer Rx method.
   Lower values use more memory, but are faster. */
static int rx_copybreak = 200;

/* Maximum events (Rx packets, etc.) to handle at each interrupt. */
static int max_interrupt_work = 200;

/* Maximum number of multicast addresses to filter (vs. rx-all-multicast) */
static int multicast_filter_limit = 64;

/* 'options' is used to pass a transceiver override or full-duplex flag
   e.g. "options=16" for FD, "options=32" for 100mbps-only. */
static int full_duplex[] = {-1, -1, -1, -1, -1, -1, -1, -1};
static int options[] = {-1, -1, -1, -1, -1, -1, -1, -1};

//static int debug = -1;			/* The debug level */

/* A few values that may be tweaked. */
/* The ring sizes should be a power of two for efficiency. */
#define TX_RING_SIZE	64
#define RX_RING_SIZE	64
/* How much slots multicast filter setup may take.
   Do not descrease without changing set_rx_mode() implementaion. */
#define TX_MULTICAST_SIZE   2
#define TX_MULTICAST_RESERV (TX_MULTICAST_SIZE*2)
/* Actual number of TX packets queued, must be
   <= TX_RING_SIZE-TX_MULTICAST_RESERV. */
#define TX_QUEUE_LIMIT  (TX_RING_SIZE-TX_MULTICAST_RESERV)
/* Hysteresis marking queue as no longer full. */
#define TX_QUEUE_UNFULL (TX_QUEUE_LIMIT-4)

/* Operational parameters that usually are not changed. */

/* Time in jiffies before concluding the transmitter is hung. */
#define TX_TIMEOUT		(2*HZ)
/* Size of an pre-allocated Rx buffer: <Ethernet MTU> + slack.*/
#define PKT_BUF_SZ		1536

#if !defined(__OPTIMIZE__)  ||  !defined(__KERNEL__)
#warning  You must compile this file with the correct options!
#warning  See the last lines of the source file.
#error You must compile this driver with "-O".
#endif

#define RUN_AT(x) (jiffies + (x))
/* Condensed bus+endian portability operations. */

#define net_device              device
#define pci_base_address(p, n)  (p)->base_address[n]

#define dev_free_skb(skb)       dev_kfree_skb(skb);
#define netif_wake_queue(dev)   do { \
									clear_bit(0, (void*)&dev->tbusy); \
								} while(0)
#define netif_start_queue(dev)  clear_bit(0, (void*)&dev->tbusy)
#define netif_stop_queue(dev)   set_bit(0, (void*)&dev->tbusy)

#ifndef PCI_VENDOR_ID_INTEL
#define PCI_VENDOR_ID_INTEL 0x8086
#endif
#ifndef PCI_DEVICE_ID_INTEL_82559ER
#define PCI_DEVICE_ID_INTEL_82559ER 0x1209
#endif
#ifndef PCI_DEVICE_ID_INTEL_ID1029
#define PCI_DEVICE_ID_INTEL_ID1029 0x1029
#endif
#ifndef PCI_DEVICE_ID_INTEL_ID1030
#define PCI_DEVICE_ID_INTEL_ID1030 0x1030
#endif
#ifndef PCI_DEVICE_ID_INTEL_ID1031
#define PCI_DEVICE_ID_INTEL_ID1031 0x1031
#endif
#ifndef PCI_DEVICE_ID_INTEL_ID2449
#define PCI_DEVICE_ID_INTEL_ID2449 0x2449
#endif

/* The total I/O port extent of the board.
   The registers beyond 0x18 only exist on the i82558. */
#define SPEEDO3_TOTAL_SIZE 0x20

int speedo_debug = 2;

/*
				Theory of Operation

I. Board Compatibility

This device driver is designed for the Intel i82557 "Speedo3" chip, Intel's
single-chip fast Ethernet controller for PCI, as used on the Intel
EtherExpress Pro 100 adapter.

II. Board-specific settings

PCI bus devices are configured by the system at boot time, so no jumpers
need to be set on the board.  The system BIOS should be set to assign the
PCI INTA signal to an otherwise unused system IRQ line.  While it's
possible to share PCI interrupt lines, it negatively impacts performance and
only recent kernels support it.

III. Driver operation

IIIA. General
The Speedo3 is very similar to other Intel network chips, that is to say
"apparently designed on a different planet".  This chips retains the complex
Rx and Tx descriptors and multiple buffers pointers as previous chips, but
also has simplified Tx and Rx buffer modes.  This driver uses the "flexible"
Tx mode, but in a simplified lower-overhead manner: it associates only a
single buffer descriptor with each frame descriptor.

Despite the extra space overhead in each receive skbuff, the driver must use
the simplified Rx buffer mode to assure that only a single data buffer is
associated with each RxFD. The driver implements this by reserving space
for the Rx descriptor at the head of each Rx skbuff.

The Speedo-3 has receive and command unit base addresses that are added to
almost all descriptor pointers.  The driver sets these to zero, so that all
pointer fields are absolute addresses.

The System Control Block (SCB) of some previous Intel chips exists on the
chip in both PCI I/O and memory space.  This driver uses the I/O space
registers, but might switch to memory mapped mode to better support non-x86
processors.

IIIB. Transmit structure

The driver must use the complex Tx command+descriptor mode in order to
have a indirect pointer to the skbuff data section.  Each Tx command block
(TxCB) is associated with two immediately appended Tx Buffer Descriptor
(TxBD).  A fixed ring of these TxCB+TxBD pairs are kept as part of the
speedo_private data structure for each adapter instance.

The newer i82558 explicitly supports this structure, and can read the two
TxBDs in the same PCI burst as the TxCB.

This ring structure is used for all normal transmit packets, but the
transmit packet descriptors aren't long enough for most non-Tx commands such
as CmdConfigure.  This is complicated by the possibility that the chip has
already loaded the link address in the previous descriptor.  So for these
commands we convert the next free descriptor on the ring to a NoOp, and point
that descriptor's link to the complex command.

An additional complexity of these non-transmit commands are that they may be
added asynchronous to the normal transmit queue, so we disable interrupts
whenever the Tx descriptor ring is manipulated.

A notable aspect of these special configure commands is that they do
work with the normal Tx ring entry scavenge method.  The Tx ring scavenge
is done at interrupt time using the 'dirty_tx' index, and checking for the
command-complete bit.  While the setup frames may have the NoOp command on the
Tx ring marked as complete, but not have completed the setup command, this
is not a problem.  The tx_ring entry can be still safely reused, as the
tx_skbuff[] entry is always empty for config_cmd and mc_setup frames.

Commands may have bits set e.g. CmdSuspend in the command word to either
suspend or stop the transmit/command unit.  This driver always flags the last
command with CmdSuspend, erases the CmdSuspend in the previous command, and
then issues a CU_RESUME.
Note: Watch out for the potential race condition here: imagine
	erasing the previous suspend
		the chip processes the previous command
		the chip processes the final command, and suspends
	doing the CU_RESUME
		the chip processes the next-yet-valid post-final-command.
So blindly sending a CU_RESUME is only safe if we do it immediately after
after erasing the previous CmdSuspend, without the possibility of an
intervening delay.  Thus the resume command is always within the
interrupts-disabled region.  This is a timing dependence, but handling this
condition in a timing-independent way would considerably complicate the code.

Note: In previous generation Intel chips, restarting the command unit was a
notoriously slow process.  This is presumably no longer true.

IIIC. Receive structure

Because of the bus-master support on the Speedo3 this driver uses the new
SKBUFF_RX_COPYBREAK scheme, rather than a fixed intermediate receive buffer.
This scheme allocates full-sized skbuffs as receive buffers.  The value
SKBUFF_RX_COPYBREAK is used as the copying breakpoint: it is chosen to
trade-off the memory wasted by passing the full-sized skbuff to the queue
layer for all frames vs. the copying cost of copying a frame to a
correctly-sized skbuff.

For small frames the copying cost is negligible (esp. considering that we
are pre-loading the cache with immediately useful header information), so we
allocate a new, minimally-sized skbuff.  For large frames the copying cost
is non-trivial, and the larger copy might flush the cache of useful data, so
we pass up the skbuff the packet was received into.

IV. Notes

Thanks to Steve Williams of Intel for arranging the non-disclosure agreement
that stated that I could disclose the information.  But I still resent
having to sign an Intel NDA when I'm helping Intel sell their own product!

*/

#define MAX_ADDR_LEN    6


struct device
{

    /*
     * This is the first field of the "visible" part of this structure
     * (i.e. as seen by users in the "Space.c" file).  It is the name
     * the interface.
     */
    char            *name;

     /*
     *  I/O specific fields
     *  FIXME: Merge these and struct ifmap into one
     */
    unsigned long   rmem_end; /* shmem "recv" end */
    unsigned long   rmem_start; /* shmem "recv" start */
    unsigned long   mem_end;  /* shared mem end */
    unsigned long   mem_start;  /* shared mem start */
    unsigned long       base_addr;  /* device I/O address   */
    unsigned int        irq;        /* device IRQ number    */
    
    /* Low-level status flags. */
    volatile unsigned char  start;      /* start an operation   */
    /*
     * These two are just single-bit flags, but due to atomicity
     * reasons they have to be inside a "unsigned long". However,
     * they should be inside the SAME unsigned long instead of
     * this wasteful use of memory..
     */
    unsigned char   if_port;  /* Selectable AUI, TP,..*/
    unsigned char   dma;    /* DMA channel    */
                                                                                                                                                                                                        
//  unsigned long   state;
    
    unsigned long       interrupt;  /* bitops.. */
    unsigned long       tbusy;      /* transmitter busy */
    
    struct device       *next;
    
    /*
     * This marks the end of the "visible" part of the structure. All
     * fields hereafter are internal to the system, and may change at
     * will (read: may be cleaned up at will).
     */

    /* These may be needed for future network-power-down code. */
    unsigned long       trans_start;    /* Time (in jiffies) of last Tx */
    unsigned long           last_rx;        /* Time of last Rx      */
    unsigned    mtu;  /* interface MTU value    */
    unsigned short    type; /* interface hardware type  */
    unsigned short    hard_header_len;  /* hardware hdr length  */   
    unsigned short      flags;  /* interface flags (a la BSD) */
    void            *priv;  /* pointer to private data  */

    /* Interface address info. */
    unsigned char   broadcast[MAX_ADDR_LEN];  /* hw bcast add */
    unsigned char   pad;    /* make dev_addr aligned to 8 bytes */
    unsigned char   dev_addr[MAX_ADDR_LEN]; /* hw address */
    unsigned char   addr_len; /* hardware address length  */    

	struct dev_mc_list	*mc_list;	/* Multicast mac addresses	*/
	int			mc_count;	/* Number of installed mcasts	*/
//	int			promiscuity;
//	int			allmulti;

    NetQueue_s		*packet_queue;
    int irq_handle; /* IRQ handler handle */
    
    int node_handle;    /* handle of device node in /dev */
};

static DeviceOperations_s g_sDevOps;

/* This table drives the PCI probe routines. */
//static struct net_device *speedo_found1(struct pci_dev *pdev, int pci_bus, 
//										int pci_devfn, long ioaddr, 
//										int chip_idx, int card_idx);

#ifdef USE_IO
#define SPEEDO_IOTYPE   PCI_USES_MASTER|PCI_USES_IO|PCI_ADDR1
#define SPEEDO_SIZE		32
#else
#define SPEEDO_IOTYPE   PCI_USES_MASTER|PCI_USES_MEM|PCI_ADDR0
#define SPEEDO_SIZE		0x1000
#endif

enum pci_flags_bit {
	PCI_USES_IO=1, PCI_USES_MEM=2, PCI_USES_MASTER=4,
	PCI_ADDR0=0x10<<0, PCI_ADDR1=0x10<<1, PCI_ADDR2=0x10<<2, PCI_ADDR3=0x10<<3,
};
struct pci_id_info {
	char *name;
	u16	vendor_id, device_id;
	int pci_index;
} static pci_tbl[] = {
	{ "Intel PCI EtherExpress Pro100 82557",
	  PCI_VENDOR_ID_INTEL, PCI_DEVICE_ID_INTEL_82557,
	  0
	},
	{ "Intel PCI EtherExpress Pro100 82559ER",
	  PCI_VENDOR_ID_INTEL, PCI_DEVICE_ID_INTEL_82559ER,
	  0
	},
	{ "Intel PCI EtherExpress Pro100 ID1029",
	  PCI_VENDOR_ID_INTEL, PCI_DEVICE_ID_INTEL_ID1029,
	  0 
	},
	{ "Intel Corporation 82559 InBusiness 10/100",
	  PCI_VENDOR_ID_INTEL, PCI_DEVICE_ID_INTEL_ID1030,
	  0
	},
	{ "Intel Pro/100 VE (LOM)",
	  PCI_VENDOR_ID_INTEL, PCI_DEVICE_ID_INTEL_ID1031,
	  0 
	},
	{ "Intel Pro/100 VE",
	  PCI_VENDOR_ID_INTEL, 0x1032,
	  0 
	},
	{ "Intel Pro/100 VM (LOM)",
	  PCI_VENDOR_ID_INTEL, 0x1033,
	  0 
	},
	{ "Intel Pro/100 VM",
	  PCI_VENDOR_ID_INTEL, 0x1034,
	  0 
	},
	{ "Intel Pro/100 82562EH (LOM)",
	  PCI_VENDOR_ID_INTEL, 0x1035,
	  0 
	},
	{ "Intel Pro/100 82562EH",
	  PCI_VENDOR_ID_INTEL, 0x1036,
	  0 
	},
	{ "Intel Pro/100 ICH3 Chipset Ethernet Controller",
	  PCI_VENDOR_ID_INTEL, 0x1037,
	  0
	},
	{ "Intel Pro/100 VM (KM)",
	  PCI_VENDOR_ID_INTEL, 0x1038,
	  0 
	},
	{ "Intel Pro/100 VE (LOM)",
	  PCI_VENDOR_ID_INTEL, 0x1039,
	  0 
	},
	{ "Intel Pro/100 VE (CNR)",
	  PCI_VENDOR_ID_INTEL, 0x103a,
	  0 
	},
	{ "Intel Pro/100 VM (LOM)",
	  PCI_VENDOR_ID_INTEL, 0x103b,
	  0 
	},
	{ "Intel Pro/100 VM (CNR)",
	  PCI_VENDOR_ID_INTEL, 0x103c,
	  0 
	},
	{ "Intel Pro/100 VE (MOB)",
	  PCI_VENDOR_ID_INTEL, 0x103d,
	  0 
	},
	{ "Intel Pro/100 VM (MOB)",
	  PCI_VENDOR_ID_INTEL, 0x103e,
	  0 
	},
	{ "Intel 82801BA (D865) Pro/100 VE",
	  PCI_VENDOR_ID_INTEL, 0x1050,
	  0 
	},
	{ "Intel 82551QM Pro/100 M Mobile",
	  PCI_VENDOR_ID_INTEL, 0x1059,
	  0 
	},
	{ "Intel PCI EtherExpress Pro100 82562EM",
	  PCI_VENDOR_ID_INTEL, PCI_DEVICE_ID_INTEL_ID2449,
	  0
	},
	{0,}						/* 0 terminated list. */
};

static inline unsigned int io_inw(unsigned long port)
{
	return inw(port);
}
static inline void io_outw(unsigned int val, unsigned long port)
{
	outw(val, port);
}

#ifndef USE_IO
#undef inb
#undef inw
#undef inl
#undef outb
#undef outw
#undef outl
#define inb readb
#define inw readw
#define inl readl
#define outb writeb
#define outw writew
#define outl writel
#endif

/* How to wait for the command unit to accept a command.
   Typically this takes 0 ticks. */
static inline void wait_for_cmd_done(long cmd_ioaddr)
{
	int wait = 20000;
	char cmd_reg1, cmd_reg2;
	do   ;
	while((cmd_reg1 = inb(cmd_ioaddr)) && (--wait >= 0));

	/* Last chance to change your mind --Dragan*/
	if (wait < 0){
		cmd_reg2 = inb(cmd_ioaddr);
		if(cmd_reg2){
			printk(KERN_ALERT "eepro100: cmd_wait for(%#2.2x) timedout with(%#2.2x)!\n",
				cmd_reg1, cmd_reg2);
		
		}
	}

}

/* Offsets to the various registers.
   All accesses need not be longword aligned. */
enum speedo_offsets {
	SCBStatus = 0, SCBCmd = 2,	/* Rx/Command Unit command and status. */
	SCBPointer = 4,				/* General purpose pointer. */
	SCBPort = 8,				/* Misc. commands and operands.  */
	SCBflash = 12, SCBeeprom = 14, /* EEPROM and flash memory control. */
	SCBCtrlMDI = 16,			/* MDI interface control. */
	SCBEarlyRx = 20,			/* Early receive byte count. */
};
/* Commands that can be put in a command list entry. */
enum commands {
	CmdNOp = 0, CmdIASetup = 0x10000, CmdConfigure = 0x20000,
	CmdMulticastList = 0x30000, CmdTx = 0x40000, CmdTDR = 0x50000,
	CmdDump = 0x60000, CmdDiagnose = 0x70000,
	CmdSuspend = 0x40000000,	/* Suspend after completion. */
	CmdIntr = 0x20000000,		/* Interrupt after completion. */
	CmdTxFlex = 0x00080000,		/* Use "Flexible mode" for CmdTx command. */
};
/* Clear CmdSuspend (1<<30) avoiding interference with the card access to the
   status bits.  Previous driver versions used separate 16 bit fields for
   commands and statuses.  --SAW
 */
#define clear_suspend(cmd)  ((u16 *)&(cmd)->cmd_status)[1] &= ~0x4000

enum SCBCmdBits {
     SCBMaskCmdDone=0x8000, SCBMaskRxDone=0x4000, SCBMaskCmdIdle=0x2000,
     SCBMaskRxSuspend=0x1000, SCBMaskEarlyRx=0x0800, SCBMaskFlowCtl=0x0400,
     SCBTriggerIntr=0x0200, SCBMaskAll=0x0100,
     /* The rest are Rx and Tx commands. */
     CUStart=0x0010, CUResume=0x0020, CUStatsAddr=0x0040, CUShowStats=0x0050,
     CUCmdBase=0x0060,  /* CU Base address (set to zero) . */
     CUDumpStats=0x0070, /* Dump then reset stats counters. */
     RxStart=0x0001, RxResume=0x0002, RxAbort=0x0004, RxAddrLoad=0x0006,
     RxResumeNoResources=0x0007,
};

enum SCBPort_cmds {
	PortReset=0, PortSelfTest=1, PortPartialReset=2, PortDump=3,
};

/* The Speedo3 Rx and Tx frame/buffer descriptors. */
struct descriptor {			    /* A generic descriptor. */
	s32 cmd_status;				/* All command and status fields. */
	u32 link;				    /* struct descriptor *  */
	unsigned char params[0];
};

/* The Speedo3 Rx and Tx buffer descriptors. */
struct RxFD {					/* Receive frame descriptor. */
	s32 status;
	u32 link;					/* struct RxFD * */
	u32 rx_buf_addr;			/* void * */
	u32 count;
};

/* Selected elements of the Tx/RxFD.status word. */
enum RxFD_bits {
	RxComplete=0x8000, RxOK=0x2000,
	RxErrCRC=0x0800, RxErrAlign=0x0400, RxErrTooBig=0x0200, RxErrSymbol=0x0010,
	RxEth2Type=0x0020, RxNoMatch=0x0004, RxNoIAMatch=0x0002,
	TxUnderrun=0x1000,  StatusComplete=0x8000,
};

struct TxFD {					/* Transmit frame descriptor set. */
	s32 status;
	u32 link;					/* void * */
	u32 tx_desc_addr;			/* Always points to the tx_buf_addr element. */
	s32 count;					/* # of TBD (=1), Tx start thresh., etc. */
	/* This constitutes two "TBD" entries -- we only use one. */
	u32 tx_buf_addr0;			/* void *, frame to be transmitted.  */
	s32 tx_buf_size0;			/* Length of Tx frame. */
	u32 tx_buf_addr1;			/* void *, frame to be transmitted.  */
	s32 tx_buf_size1;			/* Length of Tx frame. */
};

/* Multicast filter setting block.  --SAW */
struct speedo_mc_block {
	struct speedo_mc_block *next;
	unsigned int tx;
	struct descriptor frame __attribute__ ((__aligned__(16)));
};

/* Elements of the dump_statistics block. This block must be lword aligned. */
struct speedo_stats {
	u32 tx_good_frames;
	u32 tx_coll16_errs;
	u32 tx_late_colls;
	u32 tx_underruns;
	u32 tx_lost_carrier;
	u32 tx_deferred;
	u32 tx_one_colls;
	u32 tx_multi_colls;
	u32 tx_total_colls;
	u32 rx_good_frames;
	u32 rx_crc_errs;
	u32 rx_align_errs;
	u32 rx_resource_errs;
	u32 rx_overrun_errs;
	u32 rx_colls_errs;
	u32 rx_runt_errs;
	u32 done_marker;
};

enum Rx_ring_state_bits {
	RrNoMem=1, RrPostponed=2, RrNoResources=4, RrOOMReported=8,
};

/* Do not change the position (alignment) of the first few elements!
   The later elements are grouped for cache locality. */
struct speedo_private {
	struct TxFD	tx_ring[TX_RING_SIZE];	/* Commands (usually CmdTxPacket). */
	struct RxFD *rx_ringp[RX_RING_SIZE];	/* Rx descriptor, used as ring. */
	/* The addresses of a Tx/Rx-in-place packets/buffers. */
	PacketBuf_s *tx_skbuff[TX_RING_SIZE];
	PacketBuf_s *rx_skbuff[RX_RING_SIZE];
	struct descriptor  *last_cmd;	/* Last command sent. */
	unsigned int cur_tx, dirty_tx;	/* The ring entries to be free()ed. */
	SpinLock_s lock;				/* Group with Tx control cache line. */
	u32 tx_threshold;					/* The value for txdesc.count. */
	struct RxFD *last_rxf;	/* Last command sent. */
	unsigned int cur_rx, dirty_rx;		/* The next free ring entry */
	long last_rx_time;			/* Last Rx, in jiffies, to handle Rx hang. */
	const char *product_name;
	struct net_device *next_module;
	void *priv_addr;					/* Unaligned address for kfree */
	struct speedo_stats lstats;
	int chip_id;
	PCI_Info_s *pci_dev;
	unsigned char acpi_pwr;
	ktimer_t timer;	/* Media selection timer. */
	struct speedo_mc_block *mc_setup_head;/* Multicast setup frame list head. */
	struct speedo_mc_block *mc_setup_tail;/* Multicast setup frame list tail. */
	int in_interrupt;					/* Word-aligned dev->interrupt */
	char rx_mode;						/* Current PROMISC/ALLMULTI setting. */
	unsigned int tx_full:1;				/* The Tx queue is full. */
	unsigned int full_duplex:1;			/* Full-duplex operation requested. */
	unsigned int flow_ctrl:1;			/* Use 802.3x flow control. */
	unsigned int rx_bug:1;				/* Work around receiver hang errata. */
	unsigned int rx_bug10:1;			/* Receiver might hang at 10mbps. */
	unsigned int rx_bug100:1;			/* Receiver might hang at 100mbps. */
	unsigned char default_port:8;		/* Last dev->if_port value. */
	unsigned char rx_ring_state;		/* RX ring status flags. */
	unsigned short phy[2];				/* PHY media interfaces available. */
	unsigned short advertising;			/* Current PHY advertised caps. */
	unsigned short partner;				/* Link partner caps. */
};

/* The parameters for a CmdConfigure operation.
   There are so many options that it would be difficult to document each bit.
   We mostly use the default or recommended settings. */
const char i82557_config_cmd[22] = {
	22, 0x08, 0, 0,  0, 0, 0x32, 0x03,  1, /* 1=Use MII  0=Use AUI */
	0, 0x2E, 0,  0x60, 0,
	0xf2, 0x48,   0, 0x40, 0xf2, 0x80, 		/* 0x40=Force full-duplex */
	0x3f, 0x05, };
const char i82558_config_cmd[22] = {
	22, 0x08, 0, 1,  0, 0, 0x22, 0x03,  1, /* 1=Use MII  0=Use AUI */
	0, 0x2E, 0,  0x60, 0x08, 0x88,
	0x68, 0, 0x40, 0xf2, 0x84,		/* Disable FC */
	0x31, 0x05, };

/* PHY media interface chips. */
static const char *phys[] = {
	"None", "i82553-A/B", "i82553-C", "i82503",
	"DP83840", "80c240", "80c24", "i82555",
	"unknown-8", "unknown-9", "DP83840A", "unknown-11",
	"unknown-12", "unknown-13", "unknown-14", "unknown-15", };
enum phy_chips { NonSuchPhy=0, I82553AB, I82553C, I82503, DP83840, S80C240,
					 S80C24, I82555, DP83840A=10, };
static const char is_mii[] = { 0, 1, 1, 0, 1, 1, 0, 1 };
#define EE_READ_CMD		(6)

#ifdef honor_default_port
/* Optional driver feature to allow forcing the transceiver setting.
   Not recommended. */
static int mii_ctrl[8] = { 0x3300, 0x3100, 0x0000, 0x0100,
						   0x2000, 0x2100, 0x0400, 0x3100};
#endif

/* A list of all installed Speedo devices, for removing the driver module. */
static struct net_device *root_speedo_dev = NULL;

/* ------------------- */
static int do_eeprom_cmd(long ioaddr, int cmd, int cmd_len);
static int mdio_read(long ioaddr, int phy_id, int location);
static int mdio_write(long ioaddr, int phy_id, int location, int value);
static int speedo_open(struct net_device *dev);
static void speedo_resume(struct net_device *dev);
static void speedo_timer(unsigned long data);
int speedo_init_rx_ring(struct net_device *dev);
static void speedo_tx_timeout(struct net_device *dev);
static int speedo_start_xmit(PacketBuf_s *skb, struct net_device *dev);
static int speedo_refill_rx_buf(struct net_device *dev, int force);
static void speedo_refill_rx_buffers(struct net_device *dev, int force);
static int speedo_rx(struct net_device *dev);
static void speedo_tx_buffer_gc(struct net_device *dev);


/* Serial EEPROM section.
   A "bit" grungy, but we work our way through bit-by-bit :->. */
/*  EEPROM_Ctrl bits. */
#define EE_SHIFT_CLK	0x01	/* EEPROM shift clock. */
#define EE_CS			0x02	/* EEPROM chip select. */
#define EE_DATA_WRITE	0x04	/* EEPROM chip data in. */
#define EE_DATA_READ	0x08	/* EEPROM chip data out. */
#define EE_ENB			(0x4800 | EE_CS)
#define EE_WRITE_0		0x4802
#define EE_WRITE_1		0x4806
#define EE_OFFSET		SCBeeprom

/* The fixes for the code were kindly provided by Dragan Stancevic
   <visitor@valinux.com> to strictly follow Intel specifications of EEPROM
   access timing.
   The publicly available sheet 64486302 (sec. 3.1) specifies 1us access
   interval for serial EEPROM.  However, it looks like that there is an
   additional requirement dictating larger udelay's in the code below.
   2000/05/24  SAW */
static int do_eeprom_cmd(long ioaddr, int cmd, int cmd_len)
{
	unsigned retval = 0;
	long ee_addr = ioaddr + SCBeeprom;

	io_outw(EE_ENB, ee_addr); udelay(2);
	io_outw(EE_ENB | EE_SHIFT_CLK, ee_addr); udelay(2);

	/* Shift the command bits out. */
	do {
		short dataval = (cmd & (1 << cmd_len)) ? EE_WRITE_1 : EE_WRITE_0;
		io_outw(dataval, ee_addr); udelay(2);
		io_outw(dataval | EE_SHIFT_CLK, ee_addr); udelay(2);
		retval = (retval << 1) | ((io_inw(ee_addr) & EE_DATA_READ) ? 1 : 0);
	} while (--cmd_len >= 0);
	io_outw(EE_ENB, ee_addr); udelay(2);

	/* Terminate the EEPROM access. */
	io_outw(EE_ENB & ~EE_CS, ee_addr);
	return retval;
}

static int mdio_read(long ioaddr, int phy_id, int location)
{
	int val, boguscnt = 64*10;		/* <64 usec. to complete, typ 27 ticks */
	outl(0x08000000 | (location<<16) | (phy_id<<21), ioaddr + SCBCtrlMDI);
	do {
		val = inl(ioaddr + SCBCtrlMDI);
		if (--boguscnt < 0) {
			printk(KERN_ERR " mdio_read() timed out with val = %8.8x.\n", val);
			break;
		}
	} while (! (val & 0x10000000));
	return val & 0xffff;
}

static int mdio_write(long ioaddr, int phy_id, int location, int value)
{
	int val, boguscnt = 64*10;		/* <64 usec. to complete, typ 27 ticks */
	outl(0x04000000 | (location<<16) | (phy_id<<21) | value,
		 ioaddr + SCBCtrlMDI);
	do {
		val = inl(ioaddr + SCBCtrlMDI);
		if (--boguscnt < 0) {
			printk(KERN_ERR" mdio_write() timed out with val = %8.8x.\n", val);
			break;
		}
	} while (! (val & 0x10000000));
	return val & 0xffff;
}

static struct net_device *speedo_found1(PCI_Info_s *pdev, long ioaddr,
    int chip_idx, int card_idx, int device_handle)
{
	struct net_device *dev;
	struct speedo_private *sp;
	char *product;
	int i, option;
	u16 eeprom[0x100];
	int acpi_idle_state = 0;
	char node_path[64];

	//dev = init_etherdev(NULL, sizeof(struct speedo_private));
	dev = kmalloc(sizeof(struct speedo_private), MEMF_KERNEL | MEMF_CLEAR);
	if (dev == NULL) {
		printk(KERN_ERR "eepro100: Unable to allocate net_device structure!\n");
		return NULL;
	}

	if (dev->mem_start > 0)
		option = dev->mem_start;
	else if (card_idx >= 0  &&  options[card_idx] >= 0)
		option = options[card_idx];
	else
		option = 0;

	/* Read the station address EEPROM before doing the reset.
	   Nominally his should even be done before accepting the device, but
	   then we wouldn't have a device name with which to report the error.
	   The size test is for 6 bit vs. 8 bit address serial EEPROMs.
	*/
	{
		unsigned long iobase;
		int read_cmd, ee_size;
		u16 sum;
		int j;

		/* Use IO only to avoid postponed writes and satisfy EEPROM timing
		   requirements. */
		iobase = pdev->u.h0.nBase1 & PCI_ADDRESS_IO_MASK;
		if ((do_eeprom_cmd(iobase, EE_READ_CMD << 24, 27) & 0xffe0000)
			== 0xffe0000) {
			ee_size = 0x100;
			read_cmd = EE_READ_CMD << 24;
		} else {
			ee_size = 0x40;
			read_cmd = EE_READ_CMD << 22;
		}

		for (j = 0, i = 0, sum = 0; i < ee_size; i++) {
			u16 value = do_eeprom_cmd(iobase, read_cmd | (i << 16), 27);
			eeprom[i] = value;
			sum += value;
			if (i < 3) {
				dev->dev_addr[j++] = value;
				dev->dev_addr[j++] = value >> 8;
			}
		}
		if ((u32)sum != 0xBABA)
			printk(KERN_WARNING "%s: Invalid EEPROM checksum %#4.4x, "
				   "check settings before activating this device!\n",
				   dev->name, sum);
		/* Don't  unregister_netdev(dev);  as the EEPro may actually be
		   usable, especially if the MAC address is set later.
		   On the other hand, it may be unusable if MDI data is corrupted. */
	}

	/* Reset the chip: stop Tx and Rx processes and clear counters.
	   This takes less than 10usec and will easily finish before the next
	   action. */
	outl(PortReset, ioaddr + SCBPort);
	inl(ioaddr + SCBPort);
	/* Honor PortReset timing. */
	udelay(10);

	if (eeprom[3] & 0x0100)
		product = "OEM i82557/i82558 10/100 Ethernet";
	else
		product = pci_tbl[chip_idx].name;

	printk(KERN_INFO "eepro100: %s, I/O at %#3lx, IRQ %d.\n",
		product, ioaddr, pdev->u.h0.nInterruptLine);



	if (speedo_debug > 2)
	{
	    for (i = 0; i < 5; i++)
	    	printk("%2.2X:", dev->dev_addr[i]);
	    printk("%2.2X: ", dev->dev_addr[i]);

	    /* OK, this is pure kernel bloat.  I don't like it when other drivers
	       waste non-pageable kernel space to emit similar messages, but I need
	       them for bug reports. */
	    {
	    	const char *connectors[] = {" RJ45", " BNC", " AUI", " MII"};
	    	/* The self-test results must be paragraph aligned. */
	    	s32 str[6], *volatile self_test_results;
	    	int boguscnt = 16000;	/* Timeout for set-test. */
	    	if ((eeprom[3] & 0x03) != 0x03)
	    		printk(KERN_INFO "  Receiver lock-up bug exists -- enabling"
	    			   " work-around.\n");
	    	printk(KERN_INFO "  Board assembly %4.4x%2.2x-%3.3d, Physical"
	    		   " connectors present:",
	    		   eeprom[8], eeprom[9]>>8, eeprom[9] & 0xff);
	    	for (i = 0; i < 4; i++)
	    		if (eeprom[5] & (1<<i))
	    			printk(connectors[i]);
	    	printk("\n"KERN_INFO"  Primary interface chip %s PHY #%d.\n",
	    		   phys[(eeprom[6]>>8)&15], eeprom[6] & 0x1f);
	    	if (eeprom[7] & 0x0700)
	    		printk(KERN_INFO "    Secondary interface chip %s.\n",
	    			   phys[(eeprom[7]>>8)&7]);
	    	if (((eeprom[6]>>8) & 0x3f) == DP83840
	    		||  ((eeprom[6]>>8) & 0x3f) == DP83840A) {
	    		int mdi_reg23 = mdio_read(ioaddr, eeprom[6] & 0x1f, 23) | 0x0422;
	    		if (congenb)
	    		  mdi_reg23 |= 0x0100;
	    		printk(KERN_INFO"  DP83840 specific setup, setting register 23 to %4.4x.\n",
	    			   mdi_reg23);
	    		mdio_write(ioaddr, eeprom[6] & 0x1f, 23, mdi_reg23);
	    	}
	    	if ((option >= 0) && (option & 0x70)) {
	    		printk(KERN_INFO "  Forcing %dMbs %s-duplex operation.\n",
	    			   (option & 0x20 ? 100 : 10),
	    			   (option & 0x10 ? "full" : "half"));
	    		mdio_write(ioaddr, eeprom[6] & 0x1f, 0,
	    				   ((option & 0x20) ? 0x2000 : 0) | 	/* 100mbps? */
	    				   ((option & 0x10) ? 0x0100 : 0)); /* Full duplex? */
	    	}

	    	/* Perform a system self-test. */
	    	self_test_results = (s32*) ((((long) str) + 15) & ~0xf);
	    	self_test_results[0] = 0;
	    	self_test_results[1] = -1;
	    	outl(virt_to_bus(self_test_results) | PortSelfTest, ioaddr + SCBPort);
	    	do {
	    		udelay(10);
	    	} while (self_test_results[1] == -1  &&  --boguscnt >= 0);
    
    		if (boguscnt < 0) {		/* Test optimized out. */
    			printk(KERN_ERR "Self test failed, status %8.8x:\n"
    				   KERN_ERR " Failure to initialize the i82557.\n"
    				   KERN_ERR " Verify that the card is a bus-master"
    				   " capable slot.\n",
    				   (unsigned int) self_test_results[1]);
    		} else
    			printk(KERN_INFO "  General self-test: %s.\n"
    				   KERN_INFO "  Serial sub-system self-test: %s.\n"
    				   KERN_INFO "  Internal registers self-test: %s.\n"
    				   KERN_INFO "  ROM checksum self-test: %s (%#8.8x).\n",
    				   self_test_results[1] & 0x1000 ? "failed" : "passed",
    				   self_test_results[1] & 0x0020 ? "failed" : "passed",
    				   self_test_results[1] & 0x0008 ? "failed" : "passed",
    				   self_test_results[1] & 0x0004 ? "failed" : "passed",
    				   (unsigned int) self_test_results[0]);
    	    }
	} /* kernel_bloat */

	outl(PortReset, ioaddr + SCBPort);
	inl(ioaddr + SCBPort);
	/* Honor PortReset timing. */
	udelay(10);

	dev->base_addr = ioaddr;
	dev->irq = pdev->u.h0.nInterruptLine;
	dev->name = product;

	sp = dev->priv;
	if (dev->priv == NULL) {
		void *mem = kmalloc(sizeof(*sp), MEMF_KERNEL);
		dev->priv = sp = mem;		/* Cache align here if kmalloc does not. */
		sp->priv_addr = mem;
	}
	memset(sp, 0, sizeof(*sp));
	sp->next_module = root_speedo_dev;
	root_speedo_dev = dev;

	sp->chip_id = chip_idx;
	sp->pci_dev = pdev;
	sp->acpi_pwr = acpi_idle_state;

	sp->full_duplex = option >= 0 && (option & 0x10) ? 1 : 0;
	if (card_idx >= 0) {
		if (full_duplex[card_idx] >= 0)
			sp->full_duplex = full_duplex[card_idx];
	}
	sp->default_port = option >= 0 ? (option & 0x0f) : 0;

	sp->phy[0] = eeprom[6];
	sp->phy[1] = eeprom[7];
	sp->rx_bug = (eeprom[3] & 0x03) == 3 ? 0 : 1;

	if (sp->rx_bug)
		printk(KERN_INFO "eepro100: Receiver lock-up workaround activated.\n");

	/* The Speedo-specific entries in the device structure. */
	//dev->open = &speedo_open;
	//dev->hard_start_xmit = &speedo_start_xmit;
#if defined(HAS_NETIF_QUEUE)
	//dev->tx_timeout = &speedo_tx_timeout;
	//dev->watchdog_timeo = TX_TIMEOUT;
#endif
	//dev->stop = &speedo_close;
	//dev->get_stats = &speedo_get_stats;
	//dev->set_multicast_list = &set_rx_mode;
	//dev->do_ioctl = &speedo_ioctl;

	sp->timer = create_timer();
	
	/* Create AtheOS device node */
	sprintf( node_path, "net/eth/eepro100-%d", card_idx );
	dev->node_handle = create_device_node( device_handle, pdev->nHandle, node_path, &g_sDevOps, dev );

	return dev;
}

static void speedo_tx_buffer_gc(struct net_device *dev)
{
	unsigned int dirty_tx;
	struct speedo_private *sp = (struct speedo_private *)dev->priv;

	dirty_tx = sp->dirty_tx;
	while ((int)(sp->cur_tx - dirty_tx) > 0) {
		int entry = dirty_tx % TX_RING_SIZE;
		int status = le32_to_cpu(sp->tx_ring[entry].status);

		if (speedo_debug > 5)
			printk(KERN_DEBUG " scavenge candidate %d status %4.4x.\n",
				   entry, status);
		if ((status & StatusComplete) == 0)
			break;			/* It still hasn't been processed. */
		if (status & TxUnderrun)
			if (sp->tx_threshold < 0x01e08000) {
				if (speedo_debug > 2)
					printk(KERN_DEBUG "%s: TX underrun, threshold adjusted.\n",
						   dev->name);
				sp->tx_threshold += 0x00040000;
			}
		/* Free the original skb. */
		if (sp->tx_skbuff[entry]) {
			//sp->stats.tx_packets++;	/* Count only user packets. */
			//sp->stats.tx_bytes += sp->tx_skbuff[entry]->pb_nSize;
			free_pkt_buffer(sp->tx_skbuff[entry]);
			sp->tx_skbuff[entry] = 0;
		}
		dirty_tx++;
	}

	if (speedo_debug && (int)(sp->cur_tx - dirty_tx) > TX_RING_SIZE) {
		printk(KERN_ERR "out-of-sync dirty pointer, %d vs. %d,"
			   " full=%d.\n",
			   dirty_tx, sp->cur_tx, sp->tx_full);
		dirty_tx += TX_RING_SIZE;
	}

#ifndef __ATHEOS__
	while (sp->mc_setup_head != NULL
		   && (int)(dirty_tx - sp->mc_setup_head->tx - 1) > 0) {
		struct speedo_mc_block *t;
		if (speedo_debug > 1)
			printk(KERN_DEBUG "%s: freeing mc frame.\n", dev->name);
		t = sp->mc_setup_head->next;
		kfree(sp->mc_setup_head);
		sp->mc_setup_head = t;
	}
	if (sp->mc_setup_head == NULL)
		sp->mc_setup_tail = NULL;
#endif

	sp->dirty_tx = dirty_tx;
}


int speedo_init_rx_ring(struct net_device *dev)
{
	struct speedo_private *sp = (struct speedo_private *)dev->priv;
	struct RxFD *rxf, *last_rxf = NULL;
	int i;

	sp->cur_rx = 0;

	for (i = 0; i < RX_RING_SIZE; i++) {
		PacketBuf_s *skb;
		skb = alloc_pkt_buffer(PKT_BUF_SZ + sizeof(struct RxFD));
		sp->rx_skbuff[i] = skb;
		if (skb == NULL)
			break;			/* OK.  Just initially short of Rx bufs. */
		//skb->dev = dev;			/* Mark as being used by this device. */
		skb->pb_nSize = 0;
		rxf = (struct RxFD *) skb->pb_pData;
		sp->rx_ringp[i] = rxf;
		//skb_reserve(skb, sizeof(struct RxFD));
		skb->pb_pData += sizeof(struct RxFD); /* !!! */
		if (last_rxf)
			last_rxf->link = virt_to_le32desc(rxf);
		last_rxf = rxf;
		rxf->status = cpu_to_le32(0x00000001);	/* '1' is flag value only. */
		rxf->link = 0;						/* None yet. */
		/* This field unused by i82557. */
		rxf->rx_buf_addr = 0xffffffff;
		rxf->count = cpu_to_le32(PKT_BUF_SZ << 16);
	}
	sp->dirty_rx = (unsigned int)(i - RX_RING_SIZE);
	/* Mark the last entry as end-of-list. */
	last_rxf->status = cpu_to_le32(0xC0000002);	/* '2' is flag value only. */
	sp->last_rxf = last_rxf;
	return 0;
}

/* Start the chip hardware after a full reset. */
static void speedo_resume(struct net_device *dev)
{
	struct speedo_private *sp = (struct speedo_private *)dev->priv;
	long ioaddr = dev->base_addr;

	/* Start with a Tx threshold of 256 (0x..20.... 8 byte units). */
	sp->tx_threshold = 0x01208000;

	/* Set the segment registers to '0'. */
	wait_for_cmd_done(ioaddr + SCBCmd);
	outl(0, ioaddr + SCBPointer);
	/* impose a delay to avoid a bug */
	inl(ioaddr + SCBPointer);
	udelay(10);
	outb(RxAddrLoad, ioaddr + SCBCmd);
	wait_for_cmd_done(ioaddr + SCBCmd);
	outb(CUCmdBase, ioaddr + SCBCmd);
	wait_for_cmd_done(ioaddr + SCBCmd);

	/* Load the statistics block and rx ring addresses. */
	outl(virt_to_bus(&sp->lstats), ioaddr + SCBPointer);
	outb(CUStatsAddr, ioaddr + SCBCmd);
	sp->lstats.done_marker = 0;
	wait_for_cmd_done(ioaddr + SCBCmd);

	if (sp->rx_ringp[sp->cur_rx % RX_RING_SIZE] == NULL) {
		if (speedo_debug > 2)
			printk(KERN_DEBUG "%s: NULL cur_rx in speedo_resume().\n",
					dev->name);
	} else {
		outl(virt_to_bus(sp->rx_ringp[sp->cur_rx % RX_RING_SIZE]),
			 ioaddr + SCBPointer);
		outb(RxStart, ioaddr + SCBCmd);
		wait_for_cmd_done(ioaddr + SCBCmd);
	}

	outb(CUDumpStats, ioaddr + SCBCmd);

	/* Fill the first command with our physical address. */
	{
		struct descriptor *ias_cmd;

		ias_cmd =
			(struct descriptor *)&sp->tx_ring[sp->cur_tx++ % TX_RING_SIZE];
		/* Avoid a bug(?!) here by marking the command already completed. */
		ias_cmd->cmd_status = cpu_to_le32((CmdSuspend | CmdIASetup) | 0xa000);
		ias_cmd->link =
			virt_to_le32desc(&sp->tx_ring[sp->cur_tx % TX_RING_SIZE]);
		memcpy(ias_cmd->params, dev->dev_addr, 6);
		sp->last_cmd = ias_cmd;
	}

	/* Start the chip's Tx process and unmask interrupts. */
	wait_for_cmd_done(ioaddr + SCBCmd);
	outl(virt_to_bus(&sp->tx_ring[sp->dirty_tx % TX_RING_SIZE]),
		 ioaddr + SCBPointer);
	/* We are not ACK-ing FCP and ER in the interrupt handler yet so they should
	   remain masked --Dragan */
	outw(CUStart | SCBMaskEarlyRx | SCBMaskFlowCtl, ioaddr + SCBCmd);
}

static void speedo_refill_rx_buffers(struct net_device *dev, int force)
{
	struct speedo_private *sp = (struct speedo_private *)dev->priv;

	/* Refill the RX ring. */
	while ((int)(sp->cur_rx - sp->dirty_rx) > 0 &&
			speedo_refill_rx_buf(dev, force) != -1);
}

/* The interrupt handler does all of the Rx thread work and cleans up
   after the Tx thread. */
static int speedo_interrupt(int irq, void *dev_instance, SysCallRegs_s *regs)
{
	struct net_device *dev = (struct net_device *)dev_instance;
	struct speedo_private *sp;
	long ioaddr, boguscnt = max_interrupt_work;
	unsigned short status;

#ifndef final_version
	if (dev == NULL) {
		printk(KERN_ERR "speedo_interrupt(): irq %d for unknown device.\n", irq);
		return 0;
	}
#endif

	ioaddr = dev->base_addr;
	sp = (struct speedo_private *)dev->priv;

#ifndef final_version
	/* A lock to prevent simultaneous entry on SMP machines. */
	if (test_and_set_bit(0, (void*)&sp->in_interrupt)) {
		printk(KERN_ERR"%s: SMP simultaneous entry of an interrupt handler.\n",
			   dev->name);
		sp->in_interrupt = 0;	/* Avoid halting machine. */
		return 0;
	}
	dev->interrupt = 1;
#endif

	do {
		status = inw(ioaddr + SCBStatus);
		/* Acknowledge all of the current interrupt sources ASAP. */
		/* Will change from 0xfc00 to 0xff00 when we start handling
		   FCP and ER interrupts --Dragan */
		outw(status & 0xfc00, ioaddr + SCBStatus);

		if (speedo_debug > 3)
			printk(KERN_DEBUG "%s: interrupt  status=%#4.4x.\n",
				   dev->name, status);

		if ((status & 0xfc00) == 0)
			break;

		/* Always check if all rx buffers are allocated.  --SAW */
		speedo_refill_rx_buffers(dev, 0);

		if ((status & 0x5000) ||	/* Packet received, or Rx error. */
			(sp->rx_ring_state&(RrNoMem|RrPostponed)) == RrPostponed)
									/* Need to gather the postponed packet. */
			speedo_rx(dev);

		if (status & 0x1000) {
			spin_lock(&sp->lock);
			if ((status & 0x003c) == 0x0028) {		/* No more Rx buffers. */
				struct RxFD *rxf;
				printk(KERN_WARNING "%s: card reports no RX buffers.\n",
						dev->name);
				rxf = sp->rx_ringp[sp->cur_rx % RX_RING_SIZE];
				if (rxf == NULL) {
					if (speedo_debug > 2)
						printk(KERN_DEBUG
								"%s: NULL cur_rx in speedo_interrupt().\n",
								dev->name);
					sp->rx_ring_state |= RrNoMem|RrNoResources;
				} else if (rxf == sp->last_rxf) {
					if (speedo_debug > 2)
						printk(KERN_DEBUG
								"%s: cur_rx is last in speedo_interrupt().\n",
								dev->name);
					sp->rx_ring_state |= RrNoMem|RrNoResources;
				} else
					outb(RxResumeNoResources, ioaddr + SCBCmd);
			} else if ((status & 0x003c) == 0x0008) { /* No resources. */
				struct RxFD *rxf;
				printk(KERN_WARNING "%s: card reports no resources.\n",
						dev->name);
				rxf = sp->rx_ringp[sp->cur_rx % RX_RING_SIZE];
				if (rxf == NULL) {
					if (speedo_debug > 2)
						printk(KERN_DEBUG
								"%s: NULL cur_rx in speedo_interrupt().\n",
								dev->name);
					sp->rx_ring_state |= RrNoMem|RrNoResources;
				} else if (rxf == sp->last_rxf) {
					if (speedo_debug > 2)
						printk(KERN_DEBUG
								"%s: cur_rx is last in speedo_interrupt().\n",
								dev->name);
					sp->rx_ring_state |= RrNoMem|RrNoResources;
				} else {
					/* Restart the receiver. */
					outl(virt_to_bus(sp->rx_ringp[sp->cur_rx % RX_RING_SIZE]),
					   ioaddr + SCBPointer);
					outb(RxStart, ioaddr + SCBCmd);
				}
			}
			//sp->stats.rx_errors++;
			spin_unlock(&sp->lock);
		}

		if ((sp->rx_ring_state&(RrNoMem|RrNoResources)) == RrNoResources) {
			printk(KERN_WARNING
					"%s: restart the receiver after a possible hang.\n",
					dev->name);
			spin_lock(&sp->lock);
			/* Restart the receiver.
			   I'm not sure if it's always right to restart the receiver
			   here but I don't know another way to prevent receiver hangs.
			   1999/12/25 SAW */
			outl(virt_to_bus(sp->rx_ringp[sp->cur_rx % RX_RING_SIZE]),
			   ioaddr + SCBPointer);
			outb(RxStart, ioaddr + SCBCmd);
			sp->rx_ring_state &= ~RrNoResources;
			spin_unlock(&sp->lock);
		}

		/* User interrupt, Command/Tx unit interrupt or CU not active. */
		if (status & 0xA400) {
			spin_lock(&sp->lock);
			speedo_tx_buffer_gc(dev);
			if (sp->tx_full
				&& (int)(sp->cur_tx - sp->dirty_tx) < TX_QUEUE_UNFULL) {
				/* The ring is no longer full. */
				sp->tx_full = 0;
				netif_wake_queue(dev); /* Attention: under a spinlock.  --SAW */
			}
			spin_unlock(&sp->lock);
		}

		if (--boguscnt < 0) {
			printk(KERN_ERR "%s: Too much work at interrupt, status=0x%4.4x.\n",
				   dev->name, status);
			/* Clear all interrupt sources. */
			/* Will change from 0xfc00 to 0xff00 when we start handling
			   FCP and ER interrupts --Dragan */
			outl(0xfc00, ioaddr + SCBStatus);
			break;
		}
	} while (1);

	if (speedo_debug > 3)
		printk(KERN_DEBUG "%s: exiting interrupt, status=%#4.4x.\n",
			   dev->name, inw(ioaddr + SCBStatus));

	dev->interrupt = 0;
	clear_bit(0, (void*)&sp->in_interrupt);
	return 0;
}

static int
speedo_rx(struct net_device *dev)
{
	struct speedo_private *sp = (struct speedo_private *)dev->priv;
	int entry = sp->cur_rx % RX_RING_SIZE;
	int status;
	int rx_work_limit = sp->dirty_rx + RX_RING_SIZE - sp->cur_rx;
	int alloc_ok = 1;

	if (speedo_debug > 4)
		printk(KERN_DEBUG " In speedo_rx().\n");

	/* If we own the next entry, it's a new packet. Send it up. */
	while (sp->rx_ringp[entry] != NULL &&
		   (status = le32_to_cpu(sp->rx_ringp[entry]->status)) & RxComplete) {
		int pkt_len = le32_to_cpu(sp->rx_ringp[entry]->count) & 0x3fff;
		
		if (--rx_work_limit < 0)
			break;

		/* Check for a rare out-of-memory case: the current buffer is
		   the last buffer allocated in the RX ring.  --SAW */
		if (sp->last_rxf == sp->rx_ringp[entry]) {
			/* Postpone the packet.  It'll be reaped at an interrupt when this
			   packet is no longer the last packet in the ring. */
			if (speedo_debug > 2)
				printk(KERN_DEBUG "%s: RX packet postponed!\n",
					   dev->name);
			sp->rx_ring_state |= RrPostponed;
			break;
		}

		if (speedo_debug > 4)
			printk(KERN_DEBUG "  speedo_rx() status %8.8x len %d.\n", status,
				   pkt_len);
		if ((status & (RxErrTooBig|RxOK|0x0f90)) != RxOK) {
			if (status & RxErrTooBig)
				printk(KERN_ERR "%s: Ethernet frame overran the Rx buffer, "
					   "status %8.8x!\n", dev->name, status);
			else if (! (status & RxOK)) {
				/* There was a fatal error.  This *should* be impossible. */
				//sp->stats.rx_errors++;
				printk(KERN_ERR "%s: Anomalous event in speedo_rx(), "
					   "status %8.8x.\n",
					   dev->name, status);
			}
		} else {
			PacketBuf_s *skb;

			/* Check if the packet is long enough to just accept without
			   copying to a properly sized skbuff. */
			if (pkt_len < rx_copybreak
				&& (skb = alloc_pkt_buffer(pkt_len + 2)) != 0) {
				//skb->dev = dev;
				skb->pb_nSize = 0; /* << This was missing! grmpf! */
				//skb_reserve(skb, 2);	/* Align IP on 16 byte boundaries */
				/* 'skb_put()' points to the start of sk_buff data area. */
				memcpy(skb_put(skb, pkt_len), sp->rx_skbuff[entry]->pb_pData,
					   pkt_len);
			} else {
				/* Pass up the already-filled skbuff. */
				skb = sp->rx_skbuff[entry];
				if (skb == NULL) {
					printk(KERN_ERR "%s: Inconsistent Rx descriptor chain.\n",
						   dev->name);
					break;
				}
				sp->rx_skbuff[entry] = NULL;
				skb_put(skb, pkt_len);
				sp->rx_ringp[entry] = NULL;
			}
			if ( dev->packet_queue != NULL ) {
			    skb->pb_uMacHdr.pRaw = skb->pb_pData;
			    enqueue_packet( dev->packet_queue, skb);
			} else {
			    printk( "Error: speedo_rx() received packet to downed device!\n" );
			    free_pkt_buffer( skb );
			}
			//sp->stats.rx_packets++;
			//sp->stats.rx_bytes += pkt_len;
		}
		entry = (++sp->cur_rx) % RX_RING_SIZE;
		sp->rx_ring_state &= ~RrPostponed;
		/* Refill the recently taken buffers.
		   Do it one-by-one to handle traffic bursts better. */
		if (alloc_ok && speedo_refill_rx_buf(dev, 0) == -1)
			alloc_ok = 0;
	}

	/* Try hard to refill the recently taken buffers. */
	speedo_refill_rx_buffers(dev, 1);

	sp->last_rx_time = jiffies;

	return 0;
}

/* Set or clear the multicast filter for this adaptor.
   This is very ugly with Intel chips -- we usually have to execute an
   entire configuration command, plus process a multicast command.
   This is complicated.  We must put a large configuration command and
   an arbitrarily-sized multicast command in the transmit list.
   To minimize the disruption -- the previous command might have already
   loaded the link -- we convert the current command block, normally a Tx
   command, into a no-op and link it to the new command.
*/
static void set_rx_mode(struct net_device *dev)
{
	struct speedo_private *sp = (struct speedo_private *)dev->priv;
	long ioaddr = dev->base_addr;
	struct descriptor *last_cmd;
	char new_rx_mode;
	unsigned long flags;
	int entry;
	//int i;

	if (dev->flags & IFF_PROMISC) {			/* Set promiscuous. */
		new_rx_mode = 3;
	} else if ((dev->flags & IFF_ALLMULTI)  ||
			   dev->mc_count > multicast_filter_limit) {
		new_rx_mode = 1;
	} else
		new_rx_mode = 0;

	if (speedo_debug > 3)
		printk(KERN_DEBUG "%s: set_rx_mode %d -> %d\n", dev->name,
				sp->rx_mode, new_rx_mode);

	if ((int)(sp->cur_tx - sp->dirty_tx) > TX_RING_SIZE - TX_MULTICAST_SIZE) {
	    /* The Tx ring is full -- don't add anything!  Hope the mode will be
		 * set again later. */
		sp->rx_mode = -1;
		return;
	}

	if (new_rx_mode != sp->rx_mode) {
		u8 *config_cmd_data;

		spin_lock_irqsave(&sp->lock, flags);
		entry = sp->cur_tx++ % TX_RING_SIZE;
		last_cmd = sp->last_cmd;
		sp->last_cmd = (struct descriptor *)&sp->tx_ring[entry];

		sp->tx_skbuff[entry] = 0;			/* Redundant. */
		sp->tx_ring[entry].status = cpu_to_le32(CmdSuspend | CmdConfigure);
		sp->tx_ring[entry].link =
			virt_to_le32desc(&sp->tx_ring[(entry + 1) % TX_RING_SIZE]);
		config_cmd_data = (void *)&sp->tx_ring[entry].tx_desc_addr;
		/* Construct a full CmdConfig frame. */
		memcpy(config_cmd_data, i82558_config_cmd, sizeof(i82558_config_cmd));
		config_cmd_data[1] = (txfifo << 4) | rxfifo;
		config_cmd_data[4] = rxdmacount;
		config_cmd_data[5] = txdmacount + 0x80;
		config_cmd_data[15] |= (new_rx_mode & 2) ? 1 : 0;
		/* 0x80 doesn't disable FC 0x84 does.
		   Disable Flow control since we are not ACK-ing any FC interrupts
		   for now. --Dragan */
		config_cmd_data[19] = 0x84;
		config_cmd_data[19] |= sp->full_duplex ? 0x40 : 0;
		config_cmd_data[21] = (new_rx_mode & 1) ? 0x0D : 0x05;
		if (sp->phy[0] & 0x8000) {			/* Use the AUI port instead. */
			config_cmd_data[15] |= 0x80;
			config_cmd_data[8] = 0;
		}
		/* Trigger the command unit resume. */
		wait_for_cmd_done(ioaddr + SCBCmd);
		clear_suspend(last_cmd);
		outb(CUResume, ioaddr + SCBCmd);
		if ((int)(sp->cur_tx - sp->dirty_tx) >= TX_QUEUE_LIMIT) {
			netif_stop_queue(dev);
			sp->tx_full = 1;
		}
		spin_unlock_irqrestore(&sp->lock, flags);
	}

#ifndef __ATHEOS__
	if (new_rx_mode == 0  &&  dev->mc_count < 4) {
		/* The simple case of 0-3 multicast list entries occurs often, and
		   fits within one tx_ring[] entry. */
		struct dev_mc_list *mclist;
		u16 *setup_params, *eaddrs;

		spin_lock_irqsave(&sp->lock, flags);
		entry = sp->cur_tx++ % TX_RING_SIZE;
		last_cmd = sp->last_cmd;
		sp->last_cmd = (struct descriptor *)&sp->tx_ring[entry];

		sp->tx_skbuff[entry] = 0;
		sp->tx_ring[entry].status = cpu_to_le32(CmdSuspend | CmdMulticastList);
		sp->tx_ring[entry].link =
			virt_to_le32desc(&sp->tx_ring[(entry + 1) % TX_RING_SIZE]);
		sp->tx_ring[entry].tx_desc_addr = 0; /* Really MC list count. */
		setup_params = (u16 *)&sp->tx_ring[entry].tx_desc_addr;
		*setup_params++ = cpu_to_le16(dev->mc_count*6);
		/* Fill in the multicast addresses. */
		for (i = 0, mclist = dev->mc_list; i < dev->mc_count;
			 i++, mclist = mclist->next) {
			eaddrs = (u16 *)mclist->dmi_addr;
			*setup_params++ = *eaddrs++;
			*setup_params++ = *eaddrs++;
			*setup_params++ = *eaddrs++;
		}

		wait_for_cmd_done(ioaddr + SCBCmd);
		clear_suspend(last_cmd);
		/* Immediately trigger the command unit resume. */
		outb(CUResume, ioaddr + SCBCmd);

		if ((int)(sp->cur_tx - sp->dirty_tx) >= TX_QUEUE_LIMIT) {
			netif_stop_queue(dev);
			sp->tx_full = 1;
		}
		spin_unlock_irqrestore(&sp->lock, flags);
	} else if (new_rx_mode == 0) {
		struct dev_mc_list *mclist;
		u16 *setup_params, *eaddrs;
		struct speedo_mc_block *mc_blk;
		struct descriptor *mc_setup_frm;
		int i;

		mc_blk = kmalloc(sizeof(*mc_blk) + 2 + multicast_filter_limit*6,
						 MEMF_KERNEL | MEMF_CLEAR);
		if (mc_blk == NULL) {
			printk(KERN_ERR "%s: Failed to allocate a setup frame.\n",
				   dev->name);
			sp->rx_mode = -1; /* We failed, try again. */
			return;
		}
		mc_blk->next = NULL;
		mc_setup_frm = &mc_blk->frame;

		/* Fill the setup frame. */
		if (speedo_debug > 1)
			printk(KERN_DEBUG "%s: Constructing a setup frame at %p.\n",
				   dev->name, mc_setup_frm);
		mc_setup_frm->cmd_status =
			cpu_to_le32(CmdSuspend | CmdIntr | CmdMulticastList);
		/* Link set below. */
		setup_params = (u16 *)&mc_setup_frm->params;
		*setup_params++ = cpu_to_le16(dev->mc_count*6);
		/* Fill in the multicast addresses. */
		for (i = 0, mclist = dev->mc_list; i < dev->mc_count;
			 i++, mclist = mclist->next) {
			eaddrs = (u16 *)mclist->dmi_addr;
			*setup_params++ = *eaddrs++;
			*setup_params++ = *eaddrs++;
			*setup_params++ = *eaddrs++;
		}

		/* Disable interrupts while playing with the Tx Cmd list. */
		spin_lock_irqsave(&sp->lock, flags);

		if (sp->mc_setup_tail)
			sp->mc_setup_tail->next = mc_blk;
		else
			sp->mc_setup_head = mc_blk;
		sp->mc_setup_tail = mc_blk;
		mc_blk->tx = sp->cur_tx;

		entry = sp->cur_tx++ % TX_RING_SIZE;
		last_cmd = sp->last_cmd;
		sp->last_cmd = mc_setup_frm;

		/* Change the command to a NoOp, pointing to the CmdMulti command. */
		sp->tx_skbuff[entry] = 0;
		sp->tx_ring[entry].status = cpu_to_le32(CmdNOp);
		sp->tx_ring[entry].link = virt_to_le32desc(mc_setup_frm);

		/* Set the link in the setup frame. */
		mc_setup_frm->link =
			virt_to_le32desc(&(sp->tx_ring[(entry+1) % TX_RING_SIZE]));

		wait_for_cmd_done(ioaddr + SCBCmd);
		clear_suspend(last_cmd);
		/* Immediately trigger the command unit resume. */
		outb(CUResume, ioaddr + SCBCmd);

		if ((int)(sp->cur_tx - sp->dirty_tx) >= TX_QUEUE_LIMIT) {
			netif_stop_queue(dev);
			sp->tx_full = 1;
		}
		spin_unlock_irqrestore(&sp->lock, flags);

		if (speedo_debug > 5)
			printk(" CmdMCSetup frame length %d in entry %d.\n",
				   dev->mc_count, entry);
	}

#endif
	sp->rx_mode = new_rx_mode;
}

static inline struct RxFD *speedo_rx_alloc(struct net_device *dev, int entry)
{
	struct speedo_private *sp = (struct speedo_private *)dev->priv;
	struct RxFD *rxf;
	PacketBuf_s *skb;
	/* Get a fresh skbuff to replace the consumed one. */
	skb = alloc_pkt_buffer(PKT_BUF_SZ + sizeof(struct RxFD));
	sp->rx_skbuff[entry] = skb;
	if (skb == NULL) {
		sp->rx_ringp[entry] = NULL;
		return NULL;
	}
	rxf = sp->rx_ringp[entry] = (struct RxFD *)skb->pb_pData;
	//skb->dev = dev;
	skb->pb_nSize = 0;
	//skb_reserve(skb, sizeof(struct RxFD));
	skb->pb_pData += sizeof(struct RxFD);
	rxf->rx_buf_addr = virt_to_bus(skb->pb_pData);
	return rxf;
}

static inline void speedo_rx_link(struct net_device *dev, int entry,
								  struct RxFD *rxf)
{
	struct speedo_private *sp = (struct speedo_private *)dev->priv;
	rxf->status = cpu_to_le32(0xC0000001); 	/* '1' for driver use only. */
	rxf->link = 0;			/* None yet. */
	rxf->count = cpu_to_le32(PKT_BUF_SZ << 16);
	sp->last_rxf->link = virt_to_le32desc(rxf);
	sp->last_rxf->status &= cpu_to_le32(~0xC0000000);
	sp->last_rxf = rxf;
}

static int speedo_refill_rx_buf(struct net_device *dev, int force)
{
	struct speedo_private *sp = (struct speedo_private *)dev->priv;
	int entry;
	struct RxFD *rxf;

	entry = sp->dirty_rx % RX_RING_SIZE;
	if (sp->rx_skbuff[entry] == NULL) {
		rxf = speedo_rx_alloc(dev, entry);
		if (rxf == NULL) {
			unsigned int forw;
			int forw_entry;
			if (speedo_debug > 2 || !(sp->rx_ring_state & RrOOMReported)) {
				printk(KERN_WARNING "%s: can't fill rx buffer (force %d)!\n",
						dev->name, force);
				// speedo_show_state(dev);
				sp->rx_ring_state |= RrOOMReported;
			}
			if (!force)
				return -1;	/* Better luck next time!  */
			/* Borrow an skb from one of next entries. */
			for (forw = sp->dirty_rx + 1; forw != sp->cur_rx; forw++)
				if (sp->rx_skbuff[forw % RX_RING_SIZE] != NULL)
					break;
			if (forw == sp->cur_rx)
				return -1;
			forw_entry = forw % RX_RING_SIZE;
			sp->rx_skbuff[entry] = sp->rx_skbuff[forw_entry];
			sp->rx_skbuff[forw_entry] = NULL;
			rxf = sp->rx_ringp[forw_entry];
			sp->rx_ringp[forw_entry] = NULL;
			sp->rx_ringp[entry] = rxf;
		}
	} else {
		rxf = sp->rx_ringp[entry];
	}
	speedo_rx_link(dev, entry, rxf);
	sp->dirty_rx++;
	sp->rx_ring_state &= ~(RrNoMem|RrOOMReported); /* Mark the progress. */
	return 0;
}

/* Media monitoring and control. */
static void speedo_timer(unsigned long data)
{
	struct net_device *dev = (struct net_device *)data;
	struct speedo_private *sp = (struct speedo_private *)dev->priv;
	long ioaddr = dev->base_addr;
	int phy_num = sp->phy[0] & 0x1f;

	/* We have MII and lost link beat. */
	if ((sp->phy[0] & 0x8000) == 0) {
		int partner = mdio_read(ioaddr, phy_num, 5);
		if (partner != sp->partner) {
			int flow_ctrl = sp->advertising & partner & 0x0400 ? 1 : 0;
			if (speedo_debug > 2) {
				printk(KERN_DEBUG "%s: Link status change.\n", dev->name);
				printk(KERN_DEBUG "%s: Old partner %x, new %x, adv %x.\n",
					   dev->name, sp->partner, partner, sp->advertising);
			}
			sp->partner = partner;
			if (flow_ctrl != sp->flow_ctrl) {
				sp->flow_ctrl = flow_ctrl;
				sp->rx_mode = -1;	/* Trigger a reload. */
			}
			/* Clear sticky bit. */
			mdio_read(ioaddr, phy_num, 1);
			/* If link beat has returned... */
			if (mdio_read(ioaddr, phy_num, 1) & 0x0004)
				dev->flags |= IFF_RUNNING;
			else
				dev->flags &= ~IFF_RUNNING;
		}
	}
	if (speedo_debug > 3) {
		printk(KERN_DEBUG "%s: Media control tick, status %4.4x.\n",
			   dev->name, inw(ioaddr + SCBStatus));
	}
	if (sp->rx_mode < 0  ||
		(sp->rx_bug  && jiffies - sp->last_rx_time > 2*HZ)) {
		/* We haven't received a packet in a Long Time.  We might have been
		   bitten by the receiver hang bug.  This can be cleared by sending
		   a set multicast list command. */
		if (speedo_debug > 2)
			printk(KERN_DEBUG "%s: Sending a multicast list set command"
				   " from a timer routine.\n", dev->name);
		set_rx_mode(dev);
	}
	/* We must continue to monitor the media. */
	//sp->timer.expires = RUN_AT(2*HZ); 			/* 2.0 sec. */
	//add_timer(&sp->timer);
	start_timer( sp->timer, (timer_callback *) &speedo_timer, dev, 2000000, true );
}

static void speedo_purge_tx(struct net_device *dev)
{
	struct speedo_private *sp = (struct speedo_private *)dev->priv;
	int entry;

	while ((int)(sp->cur_tx - sp->dirty_tx) > 0) {
		entry = sp->dirty_tx % TX_RING_SIZE;
		if (sp->tx_skbuff[entry]) {
			//sp->stats.tx_errors++;
			free_pkt_buffer(sp->tx_skbuff[entry]);
			sp->tx_skbuff[entry] = 0;
		}
		sp->dirty_tx++;
	}
#ifndef __ATHEOS__
	while (sp->mc_setup_head != NULL) {
		struct speedo_mc_block *t;
		if (speedo_debug > 1)
			printk(KERN_DEBUG "%s: freeing mc frame.\n", dev->name);
		t = sp->mc_setup_head->next;
		kfree(sp->mc_setup_head);
		sp->mc_setup_head = t;
	}
	sp->mc_setup_tail = NULL;
#endif
	sp->tx_full = 0;
	netif_wake_queue(dev);
}

static void reset_mii(struct net_device *dev)
{
	struct speedo_private *sp = (struct speedo_private *)dev->priv;
	long ioaddr = dev->base_addr;
	/* Reset the MII transceiver, suggested by Fred Young @ scalable.com. */
	if ((sp->phy[0] & 0x8000) == 0) {
		int phy_addr = sp->phy[0] & 0x1f;
		int advertising = mdio_read(ioaddr, phy_addr, 4);
		int mii_bmcr = mdio_read(ioaddr, phy_addr, 0);
		mdio_write(ioaddr, phy_addr, 0, 0x0400);
		mdio_write(ioaddr, phy_addr, 1, 0x0000);
		mdio_write(ioaddr, phy_addr, 4, 0x0000);
		mdio_write(ioaddr, phy_addr, 0, 0x8000);
#ifdef honor_default_port
		mdio_write(ioaddr, phy_addr, 0, mii_ctrl[dev->default_port & 7]);
#else
		mdio_read(ioaddr, phy_addr, 0);
		mdio_write(ioaddr, phy_addr, 0, mii_bmcr);
		mdio_write(ioaddr, phy_addr, 4, advertising);
#endif
	}
}

static void speedo_tx_timeout(struct net_device *dev)
{
	struct speedo_private *sp = (struct speedo_private *)dev->priv;
	long ioaddr = dev->base_addr;
	int status = inw(ioaddr + SCBStatus);
	unsigned long flags;

	printk(KERN_WARNING "%s: Transmit timed out: status %4.4x "
		   " %4.4x at %d/%d command %8.8x.\n",
		   dev->name, status, inw(ioaddr + SCBCmd),
		   sp->dirty_tx, sp->cur_tx,
		   (unsigned int) sp->tx_ring[sp->dirty_tx % TX_RING_SIZE].status);

	/* Trigger a stats dump to give time before the reset. */
	//speedo_get_stats(dev);

	//speedo_show_state(dev);
#if 0
	if ((status & 0x00C0) != 0x0080
		&&  (status & 0x003C) == 0x0010) {
		/* Only the command unit has stopped. */
		printk(KERN_WARNING "%s: Trying to restart the transmitter...\n",
			   dev->name);
		outl(virt_to_bus(&sp->tx_ring[sp->dirty_tx % TX_RING_SIZE]),
			 ioaddr + SCBPointer);
		outw(CUStart, ioaddr + SCBCmd);
		reset_mii(dev);
	} else {
#else
	{
#endif
		//start_bh_atomic();
		/* Ensure that timer routine doesn't run! */
		delete_timer(sp->timer);
		sp->timer = create_timer();
		//end_bh_atomic();
		/* Reset the Tx and Rx units. */
		outl(PortReset, ioaddr + SCBPort);
		/* We may get spurious interrupts here.  But I don't think that they
		   may do much harm.  1999/12/09 SAW */
		udelay(10);
		/* Disable interrupts. */
		outw(SCBMaskAll, ioaddr + SCBCmd);
		//synchronize_irq();
		speedo_tx_buffer_gc(dev);
		/* Free as much as possible.
		   It helps to recover from a hang because of out-of-memory.
		   It also simplifies speedo_resume() in case TX ring is full or
		   close-to-be full. */
		speedo_purge_tx(dev);
		speedo_refill_rx_buffers(dev, 1);
		spin_lock_irqsave(&sp->lock, flags);
		speedo_resume(dev);
		sp->rx_mode = -1;
		dev->trans_start = jiffies;
		spin_unlock_irqrestore(&sp->lock, flags);
		set_rx_mode(dev); /* it takes the spinlock itself --SAW */
		/* Reset MII transceiver.  Do it before starting the timer to serialize
		   mdio_xxx operations.  Yes, it's a paranoia :-)  2000/05/09 SAW */
		reset_mii(dev);
		start_timer( sp->timer, (timer_callback *) &speedo_timer, dev, 2400000, true );
	}
	return;
}

static int
speedo_start_xmit(PacketBuf_s *skb, struct net_device *dev)
{
	struct speedo_private *sp = (struct speedo_private *)dev->priv;
	long ioaddr = dev->base_addr;
	int entry;

	if (test_bit(0, (void*)&dev->tbusy) != 0) {
		int tickssofar = jiffies - dev->trans_start;
		if (tickssofar < TX_TIMEOUT - 2)
			return 1;
		if (tickssofar < TX_TIMEOUT) {
			/* Reap sent packets from the full Tx queue. */
			unsigned long flags;
			/* Take a spinlock to make wait_for_cmd_done and sending the
			command atomic.  --SAW */
			spin_lock_irqsave(&sp->lock, flags);
			wait_for_cmd_done(ioaddr + SCBCmd);
			outw(SCBTriggerIntr, ioaddr + SCBCmd);
			spin_unlock_irqrestore(&sp->lock, flags);
			return 1;
		}
		speedo_tx_timeout(dev);
		return 1;
	}

	{	/* Prevent interrupts from changing the Tx ring from underneath us. */
		unsigned long flags;

		spin_lock_irqsave(&sp->lock, flags);

		/* Check if there are enough space. */
		if ((int)(sp->cur_tx - sp->dirty_tx) >= TX_QUEUE_LIMIT) {
			printk(KERN_ERR "%s: incorrect tbusy state, fixed.\n", dev->name);
			netif_stop_queue(dev);
			sp->tx_full = 1;
			spin_unlock_irqrestore(&sp->lock, flags);
			return 1;
		}

		/* Calculate the Tx descriptor entry. */
		entry = sp->cur_tx++ % TX_RING_SIZE;

		sp->tx_skbuff[entry] = skb;
		sp->tx_ring[entry].status =
			cpu_to_le32(CmdSuspend | CmdTx | CmdTxFlex);
		if (!(entry & ((TX_RING_SIZE>>2)-1)))
			sp->tx_ring[entry].status |= cpu_to_le32(CmdIntr);
		sp->tx_ring[entry].link =
			virt_to_le32desc(&sp->tx_ring[sp->cur_tx % TX_RING_SIZE]);
		sp->tx_ring[entry].tx_desc_addr =
			virt_to_le32desc(&sp->tx_ring[entry].tx_buf_addr0);
		/* The data region is always in one buffer descriptor. */
		sp->tx_ring[entry].count = cpu_to_le32(sp->tx_threshold);
		sp->tx_ring[entry].tx_buf_addr0 = virt_to_le32desc(skb->pb_pData);
		sp->tx_ring[entry].tx_buf_size0 = cpu_to_le32(skb->pb_nSize);
		/* Trigger the command unit resume. */
		wait_for_cmd_done(ioaddr + SCBCmd);
		clear_suspend(sp->last_cmd);
		/* We want the time window between clearing suspend flag on the previous
		   command and resuming CU to be as small as possible.
		   Interrupts in between are very undesired.  --SAW */
		outb(CUResume, ioaddr + SCBCmd);
		sp->last_cmd = (struct descriptor *)&sp->tx_ring[entry];

		/* Leave room for set_rx_mode(). If there is no more space than reserved
		   for multicast filter mark the ring as full. */
		if ((int)(sp->cur_tx - sp->dirty_tx) >= TX_QUEUE_LIMIT) {
			netif_stop_queue(dev);
			sp->tx_full = 1;
		}

		spin_unlock_irqrestore(&sp->lock, flags);
	}

	dev->trans_start = jiffies;

	return 0;
}

static int
speedo_open(struct net_device *dev)
{
	struct speedo_private *sp = (struct speedo_private *)dev->priv;
	long ioaddr = dev->base_addr;

	if (speedo_debug > 1)
		printk(KERN_DEBUG "%s: speedo_open() irq %d.\n", dev->name, dev->irq);

	/* Set up the Tx queue early.. */
	sp->cur_tx = 0;
	sp->dirty_tx = 0;
	sp->last_cmd = 0;
	sp->tx_full = 0;
	spinlock_init(&sp->lock, "eepro100_lock");
	sp->in_interrupt = 0;

	/* .. we can safely take handler calls during init. */
	if ((dev->irq_handle = request_irq(dev->irq, speedo_interrupt, NULL,
			SA_SHIRQ, dev->name, dev)) < 0) {
		return -EAGAIN;
	}

	dev->if_port = sp->default_port;

#ifdef oh_no_you_dont_unless_you_honour_the_options_passed_in_to_us
	/* Retrigger negotiation to reset previous errors. */
	if ((sp->phy[0] & 0x8000) == 0) {
		int phy_addr = sp->phy[0] & 0x1f ;
		/* Use 0x3300 for restarting NWay, other values to force xcvr:
		   0x0000 10-HD
		   0x0100 10-FD
		   0x2000 100-HD
		   0x2100 100-FD
		*/
#ifdef honor_default_port
		mdio_write(ioaddr, phy_addr, 0, mii_ctrl[dev->default_port & 7]);
#else
		mdio_write(ioaddr, phy_addr, 0, 0x3300);
#endif
	}
#endif

	speedo_init_rx_ring(dev);

	/* Fire up the hardware. */
	outw(SCBMaskAll, ioaddr + SCBCmd);
	speedo_resume(dev);

	dev->interrupt = 0;
	dev->start = 1;
	netif_start_queue(dev);

	/* Setup the chip and configure the multicast list. */
	sp->mc_setup_head = NULL;
	sp->mc_setup_tail = NULL;
	sp->flow_ctrl = sp->partner = 0;
	sp->rx_mode = -1;			/* Invalid -> always reset the mode. */
	set_rx_mode(dev);
	if ((sp->phy[0] & 0x8000) == 0)
		sp->advertising = mdio_read(ioaddr, sp->phy[0] & 0x1f, 4);

	if (speedo_debug > 2) {
		printk(KERN_DEBUG "%s: Done speedo_open(), status %8.8x.\n",
			   dev->name, inw(ioaddr + SCBStatus));
	}

	/* Set the timer.  The timer serves a dual purpose:
	   1) to monitor the media interface (e.g. link beat) and perhaps switch
	   to an alternate media type
	   2) to monitor Rx activity, and restart the Rx process if the receiver
	   hangs. */
	sp->timer = create_timer();
	//sp->timer.expires = RUN_AT((24*HZ)/10); 			/* 2.4 sec. */
	//sp->timer.data = (unsigned long)dev;
	//sp->timer.function = &speedo_timer;					/* timer handler */
	//add_timer(&sp->timer);
	start_timer( sp->timer, (timer_callback *) &speedo_timer, dev, 2400000, true );

	/* No need to wait for the command unit to accept here. */
	if ((sp->phy[0] & 0x8000) == 0)
		mdio_read(ioaddr, sp->phy[0] & 0x1f, 0);

	return 0;
}

static int
speedo_close(struct net_device *dev)
{
	long ioaddr = dev->base_addr;
	struct speedo_private *sp = (struct speedo_private *)dev->priv;
	int i;

	dev->start = 0;
	netif_stop_queue(dev);

	if (speedo_debug > 1)
		printk(KERN_DEBUG "%s: Shutting down ethercard, status was %4.4x.\n",
			   dev->name, inw(ioaddr + SCBStatus));

	/* Shut off the media monitoring timer. */
	//start_bh_atomic();
	delete_timer(sp->timer);
	sp->timer = create_timer();
	//end_bh_atomic();

	/* Shutting down the chip nicely fails to disable flow control. So.. */
	outl(PortPartialReset, ioaddr + SCBPort);

	release_irq(dev->irq, 0);

	/* Print a few items for debugging. */
	//if (speedo_debug > 3)
		//speedo_show_state(dev);

    /* Free all the skbuffs in the Rx and Tx queues. */
	for (i = 0; i < RX_RING_SIZE; i++) {
		PacketBuf_s *skb = sp->rx_skbuff[i];
		sp->rx_skbuff[i] = 0;
		/* Clear the Rx descriptors. */
		if (skb)
			free_pkt_buffer(skb);
	}

	for (i = 0; i < TX_RING_SIZE; i++) {
		PacketBuf_s *skb = sp->tx_skbuff[i];
		sp->tx_skbuff[i] = 0;
		/* Clear the Tx descriptors. */
		if (skb)
			free_pkt_buffer(skb);
	}

#ifndef __ATHEOS__
	/* Free multicast setting blocks. */
	for (i = 0; sp->mc_setup_head != NULL; i++) {
		struct speedo_mc_block *t;
		t = sp->mc_setup_head->next;
		kfree(sp->mc_setup_head);
		sp->mc_setup_head = t;
	}
	sp->mc_setup_tail = NULL;
	if (speedo_debug > 0)
		printk(KERN_DEBUG "%s: %d multicast blocks dropped.\n", dev->name, i);
#endif

	return 0;
}

/* ------------------- */

static int device_eepro100_read( void* pNode, void* pCookie, off_t nPosition, void* pBuffer, size_t nSize )
{
    return -ENOSYS;
}

static int device_eepro100_write( void* pNode, void* pCookie, off_t nPosition, const void* pBuffer, size_t nSize )
{
    PacketBuf_s *psBuffer;
    
    psBuffer = alloc_pkt_buffer( nSize );
    if (!psBuffer)
        return -ENOMEM;

    memcpy( psBuffer->pb_pData, pBuffer, nSize );
    speedo_start_xmit( psBuffer, (struct device *) pNode);

    return( nSize );
}

static status_t device_eepro100_ioctl( void* pNode, void* pCookie,
                        uint32 nCommand, void* pArgs, bool bFromKernel )
{
    struct device* dev = pNode;
    int nError = 0;
    
    switch( nCommand ) 
    {
    case SIOC_ETH_START:
		if (dev->packet_queue) {
			return -ENOSYS;  // FIXME: probably not correct value
		}
        dev->packet_queue = pArgs;
		if (!dev->packet_queue) {
			return -ENOSYS;  // FIXME: probably not correct value
		}
        nError = speedo_open( dev );
        if (nError != 0) {
            printk(KERN_ERR "eepro100: speedo_open() failed\n");
            dev->packet_queue = NULL;
        }
        break;

    case SIOC_ETH_STOP:
        speedo_close( dev );
        dev->packet_queue = NULL;
        break;
        
    case SIOCSIFHWADDR:
        nError = -ENOSYS;
        break;

    case SIOCGIFHWADDR:
        memcpy( ((struct ifreq*)pArgs)->ifr_hwaddr.sa_data, dev->dev_addr, 6 );
        break;
        
    default:
        nError = -ENOSYS;
        break;
    }
    
    return( nError );
}

static status_t device_eepro100_open( void* pNode, uint32 nFlags, void **pCookie )
{
	return 0;
}

static status_t device_eepro100_close( void* pNode, void* pCookie )
{
    return 0;
}

int device_init( int nDeviceID )
{
	int cards_found = 0;
	int chip_idx;
	PCI_Info_s *pdev;
	PCI_bus_s *psBus = get_busmanager( PCI_BUS_NAME, PCI_BUS_VERSION );
	if( psBus == NULL)
		return -ENODEV;

	printk(version);

	for (chip_idx = 0; pci_tbl[chip_idx].name; chip_idx++) {
		long ioaddr;
		int irq;
		unsigned char pci_latency;
		u16 pci_command, new_command;

		if ((pdev = pci_find_device(pci_tbl[chip_idx].vendor_id,
					 			pci_tbl[chip_idx].device_id,
					 			pdev)) != NULL) {

			/* Remove I/O space marker in bit 0. */
			ioaddr = pdev->u.h0.nBase1 & PCI_ADDRESS_IO_MASK;
			irq = pdev->u.h0.nInterruptLine;

			if (speedo_debug > 2)
				printk(KERN_INFO "eepro100: Intel i82557 PCI Speedo found at I/O %#lx, IRQ %d.\n",
					   ioaddr, irq);
			
			if( claim_device( nDeviceID, pdev->nHandle, "Intel i82557 PCI Speedo", DEVICE_NET ) != 0 )
				break;

			/* Get and check the bus-master and latency values. */
        	pci_command = psBus->read_pci_config( pdev->nBus, pdev->nDevice,
                                        	pdev->nFunction, PCI_COMMAND, 2 );
        	new_command = pci_command | PCI_COMMAND_MASTER | PCI_COMMAND_IO;
        	if (pci_command != new_command)
        	{
                printk(KERN_INFO "eepro100: The PCI BIOS hasn't enabled the device "
               	    "at %d/%d/%d!    Updating PCI command %4.4x->%4.4x.\n",
                   	pdev->nBus, pdev->nDevice, pdev->nFunction,
                   	pci_command, new_command);
				psBus->write_pci_config( pdev->nBus, pdev->nDevice, pdev->nFunction,
					PCI_COMMAND, 2, new_command);
        	}

        	pci_latency = psBus->read_pci_config( pdev->nBus, pdev->nDevice,
                                        	pdev->nFunction, PCI_LATENCY, 1 );
	       	if (pci_latency < 32)
        	{
				printk("eepro100: PCI latency timer (CFLT) is unreasonably low at %d."
					   "  Setting to 32 clocks.\n", pci_latency);
				psBus->write_pci_config( pdev->nBus, pdev->nDevice, pdev->nFunction,
					PCI_LATENCY, 1, 32);
			} else if (speedo_debug > 2)
				printk("  PCI latency timer (CFLT) is %#x.\n", pci_latency);
			
			if (speedo_found1(pdev, ioaddr, chip_idx, cards_found, nDeviceID))
				cards_found++;
	    }
	}
	if(cards_found == 0)
	{
		printk("eepro100: No Intel Speedo cards found.");
		disable_device( nDeviceID );
		return -ENODEV;
	}
	return cards_found;
}

int device_uninit( int device_handle )
{
	return 0;
}

/* AtheOS device section */
static DeviceOperations_s g_sDevOps =
{
    device_eepro100_open,
    device_eepro100_close,
    device_eepro100_ioctl,
    device_eepro100_read,
    device_eepro100_write,
    NULL,    // dop_readv 
    NULL,    // dop_writev 
    NULL,    // dop_add_select_req 
    NULL     // dop_rem_select_req 
};
