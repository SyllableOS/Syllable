/*
 * New driver for Marvell Yukon chipset and SysKonnect Gigabit
 * Ethernet adapters. Based on earlier sk98lin, e100 and
 * FreeBSD if_sk drivers.
 *
 * Port from Linux to Syllable by Kristian Van Der Vliet.
 *
 * This driver intentionally does not support all the features
 * of the original driver such as link fail-over and link management because
 * those should be done at higher levels.
 *
 * Copyright (C) 2004, 2005 Stephen Hemminger <shemminger@osdl.org>
 * Copyright (C) 2007 Kristian Van Der Vliet <vanders@liqwyd.com>
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <atheos/kernel.h>
#include <atheos/kdebug.h>
#include <atheos/types.h>
#include <atheos/device.h>
#include <atheos/pci.h>
#include <atheos/spinlock.h>
#include <atheos/semaphore.h>
#include <atheos/irq.h>
#include <atheos/udelay.h>
#include <atheos/bitops.h>
#include <atheos/seqlock.h>
#include <posix/errno.h>
#include <posix/signal.h>
#include <net/net_device.h>
#include <net/mii.h>
#include <net/sockios.h>

#define NO_DEBUG_STUBS 1
#include <atheos/linux_compat.h>

#include <skge.h>

#define DEFAULT_TX_RING_SIZE	128
#define DEFAULT_RX_RING_SIZE	512
#define MAX_TX_RING_SIZE	1024
#define TX_LOW_WATER		(MAX_SKB_FRAGS + 1)
#define MAX_RX_RING_SIZE	4096
#define RX_COPY_THRESHOLD	128
#define RX_BUF_SIZE		1536
#define PHY_RETRIES	        1000
#define ETH_JUMBO_MTU		9000
#define TX_WATCHDOG		(5 * HZ)
#define NAPI_WEIGHT		64
#define BLINK_MS		250

static PCI_bus_s* g_psBus;
static int g_nPorts = 0;

/* XXXKV: Enable debugging */
#if 0
 #undef DEBUG_LIMIT
 #define DEBUG_LIMIT KERN_DEBUG_LOW
#endif

/* XXXKV: Possibly add these to pci.h & linux_compat.h */
#define PCI_STATUS_DETECTED_PARITY		PCI_STATUS_PARITY_ERROR_DETECTED
#define PCI_STATUS_SIG_SYSTEM_ERROR		0x4000
#define PCI_STATUS_REC_MASTER_ABORT		PCI_STATUS_MASTER_ABORT_RECEIVED
#define PCI_STATUS_REC_TARGET_ABORT		PCI_STATUS_TARGET_ABORT_RECEIVED
#define PCI_STATUS_PARITY				PCI_STATUS_PARITY_SIGNALLED

static const struct pci_device_id skge_pci_tbl[] = {
	{ PCI_VENDOR_ID_3COM, PCI_DEVICE_ID_3COM_3C940,
	  PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0UL },
	{ PCI_VENDOR_ID_3COM, PCI_DEVICE_ID_3COM_3C940B,
	  PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0UL },
	{ PCI_VENDOR_ID_SYSKONNECT, PCI_DEVICE_ID_SYSKONNECT_GE,
	  PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0UL },
	{ PCI_VENDOR_ID_SYSKONNECT, PCI_DEVICE_ID_SYSKONNECT_YU,
	  PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0UL },
	{ PCI_VENDOR_ID_DLINK, PCI_DEVICE_ID_DLINK_DGE510T,
	  PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0UL },
	{ PCI_VENDOR_ID_DLINK, 0x4b01,
	  PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0UL },	/* DGE-530T */
	{ PCI_VENDOR_ID_MARVELL, 0x4320,
	  PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0UL },
	{ PCI_VENDOR_ID_MARVELL, 0x5005,
	  PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0UL },	/* Belkin */
	{ PCI_VENDOR_ID_CNET, PCI_DEVICE_ID_CNET_GIGACARD,
	  PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0UL },
	{ PCI_VENDOR_ID_LINKSYS, PCI_DEVICE_ID_LINKSYS_EG1064,
	  PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0UL },
	{ PCI_VENDOR_ID_LINKSYS, 0x1032,
      PCI_ANY_ID, 0x0015, 0, 0, 0UL },
	{ 0 }
};

static int skge_up(struct net_device *dev);
static int skge_down(struct net_device *dev);
static void skge_phy_reset(struct skge_port *skge);
static void skge_tx_clean(struct skge_port *skge);
static int xm_phy_write(struct skge_hw *hw, int port, u16 reg, u16 val);
static int gm_phy_write(struct skge_hw *hw, int port, u16 reg, u16 val);
static void genesis_get_stats(struct skge_port *skge, u64 *data);
static void yukon_get_stats(struct skge_port *skge, u64 *data);
static void yukon_init(struct skge_hw *hw, int port);
static void genesis_mac_init(struct skge_hw *hw, int port);
static void genesis_link_up(struct skge_port *skge);

/* Avoid conditionals by using array */
static const int txqaddr[] = { Q_XA1, Q_XA2 };
static const int rxqaddr[] = { Q_R1, Q_R2 };
static const u32 rxirqmask[] = { IS_R1_F, IS_R2_F };
static const u32 txirqmask[] = { IS_XA1_F, IS_XA2_F };

/* Determine supported/advertised modes based on hardware.
 * Note: ethtool ADVERTISED_xxx == SUPPORTED_xxx
 */
static u32 skge_supported_modes(const struct skge_hw *hw)
{
	u32 supported;

	if (hw->copper) {
		supported = SUPPORTED_10baseT_Half
			| SUPPORTED_10baseT_Full
			| SUPPORTED_100baseT_Half
			| SUPPORTED_100baseT_Full
			| SUPPORTED_1000baseT_Half
			| SUPPORTED_1000baseT_Full
			| SUPPORTED_Autoneg| SUPPORTED_TP;

		if (hw->chip_id == CHIP_ID_GENESIS)
			supported &= ~(SUPPORTED_10baseT_Half
					     | SUPPORTED_10baseT_Full
					     | SUPPORTED_100baseT_Half
					     | SUPPORTED_100baseT_Full);

		else if (hw->chip_id == CHIP_ID_YUKON)
			supported &= ~SUPPORTED_1000baseT_Half;
	} else
		supported = SUPPORTED_1000baseT_Full | SUPPORTED_FIBRE
			| SUPPORTED_Autoneg;

	return supported;
}

/* Chip internal frequency for clock calculations */
static inline u32 hwkhz(const struct skge_hw *hw)
{
	return (hw->chip_id == CHIP_ID_GENESIS) ? 53125 : 78125;
}

/* Chip HZ to microseconds */
static inline u32 skge_clk2usec(const struct skge_hw *hw, u32 ticks)
{
	return (ticks * 1000) / hwkhz(hw);
}

/* Microseconds to chip HZ */
static inline u32 skge_usecs2clk(const struct skge_hw *hw, u32 usec)
{
	return hwkhz(hw) * usec / 1000;
}

enum led_mode { LED_MODE_OFF, LED_MODE_ON, LED_MODE_TST };
static void skge_led(struct skge_port *skge, enum led_mode mode)
{
	struct skge_hw *hw = skge->hw;
	int port = skge->port;

	LOCK(hw->phy_mutex);
	if (hw->chip_id == CHIP_ID_GENESIS) {
		switch (mode) {
		case LED_MODE_OFF:
			xm_phy_write(hw, port, PHY_BCOM_P_EXT_CTRL, PHY_B_PEC_LED_OFF);
			skge_write8(hw, SK_REG(port, LNK_LED_REG), LINKLED_OFF);
			skge_write32(hw, SK_REG(port, RX_LED_VAL), 0);
			skge_write8(hw, SK_REG(port, RX_LED_CTRL), LED_T_OFF);
			break;

		case LED_MODE_ON:
			skge_write8(hw, SK_REG(port, LNK_LED_REG), LINKLED_ON);
			skge_write8(hw, SK_REG(port, LNK_LED_REG), LINKLED_LINKSYNC_ON);

			skge_write8(hw, SK_REG(port, RX_LED_CTRL), LED_START);
			skge_write8(hw, SK_REG(port, TX_LED_CTRL), LED_START);

			break;

		case LED_MODE_TST:
			skge_write8(hw, SK_REG(port, RX_LED_TST), LED_T_ON);
			skge_write32(hw, SK_REG(port, RX_LED_VAL), 100);
			skge_write8(hw, SK_REG(port, RX_LED_CTRL), LED_START);

			xm_phy_write(hw, port, PHY_BCOM_P_EXT_CTRL, PHY_B_PEC_LED_ON);
			break;
		}
	} else {
		switch (mode) {
		case LED_MODE_OFF:
			gm_phy_write(hw, port, PHY_MARV_LED_CTRL, 0);
			gm_phy_write(hw, port, PHY_MARV_LED_OVER,
				     PHY_M_LED_MO_DUP(MO_LED_OFF)  |
				     PHY_M_LED_MO_10(MO_LED_OFF)   |
				     PHY_M_LED_MO_100(MO_LED_OFF)  |
				     PHY_M_LED_MO_1000(MO_LED_OFF) |
				     PHY_M_LED_MO_RX(MO_LED_OFF));
			break;
		case LED_MODE_ON:
			gm_phy_write(hw, port, PHY_MARV_LED_CTRL,
				     PHY_M_LED_PULS_DUR(PULS_170MS) |
				     PHY_M_LED_BLINK_RT(BLINK_84MS) |
				     PHY_M_LEDC_TX_CTRL |
				     PHY_M_LEDC_DP_CTRL);

			gm_phy_write(hw, port, PHY_MARV_LED_OVER,
				     PHY_M_LED_MO_RX(MO_LED_OFF) |
				     (skge->speed == SPEED_100 ?
				      PHY_M_LED_MO_100(MO_LED_ON) : 0));
			break;
		case LED_MODE_TST:
			gm_phy_write(hw, port, PHY_MARV_LED_CTRL, 0);
			gm_phy_write(hw, port, PHY_MARV_LED_OVER,
				     PHY_M_LED_MO_DUP(MO_LED_ON)  |
				     PHY_M_LED_MO_10(MO_LED_ON)   |
				     PHY_M_LED_MO_100(MO_LED_ON)  |
				     PHY_M_LED_MO_1000(MO_LED_ON) |
				     PHY_M_LED_MO_RX(MO_LED_ON));
		}
	}
	UNLOCK(hw->phy_mutex);
}

/*
 * Allocate ring elements and chain them together
 * One-to-one association of board descriptors with ring elements
 */
static int skge_ring_alloc(struct skge_ring *ring, void *vaddr, u32 base)
{
	struct skge_tx_desc *d;
	struct skge_element *e;
	int i;

	ring->start = kmalloc(sizeof(*e) * ring->count, MEMF_KERNEL|MEMF_CLEAR);
	if (!ring->start)
		return -ENOMEM;

	for (i = 0, e = ring->start, d = vaddr; i < ring->count; i++, e++, d++) {
		e->desc = d;
		if (i == ring->count - 1) {
			e->next = ring->start;
			d->next_offset = base;
		} else {
			e->next = e + 1;
			d->next_offset = base + (i+1) * sizeof(*d);
		}
	}
	ring->to_use = ring->to_clean = ring->start;

	return 0;
}


/* Free all  buffers in receive ring, assumes receiver stopped */
static void skge_rx_clean(struct skge_port *skge)
{
	struct skge_hw *hw = skge->hw;
	struct skge_ring *ring = &skge->rx_ring;
	struct skge_element *e;

	e = ring->start;
	do {
		struct skge_rx_desc *rd = e->desc;
		rd->control = 0;
		if (e->skb) {
			pci_unmap_single(hw->pdev,
					 pci_unmap_addr(e, mapaddr),
					 pci_unmap_len(e, maplen),
					 PCI_DMA_FROMDEVICE);
			free_pkt_buffer(e->skb);
			e->skb = NULL;
		}
	} while ((e = e->next) != ring->start);
}

/* Allocate and setup a new buffer for receiving */
static void skge_rx_setup(struct skge_port *skge, struct skge_element *e,
			  PacketBuf_s *skb, unsigned int bufsize)
{
	struct skge_rx_desc *rd = e->desc;
	u64 map;

	map = pci_map_single(skge->hw->pdev, skb->pb_pData, bufsize,
			     PCI_DMA_FROMDEVICE);

	rd->dma_lo = map;
	rd->dma_hi = map >> 32;
	e->skb = skb;
	rd->csum1_start = ETH_HLEN;
	rd->csum2_start = ETH_HLEN;
	rd->csum1 = 0;
	rd->csum2 = 0;

	smp_wmb();

	rd->control = BMU_OWN | BMU_STF | BMU_IRQ_EOF | BMU_TCP_CHECK | bufsize;
    e->mapaddr = map;
    e->maplen = bufsize;
}

/* Resume receiving using existing skb,
 * Note: DMA address is not changed by chip.
 * 	 MTU not changed while receiver active.
 */
static inline void skge_rx_reuse(struct skge_element *e, unsigned int size)
{
	struct skge_rx_desc *rd = e->desc;

	rd->csum2 = 0;
	rd->csum2_start = ETH_HLEN;

	smp_wmb();

	rd->control = BMU_OWN | BMU_STF | BMU_IRQ_EOF | BMU_TCP_CHECK | size;
}

