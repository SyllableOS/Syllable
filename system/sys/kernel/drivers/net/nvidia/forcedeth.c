/*
 * forcedeth: Ethernet driver for NVIDIA nForce media access controllers.
 *
 * Note: This driver is a cleanroom reimplementation based on reverse
 *      engineered documentation written by Carl-Daniel Hailfinger
 *      and Andrew de Quincey. It's neither supported nor endorsed
 *      by NVIDIA Corp. Use at your own risk.
 *
 * NVIDIA, nForce and other NVIDIA marks are trademarks or registered
 * trademarks of NVIDIA Corporation in the United States and other
 * countries.
 *
 * Copyright (C) 2003 Manfred Spraul
 * Copyright (C) 2004 Andrew de Quincey (wol support)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Changelog:
 * 	0.01: 05 Oct 2003: First release that compiles without warnings.
 * 	0.02: 05 Oct 2003: Fix bug for nv_drain_tx: do not try to free NULL skbs.
 * 			   Check all PCI BARs for the register window.
 * 			   udelay added to mii_rw.
 * 	0.03: 06 Oct 2003: Initialize dev->irq.
 * 	0.04: 07 Oct 2003: Initialize np->lock, reduce handled irqs, add printks.
 * 	0.05: 09 Oct 2003: printk removed again, irq status print tx_timeout.
 * 	0.06: 10 Oct 2003: MAC Address read updated, pff flag generation updated,
 * 			   irq mask updated
 * 	0.07: 14 Oct 2003: Further irq mask updates.
 * 	0.08: 20 Oct 2003: rx_desc.Length initialization added, nv_alloc_rx refill
 * 			   added into irq handler, NULL check for drain_ring.
 * 	0.09: 20 Oct 2003: Basic link speed irq implementation. Only handle the
 * 			   requested interrupt sources.
 * 	0.10: 20 Oct 2003: First cleanup for release.
 * 	0.11: 21 Oct 2003: hexdump for tx added, rx buffer sizes increased.
 * 			   MAC Address init fix, set_multicast cleanup.
 * 	0.12: 23 Oct 2003: Cleanups for release.
 * 	0.13: 25 Oct 2003: Limit for concurrent tx packets increased to 10.
 * 			   Set link speed correctly. start rx before starting
 * 			   tx (nv_start_rx sets the link speed).
 * 	0.14: 25 Oct 2003: Nic dependant irq mask.
 * 	0.15: 08 Nov 2003: fix smp deadlock with set_multicast_list during
 * 			   open.
 * 	0.16: 15 Nov 2003: include file cleanup for ppc64, rx buffer size
 * 			   increased to 1628 bytes.
 * 	0.17: 16 Nov 2003: undo rx buffer size increase. Substract 1 from
 * 			   the tx length.
 * 	0.18: 17 Nov 2003: fix oops due to late initialization of dev_stats
 * 	0.19: 29 Nov 2003: Handle RxNoBuf, detect & handle invalid mac
 * 			   addresses, really stop rx if already running
 * 			   in nv_start_rx, clean up a bit.
 * 				(C) Carl-Daniel Hailfinger
 * 	0.20: 07 Dec 2003: alloc fixes
 * 	0.21: 12 Jan 2004: additional alloc fix, nic polling fix.
 *	0.22: 19 Jan 2004: reprogram timer to a sane rate, avoid lockup
 * 			   on close.
 * 				(C) Carl-Daniel Hailfinger, Manfred Spraul
 *	0.23: 26 Jan 2004: various small cleanups
 *	0.24: 27 Feb 2004: make driver even less anonymous in backtraces
 *	0.25: 09 Mar 2004: wol support
 *	0.26: 01 Sep 2004: port to Syllable
 *
 * Known bugs:
 * We suspect that on some hardware no TX done interrupts are generated.
 * This means recovery from netif_stop_queue only happens if the hw timer
 * interrupt fires (100 times/second, configurable with NVREG_POLL_DEFAULT)
 * and the timer is active in the IRQMask, or if a rx packet arrives by chance.
 * If your hardware reliably generates tx done interrupts, then you can remove
 * DEV_NEED_TIMERIRQ from the driver_data flags.
 * DEV_NEED_TIMERIRQ will not harm you on sane hardware, only generating a few
 * superfluous timer interrupts from the nic.
 */
#define FORCEDETH_VERSION		"0.26"

/* AtheOS includes */
#include <atheos/kernel.h>
#include <atheos/kdebug.h>
#include <atheos/irq.h>
#include <atheos/isa_io.h>
#include <atheos/udelay.h>
#include <atheos/time.h>
#include <atheos/timer.h>
#include <atheos/pci.h>
#include <atheos/random.h>
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

static PCI_bus_s *g_psBus;
static DeviceOperations_s g_sDevOps;

#include "linuxcomp.h"

#if 0
#define dprintk			printk
#else
#define dprintk(x...)		do { } while (0)
#endif

/*
 * Hardware access:
 */

#define DEV_NEED_LASTPACKET1	0x0001
#define DEV_IRQMASK_1		0x0002
#define DEV_IRQMASK_2		0x0004
#define DEV_NEED_TIMERIRQ	0x0008

enum {
	NvRegIrqStatus = 0x000,
#define NVREG_IRQSTAT_MIIEVENT	0x040
#define NVREG_IRQSTAT_MASK		0x1ff
	NvRegIrqMask = 0x004,
#define NVREG_IRQ_RX			0x0002
#define NVREG_IRQ_RX_NOBUF		0x0004
#define NVREG_IRQ_TX_ERR		0x0008
#define NVREG_IRQ_TX2			0x0010
#define NVREG_IRQ_TIMER			0x0020
#define NVREG_IRQ_LINK			0x0040
#define NVREG_IRQ_TX1			0x0100
#define NVREG_IRQMASK_WANTED_1		0x005f
#define NVREG_IRQMASK_WANTED_2		0x0147
#define NVREG_IRQ_UNKNOWN		(~(NVREG_IRQ_RX|NVREG_IRQ_RX_NOBUF|NVREG_IRQ_TX_ERR|NVREG_IRQ_TX2|NVREG_IRQ_TIMER|NVREG_IRQ_LINK|NVREG_IRQ_TX1))

	NvRegUnknownSetupReg6 = 0x008,
#define NVREG_UNKSETUP6_VAL		3

/*
 * NVREG_POLL_DEFAULT is the interval length of the timer source on the nic
 * NVREG_POLL_DEFAULT=97 would result in an interval length of 1 ms
 */
	NvRegPollingInterval = 0x00c,
#define NVREG_POLL_DEFAULT	970
	NvRegMisc1 = 0x080,
#define NVREG_MISC1_HD		0x02
#define NVREG_MISC1_FORCE	0x3b0f3c

	NvRegTransmitterControl = 0x084,
#define NVREG_XMITCTL_START	0x01
	NvRegTransmitterStatus = 0x088,
#define NVREG_XMITSTAT_BUSY	0x01

	NvRegPacketFilterFlags = 0x8c,
#define NVREG_PFF_ALWAYS	0x7F0008
#define NVREG_PFF_PROMISC	0x80
#define NVREG_PFF_MYADDR	0x20

	NvRegOffloadConfig = 0x90,
#define NVREG_OFFLOAD_HOMEPHY	0x601
#define NVREG_OFFLOAD_NORMAL	0x5ee
	NvRegReceiverControl = 0x094,
#define NVREG_RCVCTL_START	0x01
	NvRegReceiverStatus = 0x98,
#define NVREG_RCVSTAT_BUSY	0x01

	NvRegRandomSeed = 0x9c,
#define NVREG_RNDSEED_MASK	0x00ff
#define NVREG_RNDSEED_FORCE	0x7f00

	NvRegUnknownSetupReg1 = 0xA0,
#define NVREG_UNKSETUP1_VAL	0x16070f
	NvRegUnknownSetupReg2 = 0xA4,
#define NVREG_UNKSETUP2_VAL	0x16
	NvRegMacAddrA = 0xA8,
	NvRegMacAddrB = 0xAC,
	NvRegMulticastAddrA = 0xB0,
#define NVREG_MCASTADDRA_FORCE	0x01
	NvRegMulticastAddrB = 0xB4,
	NvRegMulticastMaskA = 0xB8,
	NvRegMulticastMaskB = 0xBC,

	NvRegTxRingPhysAddr = 0x100,
	NvRegRxRingPhysAddr = 0x104,
	NvRegRingSizes = 0x108,
#define NVREG_RINGSZ_TXSHIFT 0
#define NVREG_RINGSZ_RXSHIFT 16
	NvRegUnknownTransmitterReg = 0x10c,
	NvRegLinkSpeed = 0x110,
#define NVREG_LINKSPEED_FORCE 0x10000
#define NVREG_LINKSPEED_10	10
#define NVREG_LINKSPEED_100	100
#define NVREG_LINKSPEED_1000	1000
	NvRegUnknownSetupReg5 = 0x130,
#define NVREG_UNKSETUP5_BIT31	(1<<31)
	NvRegUnknownSetupReg3 = 0x134,
#define NVREG_UNKSETUP3_VAL1	0x200010
	NvRegTxRxControl = 0x144,
#define NVREG_TXRXCTL_KICK	0x0001
#define NVREG_TXRXCTL_BIT1	0x0002
#define NVREG_TXRXCTL_BIT2	0x0004
#define NVREG_TXRXCTL_IDLE	0x0008
#define NVREG_TXRXCTL_RESET	0x0010
	NvRegMIIStatus = 0x180,
#define NVREG_MIISTAT_ERROR		0x0001
#define NVREG_MIISTAT_LINKCHANGE	0x0008
#define NVREG_MIISTAT_MASK		0x000f
#define NVREG_MIISTAT_MASK2		0x000f
	NvRegUnknownSetupReg4 = 0x184,
#define NVREG_UNKSETUP4_VAL	8

