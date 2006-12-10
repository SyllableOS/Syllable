/* rtl8139.c: A RealTek RTL8129/8139 Fast Ethernet driver for Linux. */
/*
	Written and Copyright 1997-2003 by Donald Becker.
	This software may be used and distributed according to the terms of
	the GNU General Public License (GPL), incorporated herein by reference.
	Drivers based on or derived from this code fall under the GPL and must
	retain the authorship, copyright and license notice.  This file is not
	a complete program and may only be used when the entire operating
	system is licensed under the GPL.

	This driver is for boards based on the RTL8129 and RTL8139 PCI ethernet
	chips.

	The author may be reached as becker@scyld.com, or C/O
	Scyld Computing Corporation
	914 Bay Ridge Road, Suite 220
	Annapolis MD 21403

	Support and updates for Linux version available at
	http://www.scyld.com/network/rtl8139.html

	Twister-tuning table provided by Kinston <shangh@realtek.com.tw>.
	Ported to AtheOS by kurt Skauen <kurt@atheos.cx>
	Updated by Michael Krüger <invenies@web.de> to driver v1.24
*/

/* These identify the driver base version and may not be removed. */
static const char versionA[] =
"rtl8139.c:v1.24 11/13/2003 Donald Becker, becker@scyld.com.\n";
static const char versionB[] =
" http://www.scyld.com/network/rtl8139.html\n";

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
#define NO_DEBUG_STUBS 1
#include <atheos/linux_compat.h>

#include <posix/unistd.h>
#include <posix/errno.h>
#include <posix/signal.h>
#include <net/net.h>
#include <net/ip.h>
#include <net/sockios.h>
#include <net/net_device.h>

#define KERN_ERR "Error: "
#define KERN_DEBUG "Debug: "
#define KERN_WARNING "Warning: "
#define KERN_INFO "Info: "
#define KERN_NOTICE "Note: "

static DeviceOperations_s g_sDevOps;
PCI_bus_s* g_psBus;

/* Note: Register access width and timing restrictions apply in MMIO mode.
   This updated driver should nominally work, but I/O mode is better tested. */
#define USE_IO_OPS

/* The user-configurable values.
   These may be modified when a driver module is loaded.*/
/* Message enable level: 0..31 = no..all messages.  See NETIF_MSG docs. */
static int debug = 2;

/* Maximum events (Rx packets, etc.) to handle at each interrupt. */
static int max_interrupt_work = 20;

/* Maximum number of multicast addresses to filter (vs. Rx-all-multicast).
   The RTL chips use a 64 element hash table based on the Ethernet CRC.  It
   is efficient to update the hardware filter, but recalculating the table
   for a long filter list is painful.  */
static int multicast_filter_limit = 32;

/* Used to pass the full-duplex flag, etc. */
#define MAX_UNITS 8		/* More are supported, limit only on options */
static int options[MAX_UNITS] = {-1, -1, -1, -1, -1, -1, -1, -1};
static int full_duplex[MAX_UNITS] = {-1, -1, -1, -1, -1, -1, -1, -1};

/* Operational parameters that are set at compile time. */

/* Maximum size of the in-memory receive ring (smaller if no memory). */
#define RX_BUF_LEN_IDX	2			/* 0==8K, 1==16K, 2==32K, 3==64K */
/* Size of the Tx bounce buffers -- must be at least (dev->mtu+14+4). */
#define TX_BUF_SIZE	1536

/* PCI Tuning Parameters
   Threshold is bytes transferred to chip before transmission starts. */
#define TX_FIFO_THRESH 256	/* In bytes, rounded down to 32 byte units. */

/* The following settings are log_2(bytes)-4:  0 == 16 bytes .. 6==1024. */
#define RX_FIFO_THRESH	4		/* Rx buffer level before first PCI xfer.  */
#define RX_DMA_BURST	4		/* Maximum PCI burst, '4' is 256 bytes */
#define TX_DMA_BURST	4		/* Calculate as 16<<val. */

/* Operational parameters that usually are not changed. */
/* Time in jiffies before concluding the transmitter is hung. */
#define TX_TIMEOUT  (6*HZ)

/* Allocation size of Rx buffers with full-sized Ethernet frames.
   This is a cross-driver value that is not a limit,
   but a way to keep a consistent allocation size among drivers.
 */
#define PKT_BUF_SZ		1536

/* Kernel compatibility defines, some common to David Hind's PCMCIA package.
   This is only in the support-all-kernels source code. */

#define RUN_AT(x) (jiffies + (x))

/*
				Theory of Operation

I. Board Compatibility

This device driver is designed for the RealTek RTL8129 series, the RealTek
Fast Ethernet controllers for PCI and CardBus.  This chip is used on many
low-end boards, sometimes with custom chip labels.


II. Board-specific settings

PCI bus devices are configured by the system at boot time, so no jumpers
need to be set on the board.  The system BIOS will assign the
PCI INTA signal to a (preferably otherwise unused) system IRQ line.
Note: Kernel versions earlier than 1.3.73 do not support shared PCI
interrupt lines.

III. Driver operation

IIIa. Rx Ring buffers

The receive unit uses a single linear ring buffer rather than the more
common (and more efficient) descriptor-based architecture.  Incoming frames
are sequentially stored into the Rx region, and the host copies them into
skbuffs.

Comment: While it is theoretically possible to process many frames in place,
any delay in Rx processing would block the Rx ring and cause us to drop
frames.  It would be difficult to design a protocol stack where the data
buffer could be recalled by the device driver.

IIIb. Tx operation

The RTL8129 uses a fixed set of four Tx descriptors in register space.  Tx
frames must be 32 bit aligned.  Linux aligns the IP header on word
boundaries, and 14 byte ethernet header means that almost all frames will
need to be copied to an alignment buffer.  The driver statically allocates
alignment the four alignment buffers at open() time.

IVb. References

http://www.realtek.com.tw/cn/cn.html
http://cesdis.gsfc.nasa.gov/linux/misc/NWay.html

IVc. Errata

*/


/* This table drives the PCI probe routines.  It's mostly boilerplate in all
   of the drivers, and will likely be provided by some future kernel.
   Note the matching code -- the first table entry matchs all 56** cards but
   second only the 1234 card.
*/
enum pci_id_flags_bit {
	PCI_USES_IO=1, PCI_USES_MEM=2, PCI_USES_MASTER=4,
	PCI_ADDR0=0<<4, PCI_ADDR1=1<<4, PCI_ADDR2=2<<4, PCI_ADDR3=3<<4,
	PCI_ADDR_64BITS=0x100, PCI_NO_ACPI_WAKE=0x200, PCI_NO_MIN_LATENCY=0x400,
};
struct pci_id_info {
	const char *name;
	uint16	vendor_id, device_id, device_id_mask;
	uint16 pci_flags;
	int io_size;
	int drv_flags;
};


static struct net_device * rtl8129_probe1( int bus, int device, int function,
									   int device_handle, int nHandle, long ioaddr,
									   int irq, int chp_idx, int fnd_cnt);


static int rtl_pwr_event(void *dev_instance, int event);

enum chip_capability_flags {HAS_MII_XCVR=0x01, HAS_CHIP_XCVR=0x02,
			HAS_LNK_CHNG=0x04, HAS_DESC=0x08};
#ifdef USE_IO_OPS
#define RTL8139_IOTYPE  PCI_USES_MASTER|PCI_USES_IO
#else
#define RTL8139_IOTYPE  PCI_USES_MASTER|PCI_USES_MEM
#endif
#define RTL8129_CAPS  HAS_MII_XCVR
#define RTL8139_CAPS  HAS_CHIP_XCVR|HAS_LNK_CHNG
#define RTL8139D_CAPS  HAS_CHIP_XCVR|HAS_LNK_CHNG|HAS_DESC

/* Note: Update the marked constant in _attach() if the RTL8139B entry moves.*/
static struct pci_id_info pci_tbl[] = {
	{"RealTek RTL8139C+ Fast Ethernet", 0x10ec, 0x8139, 0xffff,
	 RTL8139_IOTYPE, 0x80, RTL8139_CAPS, },
	{"RealTek RTL8129 Fast Ethernet", 0x10ec, 0x8129, 0xffff,
	 RTL8139_IOTYPE, 0x80, RTL8129_CAPS, },
	{"RealTek RTL8139B PCI/CardBus",  0x10ec, 0x8138, 0xffff,
	 RTL8139_IOTYPE, 0x80, RTL8139_CAPS, },
	{"SMC1211TX EZCard 10/100 (RealTek RTL8139)", 0x1113, 0x1211, 0xffff,
	 RTL8139_IOTYPE, 0x80, RTL8139_CAPS, },
	{"D-Link DFE-530TX+ (RealTek RTL8139C)", 0x1186, 0x1300, 0xffff,
	 RTL8139_IOTYPE, 0x100, RTL8139_CAPS, },
	{"LevelOne FPC-0106Tx (RealTek RTL8139)", 0x018a, 0x0106, 0xffff,
	 RTL8139_IOTYPE, 0x80, RTL8139_CAPS, },
	{"Compaq HNE-300 (RealTek RTL8139c)", 0x021b, 0x8139, 0xffff,
	 RTL8139_IOTYPE, 0x80, RTL8139_CAPS, },
	{"Edimax EP-4103DL CardBus (RealTek RTL8139c)", 0x13d1, 0xab06, 0xffff,
	 RTL8139_IOTYPE, 0x80, RTL8139_CAPS, },
	{"Siemens 1012v2 CardBus (RealTek RTL8139c)", 0x02ac, 0x1012, 0xffff,
	 RTL8139_IOTYPE, 0x80, RTL8139_CAPS, },
	{"Siemens 1020 PCI NIC (RealTek RTL8139c)", 0x1432, 0x9130, 0xffff,
	 RTL8139_IOTYPE, 0x80, RTL8139_CAPS, },
	{"D-Link DFE-690TXD CardBus (RealTek RTL8139C)", 0x1186, 0x1340, 0xffff,
	 RTL8139_IOTYPE, 0x100, RTL8139_CAPS, },
	{"D-Link 1300 series NIC (RealTek RTL8139C)", 0x1186, 0x1300, 0xff00,
	 RTL8139_IOTYPE, 0x100, RTL8139_CAPS, },
	{0,},						/* 0 terminated list. */
};