/* Allocate buffers for receive ring
 * For receive:  to_clean is next received frame.
 */
static int skge_rx_fill(struct skge_port *skge)
{
	struct skge_ring *ring = &skge->rx_ring;
	struct skge_element *e;

	e = ring->start;
	do {
		PacketBuf_s *skb;

		skb = alloc_pkt_buffer_aligned(skge->rx_buf_size);
		if (!skb)
			return -ENOMEM;

		//skb_reserve(skb, NET_IP_ALIGN);
		skge_rx_setup(skge, e, skb, skge->rx_buf_size);
	} while ( (e = e->next) != ring->start);

	ring->to_clean = ring->start;
	return 0;
}

static void skge_link_up(struct skge_port *skge)
{
	skge_write8(skge->hw, SK_REG(skge->port, LNK_LED_REG),
		    LED_BLK_OFF|LED_SYNC_OFF|LED_ON);

	netif_carrier_on(skge->netdev);
	netif_wake_queue(skge->netdev);

	kerndbg( KERN_INFO,
	       "%s: Link is up at %d Mbps, %s duplex, flow control %s\n",
	       skge->netdev->name, skge->speed,
	       skge->duplex == DUPLEX_FULL ? "full" : "half",
	       (skge->flow_control == FLOW_MODE_NONE) ? "none" :
	       (skge->flow_control == FLOW_MODE_LOC_SEND) ? "tx only" :
	       (skge->flow_control == FLOW_MODE_REM_SEND) ? "rx only" :
	       (skge->flow_control == FLOW_MODE_SYMMETRIC) ? "tx and rx" :
	       "unknown");
}

static void skge_link_down(struct skge_port *skge)
{
	skge_write8(skge->hw, SK_REG(skge->port, LNK_LED_REG), LED_OFF);
	netif_carrier_off(skge->netdev);
	netif_stop_queue(skge->netdev);

	kerndbg( KERN_INFO, "%s: Link is down.\n", skge->netdev->name);
}

static int __xm_phy_read(struct skge_hw *hw, int port, u16 reg, u16 *val)
{
	int i;

	xm_write16(hw, port, XM_PHY_ADDR, reg | hw->phy_addr);
	*val = xm_read16(hw, port, XM_PHY_DATA);

	for (i = 0; i < PHY_RETRIES; i++) {
		if (xm_read16(hw, port, XM_MMU_CMD) & XM_MMU_PHY_RDY)
			goto ready;
		udelay(1);
	}

	return -ETIMEDOUT;
 ready:
	*val = xm_read16(hw, port, XM_PHY_DATA);

	return 0;
}

static u16 xm_phy_read(struct skge_hw *hw, int port, u16 reg)
{
	u16 v = 0;
	if (__xm_phy_read(hw, port, reg, &v))
		kerndbg( KERN_WARNING, "%s: phy read timed out\n",
		       hw->dev[port]->name);
	return v;
}

static int xm_phy_write(struct skge_hw *hw, int port, u16 reg, u16 val)
{
	int i;

	xm_write16(hw, port, XM_PHY_ADDR, reg | hw->phy_addr);
	for (i = 0; i < PHY_RETRIES; i++) {
		if (!(xm_read16(hw, port, XM_MMU_CMD) & XM_MMU_PHY_BUSY))
			goto ready;
		udelay(1);
	}
	return -EIO;

 ready:
	xm_write16(hw, port, XM_PHY_DATA, val);
	for (i = 0; i < PHY_RETRIES; i++) {
		if (!(xm_read16(hw, port, XM_MMU_CMD) & XM_MMU_PHY_BUSY))
			return 0;
		udelay(1);
	}
	return -ETIMEDOUT;
}

static void genesis_init(struct skge_hw *hw)
{
	/* set blink source counter */
	skge_write32(hw, B2_BSC_INI, (SK_BLK_DUR * SK_FACT_53) / 100);
	skge_write8(hw, B2_BSC_CTRL, BSC_START);

	/* configure mac arbiter */
	skge_write16(hw, B3_MA_TO_CTRL, MA_RST_CLR);

	/* configure mac arbiter timeout values */
	skge_write8(hw, B3_MA_TOINI_RX1, SK_MAC_TO_53);
	skge_write8(hw, B3_MA_TOINI_RX2, SK_MAC_TO_53);
	skge_write8(hw, B3_MA_TOINI_TX1, SK_MAC_TO_53);
	skge_write8(hw, B3_MA_TOINI_TX2, SK_MAC_TO_53);

	skge_write8(hw, B3_MA_RCINI_RX1, 0);
	skge_write8(hw, B3_MA_RCINI_RX2, 0);
	skge_write8(hw, B3_MA_RCINI_TX1, 0);
	skge_write8(hw, B3_MA_RCINI_TX2, 0);

	/* configure packet arbiter timeout */
	skge_write16(hw, B3_PA_CTRL, PA_RST_CLR);
	skge_write16(hw, B3_PA_TOINI_RX1, SK_PKT_TO_MAX);
	skge_write16(hw, B3_PA_TOINI_TX1, SK_PKT_TO_MAX);
	skge_write16(hw, B3_PA_TOINI_RX2, SK_PKT_TO_MAX);
	skge_write16(hw, B3_PA_TOINI_TX2, SK_PKT_TO_MAX);
}

static void genesis_reset(struct skge_hw *hw, int port)
{
	const u8 zero[8]  = { 0 };

	skge_write8(hw, SK_REG(port, GMAC_IRQ_MSK), 0);

	/* reset the statistics module */
	xm_write32(hw, port, XM_GP_PORT, XM_GP_RES_STAT);
	xm_write16(hw, port, XM_IMSK, 0xffff);	/* disable XMAC IRQs */
	xm_write32(hw, port, XM_MODE, 0);		/* clear Mode Reg */
	xm_write16(hw, port, XM_TX_CMD, 0);	/* reset TX CMD Reg */
	xm_write16(hw, port, XM_RX_CMD, 0);	/* reset RX CMD Reg */

	/* disable Broadcom PHY IRQ */
	xm_write16(hw, port, PHY_BCOM_INT_MASK, 0xffff);

	xm_outhash(hw, port, XM_HSM, zero);
}

/* Convert mode to MII values  */
static const u16 phy_pause_map[] = {
	[FLOW_MODE_NONE] =	0,
	[FLOW_MODE_LOC_SEND] =	PHY_AN_PAUSE_ASYM,
	[FLOW_MODE_SYMMETRIC] = PHY_AN_PAUSE_CAP,
	[FLOW_MODE_REM_SEND]  = PHY_AN_PAUSE_CAP | PHY_AN_PAUSE_ASYM,
};


/* Check status of Broadcom phy link */
static void bcom_check_link(struct skge_hw *hw, int port)
{
	struct net_device *dev = hw->dev[port];
	struct skge_port *skge = netdev_priv(dev);
	u16 status;

	/* read twice because of latch */
	(void) xm_phy_read(hw, port, PHY_BCOM_STAT);
	status = xm_phy_read(hw, port, PHY_BCOM_STAT);

	if ((status & PHY_ST_LSYNC) == 0) {
		u16 cmd = xm_read16(hw, port, XM_MMU_CMD);
		cmd &= ~(XM_MMU_ENA_RX | XM_MMU_ENA_TX);
		xm_write16(hw, port, XM_MMU_CMD, cmd);
		/* dummy read to ensure writing */
		(void) xm_read16(hw, port, XM_MMU_CMD);

		if (netif_carrier_ok(dev))
			skge_link_down(skge);
	} else {
		if (skge->autoneg == AUTONEG_ENABLE &&
		    (status & PHY_ST_AN_OVER)) {
			u16 lpa = xm_phy_read(hw, port, PHY_BCOM_AUNE_LP);
			u16 aux = xm_phy_read(hw, port, PHY_BCOM_AUX_STAT);

			if (lpa & PHY_B_AN_RF) {
				kerndbg( KERN_DEBUG, "%s: remote fault\n",
				       dev->name);
				return;
			}

			/* Check Duplex mismatch */
			switch (aux & PHY_B_AS_AN_RES_MSK) {
			case PHY_B_RES_1000FD:
				skge->duplex = DUPLEX_FULL;
				break;
			case PHY_B_RES_1000HD:
				skge->duplex = DUPLEX_HALF;
				break;
			default:
				kerndbg( KERN_DEBUG, "%s: duplex mismatch\n",
				       dev->name);
				return;
			}


			/* We are using IEEE 802.3z/D5.0 Table 37-4 */
			switch (aux & PHY_B_AS_PAUSE_MSK) {
			case PHY_B_AS_PAUSE_MSK:
				skge->flow_control = FLOW_MODE_SYMMETRIC;
				break;
			case PHY_B_AS_PRR:
				skge->flow_control = FLOW_MODE_REM_SEND;
				break;
			case PHY_B_AS_PRT:
				skge->flow_control = FLOW_MODE_LOC_SEND;
				break;
			default:
				skge->flow_control = FLOW_MODE_NONE;
			}

			skge->speed = SPEED_1000;
		}

		if (!netif_carrier_ok(dev))
			genesis_link_up(skge);
	}
}

/* Broadcom 5400 only supports giagabit! SysKonnect did not put an additional
 * Phy on for 100 or 10Mbit operation
 */
static void bcom_phy_init(struct skge_port *skge, int jumbo)
{
	struct skge_hw *hw = skge->hw;
	int port = skge->port;
	int i;
	u16 id1, r, ext, ctl;

	/* magic workaround patterns for Broadcom */
	static const struct {
		u16 reg;
		u16 val;
	} A1hack[] = {
		{ 0x18, 0x0c20 }, { 0x17, 0x0012 }, { 0x15, 0x1104 },
		{ 0x17, 0x0013 }, { 0x15, 0x0404 }, { 0x17, 0x8006 },
		{ 0x15, 0x0132 }, { 0x17, 0x8006 }, { 0x15, 0x0232 },
		{ 0x17, 0x800D }, { 0x15, 0x000F }, { 0x18, 0x0420 },
	}, C0hack[] = {
		{ 0x18, 0x0c20 }, { 0x17, 0x0012 }, { 0x15, 0x1204 },
		{ 0x17, 0x0013 }, { 0x15, 0x0A04 }, { 0x18, 0x0420 },
	};

	/* read Id from external PHY (all have the same address) */
	id1 = xm_phy_read(hw, port, PHY_XMAC_ID1);

	/* Optimize MDIO transfer by suppressing preamble. */
	r = xm_read16(hw, port, XM_MMU_CMD);
	r |=  XM_MMU_NO_PRE;
	xm_write16(hw, port, XM_MMU_CMD,r);

	switch (id1) {
	case PHY_BCOM_ID1_C0:
		/*
		 * Workaround BCOM Errata for the C0 type.
		 * Write magic patterns to reserved registers.
		 */
		for (i = 0; i < ARRAY_SIZE(C0hack); i++)
			xm_phy_write(hw, port,
				     C0hack[i].reg, C0hack[i].val);

		break;
	case PHY_BCOM_ID1_A1:
		/*
		 * Workaround BCOM Errata for the A1 type.
		 * Write magic patterns to reserved registers.
		 */
		for (i = 0; i < ARRAY_SIZE(A1hack); i++)
			xm_phy_write(hw, port,
				     A1hack[i].reg, A1hack[i].val);
		break;
	}

	/*
	 * Workaround BCOM Errata (#10523) for all BCom PHYs.
	 * Disable Power Management after reset.
	 */
	r = xm_phy_read(hw, port, PHY_BCOM_AUX_CTRL);
	r |= PHY_B_AC_DIS_PM;
	xm_phy_write(hw, port, PHY_BCOM_AUX_CTRL, r);

	/* Dummy read */
	xm_read16(hw, port, XM_ISRC);

	ext = PHY_B_PEC_EN_LTR; /* enable tx led */
	ctl = PHY_CT_SP1000;	/* always 1000mbit */

	if (skge->autoneg == AUTONEG_ENABLE) {
		/*
		 * Workaround BCOM Errata #1 for the C5 type.
		 * 1000Base-T Link Acquisition Failure in Slave Mode
		 * Set Repeater/DTE bit 10 of the 1000Base-T Control Register
		 */
		u16 adv = PHY_B_1000C_RD;
		if (skge->advertising & ADVERTISED_1000baseT_Half)
			adv |= PHY_B_1000C_AHD;
		if (skge->advertising & ADVERTISED_1000baseT_Full)
			adv |= PHY_B_1000C_AFD;
		xm_phy_write(hw, port, PHY_BCOM_1000T_CTRL, adv);

		ctl |= PHY_CT_ANE | PHY_CT_RE_CFG;
	} else {
		if (skge->duplex == DUPLEX_FULL)
			ctl |= PHY_CT_DUP_MD;
		/* Force to slave */
		xm_phy_write(hw, port, PHY_BCOM_1000T_CTRL, PHY_B_1000C_MSE);
	}

	/* Set autonegotiation pause parameters */
	xm_phy_write(hw, port, PHY_BCOM_AUNE_ADV,
		     phy_pause_map[skge->flow_control] | PHY_AN_CSMA);

	/* Handle Jumbo frames */
	if (jumbo) {
		xm_phy_write(hw, port, PHY_BCOM_AUX_CTRL,
			     PHY_B_AC_TX_TST | PHY_B_AC_LONG_PACK);

		ext |= PHY_B_PEC_HIGH_LA;

	}

	xm_phy_write(hw, port, PHY_BCOM_P_EXT_CTRL, ext);
	xm_phy_write(hw, port, PHY_BCOM_CTRL, ctl);

	/* Use link status change interrupt */
	xm_phy_write(hw, port, PHY_BCOM_INT_MASK, PHY_B_DEF_MSK);

	bcom_check_link(hw, port);
}

