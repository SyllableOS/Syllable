/* natsemi.c: A Linux PCI Ethernet driver for the NatSemi DP8381x series. */
/*
	Written/copyright 1999-2001 by Donald Becker.
	Portions copyright (c) 2001,2002 Sun Microsystems (thockin@sun.com)
	Portions copyright 2001,2002 Manfred Spraul (manfred@colorfullife.com)

	Port to Syllable copyright 2005 Kristian Van Der Vliet (vanders@liqwyd.com)

	This software may be used and distributed according to the terms of
	the GNU General Public License (GPL), incorporated herein by reference.
	Drivers based on or derived from this code fall under the GPL and must
	retain the authorship, copyright and license notice.  This file is not
	a complete program and may only be used when the entire operating
	system is licensed under the GPL.  License for under other terms may be
	available.  Contact the original author for details.

	The original author may be reached as becker@scyld.com, or at
	Scyld Computing Corporation
	410 Severn Ave., Suite 210
	Annapolis MD 21403

	Support information and updates available at
	http://www.scyld.com/network/netsemi.html


	Linux kernel modifications:

	Version 1.0.1:
		- Spinlock fixes
		- Bug fixes and better intr performance (Tjeerd)
	Version 1.0.2:
		- Now reads correct MAC address from eeprom
	Version 1.0.3:
		- Eliminate redundant priv->tx_full flag
		- Call netif_start_queue from dev->tx_timeout
		- wmb() in start_tx() to flush data
		- Update Tx locking
		- Clean up PCI enable (davej)
	Version 1.0.4:
		- Merge Donald Becker's natsemi.c version 1.07
	Version 1.0.5:
		- { fill me in }
	Version 1.0.6:
		* ethtool support (jgarzik)
		* Proper initialization of the card (which sometimes
		fails to occur and leaves the card in a non-functional
		state). (uzi)

		* Some documented register settings to optimize some
		of the 100Mbit autodetection circuitry in rev C cards. (uzi)

		* Polling of the PHY intr for stuff like link state
		change and auto- negotiation to finally work properly. (uzi)

		* One-liner removal of a duplicate declaration of
		netdev_error(). (uzi)

	Version 1.0.7: (Manfred Spraul)
		* pci dma
		* SMP locking update
		* full reset added into tx_timeout
		* correct multicast hash generation (both big and little endian)
			[copied from a natsemi driver version
			 from Myrio Corporation, Greg Smith]
		* suspend/resume

	version 1.0.8 (Tim Hockin <thockin@sun.com>)
		* ETHTOOL_* support
		* Wake on lan support (Erik Gilling)
		* MXDMA fixes for serverworks
		* EEPROM reload

	version 1.0.9 (Manfred Spraul)
		* Main change: fix lack of synchronize
		netif_close/netif_suspend against a last interrupt
		or packet.
		* do not enable superflous interrupts (e.g. the
		drivers relies on TxDone - TxIntr not needed)
		* wait that the hardware has really stopped in close
		and suspend.
		* workaround for the (at least) gcc-2.95.1 compiler
		problem. Also simplifies the code a bit.
		* disable_irq() in tx_timeout - needed to protect
		against rx interrupts.
		* stop the nic before switching into silent rx mode
		for wol (required according to docu).

	version 1.0.10:
		* use long for ee_addr (various)
		* print pointers properly (DaveM)
		* include asm/irq.h (?)

	version 1.0.11:
		* check and reset if PHY errors appear (Adrian Sun)
		* WoL cleanup (Tim Hockin)
		* Magic number cleanup (Tim Hockin)
		* Don't reload EEPROM on every reset (Tim Hockin)
		* Save and restore EEPROM state across reset (Tim Hockin)
		* MDIO Cleanup (Tim Hockin)
		* Reformat register offsets/bits (jgarzik)

	version 1.0.12:
		* ETHTOOL_* further support (Tim Hockin)

	version 1.0.13:
		* ETHTOOL_[G]EEPROM support (Tim Hockin)

	version 1.0.13:
		* crc cleanup (Matt Domsch <Matt_Domsch@dell.com>)

	version 1.0.14:
		* Cleanup some messages and autoneg in ethtool (Tim Hockin)

	version 1.0.15:
		* Get rid of cable_magic flag
		* use new (National provided) solution for cable magic issue

	version 1.0.16:
		* call netdev_rx() for RxErrors (Manfred Spraul)
		* formatting and cleanups
		* change options and full_duplex arrays to be zero
		  initialized
		* enable only the WoL and PHY interrupts in wol mode

	version 1.0.17:
		* only do cable_magic on 83815 and early 83816 (Tim Hockin)
		* create a function for rx refill (Manfred Spraul)
		* combine drain_ring and init_ring (Manfred Spraul)
		* oom handling (Manfred Spraul)
		* hands_off instead of playing with netif_device_{de,a}ttach
		  (Manfred Spraul)
		* be sure to write the MAC back to the chip (Manfred Spraul)
		* lengthen EEPROM timeout, and always warn about timeouts
		  (Manfred Spraul)
		* comments update (Manfred)
		* do the right thing on a phy-reset (Manfred and Tim)

	TODO:
	* big endian support with CFG:BEM instead of cpu_to_le32
	* support for an external PHY
	* NAPI
*/

#if !defined(__OPTIMIZE__)
#warning  You must compile this file with the correct options!
#warning  See the last lines of the source file.
#error You must compile this driver with "-O".
#endif

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
#include <atheos/bitops.h>

#include <posix/unistd.h>
#include <posix/errno.h>
#include <posix/ioctls.h>
#include <posix/fcntl.h>
#include <posix/signal.h>
#include <net/net.h>
#include <net/ip.h>
#include <net/sockios.h>

#define NO_DEBUG_STUBS 1
#include <atheos/linux_compat.h>

struct net_device
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
    unsigned long   base_addr;  /* device I/O address   */
    unsigned int    irq;        /* device IRQ number    */
    
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

//	struct dev_mc_list	*mc_list;	/* Multicast mac addresses	*/
	int			mc_count;	/* Number of installed mcasts	*/
//	int			promiscuity;
//	int			allmulti;

    NetQueue_s		*packet_queue;
    int irq_handle; /* IRQ handler handle */

	int device_handle; /* device handle from probing */
    
    int node_handle;    /* handle of device node in /dev */
};

#define RX_OFFSET	2

/* Updated to recommendations in pci-skeleton v2.03. */

/* The user-configurable values.
   These may be modified when a driver module is loaded.*/

#define NATSEMI_DEF_MSG		(NETIF_MSG_DRV		| \
				 NETIF_MSG_LINK		| \
				 NETIF_MSG_WOL		| \
				 NETIF_MSG_RX_ERR	| \
				 NETIF_MSG_TX_ERR)

/* Set the copy breakpoint for the copy-only-tiny-frames scheme.
   Setting to > 1518 effectively disables this feature. */
static int rx_copybreak;

/* Used to pass the media type, etc.
   Both 'options[]' and 'full_duplex[]' should exist for driver
   interoperability.
   The media type is usually passed in 'options[]'.
*/
#define MAX_UNITS 8		/* More are supported, limit only on options */
static int options[MAX_UNITS];
static int full_duplex[MAX_UNITS];

/* Operational parameters that are set at compile time. */

/* Keep the ring sizes a power of two for compile efficiency.
   The compiler will convert <unsigned>'%'<2^N> into a bit mask.
   Making the Tx ring too large decreases the effectiveness of channel
   bonding and packet priority.
   There are no ill effects from too-large receive rings. */
#define TX_RING_SIZE	16
#define TX_QUEUE_LEN	10 /* Limit ring entries actually used, min 4. */
#define RX_RING_SIZE	32

/* Operational parameters that usually are not changed. */
/* Time in jiffies before concluding the transmitter is hung. */
#define TX_TIMEOUT  (2*HZ)

#define NATSEMI_HW_TIMEOUT	400
#define NATSEMI_TIMER_FREQ	3*HZ
#define NATSEMI_PG0_NREGS	64
#define NATSEMI_RFDR_NREGS	8
#define NATSEMI_PG1_NREGS	4
#define NATSEMI_NREGS		(NATSEMI_PG0_NREGS + NATSEMI_RFDR_NREGS + \
				 NATSEMI_PG1_NREGS)