//struct drv_id_info rtl8139_drv_id = {
	//"realtek", PCI_HOTSWAP, PCI_CLASS_NETWORK_ETHERNET<<8, pci_tbl,
	//rtl8139_probe1, rtl_pwr_event };

#ifndef USE_IO_OPS
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

/* The rest of these values should never change. */
#define NUM_TX_DESC	4			/* Number of Tx descriptor registers. */

/* Symbolic offsets to registers. */
enum RTL8129_registers {
	MAC0=0,						/* Ethernet hardware address. */
	MAR0=8,						/* Multicast filter. */
	TxStatus0=0x10,				/* Transmit status (Four 32bit registers). */
	TxAddr0=0x20,				/* Tx descriptors (also four 32bit). */
	RxBuf=0x30, RxEarlyCnt=0x34, RxEarlyStatus=0x36,
	ChipCmd=0x37, RxBufPtr=0x38, RxBufAddr=0x3A,
	IntrMask=0x3C, IntrStatus=0x3E,
	TxConfig=0x40, RxConfig=0x44,
	Timer=0x48,					/* A general-purpose counter. */
	RxMissed=0x4C,				/* 24 bits valid, write clears. */
	Cfg9346=0x50, Config0=0x51, Config1=0x52,
	FlashReg=0x54, GPPinData=0x58, GPPinDir=0x59, MII_SMI=0x5A, HltClk=0x5B,
	MultiIntr=0x5C, TxSummary=0x60,
	MII_BMCR=0x62, MII_BMSR=0x64, NWayAdvert=0x66, NWayLPAR=0x68,
	NWayExpansion=0x6A,
	/* Undocumented registers, but required for proper operation. */
	FIFOTMS=0x70,	/* FIFO Control and test. */
	CSCR=0x74,	/* Chip Status and Configuration Register. */
	PARA78=0x78, PARA7c=0x7c,	/* Magic transceiver parameter register. */
};

enum ChipCmdBits {
	CmdReset=0x10, CmdRxEnb=0x08, CmdTxEnb=0x04, RxBufEmpty=0x01, };

/* Interrupt register bits, using my own meaningful names. */
enum IntrStatusBits {
	PCIErr=0x8000, PCSTimeout=0x4000,
	RxFIFOOver=0x40, RxUnderrun=0x20, RxOverflow=0x10,
	TxErr=0x08, TxOK=0x04, RxErr=0x02, RxOK=0x01,
};
enum TxStatusBits {
	TxHostOwns=0x2000, TxUnderrun=0x4000, TxStatOK=0x8000,
	TxOutOfWindow=0x20000000, TxAborted=0x40000000, TxCarrierLost=0x80000000,
};
enum RxStatusBits {
	RxMulticast=0x8000, RxPhysical=0x4000, RxBroadcast=0x2000,
	RxBadSymbol=0x0020, RxRunt=0x0010, RxTooLong=0x0008, RxCRCErr=0x0004,
	RxBadAlign=0x0002, RxStatusOK=0x0001,
};

/* Twister tuning parameters from RealTek.
   Completely undocumented, but required to tune bad links. */
enum CSCRBits {
	CSCR_LinkOKBit=0x0400, CSCR_LinkChangeBit=0x0800,
	CSCR_LinkStatusBits=0x0f000, CSCR_LinkDownOffCmd=0x003c0,
	CSCR_LinkDownCmd=0x0f3c0,
};
#define PARA78_default	0x78fa8388
#define PARA7c_default	0xcb38de43 			/* param[0][3] */
#define PARA7c_xxx		0xcb38de43
unsigned long param[4][4]={
	{0xcb39de43, 0xcb39ce43, 0xfb38de03, 0xcb38de43},
	{0xcb39de43, 0xcb39ce43, 0xcb39ce83, 0xcb39ce83},
	{0xcb39de43, 0xcb39ce43, 0xcb39ce83, 0xcb39ce83},
	{0xbb39de43, 0xbb39ce43, 0xbb39ce83, 0xbb39ce83}
};

#define PRIV_ALIGN	15 	/* Desired alignment mask */
struct rtl8129_private {
	const char *product_name;
	struct net_device *next_module;
	void *priv_addr;					/* Unaligned address for kfree */

	int chip_id, drv_flags;
	//struct pci_dev *pci_dev;
	unsigned char pci_bus, pci_device, pci_function;
	struct enet_statistics stats;
	//struct timer_list timer;	/* Media selection timer. */
	ktimer_t timer;	/* Media selection timer. */
	sem_id		 tx_sema;
	SpinLock_s	 tx_spinlock;
	int msg_level;
	int max_interrupt_work;
	unsigned short		flags;	/* interface flags (a la BSD)	*/

	/* Receive state. */
	unsigned char *rx_ring;
	unsigned int cur_rx;		/* Index into the Rx buffer of next Rx pkt. */
	unsigned int rx_buf_len;	/* Size (8K 16K 32K or 64KB) of the Rx ring */

	/* Transmit state. */
	unsigned int cur_tx, dirty_tx, tx_flag;
	unsigned long tx_full;				/* The Tx queue is full. */
	/* The saved address of a sent-in-place packet/buffer, for skfree(). */
	PacketBuf_s* tx_skbuff[NUM_TX_DESC];
	unsigned char *tx_buf[NUM_TX_DESC];	/* Tx bounce buffers */
	unsigned char *tx_bufs;				/* Tx bounce buffer region. */

	/* Receive filter state. */
	unsigned int rx_config;
	uint32 mc_filter[2];		 /* Multicast hash filter */
	int cur_rx_mode;
	int multicast_filter_limit;

	/* Transceiver state. */
	char phys[4];						/* MII device addresses. */
	uint16 advertising;					/* NWay media advertisement */
	char twistie, twist_row, twist_col;	/* Twister tune state. */
	uint8	config1;
	unsigned int full_duplex:1;			/* Full-duplex operation requested. */
	unsigned int duplex_lock:1;
	unsigned int media2:4;				/* Secondary monitored media port. */
	unsigned int medialock:1;			/* Don't sense media type. */
	unsigned int mediasense:1;			/* Media sensing in progress. */
	unsigned int default_port;			/* Last dev->if_port value. */
};

#ifdef MODULE
/* Donald Becker is the original author and copyright holder of this code.
 * Changing the driver so that this information is removed and distributing
 * the resulting binary driver is a violation of the license.
 */
MODULE_AUTHOR("Donald Becker <becker@scyld.com>");
MODULE_DESCRIPTION("RealTek RTL8129/8139 Fast Ethernet driver");
MODULE_LICENSE("GPL");
MODULE_PARM(options, "1-" __MODULE_STRING(MAX_UNITS) "i");
MODULE_PARM(full_duplex, "1-" __MODULE_STRING(MAX_UNITS) "i");
MODULE_PARM(multicast_filter_limit, "i");
MODULE_PARM(max_interrupt_work, "i");
MODULE_PARM(debug, "i");
MODULE_PARM_DESC(debug, "Driver message level (0-31)");
MODULE_PARM_DESC(options, "Force transceiver type or fixed speed+duplex");
MODULE_PARM_DESC(full_duplex, "Non-zero to set forced full duplex.");
MODULE_PARM_DESC(multicast_filter_limit,
				 "Multicast addresses before switching to Rx-all-multicast");
MODULE_PARM_DESC(max_interrupt_work,
				 "Driver maximum events handled per interrupt");
#endif

static int rtl8129_open(struct net_device *dev);
static void rtl_hw_start(struct net_device *dev);
static int read_eeprom(long ioaddr, int location, int addr_len);
static int mdio_read(struct net_device *dev, int phy_id, int location);
static void mdio_write(struct net_device *dev, int phy_id, int location, int val);
static void rtl8129_timer(void* data);
static void rtl8129_tx_timeout(struct net_device *dev);
static void rtl8129_init_ring(struct net_device *dev);
static int rtl8129_start_xmit(PacketBuf_s *skb, struct net_device *dev);
static int rtl8129_rx(struct net_device *dev);
static int rtl8129_interrupt(int irq, void *dev_instance, SysCallRegs_s *regs);
static void rtl_error(struct net_device *dev, int status, int link_status);
static int rtl8129_close(struct net_device *dev);
//static int mii_ioctl(struct net_device *dev, struct ifreq *rq, int cmd);
//static struct net_device_stats *rtl8129_get_stats(struct net_device *dev);
static inline uint32 ether_crc(int length, unsigned char *data);
static void set_rx_mode(struct net_device *dev);


/* A list of all installed RTL8129 devices, for removing the driver module. */
static struct net_device *root_rtl8129_dev = NULL;