static void genesis_mac_init(struct skge_hw *hw, int port)
{
	struct net_device *dev = hw->dev[port];
	struct skge_port *skge = netdev_priv(dev);
	int jumbo = hw->dev[port]->mtu > ETH_DATA_LEN;
	int i;
	u32 r;
	const u8 zero[6]  = { 0 };

	for (i = 0; i < 10; i++) {
		skge_write16(hw, SK_REG(port, TX_MFF_CTRL1),
			     MFF_SET_MAC_RST);
		if (skge_read16(hw, SK_REG(port, TX_MFF_CTRL1)) & MFF_SET_MAC_RST)
			goto reset_ok;
		udelay(1);
	}

	kerndbg( KERN_WARNING, "%s: genesis reset failed\n", dev->name);

 reset_ok:
	/* Unreset the XMAC. */
	skge_write16(hw, SK_REG(port, TX_MFF_CTRL1), MFF_CLR_MAC_RST);

	/*
	 * Perform additional initialization for external PHYs,
	 * namely for the 1000baseTX cards that use the XMAC's
	 * GMII mode.
	 */
	/* Take external Phy out of reset */
	r = skge_read32(hw, B2_GP_IO);
	if (port == 0)
		r |= GP_DIR_0|GP_IO_0;
	else
		r |= GP_DIR_2|GP_IO_2;

	skge_write32(hw, B2_GP_IO, r);


	/* Enable GMII interface */
	xm_write16(hw, port, XM_HW_CFG, XM_HW_GMII_MD);

	bcom_phy_init(skge, jumbo);

	/* Set Station Address */
	xm_outaddr(hw, port, XM_SA, dev->dev_addr);

	/* We don't use match addresses so clear */
	for (i = 1; i < 16; i++)
		xm_outaddr(hw, port, XM_EXM(i), zero);

	/* Clear MIB counters */
	xm_write16(hw, port, XM_STAT_CMD,
			XM_SC_CLR_RXC | XM_SC_CLR_TXC);
	/* Clear two times according to Errata #3 */
	xm_write16(hw, port, XM_STAT_CMD,
			XM_SC_CLR_RXC | XM_SC_CLR_TXC);

	/* configure Rx High Water Mark (XM_RX_HI_WM) */
	xm_write16(hw, port, XM_RX_HI_WM, 1450);

	/* We don't need the FCS appended to the packet. */
	r = XM_RX_LENERR_OK | XM_RX_STRIP_FCS;
	if (jumbo)
		r |= XM_RX_BIG_PK_OK;

	if (skge->duplex == DUPLEX_HALF) {
		/*
		 * If in manual half duplex mode the other side might be in
		 * full duplex mode, so ignore if a carrier extension is not seen
		 * on frames received
		 */
		r |= XM_RX_DIS_CEXT;
	}
	xm_write16(hw, port, XM_RX_CMD, r);


	/* We want short frames padded to 60 bytes. */
	xm_write16(hw, port, XM_TX_CMD, XM_TX_AUTO_PAD);

	/*
	 * Bump up the transmit threshold. This helps hold off transmit
	 * underruns when we're blasting traffic from both ports at once.
	 */
	xm_write16(hw, port, XM_TX_THR, 512);

	/*
	 * Enable the reception of all error frames. This is is
	 * a necessary evil due to the design of the XMAC. The
	 * XMAC's receive FIFO is only 8K in size, however jumbo
	 * frames can be up to 9000 bytes in length. When bad
	 * frame filtering is enabled, the XMAC's RX FIFO operates
	 * in 'store and forward' mode. For this to work, the
	 * entire frame has to fit into the FIFO, but that means
	 * that jumbo frames larger than 8192 bytes will be
	 * truncated. Disabling all bad frame filtering causes
	 * the RX FIFO to operate in streaming mode, in which
	 * case the XMAC will start transferring frames out of the
	 * RX FIFO as soon as the FIFO threshold is reached.
	 */
	xm_write32(hw, port, XM_MODE, XM_DEF_MODE);


	/*
	 * Initialize the Receive Counter Event Mask (XM_RX_EV_MSK)
	 *	- Enable all bits excepting 'Octets Rx OK Low CntOv'
	 *	  and 'Octets Rx OK Hi Cnt Ov'.
	 */
	xm_write32(hw, port, XM_RX_EV_MSK, XMR_DEF_MSK);

	/*
	 * Initialize the Transmit Counter Event Mask (XM_TX_EV_MSK)
	 *	- Enable all bits excepting 'Octets Tx OK Low CntOv'
	 *	  and 'Octets Tx OK Hi Cnt Ov'.
	 */
	xm_write32(hw, port, XM_TX_EV_MSK, XMT_DEF_MSK);

	/* Configure MAC arbiter */
	skge_write16(hw, B3_MA_TO_CTRL, MA_RST_CLR);

	/* configure timeout values */
	skge_write8(hw, B3_MA_TOINI_RX1, 72);
	skge_write8(hw, B3_MA_TOINI_RX2, 72);
	skge_write8(hw, B3_MA_TOINI_TX1, 72);
	skge_write8(hw, B3_MA_TOINI_TX2, 72);

	skge_write8(hw, B3_MA_RCINI_RX1, 0);
	skge_write8(hw, B3_MA_RCINI_RX2, 0);
	skge_write8(hw, B3_MA_RCINI_TX1, 0);
	skge_write8(hw, B3_MA_RCINI_TX2, 0);

	/* Configure Rx MAC FIFO */
	skge_write8(hw, SK_REG(port, RX_MFF_CTRL2), MFF_RST_CLR);
	skge_write16(hw, SK_REG(port, RX_MFF_CTRL1), MFF_ENA_TIM_PAT);
	skge_write8(hw, SK_REG(port, RX_MFF_CTRL2), MFF_ENA_OP_MD);

	/* Configure Tx MAC FIFO */
	skge_write8(hw, SK_REG(port, TX_MFF_CTRL2), MFF_RST_CLR);
	skge_write16(hw, SK_REG(port, TX_MFF_CTRL1), MFF_TX_CTRL_DEF);
	skge_write8(hw, SK_REG(port, TX_MFF_CTRL2), MFF_ENA_OP_MD);

	if (jumbo) {
		/* Enable frame flushing if jumbo frames used */
		skge_write16(hw, SK_REG(port,RX_MFF_CTRL1), MFF_ENA_FLUSH);
	} else {
		/* enable timeout timers if normal frames */
		skge_write16(hw, B3_PA_CTRL,
			     (port == 0) ? PA_ENA_TO_TX1 : PA_ENA_TO_TX2);
	}
}

static void genesis_stop(struct skge_port *skge)
{
	struct skge_hw *hw = skge->hw;
	int port = skge->port;
	u32 reg;

	genesis_reset(hw, port);

	/* Clear Tx packet arbiter timeout IRQ */
	skge_write16(hw, B3_PA_CTRL,
		     port == 0 ? PA_CLR_TO_TX1 : PA_CLR_TO_TX2);

	/*
	 * If the transfer sticks at the MAC the STOP command will not
	 * terminate if we don't flush the XMAC's transmit FIFO !
	 */
	xm_write32(hw, port, XM_MODE,
			xm_read32(hw, port, XM_MODE)|XM_MD_FTF);


	/* Reset the MAC */
	skge_write16(hw, SK_REG(port, TX_MFF_CTRL1), MFF_SET_MAC_RST);

	/* For external PHYs there must be special handling */
	reg = skge_read32(hw, B2_GP_IO);
	if (port == 0) {
		reg |= GP_DIR_0;
		reg &= ~GP_IO_0;
	} else {
		reg |= GP_DIR_2;
		reg &= ~GP_IO_2;
	}
	skge_write32(hw, B2_GP_IO, reg);
	skge_read32(hw, B2_GP_IO);

	xm_write16(hw, port, XM_MMU_CMD,
			xm_read16(hw, port, XM_MMU_CMD)
			& ~(XM_MMU_ENA_RX | XM_MMU_ENA_TX));

	xm_read16(hw, port, XM_MMU_CMD);
}


static void genesis_mac_intr(struct skge_hw *hw, int port)
{
	struct skge_port *skge = netdev_priv(hw->dev[port]);
	u16 status = xm_read16(hw, port, XM_ISRC);

	kerndbg( KERN_DEBUG_LOW, "%s: mac interrupt status 0x%x\n",
	       skge->netdev->name, status);

	if (status & XM_IS_TXF_UR) {
		xm_write32(hw, port, XM_MODE, XM_MD_FTF);
		++skge->net_stats.tx_fifo_errors;
	}
	if (status & XM_IS_RXF_OV) {
		xm_write32(hw, port, XM_MODE, XM_MD_FRF);
		++skge->net_stats.rx_fifo_errors;
	}
}

static void genesis_link_up(struct skge_port *skge)
{
	struct skge_hw *hw = skge->hw;
	int port = skge->port;
	u16 cmd;
	u32 mode, msk;

	cmd = xm_read16(hw, port, XM_MMU_CMD);

	/*
	 * enabling pause frame reception is required for 1000BT
	 * because the XMAC is not reset if the link is going down
	 */
	if (skge->flow_control == FLOW_MODE_NONE ||
	    skge->flow_control == FLOW_MODE_LOC_SEND)
		/* Disable Pause Frame Reception */
		cmd |= XM_MMU_IGN_PF;
	else
		/* Enable Pause Frame Reception */
		cmd &= ~XM_MMU_IGN_PF;

	xm_write16(hw, port, XM_MMU_CMD, cmd);

	mode = xm_read32(hw, port, XM_MODE);
	if (skge->flow_control == FLOW_MODE_SYMMETRIC ||
	    skge->flow_control == FLOW_MODE_LOC_SEND) {
		/*
		 * Configure Pause Frame Generation
		 * Use internal and external Pause Frame Generation.
		 * Sending pause frames is edge triggered.
		 * Send a Pause frame with the maximum pause time if
		 * internal oder external FIFO full condition occurs.
		 * Send a zero pause time frame to re-start transmission.
		 */
		/* XM_PAUSE_DA = '010000C28001' (default) */
		/* XM_MAC_PTIME = 0xffff (maximum) */
		/* remember this value is defined in big endian (!) */
		xm_write16(hw, port, XM_MAC_PTIME, 0xffff);

		mode |= XM_PAUSE_MODE;
		skge_write16(hw, SK_REG(port, RX_MFF_CTRL1), MFF_ENA_PAUSE);
	} else {
		/*
		 * disable pause frame generation is required for 1000BT
		 * because the XMAC is not reset if the link is going down
		 */
		/* Disable Pause Mode in Mode Register */
		mode &= ~XM_PAUSE_MODE;

		skge_write16(hw, SK_REG(port, RX_MFF_CTRL1), MFF_DIS_PAUSE);
	}

	xm_write32(hw, port, XM_MODE, mode);

	msk = XM_DEF_MSK;
	/* disable GP0 interrupt bit for external Phy */
	msk |= XM_IS_INP_ASS;

	xm_write16(hw, port, XM_IMSK, msk);
	xm_read16(hw, port, XM_ISRC);

	/* get MMU Command Reg. */
	cmd = xm_read16(hw, port, XM_MMU_CMD);
	if (skge->duplex == DUPLEX_FULL)
		cmd |= XM_MMU_GMII_FD;

	/*
	 * Workaround BCOM Errata (#10523) for all BCom Phys
	 * Enable Power Management after link up
	 */
	xm_phy_write(hw, port, PHY_BCOM_AUX_CTRL,
		     xm_phy_read(hw, port, PHY_BCOM_AUX_CTRL)
		     & ~PHY_B_AC_DIS_PM);
	xm_phy_write(hw, port, PHY_BCOM_INT_MASK, PHY_B_DEF_MSK);

	/* enable Rx/Tx */
	xm_write16(hw, port, XM_MMU_CMD,
			cmd | XM_MMU_ENA_RX | XM_MMU_ENA_TX);
	skge_link_up(skge);
}

static inline void bcom_phy_intr(struct skge_port *skge)
{
	struct skge_hw *hw = skge->hw;
	int port = skge->port;
	u16 isrc;

	isrc = xm_phy_read(hw, port, PHY_BCOM_INT_STAT);
	kerndbg( KERN_DEBUG, "%s: phy interrupt status 0x%x\n",
	       skge->netdev->name, isrc);

	if (isrc & PHY_B_IS_PSE)
		kerndbg( KERN_DEBUG, "%s: uncorrectable pair swap error\n",
		       hw->dev[port]->name);

	/* Workaround BCom Errata:
	 *	enable and disable loopback mode if "NO HCD" occurs.
	 */
	if (isrc & PHY_B_IS_NO_HDCL) {
		u16 ctrl = xm_phy_read(hw, port, PHY_BCOM_CTRL);
		xm_phy_write(hw, port, PHY_BCOM_CTRL,
				  ctrl | PHY_CT_LOOP);
		xm_phy_write(hw, port, PHY_BCOM_CTRL,
				  ctrl & ~PHY_CT_LOOP);
	}

	if (isrc & (PHY_B_IS_AN_PR | PHY_B_IS_LST_CHANGE))
		bcom_check_link(hw, port);

}