#define NATSEMI_REGS_VER	1 /* v1 added RFDR registers */
#define NATSEMI_REGS_SIZE	(NATSEMI_NREGS * sizeof(u32))
#define NATSEMI_EEPROM_SIZE	24 /* 12 16-bit values */

#define PKT_BUF_SZ		1536 /* Size of each temporary Rx buffer. */

static DeviceOperations_s g_sDevOps;

PCI_bus_s* g_psBus;

/*
				Theory of Operation

I. Board Compatibility

This driver is designed for National Semiconductor DP83815 PCI Ethernet NIC.
It also works with other chips in in the DP83810 series.

II. Board-specific settings

This driver requires the PCI interrupt line to be valid.
It honors the EEPROM-set values.

III. Driver operation

IIIa. Ring buffers

This driver uses two statically allocated fixed-size descriptor lists
formed into rings by a branch from the final descriptor to the beginning of
the list.  The ring sizes are set at compile time by RX/TX_RING_SIZE.
The NatSemi design uses a 'next descriptor' pointer that the driver forms
into a list.

IIIb/c. Transmit/Receive Structure

This driver uses a zero-copy receive and transmit scheme.
The driver allocates full frame size skbuffs for the Rx ring buffers at
open() time and passes the skb->data field to the chip as receive data
buffers.  When an incoming frame is less than RX_COPYBREAK bytes long,
a fresh skbuff is allocated and the frame is copied to the new skbuff.
When the incoming frame is larger, the skbuff is passed directly up the
protocol stack.  Buffers consumed this way are replaced by newly allocated
skbuffs in a later phase of receives.

The RX_COPYBREAK value is chosen to trade-off the memory wasted by
using a full-sized skbuff for small frames vs. the copying costs of larger
frames.  New boards are typically used in generously configured machines
and the underfilled buffers have negligible impact compared to the benefit of
a single allocation size, so the default value of zero results in never
copying packets.  When copying is done, the cost is usually mitigated by using
a combined copy/checksum routine.  Copying also preloads the cache, which is
most useful with small frames.

A subtle aspect of the operation is that unaligned buffers are not permitted
by the hardware.  Thus the IP header at offset 14 in an ethernet frame isn't
longword aligned for further processing.  On copies frames are put into the
skbuff at an offset of "+2", 16-byte aligning the IP header.

IIId. Synchronization

Most operations are synchronized on the np->lock irq spinlock, except the
performance critical codepaths:

The rx process only runs in the interrupt handler. Access from outside
the interrupt handler is only permitted after disable_irq().

The rx process usually runs under the dev->xmit_lock. If np->intr_tx_reap
is set, then access is permitted under spin_lock_irq(&np->lock).

Thus configuration functions that want to access everything must call
	disable_irq(dev->irq);
	spin_lock_bh(dev->xmit_lock);
	spin_lock_irq(&np->lock);

IV. Notes

NatSemi PCI network controllers are very uncommon.

IVb. References

http://www.scyld.com/expert/100mbps.html
http://www.scyld.com/expert/NWay.html
Datasheet is available from:
http://www.national.com/pf/DP/DP83815.html

IVc. Errata

None characterised.
*/



enum pcistuff {
	PCI_USES_IO = 0x01,
	PCI_USES_MEM = 0x02,
	PCI_USES_MASTER = 0x04,
	PCI_ADDR0 = 0x08,
	PCI_ADDR1 = 0x10,
};

/* MMIO operations required */
#define PCI_IOTYPE (PCI_USES_MASTER | PCI_USES_MEM | PCI_ADDR1)

/* array of board data directly indexed by pci_tbl[x].driver_data */
static struct {
	const char *name;
	unsigned long flags;
} natsemi_pci_info[] = {
	{ "NatSemi DP8381[56]", PCI_IOTYPE },
};

/* KV: This really should be in pci_vendors.h */
#define PCI_DEVICE_ID_NS_83815 0x0020

/* KV: This should be in pci.h */
#define PCI_PM_CTRL_STATE_MASK	0x0003

/* KV: This should be someplace else, too */
#define MII_ADVERTISE 0x04

static struct pci_device_id natsemi_pci_tbl[] = {
	{ PCI_VENDOR_ID_NS, PCI_DEVICE_ID_NS_83815, PCI_ANY_ID, PCI_ANY_ID, },
	{ 0, },
};

/* Offsets to the device registers.
   Unlike software-only systems, device drivers interact with complex hardware.
   It's not useful to define symbolic names for every register bit in the
   device.
*/
enum register_offsets {
	ChipCmd			= 0x00,
	ChipConfig		= 0x04,
	EECtrl			= 0x08,
	PCIBusCfg		= 0x0C,
	IntrStatus		= 0x10,
	IntrMask		= 0x14,
	IntrEnable		= 0x18,
	IntrHoldoff		= 0x16, /* DP83816 only */
	TxRingPtr		= 0x20,
	TxConfig		= 0x24,
	RxRingPtr		= 0x30,
	RxConfig		= 0x34,
	ClkRun			= 0x3C,
	WOLCmd			= 0x40,
	PauseCmd		= 0x44,
	RxFilterAddr		= 0x48,
	RxFilterData		= 0x4C,
	BootRomAddr		= 0x50,
	BootRomData		= 0x54,
	SiliconRev		= 0x58,
	StatsCtrl		= 0x5C,
	StatsData		= 0x60,
	RxPktErrs		= 0x60,
	RxMissed		= 0x68,
	RxCRCErrs		= 0x64,
	BasicControl		= 0x80,
	BasicStatus		= 0x84,
	AnegAdv			= 0x90,
	AnegPeer		= 0x94,
	PhyStatus		= 0xC0,
	MIntrCtrl		= 0xC4,
	MIntrStatus		= 0xC8,
	PhyCtrl			= 0xE4,

	/* These are from the spec, around page 78... on a separate table.
	 * The meaning of these registers depend on the value of PGSEL. */
	PGSEL			= 0xCC,
	PMDCSR			= 0xE4,
	TSTDAT			= 0xFC,
	DSPCFG			= 0xF4,
	SDCFG			= 0xF8
};
/* the values for the 'magic' registers above (PGSEL=1) */
#define PMDCSR_VAL	0x189c	/* enable preferred adaptation circuitry */
#define TSTDAT_VAL	0x0
#define DSPCFG_VAL	0x5040
#define SDCFG_VAL	0x008c	/* set voltage thresholds for Signal Detect */
#define DSPCFG_LOCK	0x20	/* coefficient lock bit in DSPCFG */
#define TSTDAT_FIXED	0xe8	/* magic number for bad coefficients */

/* misc PCI space registers */
enum pci_register_offsets {
	PCIPM			= 0x44,
};

enum ChipCmd_bits {
	ChipReset		= 0x100,
	RxReset			= 0x20,
	TxReset			= 0x10,
	RxOff			= 0x08,
	RxOn			= 0x04,
	TxOff			= 0x02,
	TxOn			= 0x01,
};

enum ChipConfig_bits {
	CfgPhyDis		= 0x200,
	CfgPhyRst		= 0x400,
	CfgExtPhy		= 0x1000,
	CfgAnegEnable		= 0x2000,
	CfgAneg100		= 0x4000,
	CfgAnegFull		= 0x8000,
	CfgAnegDone		= 0x8000000,
	CfgFullDuplex		= 0x20000000,
	CfgSpeed100		= 0x40000000,
	CfgLink			= 0x80000000,
};

enum EECtrl_bits {
	EE_ShiftClk		= 0x04,
	EE_DataIn		= 0x01,
	EE_ChipSelect		= 0x08,
	EE_DataOut		= 0x02,
};

enum PCIBusCfg_bits {
	EepromReload		= 0x4,
};

/* Bits in the interrupt status/mask registers. */
enum IntrStatus_bits {
	IntrRxDone		= 0x0001,
	IntrRxIntr		= 0x0002,
	IntrRxErr		= 0x0004,
	IntrRxEarly		= 0x0008,
	IntrRxIdle		= 0x0010,
	IntrRxOverrun		= 0x0020,
	IntrTxDone		= 0x0040,
	IntrTxIntr		= 0x0080,
	IntrTxErr		= 0x0100,
	IntrTxIdle		= 0x0200,
	IntrTxUnderrun		= 0x0400,
	StatsMax		= 0x0800,
	SWInt			= 0x1000,
	WOLPkt			= 0x2000,
	LinkChange		= 0x4000,
	IntrHighBits		= 0x8000,
	RxStatusFIFOOver	= 0x10000,
	IntrPCIErr		= 0xf00000,
	RxResetDone		= 0x1000000,
	TxResetDone		= 0x2000000,
	IntrAbnormalSummary	= 0xCD20,
};

