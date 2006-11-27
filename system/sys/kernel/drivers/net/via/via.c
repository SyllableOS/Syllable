/* via.c: A Syllable Ethernet device driver for VIA Rhine family chips. */
/*
	Written 1998-2003 by Donald Becker.
	Ported 2003 to Syllable by Arno Klenke

	This software may be used and distributed according to the terms of
	the GNU General Public License (GPL), incorporated herein by reference.
	Drivers based on or derived from this code fall under the GPL and must
	retain the authorship, copyright and license notice.  This file is not
	a complete program and may only be used when the entire operating
	system is licensed under the GPL.

	This driver is designed for the VIA VT86c100A Rhine-II PCI Fast Ethernet
	controller.  It also works with the older 3043 Rhine-I chip.

	The author may be reached as becker@scyld.com, or C/O
	Scyld Computing Corporation
	410 Severn Ave., Suite 210
	Annapolis MD 21403


	This driver contains some changes from the original Donald Becker
	version. He may or may not be interested in bug reports on this
	code. You can find his versions at:
	http://www.scyld.com/network/via-rhine.html


	
*/

#define DRV_NAME	"VIA"
#define DRV_VERSION	"1.1.13"
#define DRV_RELDATE	"Nov-17-2001"


/* A few user-configurable values.
   These may be modified when a driver module is loaded. */

static int debug = 0;			/* 1 normal messages, 0 quiet .. 7 verbose. */
static int max_interrupt_work = 20;

/* Set the copy breakpoint for the copy-only-tiny-frames scheme.
   Setting to > 1518 effectively disables this feature. */
static int rx_copybreak = 200;

/* Used to pass the media type, etc.
   Both 'options[]' and 'full_duplex[]' should exist for driver
   interoperability.
   The media type is usually passed in 'options[]'.
   The default is autonegotation for speed and duplex.
     This should rarely be overridden.
   Use option values 0x10/0x20 for 10Mbps, 0x100,0x200 for 100Mbps.
   Use option values 0x10 and 0x100 for forcing half duplex fixed speed.
   Use option values 0x20 and 0x200 for forcing full duplex operation.
*/
#define MAX_UNITS 8		/* More are supported, limit only on options */
static int options[MAX_UNITS] = {-1, -1, -1, -1, -1, -1, -1, -1};
static int full_duplex[MAX_UNITS] = {-1, -1, -1, -1, -1, -1, -1, -1};

/* Maximum number of multicast addresses to filter (vs. rx-all-multicast).
   The Rhine has a 64 element 8390-like hash table.  */
static const int multicast_filter_limit = 32;


/* Operational parameters that are set at compile time. */

/* Keep the ring sizes a power of two for compile efficiency.
   The compiler will convert <unsigned>'%'<2^N> into a bit mask.
   Making the Tx ring too large decreases the effectiveness of channel
   bonding and packet priority.
   There are no ill effects from too-large receive rings. */
#define TX_RING_SIZE	16
#define TX_QUEUE_LEN	10		/* Limit ring entries actually used.  */
#define RX_RING_SIZE	32


/* Operational parameters that usually are not changed. */

/* Time in jiffies before concluding the transmitter is hung. */
#define TX_TIMEOUT  (6*1000)

#define PKT_BUF_SZ		1536			/* Size of each temporary Rx buffer.*/

/* max time out delay time */
#define W_MAX_TIMEOUT	0x0FFFU

#if !defined(__OPTIMIZE__)  ||  !defined(__KERNEL__)
#warning  You must compile this file with the correct options!
#warning  See the last lines of the source file.
#error  You must compile this driver with "-O".
#endif

#include <atheos/kernel.h>
#include <atheos/irq.h>
#include <atheos/isa_io.h>
#include <atheos/time.h>
#include <atheos/pci.h>
#include <atheos/semaphore.h>
#include <atheos/spinlock.h>
#include <atheos/ctype.h>
#include <atheos/device.h>
#include <atheos/udelay.h>
#include <atheos/bitops.h>

#include <posix/unistd.h>
#include <posix/errno.h>
#include <posix/signal.h>
#include <net/net.h>
#include <net/ip.h>
#include <net/sockios.h>


#define KERN_ERR "Error: "
#define KERN_DEBUG ""
#define KERN_WARNING ""
#define KERN_INFO ""
#define KERN_NOTICE ""

/* These identify the driver base version and may not be removed. */
static char version[] =
KERN_INFO DRV_NAME ": v1.16-LK" DRV_VERSION "  " DRV_RELDATE "  Written by Donald Becker\n"
KERN_INFO "  http://www.scyld.com/network/via-rhine.html\n";

static char shortname[] = DRV_NAME;

//#define CONFIG_VIA_RHINE_MMIO
#if 0
#define __io_virt(x) ((void *)(x))
#define readb(addr) (*(volatile unsigned char *) __io_virt(addr))
#define readw(addr) (*(volatile unsigned short *) __io_virt(addr))
#define readl(addr) (*(volatile unsigned int *) __io_virt(addr))
#define __raw_readb readb
#define __raw_readw readw
#define __raw_readl readl

#define writeb(b,addr) (*(volatile unsigned char *) __io_virt(addr) = (b))
#define writew(b,addr) (*(volatile unsigned short *) __io_virt(addr) = (b))
#define writel(b,addr) (*(volatile unsigned int *) __io_virt(addr) = (b))
#define __raw_writeb writeb
#define __raw_writew writew
#define __raw_writel writel
#endif


/* This driver was written to use PCI memory space, however most versions
   of the Rhine only work correctly with I/O space accesses. */
#ifdef CONFIG_VIA_RHINE_MMIO
#define USE_MEM
#else
#define USE_IO
#undef readb
#undef readw
#undef readl
#undef writeb
#undef writew
#undef writel
#define readb inb
#define readw inw
#define readl inl
#define writeb outb
#define writew outw
#define writel outl
#endif

/*
				Theory of Operation

I. Board Compatibility

This driver is designed for the VIA 86c100A Rhine-II PCI Fast Ethernet
controller.

II. Board-specific settings

Boards with this chip are functional only in a bus-master PCI slot.

Many operational settings are loaded from the EEPROM to the Config word at
offset 0x78.  This driver assumes that they are correct.
If this driver is compiled to use PCI memory space operations the EEPROM
must be configured to enable memory ops.

III. Driver operation

IIIa. Ring buffers

This driver uses two statically allocated fixed-size descriptor lists
formed into rings by a branch from the final descriptor to the beginning of
the list.  The ring sizes are set at compile time by RX/TX_RING_SIZE.

IIIb/c. Transmit/Receive Structure

This driver attempts to use a zero-copy receive and transmit scheme.

Alas, all data buffers are required to start on a 32 bit boundary, so
the driver must often copy transmit packets into bounce buffers.

The driver allocates full frame size skbuffs for the Rx ring buffers at
open() time and passes the skb->data field to the chip as receive data
buffers.  When an incoming frame is less than RX_COPYBREAK bytes long,
a fresh skbuff is allocated and the frame is copied to the new skbuff.
When the incoming frame is larger, the skbuff is passed directly up the
protocol stack.  Buffers consumed this way are replaced by newly allocated
skbuffs in the last phase of via_rhine_rx().

The RX_COPYBREAK value is chosen to trade-off the memory wasted by
using a full-sized skbuff for small frames vs. the copying costs of larger
frames.  New boards are typically used in generously configured machines
and the underfilled buffers have negligible impact compared to the benefit of
a single allocation size, so the default value of zero results in never
copying packets.  When copying is done, the cost is usually mitigated by using
a combined copy/checksum routine.  Copying also preloads the cache, which is
most useful with small frames.

Since the VIA chips are only able to transfer data to buffers on 32 bit
boundaries, the IP header at offset 14 in an ethernet frame isn't
longword aligned for further processing.  Copying these unaligned buffers
has the beneficial effect of 16-byte aligning the IP header.

IIId. Synchronization

The driver runs as two independent, single-threaded flows of control.  One
is the send-packet routine, which enforces single-threaded use by the
dev->priv->lock spinlock. The other thread is the interrupt handler, which 
is single threaded by the hardware and interrupt handling software.

The send packet thread has partial control over the Tx ring. It locks the 
dev->priv->lock whenever it's queuing a Tx packet. If the next slot in the ring
is not available it stops the transmit queue by calling netif_stop_queue.

The interrupt handler has exclusive control over the Rx ring and records stats
from the Tx ring.  After reaping the stats, it marks the Tx queue entry as
empty by incrementing the dirty_tx mark. If at least half of the entries in
the Rx ring are available the transmit queue is woken up if it was stopped.

IV. Notes

IVb. References

This driver was originally written using a preliminary VT86C100A manual
from
  http://www.via.com.tw/ 
The usual background material was used:
  http://www.scyld.com/expert/100mbps.html
  http://scyld.com/expert/NWay.html

Additional information is now available, especially for the newer chips.
   http://www.via.com.tw/en/Networking/DS6105LOM100.pdf

IVc. Errata

The VT86C100A manual is not reliable information.
The 3043 chip does not handle unaligned transmit or receive buffers,
resulting in significant performance degradation for bounce buffer
copies on transmit and unaligned IP headers on receive.
The chip does not pad to minimum transmit length.

There is a bug with the transmit descriptor pointer handling when the
chip encounters a transmit error.

*/