static int gm_phy_write(struct skge_hw *hw, int port, u16 reg, u16 val)
{
	int i;

	gma_write16(hw, port, GM_SMI_DATA, val);
	gma_write16(hw, port, GM_SMI_CTRL,
			 GM_SMI_CT_PHY_AD(hw->phy_addr) | GM_SMI_CT_REG_AD(reg));
	for (i = 0; i < PHY_RETRIES; i++) {
		udelay(1);

		if (!(gma_read16(hw, port, GM_SMI_CTRL) & GM_SMI_CT_BUSY))
			return 0;
	}

	kerndbg( KERN_WARNING, "%s: phy write timeout\n",
	       hw->dev[port]->name);
	return -EIO;
}

static int __gm_phy_read(struct skge_hw *hw, int port, u16 reg, u16 *val)
{
	int i;

	gma_write16(hw, port, GM_SMI_CTRL,
			 GM_SMI_CT_PHY_AD(hw->phy_addr)
			 | GM_SMI_CT_REG_AD(reg) | GM_SMI_CT_OP_RD);

	for (i = 0; i < PHY_RETRIES; i++) {
		udelay(1);
		if (gma_read16(hw, port, GM_SMI_CTRL) & GM_SMI_CT_RD_VAL)
			goto ready;
	}

	return -ETIMEDOUT;
 ready:
	*val = gma_read16(hw, port, GM_SMI_DATA);
	return 0;
}

static u16 gm_phy_read(struct skge_hw *hw, int port, u16 reg)
{
	u16 v = 0;
	if (__gm_phy_read(hw, port, reg, &v))
		kerndbg( KERN_WARNING, "%s: phy read timeout\n",
	       hw->dev[port]->name);
	return v;
}

/* Marvell Phy Initialization */
static void yukon_init(struct skge_hw *hw, int port)
{
	struct skge_port *skge = netdev_priv(hw->dev[port]);
	u16 ctrl, ct1000, adv;

	if (skge->autoneg == AUTONEG_ENABLE) {
		u16 ectrl = gm_phy_read(hw, port, PHY_MARV_EXT_CTRL);

		ectrl &= ~(PHY_M_EC_M_DSC_MSK | PHY_M_EC_S_DSC_MSK |
			  PHY_M_EC_MAC_S_MSK);
		ectrl |= PHY_M_EC_MAC_S(MAC_TX_CLK_25_MHZ);

		ectrl |= PHY_M_EC_M_DSC(0) | PHY_M_EC_S_DSC(1);

		gm_phy_write(hw, port, PHY_MARV_EXT_CTRL, ectrl);
	}

	ctrl = gm_phy_read(hw, port, PHY_MARV_CTRL);
	if (skge->autoneg == AUTONEG_DISABLE)
		ctrl &= ~PHY_CT_ANE;

	ctrl |= PHY_CT_RESET;
	gm_phy_write(hw, port, PHY_MARV_CTRL, ctrl);

	ctrl = 0;
	ct1000 = 0;
	adv = PHY_AN_CSMA;

	if (skge->autoneg == AUTONEG_ENABLE) {
		if (hw->copper) {
			if (skge->advertising & ADVERTISED_1000baseT_Full)
				ct1000 |= PHY_M_1000C_AFD;
			if (skge->advertising & ADVERTISED_1000baseT_Half)
				ct1000 |= PHY_M_1000C_AHD;
			if (skge->advertising & ADVERTISED_100baseT_Full)
				adv |= PHY_M_AN_100_FD;
			if (skge->advertising & ADVERTISED_100baseT_Half)
				adv |= PHY_M_AN_100_HD;
			if (skge->advertising & ADVERTISED_10baseT_Full)
				adv |= PHY_M_AN_10_FD;
			if (skge->advertising & ADVERTISED_10baseT_Half)
				adv |= PHY_M_AN_10_HD;
		} else	/* special defines for FIBER (88E1011S only) */
			adv |= PHY_M_AN_1000X_AHD | PHY_M_AN_1000X_AFD;

		/* Set Flow-control capabilities */
		adv |= phy_pause_map[skge->flow_control];

		/* Restart Auto-negotiation */
		ctrl |= PHY_CT_ANE | PHY_CT_RE_CFG;
	} else {
		/* forced speed/duplex settings */
		ct1000 = PHY_M_1000C_MSE;

		if (skge->duplex == DUPLEX_FULL)
			ctrl |= PHY_CT_DUP_MD;

		switch (skge->speed) {
		case SPEED_1000:
			ctrl |= PHY_CT_SP1000;
			break;
		case SPEED_100:
			ctrl |= PHY_CT_SP100;
			break;
		}

		ctrl |= PHY_CT_RESET;
	}

	gm_phy_write(hw, port, PHY_MARV_1000T_CTRL, ct1000);

	gm_phy_write(hw, port, PHY_MARV_AUNE_ADV, adv);
	gm_phy_write(hw, port, PHY_MARV_CTRL, ctrl);

	/* Enable phy interrupt on autonegotiation complete (or link up) */
	if (skge->autoneg == AUTONEG_ENABLE)
		gm_phy_write(hw, port, PHY_MARV_INT_MASK, PHY_M_IS_AN_MSK);
	else
		gm_phy_write(hw, port, PHY_MARV_INT_MASK, PHY_M_IS_DEF_MSK);
}

static void yukon_reset(struct skge_hw *hw, int port)
{
	gm_phy_write(hw, port, PHY_MARV_INT_MASK, 0);/* disable PHY IRQs */
	gma_write16(hw, port, GM_MC_ADDR_H1, 0);	/* clear MC hash */
	gma_write16(hw, port, GM_MC_ADDR_H2, 0);
	gma_write16(hw, port, GM_MC_ADDR_H3, 0);
	gma_write16(hw, port, GM_MC_ADDR_H4, 0);

	gma_write16(hw, port, GM_RX_CTRL,
			 gma_read16(hw, port, GM_RX_CTRL)
			 | GM_RXCR_UCF_ENA | GM_RXCR_MCF_ENA);
}

/* Apparently, early versions of Yukon-Lite had wrong chip_id? */
static int is_yukon_lite_a0(struct skge_hw *hw)
{
	u32 reg;
	int ret;

	if (hw->chip_id != CHIP_ID_YUKON)
		return 0;

	reg = skge_read32(hw, B2_FAR);
	skge_write8(hw, B2_FAR + 3, 0xff);
	ret = (skge_read8(hw, B2_FAR + 3) != 0);
	skge_write32(hw, B2_FAR, reg);
	return ret;
}

static void yukon_mac_init(struct skge_hw *hw, int port)
{
	struct skge_port *skge = netdev_priv(hw->dev[port]);
	int i;
	u32 reg;
	const u8 *addr = hw->dev[port]->dev_addr;

	/* WA code for COMA mode -- set PHY reset */
	if (hw->chip_id == CHIP_ID_YUKON_LITE &&
	    hw->chip_rev >= CHIP_REV_YU_LITE_A3) {
		reg = skge_read32(hw, B2_GP_IO);
		reg |= GP_DIR_9 | GP_IO_9;
		skge_write32(hw, B2_GP_IO, reg);
	}

	/* hard reset */
	skge_write32(hw, SK_REG(port, GPHY_CTRL), GPC_RST_SET);
	skge_write32(hw, SK_REG(port, GMAC_CTRL), GMC_RST_SET);

	/* WA code for COMA mode -- clear PHY reset */
	if (hw->chip_id == CHIP_ID_YUKON_LITE &&
	    hw->chip_rev >= CHIP_REV_YU_LITE_A3) {
		reg = skge_read32(hw, B2_GP_IO);
		reg |= GP_DIR_9;
		reg &= ~GP_IO_9;
		skge_write32(hw, B2_GP_IO, reg);
	}

	/* Set hardware config mode */
	reg = GPC_INT_POL_HI | GPC_DIS_FC | GPC_DIS_SLEEP |
		GPC_ENA_XC | GPC_ANEG_ADV_ALL_M | GPC_ENA_PAUSE;
	reg |= hw->copper ? GPC_HWCFG_GMII_COP : GPC_HWCFG_GMII_FIB;

	/* Clear GMC reset */
	skge_write32(hw, SK_REG(port, GPHY_CTRL), reg | GPC_RST_SET);
	skge_write32(hw, SK_REG(port, GPHY_CTRL), reg | GPC_RST_CLR);
	skge_write32(hw, SK_REG(port, GMAC_CTRL), GMC_PAUSE_ON | GMC_RST_CLR);

	if (skge->autoneg == AUTONEG_DISABLE) {
		reg = GM_GPCR_AU_ALL_DIS;
		gma_write16(hw, port, GM_GP_CTRL,
				 gma_read16(hw, port, GM_GP_CTRL) | reg);

		switch (skge->speed) {
		case SPEED_1000:
			reg &= ~GM_GPCR_SPEED_100;
			reg |= GM_GPCR_SPEED_1000;
			break;
		case SPEED_100:
			reg &= ~GM_GPCR_SPEED_1000;
			reg |= GM_GPCR_SPEED_100;
			break;
		case SPEED_10:
			reg &= ~(GM_GPCR_SPEED_1000 | GM_GPCR_SPEED_100);
			break;
		}

		if (skge->duplex == DUPLEX_FULL)
			reg |= GM_GPCR_DUP_FULL;
	} else
		reg = GM_GPCR_SPEED_1000 | GM_GPCR_SPEED_100 | GM_GPCR_DUP_FULL;

	switch (skge->flow_control) {
	case FLOW_MODE_NONE:
		skge_write32(hw, SK_REG(port, GMAC_CTRL), GMC_PAUSE_OFF);
		reg |= GM_GPCR_FC_TX_DIS | GM_GPCR_FC_RX_DIS | GM_GPCR_AU_FCT_DIS;
		break;
	case FLOW_MODE_LOC_SEND:
		/* disable Rx flow-control */
		reg |= GM_GPCR_FC_RX_DIS | GM_GPCR_AU_FCT_DIS;
	}

	gma_write16(hw, port, GM_GP_CTRL, reg);
	skge_read16(hw, SK_REG(port, GMAC_IRQ_SRC));

	yukon_init(hw, port);

	/* MIB clear */
	reg = gma_read16(hw, port, GM_PHY_ADDR);
	gma_write16(hw, port, GM_PHY_ADDR, reg | GM_PAR_MIB_CLR);

	for (i = 0; i < GM_MIB_CNT_SIZE; i++)
		gma_read16(hw, port, GM_MIB_CNT_BASE + 8*i);
	gma_write16(hw, port, GM_PHY_ADDR, reg);

	/* transmit control */
	gma_write16(hw, port, GM_TX_CTRL, TX_COL_THR(TX_COL_DEF));

	/* receive control reg: unicast + multicast + no FCS  */
	gma_write16(hw, port, GM_RX_CTRL,
			 GM_RXCR_UCF_ENA | GM_RXCR_CRC_DIS | GM_RXCR_MCF_ENA);

	/* transmit flow control */
	gma_write16(hw, port, GM_TX_FLOW_CTRL, 0xffff);

	/* transmit parameter */
	gma_write16(hw, port, GM_TX_PARAM,
			 TX_JAM_LEN_VAL(TX_JAM_LEN_DEF) |
			 TX_JAM_IPG_VAL(TX_JAM_IPG_DEF) |
			 TX_IPG_JAM_DATA(TX_IPG_JAM_DEF));

	/* serial mode register */
	reg = GM_SMOD_VLAN_ENA | IPG_DATA_VAL(IPG_DATA_DEF);
	if (hw->dev[port]->mtu > 1500)
		reg |= GM_SMOD_JUMBO_ENA;

	gma_write16(hw, port, GM_SERIAL_MODE, reg);

	/* physical address: used for pause frames */
	gma_set_addr(hw, port, GM_SRC_ADDR_1L, addr);
	/* virtual address for data */
	gma_set_addr(hw, port, GM_SRC_ADDR_2L, addr);

	/* enable interrupt mask for counter overflows */
	gma_write16(hw, port, GM_TX_IRQ_MSK, 0);
	gma_write16(hw, port, GM_RX_IRQ_MSK, 0);
	gma_write16(hw, port, GM_TR_IRQ_MSK, 0);

	/* Initialize Mac Fifo */

	/* Configure Rx MAC FIFO */
	skge_write16(hw, SK_REG(port, RX_GMF_FL_MSK), RX_FF_FL_DEF_MSK);
	reg = GMF_OPER_ON | GMF_RX_F_FL_ON;

	/* disable Rx GMAC FIFO Flush for YUKON-Lite Rev. A0 only */
	if (is_yukon_lite_a0(hw))
		reg &= ~GMF_RX_F_FL_ON;

	skge_write8(hw, SK_REG(port, RX_GMF_CTRL_T), GMF_RST_CLR);
	skge_write16(hw, SK_REG(port, RX_GMF_CTRL_T), reg);
	/*
	 * because Pause Packet Truncation in GMAC is not working
	 * we have to increase the Flush Threshold to 64 bytes
	 * in order to flush pause packets in Rx FIFO on Yukon-1
	 */
	skge_write16(hw, SK_REG(port, RX_GMF_FL_THR), RX_GMF_FL_THR_DEF+1);

	/* Configure Tx MAC FIFO */
	skge_write8(hw, SK_REG(port, TX_GMF_CTRL_T), GMF_RST_CLR);
	skge_write16(hw, SK_REG(port, TX_GMF_CTRL_T), GMF_OPER_ON);
}