	NvRegAdapterControl = 0x188,
#define NVREG_ADAPTCTL_START	0x02
#define NVREG_ADAPTCTL_LINKUP	0x04
#define NVREG_ADAPTCTL_PHYVALID	0x4000
#define NVREG_ADAPTCTL_RUNNING	0x100000
#define NVREG_ADAPTCTL_PHYSHIFT	24
	NvRegMIISpeed = 0x18c,
#define NVREG_MIISPEED_BIT8	(1<<8)
#define NVREG_MIIDELAY	5
	NvRegMIIControl = 0x190,
#define NVREG_MIICTL_INUSE	0x10000
#define NVREG_MIICTL_WRITE	0x08000
#define NVREG_MIICTL_ADDRSHIFT	5
	NvRegMIIData = 0x194,
	NvRegWakeUpFlags = 0x200,
#define NVREG_WAKEUPFLAGS_VAL		0x7770
#define NVREG_WAKEUPFLAGS_BUSYSHIFT	24
#define NVREG_WAKEUPFLAGS_ENABLESHIFT	16
#define NVREG_WAKEUPFLAGS_D3SHIFT	12
#define NVREG_WAKEUPFLAGS_D2SHIFT	8
#define NVREG_WAKEUPFLAGS_D1SHIFT	4
#define NVREG_WAKEUPFLAGS_D0SHIFT	0
#define NVREG_WAKEUPFLAGS_ACCEPT_MAGPAT		0x01
#define NVREG_WAKEUPFLAGS_ACCEPT_WAKEUPPAT	0x02
#define NVREG_WAKEUPFLAGS_ACCEPT_LINKCHANGE	0x04
#define NVREG_WAKEUPFLAGS_ENABLE	0x1111

	NvRegPatternCRC = 0x204,
	NvRegPatternMask = 0x208,
	NvRegPowerCap = 0x268,
#define NVREG_POWERCAP_D3SUPP	(1<<30)
#define NVREG_POWERCAP_D2SUPP	(1<<26)
#define NVREG_POWERCAP_D1SUPP	(1<<25)
	NvRegPowerState = 0x26c,
#define NVREG_POWERSTATE_POWEREDUP	0x8000
#define NVREG_POWERSTATE_VALID		0x0100
#define NVREG_POWERSTATE_MASK		0x0003
#define NVREG_POWERSTATE_D0		0x0000
#define NVREG_POWERSTATE_D1		0x0001
#define NVREG_POWERSTATE_D2		0x0002
#define NVREG_POWERSTATE_D3		0x0003
};

struct ring_desc {
	u32 PacketBuffer;
	u16 Length;
	u16 Flags;
};

#define NV_TX_LASTPACKET	(1<<0)
#define NV_TX_RETRYERROR	(1<<3)
#define NV_TX_LASTPACKET1	(1<<8)
#define NV_TX_DEFERRED		(1<<10)
#define NV_TX_CARRIERLOST	(1<<11)
#define NV_TX_LATECOLLISION	(1<<12)
#define NV_TX_UNDERFLOW		(1<<13)
#define NV_TX_ERROR		(1<<14)
#define NV_TX_VALID		(1<<15)

#define NV_RX_DESCRIPTORVALID	(1<<0)
#define NV_RX_MISSEDFRAME	(1<<1)
#define NV_RX_SUBSTRACT1	(1<<3)
#define NV_RX_ERROR1		(1<<7)
#define NV_RX_ERROR2		(1<<8)
#define NV_RX_ERROR3		(1<<9)
#define NV_RX_ERROR4		(1<<10)
#define NV_RX_CRCERR		(1<<11)
#define NV_RX_OVERFLOW		(1<<12)
#define NV_RX_FRAMINGERR	(1<<13)
#define NV_RX_ERROR		(1<<14)
#define NV_RX_AVAIL		(1<<15)

/* Miscelaneous hardware related defines: */
#define NV_PCI_REGSZ		0x270

/* various timeout delays: all in usec */
#define NV_TXRX_RESET_DELAY	4
#define NV_TXSTOP_DELAY1	10
#define NV_TXSTOP_DELAY1MAX	500000
#define NV_TXSTOP_DELAY2	100
#define NV_RXSTOP_DELAY1	10
#define NV_RXSTOP_DELAY1MAX	500000
#define NV_RXSTOP_DELAY2	100
#define NV_SETUP5_DELAY		5
#define NV_SETUP5_DELAYMAX	50000
#define NV_POWERUP_DELAY	5
#define NV_POWERUP_DELAYMAX	5000
#define NV_MIIBUSY_DELAY	50
#define NV_MIIPHY_DELAY	10
#define NV_MIIPHY_DELAYMAX	10000

#define NV_WAKEUPPATTERNS	5
#define NV_WAKEUPMASKENTRIES	4

/* General driver defaults */
#define NV_WATCHDOG_TIMEO	(5*HZ)
#define DEFAULT_MTU		1500	/* also maximum supported, at least for now */

#define RX_RING		128
#define TX_RING		16
/* limited to 1 packet until we understand NV_TX_LASTPACKET */
#define TX_LIMIT_STOP	10
#define TX_LIMIT_START	5

/* rx/tx mac addr + type + vlan + align + slack*/
#define RX_NIC_BUFSIZE		(DEFAULT_MTU + 64)
/* even more slack */
#define RX_ALLOC_BUFSIZE	(DEFAULT_MTU + 128)

#define OOM_REFILL	(1+HZ/20)
#define POLL_WAIT	(1+HZ/100)


#define MII_BMCR            0x00        /* Basic mode control register */
#define MII_BMSR            0x01        /* Basic mode status register  */
#define MII_PHYSID1         0x02        /* PHYS ID 1                   */
#define MII_PHYSID2         0x03        /* PHYS ID 2                   */
#define MII_ADVERTISE       0x04        /* Advertisement control reg   */
#define MII_LPA             0x05        /* Link partner ability reg    */
#define MII_EXPANSION       0x06        /* Expansion register          */
#define MII_DCOUNTER        0x12        /* Disconnect counter          */
#define MII_FCSCOUNTER      0x13        /* False carrier counter       */
#define MII_NWAYTEST        0x14        /* N-way auto-neg test reg     */
#define MII_RERRCOUNTER     0x15        /* Receive error counter       */
#define MII_SREVISION       0x16        /* Silicon revision            */
#define MII_RESV1           0x17        /* Reserved...                 */
#define MII_LBRERROR        0x18        /* Lpback, rx, bypass error    */
#define MII_PHYADDR         0x19        /* PHY address                 */
#define MII_RESV2           0x1a        /* Reserved...                 */
#define MII_TPISTATUS       0x1b        /* TPI status for 10mbps       */
#define MII_NCONFIG         0x1c        /* Network interface config    */

/* Basic mode control register. */
#define BMCR_RESV               0x007f  /* Unused...                   */
#define BMCR_CTST               0x0080  /* Collision test              */
#define BMCR_FULLDPLX           0x0100  /* Full duplex                 */
#define BMCR_ANRESTART          0x0200  /* Auto negotiation restart    */
#define BMCR_ISOLATE            0x0400  /* Disconnect DP83840 from MII */
#define BMCR_PDOWN              0x0800  /* Powerdown the DP83840       */
#define BMCR_ANENABLE           0x1000  /* Enable auto negotiation     */
#define BMCR_SPEED100           0x2000  /* Select 100Mbps              */
#define BMCR_LOOPBACK           0x4000  /* TXD loopback bits           */
#define BMCR_RESET              0x8000  /* Reset the DP83840           */

/* Basic mode status register. */
#define BMSR_ERCAP              0x0001  /* Ext-reg capability          */
#define BMSR_JCD                0x0002  /* Jabber detected             */
#define BMSR_LSTATUS            0x0004  /* Link status                 */
#define BMSR_ANEGCAPABLE        0x0008  /* Able to do auto-negotiation */
#define BMSR_RFAULT             0x0010  /* Remote fault detected       */
#define BMSR_ANEGCOMPLETE       0x0020  /* Auto-negotiation complete   */
#define BMSR_RESV               0x07c0  /* Unused...                   */
#define BMSR_10HALF             0x0800  /* Can do 10mbps, half-duplex  */
#define BMSR_10FULL             0x1000  /* Can do 10mbps, full-duplex  */
#define BMSR_100HALF            0x2000  /* Can do 100mbps, half-duplex */
#define BMSR_100FULL            0x4000  /* Can do 100mbps, full-duplex */
#define BMSR_100BASE4           0x8000  /* Can do 100mbps, 4k packets  */