/* This table drives the PCI probe routines.  It's mostly boilerplate in all
   of the drivers, and will likely be provided by some future kernel.
   Note the matching code -- the first table entry matchs all 56** cards but
   second only the 1234 card.
*/

enum pci_flags_bit {
	PCI_USES_IO=1, PCI_USES_MEM=2, PCI_USES_MASTER=4,
	PCI_ADDR0=0x10<<0, PCI_ADDR1=0x10<<1, PCI_ADDR2=0x10<<2, PCI_ADDR3=0x10<<3,
};

struct via_rhine_chip_info {
	const char *name;
	uint16 pci_flags;
	int io_size;
	int drv_flags;
};


enum chip_capability_flags {
	CanHaveMII=1, HasESIPhy=2, HasDavicomPhy=4, HasV1TxStat=8,
	ReqTxAlign=0x10, HasWOL=0x20, HasIPChecksum=0x40, HasVLAN=0x80 };

#ifdef USE_MEM
#define RHINE_IOTYPE (PCI_USES_MEM | PCI_USES_MASTER | PCI_ADDR1)
#else
#define RHINE_IOTYPE (PCI_USES_IO  | PCI_USES_MASTER | PCI_ADDR0)
#endif


enum via_rhine_chips {
	VT3043 = 0,
	VT86C100A,
	VT6102,
	VT6105LOM,
	VT6105M
};

/* directly indexed by enum via_rhine_chips, above */
static struct via_rhine_chip_info via_rhine_chip_info[] =
{
	{ "VIA VT3043 Rhine",    RHINE_IOTYPE, 128,
	  CanHaveMII | ReqTxAlign | HasV1TxStat },
	{ "VIA VT86C100A Rhine", RHINE_IOTYPE, 128,
	  CanHaveMII | ReqTxAlign | HasV1TxStat },
	{ "VIA VT6102 Rhine-II", RHINE_IOTYPE, 256,
	  CanHaveMII | HasWOL },
	{ "VIA VT6105LOM Rhine-III", RHINE_IOTYPE, 256,
	  CanHaveMII | HasWOL },
	{ "VIA VT6105M Rhine-III", RHINE_IOTYPE, 256,
	 CanHaveMII | HasWOL }
};

/* Offsets to the device registers. */
enum register_offsets {
	StationAddr=0x00, RxConfig=0x06, TxConfig=0x07, ChipCmd=0x08,
	IntrStatus=0x0C, IntrEnable=0x0E,
	MulticastFilter0=0x10, MulticastFilter1=0x14,
	RxRingPtr=0x18, TxRingPtr=0x1C, GFIFOTest=0x54,
	MIIPhyAddr=0x6C, MIIStatus=0x6D, PCIBusConfig=0x6E,
	MIICmd=0x70, MIIRegAddr=0x71, MIIData=0x72, MACRegEEcsr=0x74,
	ConfigA=0x78, ConfigB=0x79, ConfigC=0x7A, ConfigD=0x7B,
	RxMissed=0x7C, RxCRCErrs=0x7E,
	StickyHW=0x83, WOLcrClr=0xA4, WOLcgClr=0xA7, 
	PwrcsrSet=0xA8, PwrcsrClr=0xAC,
};

#ifdef USE_MEM
/* Registers we check that mmio and reg are the same. */
int mmio_verify_registers[] = {
	RxConfig, TxConfig, IntrEnable, ConfigA, ConfigB, ConfigC, ConfigD,
	0
};
#endif

/* Bits in the interrupt status/mask registers. */
enum intr_status_bits {
	IntrRxDone=0x0001, IntrRxErr=0x0004, IntrRxEmpty=0x0020,
	IntrTxDone=0x0002, IntrTxAbort=0x0008, IntrTxUnderrun=0x0010,
	IntrPCIErr=0x0040,
	IntrStatsMax=0x0080, IntrRxEarly=0x0100, IntrMIIChange=0x0200,
	IntrRxOverflow=0x0400, IntrRxDropped=0x0800, IntrRxNoBuf=0x1000,
	IntrTxAborted=0x2000, IntrLinkChange=0x4000,
	IntrRxWakeUp=0x8000,
	IntrNormalSummary=0x0003, IntrAbnormalSummary=0xC260,
};

/* Bits in WOLcrSet/WOLcrClr and PwrcsrSet/PwrcsrClr */
enum wol_bits {
	WOLucast	= 0x10,
	WOLmagic	= 0x20,
	WOLbmcast	= 0x30,
	WOLlnkon	= 0x40,
	WOLlnkoff	= 0x80,
};

/* MII interface, status flags.
   Not to be confused with the MIIStatus register ... */
enum mii_status_bits {
	MIICap100T4			= 0x8000,
	MIICap10100HdFd		= 0x7800,
	MIIPreambleSupr		= 0x0040,
	MIIAutoNegCompleted	= 0x0020,
	MIIRemoteFault		= 0x0010,
	MIICapAutoNeg		= 0x0008,
	MIILink				= 0x0004,
	MIIJabber			= 0x0002,
	MIIExtended			= 0x0001
};

/* The Rx and Tx buffer descriptors. */
struct rx_desc {
	int32 rx_status;
	uint32 desc_length;
	uint32 addr;
	uint32 next_desc;
};
struct tx_desc {
	int32 tx_status;
	uint32 desc_length;
	uint32 addr;
	uint32 next_desc;
};

/* Bits in *_desc.status */
enum rx_status_bits {
	RxOK=0x8000, RxWholePkt=0x0300, RxErr=0x008F
};

enum desc_status_bits {
	DescOwn=0x80000000, DescEndPacket=0x4000, DescIntr=0x1000,
};

/* Bits in ChipCmd. */
enum chip_cmd_bits {
	CmdInit=0x0001, CmdStart=0x0002, CmdStop=0x0004, CmdRxOn=0x0008,
	CmdTxOn=0x0010, CmdTxDemand=0x0020, CmdRxDemand=0x0040,
	CmdEarlyRx=0x0100, CmdEarlyTx=0x0200, CmdFDuplex=0x0400,
	CmdNoTxPoll=0x0800, CmdReset=0x8000,
};


struct /*enet_statistics*/  net_device_stats
{
	unsigned long	rx_packets;		/* total packets received	*/
	unsigned long	tx_packets;		/* total packets transmitted	*/
	unsigned long	rx_bytes;		/* total bytes received 	*/
	unsigned long	tx_bytes;		/* total bytes transmitted	*/
	unsigned long	rx_errors;		/* bad packets received		*/
	unsigned long	tx_errors;		/* packet transmit problems	*/
	unsigned long	rx_dropped;		/* no space in linux buffers	*/
	unsigned long	tx_dropped;		/* no space available in linux	*/
	unsigned long	multicast;		/* multicast packets received	*/
	unsigned long	collisions;

	/* detailed rx_errors: */
	unsigned long	rx_length_errors;
	unsigned long	rx_over_errors;		/* receiver ring buff overflow	*/
	unsigned long	rx_crc_errors;		/* recved pkt with crc error	*/
	unsigned long	rx_frame_errors;	/* recv'd frame alignment error */
	unsigned long	rx_fifo_errors;		/* recv'r fifo overrun		*/
	unsigned long	rx_missed_errors;	/* receiver missed packet	*/

	/* detailed tx_errors */
	unsigned long	tx_aborted_errors;
	unsigned long	tx_carrier_errors;
	unsigned long	tx_fifo_errors;
	unsigned long	tx_heartbeat_errors;
	unsigned long	tx_window_errors;
	
	/* for cslip etc */
	unsigned long	rx_compressed;
	unsigned long	tx_compressed;
};


void* skb_put( PacketBuf_s* psBuffer, int nSize )
{
	void* pOldEnd = psBuffer->pb_pData + psBuffer->pb_nSize;
	psBuffer->pb_nSize += nSize;
	return( pOldEnd );
}



#define MAX_ADDR_LEN	6		/* Largest hardware address length */

struct device
{

	/*
	 * This is the first field of the "visible" part of this structure
	 * (i.e. as seen by users in the "Space.c" file).  It is the name
	 * the interface.
	 */
	char			*name;