/* Ideally we would detect all network cards in slot order.  That would
   be best done a central PCI probe dispatch, which wouldn't work
   well when dynamically adding drivers.  So instead we detect just the
   Rtl81*9 cards in slot order. */

int rtl8139_probe( int device_handle )
{
	int cards_found = 0;
//	int pci_index = 0;
//	unsigned char pci_bus, pci_device_fn;
	struct net_device* dev;
	
    int i;
    PCI_Info_s sInfo;
   
    for ( i = 0 ; g_psBus->get_pci_info( &sInfo, i ) == 0 ; ++i ) {
		int chip_idx;
		int ioaddr;
		int irq;
		int pci_command;
		int new_command;
		
		for ( chip_idx = 0 ; pci_tbl[chip_idx].vendor_id ; chip_idx++ ) {
			if ( sInfo.nVendorID == pci_tbl[chip_idx].vendor_id &&
				 (sInfo.nDeviceID & pci_tbl[chip_idx].device_id_mask) == pci_tbl[chip_idx].device_id) {
				break;
			}
		}
		if (pci_tbl[chip_idx].vendor_id == 0) { 		/* Compiled out! */
			continue;
		}
		
		if( claim_device( device_handle, sInfo.nHandle, "Realtek 8139", DEVICE_NET ) != 0 )
			continue;
			
		
		printk( "rtl8139_probe() found NIC\n" );
		ioaddr = sInfo.u.h0.nBase0 & PCI_ADDRESS_IO_MASK;
		irq = sInfo.u.h0.nInterruptLine;

		pci_command = g_psBus->read_pci_config( sInfo.nBus, sInfo.nDevice, sInfo.nFunction, PCI_COMMAND, 2 );
		new_command = pci_command | (pci_tbl[chip_idx].pci_flags & 7);
		if (pci_command != new_command) {
			printk(KERN_INFO "  The PCI BIOS has not enabled the"
				   " device at %d/%d/%d!  Updating PCI command %4.4x->%4.4x.\n",
				   sInfo.nBus, sInfo.nDevice, sInfo.nFunction, pci_command, new_command );
			g_psBus->write_pci_config( sInfo.nBus, sInfo.nDevice, sInfo.nFunction, PCI_COMMAND, 2, new_command);
		}

		dev = rtl8129_probe1( sInfo.nBus, sInfo.nDevice, sInfo.nFunction, device_handle, sInfo.nHandle,
										ioaddr, irq, chip_idx, cards_found );
										
		set_device_data( sInfo.nHandle, dev );
		
		dev = 0;
		cards_found++;
		break;
	}
	if( !cards_found )
		disable_device( device_handle );
	return cards_found ? 0 : -ENODEV;
}


static struct net_device * rtl8129_probe1( int pci_bus, int pci_device, int pci_func,
									   int device_handle, int nHandle, long ioaddr,
									   int irq, int chip_idx, int found_cnt)
{
	struct net_device *dev;
	struct rtl8129_private *np;
	void *priv_mem;
	int i, option = found_cnt < MAX_UNITS ? options[found_cnt] : 0;
	int config1;
	char node_path[64];

	dev = kmalloc( sizeof(*dev), MEMF_KERNEL | MEMF_CLEAR );
	if (!dev)
		return NULL;
	dev->name = "rtl8139";

	printk(KERN_INFO "%s: %s at %#lx, IRQ %d\n",
		   dev->name, pci_tbl[chip_idx].name, ioaddr, irq);

	/* Bring the chip out of low-power mode. */
	config1 = inb(ioaddr + Config1);
	if (pci_tbl[chip_idx].drv_flags & HAS_MII_XCVR)			/* rtl8129 chip */
		outb(config1 & ~0x03, ioaddr + Config1);

	{
		int addr_len = read_eeprom(ioaddr, 0, 8) == 0x8129 ? 8 : 6;
		for (i = 0; i < 3; i++)
			((uint16 *)(dev->dev_addr))[i] =
				read_eeprom(ioaddr, i+7, addr_len); // le16_to_cpu
	}

	printk( "rtl8139 Mac address: %02x%02x%02x%02x%02x%02x\n",
		dev->dev_addr[0], dev->dev_addr[1], dev->dev_addr[2],
		dev->dev_addr[3], dev->dev_addr[4], dev->dev_addr[5] );

	//for (i = 0; i < 5; i++)
	//	printk("%2.2x:", dev->dev_addr[i]);
	//printk("%2.2x.\n", dev->dev_addr[i]);

	/* Make certain elements e.g. descriptor lists are aligned. */
	priv_mem = kmalloc(sizeof(*np) + PRIV_ALIGN, MEMF_KERNEL | MEMF_CLEAR);
	/* Check for the very unlikely case of no memory. */
	if (priv_mem == NULL)
		return NULL;

	/* We do a request_region() to register /proc/ioports info. */
	//request_region(ioaddr, pci_tbl[chip_idx].io_size, dev->name);

	dev->base_addr = ioaddr;
	dev->irq = irq;

	dev->priv = np = (void *)(((long)priv_mem + PRIV_ALIGN) & ~PRIV_ALIGN);
	memset(np, 0, sizeof(*np));
	np->priv_addr = priv_mem;

	spinlock_init( &np->tx_spinlock, "rtl8139_tx_spinlock" );

	np->next_module = root_rtl8129_dev;
	root_rtl8129_dev = dev;

	np->chip_id = chip_idx;
	np->pci_bus 	 = pci_bus;
	np->pci_device 	 = pci_device;
	np->pci_function = pci_func;
	np->chip_id = chip_idx;
	np->drv_flags = pci_tbl[chip_idx].drv_flags;
	np->msg_level = (1 << debug) - 1;
	np->max_interrupt_work = max_interrupt_work;
	np->multicast_filter_limit = multicast_filter_limit;

	np->config1 = config1;

	/* Find the connected MII xcvrs.
	   Doing this in open() would allow detecting external xcvrs later, but
	   takes too much time. */
	if (np->drv_flags & HAS_MII_XCVR) {
		int phy, phy_idx = 0;
		for (phy = 0; phy < 32 && phy_idx < sizeof(np->phys); phy++) {
			int mii_status = mdio_read(dev, phy, 1);
			if (mii_status != 0xffff  &&  mii_status != 0x0000) {
				np->phys[phy_idx++] = phy;
				np->advertising = mdio_read(dev, phy, 4);
				printk(KERN_INFO "%s: MII transceiver %d status 0x%4.4x "
					   "advertising %4.4x.\n",
					   dev->name, phy, mii_status, np->advertising);
			}
		}
		if (phy_idx == 0) {
			printk(KERN_INFO "%s: No MII transceivers found!  Assuming SYM "
				   "transceiver.\n",
				   dev->name);
			np->phys[0] = 32;
		}
	} else
		np->phys[0] = 32;

	/* Put the chip into low-power mode. */
	outb(0xC0, ioaddr + Cfg9346);
	if (np->drv_flags & HAS_MII_XCVR)			/* rtl8129 chip */
		outb(0x03, ioaddr + Config1);

	outb('H', ioaddr + HltClk);		/* 'R' would leave the clock running. */

	/* The lower four bits are the media type. */
	if (option > 0) {
		np->full_duplex = (option & 0x220) ? 1 : 0;
		np->default_port = option & 0x330;
		if (np->default_port)
			np->medialock = 1;
	}

	if (found_cnt < MAX_UNITS  &&  full_duplex[found_cnt] > 0)
		np->full_duplex = full_duplex[found_cnt];

	if (np->full_duplex) {
		printk(KERN_INFO "%s: Media type forced to Full Duplex.\n", dev->name);
		/* Changing the MII-advertised media might prevent re-connection. */
		np->duplex_lock = 1;
	}
	if (np->default_port) {
		printk(KERN_INFO "  Forcing %dMbs %s-duplex operation.\n",
			   (option & 0x300 ? 100 : 10),
			   (option & 0x220 ? "full" : "half"));
		mdio_write(dev, np->phys[0], 0,
				   ((option & 0x300) ? 0x2000 : 0) | 	/* 100mbps? */
				   ((option & 0x220) ? 0x0100 : 0)); /* Full duplex? */
	}

	/* The rtl81x9-specific entries in the device structure. */
	//dev->open = &rtl8129_open;
	//dev->hard_start_xmit = &rtl8129_start_xmit;
	//dev->stop = &rtl8129_close;
	//dev->get_stats = &rtl8129_get_stats;
	//dev->set_multicast_list = &set_rx_mode;
	//dev->do_ioctl = &mii_ioctl;

	sprintf( node_path, "net/eth/rtl8139-%d", found_cnt );
	create_device_node( device_handle, nHandle, node_path, &g_sDevOps, dev );
	printk( "rtl8129_probe1() Create node: %s\n", node_path );

	return dev;
}

/* Serial EEPROM section. */

/*  EEPROM_Ctrl bits. */
#define EE_SHIFT_CLK	0x04	/* EEPROM shift clock. */
#define EE_CS			0x08	/* EEPROM chip select. */
#define EE_DATA_WRITE	0x02	/* EEPROM chip data in. */
#define EE_WRITE_0		0x00
#define EE_WRITE_1		0x02
#define EE_DATA_READ	0x01	/* EEPROM chip data out. */
#define EE_ENB			(0x80 | EE_CS)

/* Delay between EEPROM clock transitions.
   No extra delay is needed with 33Mhz PCI, but 66Mhz may change this.
 */

#define eeprom_delay()	inl(ee_addr)