/* Advertisement control register. */
#define ADVERTISE_SLCT          0x001f  /* Selector bits               */
#define ADVERTISE_CSMA          0x0001  /* Only selector supported     */
#define ADVERTISE_10HALF        0x0020  /* Try for 10mbps half-duplex  */
#define ADVERTISE_10FULL        0x0040  /* Try for 10mbps full-duplex  */
#define ADVERTISE_100HALF       0x0080  /* Try for 100mbps half-duplex */
#define ADVERTISE_100FULL       0x0100  /* Try for 100mbps full-duplex */
#define ADVERTISE_100BASE4      0x0200  /* Try for 100mbps 4k packets  */
#define ADVERTISE_RESV          0x1c00  /* Unused...                   */
#define ADVERTISE_RFAULT        0x2000  /* Say we can detect faults    */
#define ADVERTISE_LPACK         0x4000  /* Ack link partners response  */
#define ADVERTISE_NPAGE         0x8000  /* Next page bit               */

#define ADVERTISE_FULL (ADVERTISE_100FULL | ADVERTISE_10FULL | \
			ADVERTISE_CSMA)
#define ADVERTISE_ALL (ADVERTISE_10HALF | ADVERTISE_10FULL | \
                       ADVERTISE_100HALF | ADVERTISE_100FULL)

/* Link partner ability register. */
#define LPA_SLCT                0x001f  /* Same as advertise selector  */
#define LPA_10HALF              0x0020  /* Can do 10mbps half-duplex   */
#define LPA_10FULL              0x0040  /* Can do 10mbps full-duplex   */
#define LPA_100HALF             0x0080  /* Can do 100mbps half-duplex  */
#define LPA_100FULL             0x0100  /* Can do 100mbps full-duplex  */
#define LPA_100BASE4            0x0200  /* Can do 100mbps 4k packets   */
#define LPA_RESV                0x1c00  /* Unused...                   */
#define LPA_RFAULT              0x2000  /* Link partner faulted        */
#define LPA_LPACK               0x4000  /* Link partner acked us       */
#define LPA_NPAGE               0x8000  /* Next page bit               */

#define LPA_DUPLEX		(LPA_10FULL | LPA_100FULL)
#define LPA_100			(LPA_100FULL | LPA_100HALF | LPA_100BASE4)

/* Expansion register for auto-negotiation. */
#define EXPANSION_NWAY          0x0001  /* Can do N-way auto-nego      */
#define EXPANSION_LCWP          0x0002  /* Got new RX page code word   */
#define EXPANSION_ENABLENPAGE   0x0004  /* This enables npage words    */
#define EXPANSION_NPCAPABLE     0x0008  /* Link partner supports npage */
#define EXPANSION_MFAULTS       0x0010  /* Multiple faults detected    */
#define EXPANSION_RESV          0xffe0  /* Unused...                   */

/* N-way test register. */
#define NWAYTEST_RESV1          0x00ff  /* Unused...                   */
#define NWAYTEST_LOOPBACK       0x0100  /* Enable loopback for N-way   */
#define NWAYTEST_RESV2          0xfe00  /* Unused...                   */


struct mac_chip_info {
	const char *name;
	uint16 vendor_id, device_id, flags;
	uint16 data;
};

static struct mac_chip_info  mac_chip_table[] = {
	{ "NVidia nForce network controller", PCI_VENDOR_ID_NVIDIA, 0x01c3,
	  PCI_COMMAND_IO|PCI_COMMAND_MASTER, DEV_IRQMASK_1|DEV_NEED_TIMERIRQ },
	{ "NVidia nForce2 network controller",PCI_VENDOR_ID_NVIDIA, 0x0066,
	  PCI_COMMAND_IO|PCI_COMMAND_MASTER, DEV_NEED_LASTPACKET1|DEV_IRQMASK_2|DEV_NEED_TIMERIRQ },
	{ "NVidia nForce3 network controller",PCI_VENDOR_ID_NVIDIA, 0x00d6,
	  PCI_COMMAND_IO|PCI_COMMAND_MASTER, DEV_NEED_LASTPACKET1|DEV_IRQMASK_2|DEV_NEED_TIMERIRQ },
	{0,},					       /* 0 terminated list. */
};



/* Linux net_device structure */
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

//	struct dev_mc_list	*mc_list;	/* Multicast mac addresses	*/
	int			mc_count;	/* Number of installed mcasts	*/
//	int			promiscuity;
//	int			allmulti;

    NetQueue_s		*packet_queue;
    int irq_handle; /* IRQ handler handle */

	int device_handle; /* device handle from probing */
    
    int node_handle;    /* handle of device node in /dev */
};


/*
 * SMP locking:
 * All hardware access under dev->priv->lock, except the performance
 * critical parts:
 * - rx is (pseudo-) lockless: it relies on the single-threading provided
 * 	by the arch code for interrupts.
 * - tx setup is lockless: it relies on dev->xmit_lock. Actual submission
 *	needs dev->priv->lock :-(
 * - set_multicast_list: preparation lockless, relies on dev->xmit_lock.
 */

/* in dev: base, irq */
struct fe_priv {
	SpinLock_s lock;

	/* General data: */
	int in_shutdown;
	u32 linkspeed;
	int duplex;
	int phyaddr;
	int wolenabled;

	/* General data: RO fields */
	dma_addr_t ring_addr;
	PCI_Info_s * pci_dev;
	u32 orig_mac[2];
	u32 irqmask;

	area_id reg_area;

	/* rx specific fields.
	 * Locking: Within irq hander or disable_irq+spin_lock(&np->lock);
	 */
	struct ring_desc *rx_ring;
	unsigned int cur_rx, refill_rx;
	PacketBuf_s *rx_skbuff[RX_RING];
	dma_addr_t rx_dma[RX_RING];
	unsigned int rx_buf_sz;
	ktimer_t oom_kick;
	ktimer_t nic_poll;

	/*
	 * tx specific fields.
	 */
	struct ring_desc *tx_ring;
	unsigned int next_tx, nic_tx;
	PacketBuf_s *tx_skbuff[TX_RING];
	dma_addr_t tx_dma[TX_RING];
	u16 tx_flags;
};

static void nv_do_nic_poll(unsigned long data);

/*
 * Maximum number of loops until we assume that a bit in the irq mask
 * is stuck. Overridable with module param.
 */
static int max_interrupt_work = 5;

static inline struct fe_priv *get_nvpriv(struct net_device *dev)
{
	return (struct fe_priv *) dev->priv;
}

static inline u8 *get_hwbase(struct net_device *dev)
{
	return (u8 *) dev->base_addr;
}

static inline void pci_push(u8 * base)
{
	/* force out pending posted writes */
	readl(base);
}

static int reg_delay(struct net_device *dev, int offset, u32 mask, u32 target,
				int delay, int delaymax, const char *msg)
{
	u8 *base = get_hwbase(dev);

	pci_push(base);
	do {
		udelay(delay);
		delaymax -= delay;
		if (delaymax < 0) {
			if (msg)
				printk(msg);
			return 1;
		}
	} while ((readl(base + offset) & mask) != target);
	return 0;
}

#define MII_READ	(-1)
/* mii_rw: read/write a register on the PHY.
 *
 * Caller must guarantee serialization
 */
static int mii_rw(struct net_device *dev, int addr, int miireg, int value)
{
	u8 *base = get_hwbase(dev);
	int was_running;
	u32 reg;
	int retval;

	writel(NVREG_MIISTAT_MASK, base + NvRegMIIStatus);
	was_running = 0;
	reg = readl(base + NvRegAdapterControl);
	if (reg & NVREG_ADAPTCTL_RUNNING) {
		was_running = 1;
		writel(reg & ~NVREG_ADAPTCTL_RUNNING, base + NvRegAdapterControl);
	}
	reg = readl(base + NvRegMIIControl);
	if (reg & NVREG_MIICTL_INUSE) {
		writel(NVREG_MIICTL_INUSE, base + NvRegMIIControl);
		udelay(NV_MIIBUSY_DELAY);
	}

	reg = NVREG_MIICTL_INUSE | (addr << NVREG_MIICTL_ADDRSHIFT) | miireg;
	if (value != MII_READ) {
		writel(value, base + NvRegMIIData);
		reg |= NVREG_MIICTL_WRITE;
	}
	writel(reg, base + NvRegMIIControl);

	if (reg_delay(dev, NvRegMIIControl, NVREG_MIICTL_INUSE, 0,
			NV_MIIPHY_DELAY, NV_MIIPHY_DELAYMAX, NULL)) {
		dprintk(KERN_DEBUG "%s: mii_rw of reg %d at PHY %d timed out.\n",
				dev->name, miireg, addr);
		retval = -1;
	} else if (value != MII_READ) {
		/* it was a write operation - fewer failures are detectable */
		dprintk(KERN_DEBUG "%s: mii_rw wrote 0x%x to reg %d at PHY %d\n",
				dev->name, value, miireg, addr);
		retval = 0;
	} else if (readl(base + NvRegMIIStatus) & NVREG_MIISTAT_ERROR) {
		dprintk(KERN_DEBUG "%s: mii_rw of reg %d at PHY %d failed.\n",
				dev->name, miireg, addr);
		retval = -1;
	} else {
		/* FIXME: why is that required? */
		udelay(50);
		retval = readl(base + NvRegMIIData);
		dprintk(KERN_DEBUG "%s: mii_rw read from reg %d at PHY %d: 0x%x.\n",
				dev->name, miireg, addr, retval);
	}
	if (was_running) {
		reg = readl(base + NvRegAdapterControl);
		writel(reg | NVREG_ADAPTCTL_RUNNING, base + NvRegAdapterControl);
	}
	return retval;
}