	/*
	 *	I/O specific fields
	 *	FIXME: Merge these and struct ifmap into one
	 */
	unsigned long		base_addr;	/* device I/O address	*/
	unsigned int		irq;		/* device IRQ number	*/
	
	/* Low-level status flags. */
	volatile unsigned char	start;		/* start an operation	*/
	/*
	 * These two are just single-bit flags, but due to atomicity
	 * reasons they have to be inside a "unsigned long". However,
	 * they should be inside the SAME unsigned long instead of
	 * this wasteful use of memory..
	 */
	unsigned long		interrupt;	/* bitops.. */
	unsigned long		tbusy;		/* transmitter busy */
	
	struct device		*next;
	
	/*
	 * This marks the end of the "visible" part of the structure. All
	 * fields hereafter are internal to the system, and may change at
	 * will (read: may be cleaned up at will).
	 */

	/* These may be needed for future network-power-down code. */
	unsigned long		trans_start;	/* Time (in jiffies) of last Tx	*/
	
	unsigned short		flags;	/* interface flags (a la BSD)	*/
	void			*priv;	/* pointer to private data	*/
	
	/* Interface address info. */
//	unsigned char		broadcast[MAX_ADDR_LEN];	/* hw bcast add	*/
	unsigned char		pad;		/* make dev_addr aligned to 8 bytes */
	unsigned char		dev_addr[MAX_ADDR_LEN];	/* hw address	*/
//	unsigned char		addr_len;	/* hardware address length	*/

	struct dev_mc_list	*mc_list;	/* Multicast mac addresses	*/
	int			mc_count;	/* Number of installed mcasts	*/
//	int			promiscuity;
//	int			allmulti;
    
	/* For load balancing driver pair support */
  

	NetQueue_s*   packet_queue;
	volatile bool run_timer;
	thread_id	  timer_thread;
	int irq_handle; /* IRQ handler handle */
};

#include "mii.h"

#define MAX_MII_CNT	4
struct netdev_private {
	struct device *next_module;
	
	/* Descriptor rings */
	struct rx_desc *rx_ring;
	struct tx_desc *tx_ring;
	uint32 rx_ring_handle;
	uint32 tx_ring_handle;

	/* The addresses of receive-in-place skbuffs. */
	PacketBuf_s *rx_skbuff[RX_RING_SIZE];
	uint32 rx_skbuff_dma[RX_RING_SIZE];

	/* The saved address of a sent-in-place packet/buffer, for later free(). */
	PacketBuf_s *tx_skbuff[TX_RING_SIZE];
	uint32 tx_skbuff_dma[TX_RING_SIZE];

	/* Tx bounce buffers */
	unsigned char *tx_buf[TX_RING_SIZE];
	unsigned char *tx_bufs;
	uint32 tx_bufs_handle;

	PCI_Info_s pdev;
	struct net_device_stats stats;
	//struct timer_list timer;	/* Media monitoring timer. */
	SpinLock_s lock;

	/* Frequently used values: keep some adjacent for cache effect. */
	int chip_id, drv_flags;
	struct rx_desc *rx_head_desc;
	unsigned int cur_rx, dirty_rx;		/* Producer/consumer ring indices */
	unsigned int cur_tx, dirty_tx;
	unsigned int rx_buf_sz;				/* Based on MTU+slack. */
	uint16 chip_cmd;						/* Current setting for ChipCmd */

	/* These values are keep track of the transceiver/media in use. */
	sem_id		 tx_sema;
	unsigned int tx_full:1;				/* The Tx queue is full. */
	unsigned int full_duplex:1;			/* Full-duplex operation requested. */
	unsigned int duplex_lock:1;
	unsigned int default_port:4;		/* Last dev->if_port value. */
	uint8 tx_thresh, rx_thresh;

	/* MII transceiver section. */
	uint16 advertising;					/* NWay media advertisement */
	unsigned char phys[2];			/* MII device addresses. */
	unsigned int mii_cnt;			/* number of MIIs found, but only the first one is used */
	uint16 mii_status;						/* last read MII status */
};


static DeviceOperations_s g_sDevOps;
PCI_bus_s* g_psBus;
static struct device *root_via_dev = NULL;

static int  mdio_read(struct device *dev, int phy_id, int location);
static void mdio_write(struct device *dev, int phy_id, int location, int value);
static int  via_rhine_open(struct device *dev);
static void via_rhine_check_duplex(struct device *dev);
static int32 via_rhine_timer(void* data);
static void via_rhine_tx_timeout(struct device *dev);
static int  via_rhine_start_tx(PacketBuf_s *skb, struct device *dev);
static int via_rhine_interrupt(int irq, void *dev_instance, SysCallRegs_s *regs);
static void via_rhine_tx(struct device *dev);
static void via_rhine_rx(struct device *dev);
static void via_rhine_error(struct device *dev, int intr_status);
static void via_rhine_set_rx_mode(struct device *dev);
static struct net_device_stats *via_rhine_get_stats(struct device *dev);
static int via_rhine_ioctl(struct device *dev, struct ifreq *rq, int cmd);
static int  via_rhine_close(struct device *dev);
static inline void clear_tally_counters(long ioaddr);


/*
 * Get power related registers into sane state.
 * Notify user about past WOL event.
 */
static void rhine_power_init(struct device *dev)
{
	long ioaddr = dev->base_addr;
	uint16 wolstat;
	struct netdev_private *np = dev->priv;

	if (np->drv_flags & HasWOL) {
		/* Make sure chip is in power state D0 */
		writeb(readb(ioaddr + StickyHW) & 0xFC, ioaddr + StickyHW);

		/* Disable "force PME-enable" */
		writeb(0x80, ioaddr + WOLcgClr);

		/* Clear power-event config bits (WOL) */
		writeb(0xFF, ioaddr + WOLcrClr);
		/* More recent cards can manage two additional patterns */
		//if (drv_flags & rq6patterns)
		//	iowrite8(0x03, ioaddr + WOLcrClr1);

		/* Save power-event status bits */
		wolstat = readb(ioaddr + PwrcsrSet);
		//if (drv_flags & rq6patterns)
		//	wolstat |= (ioread8(ioaddr + PwrcsrSet1) & 0x03) << 8;

		/* Clear power-event status bits */
		writeb(0xFF, ioaddr + PwrcsrClr);
		//if (drv_flags & rq6patterns)
		//	iowrite8(0x03, ioaddr + PwrcsrClr1);

		if (wolstat) {
			char *reason;
			switch (wolstat) {
			case WOLmagic:
				reason = "Magic packet";
				break;
			case WOLlnkon:
				reason = "Link went up";
				break;
			case WOLlnkoff:
				reason = "Link went down";
				break;
			case WOLucast:
				reason = "Unicast packet";
				break;
			case WOLbmcast:
				reason = "Multicast/broadcast packet";
				break;
			default:
				reason = "Unknown";
			}
			printk(KERN_INFO "%s: Woke system up. Reason: %s.\n",
			       DRV_NAME, reason);
		}
	}
}