/* The EEPROM commands include the alway-set leading bit. */
#define EE_WRITE_CMD	(5)
#define EE_READ_CMD		(6)
#define EE_ERASE_CMD	(7)

static int read_eeprom(long ioaddr, int location, int addr_len)
{
	int i;
	unsigned retval = 0;
	long ee_addr = ioaddr + Cfg9346;
	int read_cmd = location | (EE_READ_CMD << addr_len);

	outb(EE_ENB & ~EE_CS, ee_addr);
	outb(EE_ENB, ee_addr);

	/* Shift the read command bits out. */
	for (i = 4 + addr_len; i >= 0; i--) {
		int dataval = (read_cmd & (1 << i)) ? EE_DATA_WRITE : 0;
		outb(EE_ENB | dataval, ee_addr);
		eeprom_delay();
		outb(EE_ENB | dataval | EE_SHIFT_CLK, ee_addr);
		eeprom_delay();
	}
	outb(EE_ENB, ee_addr);
	eeprom_delay();

	for (i = 16; i > 0; i--) {
		outb(EE_ENB | EE_SHIFT_CLK, ee_addr);
		eeprom_delay();
		retval = (retval << 1) | ((inb(ee_addr) & EE_DATA_READ) ? 1 : 0);
		outb(EE_ENB, ee_addr);
		eeprom_delay();
	}

	/* Terminate the EEPROM access. */
	outb(~EE_CS, ee_addr);
	return retval;
}

/* MII serial management: mostly bogus for now. */
/* Read and write the MII management registers using software-generated
   serial MDIO protocol.
   The maximum data clock rate is 2.5 Mhz.  The minimum timing is usually
   met by back-to-back PCI I/O cycles, but we insert a delay to avoid
   "overclocking" issues. */
#define MDIO_DIR		0x80
#define MDIO_DATA_OUT	0x04
#define MDIO_DATA_IN	0x02
#define MDIO_CLK		0x01
#define MDIO_WRITE0 (MDIO_DIR)
#define MDIO_WRITE1 (MDIO_DIR | MDIO_DATA_OUT)

#define mdio_delay(mdio_addr)	inb(mdio_addr)

static char mii_2_8139_map[8] = {MII_BMCR, MII_BMSR, 0, 0, NWayAdvert,
								 NWayLPAR, NWayExpansion, 0 };

/* Syncronize the MII management interface by shifting 32 one bits out. */
static void mdio_sync(long mdio_addr)
{
	int i;

	for (i = 32; i >= 0; i--) {
		outb(MDIO_WRITE1, mdio_addr);
		mdio_delay(mdio_addr);
		outb(MDIO_WRITE1 | MDIO_CLK, mdio_addr);
		mdio_delay(mdio_addr);
	}
	return;
}
static int mdio_read(struct net_device *dev, int phy_id, int location)
{
	long mdio_addr = dev->base_addr + MII_SMI;
	int mii_cmd = (0xf6 << 10) | (phy_id << 5) | location;
	int retval = 0;
	int i;

	if (phy_id > 31) {	/* Really a 8139.  Use internal registers. */
		return location < 8 && mii_2_8139_map[location] ?
			inw(dev->base_addr + mii_2_8139_map[location]) : 0;
	}
	mdio_sync(mdio_addr);
	/* Shift the read command bits out. */
	for (i = 15; i >= 0; i--) {
		int dataval = (mii_cmd & (1 << i)) ? MDIO_DATA_OUT : 0;

		outb(MDIO_DIR | dataval, mdio_addr);
		mdio_delay(mdio_addr);
		outb(MDIO_DIR | dataval | MDIO_CLK, mdio_addr);
		mdio_delay(mdio_addr);
	}

	/* Read the two transition, 16 data, and wire-idle bits. */
	for (i = 19; i > 0; i--) {
		outb(0, mdio_addr);
		mdio_delay(mdio_addr);
		retval = (retval << 1) | ((inb(mdio_addr) & MDIO_DATA_IN) ? 1 : 0);
		outb(MDIO_CLK, mdio_addr);
		mdio_delay(mdio_addr);
	}
	return (retval>>1) & 0xffff;
}

static void mdio_write(struct net_device *dev, int phy_id, int location,
					   int value)
{
	long mdio_addr = dev->base_addr + MII_SMI;
	int mii_cmd = (0x5002 << 16) | (phy_id << 23) | (location<<18) | value;
	int i;

	if (phy_id > 31) {	/* Really a 8139.  Use internal registers. */
		long ioaddr = dev->base_addr;
		if (location == 0) {
			outb(0xC0, ioaddr + Cfg9346);
			outw(value, ioaddr + MII_BMCR);
			outb(0x00, ioaddr + Cfg9346);
		} else if (location < 8  &&  mii_2_8139_map[location])
			outw(value, ioaddr + mii_2_8139_map[location]);
		return;
	}
	mdio_sync(mdio_addr);

	/* Shift the command bits out. */
	for (i = 31; i >= 0; i--) {
		int dataval = (mii_cmd & (1 << i)) ? MDIO_WRITE1 : MDIO_WRITE0;
		outb(dataval, mdio_addr);
		mdio_delay(mdio_addr);
		outb(dataval | MDIO_CLK, mdio_addr);
		mdio_delay(mdio_addr);
	}
	/* Clear out extra bits. */
	for (i = 2; i > 0; i--) {
		outb(0, mdio_addr);
		mdio_delay(mdio_addr);
		outb(MDIO_CLK, mdio_addr);
		mdio_delay(mdio_addr);
	}
	return;
}


static int rtl8129_open(struct net_device *dev)
{
	struct rtl8129_private *tp = (struct rtl8129_private *)dev->priv;
	long ioaddr = dev->base_addr;
	int rx_buf_len_idx;

	//MOD_INC_USE_COUNT;
	dev->irq_handle = request_irq(dev->irq, rtl8129_interrupt, NULL, SA_SHIRQ, dev->name, dev);
	if(dev->irq_handle < 0) {
		//MOD_DEC_USE_COUNT;
		return -EAGAIN;
	}

	/* The Rx ring allocation size is 2^N + delta, which is worst-case for
	   the kernel binary-buddy allocation.  We allocate the Tx bounce buffers
	   at the same time to use some of the otherwise wasted space.
	   The delta of +16 is required for dribble-over because the receiver does
	   not wrap when the packet terminates just beyond the end of the ring. */
	rx_buf_len_idx = RX_BUF_LEN_IDX;
	do {
		tp->rx_buf_len = 8192 << rx_buf_len_idx;
		tp->rx_ring = kmalloc(tp->rx_buf_len + 16 +
							  (TX_BUF_SIZE * NUM_TX_DESC), MEMF_KERNEL);
	} while (tp->rx_ring == NULL  &&  --rx_buf_len_idx >= 0);

	if (tp->rx_ring == NULL) {
		if (debug > 0)
			printk(KERN_ERR "%s: Couldn't allocate a %d byte receive ring.\n",
				   dev->name, tp->rx_buf_len);
		//MOD_DEC_USE_COUNT;
		return -ENOMEM;
	}
	tp->tx_bufs = tp->rx_ring + tp->rx_buf_len + 16;

	rtl8129_init_ring(dev);
	tp->full_duplex = tp->duplex_lock;
	tp->tx_flag = (TX_FIFO_THRESH<<11) & 0x003f0000;
	tp->rx_config =
		(RX_FIFO_THRESH << 13) | (rx_buf_len_idx << 11) | (RX_DMA_BURST<<8);

	rtl_hw_start(dev);
	netif_start_tx_queue(dev);

 	if (debug > 3)
		printk(KERN_DEBUG"%s: rtl8129_open() ioaddr %#lx IRQ %d"
			   " GP Pins %2.2x %s-duplex.\n",
			   dev->name, ioaddr, dev->irq, inb(ioaddr + GPPinData),
			   tp->full_duplex ? "full" : "half");

	/* Set the timer to switch to check for link beat and perhaps switch
	   to an alternate media type. */

	/*
	/nit_timer(&tp->timer);
	tp->timer.expires = jiffies + 3*HZ;
	tp->timer.data = (unsigned long)dev;
	tp->timer.function = &rtl8129_timer;
	add_timer(&tp->timer);
	*/
	tp->timer = create_timer();
	start_timer( tp->timer, (timer_callback *) &rtl8129_timer, dev, 2400000, true );
	
	return 0;
}