static void nv_start_rx(struct net_device *dev)
{
	struct fe_priv *np = get_nvpriv(dev);
	u8 *base = get_hwbase(dev);

	dprintk(KERN_DEBUG "%s: nv_start_rx\n", dev->name);
	/* Already running? Stop it. */
	if (readl(base + NvRegReceiverControl) & NVREG_RCVCTL_START) {
		writel(0, base + NvRegReceiverControl);
		pci_push(base);
	}
	writel(np->linkspeed, base + NvRegLinkSpeed);
	pci_push(base);
	writel(NVREG_RCVCTL_START, base + NvRegReceiverControl);
	pci_push(base);
}

static void nv_stop_rx(struct net_device *dev)
{
	u8 *base = get_hwbase(dev);

	dprintk(KERN_DEBUG "%s: nv_stop_rx\n", dev->name);
	writel(0, base + NvRegReceiverControl);
	reg_delay(dev, NvRegReceiverStatus, NVREG_RCVSTAT_BUSY, 0,
		       NV_RXSTOP_DELAY1, NV_RXSTOP_DELAY1MAX,
		       KERN_INFO "nv_stop_rx: ReceiverStatus remained busy");

	udelay(NV_RXSTOP_DELAY2);
	writel(0, base + NvRegLinkSpeed);
}

static void nv_start_tx(struct net_device *dev)
{
	u8 *base = get_hwbase(dev);

	dprintk(KERN_DEBUG "%s: nv_start_tx\n", dev->name);
	writel(NVREG_XMITCTL_START, base + NvRegTransmitterControl);
	pci_push(base);
}

static void nv_stop_tx(struct net_device *dev)
{
	u8 *base = get_hwbase(dev);

	dprintk(KERN_DEBUG "%s: nv_stop_tx\n", dev->name);
	writel(0, base + NvRegTransmitterControl);
	reg_delay(dev, NvRegTransmitterStatus, NVREG_XMITSTAT_BUSY, 0,
		       NV_TXSTOP_DELAY1, NV_TXSTOP_DELAY1MAX,
		       KERN_INFO "nv_stop_tx: TransmitterStatus remained busy");

	udelay(NV_TXSTOP_DELAY2);
	writel(0, base + NvRegUnknownTransmitterReg);
}

static void nv_txrx_reset(struct net_device *dev)
{
	u8 *base = get_hwbase(dev);

	dprintk(KERN_DEBUG "%s: nv_txrx_reset\n", dev->name);
	writel(NVREG_TXRXCTL_BIT2 | NVREG_TXRXCTL_RESET, base + NvRegTxRxControl);
	pci_push(base);
	udelay(NV_TXRX_RESET_DELAY);
	writel(NVREG_TXRXCTL_BIT2, base + NvRegTxRxControl);
	pci_push(base);
}

/*
 * nv_alloc_rx: fill rx ring entries.
 * Return 1 if the allocations for the skbs failed and the
 * rx engine is without Available descriptors
 */
static int nv_alloc_rx(struct net_device *dev)
{
	struct fe_priv *np = get_nvpriv(dev);
	unsigned int refill_rx = np->refill_rx;

	while (np->cur_rx != refill_rx) {
		int nr = refill_rx % RX_RING;
		PacketBuf_s *skb;

		if (np->rx_skbuff[nr] == NULL) {

			skb = alloc_pkt_buffer(RX_ALLOC_BUFSIZE);
			if (!skb)
				break;

			//skb->dev = dev;
			skb->pb_nSize = 0;
			np->rx_skbuff[nr] = skb;
		} else {
			skb = np->rx_skbuff[nr];
		}
		np->rx_dma[nr] = pci_map_single(np->pci_dev, skb->pb_pData, skb->pb_nSize,
						PCI_DMA_FROMDEVICE);
		np->rx_ring[nr].PacketBuffer = cpu_to_le32(np->rx_dma[nr]);
		np->rx_ring[nr].Length = cpu_to_le16(RX_NIC_BUFSIZE);
		//wmb();
		np->rx_ring[nr].Flags = cpu_to_le16(NV_RX_AVAIL);
		dprintk(KERN_DEBUG "%s: nv_alloc_rx: Packet  %d marked as Available\n",
					dev->name, refill_rx);
		refill_rx++;
	}
	np->refill_rx = refill_rx;
	if (np->cur_rx - refill_rx == RX_RING)
		return 1;
	return 0;
}

static void nv_do_rx_refill(unsigned long data)
{
	struct net_device *dev = (struct net_device *) data;
	struct fe_priv *np = get_nvpriv(dev);

	disable_irq(dev->irq);
	if (nv_alloc_rx(dev)) {
		spin_lock(&np->lock);
		if (!np->in_shutdown) {
			start_timer(np->oom_kick, (timer_callback *) &nv_do_rx_refill, dev, OOM_REFILL * 1000, true);
		}
			//mod_timer(&np->oom_kick, jiffies + OOM_REFILL);
		spin_unlock(&np->lock);
	}
	enable_irq(dev->irq);
}

static int nv_init_ring(struct net_device *dev)
{
	struct fe_priv *np = get_nvpriv(dev);
	int i;

	np->next_tx = np->nic_tx = 0;
	for (i = 0; i < TX_RING; i++) {
		np->tx_ring[i].Flags = 0;
	}

	np->cur_rx = RX_RING;
	np->refill_rx = 0;
	for (i = 0; i < RX_RING; i++) {
		np->rx_ring[i].Flags = 0;
	}
	return nv_alloc_rx(dev);
}

static void nv_drain_tx(struct net_device *dev)
{
	struct fe_priv *np = get_nvpriv(dev);
	int i;
	for (i = 0; i < TX_RING; i++) {
		np->tx_ring[i].Flags = 0;
		if (np->tx_skbuff[i]) {
			pci_unmap_single(np->pci_dev, np->tx_dma[i],
						np->tx_skbuff[i]->pb_nSize,
						PCI_DMA_TODEVICE);
			free_pkt_buffer(np->tx_skbuff[i]);
			np->tx_skbuff[i] = NULL;
		}
	}
}

static void nv_drain_rx(struct net_device *dev)
{
	struct fe_priv *np = get_nvpriv(dev);
	int i;
	for (i = 0; i < RX_RING; i++) {
		np->rx_ring[i].Flags = 0;
		//wmb();
		if (np->rx_skbuff[i]) {
			pci_unmap_single(np->pci_dev, np->rx_dma[i],
						np->rx_skbuff[i]->len,
						PCI_DMA_FROMDEVICE);
			free_pkt_buffer(np->rx_skbuff[i]);
			np->rx_skbuff[i] = NULL;
		}
	}
}

static void drain_ring(struct net_device *dev)
{
	nv_drain_tx(dev);
	nv_drain_rx(dev);
}

/*
 * nv_start_xmit: dev->hard_start_xmit function
 * Called with dev->xmit_lock held.
 */
static int nv_start_xmit(PacketBuf_s *skb, struct net_device *dev)
{
	struct fe_priv *np = get_nvpriv(dev);
	int nr = np->next_tx % TX_RING;

	np->tx_skbuff[nr] = skb;
	np->tx_dma[nr] = pci_map_single(np->pci_dev, skb->pb_pData,skb->pb_nSize,
					PCI_DMA_TODEVICE);

	np->tx_ring[nr].PacketBuffer = cpu_to_le32(np->tx_dma[nr]);
	np->tx_ring[nr].Length = cpu_to_le16(skb->pb_nSize-1);

	spin_lock_irq(&np->lock);
	//wmb();
	np->tx_ring[nr].Flags = np->tx_flags;
	dprintk(KERN_DEBUG "%s: nv_start_xmit: packet packet %d queued for transmission.\n",
				dev->name, np->next_tx);
#if 0
	{
		int j;
		for (j=0; j<64; j++) {
			if ((j%16) == 0)
				dprintk("\n%03x:", j);
			dprintk(" %02x", ((unsigned char*)skb->pb_pData)[j]);
		}
		dprintk("\n");
	}
#endif

	np->next_tx++;

	dev->trans_start = jiffies;
	if (np->next_tx - np->nic_tx >= TX_LIMIT_STOP)
		netif_stop_queue(dev);
	spin_unlock_irq(&np->lock);
	writel(NVREG_TXRXCTL_KICK, get_hwbase(dev) + NvRegTxRxControl);
	pci_push(get_hwbase(dev));
	return 0;
}

/*
 * nv_tx_done: check for completed packets, release the skbs.
 *
 * Caller must own np->lock.
 */