static int via_rhine_init_one ( int device_handle, PCI_Info_s pdev, int chip_id )
{
	struct device *dev;
	struct netdev_private *np;
	int i, option;
	static int card_idx = -1;
	long ioaddr;
	long memaddr;
	int io_size;
	int pci_flags;
	char node_path[64];
	void* pAddr = NULL;
#ifdef USE_MEM
	long ioaddr0;
#endif
	PCI_bus_s* psBus = get_busmanager( PCI_BUS_NAME, PCI_BUS_VERSION );
	
/* when built into the kernel, we only print version if device is found */

	static int printed_version;
	if (!printed_version++)
		printk(version);


	card_idx++;
	option = card_idx < MAX_UNITS ? options[card_idx] : 0;
	io_size = via_rhine_chip_info[chip_id].io_size;
	pci_flags = via_rhine_chip_info[chip_id].pci_flags;

	psBus->write_pci_config( pdev.nBus, pdev.nDevice, pdev.nFunction, PCI_COMMAND, 2, 
		psBus->read_pci_config( pdev.nBus, pdev.nDevice, pdev.nFunction, PCI_COMMAND , 2 ) | PCI_COMMAND_MASTER | PCI_COMMAND_IO | PCI_COMMAND_MEMORY );


	
	ioaddr = pdev.u.h0.nBase0 & PCI_ADDRESS_IO_MASK;
	memaddr = pdev.u.h0.nBase1 & PCI_ADDRESS_MEMORY_32_MASK;

	dev = kmalloc( sizeof(*dev), MEMF_KERNEL | MEMF_CLEAR );
	dev->name = "VIA";
	np = kmalloc(sizeof(*np), MEMF_KERNEL );
	memset(np, 0, sizeof(*np));
	dev->priv = np;

	np->next_module = root_via_dev;
	root_via_dev = dev;
	
	np->chip_id = chip_id;
	np->drv_flags = via_rhine_chip_info[chip_id].drv_flags;
	np->pdev = pdev;
	

#ifdef USE_MEM
	ioaddr0 = ioaddr;
	
	
	remap_area( create_area( "via_mmio", &pAddr, PAGE_SIZE * 2, PAGE_SIZE * 2, AREA_ANY_ADDRESS|AREA_KERNEL,
								AREA_FULL_LOCK ), (void*)( memaddr & PAGE_MASK ) ); 

	ioaddr = (uint32)pAddr + ( memaddr - ( memaddr & PAGE_MASK ) );

	/* Check that selected MMIO registers match the PIO ones */
	i = 0;
	while (mmio_verify_registers[i]) {
		int reg = mmio_verify_registers[i++];
		unsigned char a = inb(ioaddr0+reg);
		unsigned char b = readb(ioaddr+reg);
		if (a != b) {
			printk (KERN_ERR "MMIO do not match PIO [%02x] (%02x != %02x)\n",
					reg, a, b);
			goto err_out_unmap;
		}
	}
	
#endif

	dev->base_addr = ioaddr;
	rhine_power_init(dev);
	
	for (i = 0; i < 6; i++)
		dev->dev_addr[i] = readb(ioaddr + StationAddr + i);

	if( memcmp(dev->dev_addr, "\0\0\0\0\0", 6) == 0) {
         /* Reload the station address from the EEPROM. */
         writeb(0x20, ioaddr + MACRegEEcsr);
             /* Typically 2 cycles to reload. */
             for (i = 0; i < 150; i++)
                    if (! (readb(ioaddr + MACRegEEcsr) & 0x20))
                              break;
               for (i = 0; i < 6; i++)
                        dev->dev_addr[i] = readb(ioaddr + StationAddr + i);
                if (memcmp(dev->dev_addr, "\0\0\0\0\0", 6) == 0) {
                        printk(" (MISSING EEPROM ADDRESS)");
                        memcpy(dev->dev_addr, "\100Linux", 6);
                }
        }


	/* Reset the chip to erase previous misconfiguration. */
	writew(CmdReset, ioaddr + ChipCmd);

	/*if (!is_valid_ether_addr(dev->dev_addr)) {
		printk(KERN_ERR "Invalid MAC address for card #%d\n", card_idx);
		goto err_out_unmap;
	}*/

	dev->irq = pdev.u.h0.nInterruptLine;

	spinlock_init( &np->lock, "via_spinlock" );


	/* The lower four bits are the media type. */
	if (option > 0) {
		if (option & 0x200)
			np->full_duplex = 1;
		np->default_port = option & 15;
	}
	if (card_idx < MAX_UNITS  &&  full_duplex[card_idx] > 0)
		np->full_duplex = 1;

	if (np->full_duplex) {
		printk(KERN_INFO "%s: Set to forced full duplex, autonegotiation"
			   " disabled.\n", dev->name);
		np->duplex_lock = 1;
	}

	/* The chip-specific entries in the device structure. */
	/*dev->open = via_rhine_open;
	dev->hard_start_xmit = via_rhine_start_tx;
	dev->stop = via_rhine_close;
	dev->get_stats = via_rhine_get_stats;
	dev->set_multicast_list = via_rhine_set_rx_mode;
	dev->do_ioctl = via_rhine_ioctl;
	dev->tx_timeout = via_rhine_tx_timeout;
	
	dev->watchdog_timeo = TX_TIMEOUT;
	*/
	
	//if (np->drv_flags & ReqTxAlign)
		//dev->features |= NETIF_F_SG|NETIF_F_HW_CSUM;

	printk(KERN_INFO "%s: %s at 0x%lx IRQ %d\n",
		   dev->name, via_rhine_chip_info[chip_id].name,
		   (pci_flags & PCI_USES_IO) ? ioaddr : memaddr, dev->irq );

	
	if (np->drv_flags & CanHaveMII) {
		int phy, phy_idx = 0;
		np->phys[0] = 1;		/* Standard for this chip. */
		for (phy = 1; phy < 32 && phy_idx < MAX_MII_CNT; phy++) {
			int mii_status = mdio_read(dev, phy, 1);
			if (mii_status != 0xffff  &&  mii_status != 0x0000) {
				np->phys[phy_idx++] = phy;
				np->advertising = mdio_read(dev, phy, 4);
				printk(KERN_INFO "%s: MII PHY found at address %d, status "
					   "0x%4.4x advertising %4.4x Link %4.4x.\n",
					   dev->name, phy, mii_status, np->advertising,
					   mdio_read(dev, phy, 5));

				if (mii_status & MIILink)
					printk( "VIA: Cable connected\n" );
			}
		}
		np->mii_cnt = phy_idx;
	}

	/* Allow forcing the media type. */
	if (option > 0) {
		if (option & 0x220)
			np->full_duplex = 1;
		np->default_port = option & 0x3ff;
		if (np->default_port & 0x330) {
			/* FIXME: shouldn't someone check this variable? */
			/* np->medialock = 1; */
			printk(KERN_INFO "  Forcing %dMbs %s-duplex operation.\n",
				   (option & 0x300 ? 100 : 10),
				   (option & 0x220 ? "full" : "half"));
			if (np->mii_cnt)
				mdio_write(dev, np->phys[0], 0,
						   ((option & 0x300) ? 0x2000 : 0) |  /* 100mbps? */
						   ((option & 0x220) ? 0x0100 : 0));  /* Full duplex? */
		}
	}
	
	if( claim_device( device_handle, pdev.nHandle, via_rhine_chip_info[chip_id].name, DEVICE_NET ) != 0 )
		goto err_out_free_netdev;
	sprintf( node_path, "net/eth/via-%d", card_idx );
	create_device_node( device_handle, pdev.nHandle, node_path, &g_sDevOps, dev );
	printk( "via_init_one() Create node: %s\n", node_path );
	return 0;

err_out_unmap:
#ifdef USE_MEM
err_out_free_res:
#endif
err_out_free_netdev:
	kfree (dev);
err_out:
	return -ENODEV;
}

static int alloc_ring(struct device* dev)
{
	struct netdev_private *np = dev->priv;
	void *ring;
	uint32 ring_handle;
	
	ring = kmalloc( RX_RING_SIZE * sizeof(struct rx_desc) +
				    TX_RING_SIZE * sizeof(struct tx_desc) + PAGE_SIZE, MEMF_KERNEL );
	ring_handle = (uint32)ring;
	ring = (void*)(( ring_handle + PAGE_SIZE ) & ~( PAGE_SIZE - 1 ));

	/*ring = pci_alloc_consistent(np->pdev, 
				    RX_RING_SIZE * sizeof(struct rx_desc) +
				    TX_RING_SIZE * sizeof(struct tx_desc),
				    &ring_dma);*/
	if (!ring) {
		printk(KERN_ERR "Could not allocate DMA memory.\n");
		return -ENOMEM;
	}
	if (np->drv_flags & ReqTxAlign) {
		np->tx_bufs = kmalloc( PKT_BUF_SZ * TX_RING_SIZE + PAGE_SIZE, MEMF_KERNEL );
		np->tx_bufs_handle = (uint32)np->tx_bufs;
		np->tx_bufs = (void*)(( np->tx_bufs_handle + PAGE_SIZE ) & ~( PAGE_SIZE - 1 ));
		/*np->tx_bufs = pci_alloc_consistent(np->pdev, PKT_BUF_SZ * TX_RING_SIZE,
								   &np->tx_bufs_dma);*/
		if (np->tx_bufs == NULL) {
			kfree( (void*)np->tx_bufs_handle );
			return -ENOMEM;
		}
	}

	np->rx_ring = ring;
	np->tx_ring = ring + RX_RING_SIZE * sizeof(struct rx_desc);
	np->rx_ring_handle = ring_handle;
	np->tx_ring_handle = ring_handle + RX_RING_SIZE * sizeof(struct rx_desc);
	
	
	printk( "VIA: RX ring @ %x TX ring @ %x\n", (uint)np->rx_ring, (uint)np->tx_ring ); 
	return 0;
}

void free_ring(struct device* dev)
{
	struct netdev_private *np = dev->priv;

	kfree( (void*)np->rx_ring_handle );
	
	np->tx_ring = NULL;

	if (np->tx_bufs)
		kfree( (void*)np->tx_bufs_handle );
		/*pci_free_consistent(np->pdev, PKT_BUF_SZ * TX_RING_SIZE,
							np->tx_bufs, np->tx_bufs_dma);*/

	np->tx_bufs = NULL;

}