/*
 * Default Interrupts:
 * Rx OK, Rx Packet Error, Rx Overrun,
 * Tx OK, Tx Packet Error, Tx Underrun,
 * MIB Service, Phy Interrupt, High Bits,
 * Rx Status FIFO overrun,
 * Received Target Abort, Received Master Abort,
 * Signalled System Error, Received Parity Error
 */
#define DEFAULT_INTR 0x00f1cd65

enum TxConfig_bits {
	TxDrthMask		= 0x3f,
	TxFlthMask		= 0x3f00,
	TxMxdmaMask		= 0x700000,
	TxMxdma_512		= 0x0,
	TxMxdma_4		= 0x100000,
	TxMxdma_8		= 0x200000,
	TxMxdma_16		= 0x300000,
	TxMxdma_32		= 0x400000,
	TxMxdma_64		= 0x500000,
	TxMxdma_128		= 0x600000,
	TxMxdma_256		= 0x700000,
	TxCollRetry		= 0x800000,
	TxAutoPad		= 0x10000000,
	TxMacLoop		= 0x20000000,
	TxHeartIgn		= 0x40000000,
	TxCarrierIgn		= 0x80000000
};

enum RxConfig_bits {
	RxDrthMask		= 0x3e,
	RxMxdmaMask		= 0x700000,
	RxMxdma_512		= 0x0,
	RxMxdma_4		= 0x100000,
	RxMxdma_8		= 0x200000,
	RxMxdma_16		= 0x300000,
	RxMxdma_32		= 0x400000,
	RxMxdma_64		= 0x500000,
	RxMxdma_128		= 0x600000,
	RxMxdma_256		= 0x700000,
	RxAcceptLong		= 0x8000000,
	RxAcceptTx		= 0x10000000,
	RxAcceptRunt		= 0x40000000,
	RxAcceptErr		= 0x80000000
};

enum ClkRun_bits {
	PMEEnable		= 0x100,
	PMEStatus		= 0x8000,
};

enum WolCmd_bits {
	WakePhy			= 0x1,
	WakeUnicast		= 0x2,
	WakeMulticast		= 0x4,
	WakeBroadcast		= 0x8,
	WakeArp			= 0x10,
	WakePMatch0		= 0x20,
	WakePMatch1		= 0x40,
	WakePMatch2		= 0x80,
	WakePMatch3		= 0x100,
	WakeMagic		= 0x200,
	WakeMagicSecure		= 0x400,
	SecureHack		= 0x100000,
	WokePhy			= 0x400000,
	WokeUnicast		= 0x800000,
	WokeMulticast		= 0x1000000,
	WokeBroadcast		= 0x2000000,
	WokeArp			= 0x4000000,
	WokePMatch0		= 0x8000000,
	WokePMatch1		= 0x10000000,
	WokePMatch2		= 0x20000000,
	WokePMatch3		= 0x40000000,
	WokeMagic		= 0x80000000,
	WakeOptsSummary		= 0x7ff
};

enum RxFilterAddr_bits {
	RFCRAddressMask		= 0x3ff,
	AcceptMulticast		= 0x00200000,
	AcceptMyPhys		= 0x08000000,
	AcceptAllPhys		= 0x10000000,
	AcceptAllMulticast	= 0x20000000,
	AcceptBroadcast		= 0x40000000,
	RxFilterEnable		= 0x80000000
};

enum StatsCtrl_bits {
	StatsWarn		= 0x1,
	StatsFreeze		= 0x2,
	StatsClear		= 0x4,
	StatsStrobe		= 0x8,
};

enum MIntrCtrl_bits {
	MICRIntEn		= 0x2,
};

enum PhyCtrl_bits {
	PhyAddrMask		= 0xf,
};

/* values we might find in the silicon revision register */
#define SRR_DP83815_C	0x0302
#define SRR_DP83815_D	0x0403
#define SRR_DP83816_A4	0x0504
#define SRR_DP83816_A5	0x0505

/* The Rx and Tx buffer descriptors. */
/* Note that using only 32 bit fields simplifies conversion to big-endian
   architectures. */
struct netdev_desc {
	u32 next_desc;
	s32 cmd_status;
	u32 addr;
	u32 software_use;
};

/* Bits in network_desc.status */
enum desc_status_bits {
	DescOwn=0x80000000, DescMore=0x40000000, DescIntr=0x20000000,
	DescNoCRC=0x10000000, DescPktOK=0x08000000,
	DescSizeMask=0xfff,

	DescTxAbort=0x04000000, DescTxFIFO=0x02000000,
	DescTxCarrier=0x01000000, DescTxDefer=0x00800000,
	DescTxExcDefer=0x00400000, DescTxOOWCol=0x00200000,
	DescTxExcColl=0x00100000, DescTxCollCount=0x000f0000,

	DescRxAbort=0x04000000, DescRxOver=0x02000000,
	DescRxDest=0x01800000, DescRxLong=0x00400000,
	DescRxRunt=0x00200000, DescRxInvalid=0x00100000,
	DescRxCRC=0x00080000, DescRxAlign=0x00040000,
	DescRxLoop=0x00020000, DesRxColl=0x00010000,
};

struct netdev_private {
	/* Descriptor rings first for alignment */
	dma_addr_t ring_dma;
	struct netdev_desc *rx_ring;
	struct netdev_desc *tx_ring;
	/* The addresses of receive-in-place skbuffs */
	PacketBuf_s *rx_skbuff[RX_RING_SIZE];
	dma_addr_t rx_dma[RX_RING_SIZE];
	/* address of a sent-in-place packet/buffer, for later free() */
	PacketBuf_s *tx_skbuff[TX_RING_SIZE];
	dma_addr_t tx_dma[TX_RING_SIZE];
	/* Media monitoring timer */
	ktimer_t timer;
	/* Frequently used values: keep some adjacent for cache effect */
	PCI_Info_s *pci_dev;
	struct netdev_desc *rx_head_desc;
	/* Producer/consumer ring indices */
	unsigned int cur_rx, dirty_rx;
	unsigned int cur_tx, dirty_tx;
	/* Based on MTU+slack. */
	unsigned int rx_buf_sz;
	int oom;
	/* Do not touch the nic registers */
	int hands_off;
	/* These values are keep track of the transceiver/media in use */
	unsigned int full_duplex;
	/* Rx filter */
	uint32 cur_rx_mode;
	uint32 rx_filter[16];
	/* FIFO and PCI burst thresholds */
	uint32 tx_config, rx_config;
	/* original contents of ClkRun register */
	uint32 SavedClkRun;
	/* silicon revision */
	uint32 srr;
	/* expected DSPCFG value */
	uint16 dspcfg;
	/* MII transceiver section */
	uint16 advertising;
	unsigned int iosize;
	SpinLock_s lock;
	uint32 msg_enable;
	area_id	reg_area;	/* registers */
};

static int eeprom_read(long ioaddr, int location);
static int mdio_read(struct net_device *dev, int phy_id, int reg);
static void natsemi_reset(struct net_device *dev);
static void natsemi_reload_eeprom(struct net_device *dev);
static void natsemi_stop_rxtx(struct net_device *dev);
static int netdev_open(struct net_device *dev);
static void do_cable_magic(struct net_device *dev);
static void undo_cable_magic(struct net_device *dev);
static void check_link(struct net_device *dev);
static void netdev_timer(unsigned long data);
static int alloc_ring(struct net_device *dev);
static void refill_rx(struct net_device *dev);
static void init_ring(struct net_device *dev);
static void drain_tx(struct net_device *dev);
static void drain_ring(struct net_device *dev);
static void free_ring(struct net_device *dev);
static void reinit_ring(struct net_device *dev);
static void init_registers(struct net_device *dev);
static int start_tx(PacketBuf_s *skb, struct net_device *dev);
static int intr_handler(int irq, void *dev_instance, SysCallRegs_s *regs);
static void netdev_error(struct net_device *dev, int intr_status);
static void netdev_rx(struct net_device *dev);
static void netdev_tx_done(struct net_device *dev);
static void __set_rx_mode(struct net_device *dev);
static int netdev_close(struct net_device *dev);