/* Go into power down mode */
static void yukon_suspend(struct skge_hw *hw, int port)
{
	u16 ctrl;

	ctrl = gm_phy_read(hw, port, PHY_MARV_PHY_CTRL);
	ctrl |= PHY_M_PC_POL_R_DIS;
	gm_phy_write(hw, port, PHY_MARV_PHY_CTRL, ctrl);

	ctrl = gm_phy_read(hw, port, PHY_MARV_CTRL);
	ctrl |= PHY_CT_RESET;
	gm_phy_write(hw, port, PHY_MARV_CTRL, ctrl);

	/* switch IEEE compatible power down mode on */
	ctrl = gm_phy_read(hw, port, PHY_MARV_CTRL);
	ctrl |= PHY_CT_PDOWN;
	gm_phy_write(hw, port, PHY_MARV_CTRL, ctrl);
}

static void yukon_stop(struct skge_port *skge)
{
	struct skge_hw *hw = skge->hw;
	int port = skge->port;

	skge_write8(hw, SK_REG(port, GMAC_IRQ_MSK), 0);
	yukon_reset(hw, port);

	gma_write16(hw, port, GM_GP_CTRL,
			 gma_read16(hw, port, GM_GP_CTRL)
			 & ~(GM_GPCR_TX_ENA|GM_GPCR_RX_ENA));
	gma_read16(hw, port, GM_GP_CTRL);

	yukon_suspend(hw, port);

	/* set GPHY Control reset */
	skge_write8(hw, SK_REG(port, GPHY_CTRL), GPC_RST_SET);
	skge_write8(hw, SK_REG(port, GMAC_CTRL), GMC_RST_SET);
}


static void yukon_mac_intr(struct skge_hw *hw, int port)
{
	struct net_device *dev = hw->dev[port];
	struct skge_port *skge = netdev_priv(dev);
	u8 status = skge_read8(hw, SK_REG(port, GMAC_IRQ_SRC));

	kerndbg( KERN_DEBUG_LOW, "%s: mac interrupt status 0x%x\n",
	       dev->name, status);

	if (status & GM_IS_RX_FF_OR) {
		++skge->net_stats.rx_fifo_errors;
		skge_write8(hw, SK_REG(port, RX_GMF_CTRL_T), GMF_CLI_RX_FO);
	}

	if (status & GM_IS_TX_FF_UR) {
		++skge->net_stats.tx_fifo_errors;
		skge_write8(hw, SK_REG(port, TX_GMF_CTRL_T), GMF_CLI_TX_FU);
	}

}


static u16 yukon_speed(const struct skge_hw *hw, u16 aux)
{
	switch (aux & PHY_M_PS_SPEED_MSK) {
	case PHY_M_PS_SPEED_1000:
		return SPEED_1000;
	case PHY_M_PS_SPEED_100:
		return SPEED_100;
	default:
		return SPEED_10;
	}
}

static void yukon_link_up(struct skge_port *skge)
{
	struct skge_hw *hw = skge->hw;
	int port = skge->port;
	u16 reg;

	/* Enable Transmit FIFO Underrun */
	skge_write8(hw, SK_REG(port, GMAC_IRQ_MSK), GMAC_DEF_MSK);

	reg = gma_read16(hw, port, GM_GP_CTRL);
	if (skge->duplex == DUPLEX_FULL || skge->autoneg == AUTONEG_ENABLE)
		reg |= GM_GPCR_DUP_FULL;

	/* enable Rx/Tx */
	reg |= GM_GPCR_RX_ENA | GM_GPCR_TX_ENA;
	gma_write16(hw, port, GM_GP_CTRL, reg);

	gm_phy_write(hw, port, PHY_MARV_INT_MASK, PHY_M_IS_DEF_MSK);
	skge_link_up(skge);
}

static void yukon_link_down(struct skge_port *skge)
{
	struct skge_hw *hw = skge->hw;
	int port = skge->port;
	u16 ctrl;

	gm_phy_write(hw, port, PHY_MARV_INT_MASK, 0);

	ctrl = gma_read16(hw, port, GM_GP_CTRL);
	ctrl &= ~(GM_GPCR_RX_ENA | GM_GPCR_TX_ENA);
	gma_write16(hw, port, GM_GP_CTRL, ctrl);

	if (skge->flow_control == FLOW_MODE_REM_SEND) {
		/* restore Asymmetric Pause bit */
		gm_phy_write(hw, port, PHY_MARV_AUNE_ADV,
				  gm_phy_read(hw, port,
						   PHY_MARV_AUNE_ADV)
				  | PHY_M_AN_ASP);

	}

	yukon_reset(hw, port);
	skge_link_down(skge);

	yukon_init(hw, port);
}

static void yukon_phy_intr(struct skge_port *skge)
{
	struct skge_hw *hw = skge->hw;
	int port = skge->port;
	const char *reason = NULL;
	u16 istatus, phystat;

	istatus = gm_phy_read(hw, port, PHY_MARV_INT_STAT);
	phystat = gm_phy_read(hw, port, PHY_MARV_PHY_STAT);

	kerndbg( KERN_DEBUG, "%s: phy interrupt status 0x%x 0x%x\n",
	       skge->netdev->name, istatus, phystat);

	if (istatus & PHY_M_IS_AN_COMPL) {
		if (gm_phy_read(hw, port, PHY_MARV_AUNE_LP)
		    & PHY_M_AN_RF) {
			reason = "remote fault";
			goto failed;
		}

		if (gm_phy_read(hw, port, PHY_MARV_1000T_STAT) & PHY_B_1000S_MSF) {
			reason = "master/slave fault";
			goto failed;
		}

		if (!(phystat & PHY_M_PS_SPDUP_RES)) {
			reason = "speed/duplex";
			goto failed;
		}

		skge->duplex = (phystat & PHY_M_PS_FULL_DUP)
			? DUPLEX_FULL : DUPLEX_HALF;
		skge->speed = yukon_speed(hw, phystat);

		/* We are using IEEE 802.3z/D5.0 Table 37-4 */
		switch (phystat & PHY_M_PS_PAUSE_MSK) {
		case PHY_M_PS_PAUSE_MSK:
			skge->flow_control = FLOW_MODE_SYMMETRIC;
			break;
		case PHY_M_PS_RX_P_EN:
			skge->flow_control = FLOW_MODE_REM_SEND;
			break;
		case PHY_M_PS_TX_P_EN:
			skge->flow_control = FLOW_MODE_LOC_SEND;
			break;
		default:
			skge->flow_control = FLOW_MODE_NONE;
		}

		if (skge->flow_control == FLOW_MODE_NONE ||
		    (skge->speed < SPEED_1000 && skge->duplex == DUPLEX_HALF))
			skge_write8(hw, SK_REG(port, GMAC_CTRL), GMC_PAUSE_OFF);
		else
			skge_write8(hw, SK_REG(port, GMAC_CTRL), GMC_PAUSE_ON);
		yukon_link_up(skge);
		return;
	}

	if (istatus & PHY_M_IS_LSP_CHANGE)
		skge->speed = yukon_speed(hw, phystat);

	if (istatus & PHY_M_IS_DUP_CHANGE)
		skge->duplex = (phystat & PHY_M_PS_FULL_DUP) ? DUPLEX_FULL : DUPLEX_HALF;
	if (istatus & PHY_M_IS_LST_CHANGE) {
		if (phystat & PHY_M_PS_LINK_UP)
			yukon_link_up(skge);
		else
			yukon_link_down(skge);
	}
	return;
 failed:
	kerndbg( KERN_WARNING, "%s: autonegotiation failed (%s)\n",
	       skge->netdev->name, reason);

	/* XXX restart autonegotiation? */
}

static void skge_ramset(struct skge_hw *hw, u16 q, u32 start, size_t len)
{
	u32 end;

	start /= 8;
	len /= 8;
	end = start + len - 1;

	skge_write8(hw, RB_ADDR(q, RB_CTRL), RB_RST_CLR);
	skge_write32(hw, RB_ADDR(q, RB_START), start);
	skge_write32(hw, RB_ADDR(q, RB_WP), start);
	skge_write32(hw, RB_ADDR(q, RB_RP), start);
	skge_write32(hw, RB_ADDR(q, RB_END), end);

	if (q == Q_R1 || q == Q_R2) {
		/* Set thresholds on receive queue's */
		skge_write32(hw, RB_ADDR(q, RB_RX_UTPP),
			     start + (2*len)/3);
		skge_write32(hw, RB_ADDR(q, RB_RX_LTPP),
			     start + (len/3));
	} else {
		/* Enable store & forward on Tx queue's because
		 * Tx FIFO is only 4K on Genesis and 1K on Yukon
		 */
		skge_write8(hw, RB_ADDR(q, RB_CTRL), RB_ENA_STFWD);
	}

	skge_write8(hw, RB_ADDR(q, RB_CTRL), RB_ENA_OP_MD);
}

/* Setup Bus Memory Interface */
static void skge_qset(struct skge_port *skge, u16 q,
		      const struct skge_element *e)
{
	struct skge_hw *hw = skge->hw;
	u32 watermark = 0x600;
	u64 base = skge->dma + (e->desc - skge->mem);

	/* optimization to reduce window on 32bit/33mhz */
	if ((skge_read16(hw, B0_CTST) & (CS_BUS_CLOCK | CS_BUS_SLOT_SZ)) == 0)
		watermark /= 2;

	skge_write32(hw, Q_ADDR(q, Q_CSR), CSR_CLR_RESET);
	skge_write32(hw, Q_ADDR(q, Q_F), watermark);
	skge_write32(hw, Q_ADDR(q, Q_DA_H), (u32)(base >> 32));
	skge_write32(hw, Q_ADDR(q, Q_DA_L), (u32)base);
}

static int skge_up(struct net_device *dev)
{
	struct skge_port *skge = netdev_priv(dev);
	struct skge_hw *hw = skge->hw;
	int port = skge->port;
	u32 chunk, ram_addr;
	size_t rx_size, tx_size;
	int err;

	kerndbg( KERN_DEBUG, "%s: enabling interface\n", dev->name);

	if (dev->mtu > RX_BUF_SIZE)
		skge->rx_buf_size = dev->mtu + ETH_HLEN;
	else
		skge->rx_buf_size = RX_BUF_SIZE;


	rx_size = skge->rx_ring.count * sizeof(struct skge_rx_desc);
	tx_size = skge->tx_ring.count * sizeof(struct skge_tx_desc);
	skge->mem_size = tx_size + rx_size;
	skge->mem = pci_alloc_consistent(hw->pdev, skge->mem_size, &skge->dma);
	if (!skge->mem)
		return -ENOMEM;

	if ((u64)skge->dma >> 32 != ((u64) skge->dma + skge->mem_size) >> 32) {
		kerndbg( KERN_WARNING, "pci_alloc_consistent region crosses 4G boundary\n");
		err = -EINVAL;
		goto free_pci_mem;
	}

	memset(skge->mem, 0, skge->mem_size);

	err = skge_ring_alloc(&skge->rx_ring, skge->mem, skge->dma);
	if (err)
		goto free_pci_mem;

	err = skge_rx_fill(skge);
	if (err)
		goto free_rx_ring;

	err = skge_ring_alloc(&skge->tx_ring, skge->mem + rx_size,
			      skge->dma + rx_size);
	if (err)
		goto free_rx_ring;

	/* Initialize MAC */
	LOCK(hw->phy_mutex);
	if (hw->chip_id == CHIP_ID_GENESIS)
		genesis_mac_init(hw, port);
	else
		yukon_mac_init(hw, port);
	UNLOCK(hw->phy_mutex);

	/* Configure RAMbuffers */
	chunk = hw->ram_size / ((hw->ports + 1)*2);
	ram_addr = hw->ram_offset + 2 * chunk * port;

	skge_ramset(hw, rxqaddr[port], ram_addr, chunk);
	skge_qset(skge, rxqaddr[port], skge->rx_ring.to_clean);

	skge_ramset(hw, txqaddr[port], ram_addr+chunk, chunk);
	skge_qset(skge, txqaddr[port], skge->tx_ring.to_use);

	/* Start receiver BMU */
	smp_wmb();
	skge_write8(hw, Q_ADDR(rxqaddr[port], Q_CSR), CSR_START | CSR_IRQ_CL_F);
	skge_led(skge, LED_MODE_ON);

	return 0;

 free_rx_ring:
	skge_rx_clean(skge);
	kfree(skge->rx_ring.start);
 free_pci_mem:
	pci_free_consistent(hw->pdev, skge->mem_size, skge->mem, skge->dma);
	skge->mem = NULL;

	return err;
}