static void alloc_rbufs(struct device *dev)
{
	struct netdev_private *np = dev->priv;
	uint32 next;
	int i;

	np->dirty_rx = np->cur_rx = 0;

	np->rx_buf_sz = PKT_BUF_SZ;
	np->rx_head_desc = &np->rx_ring[0];
	next = (uint32)np->rx_ring;
	
	/* Init the ring entries */
	for (i = 0; i < RX_RING_SIZE; i++) {
		np->rx_ring[i].rx_status = 0;
		np->rx_ring[i].desc_length = np->rx_buf_sz;
		next += sizeof(struct rx_desc);
		np->rx_ring[i].next_desc = next;
		np->rx_skbuff[i] = 0;
	}
	/* Mark the last entry as wrapping the ring. */
	np->rx_ring[i-1].next_desc = (uint32)np->rx_ring;

	/* Fill in the Rx buffers.  Handle allocation failure gracefully. */
	for (i = 0; i < RX_RING_SIZE; i++) {
		PacketBuf_s *skb = alloc_pkt_buffer(np->rx_buf_sz);
		np->rx_skbuff[i] = skb;
		if (skb == NULL)
			break;
		skb->pb_nSize = 0;
		//skb->dev = dev;                 /* Mark as being used by this device. */

		np->rx_skbuff_dma[i] = (uint32)skb->pb_pData;
			/*pci_map_single(np->pdev, skb->tail, np->rx_buf_sz,
						   PCI_DMA_FROMDEVICE);*/

		np->rx_ring[i].addr = np->rx_skbuff_dma[i];
		np->rx_ring[i].rx_status = DescOwn;
	}
	np->dirty_rx = (unsigned int)(i - RX_RING_SIZE);
}

static void free_rbufs(struct device* dev)
{
	struct netdev_private *np = dev->priv;
	int i;

	/* Free all the skbuffs in the Rx queue. */
	for (i = 0; i < RX_RING_SIZE; i++) {
		np->rx_ring[i].rx_status = 0;
		np->rx_ring[i].addr = 0xBADF00D0; /* An invalid address. */
		if (np->rx_skbuff[i]) {
			/*pci_unmap_single(np->pdev,
							 np->rx_skbuff_dma[i],
							 np->rx_buf_sz, PCI_DMA_FROMDEVICE);*/
			free_pkt_buffer(np->rx_skbuff[i]);
		}
		np->rx_skbuff[i] = 0;
	}
}

static void alloc_tbufs(struct device* dev)
{
	struct netdev_private *np = dev->priv;
	uint32 next;
	int i;

	np->dirty_tx = np->cur_tx = np->tx_full = 0;
	next = (uint32)np->tx_ring;
	for (i = 0; i < TX_RING_SIZE; i++) {
		np->tx_skbuff[i] = 0;
		np->tx_ring[i].tx_status = 0;
		np->tx_ring[i].desc_length = 0x00e08000;
		next += sizeof(struct tx_desc);
		np->tx_ring[i].next_desc = next;
		np->tx_buf[i] = &np->tx_bufs[i * PKT_BUF_SZ];
	}
	np->tx_ring[i-1].next_desc = (uint32)np->tx_ring;

}

static void free_tbufs(struct device* dev)
{
	struct netdev_private *np = dev->priv;
	int i;

	for (i = 0; i < TX_RING_SIZE; i++) {
		np->tx_ring[i].tx_status = 0;
		np->tx_ring[i].desc_length = 0x00e08000;
		np->tx_ring[i].addr = 0xBADF00D0; /* An invalid address. */
		if (np->tx_skbuff[i]) {
			/*if (np->tx_skbuff_dma[i]) {
				pci_unmap_single(np->pdev,
								 np->tx_skbuff_dma[i],
								 np->tx_skbuff[i]->len, PCI_DMA_TODEVICE);
			}*/
			free_pkt_buffer(np->tx_skbuff[i]);
		}
		np->tx_skbuff[i] = 0;
		np->tx_buf[i] = 0;
	}
}

static void init_registers(struct device *dev)
{
	struct netdev_private *np = dev->priv;
	long ioaddr = dev->base_addr;
	int i;

	for (i = 0; i < 6; i++)
		writeb(dev->dev_addr[i], ioaddr + StationAddr + i);

	/* Initialize other registers. */
	writew(0x0006, ioaddr + PCIBusConfig);	/* Tune configuration??? */
	/* Configure the FIFO thresholds. */
	writeb(0x20, ioaddr + TxConfig);	/* Initial threshold 32 bytes */
	np->tx_thresh = 0x20;
	np->rx_thresh = 0x60;			/* Written in via_rhine_set_rx_mode(). */

//	if (dev->if_port == 0)
	//	dev->if_port = np->default_port;

	writel((uint32)np->rx_ring, ioaddr + RxRingPtr);
	writel((uint32)np->tx_ring, ioaddr + TxRingPtr);

	via_rhine_set_rx_mode(dev);

	/* Enable interrupts by setting the interrupt mask. */
	writew(IntrRxDone | IntrRxErr | IntrRxEmpty| IntrRxOverflow| IntrRxDropped|
		   IntrTxDone | IntrTxAbort | IntrTxUnderrun |
		   IntrPCIErr | IntrStatsMax | IntrLinkChange | IntrMIIChange,
		   ioaddr + IntrEnable);

	np->chip_cmd = CmdStart|CmdTxOn|CmdRxOn|CmdNoTxPoll;
	if (np->duplex_lock)
		np->chip_cmd |= CmdFDuplex;
	writew(np->chip_cmd, ioaddr + ChipCmd);

	via_rhine_check_duplex(dev);

	/* The LED outputs of various MII xcvrs should be configured.  */
	/* For NS or Mison phys, turn on bit 1 in register 0x17 */
	/* For ESI phys, turn on bit 7 in register 0x17. */
	mdio_write(dev, np->phys[0], 0x17, mdio_read(dev, np->phys[0], 0x17) |
			   (np->drv_flags & HasESIPhy) ? 0x0080 : 0x0001);
}
/* Read and write over the MII Management Data I/O (MDIO) interface. */

static int mdio_read(struct device *dev, int phy_id, int regnum)
{
	long ioaddr = dev->base_addr;
	int boguscnt = 1024;

	/* Wait for a previous command to complete. */
	while ((readb(ioaddr + MIICmd) & 0x60) && --boguscnt > 0)
		;
	writeb(0x00, ioaddr + MIICmd);
	writeb(phy_id, ioaddr + MIIPhyAddr);
	writeb(regnum, ioaddr + MIIRegAddr);
	writeb(0x40, ioaddr + MIICmd);			/* Trigger read */
	boguscnt = 1024;
	while ((readb(ioaddr + MIICmd) & 0x40) && --boguscnt > 0)
		;
	return readw(ioaddr + MIIData);
}

static void mdio_write(struct device *dev, int phy_id, int regnum, int value)
{
	struct netdev_private *np = dev->priv;
	long ioaddr = dev->base_addr;
	int boguscnt = 1024;

	if (phy_id == np->phys[0]) {
		switch (regnum) {
		case 0:							/* Is user forcing speed/duplex? */
			if (value & 0x9000)			/* Autonegotiation. */
				np->duplex_lock = 0;
			else
				np->full_duplex = (value & 0x0100) ? 1 : 0;
			break;
		case 4:
			np->advertising = value;
			break;
		}
	}

	/* Wait for a previous command to complete. */
	while ((readb(ioaddr + MIICmd) & 0x60) && --boguscnt > 0)
		;
	writeb(0x00, ioaddr + MIICmd);
	writeb(phy_id, ioaddr + MIIPhyAddr);
	writeb(regnum, ioaddr + MIIRegAddr);
	writew(value, ioaddr + MIIData);
	writeb(0x20, ioaddr + MIICmd);			/* Trigger write. */
}


static int via_rhine_open(struct device *dev)
{
	struct netdev_private *np = dev->priv;
	long ioaddr = dev->base_addr;
	int i;

	/* Reset the chip. */
	writew(CmdReset, ioaddr + ChipCmd);

	np->tx_sema = create_semaphore( "via_tx_sem", 0, 0 );
	dev->irq_handle = request_irq(dev->irq, &via_rhine_interrupt, NULL, SA_SHIRQ, dev->name, dev);
	if (dev->irq_handle < 0)
		return dev->irq_handle;

	if (debug > 1)
		printk(KERN_DEBUG "%s: via_rhine_open() irq %d.\n",
			   dev->name, dev->irq);
	
	i = alloc_ring(dev);
	if (i)
		return i;
	alloc_rbufs(dev);
	alloc_tbufs(dev);
	
	dev->tbusy = 0;
	dev->interrupt = 0;
	dev->start = 1;
	
	init_registers(dev);
	if (debug > 2)
		printk(KERN_DEBUG "%s: Done via_rhine_open(), status %4.4x "
			   "MII status: %4.4x.\n",
			   dev->name, readw(ioaddr + ChipCmd),
			   mdio_read(dev, np->phys[0], MII_BMSR));

	return 0;
}