static void nv_tx_done(struct net_device *dev)
{
	struct fe_priv *np = get_nvpriv(dev);

	while (np->nic_tx < np->next_tx) {
		struct ring_desc *prd;
		int i = np->nic_tx % TX_RING;

		prd = &np->tx_ring[i];

		dprintk(KERN_DEBUG "%s: nv_tx_done: looking at packet %d, Flags 0x%x.\n",
					dev->name, np->nic_tx, prd->Flags);
		if (prd->Flags & cpu_to_le16(NV_TX_VALID))
			break;
		if (prd->Flags & cpu_to_le16(NV_TX_RETRYERROR|NV_TX_CARRIERLOST|NV_TX_LATECOLLISION|
						NV_TX_UNDERFLOW|NV_TX_ERROR)) {
			//if (prd->Flags & cpu_to_le16(NV_TX_UNDERFLOW))
			//	np->stats.tx_fifo_errors++;
			//if (prd->Flags & cpu_to_le16(NV_TX_CARRIERLOST))
			//	np->stats.tx_carrier_errors++;
			//np->stats.tx_errors++;
		} else {
			//np->stats.tx_packets++;
			//np->stats.tx_bytes += np->tx_skbuff[i]->len;
		}
		pci_unmap_single(np->pci_dev, np->tx_dma[i],
					np->tx_skbuff[i]->len,
					PCI_DMA_TODEVICE);
		free_pkt_buffer(np->tx_skbuff[i]);
		np->tx_skbuff[i] = NULL;
		np->nic_tx++;
	}
	if (np->next_tx - np->nic_tx < TX_LIMIT_START)
		netif_wake_queue(dev);
}

#ifndef __SYLLABLE__
/*
 * nv_tx_timeout: dev->tx_timeout function
 * Called with dev->xmit_lock held.
 */
static void nv_tx_timeout(struct net_device *dev)
{
	struct fe_priv *np = get_nvpriv(dev);
	u8 *base = get_hwbase(dev);

	dprintk(KERN_DEBUG "%s: Got tx_timeout. irq: %08x\n", dev->name,
			readl(base + NvRegIrqStatus) & NVREG_IRQSTAT_MASK);

	spin_lock_irq(&np->lock);

	/* 1) stop tx engine */
	nv_stop_tx(dev);

	/* 2) check that the packets were not sent already: */
	nv_tx_done(dev);

	/* 3) if there are dead entries: clear everything */
	if (np->next_tx != np->nic_tx) {
		printk(KERN_DEBUG "%s: tx_timeout: dead entries!\n", dev->name);
		nv_drain_tx(dev);
		np->next_tx = np->nic_tx = 0;
		writel((u32) (np->ring_addr + RX_RING*sizeof(struct ring_desc)), base + NvRegTxRingPhysAddr);
		netif_wake_queue(dev);
	}

	/* 4) restart tx engine */
	nv_start_tx(dev);
	spin_unlock_irq(&np->lock);
}
#endif /* !__SYLLABLE__ */

static void nv_rx_process(struct net_device *dev)
{
	struct fe_priv *np = get_nvpriv(dev);

	for (;;) {
		struct ring_desc *prd;
		PacketBuf_s *skb;
		int len;
		int i;
		if (np->cur_rx - np->refill_rx >= RX_RING)
			break;	/* we scanned the whole ring - do not continue */

		i = np->cur_rx % RX_RING;
		prd = &np->rx_ring[i];
		dprintk(KERN_DEBUG "%s: nv_rx_process: looking at packet %d, Flags 0x%x.\n",
					dev->name, np->cur_rx, prd->Flags);

		if (prd->Flags & cpu_to_le16(NV_RX_AVAIL))
			break;	/* still owned by hardware, */

		/*
		 * the packet is for us - immediately tear down the pci mapping.
		 * TODO: check if a prefetch of the first cacheline improves
		 * the performance.
		 */
		pci_unmap_single(np->pci_dev, np->rx_dma[i],
				np->rx_skbuff[i]->pb_nSize,
				PCI_DMA_FROMDEVICE);

#if 0
		{
			int j;
			dprintk(KERN_DEBUG "Dumping packet (flags 0x%x).",prd->Flags);
			for (j=0; j<64; j++) {
				if ((j%16) == 0)
					dprintk("\n%03x:", j);
				dprintk(" %02x", ((unsigned char*)np->rx_skbuff[i]->pb_pData)[j]);
			}
			dprintk("\n");
		}
#endif
		/* look at what we actually got: */
		if (!(prd->Flags & cpu_to_le16(NV_RX_DESCRIPTORVALID)))
			goto next_pkt;


		len = le16_to_cpu(prd->Length);

		if (prd->Flags & cpu_to_le16(NV_RX_MISSEDFRAME)) {
			//np->stats.rx_missed_errors++;
			//np->stats.rx_errors++;
			goto next_pkt;
		}
		if (prd->Flags & cpu_to_le16(NV_RX_ERROR1|NV_RX_ERROR2|NV_RX_ERROR3|NV_RX_ERROR4)) {
			//np->stats.rx_errors++;
			goto next_pkt;
		}
		if (prd->Flags & cpu_to_le16(NV_RX_CRCERR)) {
			//np->stats.rx_crc_errors++;
			//np->stats.rx_errors++;
			goto next_pkt;
		}
		if (prd->Flags & cpu_to_le16(NV_RX_OVERFLOW)) {
			//np->stats.rx_over_errors++;
			//np->stats.rx_errors++;
			goto next_pkt;
		}
		if (prd->Flags & cpu_to_le16(NV_RX_ERROR)) {
			/* framing errors are soft errors, the rest is fatal. */
			if (prd->Flags & cpu_to_le16(NV_RX_FRAMINGERR)) {
				if (prd->Flags & cpu_to_le16(NV_RX_SUBSTRACT1)) {
					len--;
				}
			} else {
				//np->stats.rx_errors++;
				goto next_pkt;
			}
		}
		/* got a valid packet - forward it to the network core */
		skb = np->rx_skbuff[i];
		np->rx_skbuff[i] = NULL;

		skb_put(skb, len);

		if ( dev->packet_queue != NULL ) {
			skb->pb_uMacHdr.pRaw = skb->pb_pData;
			enqueue_packet( dev->packet_queue, skb );
			dprintk("Accepted packet %d\n", np->cur_rx);
		} else {
			printk( "Error: received packet %d to downed device!\n", np->cur_rx );
			free_pkt_buffer( skb );
		}

		dev->last_rx = jiffies;
		//np->stats.rx_packets++;
		//np->stats.rx_bytes += len;
next_pkt:
		np->cur_rx++;
	}
}

#ifndef __SYLLABLE__
/*
 * nv_change_mtu: dev->change_mtu function
 * Called with dev_base_lock held for read.
 */
static int nv_change_mtu(struct net_device *dev, int new_mtu)
{
	if (new_mtu > DEFAULT_MTU)
		return -EINVAL;
	dev->mtu = new_mtu;
	return 0;
}

/*
 * nv_set_multicast: dev->set_multicast function
 * Called with dev->xmit_lock held.
 */
static void nv_set_multicast(struct net_device *dev)
{
	struct fe_priv *np = get_nvpriv(dev);
	u8 *base = get_hwbase(dev);
	u32 addr[2];
	u32 mask[2];
	u32 pff;

	memset(addr, 0, sizeof(addr));
	memset(mask, 0, sizeof(mask));

	//if (dev->flags & IFF_PROMISC) {
		printk(KERN_NOTICE "%s: Promiscuous mode enabled.\n", dev->name);
		pff = NVREG_PFF_PROMISC;
#ifndef __ATHEOS__
	} else {
		pff = NVREG_PFF_MYADDR;

		if (dev->flags & IFF_ALLMULTI || dev->mc_list) {
			u32 alwaysOff[2];
			u32 alwaysOn[2];

			alwaysOn[0] = alwaysOn[1] = alwaysOff[0] = alwaysOff[1] = 0xffffffff;
			if (dev->flags & IFF_ALLMULTI) {
				alwaysOn[0] = alwaysOn[1] = alwaysOff[0] = alwaysOff[1] = 0;
			} else {
				struct dev_mc_list *walk;

				walk = dev->mc_list;
				while (walk != NULL) {
					u32 a, b;
					a = le32_to_cpu(*(u32 *) walk->dmi_addr);
					b = le16_to_cpu(*(u16 *) (&walk->dmi_addr[4]));
					alwaysOn[0] &= a;
					alwaysOff[0] &= ~a;
					alwaysOn[1] &= b;
					alwaysOff[1] &= ~b;
					walk = walk->next;
				}
			}
			addr[0] = alwaysOn[0];
			addr[1] = alwaysOn[1];
			mask[0] = alwaysOn[0] | alwaysOff[0];
			mask[1] = alwaysOn[1] | alwaysOff[1];
		}
	}
#endif /* !__ATHEOS__ */
	addr[0] |= NVREG_MCASTADDRA_FORCE;
	pff |= NVREG_PFF_ALWAYS;
	spin_lock_irq(&np->lock);
	nv_stop_rx(dev);
	writel(addr[0], base + NvRegMulticastAddrA);
	writel(addr[1], base + NvRegMulticastAddrB);
	writel(mask[0], base + NvRegMulticastMaskA);
	writel(mask[1], base + NvRegMulticastMaskB);
	writel(pff, base + NvRegPacketFilterFlags);
	nv_start_rx(dev);
	spin_unlock_irq(&np->lock);
}
#endif /* !__SYLLABLE__ */