static int skge_down(struct net_device *dev)
{
	struct skge_port *skge = netdev_priv(dev);
	struct skge_hw *hw = skge->hw;
	int port = skge->port;

	if (skge->mem == NULL)
		return 0;

	kerndbg( KERN_INFO, "%s: disabling interface\n", dev->name);

	netif_stop_queue(dev);

	skge_write8(skge->hw, SK_REG(skge->port, LNK_LED_REG), LED_OFF);
	if (hw->chip_id == CHIP_ID_GENESIS)
		genesis_stop(skge);
	else
		yukon_stop(skge);

	/* Stop transmitter */
	skge_write8(hw, Q_ADDR(txqaddr[port], Q_CSR), CSR_STOP);
	skge_write32(hw, RB_ADDR(txqaddr[port], RB_CTRL),
		     RB_RST_SET|RB_DIS_OP_MD);


	/* Disable Force Sync bit and Enable Alloc bit */
	skge_write8(hw, SK_REG(port, TXA_CTRL),
		    TXA_DIS_FSYNC | TXA_DIS_ALLOC | TXA_STOP_RC);

	/* Stop Interval Timer and Limit Counter of Tx Arbiter */
	skge_write32(hw, SK_REG(port, TXA_ITI_INI), 0L);
	skge_write32(hw, SK_REG(port, TXA_LIM_INI), 0L);

	/* Reset PCI FIFO */
	skge_write32(hw, Q_ADDR(txqaddr[port], Q_CSR), CSR_SET_RESET);
	skge_write32(hw, RB_ADDR(txqaddr[port], RB_CTRL), RB_RST_SET);

	/* Reset the RAM Buffer async Tx queue */
	skge_write8(hw, RB_ADDR(port == 0 ? Q_XA1 : Q_XA2, RB_CTRL), RB_RST_SET);
	/* stop receiver */
	skge_write8(hw, Q_ADDR(rxqaddr[port], Q_CSR), CSR_STOP);
	skge_write32(hw, RB_ADDR(port ? Q_R2 : Q_R1, RB_CTRL),
		     RB_RST_SET|RB_DIS_OP_MD);
	skge_write32(hw, Q_ADDR(rxqaddr[port], Q_CSR), CSR_SET_RESET);

	if (hw->chip_id == CHIP_ID_GENESIS) {
		skge_write8(hw, SK_REG(port, TX_MFF_CTRL2), MFF_RST_SET);
		skge_write8(hw, SK_REG(port, RX_MFF_CTRL2), MFF_RST_SET);
	} else {
		skge_write8(hw, SK_REG(port, RX_GMF_CTRL_T), GMF_RST_SET);
		skge_write8(hw, SK_REG(port, TX_GMF_CTRL_T), GMF_RST_SET);
	}

	skge_led(skge, LED_MODE_OFF);

	skge_tx_clean(skge);
	skge_rx_clean(skge);

	kfree(skge->rx_ring.start);
	kfree(skge->tx_ring.start);
	pci_free_consistent(hw->pdev, skge->mem_size, skge->mem, skge->dma);
	skge->mem = NULL;
	return 0;
}

static inline int skge_avail(const struct skge_ring *ring)
{
	return ((ring->to_clean > ring->to_use) ? 0 : ring->count)
		+ (ring->to_clean - ring->to_use) - 1;
}

static int skge_xmit_frame(PacketBuf_s *skb, struct net_device *dev)
{
	struct skge_port *skge = netdev_priv(dev);
	struct skge_hw *hw = skge->hw;
	struct skge_element *e;
	struct skge_tx_desc *td;
	int i;
	u32 control, len;
	u64 map;
	unsigned long flags;

	flags = cli();
	if (spin_trylock(&skge->tx_lock))
	{
		/* Collision - tell upper layer to requeue */
		put_cpu_flags(flags);
		return -1;	//NETDEV_TX_LOCKED;
	}

	if (skge_avail(&skge->tx_ring) < 1) {
		if (!netif_queue_stopped(dev)) {
			netif_stop_queue(dev);

			kerndbg( KERN_WARNING, "%s: ring full when queue awake!\n",
			       dev->name);
		}
		spin_unlock_irqrestore(&skge->tx_lock, flags);
		return -1;	//NETDEV_TX_BUSY;
	}

	e = skge->tx_ring.to_use;
	td = e->desc;
	e->skb = skb;
	len = skb->pb_nSize;
	map = pci_map_single(hw->pdev, skb->pb_pData, len, PCI_DMA_TODEVICE);
	e->mapaddr = map;
	e->maplen = len;

	td->dma_lo = map;
	td->dma_hi = map >> 32;

	control = BMU_CHECK | BMU_EOF| BMU_IRQ_EOF;


	/* Make sure all the descriptors written */
	smp_wmb();
	td->control = BMU_OWN | BMU_SW | BMU_STF | control | len;
	smp_wmb();

	skge_write8(hw, Q_ADDR(txqaddr[skge->port], Q_CSR), CSR_START);

	kerndbg( KERN_DEBUG_LOW, "%s: tx queued, slot %td, len %d\n",
	       dev->name, e - skge->tx_ring.start, skb->pb_nSize);

	skge->tx_ring.to_use = e->next;
	if (skge_avail(&skge->tx_ring) <= TX_LOW_WATER) {
		kerndbg( KERN_DEBUG, "%s: transmit queue full\n", dev->name);
		netif_stop_queue(dev);
	}

	spin_unlock_irqrestore(&skge->tx_lock, flags);

	dev->trans_start = jiffies;

	return 0;
}


/* Free resources associated with this reing element */
static void skge_tx_free(struct skge_port *skge, struct skge_element *e,
			 u32 control)
{
	PCI_Info_s *pdev = skge->hw->pdev;

	if (control & BMU_EOF) {
		kerndbg( KERN_DEBUG,"%s: tx done slot %td\n",
		       skge->netdev->name, e - skge->tx_ring.start);

		free_pkt_buffer(e->skb);
	}
	e->skb = NULL;
}

/* Free all buffers in transmit ring */
static void skge_tx_clean(struct skge_port *skge)
{
	struct skge_element *e;
	unsigned long flags;

	spin_lock_irqsave(&skge->tx_lock, flags);
	for (e = skge->tx_ring.to_clean; e != skge->tx_ring.to_use; e = e->next) {
		struct skge_tx_desc *td = e->desc;
		skge_tx_free(skge, e, td->control);
		td->control = 0;
	}

	skge->tx_ring.to_clean = e;
	netif_wake_queue(skge->netdev);
	spin_unlock_irqrestore(&skge->tx_lock, flags);
}

static inline u16 phy_length(const struct skge_hw *hw, u32 status)
{
	if (hw->chip_id == CHIP_ID_GENESIS)
		return status >> XMR_FS_LEN_SHIFT;
	else
		return status >> GMR_FS_LEN_SHIFT;
}

static inline int bad_phy_status(const struct skge_hw *hw, u32 status)
{
	if (hw->chip_id == CHIP_ID_GENESIS)
		return (status & (XMR_FS_ERR | XMR_FS_2L_VLAN)) != 0;
	else
		return (status & GMR_FS_ANY_ERR) ||
			(status & GMR_FS_RX_OK) == 0;
}


/* Get receive buffer from descriptor.
 * Handles copy of small buffers and reallocation failures
 */
static inline PacketBuf_s *skge_rx_get(struct skge_port *skge,
					  struct skge_element *e,
					  u32 control, u32 status, u16 csum)
{
	PacketBuf_s *skb;
	u16 len = control & BMU_BBC;

	kerndbg( KERN_DEBUG_LOW, "%s: rx slot %td status 0x%x len %d\n",
	       skge->netdev->name, e - skge->rx_ring.start,
	       status, len);

	if (len > skge->rx_buf_size)
		goto error;

	if ((control & (BMU_EOF|BMU_STF)) != (BMU_STF|BMU_EOF))
		goto error;

	if (bad_phy_status(skge->hw, status))
		goto error;

	if (phy_length(skge->hw, status) != len)
		goto error;

	if (len < RX_COPY_THRESHOLD) {
		skb = alloc_pkt_buffer(len + 2);
		if (!skb)
			goto resubmit;

		memcpy(skb->pb_pData, e->skb->pb_pData, len);
		skge_rx_reuse(e, skge->rx_buf_size);
	} else {
		PacketBuf_s *nskb;
		nskb = alloc_pkt_buffer_aligned(skge->rx_buf_size);
		if (!nskb)
			goto resubmit;

		skb = e->skb;
		skge_rx_setup(skge, e, nskb, skge->rx_buf_size);
	}

	skb_put(skb, len);
	return skb;
error:

	kerndbg( KERN_DEBUG, "%s: rx err, slot %td control 0x%x status 0x%x\n",
	       skge->netdev->name, e - skge->rx_ring.start,
	       control, status);

	if (skge->hw->chip_id == CHIP_ID_GENESIS) {
		if (status & (XMR_FS_RUNT|XMR_FS_LNG_ERR))
			skge->net_stats.rx_length_errors++;
		if (status & XMR_FS_FRA_ERR)
			skge->net_stats.rx_frame_errors++;
		if (status & XMR_FS_FCS_ERR)
			skge->net_stats.rx_crc_errors++;
	} else {
		if (status & (GMR_FS_LONG_ERR|GMR_FS_UN_SIZE))
			skge->net_stats.rx_length_errors++;
		if (status & GMR_FS_FRAGMENT)
			skge->net_stats.rx_frame_errors++;
		if (status & GMR_FS_CRC_ERR)
			skge->net_stats.rx_crc_errors++;
	}

resubmit:
	skge_rx_reuse(e, skge->rx_buf_size);
	return NULL;
}

/* Free all buffers in Tx ring which are no longer owned by device */
static void skge_txirq(struct net_device *dev)
{
	struct skge_port *skge = netdev_priv(dev);
	struct skge_ring *ring = &skge->tx_ring;
	struct skge_element *e;

	smp_rmb();

	spin_lock(&skge->tx_lock);
	for (e = ring->to_clean; e != ring->to_use; e = e->next) {
		struct skge_tx_desc *td = e->desc;

		if (td->control & BMU_OWN)
			break;

		skge_tx_free(skge, e, td->control);
	}
	skge->tx_ring.to_clean = e;

	if (netif_queue_stopped(skge->netdev)
	    && skge_avail(&skge->tx_ring) > TX_LOW_WATER)
		netif_wake_queue(skge->netdev);

	spin_unlock(&skge->tx_lock);
}

static int skge_poll(struct net_device *dev)
{
	struct skge_port *skge = netdev_priv(dev);
	struct skge_hw *hw = skge->hw;
	struct skge_ring *ring = &skge->rx_ring;
	struct skge_element *e;
	int to_do = 100; /* XXXKV: Place holder value */
	int work_done = 0;

	for (e = ring->to_clean; work_done < to_do; e = e->next) {
		struct skge_rx_desc *rd = e->desc;
		PacketBuf_s *skb;
		u32 control;

		smp_rmb();
		control = rd->control;
		if (control & BMU_OWN)
			break;

		skb = skge_rx_get(skge, e, control, rd->status, rd->csum2);
		if (skb) {
			dev->last_rx = jiffies;
			if ( dev->packet_queue != NULL ){
				skb->pb_uMacHdr.pRaw = skb->pb_pData;
				enqueue_packet( dev->packet_queue, skb );
			}

			++work_done;
		}
	}
	ring->to_clean = e;

	/* restart receiver */
	smp_wmb();
	skge_write8(hw, Q_ADDR(rxqaddr[skge->port], Q_CSR), CSR_START);

	spin_lock_irq(&hw->hw_lock);
	hw->intr_mask |= rxirqmask[skge->port];
  	skge_write32(hw, B0_IMSK, hw->intr_mask);
	spin_unlock_irq(&hw->hw_lock);

	return 0;
}

/* Parity errors seem to happen when Genesis is connected to a switch
 * with no other ports present. Heartbeat error??
 */
static void skge_mac_parity(struct skge_hw *hw, int port)
{
	struct net_device *dev = hw->dev[port];

	if (dev) {
		struct skge_port *skge = netdev_priv(dev);
		++skge->net_stats.tx_heartbeat_errors;
	}

	if (hw->chip_id == CHIP_ID_GENESIS)
		skge_write16(hw, SK_REG(port, TX_MFF_CTRL1),
			     MFF_CLR_PERR);
	else
		/* HW-Bug #8: cleared by GMF_CLI_TX_FC instead of GMF_CLI_TX_PE */
		skge_write8(hw, SK_REG(port, TX_GMF_CTRL_T),
			    (hw->chip_id == CHIP_ID_YUKON && hw->chip_rev == 0)
			    ? GMF_CLI_TX_FC : GMF_CLI_TX_PE);
}

static void skge_mac_intr(struct skge_hw *hw, int port)
{
	if (hw->chip_id == CHIP_ID_GENESIS)
		genesis_mac_intr(hw, port);
	else
		yukon_mac_intr(hw, port);
}