static void via_rhine_check_duplex(struct device *dev)
{
	struct netdev_private *np = dev->priv;
	long ioaddr = dev->base_addr;
	int mii_lpa = mdio_read(dev, np->phys[0], MII_LPA);
	int negotiated = mii_lpa & np->advertising;
	int duplex;

	if (np->duplex_lock  ||  mii_lpa == 0xffff)
		return;
	duplex = (negotiated & 0x0100) || (negotiated & 0x01C0) == 0x0040;
	if (np->full_duplex != duplex) {
		np->full_duplex = duplex;
		if (debug)
			printk(KERN_INFO "%s: Setting %s-duplex based on MII #%d link"
				   " partner capability of %4.4x.\n", dev->name,
				   duplex ? "full" : "half", np->phys[0], mii_lpa);
		if (duplex)
			np->chip_cmd |= CmdFDuplex;
		else
			np->chip_cmd &= ~CmdFDuplex;
		writew(np->chip_cmd, ioaddr + ChipCmd);
	}
}


static int32 via_rhine_timer( void* data )
{
	struct device *dev = (struct device *)data;
	struct netdev_private *np = dev->priv;
	long ioaddr = dev->base_addr;
	int next_tick = 10*100000LL;
	int mii_status;

	snooze( 2400000LL ); // 2,4 sec
	while ( dev->run_timer ) {
		/*if (debug > 3) {
			printk(KERN_DEBUG "%s: VIA Rhine monitor tick, status %4.4x.\n",
			   dev->name, readw(ioaddr + IntrStatus));
		}*/

		spinlock(&np->lock);

		via_rhine_check_duplex(dev);

		/* make IFF_RUNNING follow the MII status bit "Link established" */
		mii_status = mdio_read(dev, np->phys[0], MII_BMSR);
		if ( (mii_status & MIILink) != (np->mii_status & MIILink) ) {
			if (mii_status & MIILink)
				printk( "VIA: Cable connected\n" );
			else
				printk( "VIA: Cable disconnected\n" );
		}
		np->mii_status = mii_status;
		
		spinunlock(&np->lock);

		snooze( next_tick );
	}
}


static void via_rhine_tx_timeout (struct device *dev)
{
	struct netdev_private *np = dev->priv;
	long ioaddr = dev->base_addr;

	printk (KERN_WARNING "%s: Transmit timed out, status %4.4x, PHY status "
		"%4.4x, resetting...\n",
		dev->name, readw (ioaddr + IntrStatus),
		mdio_read (dev, np->phys[0], MII_BMSR));

	//dev->if_port = 0;
	
	writel((uint32)np->tx_ring + ( np->dirty_tx % TX_RING_SIZE ), ioaddr + TxRingPtr);
	writew( CmdTxDemand | np->chip_cmd, ioaddr + ChipCmd );

	dev->trans_start = get_system_time() / 1000L;
	np->stats.tx_errors++;
	return;
	//netif_wake_queue(dev);
}

static int via_rhine_start_tx(PacketBuf_s *skb, struct device *dev)
{
	struct netdev_private *np = dev->priv;
	unsigned entry;
	int flags;
	
	/* Block a timer-based transmit from overlapping.  This could better be
	   done with atomic_swap(1, dev->tbusy), but set_bit() works as well. */
again:
	flags = spinlock_disable( &np->lock );
	if (test_and_set_bit(0, (void*)&dev->tbusy) != 0) {
		if ( spinunlock_and_suspend( np->tx_sema, &np->lock, flags, 4000000LL ) < 0 ) {
			printk( "Error: via_wait_for_tx() timed out\n" );
			return 1;
		}
		if (get_system_time() / 1000 - dev->trans_start < TX_TIMEOUT) {
			goto again;
		}
		via_rhine_tx_timeout(dev);
		return 1;
	}

	/* Caution: the write order is important here, set the field
	   with the "ownership" bits last. */

	/* Calculate the next Tx descriptor entry. */
	entry = np->cur_tx % TX_RING_SIZE;

	np->tx_skbuff[entry] = skb;

	if ((np->drv_flags & ReqTxAlign) &&
		(((long)skb->pb_pData & 3) /*|| skb_shinfo(skb)->nr_frags != 0 || skb->ip_summed == CHECKSUM_HW*/)
		) {
		/* Must use alignment buffer. */
		if (skb->pb_nSize > PKT_BUF_SZ) {
			/* packet too long, drop it */
			free_pkt_buffer(skb);
			np->tx_skbuff[entry] = NULL;
			np->stats.tx_dropped++;
			return 0;
		}
		memcpy(np->tx_buf[entry], skb->pb_pData, skb->pb_nSize);
		
		
		//skb_copy_and_csum_dev(skb, np->tx_buf[entry]);
		np->tx_skbuff_dma[entry] = 0;
		np->tx_ring[entry].addr =   (uint32)np->tx_buf[entry] - (uint32)np->tx_bufs;
	} else {
		np->tx_skbuff_dma[entry] = (uint32)skb->pb_pData;
			//pci_map_single(np->pdev, skb->data, skb->len, PCI_DMA_TODEVICE);
		np->tx_ring[entry].addr = np->tx_skbuff_dma[entry];
	}

	np->tx_ring[entry].desc_length = 
		0x00E08000 | (skb->pb_nSize >= ETH_ZLEN ? skb->pb_nSize : ETH_ZLEN);

	
	//wmb();
	np->tx_ring[entry].tx_status = DescOwn;
	//wmb();

	np->cur_tx++;

	/* Non-x86 Todo: explicitly flush cache lines here. */

	/* Wake the potentially-idle transmit channel. */
	writew(CmdTxDemand | np->chip_cmd, dev->base_addr + ChipCmd);


	if (np->cur_tx - np->dirty_tx < TX_QUEUE_LEN) {/* Typical path */
		clear_bit(0, (void*)&dev->tbusy);
		wakeup_sem( np->tx_sema, true );
	} else {
		set_bit(0, (void*)&dev->tbusy);
		np->tx_full = 1;
	}
	//if (np->cur_tx == np-> + TX_QUEUE_LEN)
		//netif_stop_queue(dev);

	dev->trans_start = get_system_time() / 1000L;

	
	spinunlock_enable( &np->lock, flags );

	if (debug > 4) {
		printk(KERN_DEBUG "%s: Transmit frame #%d queued in slot %d.\n",
			   dev->name, np->cur_tx, entry);
	}
	return 0;
}

/* The interrupt handler does all of the Rx thread work and cleans up
   after the Tx thread. */
static int via_rhine_interrupt(int irq, void *dev_instance, SysCallRegs_s* regs )
{
	struct device *dev = (struct device *)dev_instance;
	long ioaddr;
	uint32 intr_status;
	int boguscnt = max_interrupt_work;

	ioaddr = dev->base_addr;
	
	while ((intr_status = readw(ioaddr + IntrStatus))) {
		/* Acknowledge all of the current interrupt sources ASAP. */
		writew(intr_status & 0xffff, ioaddr + IntrStatus);

		if (debug > 4)
			printk(KERN_DEBUG "%s: Interrupt, status %4.4x.\n",
				   dev->name, intr_status);

		if (intr_status & (IntrRxDone | IntrRxErr | IntrRxDropped |
						   IntrRxWakeUp | IntrRxEmpty | IntrRxNoBuf))
			via_rhine_rx(dev);

		if (intr_status & (IntrTxDone | IntrTxAbort | IntrTxUnderrun |
						   IntrTxAborted))
			via_rhine_tx(dev);

		/* Abnormal error summary/uncommon events handlers. */
		if (intr_status & (IntrPCIErr | IntrLinkChange | IntrMIIChange |
						   IntrStatsMax | IntrTxAbort | IntrTxUnderrun))
			via_rhine_error(dev, intr_status);

		if (--boguscnt < 0) {
			printk(KERN_WARNING "%s: Too much work at interrupt, "
				   "status=0x%4.4x.\n",
				   dev->name, intr_status);
			break;
		}
	}

	//if (debug > 3)
		//printk(KERN_DEBUG "%s: exiting interrupt, status=%#4.4x.\n",
			 //  dev->name, readw(ioaddr + IntrStatus));
	return( 0 );
}

/* This routine is logically part of the interrupt handler, but isolated
   for clarity. */