/* Read the EEPROM and MII Management Data I/O (MDIO) interfaces.
   The EEPROM code is for the common 93c06/46 EEPROMs with 6 bit addresses. */

/* Delay between EEPROM clock transitions.
   No extra delay is needed with 33Mhz PCI, but future 66Mhz access may need
   a delay.  Note that pre-2.0.34 kernels had a cache-alignment bug that
   made udelay() unreliable.
   The old method of using an ISA access as a delay, __SLOW_DOWN_IO__, is
   depricated.
*/
#define eeprom_delay(ee_addr)	readl(ee_addr)

#define EE_Write0 (EE_ChipSelect)
#define EE_Write1 (EE_ChipSelect | EE_DataIn)

/* The EEPROM commands include the alway-set leading bit. */
enum EEPROM_Cmds {
	EE_WriteCmd=(5 << 6), EE_ReadCmd=(6 << 6), EE_EraseCmd=(7 << 6),
};

static int eeprom_read(long addr, int location)
{
	int i;
	int retval = 0;
	long ee_addr = addr + EECtrl;
	int read_cmd = location | EE_ReadCmd;
	writel(EE_Write0, ee_addr);

	/* Shift the read command bits out. */
	for (i = 10; i >= 0; i--) {
		short dataval = (read_cmd & (1 << i)) ? EE_Write1 : EE_Write0;
		writel(dataval, ee_addr);
		eeprom_delay(ee_addr);
		writel(dataval | EE_ShiftClk, ee_addr);
		eeprom_delay(ee_addr);
	}
	writel(EE_ChipSelect, ee_addr);
	eeprom_delay(ee_addr);

	for (i = 0; i < 16; i++) {
		writel(EE_ChipSelect | EE_ShiftClk, ee_addr);
		eeprom_delay(ee_addr);
		retval |= (readl(ee_addr) & EE_DataOut) ? 1 << i : 0;
		writel(EE_ChipSelect, ee_addr);
		eeprom_delay(ee_addr);
	}

	/* Terminate the EEPROM access. */
	writel(EE_Write0, ee_addr);
	writel(0, ee_addr);
	return retval;
}

/* MII transceiver control section.
 * The 83815 series has an internal transceiver, and we present the
 * management registers as if they were MII connected. */

static int mdio_read(struct net_device *dev, int phy_id, int reg)
{
	if (phy_id == 1 && reg < 32)
		return readl(dev->base_addr+BasicControl+(reg<<2))&0xffff;
	else
		return 0xffff;
}

/* CFG bits [13:16] [18:23] */
#define CFG_RESET_SAVE 0xfde000
/* WCSR bits [0:4] [9:10] */
#define WCSR_RESET_SAVE 0x61f
/* RFCR bits [20] [22] [27:31] */
#define RFCR_RESET_SAVE 0xf8500000;

static void natsemi_reset(struct net_device *dev)
{
	int i;
	u32 cfg;
	u32 wcsr;
	u32 rfcr;
	u16 pmatch[3];
	u16 sopass[3];

	/*
	 * Resetting the chip causes some registers to be lost.
	 * Natsemi suggests NOT reloading the EEPROM while live, so instead
	 * we save the state that would have been loaded from EEPROM
	 * on a normal power-up (see the spec EEPROM map).  This assumes
	 * whoever calls this will follow up with init_registers() eventually.
	 */

	/* CFG */
	cfg = readl(dev->base_addr + ChipConfig) & CFG_RESET_SAVE;
	/* WCSR */
	wcsr = readl(dev->base_addr + WOLCmd) & WCSR_RESET_SAVE;
	/* RFCR */
	rfcr = readl(dev->base_addr + RxFilterAddr) & RFCR_RESET_SAVE;
	/* PMATCH */
	for (i = 0; i < 3; i++) {
		writel(i*2, dev->base_addr + RxFilterAddr);
		pmatch[i] = readw(dev->base_addr + RxFilterData);
	}
	/* SOPAS */
	for (i = 0; i < 3; i++) {
		writel(0xa+(i*2), dev->base_addr + RxFilterAddr);
		sopass[i] = readw(dev->base_addr + RxFilterData);
	}

	/* now whack the chip */
	writel(ChipReset, dev->base_addr + ChipCmd);
	for (i=0;i<NATSEMI_HW_TIMEOUT;i++) {
		if (!(readl(dev->base_addr + ChipCmd) & ChipReset))
			break;
		udelay(5);
	}
	if (i==NATSEMI_HW_TIMEOUT) {
		kerndbg(KERN_WARNING,"%s: reset did not complete in %d usec.\n",
			dev->name, i*5);
	}

	/* restore CFG */
	cfg |= readl(dev->base_addr + ChipConfig) & ~CFG_RESET_SAVE;
	writel(cfg, dev->base_addr + ChipConfig);
	/* restore WCSR */
	wcsr |= readl(dev->base_addr + WOLCmd) & ~WCSR_RESET_SAVE;
	writel(wcsr, dev->base_addr + WOLCmd);
	/* read RFCR */
	rfcr |= readl(dev->base_addr + RxFilterAddr) & ~RFCR_RESET_SAVE;
	/* restore PMATCH */
	for (i = 0; i < 3; i++) {
		writel(i*2, dev->base_addr + RxFilterAddr);
		writew(pmatch[i], dev->base_addr + RxFilterData);
	}
	for (i = 0; i < 3; i++) {
		writel(0xa+(i*2), dev->base_addr + RxFilterAddr);
		writew(sopass[i], dev->base_addr + RxFilterData);
	}
	/* restore RFCR */
	writel(rfcr, dev->base_addr + RxFilterAddr);
}

static void natsemi_reload_eeprom(struct net_device *dev)
{
	int i;

	writel(EepromReload, dev->base_addr + PCIBusCfg);
	for (i=0;i<NATSEMI_HW_TIMEOUT;i++) {
		udelay(50);
		if (!(readl(dev->base_addr + PCIBusCfg) & EepromReload))
			break;
	}
	if (i==NATSEMI_HW_TIMEOUT) {
		kerndbg(KERN_WARNING,"%s: EEPROM did not reload in %d usec.\n",
			dev->name, i*50);
	}
}

static void natsemi_stop_rxtx(struct net_device *dev)
{
	long ioaddr = dev->base_addr;
	int i;

	writel(RxOff | TxOff, ioaddr + ChipCmd);
	for(i=0;i< NATSEMI_HW_TIMEOUT;i++) {
		if ((readl(ioaddr + ChipCmd) & (TxOn|RxOn)) == 0)
			break;
		udelay(5);
	}
	if (i==NATSEMI_HW_TIMEOUT) {
		kerndbg(KERN_WARNING,"%s: Tx/Rx process did not stop in %d usec.\n",
			dev->name, i*5);
	}
}

static int netdev_open(struct net_device *dev)
{
	struct netdev_private *np = dev->priv;
	long ioaddr = dev->base_addr;
	int i;

	/* Reset the chip, just in case. */
	natsemi_reset(dev);

	if ((dev->irq_handle = request_irq(dev->irq, &intr_handler, NULL,
			SA_SHIRQ, dev->name, dev)) < 0) {
		return -EAGAIN;
	}

	kerndbg(KERN_DEBUG,"%s: netdev_open() irq %d.\n",
		dev->name, dev->irq);

	i = alloc_ring(dev);
	if (i < 0) {
		release_irq(dev->irq, dev->irq_handle);
		return i;
	}
	init_ring(dev);
	spin_lock_irq(&np->lock);
	init_registers(dev);
	/* now set the MAC address according to dev->dev_addr */
	for (i = 0; i < 3; i++) {
		u16 mac = (dev->dev_addr[2*i+1]<<8) + dev->dev_addr[2*i];

		writel(i*2, ioaddr + RxFilterAddr);
		writew(mac, ioaddr + RxFilterData);
	}
	writel(np->cur_rx_mode, ioaddr + RxFilterAddr);
	spin_unlock_irq(&np->lock);

	netif_start_queue(dev);

	kerndbg(KERN_DEBUG,"%s: Done netdev_open(), status: %#08x.\n",
		dev->name, (int)readl(ioaddr + ChipCmd));

	np->timer = create_timer();
	start_timer(np->timer, (timer_callback *) &netdev_timer, dev, 2000000, true );

	return 0;
}