static int nv_update_linkspeed(struct net_device *dev)
{
	struct fe_priv *np = get_nvpriv(dev);
	int adv, lpa, newls, newdup;

	adv = mii_rw(dev, np->phyaddr, MII_ADVERTISE, MII_READ);
	lpa = mii_rw(dev, np->phyaddr, MII_LPA, MII_READ);
	dprintk(KERN_DEBUG "%s: nv_update_linkspeed: PHY advertises 0x%04x, lpa 0x%04x.\n",
				dev->name, adv, lpa);

	/* FIXME: handle parallel detection properly, handle gigabit ethernet */
	lpa = lpa & adv;
	if (lpa  & LPA_100FULL) {
		newls = NVREG_LINKSPEED_FORCE|NVREG_LINKSPEED_100;
		newdup = 1;
	} else if (lpa & LPA_100HALF) {
		newls = NVREG_LINKSPEED_FORCE|NVREG_LINKSPEED_100;
		newdup = 0;
	} else if (lpa & LPA_10FULL) {
		newls = NVREG_LINKSPEED_FORCE|NVREG_LINKSPEED_10;
		newdup = 1;
	} else if (lpa & LPA_10HALF) {
		newls = NVREG_LINKSPEED_FORCE|NVREG_LINKSPEED_10;
		newdup = 0;
	} else {
		dprintk(KERN_DEBUG "%s: bad ability %04x - falling back to 10HD.\n", dev->name, lpa);
		newls = NVREG_LINKSPEED_FORCE|NVREG_LINKSPEED_10;
		newdup = 0;
	}
	if (np->duplex != newdup || np->linkspeed != newls) {
		np->duplex = newdup;
		np->linkspeed = newls;
		return 1;
	}
	return 0;
}

static void nv_link_irq(struct net_device *dev)
{
	struct fe_priv *np = get_nvpriv(dev);
	u8 *base = get_hwbase(dev);
	u32 miistat;
	int miival;

	miistat = readl(base + NvRegMIIStatus);
	writel(NVREG_MIISTAT_MASK, base + NvRegMIIStatus);
	dprintk(KERN_DEBUG "%s: link change notification, status 0x%x.\n", dev->name, miistat);

	miival = mii_rw(dev, np->phyaddr, MII_BMSR, MII_READ);
	if (miival & BMSR_ANEGCOMPLETE) {
		nv_update_linkspeed(dev);

		if (netif_carrier_ok(dev)) {
			nv_stop_rx(dev);
		} else {
			netif_carrier_on(dev);
			printk(KERN_INFO "%s: link up.\n", dev->name);
		}
		writel(NVREG_MISC1_FORCE | ( np->duplex ? 0 : NVREG_MISC1_HD),
					base + NvRegMisc1);
		nv_start_rx(dev);
	} else {
		if (netif_carrier_ok(dev)) {
			netif_carrier_off(dev);
			printk(KERN_INFO "%s: link down.\n", dev->name);
			nv_stop_rx(dev);
		}
		writel(np->linkspeed, base + NvRegLinkSpeed);
		pci_push(base);
	}
}

static int nv_nic_irq(int foo, void *data, SysCallRegs_s *regs)
{
	struct net_device *dev = (struct net_device *) data;
	struct fe_priv *np = get_nvpriv(dev);
	u8 *base = get_hwbase(dev);
	u32 events;
	int i;

	//dprintk(KERN_DEBUG "%s: nv_nic_irq\n", dev->name);

	for (i=0; ; i++) {
		events = readl(base + NvRegIrqStatus) & NVREG_IRQSTAT_MASK;
		writel(NVREG_IRQSTAT_MASK, base + NvRegIrqStatus);
		pci_push(base);
		//dprintk(KERN_DEBUG "%s: irq: %08x\n", dev->name, events);
		if (!(events & np->irqmask))
			break;

		if (events & (NVREG_IRQ_TX1|NVREG_IRQ_TX2|NVREG_IRQ_TX_ERR)) {
			spin_lock(&np->lock);
			nv_tx_done(dev);
			spin_unlock(&np->lock);
		}

		if (events & (NVREG_IRQ_RX|NVREG_IRQ_RX_NOBUF)) {
			nv_rx_process(dev);
			if (nv_alloc_rx(dev)) {
				spin_lock(&np->lock);
				if (!np->in_shutdown) {
					start_timer(np->oom_kick, (timer_callback *) &nv_do_rx_refill, dev, OOM_REFILL * 1000, true);
				}
					//mod_timer(&np->oom_kick, jiffies + OOM_REFILL);
				spin_unlock(&np->lock);
			}
		}

		if (events & NVREG_IRQ_LINK) {
			spin_lock(&np->lock);
			nv_link_irq(dev);
			spin_unlock(&np->lock);
		}
		if (events & (NVREG_IRQ_TX_ERR)) {
			//dprintk(KERN_DEBUG "%s: received irq with events 0x%x. Probably TX fail.\n",
			//			dev->name, events);
		}
		if (events & (NVREG_IRQ_UNKNOWN)) {
			//printk(KERN_DEBUG "%s: received irq with unknown events 0x%x. Please report\n",
			//			dev->name, events);
 		}
		if (i > max_interrupt_work) {
			spin_lock(&np->lock);
			/* disable interrupts on the nic */
			writel(0, base + NvRegIrqMask);
			pci_push(base);

			if (!np->in_shutdown)
				start_timer(np->nic_poll, (timer_callback *) &nv_do_nic_poll, dev, POLL_WAIT * 1000, true);
				//mod_timer(&np->nic_poll, jiffies + POLL_WAIT);
			//printk(KERN_DEBUG "%s: too many iterations (%d) in nv_nic_irq.\n", dev->name, i);
			spin_unlock(&np->lock);
			break;
		}

	}
	//dprintk(KERN_DEBUG "%s: nv_nic_irq completed\n", dev->name);

	//return IRQ_RETVAL(i);
	return 0;
}

static void nv_do_nic_poll(unsigned long data)
{
	struct net_device *dev = (struct net_device *) data;
	struct fe_priv *np = get_nvpriv(dev);
	u8 *base = get_hwbase(dev);

	disable_irq(dev->irq);
	/* FIXME: Do we need synchronize_irq(dev->irq) here? */
	/*
	 * reenable interrupts on the nic, we have to do this before calling
	 * nv_nic_irq because that may decide to do otherwise
	 */
	writel(np->irqmask, base + NvRegIrqMask);
	pci_push(base);
	nv_nic_irq((int) 0, (void *) data, (SysCallRegs_s *) NULL);
	enable_irq(dev->irq);
}