/* Start the hardware at open or resume. */
static void rtl_hw_start(struct net_device *dev)
{
	struct rtl8129_private *tp = (struct rtl8129_private *)dev->priv;
	long ioaddr = dev->base_addr;
	int i;

	/* Soft reset the chip. */
	outb(CmdReset, ioaddr + ChipCmd);
	/* Check that the chip has finished the reset. */
	for (i = 1000; i > 0; i--)
		if ((inb(ioaddr + ChipCmd) & CmdReset) == 0)
			break;
	/* Restore our idea of the MAC address. */
	outb(0xC0, ioaddr + Cfg9346);
	outl(cpu_to_le32(*(uint32*)(dev->dev_addr + 0)), ioaddr + MAC0 + 0);
	outl(cpu_to_le32(*(uint32*)(dev->dev_addr + 4)), ioaddr + MAC0 + 4);

	/* Hmmm, do these belong here? */
	tp->cur_rx = 0;

	/* Must enable Tx/Rx before setting transfer thresholds! */
	outb(CmdRxEnb | CmdTxEnb, ioaddr + ChipCmd);
	outl(tp->rx_config, ioaddr + RxConfig);
	/* Check this value: the documentation contradicts ifself.  Is the
	   IFG correct with bit 28:27 zero, or with |0x03000000 ? */
	outl((TX_DMA_BURST<<8), ioaddr + TxConfig);

	/* This is check_duplex() */
	if (tp->phys[0] >= 0  ||  (tp->drv_flags & HAS_MII_XCVR)) {
		uint16 mii_reg5 = mdio_read(dev, tp->phys[0], 5);
		if (mii_reg5 == 0xffff)
			;					/* Not there */
		else if ((mii_reg5 & 0x0100) == 0x0100
				 || (mii_reg5 & 0x00C0) == 0x0040)
			tp->full_duplex = 1;
		if (debug > 2)
			printk(KERN_INFO"%s: Setting %s%s-duplex based on"
				   " auto-negotiated partner ability %4.4x.\n", dev->name,
				   mii_reg5 == 0 ? "" :
				   (mii_reg5 & 0x0180) ? "100mbps " : "10mbps ",
				   tp->full_duplex ? "full" : "half", mii_reg5);
	}

	if (tp->drv_flags & HAS_MII_XCVR)			/* rtl8129 chip */
		outb(tp->full_duplex ? 0x60 : 0x20, ioaddr + Config1);
	outb(0x00, ioaddr + Cfg9346);

	outl(virt_to_bus(tp->rx_ring), ioaddr + RxBuf);
	/* Start the chip's Tx and Rx process. */
	outl(0, ioaddr + RxMissed);
	set_rx_mode(dev);
	outb(CmdRxEnb | CmdTxEnb, ioaddr + ChipCmd);
	/* Enable all known interrupts by setting the interrupt mask. */
	outw(PCIErr | PCSTimeout | RxUnderrun | RxOverflow | RxFIFOOver
		 | TxErr | TxOK | RxErr | RxOK, ioaddr + IntrMask);

}

static void rtl8129_timer(void* data)
{
	struct net_device *dev = (struct net_device *)data;
	struct rtl8129_private *np = (struct rtl8129_private *)dev->priv;
	long ioaddr = dev->base_addr;
	int next_tick = 60*HZ;
	

	int mii_reg5 = mdio_read(dev, np->phys[0], 5);
	if (! np->duplex_lock  &&  mii_reg5 != 0xffff) {
		int duplex = (mii_reg5&0x0100) || (mii_reg5 & 0x01C0) == 0x0040;
		if (np->full_duplex != duplex) {
			np->full_duplex = duplex;
			printk(KERN_INFO "%s: Using %s-duplex based on MII #%d link"
				   " partner ability of %4.4x.\n", dev->name,
				   np->full_duplex ? "full" : "half", np->phys[0], mii_reg5);
			if (np->drv_flags & HAS_MII_XCVR) {
				outb(0xC0, ioaddr + Cfg9346);
				outb(np->full_duplex ? 0x60 : 0x20, ioaddr + Config1);
				outb(0x00, ioaddr + Cfg9346);
			}
		}

	}
	if (netif_queue_paused(dev)  &&
		np->cur_tx - np->dirty_tx > 1  &&
		(jiffies - dev->trans_start) > TX_TIMEOUT) {
		rtl8129_tx_timeout(dev);
	}


#if defined(RTL_TUNE_TWISTER)
	/* This is a complicated state machine to configure the "twister" for
	   impedance/echos based on the cable length.
	   All of this is magic and undocumented.
	   */
	if (np->twistie) switch(np->twistie) {
	case 1: {
		if (inw(ioaddr + CSCR) & CSCR_LinkOKBit) {
			/* We have link beat, let us tune the twister. */
			outw(CSCR_LinkDownOffCmd, ioaddr + CSCR);
			np->twistie = 2;	/* Change to state 2. */
			next_tick = HZ/10;
		} else {
			/* Just put in some reasonable defaults for when beat returns. */
			outw(CSCR_LinkDownCmd, ioaddr + CSCR);
			outl(0x20,ioaddr + FIFOTMS);	/* Turn on cable test mode. */
			outl(PARA78_default ,ioaddr + PARA78);
			outl(PARA7c_default ,ioaddr + PARA7c);
			np->twistie = 0;	/* Bail from future actions. */
		}
	} break;
	case 2: {
		/* Read how long it took to hear the echo. */
		int linkcase = inw(ioaddr + CSCR) & CSCR_LinkStatusBits;
		if (linkcase == 0x7000) np->twist_row = 3;
		else if (linkcase == 0x3000) np->twist_row = 2;
		else if (linkcase == 0x1000) np->twist_row = 1;
		else np->twist_row = 0;
		np->twist_col = 0;
		np->twistie = 3;	/* Change to state 2. */
		next_tick = HZ/10;
	} break;
	case 3: {
		/* Put out four tuning parameters, one per 100msec. */
		if (np->twist_col == 0) outw(0, ioaddr + FIFOTMS);
		outl(param[(int)np->twist_row][(int)np->twist_col], ioaddr + PARA7c);
		next_tick = HZ/10;
		if (++np->twist_col >= 4) {
			/* For short cables we are done.
			   For long cables (row == 3) check for mistune. */
			np->twistie = (np->twist_row == 3) ? 4 : 0;
		}
	} break;
	case 4: {
		/* Special case for long cables: check for mistune. */
		if ((inw(ioaddr + CSCR) & CSCR_LinkStatusBits) == 0x7000) {
			np->twistie = 0;
			break;
		} else {
			outl(0xfb38de03, ioaddr + PARA7c);
			np->twistie = 5;
			next_tick = HZ/10;
		}
	} break;
	case 5: {
		/* Retune for shorter cable (column 2). */
		outl(0x20,ioaddr + FIFOTMS);
		outl(PARA78_default,  ioaddr + PARA78);
		outl(PARA7c_default,  ioaddr + PARA7c);
		outl(0x00,ioaddr + FIFOTMS);
		np->twist_row = 2;
		np->twist_col = 0;
		np->twistie = 3;
		next_tick = HZ/10;
	} break;
	}
#endif

		if (debug > 2) {
			if (np->drv_flags & HAS_MII_XCVR)
				printk(KERN_DEBUG"%s: Media selection tick, GP pins %2.2x.\n",
					   dev->name, inb(ioaddr + GPPinData));
			else
				printk(KERN_DEBUG"%s: Media selection tick, Link partner %4.4x.\n",
					   dev->name, inw(ioaddr + NWayLPAR));
			printk(KERN_DEBUG"%s:  Other registers are IntMask %4.4x "
				   "IntStatus %4.4x RxStatus %4.4x.\n",
				   dev->name, inw(ioaddr + IntrMask), inw(ioaddr + IntrStatus),
				   (int)inl(ioaddr + RxEarlyStatus));
			printk(KERN_DEBUG"%s:  Chip config %2.2x %2.2x.\n",
				   dev->name, inb(ioaddr + Config0), inb(ioaddr + Config1));
		}

	start_timer( np->timer, (timer_callback *) &rtl8129_timer, dev, next_tick * 1000, true );
	/*
	np->timer.expires = jiffies + next_tick;
	add_timer(&np->timer);
	*/
}

static void rtl8129_tx_timeout(struct net_device *dev)
{
	struct rtl8129_private *tp = (struct rtl8129_private *)dev->priv;
	long ioaddr = dev->base_addr;
	int status = inw(ioaddr + IntrStatus);
	int mii_reg, i;
	int flags = spinlock_disable( &tp->tx_spinlock );

	/* Could be wrapped with if (tp->msg_level & NETIF_MSG_TX_ERR) */
	printk(KERN_ERR "%s: Transmit timeout, status %2.2x %4.4x "
		   "media %2.2x.\n",
		   dev->name, inb(ioaddr + ChipCmd), status, inb(ioaddr + GPPinData));

	if (status & (TxOK | RxOK)) {
		printk(KERN_ERR "%s: RTL8139 Interrupt line blocked, status %x.\n",
			   dev->name, status);
	}

	/* Disable interrupts by clearing the interrupt mask. */
	outw(0x0000, ioaddr + IntrMask);
	/* Emit info to figure out what went wrong. */
	printk(KERN_DEBUG "%s: Tx queue start entry %d  dirty entry %d%s.\n",
		   dev->name, tp->cur_tx, tp->dirty_tx, tp->tx_full ? ", full" : "");
	for (i = 0; i < NUM_TX_DESC; i++)
		printk(KERN_DEBUG "%s:  Tx descriptor %d is %8.8x.%s\n",
			   dev->name, i, (int)inl(ioaddr + TxStatus0 + i*4),
			   i == tp->dirty_tx % NUM_TX_DESC ? " (queue head)" : "");
	printk(KERN_DEBUG "%s: MII #%d registers are:", dev->name, tp->phys[0]);
	for (mii_reg = 0; mii_reg < 8; mii_reg++)
		printk(" %4.4x", mdio_read(dev, tp->phys[0], mii_reg));
	printk(".\n");

	/* Stop a shared interrupt from scavenging while we are. */
	tp->dirty_tx = tp->cur_tx = 0;
	/* Dump the unsent Tx packets. */
	for (i = 0; i < NUM_TX_DESC; i++) {
		if (tp->tx_skbuff[i]) {
			free_pkt_buffer(tp->tx_skbuff[i]);
			tp->tx_skbuff[i] = 0;
			tp->stats.tx_dropped++;
		}
	}
	rtl_hw_start(dev);
	netif_unpause_tx_queue(dev);
	tp->tx_full = 0;
	spinunlock_enable( &tp->tx_spinlock, flags );
	return;
}