static void do_cable_magic(struct net_device *dev)
{
	struct netdev_private *np = dev->priv;

	if (np->srr >= SRR_DP83816_A5)
		return;

	/*
	 * 100 MBit links with short cables can trip an issue with the chip.
	 * The problem manifests as lots of CRC errors and/or flickering
	 * activity LED while idle.  This process is based on instructions
	 * from engineers at National.
	 */
	if (readl(dev->base_addr + ChipConfig) & CfgSpeed100) {
		u16 data;

		writew(1, dev->base_addr + PGSEL);
		/*
		 * coefficient visibility should already be enabled via
		 * DSPCFG | 0x1000
		 */
		data = readw(dev->base_addr + TSTDAT) & 0xff;
		/*
		 * the value must be negative, and within certain values
		 * (these values all come from National)
		 */
		if (!(data & 0x80) || ((data >= 0xd8) && (data <= 0xff))) {
			struct netdev_private *np = dev->priv;

			/* the bug has been triggered - fix the coefficient */
			writew(TSTDAT_FIXED, dev->base_addr + TSTDAT);
			/* lock the value */
			data = readw(dev->base_addr + DSPCFG);
			np->dspcfg = data | DSPCFG_LOCK;
			writew(np->dspcfg, dev->base_addr + DSPCFG);
		}
		writew(0, dev->base_addr + PGSEL);
	}
}

static void undo_cable_magic(struct net_device *dev)
{
	u16 data;
	struct netdev_private *np = dev->priv;

	if (np->srr >= SRR_DP83816_A5)
		return;

	writew(1, dev->base_addr + PGSEL);
	/* make sure the lock bit is clear */
	data = readw(dev->base_addr + DSPCFG);
	np->dspcfg = data & ~DSPCFG_LOCK;
	writew(np->dspcfg, dev->base_addr + DSPCFG);
	writew(0, dev->base_addr + PGSEL);
}

static void check_link(struct net_device *dev)
{
	struct netdev_private *np = dev->priv;
	long ioaddr = dev->base_addr;
	int duplex;
	int chipcfg = readl(ioaddr + ChipConfig);

	if (!(chipcfg & CfgLink)) {
		if (netif_carrier_ok(dev)) {
			kerndbg(KERN_DEBUG,"%s: link down.\n",
				dev->name);
			netif_carrier_off(dev);
			undo_cable_magic(dev);
		}
		return;
	}

	if (!netif_carrier_ok(dev)) {
		kerndbg(KERN_DEBUG,"%s: link up.\n", dev->name);
		netif_carrier_on(dev);
		do_cable_magic(dev);
	}
	duplex = np->full_duplex || (chipcfg & CfgFullDuplex ? 1 : 0);

	/* if duplex is set then bit 28 must be set, too */
	if (duplex ^ !!(np->rx_config & RxAcceptTx)) {
		if (duplex) {
			np->rx_config |= RxAcceptTx;
			np->tx_config |= TxCarrierIgn | TxHeartIgn;
		} else {
			np->rx_config &= ~RxAcceptTx;
			np->tx_config &= ~(TxCarrierIgn | TxHeartIgn);
		}
		writel(np->tx_config, ioaddr + TxConfig);
		writel(np->rx_config, ioaddr + RxConfig);
	}
}

static void init_registers(struct net_device *dev)
{
	struct netdev_private *np = dev->priv;
	long ioaddr = dev->base_addr;
	int i;

	for (i=0;i<NATSEMI_HW_TIMEOUT;i++) {
		if (readl(dev->base_addr + ChipConfig) & CfgAnegDone)
			break;
		udelay(10);
	}

	/* On page 78 of the spec, they recommend some settings for "optimum
	   performance" to be done in sequence.  These settings optimize some
	   of the 100Mbit autodetection circuitry.  They say we only want to
	   do this for rev C of the chip, but engineers at NSC (Bradley
	   Kennedy) recommends always setting them.  If you don't, you get
	   errors on some autonegotiations that make the device unusable.
	*/
	writew(1, ioaddr + PGSEL);
	writew(PMDCSR_VAL, ioaddr + PMDCSR);
	writew(TSTDAT_VAL, ioaddr + TSTDAT);
	writew(DSPCFG_VAL, ioaddr + DSPCFG);
	writew(SDCFG_VAL, ioaddr + SDCFG);
	writew(0, ioaddr + PGSEL);
	np->dspcfg = DSPCFG_VAL;

	/* Enable PHY Specific event based interrupts.  Link state change
	   and Auto-Negotiation Completion are among the affected.
	   Read the intr status to clear it (needed for wake events).
	*/
	readw(ioaddr + MIntrStatus);
	writew(MICRIntEn, ioaddr + MIntrCtrl);

	/* clear any interrupts that are pending, such as wake events */
	readl(ioaddr + IntrStatus);

	writel(np->ring_dma, ioaddr + RxRingPtr);
	writel(np->ring_dma + RX_RING_SIZE * sizeof(struct netdev_desc),
		ioaddr + TxRingPtr);

	/* Initialize other registers.
	 * Configure the PCI bus bursts and FIFO thresholds.
	 * Configure for standard, in-spec Ethernet.
	 * Start with half-duplex. check_link will update
	 * to the correct settings.
	 */

	/* DRTH: 2: start tx if 64 bytes are in the fifo
	 * FLTH: 0x10: refill with next packet if 512 bytes are free
	 * MXDMA: 0: up to 256 byte bursts.
	 * 	MXDMA must be <= FLTH
	 * ECRETRY=1
	 * ATP=1
	 */
	np->tx_config = TxAutoPad | TxCollRetry | TxMxdma_256 | (0x1002);
	writel(np->tx_config, ioaddr + TxConfig);

	/* DRTH 0x10: start copying to memory if 128 bytes are in the fifo
	 * MXDMA 0: up to 256 byte bursts
	 */
	np->rx_config = RxMxdma_256 | 0x20;
	writel(np->rx_config, ioaddr + RxConfig);

	/* Disable PME:
	 * The PME bit is initialized from the EEPROM contents.
	 * PCI cards probably have PME disabled, but motherboard
	 * implementations may have PME set to enable WakeOnLan.
	 * With PME set the chip will scan incoming packets but
	 * nothing will be written to memory. */
	np->SavedClkRun = readl(ioaddr + ClkRun);
	writel(np->SavedClkRun & ~PMEEnable, ioaddr + ClkRun);

	check_link(dev);
	__set_rx_mode(dev);

	/* Enable interrupts by setting the interrupt mask. */
	writel(DEFAULT_INTR, ioaddr + IntrMask);
	writel(1, ioaddr + IntrEnable);

	writel(RxOn | TxOn, ioaddr + ChipCmd);
	writel(StatsClear, ioaddr + StatsCtrl); /* Clear Stats */
}

/*
 * netdev_timer:
 * Purpose:
 * 1) check for link changes. Usually they are handled by the MII interrupt
 *    but it doesn't hurt to check twice.
 * 2) check for sudden death of the NIC:
 *    It seems that a reference set for this chip went out with incorrect info,
 *    and there exist boards that aren't quite right.  An unexpected voltage
 *    drop can cause the PHY to get itself in a weird state (basically reset).
 *    NOTE: this only seems to affect revC chips.
 * 3) check of death of the RX path due to OOM
 */