static void via_rhine_tx(struct device *dev)
{
	struct netdev_private *np = dev->priv;
	int txstatus = 0, entry = np->dirty_tx % TX_RING_SIZE;

	spinlock (&np->lock);

	/* find and cleanup dirty tx descriptors */
	while (np->dirty_tx != np->cur_tx) {
		txstatus = np->tx_ring[entry].tx_status;
		if (txstatus & DescOwn)
			break;
		if (debug > 6)
			printk(KERN_DEBUG " Tx scavenge %d status %8.8x.\n",
				   entry, txstatus);
		if (txstatus & 0x8000) {
			if (debug > 1)
				printk(KERN_DEBUG "%s: Transmit error, Tx status %8.8x.\n",
					   dev->name, txstatus);
			np->stats.tx_errors++;
			if (txstatus & 0x0400) np->stats.tx_carrier_errors++;
			if (txstatus & 0x0200) np->stats.tx_window_errors++;
			if (txstatus & 0x0100) np->stats.tx_aborted_errors++;
			if (txstatus & 0x0080) np->stats.tx_heartbeat_errors++;
			if (txstatus & 0x0002) np->stats.tx_fifo_errors++;
			/* Transmitter restarted in 'abnormal' handler. */
		} else {
			np->stats.collisions += (txstatus >> 3) & 15;
			np->stats.tx_bytes += np->tx_skbuff[entry]->pb_nSize;
			np->stats.tx_packets++;
		}
		/* Free the original skb. */
		/*if (np->tx_skbuff_dma[entry]) {
			pci_unmap_single(np->pdev,
							 np->tx_skbuff_dma[entry],
							 np->tx_skbuff[entry]->len, PCI_DMA_TODEVICE);
		}*/
		free_pkt_buffer(np->tx_skbuff[entry]);
		np->tx_skbuff[entry] = NULL;
		entry = (++np->dirty_tx) % TX_RING_SIZE;
	}
	if (np->tx_full && (np->cur_tx - np->dirty_tx) < TX_QUEUE_LEN - 4)
	{
		np->tx_full = 0;
		dev->tbusy = 0;
		wakeup_sem( np->tx_sema, true );
	}
	
	/*if ((np->cur_tx - np->dirty_tx) < TX_QUEUE_LEN - 4)
		netif_wake_queue (dev);
*/
	spinunlock (&np->lock);
}

/* This routine is logically part of the interrupt handler, but isolated
   for clarity and better register allocation. */
static void via_rhine_rx(struct device *dev)
{
	struct netdev_private *np = dev->priv;
	int entry = np->cur_rx % RX_RING_SIZE;
	int boguscnt = np->dirty_rx + RX_RING_SIZE - np->cur_rx;

	if (debug > 4) {
		printk(KERN_DEBUG " In via_rhine_rx(), entry %d status %8.8x.\n",
			   entry, np->rx_head_desc->rx_status);
	}

	/* If EOP is set on the next entry, it's a new packet. Send it up. */
	while ( ! (np->rx_head_desc->rx_status & DescOwn)) {
		struct rx_desc *desc = np->rx_head_desc;
		uint32 desc_status = desc->rx_status;
		int data_size = desc_status >> 16;

		if (debug > 4)
			printk(KERN_DEBUG "  via_rhine_rx() status is %8.8x.\n",
				   desc_status);
		if (--boguscnt < 0)
			break;
		if ( (desc_status & (RxWholePkt | RxErr)) !=  RxWholePkt) {
			if ((desc_status & RxWholePkt) !=  RxWholePkt) {
				printk(KERN_WARNING "%s: Oversized Ethernet frame spanned "
					   "multiple buffers, entry %#x length %d status %8.8x!\n",
					   dev->name, entry, data_size, desc_status);
				printk(KERN_WARNING "%s: Oversized Ethernet frame %p vs %p.\n",
					   dev->name, np->rx_head_desc, &np->rx_ring[entry]);
				np->stats.rx_length_errors++;
			} else if (desc_status & RxErr) {
				/* There was a error. */
				if (debug > 2)
					printk(KERN_DEBUG "  via_rhine_rx() Rx error was %8.8x.\n",
						   desc_status);
				np->stats.rx_errors++;
				if (desc_status & 0x0030) np->stats.rx_length_errors++;
				if (desc_status & 0x0048) np->stats.rx_fifo_errors++;
				if (desc_status & 0x0004) np->stats.rx_frame_errors++;
				if (desc_status & 0x0002) {
					/* this can also be updated outside the interrupt handler */
					spinlock (&np->lock);
					np->stats.rx_crc_errors++;
					spinunlock (&np->lock);
				}
			}
		} else {
			PacketBuf_s *skb;
			/* Length should omit the CRC */
			int pkt_len = data_size - 4;

			/* Check if the packet is long enough to accept without copying
			   to a minimally-sized skbuff. */
			if (pkt_len < rx_copybreak &&
				(skb = alloc_pkt_buffer(pkt_len + 2)) != NULL) {
				skb->pb_nSize = 0;
				//skb->dev = dev;
				//skb_reserve(skb, 2);	/* 16 byte align the IP header */
				//pci_dma_sync_single(np->pdev, np->rx_skbuff_dma[entry],
						//    np->rx_buf_sz, PCI_DMA_FROMDEVICE);

				/* *_IP_COPYSUM isn't defined anywhere and eth_copy_and_sum
				   is memcpy for all archs so this is kind of pointless right
				   now ... or? */
#if HAS_IP_COPYSUM                     /* Call copy + cksum if available. */
				eth_copy_and_sum(skb, np->rx_skbuff[entry]->tail, pkt_len, 0);
				skb_put(skb, pkt_len);
#else
				memcpy(skb_put(skb, pkt_len), np->rx_skbuff[entry]->pb_pData,
					   pkt_len);
#endif
			} else {
				skb = np->rx_skbuff[entry];
				if (skb == NULL) {
					printk(KERN_ERR "%s: Inconsistent Rx descriptor chain.\n",
						   dev->name);
					break;
				}
				np->rx_skbuff[entry] = NULL;
				skb_put(skb, pkt_len);
				//pci_unmap_single(np->pdev, np->rx_skbuff_dma[entry],
								// np->rx_buf_sz, PCI_DMA_FROMDEVICE);
			}
			if ( dev->packet_queue != NULL ) {
				skb->pb_uMacHdr.pRaw = skb->pb_pData;
				enqueue_packet( dev->packet_queue, skb );
			} else {
				printk( "Error: via_rhine_rx() received packet to downed device!\n" );
				free_pkt_buffer( skb );
			}
			//skb->protocol = eth_type_trans(skb, dev);
			//netif_rx(skb);
			np->stats.rx_bytes += pkt_len;
			np->stats.rx_packets++;
		}
		entry = (++np->cur_rx) % RX_RING_SIZE;
		np->rx_head_desc = &np->rx_ring[entry];
	}

	/* Refill the Rx ring buffers. */
	for (; np->cur_rx - np->dirty_rx > 0; np->dirty_rx++) {
		PacketBuf_s *skb;
		entry = np->dirty_rx % RX_RING_SIZE;
		if (np->rx_skbuff[entry] == NULL) {
			skb = alloc_pkt_buffer(np->rx_buf_sz);
			np->rx_skbuff[entry] = skb;
			if (skb == NULL)
				break;			/* Better luck next round. */
			//skb->dev = dev;			/* Mark as being used by this device. */
			np->rx_skbuff_dma[entry] = (uint32)skb->pb_pData;
				/*pci_map_single(np->pdev, skb->tail, np->rx_buf_sz, 
							   PCI_DMA_FROMDEVICE);*/
			np->rx_ring[entry].addr = np->rx_skbuff_dma[entry];
		}
		np->rx_ring[entry].rx_status = DescOwn;
	}

	/* Pre-emptively restart Rx engine. */
	writew(CmdRxDemand | np->chip_cmd, dev->base_addr + ChipCmd);
}

static void via_rhine_error(struct device *dev, int intr_status)
{
	struct netdev_private *np = dev->priv;
	long ioaddr = dev->base_addr;

	spinlock (&np->lock);

	if (intr_status & (IntrMIIChange | IntrLinkChange)) {
		if (readb(ioaddr + MIIStatus) & 0x02) {
			/* Link failed, restart autonegotiation. */
			if (np->drv_flags & HasDavicomPhy)
				mdio_write(dev, np->phys[0], MII_BMCR, 0x3300);
		} else
			via_rhine_check_duplex(dev);
		if (debug)
			printk(KERN_ERR "%s: MII status changed: Autonegotiation "
				   "advertising %4.4x  partner %4.4x.\n", dev->name,
			   mdio_read(dev, np->phys[0], MII_ADVERTISE),
			   mdio_read(dev, np->phys[0], MII_LPA));
	}
	if (intr_status & IntrStatsMax) {
		np->stats.rx_crc_errors	+= readw(ioaddr + RxCRCErrs);
		np->stats.rx_missed_errors	+= readw(ioaddr + RxMissed);
		clear_tally_counters(ioaddr);
	}
	if (intr_status & IntrTxAbort) {
		/* Stats counted in Tx-done handler, just restart Tx. */
		writew(CmdTxDemand | np->chip_cmd, dev->base_addr + ChipCmd);
	}
	if (intr_status & IntrTxUnderrun) {
		if (np->tx_thresh < 0xE0)
			writeb(np->tx_thresh += 0x20, ioaddr + TxConfig);
		if (debug > 1)
			printk(KERN_INFO "%s: Transmitter underrun, increasing Tx "
				   "threshold setting to %2.2x.\n", dev->name, np->tx_thresh);
	}
	if ((intr_status & ~( IntrLinkChange | IntrStatsMax |
						  IntrTxAbort | IntrTxAborted))) {
		if (debug > 1)
			printk(KERN_ERR "%s: Something Wicked happened! %4.4x.\n",
			   dev->name, intr_status);
		/* Recovery for other fault sources not known. */
		writew(CmdTxDemand | np->chip_cmd, dev->base_addr + ChipCmd);
	}

	spinunlock (&np->lock);
}