/* Initialize the Rx and Tx rings, along with various 'dev' bits. */
static void
rtl8129_init_ring(struct net_device *dev)
{
	struct rtl8129_private *tp = (struct rtl8129_private *)dev->priv;
	int i;

	tp->tx_full = 0;
	tp->dirty_tx = tp->cur_tx = 0;

	for (i = 0; i < NUM_TX_DESC; i++) {
		tp->tx_skbuff[i] = 0;
		tp->tx_buf[i] = &tp->tx_bufs[i*TX_BUF_SIZE];
	}
}

static int
rtl8129_start_xmit(PacketBuf_s *skb, struct net_device *dev)
{
	struct rtl8129_private *tp = (struct rtl8129_private *)dev->priv;
	long ioaddr = dev->base_addr;
	int entry;
	uint32 flags;

	if (netif_pause_tx_queue(dev) != 0) {
		/* This watchdog code is redundant with the media monitor timer. */
		if (jiffies - dev->trans_start > TX_TIMEOUT)
			rtl8129_tx_timeout(dev);
		return 1;
	}

	flags = spinlock_disable( &tp->tx_spinlock );

	/* Calculate the next Tx descriptor entry. */
	entry = tp->cur_tx % NUM_TX_DESC;

	tp->tx_skbuff[entry] = skb;
	if ((long)skb->pb_pData & 3) {			/* Must use alignment buffer. */
		memcpy(tp->tx_buf[entry], skb->pb_pData, skb->pb_nSize);
		outl(virt_to_bus(tp->tx_buf[entry]), ioaddr + TxAddr0 + entry*4);
	} else
		outl(virt_to_bus(skb->pb_pData), ioaddr + TxAddr0 + entry*4);
	/* Note: the chip doesn't have auto-pad! */
	outl(tp->tx_flag | (skb->pb_nSize >= ETH_ZLEN ? skb->pb_nSize : ETH_ZLEN),
		 ioaddr + TxStatus0 + entry*4);

	/* There is a race condition here -- we might read dirty_tx, take an
	   interrupt that clears the Tx queue, and only then set tx_full.
	   So we do this in two phases. */
	if (++tp->cur_tx - tp->dirty_tx >= NUM_TX_DESC) {
		set_bit(0, &tp->tx_full);
		if (tp->cur_tx - (volatile unsigned int)tp->dirty_tx < NUM_TX_DESC) {
			clear_bit(0, &tp->tx_full);
			netif_unpause_tx_queue(dev);
		} else
			netif_stop_tx_queue(dev);
	} else
		netif_unpause_tx_queue(dev);

	dev->trans_start = jiffies;

	spinunlock_enable( &tp->tx_spinlock, flags );
	
	if (debug > 3)
		printk(KERN_DEBUG"%s: Queued Tx packet at %p size %d to slot %d.\n",
			   dev->name, skb->pb_pData, (int)skb->pb_nSize, entry);

	return 0;
}

/* The interrupt handler does all of the Rx thread work and cleans up
   after the Tx thread. */
static int rtl8129_interrupt(int irq, void *dev_instance, SysCallRegs_s *regs)
{
	struct net_device *dev = (struct net_device *)dev_instance;
	struct rtl8129_private *np = (struct rtl8129_private *)dev->priv;
	struct rtl8129_private *tp = np;
	int boguscnt = np->max_interrupt_work;
	long ioaddr = dev->base_addr;
	int link_changed = 0;		/* Grrr, avoid bogus "uninitialized" warning */
	uint32 flags;

#if defined(__i386__)  &&  LINUX_VERSION_CODE < 0x20123
	/* A lock to prevent simultaneous entry bug on Intel SMP machines. */
	if (test_and_set_bit(0, (void*)&dev->interrupt)) {
		printk(KERN_ERR"%s: SMP simultaneous entry of an interrupt handler.\n",
			   dev->name);
		dev->interrupt = 0;	/* Avoid halting machine. */
		return( 0 );
	}
#endif
	flags = spinlock_disable( &tp->tx_spinlock );
	
	do {
		int status = inw(ioaddr + IntrStatus);
		/* Acknowledge all of the current interrupt sources ASAP, but
		   an first get an additional status bit from CSCR. */
		if (status & RxUnderrun)
			link_changed = inw(ioaddr+CSCR) & CSCR_LinkChangeBit;
		outw(status, ioaddr + IntrStatus);
		
		if (debug > 4)
			printk(KERN_DEBUG"%s: interrupt  status=%#4.4x new intstat=%#4.4x.\n",
				   dev->name, status, inw(ioaddr + IntrStatus));

		if ((status & (PCIErr|PCSTimeout|RxUnderrun|RxOverflow|RxFIFOOver
					   |TxErr|TxOK|RxErr|RxOK)) == 0)
			break;
			
		
		if (status & (RxOK|RxUnderrun|RxOverflow|RxFIFOOver))/* Rx interrupt */
			rtl8129_rx(dev);

		if (status & (TxOK | TxErr)) {
			unsigned int dirty_tx = tp->dirty_tx;

			while (tp->cur_tx - dirty_tx > 0) {
				int entry = dirty_tx % NUM_TX_DESC;
				int txstatus = inl(ioaddr + TxStatus0 + entry*4);

				if ( ! (txstatus & (TxStatOK | TxUnderrun | TxAborted)))
					break;			/* It still hasn't been Txed */

				/* Note: TxCarrierLost is always asserted at 100mbps. */
				if (txstatus & (TxOutOfWindow | TxAborted)) {
					/* There was an major error, log it. */
					if (debug > 2)
						printk(KERN_NOTICE"%s: Transmit error, Tx status %8.8x.\n",
							   dev->name, txstatus);
					tp->stats.tx_errors++;
					if (txstatus&TxAborted) {
						tp->stats.tx_aborted_errors++;
						outl(TX_DMA_BURST << 8, ioaddr + TxConfig);
					}
					if (txstatus&TxCarrierLost) tp->stats.tx_carrier_errors++;
					if (txstatus&TxOutOfWindow) tp->stats.tx_window_errors++;
#ifdef ETHER_STATS
					if ((txstatus & 0x0f000000) == 0x0f000000)
						tp->stats.collisions16++;
#endif
				} else {
					if (debug > 2)
						printk(KERN_DEBUG "%s: Transmit done, Tx status"
							   " %8.8x.\n", dev->name, txstatus);
					if (txstatus & TxUnderrun) {
						/* Add 64 to the Tx FIFO threshold. */
						if (tp->tx_flag <  0x00300000)
							tp->tx_flag += 0x00020000;
						tp->stats.tx_fifo_errors++;
					}
					tp->stats.collisions += (txstatus >> 24) & 15;
#if LINUX_VERSION_CODE > 0x20119
					tp->stats.tx_bytes += txstatus & 0x7ff;
#endif
					tp->stats.tx_packets++;
				}

				/* Free the original skb. */
				free_pkt_buffer(tp->tx_skbuff[entry]);
				tp->tx_skbuff[entry] = 0;
				if (test_bit(0, &tp->tx_full)) {
					/* The ring is no longer full, clear tbusy. */
					clear_bit(0, &tp->tx_full);
					netif_resume_tx_queue(dev);
				}
				dirty_tx++;
			}

#ifndef final_version
			if (tp->cur_tx - dirty_tx > NUM_TX_DESC) {
				printk(KERN_ERR"%s: Out-of-sync dirty pointer, %d vs. %d, full=%d.\n",
					   dev->name, dirty_tx, tp->cur_tx, (int)tp->tx_full);
				dirty_tx += NUM_TX_DESC;
			}
#endif
			tp->dirty_tx = dirty_tx;
		}

		/* Check uncommon events with one test. */
		if (status & (PCIErr|PCSTimeout |RxUnderrun|RxOverflow|RxFIFOOver
					  |TxErr|RxErr)) {
			if (status == 0xffff) 			/* Missing chip! */
				break;
			rtl_error(dev, status, link_changed);
		}

		if (--boguscnt < 0) {
			printk(KERN_WARNING"%s: Too much work at interrupt, "
				   "IntrStatus=0x%4.4x.\n",
				   dev->name, status);
			/* Clear all interrupt sources. */
			outw(0xffff, ioaddr + IntrStatus);
			break;
		}
	} while (1);

	if (debug > 2)
		printk(KERN_DEBUG"%s: exiting interrupt, intr_status=%#4.4x.\n",
			   dev->name, inw(ioaddr + IntrStatus));

//#if defined(__i386__)  &&  LINUX_VERSION_CODE < 0x20123
	clear_bit(0, (void*)&dev->interrupt);
//#endif
	spinunlock_enable( &tp->tx_spinlock, flags );
	return( 0 );
}

/* The data sheet doesn't describe the Rx ring at all, so I'm guessing at the
   field alignments and semantics. */