static void netdev_timer(unsigned long data)
{
	struct net_device *dev = (struct net_device *)data;
	struct netdev_private *np = dev->priv;
	long ioaddr = dev->base_addr;
	u16 dspcfg;

	spin_lock_irq(&np->lock);

	/* check for a nasty random phy-reset - use dspcfg as a flag */
	writew(1, ioaddr+PGSEL);
	dspcfg = readw(ioaddr+DSPCFG);
	writew(0, ioaddr+PGSEL);
	if (dspcfg != np->dspcfg) {
		if (!netif_queue_stopped(dev)) {
			spin_unlock_irq(&np->lock);
			kerndbg(KERN_INFO,"%s: possible phy reset: "
				"re-initializing\n", dev->name);

			disable_irq_nosync(dev->irq);
			spin_lock_irq(&np->lock);
			natsemi_stop_rxtx(dev);
			reinit_ring(dev);
			init_registers(dev);
			spin_unlock_irq(&np->lock);
			enable_irq(dev->irq);
		} else {
			/* hurry back */
			spin_unlock_irq(&np->lock);
		}
	} else {
		/* init_registers() calls check_link() for the above case */
		check_link(dev);
		spin_unlock_irq(&np->lock);
	}
	if (np->oom) {
		disable_irq_nosync(dev->irq);
		np->oom = 0;
		refill_rx(dev);
		enable_irq(dev->irq);
		if (!np->oom)
			writel(RxOn, dev->base_addr + ChipCmd);
	}
	start_timer( np->timer, (timer_callback *) &netdev_timer,
		dev, 2000000, true );
}

static int alloc_ring(struct net_device *dev)
{
	struct netdev_private *np = dev->priv;
	np->rx_ring = pci_alloc_consistent(np->pci_dev,
		sizeof(struct netdev_desc) * (RX_RING_SIZE+TX_RING_SIZE),
		&np->ring_dma);
	if (!np->rx_ring)
		return -ENOMEM;
	np->tx_ring = &np->rx_ring[RX_RING_SIZE];
	return 0;
}

static void refill_rx(struct net_device *dev)
{
	struct netdev_private *np = dev->priv;

	/* Refill the Rx ring buffers. */
	for (; np->cur_rx - np->dirty_rx > 0; np->dirty_rx++) {
		PacketBuf_s *skb;
		int entry = np->dirty_rx % RX_RING_SIZE;
		if (np->rx_skbuff[entry] == NULL) {
			unsigned int buflen = np->rx_buf_sz + RX_OFFSET;
			skb = alloc_pkt_buffer(buflen);
			np->rx_skbuff[entry] = skb;
			if (skb == NULL)
				break; /* Better luck next round. */
			skb->pb_nSize = 0;
			np->rx_dma[entry] = (uint32)skb->pb_pData;
			np->rx_ring[entry].addr = np->rx_dma[entry];
		}
		np->rx_ring[entry].cmd_status = cpu_to_le32(np->rx_buf_sz);
	}
	if (np->cur_rx - np->dirty_rx == RX_RING_SIZE) {
		np->oom = 1;
	}
}

/* Initialize the Rx and Tx rings, along with various 'dev' bits. */
static void init_ring(struct net_device *dev)
{
	struct netdev_private *np = dev->priv;
	int i;

	/* 1) TX ring */
	np->dirty_tx = np->cur_tx = 0;
	for (i = 0; i < TX_RING_SIZE; i++) {
		np->tx_skbuff[i] = NULL;
		np->tx_ring[i].next_desc = cpu_to_le32(np->ring_dma
			+sizeof(struct netdev_desc)
			*((i+1)%TX_RING_SIZE+RX_RING_SIZE));
		np->tx_ring[i].cmd_status = 0;
	}

	/* 2) RX ring */
	np->dirty_rx = 0;
	np->cur_rx = RX_RING_SIZE;
	np->rx_buf_sz = (dev->mtu <= 1500 ? PKT_BUF_SZ : dev->mtu + 32);
	np->oom = 0;
	np->rx_head_desc = &np->rx_ring[0];

	/* Please be carefull before changing this loop - at least gcc-2.95.1
	 * miscompiles it otherwise.
	 */
	/* Initialize all Rx descriptors. */
	for (i = 0; i < RX_RING_SIZE; i++) {
		np->rx_ring[i].next_desc = cpu_to_le32(np->ring_dma
				+sizeof(struct netdev_desc)
				*((i+1)%RX_RING_SIZE));
		np->rx_ring[i].cmd_status = cpu_to_le32(DescOwn);
		np->rx_skbuff[i] = NULL;
	}
	refill_rx(dev);
}

static void drain_tx(struct net_device *dev)
{
	struct netdev_private *np = dev->priv;
	int i;

	for (i = 0; i < TX_RING_SIZE; i++) {
		if (np->tx_skbuff[i]) {
			free_pkt_buffer(np->tx_skbuff[i]);
		}
		np->tx_skbuff[i] = NULL;
	}
}

static void drain_ring(struct net_device *dev)
{
	struct netdev_private *np = dev->priv;
	int i;

	/* Free all the skbuffs in the Rx queue. */
	for (i = 0; i < RX_RING_SIZE; i++) {
		np->rx_ring[i].cmd_status = 0;
		np->rx_ring[i].addr = 0xBADF00D0; /* An invalid address. */
		if (np->rx_skbuff[i]) {
			free_pkt_buffer(np->rx_skbuff[i]);
		}
		np->rx_skbuff[i] = NULL;
	}
	drain_tx(dev);
}

static void free_ring(struct net_device *dev)
{
	struct netdev_private *np = dev->priv;
	pci_free_consistent(np->pci_dev,
		sizeof(struct netdev_desc) * (RX_RING_SIZE+TX_RING_SIZE),
		np->rx_ring, np->ring_dma);
}

static void reinit_ring(struct net_device *dev)
{
	struct netdev_private *np = dev->priv;
	int i;

	/* drain TX ring */
	drain_tx(dev);
	np->dirty_tx = np->cur_tx = 0;
	for (i=0;i<TX_RING_SIZE;i++)
		np->tx_ring[i].cmd_status = 0;

	/* RX Ring */
	np->dirty_rx = 0;
	np->cur_rx = RX_RING_SIZE;
	np->rx_head_desc = &np->rx_ring[0];
	/* Initialize all Rx descriptors. */
	for (i = 0; i < RX_RING_SIZE; i++)
		np->rx_ring[i].cmd_status = cpu_to_le32(DescOwn);

	refill_rx(dev);
}

static int start_tx(PacketBuf_s *skb, struct net_device *dev)
{
	struct netdev_private *np = dev->priv;
	unsigned entry;

	/* Note: Ordering is important here, set the field with the
	   "ownership" bit last, and only then increment cur_tx. */

	/* Calculate the next Tx descriptor entry. */
	entry = np->cur_tx % TX_RING_SIZE;

	np->tx_skbuff[entry] = skb;
	np->tx_dma[entry] = (uint32)skb->pb_pData;
	np->tx_ring[entry].addr = np->tx_dma[entry];

	spin_lock_irq(&np->lock);

	if (!np->hands_off) {
		np->tx_ring[entry].cmd_status = cpu_to_le32(DescOwn | skb->pb_nSize);
		np->cur_tx++;
		if (np->cur_tx - np->dirty_tx >= TX_QUEUE_LEN - 1) {
			netdev_tx_done(dev);
			if (np->cur_tx - np->dirty_tx >= TX_QUEUE_LEN - 1)
				netif_stop_queue(dev);
		}
		/* Wake the potentially-idle transmit channel. */
		writel(TxOn, dev->base_addr + ChipCmd);
	} else {
		free_pkt_buffer(skb);
	}
	spin_unlock_irq(&np->lock);

	dev->trans_start = jiffies;
	kerndbg(KERN_DEBUG_LOW,"%s: Transmit frame #%d queued in slot %d.\n",
		dev->name, np->cur_tx, entry);

	return 0;
}

static void netdev_tx_done(struct net_device *dev)
{
	struct netdev_private *np = dev->priv;

	for (; np->cur_tx - np->dirty_tx > 0; np->dirty_tx++) {
		int entry = np->dirty_tx % TX_RING_SIZE;
		if (np->tx_ring[entry].cmd_status & cpu_to_le32(DescOwn))
			break;

		/* Free the original skb. */
		free_pkt_buffer(np->tx_skbuff[entry]);
		np->tx_skbuff[entry] = NULL;
	}
	if (netif_queue_stopped(dev)
		&& np->cur_tx - np->dirty_tx < TX_QUEUE_LEN - 4) {
		/* The ring is no longer full, wake queue. */
		netif_wake_queue(dev);
	}
}

/* The interrupt handler does all of the Rx thread work and cleans up
   after the Tx thread. */