/* Handle device specific framing and timeout interrupts */
static void skge_error_irq(struct skge_hw *hw)
{
	u32 hwstatus = skge_read32(hw, B0_HWE_ISRC);

	if (hw->chip_id == CHIP_ID_GENESIS) {
		/* clear xmac errors */
		if (hwstatus & (IS_NO_STAT_M1|IS_NO_TIST_M1))
			skge_write16(hw, RX_MFF_CTRL1, MFF_CLR_INSTAT);
		if (hwstatus & (IS_NO_STAT_M2|IS_NO_TIST_M2))
			skge_write16(hw, RX_MFF_CTRL2, MFF_CLR_INSTAT);
	} else {
		/* Timestamp (unused) overflow */
		if (hwstatus & IS_IRQ_TIST_OV)
			skge_write8(hw, GMAC_TI_ST_CTRL, GMT_ST_CLR_IRQ);
	}

	if (hwstatus & IS_RAM_RD_PAR) {
		kerndbg( KERN_WARNING, "Ram read data parity error\n");
		skge_write16(hw, B3_RI_CTRL, RI_CLR_RD_PERR);
	}

	if (hwstatus & IS_RAM_WR_PAR) {
		kerndbg( KERN_WARNING, "Ram write data parity error\n");
		skge_write16(hw, B3_RI_CTRL, RI_CLR_WR_PERR);
	}

	if (hwstatus & IS_M1_PAR_ERR)
		skge_mac_parity(hw, 0);

	if (hwstatus & IS_M2_PAR_ERR)
		skge_mac_parity(hw, 1);

	if (hwstatus & IS_R1_PAR_ERR) {
		kerndbg( KERN_WARNING, "%s: receive queue parity error\n",
		       hw->dev[0]->name);
		skge_write32(hw, B0_R1_CSR, CSR_IRQ_CL_P);
	}

	if (hwstatus & IS_R2_PAR_ERR) {
		kerndbg( KERN_WARNING, "%s: receive queue parity error\n",
		       hw->dev[1]->name);
		skge_write32(hw, B0_R2_CSR, CSR_IRQ_CL_P);
	}

	if (hwstatus & (IS_IRQ_MST_ERR|IS_IRQ_STAT)) {
		u16 pci_status, pci_cmd;

		pci_cmd = g_psBus->read_pci_config(hw->pdev->nBus, hw->pdev->nDevice, hw->pdev->nFunction, PCI_COMMAND, sizeof(pci_cmd));
		pci_status = g_psBus->read_pci_config(hw->pdev->nBus, hw->pdev->nDevice, hw->pdev->nFunction, PCI_STATUS, sizeof(pci_status));

		kerndbg( KERN_WARNING, "%s: PCI error cmd=%#x status=%#x\n",
			       "skge", pci_cmd, pci_status);

		/* Write the error bits back to clear them. */
		pci_status &= PCI_STATUS_ERROR_BITS;
		skge_write8(hw, B2_TST_CTRL1, TST_CFG_WRITE_ON);
		g_psBus->write_pci_config(hw->pdev->nBus, hw->pdev->nDevice, hw->pdev->nFunction, PCI_COMMAND, 2,
					pci_cmd | PCI_COMMAND_SERR | PCI_COMMAND_PARITY);
		g_psBus->write_pci_config(hw->pdev->nBus, hw->pdev->nDevice, hw->pdev->nFunction, PCI_STATUS, 2, pci_status | PCI_STATUS_ERROR_BITS);
		skge_write8(hw, B2_TST_CTRL1, TST_CFG_WRITE_OFF);

		/* if error still set then just ignore it */
		hwstatus = skge_read32(hw, B0_HWE_ISRC);
		if (hwstatus & IS_IRQ_STAT) {
			kerndbg( KERN_WARNING, "unable to clear error (so ignoring them)\n");
			hw->intr_mask &= ~IS_HW_ERR;
		}
	}
}

/*
 * Interrupt from PHY are handled in work queue
 * because accessing phy registers requires spin wait which might
 * cause excess interrupt latency.
 */
static void skge_extirq(struct skge_hw *hw)
{
	int port;

	LOCK(hw->phy_mutex);
	for (port = 0; port < hw->ports; port++) {
		struct net_device *dev = hw->dev[port];
		struct skge_port *skge = netdev_priv(dev);

		if (hw->chip_id != CHIP_ID_GENESIS)
			yukon_phy_intr(skge);
		else
			bcom_phy_intr(skge);
	}
	UNLOCK(hw->phy_mutex);

	hw->intr_mask |= IS_EXT_REG;
	skge_write32(hw, B0_IMSK, hw->intr_mask);
}

static int skge_intr(int irq, void *dev_id, SysCallRegs_s *regs)
{
	struct skge_hw *hw = dev_id;
	u32 status;

	/* Reading this register masks IRQ */
	status = skge_read32(hw, B0_SP_ISRC);
	if (status == 0)
		return 0;

	spin_lock(&hw->hw_lock);
	status &= hw->intr_mask;
	if (status & IS_EXT_REG) {
		hw->intr_mask &= ~IS_EXT_REG;
		skge_extirq(hw);
	}

	if (status & IS_XA1_F) {
		skge_write8(hw, Q_ADDR(Q_XA1, Q_CSR), CSR_IRQ_CL_F);
		skge_txirq(hw->dev[0]);
	}

	if (status & IS_R1_F) {
		skge_write8(hw, Q_ADDR(Q_R1, Q_CSR), CSR_IRQ_CL_F);
		hw->intr_mask &= ~IS_R1_F;
		skge_poll(hw->dev[0]);
	}

	if (status & IS_PA_TO_TX1)
		skge_write16(hw, B3_PA_CTRL, PA_CLR_TO_TX1);

	if (status & IS_PA_TO_RX1) {
		struct skge_port *skge = netdev_priv(hw->dev[0]);

		++skge->net_stats.rx_over_errors;
		skge_write16(hw, B3_PA_CTRL, PA_CLR_TO_RX1);
	}


	if (status & IS_MAC1)
		skge_mac_intr(hw, 0);

	if (hw->dev[1]) {
		if (status & IS_XA2_F) {
			skge_write8(hw, Q_ADDR(Q_XA2, Q_CSR), CSR_IRQ_CL_F);
			skge_txirq(hw->dev[1]);
		}

		if (status & IS_R2_F) {
			skge_write8(hw, Q_ADDR(Q_R2, Q_CSR), CSR_IRQ_CL_F);
			hw->intr_mask &= ~IS_R2_F;
			skge_poll(hw->dev[1]);
		}

		if (status & IS_PA_TO_RX2) {
			struct skge_port *skge = netdev_priv(hw->dev[1]);
			++skge->net_stats.rx_over_errors;
			skge_write16(hw, B3_PA_CTRL, PA_CLR_TO_RX2);
		}

		if (status & IS_PA_TO_TX2)
			skge_write16(hw, B3_PA_CTRL, PA_CLR_TO_TX2);

		if (status & IS_MAC2)
			skge_mac_intr(hw, 1);
	}

	if (status & IS_HW_ERR)
		skge_error_irq(hw);

	skge_write32(hw, B0_IMSK, hw->intr_mask);
	spin_unlock(&hw->hw_lock);

	return 0;
}

static const struct {
	u8 id;
	const char *name;
} skge_chips[] = {
	{ CHIP_ID_GENESIS,	"Genesis" },
	{ CHIP_ID_YUKON,	 "Yukon" },
	{ CHIP_ID_YUKON_LITE,	 "Yukon-Lite"},
	{ CHIP_ID_YUKON_LP,	 "Yukon-LP"},
};

static const char *skge_board_name(const struct skge_hw *hw)
{
	int i;
	static char buf[16];

	for (i = 0; i < ARRAY_SIZE(skge_chips); i++)
		if (skge_chips[i].id == hw->chip_id)
			return skge_chips[i].name;

	sprintf(buf, "chipid 0x%x", hw->chip_id);
	return buf;
}


/*
 * Setup the board data structure, but don't bring up
 * the port(s)
 */
static int skge_reset(struct skge_hw *hw)
{
	u32 reg;
	u16 ctst, pci_status;
	u8 t8, mac_cfg, pmd_type, phy_type;
	int i;

	ctst = skge_read16(hw, B0_CTST);

	/* do a SW reset */
	skge_write8(hw, B0_CTST, CS_RST_SET);
	skge_write8(hw, B0_CTST, CS_RST_CLR);

	/* clear PCI errors, if any */
	skge_write8(hw, B2_TST_CTRL1, TST_CFG_WRITE_ON);
	skge_write8(hw, B2_TST_CTRL2, 0);

	pci_status = g_psBus->read_pci_config(hw->pdev->nBus, hw->pdev->nDevice, hw->pdev->nFunction, PCI_STATUS, sizeof(pci_status));
	g_psBus->write_pci_config(hw->pdev->nBus, hw->pdev->nDevice, hw->pdev->nFunction, PCI_STATUS, 2, pci_status | PCI_STATUS_ERROR_BITS);

	skge_write8(hw, B2_TST_CTRL1, TST_CFG_WRITE_OFF);
	skge_write8(hw, B0_CTST, CS_MRST_CLR);

	/* restore CLK_RUN bits (for Yukon-Lite) */
	skge_write16(hw, B0_CTST,
		     ctst & (CS_CLK_RUN_HOT|CS_CLK_RUN_RST|CS_CLK_RUN_ENA));

	hw->chip_id = skge_read8(hw, B2_CHIP_ID);
	phy_type = skge_read8(hw, B2_E_1) & 0xf;
	pmd_type = skge_read8(hw, B2_PMD_TYP);
	hw->copper = (pmd_type == 'T' || pmd_type == '1');

	switch (hw->chip_id) {
	case CHIP_ID_GENESIS:
		switch (phy_type) {
		case SK_PHY_BCOM:
			hw->phy_addr = PHY_ADDR_BCOM;
			break;
		default:
			kerndbg( KERN_INFO, "%s: unsupported phy type 0x%x\n",
			       "skge", phy_type);
			return -EOPNOTSUPP;
		}
		break;

	case CHIP_ID_YUKON:
	case CHIP_ID_YUKON_LITE:
	case CHIP_ID_YUKON_LP:
		if (phy_type < SK_PHY_MARV_COPPER && pmd_type != 'S')
			hw->copper = 1;

		hw->phy_addr = PHY_ADDR_MARV;
		break;

	default:
		kerndbg( KERN_INFO, "%s: unsupported chip type 0x%x\n",
		       "skge", hw->chip_id);
		return -EOPNOTSUPP;
	}

	mac_cfg = skge_read8(hw, B2_MAC_CFG);
	hw->ports = (mac_cfg & CFG_SNG_MAC) ? 1 : 2;
	hw->chip_rev = (mac_cfg & CFG_CHIP_R_MSK) >> 4;

	/* read the adapters RAM size */
	t8 = skge_read8(hw, B2_E_0);
	if (hw->chip_id == CHIP_ID_GENESIS) {
		if (t8 == 3) {
			/* special case: 4 x 64k x 36, offset = 0x80000 */
			hw->ram_size = 0x100000;
			hw->ram_offset = 0x80000;
		} else
			hw->ram_size = t8 * 512;
	}
	else if (t8 == 0)
		hw->ram_size = 0x20000;
	else
		hw->ram_size = t8 * 4096;

	spinlock_init(&hw->hw_lock, "skge_hw_lock");
	hw->intr_mask = IS_HW_ERR | IS_EXT_REG | IS_PORT_1;
	if (hw->ports > 1)
		hw->intr_mask |= IS_PORT_2;

	if (hw->chip_id == CHIP_ID_GENESIS)
		genesis_init(hw);
	else {
		/* switch power to VCC (WA for VAUX problem) */
		skge_write8(hw, B0_POWER_CTRL,
			    PC_VAUX_ENA | PC_VCC_ENA | PC_VAUX_OFF | PC_VCC_ON);

		/* avoid boards with stuck Hardware error bits */
		if ((skge_read32(hw, B0_ISRC) & IS_HW_ERR) &&
		    (skge_read32(hw, B0_HWE_ISRC) & IS_IRQ_SENSOR)) {
			kerndbg( KERN_WARNING, "stuck hardware sensor bit\n");
			hw->intr_mask &= ~IS_HW_ERR;
		}

		/* Clear PHY COMA */
		skge_write8(hw, B2_TST_CTRL1, TST_CFG_WRITE_ON);
		reg = g_psBus->read_pci_config(hw->pdev->nBus, hw->pdev->nDevice, hw->pdev->nFunction, PCI_DEV_REG1, sizeof(reg));
		reg &= ~PCI_PHY_COMA;
		g_psBus->write_pci_config(hw->pdev->nBus, hw->pdev->nDevice, hw->pdev->nFunction, PCI_DEV_REG1, 2, reg);

		skge_write8(hw, B2_TST_CTRL1, TST_CFG_WRITE_OFF);


		for (i = 0; i < hw->ports; i++) {
			skge_write16(hw, SK_REG(i, GMAC_LINK_CTRL), GMLC_RST_SET);
			skge_write16(hw, SK_REG(i, GMAC_LINK_CTRL), GMLC_RST_CLR);
		}
	}

	/* turn off hardware timer (unused) */
	skge_write8(hw, B2_TI_CTRL, TIM_STOP);
	skge_write8(hw, B2_TI_CTRL, TIM_CLR_IRQ);
	skge_write8(hw, B0_LED, LED_STAT_ON);

	/* enable the Tx Arbiters */
	for (i = 0; i < hw->ports; i++)
		skge_write8(hw, SK_REG(i, TXA_CTRL), TXA_ENA_ARB);

	/* Initialize ram interface */
	skge_write16(hw, B3_RI_CTRL, RI_RST_CLR);

	skge_write8(hw, B3_RI_WTO_R1, SK_RI_TO_53);
	skge_write8(hw, B3_RI_WTO_XA1, SK_RI_TO_53);
	skge_write8(hw, B3_RI_WTO_XS1, SK_RI_TO_53);
	skge_write8(hw, B3_RI_RTO_R1, SK_RI_TO_53);
	skge_write8(hw, B3_RI_RTO_XA1, SK_RI_TO_53);
	skge_write8(hw, B3_RI_RTO_XS1, SK_RI_TO_53);
	skge_write8(hw, B3_RI_WTO_R2, SK_RI_TO_53);
	skge_write8(hw, B3_RI_WTO_XA2, SK_RI_TO_53);
	skge_write8(hw, B3_RI_WTO_XS2, SK_RI_TO_53);
	skge_write8(hw, B3_RI_RTO_R2, SK_RI_TO_53);
	skge_write8(hw, B3_RI_RTO_XA2, SK_RI_TO_53);
	skge_write8(hw, B3_RI_RTO_XS2, SK_RI_TO_53);

	skge_write32(hw, B0_HWE_IMSK, IS_ERR_MSK);

	/* Set interrupt moderation for Transmit only
	 * Receive interrupts avoided by NAPI
	 */
	skge_write32(hw, B2_IRQM_MSK, IS_XA1_F|IS_XA2_F);
	skge_write32(hw, B2_IRQM_INI, skge_usecs2clk(hw, 100));
	skge_write32(hw, B2_IRQM_CTRL, TIM_START);

	skge_write32(hw, B0_IMSK, hw->intr_mask);

	LOCK(hw->phy_mutex);
	for (i = 0; i < hw->ports; i++) {
		if (hw->chip_id == CHIP_ID_GENESIS)
			genesis_reset(hw, i);
		else
			yukon_reset(hw, i);
	}
	UNLOCK(hw->phy_mutex);

	return 0;
}