static int rtl8129_rx(struct net_device *dev)
{
	struct rtl8129_private *tp = (struct rtl8129_private *)dev->priv;
	long ioaddr = dev->base_addr;
	unsigned char *rx_ring = tp->rx_ring;
	uint16 cur_rx = tp->cur_rx;

	if (debug > 2)
		printk(KERN_DEBUG"%s: In rtl8129_rx(), current %4.4x BufAddr %4.4x,"
			   " free to %4.4x, Cmd %2.2x.\n",
			   dev->name, cur_rx, inw(ioaddr + RxBufAddr),
			   inw(ioaddr + RxBufPtr), inb(ioaddr + ChipCmd));

	while ((inb(ioaddr + ChipCmd) & RxBufEmpty) == 0) {
		int ring_offset = cur_rx % tp->rx_buf_len;
		uint32 rx_status = le32_to_cpu(*(uint32*)(rx_ring + ring_offset));
		int rx_size = rx_status >> 16; 				/* Includes the CRC. */

		if (debug > 4) {
			int i;
			printk(KERN_DEBUG"%s:  rtl8129_rx() status %4.4x, size %4.4x,"
				   " cur %4.4x.\n",
				   dev->name, rx_status, rx_size, cur_rx);
			printk(KERN_DEBUG"%s: Frame contents ", dev->name);
			for (i = 0; i < 70; i++)
				printk(" %2.2x", rx_ring[ring_offset + i]);
			printk(".\n");
		}
		if (rx_status & (RxBadSymbol|RxRunt|RxTooLong|RxCRCErr|RxBadAlign)) {
			if (debug > 3)
				printk(KERN_DEBUG"%s: Ethernet frame had errors,"
					   " status %8.8x.\n", dev->name, rx_status);
			if (rx_status == 0xffffffff) {
				printk(KERN_NOTICE"%s: Invalid receive status at ring "
					   "offset %4.4x\n", dev->name, ring_offset);
				rx_status = 0;
			}
			if (rx_status & RxTooLong) {
				if (debug > 4)
					printk(KERN_NOTICE"%s: Oversized Ethernet frame, status"
						   " %4.4x!\n",
						   dev->name, rx_status);
				/* A.C.: The chip hangs here.
				   This should never occur, which means that we are screwed
				   when it does.
				 */
			}
			tp->stats.rx_errors++;
			if (rx_status & (RxBadSymbol|RxBadAlign))
				tp->stats.rx_frame_errors++;
			if (rx_status & (RxRunt|RxTooLong)) tp->stats.rx_length_errors++;
			if (rx_status & RxCRCErr) tp->stats.rx_crc_errors++;
			/* Reset the receiver, based on RealTek recommendation. (Bug?) */
			tp->cur_rx = 0;
			outb(CmdTxEnb, ioaddr + ChipCmd);
			/* A.C.: Reset the multicast list. */
			set_rx_mode(dev);
			outb(CmdRxEnb | CmdTxEnb, ioaddr + ChipCmd);
		} else {
			/* Malloc up new buffer, compatible with net-2e. */
			/* Omit the four octet CRC from the length. */
			PacketBuf_s *skb;
			int pkt_size = rx_size - 4;

			/* Allocate a common-sized skbuff if we are close. */
			skb = alloc_pkt_buffer(1400 < pkt_size && pkt_size < PKT_BUF_SZ-2 ?
								PKT_BUF_SZ : pkt_size + 2);
			if (skb == NULL) {
				printk(KERN_WARNING"%s: Memory squeeze, deferring packet.\n",
					   dev->name);
				/* We should check that some rx space is free.
				   If not, free one and mark stats->rx_dropped++. */
				tp->stats.rx_dropped++;
				break;
			}
			//skb->dev = dev;
			//skb_reserve(skb, 2);	/* 16 byte align the IP fields. */
			skb->pb_nSize = 0;
			if (ring_offset + rx_size > tp->rx_buf_len) {
				int semi_count = tp->rx_buf_len - ring_offset - 4;
				/* This could presumably use two calls to copy_and_sum()? */
				memcpy(skb_put(skb, semi_count), &rx_ring[ring_offset + 4],
					   semi_count);
				memcpy(skb_put(skb, pkt_size-semi_count), rx_ring,
					   pkt_size-semi_count);
				if (debug > 4) {
					int i;
					printk(KERN_DEBUG"%s:  Frame wrap @%d",
						   dev->name, semi_count);
					for (i = 0; i < 16; i++)
						printk(" %2.2x", rx_ring[i]);
					printk(".\n");
					memset(rx_ring, 0xcc, 16);
				}
			} else {
				memcpy(skb_put(skb, pkt_size), &rx_ring[ring_offset + 4],
					   pkt_size);
			}
			//skb->protocol = eth_type_trans(skb, dev);
			//netif_rx(skb);
			
			if ( dev->packet_queue != NULL ) {
				skb->pb_uMacHdr.pRaw = skb->pb_pData;
				enqueue_packet( dev->packet_queue, skb );
			} else {
				printk( "Error: rtl8129_rx() received packet to downed device!\n" );
				free_pkt_buffer( skb );
			}
#if LINUX_VERSION_CODE > 0x20119
			tp->stats.rx_bytes += pkt_size;
#endif
			tp->stats.rx_packets++;
		}

		cur_rx = (cur_rx + rx_size + 4 + 3) & ~3;
		outw(cur_rx - 16, ioaddr + RxBufPtr);
	}
	if (debug > 4)
		printk(KERN_DEBUG"%s: Done rtl8129_rx(), current %4.4x BufAddr %4.4x,"
			   " free to %4.4x, Cmd %2.2x.\n",
			   dev->name, cur_rx, inw(ioaddr + RxBufAddr),
			   inw(ioaddr + RxBufPtr), inb(ioaddr + ChipCmd));
	tp->cur_rx = cur_rx;
	return 0;
}

/* Error and abnormal or uncommon events handlers. */
static void rtl_error(struct net_device *dev, int status, int link_changed)
{
	struct rtl8129_private *tp = (struct rtl8129_private *)dev->priv;
	long ioaddr = dev->base_addr;

	if (debug > 4)
		printk(KERN_NOTICE"%s: Abnormal interrupt, status %8.8x.\n",
			   dev->name, status);

	/* Update the error count. */
	tp->stats.rx_missed_errors += inl(ioaddr + RxMissed);
	outl(0, ioaddr + RxMissed);

	if (status & RxUnderrun){
		/* This might actually be a link change event. */
		if ((tp->drv_flags & HAS_LNK_CHNG)  &&  link_changed) {
			/* Really link-change on new chips. */
			int lpar = inw(ioaddr + NWayLPAR);
			int duplex = (lpar&0x0100) || (lpar & 0x01C0) == 0x0040
				|| tp->duplex_lock;
			/* Do not use MII_BMSR as that clears sticky bit. */
			if (inw(ioaddr + GPPinData) & 0x0004) {
				netif_link_down(dev);
			} else
				netif_link_up(dev);
			if (debug > 4)
				printk(KERN_DEBUG "%s: Link changed, link partner "
					   "%4.4x new duplex %d.\n",
					   dev->name, lpar, duplex);
			tp->full_duplex = duplex;
			/* Only count as errors with no link change. */
			status &= ~RxUnderrun;
		} else {
			/* If this does not work, we will do rtl_hw_start(dev); */
			outb(CmdTxEnb, ioaddr + ChipCmd);
			set_rx_mode(dev);	/* Reset the multicast list. */
			outb(CmdRxEnb | CmdTxEnb, ioaddr + ChipCmd);

			tp->stats.rx_errors++;
			tp->stats.rx_fifo_errors++;
		}
	}
	
	if (status & (RxOverflow | RxErr | RxFIFOOver)) tp->stats.rx_errors++;
	if (status & (PCSTimeout)) tp->stats.rx_length_errors++;
	if (status & RxFIFOOver) tp->stats.rx_fifo_errors++;
	if (status & RxOverflow) {
		tp->stats.rx_over_errors++;
		tp->cur_rx = inw(ioaddr + RxBufAddr) % tp->rx_buf_len;
		outw(tp->cur_rx - 16, ioaddr + RxBufPtr);
	}
	if (status & PCIErr) {
		//uint32 pci_cmd_status;
		//pci_read_config_dword(tp->pci_dev, PCI_COMMAND, pci_cmd_status);

		printk(KERN_ERR "%s: PCI Bus error %4.4x.\n", dev->name);
	}
}

static int
rtl8129_close(struct net_device *dev)
{
	long ioaddr = dev->base_addr;
	struct rtl8129_private *tp = (struct rtl8129_private *)dev->priv;
	int i;

	netif_stop_tx_queue(dev);

	if (debug > 3)
		printk(KERN_DEBUG"%s: Shutting down ethercard, status was 0x%4.4x.\n",
			   dev->name, inw(ioaddr + IntrStatus));

	/* Disable interrupts by clearing the interrupt mask. */
	outw(0x0000, ioaddr + IntrMask);

	/* Stop the chip's Tx and Rx DMA processes. */
	outb(0x00, ioaddr + ChipCmd);

	/* Update the error counts. */
	tp->stats.rx_missed_errors += inl(ioaddr + RxMissed);
	outl(0, ioaddr + RxMissed);

	delete_timer(tp->timer);
	tp->timer = create_timer();
	
	release_irq(dev->irq, dev->irq_handle);

	for (i = 0; i < NUM_TX_DESC; i++) {
		if (tp->tx_skbuff[i])
			free_pkt_buffer(tp->tx_skbuff[i]);
		tp->tx_skbuff[i] = 0;
	}
	kfree(tp->rx_ring);
	tp->rx_ring = 0;

	/* Green! Put the chip in low-power mode. */
	outb(0xC0, ioaddr + Cfg9346);
	outb(tp->config1 | 0x03, ioaddr + Config1);
	outb('H', ioaddr + HltClk);		/* 'R' would leave the clock running. */

	//MOD_DEC_USE_COUNT;

	return 0;
}