static int intr_handler(int irq, void *dev_instance, SysCallRegs_s *rgs)
{
	struct net_device *dev = dev_instance;
	struct netdev_private *np = dev->priv;
	long ioaddr = dev->base_addr;
	unsigned int handled = 0;

	if (np->hands_off)
		return 0;

	do {
		/* Reading automatically acknowledges all int sources. */
		u32 intr_status = readl(ioaddr + IntrStatus);

		kerndbg(KERN_DEBUG_LOW,
			"%s: Interrupt, status %#08lx, mask %#08x.\n",
			dev->name, intr_status,
			readl(ioaddr + IntrMask));

		if (intr_status == 0)
			break;
		handled = 1;

		if (intr_status &
		   (IntrRxDone | IntrRxIntr | RxStatusFIFOOver |
		    IntrRxErr | IntrRxOverrun)) {
			netdev_rx(dev);
		}

		if (intr_status &
		   (IntrTxDone | IntrTxIntr | IntrTxIdle | IntrTxErr)) {
			spin_lock(&np->lock);
			netdev_tx_done(dev);
			spin_unlock(&np->lock);
		}

		/* Abnormal error summary/uncommon events handlers. */
		if (intr_status & IntrAbnormalSummary)
			netdev_error(dev, intr_status);

	} while (1);

	kerndbg(KERN_DEBUG_LOW, "%s: exiting interrupt.\n", dev->name);
	return 0;
}

/* This routine is logically part of the interrupt handler, but separated
   for clarity and better register allocation. */
static void netdev_rx(struct net_device *dev)
{
	struct netdev_private *np = dev->priv;
	int entry = np->cur_rx % RX_RING_SIZE;
	int boguscnt = np->dirty_rx + RX_RING_SIZE - np->cur_rx;
	s32 desc_status = le32_to_cpu(np->rx_head_desc->cmd_status);

	/* If the driver owns the next entry it's a new packet. Send it up. */
	while (desc_status < 0) { /* e.g. & DescOwn */
		kerndbg(KERN_DEBUG_LOW,
			"  netdev_rx() entry %d status was %#08lx.\n",
			entry, desc_status);

		if (--boguscnt < 0)
			break;
		if ((desc_status&(DescMore|DescPktOK|DescRxLong)) != DescPktOK){
			if (desc_status & DescMore) {
				kerndbg(KERN_WARNING,
					"%s: Oversized(?) Ethernet "
					"frame spanned multiple "
					"buffers, entry %#08x "
					"status %#08lx.\n", dev->name,
					np->cur_rx, desc_status);
			}
		} else {
			PacketBuf_s *skb;
			/* Omit CRC size. */
			int pkt_len = (desc_status & DescSizeMask) - 4;
			/* Check if the packet is long enough to accept
			 * without copying to a minimally-sized skbuff. */
			if (pkt_len < rx_copybreak
			    && (skb = alloc_pkt_buffer(pkt_len + RX_OFFSET)) != NULL) {
				skb->pb_nSize = 0;
				memcpy(skb_put(skb, pkt_len), np->rx_skbuff[entry]->pb_pData,
					   pkt_len);
			} else {
				skb = np->rx_skbuff[entry];
				if (skb == NULL) {
					kerndbg(KERN_DEBUG, "%s: Inconsistent Rx descriptor chain.\n",
						   dev->name);
					break;
				}
				np->rx_skbuff[entry] = NULL;
				skb_put(skb, pkt_len);
			}

			if ( dev->packet_queue != NULL ) {
				skb->pb_uMacHdr.pRaw = skb->pb_pData;
				enqueue_packet( dev->packet_queue, skb );
			} else {
				kerndbg( KERN_DEBUG, "Error: netdev_rx() received packet to downed device!\n" );
				free_pkt_buffer( skb );
			}

			dev->last_rx = jiffies;
		}
		entry = (++np->cur_rx) % RX_RING_SIZE;
		np->rx_head_desc = &np->rx_ring[entry];
		desc_status = le32_to_cpu(np->rx_head_desc->cmd_status);
	}
	refill_rx(dev);

	/* Restart Rx engine if stopped. */
	if (np->oom)
		start_timer( np->timer, (timer_callback *) &netdev_timer,
			dev, 2000000, true );
	else
		writel(RxOn, dev->base_addr + ChipCmd);
}

static void netdev_error(struct net_device *dev, int intr_status)
{
	struct netdev_private *np = dev->priv;
	long ioaddr = dev->base_addr;

	spin_lock(&np->lock);
	if (intr_status & LinkChange) {
		/* read MII int status to clear the flag */
		readw(ioaddr + MIntrStatus);
		check_link(dev);
	}
	if (intr_status & IntrTxUnderrun) {
		if ((np->tx_config & TxDrthMask) < 62)
			np->tx_config += 2;
		writel(np->tx_config, ioaddr + TxConfig);
	}

	spin_unlock(&np->lock);
}