/* Device interface */
static status_t skge_dev_open( void* pNode, uint32 nFlags, void **pCookie )
{
	return 0;
}

static status_t skge_dev_close( void* pNode, void* pCookie )
{
	return 0;
}

static status_t skge_dev_ioctl( void* pNode, void* pCookie, uint32 nCommand, void* pArgs, bool bFromKernel )
{
	struct net_device* psNetDev = pNode;
    int nError = 0;

    switch( nCommand )
    {
		case SIOC_ETH_START:
		{
			psNetDev->packet_queue = pArgs;
			skge_up( psNetDev );
			break;
		}

		case SIOC_ETH_STOP:
		{
			skge_down( psNetDev );
			psNetDev->packet_queue = NULL;
			break;
		}

		case SIOCSIFHWADDR:
		{
			nError = -ENOSYS;
			break;
		}

		case SIOCGIFHWADDR:
		{
			memcpy( ((struct ifreq*)pArgs)->ifr_hwaddr.sa_data, psNetDev->dev_addr, 6 );
			break;
		}

		default:
		{
			kerndbg( KERN_WARNING, "skge_dev_ioctl(): Unknown command %d\n", (int)nCommand );
			nError = -ENOSYS;
			break;
		}
    }

    return nError;
}

static int skge_dev_write( void* pNode, void* pCookie, off_t nPosition, const void* pBuffer, size_t nSize )
{
	struct net_device* psNetDev = pNode;
	PacketBuf_s* psBuffer = alloc_pkt_buffer( nSize );
	if ( psBuffer != NULL )
	{
		memcpy( psBuffer->pb_pData, pBuffer, nSize );
		psBuffer->pb_nSize = nSize;
		skge_xmit_frame( psBuffer, psNetDev );
	}
	return nSize;
}

static DeviceOperations_s g_sDevOps = {
	skge_dev_open,
	skge_dev_close,
	skge_dev_ioctl,
	NULL,	/* dop_read */
	skge_dev_write,
	NULL,	/* dop_readv */
	NULL,	/* dop_writev */
	NULL,	/* dop_add_select_req */
	NULL	/* dop_rem_select_req */
};

/* Initialize network device */
static struct net_device *skge_devinit(struct skge_hw *hw, int port,
				       int highmem)
{
	struct skge_port *skge;
	struct net_device *dev;

	dev = kmalloc( sizeof( *dev ), MEMF_KERNEL | MEMF_CLEAR );
	if (!dev) {
		kerndbg( KERN_WARNING, "skge etherdev alloc failed");
		return NULL;
	}

	dev->name = "skge";
	dev->irq = hw->irq;
	dev->mtu = 1518;

	char zNodePath[64];
	sprintf( zNodePath, "net/eth/skge-%d", g_nPorts++ );

	kerndbg( KERN_DEBUG, "skge_probe(): Create node %s\n", zNodePath );
	dev->node_handle = create_device_node( hw->device_handle, hw->pdev->nHandle, zNodePath, &g_sDevOps, dev );

	skge = kmalloc( sizeof( *skge ), MEMF_KERNEL | MEMF_CLEAR );
	if (!skge) {
		kerndbg( KERN_WARNING, "skge etherdev alloc failed");
		return NULL;
	}
	dev->priv = skge;

	skge->netdev = dev;
	skge->hw = hw;
	skge->tx_ring.count = DEFAULT_TX_RING_SIZE;
	skge->rx_ring.count = DEFAULT_RX_RING_SIZE;

	/* Auto speed and flow control */
	skge->autoneg = AUTONEG_ENABLE;
	skge->flow_control = FLOW_MODE_SYMMETRIC;
	skge->duplex = -1;
	skge->speed = -1;
	skge->advertising = skge_supported_modes(hw);

	hw->dev[port] = dev;

	skge->port = port;

	spinlock_init(&skge->tx_lock, "skge_tx_lock");

	if (hw->chip_id != CHIP_ID_GENESIS) {
		skge->rx_csum = 1;
	}

	/* read the mac address */
	memcpy_fromio(dev->dev_addr, hw->regs + B2_MAC_1 + port*8, ETH_ALEN);
	memcpy(dev->perm_addr, dev->dev_addr, dev->addr_len);

	/* device is off until link detection */
	netif_carrier_off(dev);
	netif_stop_queue(dev);

	return dev;
}

static void skge_show_addr(struct net_device *dev)
{
	const struct skge_port *skge = netdev_priv(dev);

	printk("%s: addr %02x:%02x:%02x:%02x:%02x:%02x\n",
	       dev->name,
	       dev->dev_addr[0], dev->dev_addr[1], dev->dev_addr[2],
	       dev->dev_addr[3], dev->dev_addr[4], dev->dev_addr[5]);
}

static int skge_init_one( PCI_Info_s *pDev, int nDeviceID, struct skge_hw **ppCard )
{
	struct net_device *dev, *dev1;
	struct skge_hw *hw;
	unsigned long skgereg_base;
	int err, using_dac = 0;

	/* Enable device */
	uint16 nOldCommand, nNewCommand;

	nOldCommand = g_psBus->read_pci_config( pDev->nBus, pDev->nDevice, pDev->nFunction, PCI_COMMAND, 2 );
	nNewCommand = nOldCommand | ( PCI_COMMAND_MEMORY & 7);
	if( nOldCommand != nNewCommand )
		g_psBus->write_pci_config( pDev->nBus, pDev->nDevice, pDev->nFunction, PCI_COMMAND, 2, nNewCommand );

	g_psBus->enable_pci_master( pDev->nBus, pDev->nDevice, pDev->nFunction );

	hw = kmalloc( sizeof( *hw ), MEMF_KERNEL | MEMF_CLEAR );
	if( NULL == hw )
	{
		kerndbg( KERN_WARNING, "skge_init_one(): Could not allocate memory for device.\n" );
		goto err_out_free_regions;
	}

	hw->pdev = pDev;
	hw->phy_mutex = create_semaphore( "skge_phy_mutex", 1, SEMSTYLE_COUNTING );

	skgereg_base = pDev->u.h0.nBase0 & PCI_ADDRESS_MEMORY_32_MASK;

	/* Map the device registers into memory. The address is page-aligned. */
	hw->reg_area = create_area ("skge_register", (void **)&hw->regs, 0x4000, 0x4000, AREA_KERNEL | AREA_ANY_ADDRESS, AREA_NO_LOCK );
	if( hw->reg_area < 0 )
	{
		kerndbg ( KERN_DEBUG, "skge: failed to create register area (%d)\n", hw->reg_area );
		err = -EIO;
		goto err_out_free_hw;
	}

	if( remap_area (hw->reg_area, (void *)(skgereg_base & PAGE_MASK) ) < 0 )
	{
		kerndbg( KERN_DEBUG, "skge: failed to remap register area (%d)\n", hw->reg_area );
		err = -EIO;
		goto err_out_free_hw;
	}

	hw->regs = (void*)( (uint32)hw->regs + ( skgereg_base - ( skgereg_base & PAGE_MASK ) ) );
	hw->device_handle = nDeviceID;
	hw->irq = pDev->u.h0.nInterruptLine;
	hw->irq_handle = request_irq(hw->irq, skge_intr, NULL, SA_SHIRQ, "skge", hw);

	err = skge_reset(hw);
	if (err)
		goto err_out_free_irq;

	printk( "%s: addr 0x%Lx irq %d chip %s rev %d\n",
	       "skge",
	       (unsigned long long)skgereg_base, hw->irq,
	       skge_board_name(hw), hw->chip_rev);

	if ((dev = skge_devinit(hw, 0, using_dac)) == NULL)
		goto err_out_led_off;

	if (!is_valid_ether_addr(dev->dev_addr)) {
		kerndbg( KERN_WARNING, "%s: bad (zero?) ethernet address in rom\n",
		       dev->name);
		err = -EIO;
		goto err_out_free_netdev;
	}

	skge_show_addr(dev);

	if (hw->ports > 1 && (dev1 = skge_devinit(hw, 1, using_dac)))
		skge_show_addr(dev1);

	*ppCard = hw;

	return 0;

err_out_free_netdev:
	kfree(dev);
err_out_led_off:
	skge_write16(hw, B0_LED, LED_STAT_OFF);
err_out_free_irq:
	release_irq(hw->irq, hw->irq_handle);
err_out_iounmap:
	delete_area(hw->reg_area);
	hw->regs=NULL;
err_out_free_hw:
	kfree(hw);
err_out_free_regions:
err_out_disable_pdev:
err_out:
	return err;
}

static int skge_probe( int nDeviceID )
{
    PCI_Info_s sInfo;
    bool bCardsFound = false;
    int i, j;
   
	for( i = 0; g_psBus->get_pci_info( &sInfo, i ) == 0; ++i )
	{
		for( j = 0; skge_pci_tbl[j].vendor_id != 0; j++ )
		{
			if ( sInfo.nVendorID == skge_pci_tbl[j].vendor_id &&
				 sInfo.nDeviceID == skge_pci_tbl[j].device_id)
			{
				struct skge_hw *pCard = NULL;

				if( claim_device( nDeviceID, sInfo.nHandle, "SysKonnect Gigabit Ethernet", DEVICE_NET ) < 0 )
					continue;

				if( skge_init_one( &sInfo, nDeviceID, &pCard ) == 0 )
				{
					set_device_data( sInfo.nHandle, pCard );
					bCardsFound = true;
				}
			}
		}
	}

	if( bCardsFound == false )
		disable_device( nDeviceID );

	return bCardsFound ? 0 : -ENODEV;
}

/* Power management */
status_t device_suspend( int nDeviceID, int nDeviceHandle, void* pPrivateData )
{
	struct skge_hw *hw  = pPrivateData;
	int i, wol = 0;

	for (i = 0; i < 2; i++) {
		struct net_device *dev = hw->dev[i];

		if (dev) {
			struct skge_port *skge = netdev_priv(dev);
			netif_carrier_off(dev);
			if (skge->wol)
				netif_stop_queue(dev);
			else
				skge_down(dev);
			wol |= skge->wol;
		}
	}

	pci_save_state(hw->pdev, hw->pci_state);
	pci_set_power_state(hw->pdev, PCI_D3hot);

	return 0;
}

status_t device_resume( int nDeviceID, int nDeviceHandle, void* pPrivateData )
{
	struct skge_hw *hw  = pPrivateData;
	int i;

	pci_set_power_state(hw->pdev, PCI_D0);
	pci_restore_state(hw->pdev, hw->pci_state);

	skge_reset(hw);

	return 0;
}

/* Driver management */
status_t device_init( int nDeviceID )
{
	/* Get PCI bus */
	g_psBus = get_busmanager( PCI_BUS_NAME, PCI_BUS_VERSION );
	if( g_psBus == NULL )
		return -ENODEV;

	return skge_probe( nDeviceID );
}

void device_release( int nDeviceID, int nDeviceHandle, void* pPrivateData )
{
	struct net_device *pNetDev = pPrivateData;
	struct skge_hw *pPrivate = netdev_priv( pNetDev );

	set_device_data( nDeviceHandle, NULL );

	release_irq( pPrivate->irq, pPrivate->irq_handle );

	delete_area( pPrivate->reg_area );
	pPrivate->regs = NULL;

	kfree( pPrivate );
	kfree( pNetDev );

	release_device( nDeviceHandle );
}

status_t device_uninit( int nDeviceID )
{
	return 0;
}