static int nv_open(struct net_device *dev)
{
	struct fe_priv *np = get_nvpriv(dev);
	u8 *base = get_hwbase(dev);
	int ret, oom, i;

	dprintk(KERN_DEBUG "nv_open: begin\n");

	/* 1) erase previous misconfiguration */
	/* 4.1-1: stop adapter: ignored, 4.3 seems to be overkill */
	writel(NVREG_MCASTADDRA_FORCE, base + NvRegMulticastAddrA);
	writel(0, base + NvRegMulticastAddrB);
	writel(0, base + NvRegMulticastMaskA);
	writel(0, base + NvRegMulticastMaskB);
	writel(0, base + NvRegPacketFilterFlags);
	writel(0, base + NvRegAdapterControl);
	writel(0, base + NvRegLinkSpeed);
	writel(0, base + NvRegUnknownTransmitterReg);
	nv_txrx_reset(dev);
	writel(0, base + NvRegUnknownSetupReg6);

	/* 2) initialize descriptor rings */
	np->in_shutdown = 0;
	oom = nv_init_ring(dev);

	/* 3) set mac address */
	{
		u32 mac[2];

		mac[0] = (dev->dev_addr[0] <<  0) + (dev->dev_addr[1] <<  8) +
				(dev->dev_addr[2] << 16) + (dev->dev_addr[3] << 24);
		mac[1] = (dev->dev_addr[4] << 0) + (dev->dev_addr[5] << 8);

		writel(mac[0], base + NvRegMacAddrA);
		writel(mac[1], base + NvRegMacAddrB);
	}

	/* 4) continue setup */
	np->linkspeed = NVREG_LINKSPEED_FORCE|NVREG_LINKSPEED_10;
	np->duplex = 0;
	writel(NVREG_UNKSETUP3_VAL1, base + NvRegUnknownSetupReg3);
	writel(0, base + NvRegTxRxControl);
	pci_push(base);
	writel(NVREG_TXRXCTL_BIT1, base + NvRegTxRxControl);
	reg_delay(dev, NvRegUnknownSetupReg5, NVREG_UNKSETUP5_BIT31, NVREG_UNKSETUP5_BIT31,
			NV_SETUP5_DELAY, NV_SETUP5_DELAYMAX,
			KERN_INFO "open: SetupReg5, Bit 31 remained off\n");
	writel(0, base + NvRegUnknownSetupReg4);

	/* 5) Find a suitable PHY */
	writel(NVREG_MIISPEED_BIT8|NVREG_MIIDELAY, base + NvRegMIISpeed);
	for (i = 1; i < 32; i++) {
		int id1, id2;

		spin_lock_irq(&np->lock);
		id1 = mii_rw(dev, i, MII_PHYSID1, MII_READ);
		spin_unlock_irq(&np->lock);
		if (id1 < 0 || id1 == 0xffff)
			continue;
		spin_lock_irq(&np->lock);
		id2 = mii_rw(dev, i, MII_PHYSID2, MII_READ);
		spin_unlock_irq(&np->lock);
		if (id2 < 0 || id2 == 0xffff)
			continue;
		dprintk("forcedeth open: Found PHY %04x:%04x at address %d.\n",
				id1, id2, i);
		np->phyaddr = i;

		spin_lock_irq(&np->lock);
		nv_update_linkspeed(dev);
		spin_unlock_irq(&np->lock);

		break;
	}
	if (i == 32) {
		printk("forcedeth open: failing due to lack of suitable PHY.\n");
		ret = -EINVAL;
		goto out_drain;
	}

	/* 6) continue setup */
	writel(NVREG_MISC1_FORCE | ( np->duplex ? 0 : NVREG_MISC1_HD),
				base + NvRegMisc1);
	writel(readl(base + NvRegTransmitterStatus), base + NvRegTransmitterStatus);
	writel(NVREG_PFF_ALWAYS, base + NvRegPacketFilterFlags);
	writel(NVREG_OFFLOAD_NORMAL, base + NvRegOffloadConfig);

	writel(readl(base + NvRegReceiverStatus), base + NvRegReceiverStatus);
	i = (int)rand();
	writel(NVREG_RNDSEED_FORCE | (i&NVREG_RNDSEED_MASK), base + NvRegRandomSeed);
	writel(NVREG_UNKSETUP1_VAL, base + NvRegUnknownSetupReg1);
	writel(NVREG_UNKSETUP2_VAL, base + NvRegUnknownSetupReg2);
	writel(NVREG_POLL_DEFAULT, base + NvRegPollingInterval);
	writel(NVREG_UNKSETUP6_VAL, base + NvRegUnknownSetupReg6);
	writel((np->phyaddr << NVREG_ADAPTCTL_PHYSHIFT)|NVREG_ADAPTCTL_PHYVALID,
			base + NvRegAdapterControl);
	writel(NVREG_UNKSETUP4_VAL, base + NvRegUnknownSetupReg4);
	writel(NVREG_WAKEUPFLAGS_VAL, base + NvRegWakeUpFlags);

	/* 7) start packet processing */
	writel((u32) np->ring_addr, base + NvRegRxRingPhysAddr);
	writel((u32) (np->ring_addr + RX_RING*sizeof(struct ring_desc)), base + NvRegTxRingPhysAddr);
	writel( ((RX_RING-1) << NVREG_RINGSZ_RXSHIFT) + ((TX_RING-1) << NVREG_RINGSZ_TXSHIFT),
			base + NvRegRingSizes);

	i = readl(base + NvRegPowerState);
	if ( (i & NVREG_POWERSTATE_POWEREDUP) == 0)
		writel(NVREG_POWERSTATE_POWEREDUP|i, base + NvRegPowerState);

	pci_push(base);
	udelay(10);
	writel(readl(base + NvRegPowerState) | NVREG_POWERSTATE_VALID, base + NvRegPowerState);
	writel(NVREG_ADAPTCTL_RUNNING, base + NvRegAdapterControl);


	writel(0, base + NvRegIrqMask);
	pci_push(base);
	writel(NVREG_IRQSTAT_MASK, base + NvRegIrqStatus);
	pci_push(base);
	writel(NVREG_MIISTAT_MASK2, base + NvRegMIIStatus);
	writel(NVREG_IRQSTAT_MASK, base + NvRegIrqStatus);
	pci_push(base);

	ret = request_irq(dev->irq, nv_nic_irq, NULL, SA_SHIRQ, dev->name, dev);
	if (ret < 0)
		goto out_drain;
	dev->irq_handle = ret;

	writel(np->irqmask, base + NvRegIrqMask);

	spin_lock_irq(&np->lock);
	writel(NVREG_MCASTADDRA_FORCE, base + NvRegMulticastAddrA);
	writel(0, base + NvRegMulticastAddrB);
	writel(0, base + NvRegMulticastMaskA);
	writel(0, base + NvRegMulticastMaskB);
	writel(NVREG_PFF_ALWAYS|NVREG_PFF_MYADDR, base + NvRegPacketFilterFlags);
	nv_start_rx(dev);
	nv_start_tx(dev);
	netif_start_queue(dev);
	if (oom) {
		start_timer(np->oom_kick, (timer_callback *) &nv_do_rx_refill, dev, OOM_REFILL * 1000, true);
	}
		//mod_timer(&np->oom_kick, jiffies + OOM_REFILL);
	if (mii_rw(dev, np->phyaddr, MII_BMSR, MII_READ) & BMSR_ANEGCOMPLETE) {
		netif_carrier_on(dev);
	} else {
		printk("%s: no link during initialization.\n", dev->name);
		netif_carrier_off(dev);
	}

	spin_unlock_irq(&np->lock);

	return 0;
out_drain:
	drain_ring(dev);
	return ret;
}

static int nv_close(struct net_device *dev)
{
	struct fe_priv *np = get_nvpriv(dev);
	u8 *base;

	spin_lock_irq(&np->lock);
	np->in_shutdown = 1;
	spin_unlock_irq(&np->lock);
	//synchronize_irq(dev->irq);

	delete_timer(np->oom_kick);
	delete_timer(np->nic_poll);
	np->oom_kick = create_timer();
	np->nic_poll = create_timer();

	netif_stop_queue(dev);
	spin_lock_irq(&np->lock);
	nv_stop_tx(dev);
	nv_stop_rx(dev);
	base = get_hwbase(dev);

	/* disable interrupts on the nic or we will lock up */
	writel(0, base + NvRegIrqMask);
	pci_push(base);
	dprintk(KERN_INFO "%s: Irqmask is zero again\n", dev->name);

	spin_unlock_irq(&np->lock);

	release_irq(dev->irq, dev->irq_handle);

	drain_ring(dev);

	if (np->wolenabled)
		nv_start_rx(dev);

	/* FIXME: power down nic */

	return 0;
}