#define HASH_TABLE	0x200
static void __set_rx_mode(struct net_device *dev)
{
	long ioaddr = dev->base_addr;
	struct netdev_private *np = dev->priv;
	u32 rx_mode = 0;

#if 0
	if (dev->flags & IFF_PROMISC) { /* Set promiscuous. */
		/* Unconditionally log net taps. */
		kerndbg(KERN_INFO, "%s: Promiscuous mode enabled.\n",
			dev->name);
		rx_mode = RxFilterEnable | AcceptBroadcast
			| AcceptAllMulticast | AcceptAllPhys | AcceptMyPhys;
	} else if ((dev->mc_count > multicast_filter_limit)
	  || (dev->flags & IFF_ALLMULTI)) {
#else
	{
#endif
		rx_mode = RxFilterEnable | AcceptBroadcast
			| AcceptAllMulticast | AcceptMyPhys;
	}

	writel(rx_mode, ioaddr + RxFilterAddr);
	np->cur_rx_mode = rx_mode;
}

/* Device interface */

static status_t dp83815_open_dev( void* pNode, uint32 nFlags, void **pCookie )
{
    return( 0 );
}

static status_t dp83815_close_dev( void* pNode, void* pCookie )
{
    return( 0 );
}

static status_t dp83815_ioctl( void* pNode, void* pCookie, uint32 nCommand, void* pArgs, bool bFromKernel )
{
	struct net_device* psDev = pNode;
    int nError = 0;

    switch( nCommand )
    {
		case SIOC_ETH_START:
		{
			psDev->packet_queue = pArgs;
			netdev_open( psDev );
			break;
		}
		case SIOC_ETH_STOP:
		{
			netdev_close( psDev );
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
			kerndbg( KERN_WARNING, "Error: netdev_ioctl() unknown command %d\n", (int)nCommand );
			nError = -ENOSYS;
			break;
    }

    return( nError );
}

static int dp83815_read( void* pNode, void* pCookie, off_t nPosition, void* pBuffer, size_t nSize )
{
    return( -ENOSYS );
}

static int dp83815_write( void* pNode, void* pCookie, off_t nPosition, const void* pBuffer, size_t nSize )
{
	struct net_device* dev = pNode;
	PacketBuf_s* psBuffer = alloc_pkt_buffer( nSize );
	if ( psBuffer != NULL ) {
		memcpy( psBuffer->pb_pData, pBuffer, nSize );
		start_tx( psBuffer, dev );
	}
	return( nSize );
}

static int netdev_close(struct net_device *dev)
{
	long ioaddr = dev->base_addr;
	struct netdev_private *np = dev->priv;

	kerndbg(KERN_DEBUG,
		"%s: Shutting down ethercard, status was %#04x.\n",
		dev->name, (int)readl(ioaddr + ChipCmd));
	kerndbg(KERN_DEBUG, 
		"%s: Queue pointers were Tx %d / %d,  Rx %d / %d.\n",
		dev->name, np->cur_tx, np->dirty_tx,
		np->cur_rx, np->dirty_rx);

	/*
	 * FIXME: what if someone tries to close a device
	 * that is suspended?
	 * Should we reenable the nic to switch to
	 * the final WOL settings?
	 */

	delete_timer(np->timer);
	release_irq(dev->irq, dev->irq_handle);
	spin_lock_irq(&np->lock);
	/* Disable interrupts, and flush posted writes */
	writel(0, ioaddr + IntrEnable);
	readl(ioaddr + IntrEnable);
	np->hands_off = 1;
	spin_unlock_irq(&np->lock);

	/* Interrupt disabled, interrupt handler released,
	 * queue stopped, timer deleted, rtnl_lock held
	 * All async codepaths that access the driver are disabled.
	 */
	spin_lock_irq(&np->lock);
	np->hands_off = 0;
	readl(ioaddr + IntrMask);
	readw(ioaddr + MIntrStatus);

	/* Freeze Stats */
	writel(StatsFreeze, ioaddr + StatsCtrl);

	/* Stop the chip's Tx and Rx processes. */
	natsemi_stop_rxtx(dev);

	spin_unlock_irq(&np->lock);

	/* clear the carrier last - an interrupt could reenable it otherwise */
	netif_carrier_off(dev);
	netif_stop_queue(dev);

	drain_ring(dev);
	free_ring(dev);

	/* Restore PME enable bit unmolested */
	writel(np->SavedClkRun, ioaddr + ClkRun);

	return 0;
}

static DeviceOperations_s g_sDevOps = {
    dp83815_open_dev,
    dp83815_close_dev,
    dp83815_ioctl,
    dp83815_read,
    dp83815_write,
    NULL,	// dop_readv
    NULL,	// dop_writev
    NULL,	// dop_add_select_req
    NULL	// dop_rem_select_req
};

static int dp83815_probe (int nDeviceID)
{
	struct net_device *dev;
	struct netdev_private *np = NULL;
	int cards_found = 0;
	int prev_eedata;

    int i,j;
    PCI_Info_s sInfo;
   
    for ( i = 0 ; g_psBus->get_pci_info( &sInfo, i ) == 0 ; ++i )
	{
		int chip_idx;
		for ( chip_idx = 0 ; natsemi_pci_tbl[chip_idx].vendor_id ; chip_idx++ ) {
			if ( sInfo.nVendorID == natsemi_pci_tbl[chip_idx].vendor_id &&
				 sInfo.nDeviceID == natsemi_pci_tbl[chip_idx].device_id)
			{
				char node_path[64];
				int pci_command;
				int new_command;
				int tmp;
				int option;
				long addr;
				void *reg_base = NULL;

				if( claim_device( nDeviceID, sInfo.nHandle, "National Semiconductor DP83815", DEVICE_NET ) != 0 )
					continue;

				dev = kmalloc( sizeof(*dev), MEMF_KERNEL | MEMF_CLEAR );
				if (!dev)
					continue;
				dev->name = "dp83815";

				if ((dev->priv = kmalloc(sizeof(struct netdev_private), MEMF_KERNEL)) == NULL)
					continue;
				np = dev->priv;

				addr = sInfo.u.h0.nBase1 & PCI_ADDRESS_MEMORY_32_MASK;
				dev->irq = sInfo.u.h0.nInterruptLine;
				dev->device_handle = nDeviceID;

				pci_command = g_psBus->read_pci_config( sInfo.nBus, sInfo.nDevice, sInfo.nFunction, PCI_COMMAND, 2 );
				new_command = pci_command | ( PCI_COMMAND_MEMORY & 7);
				if (pci_command != new_command) {
					kerndbg(KERN_INFO, "  The PCI BIOS has not enabled the"
						   " device at %d/%d/%d!  Updating PCI command %4.4x->%4.4x.\n",
						   sInfo.nBus, sInfo.nDevice, sInfo.nFunction, pci_command, new_command );
					g_psBus->write_pci_config( sInfo.nBus, sInfo.nDevice, sInfo.nFunction, PCI_COMMAND, 2, new_command);
				}

				// allocate register area (Syllable way)
				np->reg_area = create_area ("dp83815_register", (void **)&reg_base,
					PAGE_SIZE * 2, PAGE_SIZE * 2, AREA_KERNEL|AREA_ANY_ADDRESS, AREA_NO_LOCK);
				if( np->reg_area < 0 ) {
					kerndbg ( KERN_DEBUG, "dp83815: failed to create register area (%d)\n", np->reg_area);
					goto out;
				}

				if( remap_area (np->reg_area, (void *)(addr & PAGE_MASK)) < 0 ) {
					kerndbg( KERN_DEBUG, "dp83815: failed to remap register area (%d)\n", np->reg_area);
					goto out;
				}

				dev->base_addr = (unsigned long)reg_base + ( addr - ( addr & PAGE_MASK ) );

				/* print some information about our NIC */
				kerndbg(KERN_INFO, "%s at %#lx, IRQ %d\n", dev->name,dev->base_addr, dev->irq);

				/* Do DP83815 initialisation here */
				cards_found++;

				/* natsemi has a non-standard PM control register
				 * in PCI config space.  Some boards apparently need
				 * to be brought to D0 in this manner.
				 */
				tmp = g_psBus->read_pci_config( sInfo.nBus, sInfo.nDevice, sInfo.nFunction, PCIPM, 2 );
				if (tmp & PCI_PM_CTRL_STATE_MASK) {
					/* D0 state, disable PME assertion */
					u32 newtmp = tmp & ~PCI_PM_CTRL_STATE_MASK;
					g_psBus->write_pci_config( sInfo.nBus, sInfo.nDevice, sInfo.nFunction, PCIPM, 2, newtmp);
				}

				/* Work around the dropped serial bit. */
				prev_eedata = eeprom_read(dev->base_addr, 6);
				for (j = 0; j < 3; j++) {
					int eedata = eeprom_read(dev->base_addr, j + 7);
					dev->dev_addr[j*2] = (eedata << 1) + (prev_eedata >> 15);
					dev->dev_addr[j*2+1] = eedata >> 7;
					prev_eedata = eedata;
				}

				if (natsemi_pci_info[chip_idx].flags & PCI_USES_MASTER)
					g_psBus->enable_pci_master(sInfo.nBus, sInfo.nDevice, sInfo.nFunction);

				np = dev->priv;
				np->pci_dev = &sInfo;

				spinlock_init(&np->lock, "dp83815");
				np->hands_off = 0;

				/* Reset the chip to erase previous misconfiguration. */
				natsemi_reload_eeprom(dev);
				natsemi_reset(dev);

				option = cards_found < MAX_UNITS ? options[cards_found] : 0;
				if (dev->mem_start)
					option = dev->mem_start;

				/* The lower four bits are the media type. */
				if (option) {
					if (option & 0x200)
						np->full_duplex = 1;
					if (option & 15)
						kerndbg(KERN_INFO,
							"%s: ignoring user supplied media type %d",
							dev->name, option & 15);
				}
				if (cards_found < MAX_UNITS  &&  full_duplex[cards_found])
					np->full_duplex = 1;

				np->advertising = mdio_read(dev, 1, MII_ADVERTISE);
				if ((readl(dev->base_addr + ChipConfig) & 0xe000) != 0xe000 ) {
					u32 chip_config = readl(dev->base_addr + ChipConfig);
					kerndbg(KERN_INFO, "%s: Transceiver default autonegotiation %s "
						"10%s %s duplex.\n",
						dev->name,
						chip_config & CfgAnegEnable ?
						  "enabled, advertise" : "disabled, force",
						chip_config & CfgAneg100 ? "0" : "",
						chip_config & CfgAnegFull ? "full" : "half");
				}

				/* save the silicon revision for later querying */
				np->srr = readl(dev->base_addr + SiliconRev);

				/* End of DP83815 initialisation */
				sprintf( node_path, "net/eth/dp83815-%d", cards_found-1 );
				dev->node_handle = create_device_node( nDeviceID, np->pci_dev->nHandle,
					node_path, &g_sDevOps, dev );
				kerndbg( KERN_DEBUG, "dp83815_probe() Create node: %s\n", node_path );
			}
		}
	}

	if( !cards_found )
		disable_device( nDeviceID );

	return cards_found ? 0 : -ENODEV;

out:
	kfree(dev);
	kerndbg( KERN_DEBUG, "dp83815: NIC initialization failed!\n");
	return -EIO;
}

status_t device_init( int nDeviceID )
{
	/* Get PCI bus */
    g_psBus = get_busmanager( PCI_BUS_NAME, PCI_BUS_VERSION );
    
    if( g_psBus == NULL )
    	return( -1 );
	return( dp83815_probe( nDeviceID ) );
}

status_t device_uninit( int nDeviceID )
{
    return( 0 );
}