#ifndef __ATHEOS__
/*
  Handle user-level ioctl() calls.
  We must use two numeric constants as the key because some clueless person
  changed value for the symbolic name.
*/
static int mii_ioctl(struct net_device *dev, struct ifreq *rq, int cmd)
{
	struct rtl8129_private *np = (struct rtl8129_private *)dev->priv;
	uint16 *data = (uint16 *)&rq->ifr_data;
	uint32 *data32 = (void *)&rq->ifr_data;

	switch(cmd) {
	case 0x8947: case 0x89F0:
		/* SIOCGMIIPHY: Get the address of the PHY in use. */
		data[0] = np->phys[0] & 0x3f;
		/* Fall Through */
	case 0x8948: case 0x89F1:
		/* SIOCGMIIREG: Read the specified MII register. */
		data[3] = mdio_read(dev, data[0], data[1] & 0x1f);
		return 0;
	case 0x8949: case 0x89F2:
		/* SIOCSMIIREG: Write the specified MII register */
		if (!capable(CAP_NET_ADMIN))
			return -EPERM;
		if (data[0] == np->phys[0]) {
			uint16 value = data[2];
			switch (data[1]) {
			case 0:
				/* Check for autonegotiation on or reset. */
				np->medialock = (value & 0x9000) ? 0 : 1;
				if (np->medialock)
					np->full_duplex = (value & 0x0100) ? 1 : 0;
				break;
			case 4: np->advertising = value; break;
			}
		}
		mdio_write(dev, data[0], data[1] & 0x1f, data[2]);
		return 0;
	case SIOCGPARAMS:
		data32[0] = np->msg_level;
		data32[1] = np->multicast_filter_limit;
		data32[2] = np->max_interrupt_work;
		data32[3] = 0;			/* No rx_copybreak, always copy. */
		return 0;
	case SIOCSPARAMS:
		if (!capable(CAP_NET_ADMIN))
			return -EPERM;
		np->msg_level = data32[0];
		np->multicast_filter_limit = data32[1];
		np->max_interrupt_work = data32[2];
		return 0;
	default:
		return -EOPNOTSUPP;
	}
}

static struct net_device_stats *
rtl8129_get_stats(struct net_device *dev)
{
	struct rtl8129_private *tp = (struct rtl8129_private *)dev->priv;
	long ioaddr = dev->base_addr;

	if (netif_running(dev)) {
		tp->stats.rx_missed_errors += inl(ioaddr + RxMissed);
		outl(0, ioaddr + RxMissed);
	}

	return &tp->stats;
}

#endif /* __ATHEOS__ */

/* Set or clear the multicast filter for this adaptor.
   This routine is not state sensitive and need not be SMP locked. */

static unsigned const ethernet_polynomial = 0x04c11db7U;
static inline uint32 ether_crc(int length, unsigned char *data)
{
	int crc = -1;

	while (--length >= 0) {
		unsigned char current_octet = *data++;
		int bit;
		for (bit = 0; bit < 8; bit++, current_octet >>= 1)
			crc = (crc << 1) ^
				((crc < 0) ^ (current_octet & 1) ? ethernet_polynomial : 0);
	}
	return crc;
}

/* Bits in RxConfig. */
enum rx_mode_bits {
	AcceptErr=0x20, AcceptRunt=0x10, AcceptBroadcast=0x08,
	AcceptMulticast=0x04, AcceptMyPhys=0x02, AcceptAllPhys=0x01,
};

static void set_rx_mode(struct net_device *dev)
{
	struct rtl8129_private *tp = (struct rtl8129_private *)dev->priv;
	long ioaddr = dev->base_addr;
	uint32 mc_filter[2];		 /* Multicast hash filter */
	int i, rx_mode;

	if (debug > 4)
		printk(KERN_DEBUG"%s:   set_rx_mode(%4.4x) done -- Rx config %8.8x.\n",
			   dev->name, dev->flags, (int)inl(ioaddr + RxConfig));

#ifndef __ATHEOS__
	/* Note: do not reorder, GCC is clever about common statements. */
	if (dev->flags & IFF_PROMISC) {
		/* Unconditionally log net taps. */
		printk(KERN_NOTICE"%s: Promiscuous mode enabled.\n", dev->name);
		rx_mode = AcceptBroadcast|AcceptMulticast|AcceptMyPhys|AcceptAllPhys;
		mc_filter[1] = mc_filter[0] = 0xffffffff;
	} else if ((dev->mc_count > tp->multicast_filter_limit)
			   || (dev->flags & IFF_ALLMULTI)) {
		/* Too many to filter perfectly -- accept all multicasts. */
		rx_mode = AcceptBroadcast | AcceptMulticast | AcceptMyPhys;
		mc_filter[1] = mc_filter[0] = 0xffffffff;
	} else {
		struct dev_mc_list *mclist;
		rx_mode = AcceptBroadcast | AcceptMulticast | AcceptMyPhys;
		mc_filter[1] = mc_filter[0] = 0;
		for (i = 0, mclist = dev->mc_list; mclist && i < dev->mc_count;
			 i++, mclist = mclist->next)
			set_bit(ether_crc(ETH_ALEN, mclist->dmi_addr) >> 26, mc_filter);
	}
#else /* !__ATHEOS__ */
	rx_mode = AcceptBroadcast | AcceptMulticast | AcceptMyPhys;
#endif
	/* We can safely update without stopping the chip. */
	outl(tp->rx_config | rx_mode, ioaddr + RxConfig);
	tp->mc_filter[0] = mc_filter[0];
	tp->mc_filter[1] = mc_filter[1];
	outl(mc_filter[0], ioaddr + MAR0 + 0);
	outl(mc_filter[1], ioaddr + MAR0 + 4);
	return;
}


static status_t rtl8129_open_dev( void* pNode, uint32 nFlags, void **pCookie )
{
    return( 0 );
}

static status_t rtl8129_close_dev( void* pNode, void* pCookie )
{
    return( 0 );
}

static status_t rtl8129_ioctl( void* pNode, void* pCookie, uint32 nCommand, void* pArgs, bool bFromKernel )
{
	struct net_device* psDev = pNode;
    int nError = 0;

    switch( nCommand )
    {
		case SIOC_ETH_START:
		{
			psDev->packet_queue = pArgs;
			rtl8129_open( psDev );
			break;
		}
		case SIOC_ETH_STOP:
		{
			rtl8129_close( psDev );
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
			printk( "Error: rtl8129_ioctl() unknown command %d\n", nCommand );
			nError = -ENOSYS;
			break;
    }

    return( nError );
}

static int rtl8129_read( void* pNode, void* pCookie, off_t nPosition, void* pBuffer, size_t nSize )
{
    return( -ENOSYS );
}

static int rtl8129_write( void* pNode, void* pCookie, off_t nPosition, const void* pBuffer, size_t nSize )
{
	struct net_device* dev = pNode;
	PacketBuf_s* psBuffer = alloc_pkt_buffer( nSize );
	if ( psBuffer != NULL ) {
		memcpy( psBuffer->pb_pData, pBuffer, nSize );
		rtl8129_start_xmit( psBuffer, dev );
	}
	return( nSize );
}


static DeviceOperations_s g_sDevOps = {
    rtl8129_open_dev,
    rtl8129_close_dev,
    rtl8129_ioctl,
    rtl8129_read,
    rtl8129_write,
    NULL,	// dop_readv
    NULL,	// dop_writev
    NULL,	// dop_add_select_req
    NULL	// dop_rem_select_req
};

status_t device_suspend( int nDeviceID, int nDeviceHandle, void* pData )
{
	struct net_device* dev = pData;
	long ioaddr = dev->base_addr;
	struct rtl8129_private *tp = (struct rtl8129_private *)dev->priv;
	
	printk( "Suspend %s @ 0x%x\n", dev->name, (uint)ioaddr );
	
	outw(0, ioaddr + IntrMask);
	outw(0, ioaddr + ChipCmd);
	
	tp->stats.rx_missed_errors += inl(ioaddr + RxMissed);
	outl(0, ioaddr + RxMissed);
	
	return( 0 );
}

status_t device_resume( int nDeviceID, int nDeviceHandle, void* pData )
{
	struct net_device* dev = pData;
	long ioaddr = dev->base_addr;
	struct rtl8129_private *tp = (struct rtl8129_private *)dev->priv;
	
	printk( "Resume %s @ 0x%x\n", dev->name, (uint)ioaddr );
	
	rtl8129_init_ring (dev);
	rtl_hw_start (dev);
	
	return( 0 );
}

status_t device_init( int nDeviceID )
{
	/* Get PCI bus */
    g_psBus = get_busmanager( PCI_BUS_NAME, PCI_BUS_VERSION );

	printk(KERN_INFO "%s" KERN_INFO "%s", versionA, versionB);
    
    if( g_psBus == NULL )
    	return( -1 );
	return( rtl8139_probe( nDeviceID ) );
}

status_t device_uninit( int nDeviceID )
{
    return( 0 );
}

/*
 * Local variables:
 *  compile-command: "make KERNVER=`uname -r` rtl8139.o"
 *  compile-cmd: "gcc -DMODULE -Wall -Wstrict-prototypes -O6 -c rtl8139.c"
 *  cardbus-compile-command: "gcc -DCARDBUS -DMODULE -Wall -Wstrict-prototypes -O6 -c rtl8139.c -o realtek_cb.o -I/usr/src/pcmcia/include/"
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  tab-width: 4
 * End:
 */