/* Clears the "tally counters" for CRC errors and missed frames(?).
   It has been reported that some chips need a write of 0 to clear
   these, for others the counters are set to 1 when written to and
   instead cleared when read. So we clear them both ways ... */
static inline void clear_tally_counters(const long ioaddr)
{
	writel(0, ioaddr + RxMissed);
	readw(ioaddr + RxCRCErrs);
	readw(ioaddr + RxMissed);
}


/* The big-endian AUTODIN II ethernet CRC calculation.
   N.B. Do not use for bulk data, use a table-based routine instead.
   This is common code and should be moved to net/core/crc.c */
static unsigned const ethernet_polynomial = 0x04c11db7U;
static inline uint32 ether_crc(int length, unsigned char *data)
{
    int crc = -1;

    while(--length >= 0) {
		unsigned char current_octet = *data++;
		int bit;
		for (bit = 0; bit < 8; bit++, current_octet >>= 1) {
			crc = (crc << 1) ^
				((crc < 0) ^ (current_octet & 1) ? ethernet_polynomial : 0);
		}
    }
    return crc;
}

static void via_rhine_set_rx_mode(struct device *dev)
{
	struct netdev_private *np = dev->priv;
	long ioaddr = dev->base_addr;
	uint32 mc_filter[2]= {0,0};			/* Multicast hash filter */
	uint8 rx_mode;					/* Note: 0x02=accept runt, 0x01=accept errs */
#if 0
	if (dev->flags & IFF_PROMISC) {			/* Set promiscuous. */
		/* Unconditionally log net taps. */
		printk(KERN_NOTICE "%s: Promiscuous mode enabled.\n", dev->name);
		rx_mode = 0x1C;
	} else if ((dev->mc_count > multicast_filter_limit)
			   ||  (dev->flags & IFF_ALLMULTI)) {
		/* Too many to match, or accept all multicasts. */
		writel(0xffffffff, ioaddr + MulticastFilter0);
		writel(0xffffffff, ioaddr + MulticastFilter1);
		rx_mode = 0x0C;
	} else {
		struct dev_mc_list *mclist;
		int i;
		memset(mc_filter, 0, sizeof(mc_filter));
		for (i = 0, mclist = dev->mc_list; mclist && i < dev->mc_count;
			 i++, mclist = mclist->next) {
			int bit_nr = ether_crc(ETH_ALEN, mclist->dmi_addr) >> 26;

			mc_filter[bit_nr >> 5] |= 1 << (bit_nr & 31);
		}
		writel(mc_filter[0], ioaddr + MulticastFilter0);
		writel(mc_filter[1], ioaddr + MulticastFilter1);
		rx_mode = 0x0C;
	}
	#endif
	writel(mc_filter[0], ioaddr + MulticastFilter0);
	writel(mc_filter[1], ioaddr + MulticastFilter1);
	rx_mode = 0x0C;
	writeb(np->rx_thresh | rx_mode, ioaddr + RxConfig);
}
static int via_rhine_close(struct device *dev)
{
	long ioaddr = dev->base_addr;
	struct netdev_private *np = dev->priv;

	dev->start = 0;
	dev->tbusy = 1;

	spinlock(&np->lock);

	//netif_stop_queue(dev);

	if (debug > 1)
		printk(KERN_DEBUG "%s: Shutting down ethercard, status was %4.4x.\n",
			   dev->name, readw(ioaddr + ChipCmd));

	/* Switch to loopback mode to avoid hardware races. */
	writeb(np->tx_thresh | 0x02, ioaddr + TxConfig);

	/* Disable interrupts by clearing the interrupt mask. */
	writew(0x0000, ioaddr + IntrEnable);

	/* Stop the chip's Tx and Rx processes. */
	writew(CmdStop, ioaddr + ChipCmd);

	spinunlock(&np->lock);

	release_irq(dev->irq, dev->irq_handle);
	free_rbufs(dev);
	free_tbufs(dev);
	free_ring(dev);

	return 0;
}




static status_t via_open_dev( void* pNode, uint32 nFlags, void **pCookie )
{
    return( 0 );
}

static status_t via_close_dev( void* pNode, void* pCookie )
{
    return( 0 );
}

static status_t via_ioctl( void* pNode, void* pCookie, uint32 nCommand, void* pArgs, bool bFromKernel )
{
	struct device* psDev = pNode;
    int nError = 0;

    switch( nCommand )
    {
		case SIOC_ETH_START:
		{
			psDev->packet_queue = pArgs;
			via_rhine_open( psDev );
			psDev->run_timer = true;
			psDev->timer_thread = spawn_kernel_thread( "via_timer", via_rhine_timer, NORMAL_PRIORITY, 0, psDev );
			wakeup_thread( psDev->timer_thread, true );
			break;
		}
		case SIOC_ETH_STOP:
		{
			if ( psDev->run_timer ) {
				psDev->run_timer = false;
				wakeup_thread( psDev->timer_thread, true );
				while( sys_wait_for_thread( psDev->timer_thread ) == -EINTR );
			}
			via_rhine_close( psDev );
			psDev->packet_queue = NULL;
			break;
		}
		case SIOCSIFHWADDR:
			nError = -ENOSYS;
			break;
		case SIOCGIFHWADDR:
			memcpy( ((struct ifreq*)pArgs)->ifr_hwaddr.sa_data, psDev->dev_addr, 6 );
			break;
		default:
			printk( "Error: via_ioctl() unknown command %d\n", nCommand );
			nError = -ENOSYS;
			break;
    }

    return( nError );
}

static int via_read( void* pNode, void* pCookie, off_t nPosition, void* pBuffer, size_t nSize )
{
    return( -ENOSYS );
}

static int via_write( void* pNode, void* pCookie, off_t nPosition, const void* pBuffer, size_t nSize )
{
	struct device* dev = pNode;
	PacketBuf_s* psBuffer = alloc_pkt_buffer( nSize );
	if ( psBuffer != NULL ) {
		memcpy( psBuffer->pb_pData, pBuffer, nSize );
		via_rhine_start_tx( psBuffer, dev );
	}
	return( nSize );
}


static DeviceOperations_s g_sDevOps = {
    via_open_dev,
    via_close_dev,
    via_ioctl,
    via_read,
    via_write,
    NULL,	// dop_readv
    NULL,	// dop_writev
    NULL,	// dop_add_select_req
    NULL	// dop_rem_select_req
};

status_t device_init( int nDeviceID )
{
	int i;
	bool bFound = false;
	int chip_id;
	PCI_Info_s pci;
	/* Get PCI bus */
    g_psBus = get_busmanager( PCI_BUS_NAME, PCI_BUS_VERSION );
    
    if( g_psBus == NULL )
    	return( -1 );
    	
	/* scan all PCI devices */
    for(i = 0 ; g_psBus->get_pci_info( &pci, i ) == 0 ; ++i) {
        if(pci.nVendorID == 0x1106  ) {	
        	chip_id = -1;	
        	if( pci.nDeviceID == 0x6100 ) 
        		chip_id = VT86C100A;
        	else if( pci.nDeviceID == 0x3065 ) 
        		chip_id = VT6102;
        	else if( pci.nDeviceID == 0x3043 ) 
        		chip_id = VT3043;
        	else if( pci.nDeviceID == 0x3106 ) 
        		chip_id = VT6105LOM;
        	else if( pci.nDeviceID == 0x3053 ) 
        		chip_id = VT6105M;
        		
        	if( chip_id > -1 )
        		if( via_rhine_init_one( nDeviceID, pci, chip_id ) == 0 )
        			bFound = true;
        	
        }
    }
    if( !bFound )
    	disable_device( nDeviceID );
	return( bFound ? 0 : -ENODEV );
}

status_t device_uninit( int nDeviceID )
{
    return( 0 );
}