//static int nv_probe1(struct pci_dev *pci_dev, const struct pci_device_id *id)
static int nv_probe1 (struct mac_chip_info *id, PCI_Info_s *pci_dev,
					 int device_handle, int found_cnt)
{
	struct net_device *dev;
	struct fe_priv *np;
	unsigned long addr;
	u8 *base;
	void *reg_base = NULL;
	int err;
	char node_path[64];

	dev = kmalloc(sizeof(struct net_device), MEMF_KERNEL|MEMF_CLEAR);
	err = -ENOMEM;
	if (!dev)
		goto out;

	if ((dev->priv = kmalloc(sizeof(struct fe_priv), MEMF_KERNEL)) == NULL) {
		goto out;
	}

	np = dev->priv;

	np->pci_dev = pci_dev;
	spinlock_init(&np->lock, "forcedeth_lock");

	//init_timer(&np->oom_kick);
	//np->oom_kick.data = (unsigned long) dev;
	//np->oom_kick.function = &nv_do_rx_refill;	/* timer handler */
	//init_timer(&np->nic_poll);
	//np->nic_poll.data = (unsigned long) dev;
	//np->nic_poll.function = &nv_do_nic_poll;	/* timer handler */
	np->oom_kick = create_timer();
	np->nic_poll = create_timer();

	err = -EINVAL;
#ifndef __SYLLABLE__
	addr = 0;
	for (i = 0; i < DEVICE_COUNT_RESOURCE; i++) {
		dprintk(KERN_DEBUG "%s: resource %d start %p len %ld flags 0x%08lx.\n",
				pci_name(pci_dev), i, (void*)pci_resource_start(pci_dev, i),
				pci_resource_len(pci_dev, i),
				pci_resource_flags(pci_dev, i));
		if (pci_resource_flags(pci_dev, i) & IORESOURCE_MEM &&
				pci_resource_len(pci_dev, i) >= NV_PCI_REGSZ) {
			addr = pci_resource_start(pci_dev, i);
			break;
		}
	}
	if (i == DEVICE_COUNT_RESOURCE) {
		printk(KERN_INFO "forcedeth: Couldn't find register window for device %s.\n",
					pci_name(pci_dev));
		goto out_relreg;
	}
#endif

	if(get_pci_memory_size(pci_dev, 0) >= NV_PCI_REGSZ)
		addr = pci_dev->u.h0.nBase0 & PCI_ADDRESS_MEMORY_32_MASK;
	else if(get_pci_memory_size(pci_dev, 1) >= NV_PCI_REGSZ)
		addr = pci_dev->u.h0.nBase1 & PCI_ADDRESS_MEMORY_32_MASK;
	else if(get_pci_memory_size(pci_dev, 2) >= NV_PCI_REGSZ)
		addr = pci_dev->u.h0.nBase2 & PCI_ADDRESS_MEMORY_32_MASK;
	else if(get_pci_memory_size(pci_dev, 3) >= NV_PCI_REGSZ)
		addr = pci_dev->u.h0.nBase3 & PCI_ADDRESS_MEMORY_32_MASK;
	else if(get_pci_memory_size(pci_dev, 4) >= NV_PCI_REGSZ)
		addr = pci_dev->u.h0.nBase4 & PCI_ADDRESS_MEMORY_32_MASK;
	else if(get_pci_memory_size(pci_dev, 5) >= NV_PCI_REGSZ)
		addr = pci_dev->u.h0.nBase5 & PCI_ADDRESS_MEMORY_32_MASK;
	else {
		printk("forcedeth: No proper MM I/O address found!\n");
		goto out_relreg;
	}

	// allocate register area (Syllable way)
	np->reg_area = create_area ("forcedeth_register", (void **)&reg_base,
		PAGE_SIZE * 2, PAGE_SIZE * 2, AREA_KERNEL|AREA_ANY_ADDRESS, AREA_NO_LOCK);
	if( np->reg_area < 0 ) {
		printk ("forcedeth: failed to create register area (%d)\n", np->reg_area);
		goto out_relreg;
	}

	if( remap_area (np->reg_area, (void *)(addr & PAGE_MASK)) < 0 ) {
		printk("forcedeth: failed to create register area (%d)\n", np->reg_area);
		goto out_relreg;
	}

	dev->base_addr = (unsigned long)reg_base + ( addr - ( addr & PAGE_MASK ) );

	dev->name = "nForce NIC";
	dev->irq = pci_dev->u.h0.nInterruptLine;
	np->rx_ring = pci_alloc_consistent(pci_dev, sizeof(struct ring_desc) * (RX_RING + TX_RING),
						&np->ring_addr);
	if (!np->rx_ring)
		goto out_unmap;
	np->tx_ring = &np->rx_ring[RX_RING];

	/* read the mac address */
	base = get_hwbase(dev);
	np->orig_mac[0] = readl(base + NvRegMacAddrA);
	np->orig_mac[1] = readl(base + NvRegMacAddrB);

	dev->dev_addr[0] = (np->orig_mac[1] >>  8) & 0xff;
	dev->dev_addr[1] = (np->orig_mac[1] >>  0) & 0xff;
	dev->dev_addr[2] = (np->orig_mac[0] >> 24) & 0xff;
	dev->dev_addr[3] = (np->orig_mac[0] >> 16) & 0xff;
	dev->dev_addr[4] = (np->orig_mac[0] >>  8) & 0xff;
	dev->dev_addr[5] = (np->orig_mac[0] >>  0) & 0xff;

#ifndef __SYLLABLE__
	if (!is_valid_ether_addr(dev->dev_addr)) {
		/*
		 * Bad mac address. At least one bios sets the mac address
		 * to 01:23:45:67:89:ab
		 */
		printk(KERN_ERR "%s: Invalid Mac address detected: %02x:%02x:%02x:%02x:%02x:%02x\n",
			pci_name(pci_dev),
			dev->dev_addr[0], dev->dev_addr[1], dev->dev_addr[2],
			dev->dev_addr[3], dev->dev_addr[4], dev->dev_addr[5]);
		printk(KERN_ERR "Please complain to your hardware vendor. Switching to a random MAC.\n");
		dev->dev_addr[0] = 0x00;
		dev->dev_addr[1] = 0x00;
		dev->dev_addr[2] = 0x6c;
		get_random_bytes(&dev->dev_addr[3], 3);
	}
#endif

	printk("forcedeth: MAC Address %02x:%02x:%02x:%02x:%02x:%02x\n",
			dev->dev_addr[0], dev->dev_addr[1], dev->dev_addr[2],
			dev->dev_addr[3], dev->dev_addr[4], dev->dev_addr[5]);

	/* disable WOL */
	writel(0, base + NvRegWakeUpFlags);
	np->wolenabled = 0;

	np->tx_flags = cpu_to_le16(NV_TX_LASTPACKET|NV_TX_LASTPACKET1|NV_TX_VALID);
	if (id->data & DEV_NEED_LASTPACKET1)
		np->tx_flags |= cpu_to_le16(NV_TX_LASTPACKET1);
	if (id->data & DEV_IRQMASK_1)
		np->irqmask = NVREG_IRQMASK_WANTED_1;
	if (id->data & DEV_IRQMASK_2)
		np->irqmask = NVREG_IRQMASK_WANTED_2;
	if (id->data & DEV_NEED_TIMERIRQ)
		np->irqmask |= NVREG_IRQ_TIMER;

	/* Create Syllable device node */
	sprintf( node_path, "net/eth/forcedeth-%d", found_cnt );
	dev->node_handle = create_device_node( device_handle, pci_dev->nHandle, node_path, &g_sDevOps, dev );

	printk("forcedeth: NIC initialization successful!\n");

	return 0;

//out_freering:
	//pci_free_consistent(np->pci_dev, sizeof(struct ring_desc) * (RX_RING + TX_RING),
	//			np->rx_ring, np->ring_addr);
	//pci_set_drvdata(pci_dev, NULL);
out_unmap:
	//iounmap(get_hwbase(dev));
out_relreg:
	//pci_release_regions(pci_dev);
//out_disable:
	//pci_disable_device(pci_dev);
//out_free:
	//free_netdev(dev);
	kfree(dev);
out:
	printk("forcedeth: NIC initialization failed!\n");
	return err;
}


int nv_probe( int device_handle )
{
	int cards_found = 0;
	//struct device* dev;
	
    int i;
    PCI_Info_s sInfo;
   
    for ( i = 0 ; g_psBus->get_pci_info( &sInfo, i ) == 0 ; ++i ) {
		int chip_idx;
		int ioaddr;
		int irq;
		int pci_command;
		int new_command;
		
		for ( chip_idx = 0 ; mac_chip_table[chip_idx].vendor_id ; chip_idx++ ) {
			if ( sInfo.nVendorID == mac_chip_table[chip_idx].vendor_id &&
				 sInfo.nDeviceID == mac_chip_table[chip_idx].device_id) {
				break;
			}
		}
		if (mac_chip_table[chip_idx].vendor_id == 0) { 		/* Compiled out! */
			continue;
		}
		
		if( claim_device( device_handle, sInfo.nHandle, "NVidia nForce Network Controller", DEVICE_NET ) != 0 )
			continue;
		
		printk( "forcedeth: nv_probe() found NIC\n" );
		ioaddr = sInfo.u.h0.nBase0 & PCI_ADDRESS_IO_MASK;
		irq = sInfo.u.h0.nInterruptLine;

		pci_command = g_psBus->read_pci_config( sInfo.nBus, sInfo.nDevice, sInfo.nFunction, PCI_COMMAND, 2 );
		new_command = pci_command | (mac_chip_table[chip_idx].flags & 7);
		if (pci_command != new_command) {
			printk(KERN_INFO "  The PCI BIOS has not enabled the"
				   " device at %d/%d/%d!  Updating PCI command %4.4x->%4.4x.\n",
				   sInfo.nBus, sInfo.nDevice, sInfo.nFunction, pci_command, new_command );
			g_psBus->write_pci_config( sInfo.nBus, sInfo.nDevice, sInfo.nFunction, PCI_COMMAND, 2, new_command);
		}

		if(nv_probe1( &mac_chip_table[chip_idx], &sInfo, device_handle, cards_found ) == 0)
			cards_found++;
		break;
	}
	if( !cards_found )
		disable_device( device_handle );
	return cards_found ? 0 : -ENODEV;
}

/* ----------- */

static status_t nv_open_dev( void* pNode, uint32 nFlags, void **pCookie )
{
    return( 0 );
}

static status_t nv_close_dev( void* pNode, void* pCookie )
{
    return( 0 );
}

static status_t nv_ioctl( void* pNode, void* pCookie, uint32 nCommand, void* pArgs, bool bFromKernel )
{
	struct device* psDev = pNode;
    int nError = 0;

    switch( nCommand )
    {
		case SIOC_ETH_START:
		{
			psDev->packet_queue = pArgs;
			nv_open( psDev );
			break;
		}
		case SIOC_ETH_STOP:
		{
			nv_close( psDev );
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
			printk( "Error: nv_ioctl() unknown command %d\n", (int)nCommand );
			nError = -ENOSYS;
			break;
    }

    return( nError );
}

static int nv_read( void* pNode, void* pCookie, off_t nPosition, void* pBuffer, size_t nSize )
{
    return( -ENOSYS );
}

static int nv_write( void* pNode, void* pCookie, off_t nPosition, const void* pBuffer, size_t nSize )
{
	struct device* dev = pNode;
	PacketBuf_s* psBuffer = alloc_pkt_buffer( nSize );
	if ( psBuffer != NULL ) {
		memcpy( psBuffer->pb_pData, pBuffer, nSize );
		nv_start_xmit( psBuffer, dev );
	}
	return( nSize );
}


static DeviceOperations_s g_sDevOps = {
    nv_open_dev,
    nv_close_dev,
    nv_ioctl,
    nv_read,
    nv_write,
    NULL,	// dop_readv
    NULL,	// dop_writev
    NULL,	// dop_add_select_req
    NULL	// dop_rem_select_req
};


status_t device_init( int nDeviceID )
{
	/* Get PCI bus */
    g_psBus = get_busmanager( PCI_BUS_NAME, PCI_BUS_VERSION );
    
    if( g_psBus == NULL )
    	return( -1 );
	return( nv_probe( nDeviceID ) );
}

status_t device_uninit( int nDeviceID )
{
    return( 0 );
}
