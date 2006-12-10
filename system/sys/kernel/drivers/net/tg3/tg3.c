/*
 * tg3.c: Broadcom Tigon3 ethernet driver.
 *
 * Copyright (C) 2006 Kristian Van Der Vliet (vanders@liqwyd.com)
 * Copyright (C) 2001, 2002, 2003, 2004 David S. Miller (davem@redhat.com)
 * Copyright (C) 2001, 2002, 2003 Jeff Garzik (jgarzik@pobox.com)
 * Copyright (C) 2004 Sun Microsystems Inc.
 * Copyright (C) 2005 Broadcom Corporation.
 *
 * Firmware is:
 *	Derived from proprietary unpublished source code,
 *	Copyright (C) 2000-2003 Broadcom Corporation.
 *
 *	Permission is hereby granted for the distribution of this firmware
 *	data in hexadecimal or equivalent format, provided this copyright
 *	notice is accompanying it.
 */

#include <atheos/kernel.h>
#include <atheos/kdebug.h>
#include <atheos/types.h>
#include <atheos/device.h>
#include <atheos/pci.h>
#include <atheos/spinlock.h>
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

#include <tg3.h>

static PCI_bus_s* g_psBus;

/* XXXKV: Enable debugging */
#if 0
 #undef DEBUG_LIMIT
 #define DEBUG_LIMIT KERN_DEBUG_LOW
#endif

#define TG3_DEF_MAC_MODE	0
#define TG3_DEF_RX_MODE		0
#define TG3_DEF_TX_MODE		0

/* length of time before we decide the hardware is borked,
 * and dev->tx_timeout() should be called to fix the problem
 */
#define TG3_TX_TIMEOUT			(5 * HZ)

/* hardware minimum and maximum for a single frame's data payload */
#define TG3_MIN_MTU			60
#define TG3_MAX_MTU(tp)	\
	((tp->tg3_flags2 & TG3_FLG2_JUMBO_CAPABLE) ? 9000 : 1500)

/* These numbers seem to be hard coded in the NIC firmware somehow.
 * You can't change the ring sizes, but you can change where you place
 * them in the NIC onboard memory.
 */
#define TG3_RX_RING_SIZE		512
#define TG3_DEF_RX_RING_PENDING		200
#define TG3_RX_JUMBO_RING_SIZE		256
#define TG3_DEF_RX_JUMBO_RING_PENDING	100

/* Do not place this n-ring entries value into the tp struct itself,
 * we really want to expose these constants to GCC so that modulo et
 * al.  operations are done with shifts and masks instead of with
 * hw multiply/modulo instructions.  Another solution would be to
 * replace things like '% foo' with '& (foo - 1)'.
 */
#define TG3_RX_RCB_RING_SIZE(tp)	\
	((tp->tg3_flags2 & TG3_FLG2_5705_PLUS) ?  512 : 1024)

#define TG3_TX_RING_SIZE		512
#define TG3_DEF_TX_RING_PENDING		(TG3_TX_RING_SIZE - 1)

#define TG3_RX_RING_BYTES	(sizeof(struct tg3_rx_buffer_desc) * \
				 TG3_RX_RING_SIZE)
#define TG3_RX_JUMBO_RING_BYTES	(sizeof(struct tg3_rx_buffer_desc) * \
			         TG3_RX_JUMBO_RING_SIZE)
#define TG3_RX_RCB_RING_BYTES(tp) (sizeof(struct tg3_rx_buffer_desc) * \
			           TG3_RX_RCB_RING_SIZE(tp))
#define TG3_TX_RING_BYTES	(sizeof(struct tg3_tx_buffer_desc) * \
				 TG3_TX_RING_SIZE)
#define NEXT_TX(N)		(((N) + 1) & (TG3_TX_RING_SIZE - 1))

#define RX_PKT_BUF_SZ		(1536 + tp->rx_offset + 64)
#define RX_JUMBO_PKT_BUF_SZ	(9046 + tp->rx_offset + 64)

/* minimum number of free TX descriptors required to wake up TX process */
#define TG3_TX_WAKEUP_THRESH		(TG3_TX_RING_SIZE / 4)

static struct pci_device_id tg3_pci_tbl[] = {
	{ PCI_VENDOR_ID_BROADCOM, PCI_DEVICE_ID_TIGON3_5700,
	  PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0UL },
	{ PCI_VENDOR_ID_BROADCOM, PCI_DEVICE_ID_TIGON3_5701,
	  PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0UL },
	{ PCI_VENDOR_ID_BROADCOM, PCI_DEVICE_ID_TIGON3_5702,
	  PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0UL },
	{ PCI_VENDOR_ID_BROADCOM, PCI_DEVICE_ID_TIGON3_5703,
	  PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0UL },
	{ PCI_VENDOR_ID_BROADCOM, PCI_DEVICE_ID_TIGON3_5704,
	  PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0UL },
	{ PCI_VENDOR_ID_BROADCOM, PCI_DEVICE_ID_TIGON3_5702FE,
	  PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0UL },
	{ PCI_VENDOR_ID_BROADCOM, PCI_DEVICE_ID_TIGON3_5705,
	  PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0UL },
	{ PCI_VENDOR_ID_BROADCOM, PCI_DEVICE_ID_TIGON3_5705_2,
	  PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0UL },
	{ PCI_VENDOR_ID_BROADCOM, PCI_DEVICE_ID_TIGON3_5705M,
	  PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0UL },
	{ PCI_VENDOR_ID_BROADCOM, PCI_DEVICE_ID_TIGON3_5705M_2,
	  PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0UL },
	{ PCI_VENDOR_ID_BROADCOM, PCI_DEVICE_ID_TIGON3_5702X,
	  PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0UL },
	{ PCI_VENDOR_ID_BROADCOM, PCI_DEVICE_ID_TIGON3_5703X,
	  PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0UL },
	{ PCI_VENDOR_ID_BROADCOM, PCI_DEVICE_ID_TIGON3_5704S,
	  PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0UL },
	{ PCI_VENDOR_ID_BROADCOM, PCI_DEVICE_ID_TIGON3_5702A3,
	  PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0UL },
	{ PCI_VENDOR_ID_BROADCOM, PCI_DEVICE_ID_TIGON3_5703A3,
	  PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0UL },
	{ PCI_VENDOR_ID_BROADCOM, PCI_DEVICE_ID_TIGON3_5782,
	  PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0UL },
	{ PCI_VENDOR_ID_BROADCOM, PCI_DEVICE_ID_TIGON3_5788,
	  PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0UL },
	{ PCI_VENDOR_ID_BROADCOM, PCI_DEVICE_ID_TIGON3_5789,
	  PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0UL },
	{ PCI_VENDOR_ID_BROADCOM, PCI_DEVICE_ID_TIGON3_5901,
	  PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0UL },
	{ PCI_VENDOR_ID_BROADCOM, PCI_DEVICE_ID_TIGON3_5901_2,
	  PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0UL },
	{ PCI_VENDOR_ID_BROADCOM, PCI_DEVICE_ID_TIGON3_5704S_2,
	  PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0UL },
	{ PCI_VENDOR_ID_BROADCOM, PCI_DEVICE_ID_TIGON3_5705F,
	  PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0UL },
	{ PCI_VENDOR_ID_BROADCOM, PCI_DEVICE_ID_TIGON3_5720,
	  PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0UL },
	{ PCI_VENDOR_ID_BROADCOM, PCI_DEVICE_ID_TIGON3_5721,
	  PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0UL },
	{ PCI_VENDOR_ID_BROADCOM, PCI_DEVICE_ID_TIGON3_5750,
	  PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0UL },
	{ PCI_VENDOR_ID_BROADCOM, PCI_DEVICE_ID_TIGON3_5751,
	  PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0UL },
	{ PCI_VENDOR_ID_BROADCOM, PCI_DEVICE_ID_TIGON3_5750M,
	  PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0UL },
	{ PCI_VENDOR_ID_BROADCOM, PCI_DEVICE_ID_TIGON3_5751M,
	  PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0UL },
	{ PCI_VENDOR_ID_BROADCOM, PCI_DEVICE_ID_TIGON3_5751F,
	  PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0UL },
	{ PCI_VENDOR_ID_BROADCOM, PCI_DEVICE_ID_TIGON3_5752,
	  PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0UL },
	{ PCI_VENDOR_ID_BROADCOM, PCI_DEVICE_ID_TIGON3_5752M,
	  PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0UL },
	{ PCI_VENDOR_ID_BROADCOM, PCI_DEVICE_ID_TIGON3_5753,
	  PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0UL },
	{ PCI_VENDOR_ID_BROADCOM, PCI_DEVICE_ID_TIGON3_5753M,
	  PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0UL },
	{ PCI_VENDOR_ID_BROADCOM, PCI_DEVICE_ID_TIGON3_5753F,
	  PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0UL },
	{ PCI_VENDOR_ID_BROADCOM, PCI_DEVICE_ID_TIGON3_5754,
	  PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0UL },
	{ PCI_VENDOR_ID_BROADCOM, PCI_DEVICE_ID_TIGON3_5754M,
	  PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0UL },
	{ PCI_VENDOR_ID_BROADCOM, PCI_DEVICE_ID_TIGON3_5755,
	  PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0UL },
	{ PCI_VENDOR_ID_BROADCOM, PCI_DEVICE_ID_TIGON3_5755M,
	  PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0UL },
	{ PCI_VENDOR_ID_BROADCOM, PCI_DEVICE_ID_TIGON3_5786,
	  PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0UL },
	{ PCI_VENDOR_ID_BROADCOM, PCI_DEVICE_ID_TIGON3_5787,
	  PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0UL },
	{ PCI_VENDOR_ID_BROADCOM, PCI_DEVICE_ID_TIGON3_5787M,
	  PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0UL },
	{ PCI_VENDOR_ID_BROADCOM, PCI_DEVICE_ID_TIGON3_5714,
	  PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0UL },
	{ PCI_VENDOR_ID_BROADCOM, PCI_DEVICE_ID_TIGON3_5714S,
	  PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0UL },
	{ PCI_VENDOR_ID_BROADCOM, PCI_DEVICE_ID_TIGON3_5715,
	  PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0UL },
	{ PCI_VENDOR_ID_BROADCOM, PCI_DEVICE_ID_TIGON3_5715S,
	  PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0UL },
	{ PCI_VENDOR_ID_BROADCOM, PCI_DEVICE_ID_TIGON3_5780,
	  PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0UL },
	{ PCI_VENDOR_ID_BROADCOM, PCI_DEVICE_ID_TIGON3_5780S,
	  PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0UL },
	{ PCI_VENDOR_ID_BROADCOM, PCI_DEVICE_ID_TIGON3_5781,
	  PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0UL },
	{ PCI_VENDOR_ID_SYSKONNECT, PCI_DEVICE_ID_SYSKONNECT_9DXX,
	  PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0UL },
	{ PCI_VENDOR_ID_SYSKONNECT, PCI_DEVICE_ID_SYSKONNECT_9MXX,
	  PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0UL },
	{ PCI_VENDOR_ID_ALTIMA, PCI_DEVICE_ID_ALTIMA_AC1000,
	  PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0UL },
	{ PCI_VENDOR_ID_ALTIMA, PCI_DEVICE_ID_ALTIMA_AC1001,
	  PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0UL },
	{ PCI_VENDOR_ID_ALTIMA, PCI_DEVICE_ID_ALTIMA_AC1003,
	  PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0UL },
	{ PCI_VENDOR_ID_ALTIMA, PCI_DEVICE_ID_ALTIMA_AC9100,
	  PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0UL },
	{ PCI_VENDOR_ID_APPLE, PCI_DEVICE_ID_APPLE_TIGON3,
	  PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0UL },
	{ 0, }
};

static void tg3_write32(struct tg3 *tp, u32 off, u32 val)
{
	writel(val, tp->regs + off);
}

static u32 tg3_read32(struct tg3 *tp, u32 off)
{
	return (readl(tp->regs + off)); 
}

static void tg3_write_indirect_reg32(struct tg3 *tp, u32 off, u32 val)
{
	unsigned long flags;

	spin_lock_irqsave(&tp->indirect_lock, flags);
	g_psBus->write_pci_config(tp->pdev->nBus, tp->pdev->nDevice, tp->pdev->nFunction, TG3PCI_REG_BASE_ADDR, 4, off);
	g_psBus->write_pci_config(tp->pdev->nBus, tp->pdev->nDevice, tp->pdev->nFunction, TG3PCI_REG_DATA, 4, val);
	spin_unlock_irqrestore(&tp->indirect_lock, flags);
}

static void tg3_write_flush_reg32(struct tg3 *tp, u32 off, u32 val)
{
	writel(val, tp->regs + off);
	readl(tp->regs + off);
}

static u32 tg3_read_indirect_reg32(struct tg3 *tp, u32 off)
{
	unsigned long flags;
	u32 val;

	spin_lock_irqsave(&tp->indirect_lock, flags);
	g_psBus->write_pci_config(tp->pdev->nBus, tp->pdev->nDevice, tp->pdev->nFunction, TG3PCI_REG_BASE_ADDR, 4, (uint32)off);
	val = g_psBus->read_pci_config(tp->pdev->nBus, tp->pdev->nDevice, tp->pdev->nFunction, TG3PCI_REG_DATA, sizeof(val));
	spin_unlock_irqrestore(&tp->indirect_lock, flags);
	return val;
}

static void tg3_write_indirect_mbox(struct tg3 *tp, u32 off, u32 val)
{
	unsigned long flags;

	if (off == (MAILBOX_RCVRET_CON_IDX_0 + TG3_64BIT_REG_LOW)) {
		g_psBus->write_pci_config(tp->pdev->nBus, tp->pdev->nDevice, tp->pdev->nFunction, TG3PCI_RCV_RET_RING_CON_IDX +
						TG3_64BIT_REG_LOW, 4, val);
		return;
	}
	if (off == (MAILBOX_RCV_STD_PROD_IDX + TG3_64BIT_REG_LOW)) {
		g_psBus->write_pci_config(tp->pdev->nBus, tp->pdev->nDevice, tp->pdev->nFunction, TG3PCI_STD_RING_PROD_IDX +
						TG3_64BIT_REG_LOW, 4, val);
		return;
	}

	spin_lock_irqsave(&tp->indirect_lock, flags);
	g_psBus->write_pci_config(tp->pdev->nBus, tp->pdev->nDevice, tp->pdev->nFunction, TG3PCI_REG_BASE_ADDR, 4, off + 0x5600);
	g_psBus->write_pci_config(tp->pdev->nBus, tp->pdev->nDevice, tp->pdev->nFunction, TG3PCI_REG_DATA, 4, val);
	spin_unlock_irqrestore(&tp->indirect_lock, flags);

	/* In indirect mode when disabling interrupts, we also need
	 * to clear the interrupt bit in the GRC local ctrl register.
	 */
	if ((off == (MAILBOX_INTERRUPT_0 + TG3_64BIT_REG_LOW)) &&
	    (val == 0x1)) {
		g_psBus->write_pci_config(tp->pdev->nBus, tp->pdev->nDevice, tp->pdev->nFunction, TG3PCI_MISC_LOCAL_CTRL,
						4, tp->grc_local_ctrl|GRC_LCLCTRL_CLEARINT);

	}
}

static u32 tg3_read_indirect_mbox(struct tg3 *tp, u32 off)
{
	unsigned long flags;
	u32 val;

	spin_lock_irqsave(&tp->indirect_lock, flags);
	g_psBus->write_pci_config(tp->pdev->nBus, tp->pdev->nDevice, tp->pdev->nFunction, TG3PCI_REG_BASE_ADDR, 4, (uint32)off + 0x5600);
	val = g_psBus->read_pci_config(tp->pdev->nBus, tp->pdev->nDevice, tp->pdev->nFunction, TG3PCI_REG_DATA, sizeof(val));
	spin_unlock_irqrestore(&tp->indirect_lock, flags);
	return val;
}

/* usec_wait specifies the wait time in usec when writing to certain registers
 * where it is unsafe to read back the register without some delay.
 * GRC_LOCAL_CTRL is one example if the GPIOs are toggled to switch power.
 * TG3PCI_CLOCK_CTRL is another example if the clock frequencies are changed.
 */
static void _tw32_flush(struct tg3 *tp, u32 off, u32 val, u32 usec_wait)
{
	if ((tp->tg3_flags & TG3_FLAG_PCIX_TARGET_HWBUG) ||
	    (tp->tg3_flags2 & TG3_FLG2_ICH_WORKAROUND))
		/* Non-posted methods */
		tp->write32(tp, off, val);
	else {
		/* Posted method */
		tg3_write32(tp, off, val);
		if (usec_wait)
			udelay(usec_wait);
		tp->read32(tp, off);
	}
	/* Wait again after the read for the posted method to guarantee that
	 * the wait time is met.
	 */
	if (usec_wait)
		udelay(usec_wait);
}

static inline void tw32_mailbox_flush(struct tg3 *tp, u32 off, u32 val)
{
	tp->write32_mbox(tp, off, val);
	if (!(tp->tg3_flags & TG3_FLAG_MBOX_WRITE_REORDER) &&
	    !(tp->tg3_flags2 & TG3_FLG2_ICH_WORKAROUND))
		tp->read32_mbox(tp, off);
}

static void tg3_write32_tx_mbox(struct tg3 *tp, u32 off, u32 val)
{
	void *mbox = tp->regs + off;
	writel(val, mbox);
	if (tp->tg3_flags & TG3_FLAG_TXD_MBOX_HWBUG)
		writel(val, mbox);
	if (tp->tg3_flags & TG3_FLAG_MBOX_WRITE_REORDER)
		readl(mbox);
}

#define tw32_mailbox(reg, val)	tp->write32_mbox(tp, reg, val)
#define tw32_mailbox_f(reg, val)	tw32_mailbox_flush(tp, (reg), (val))
#define tw32_rx_mbox(reg, val)	tp->write32_rx_mbox(tp, reg, val)
#define tw32_tx_mbox(reg, val)	tp->write32_tx_mbox(tp, reg, val)
#define tr32_mailbox(reg)	tp->read32_mbox(tp, reg)

#define tw32(reg,val)		tp->write32(tp, reg, val)
#define tw32_f(reg,val)		_tw32_flush(tp,(reg),(val), 0)
#define tw32_wait_f(reg,val,us)	_tw32_flush(tp,(reg),(val), (us))
#define tr32(reg)		tp->read32(tp, reg)

static void tg3_write_mem(struct tg3 *tp, u32 off, u32 val)
{
	unsigned long flags;

	spin_lock_irqsave(&tp->indirect_lock, flags);
	if (tp->tg3_flags & TG3_FLAG_SRAM_USE_CONFIG) {
		g_psBus->write_pci_config(tp->pdev->nBus, tp->pdev->nDevice, tp->pdev->nFunction, TG3PCI_MEM_WIN_BASE_ADDR, 4, off);
		g_psBus->write_pci_config(tp->pdev->nBus, tp->pdev->nDevice, tp->pdev->nFunction, TG3PCI_MEM_WIN_DATA, 4, val);

		/* Always leave this as zero. */
		g_psBus->write_pci_config(tp->pdev->nBus, tp->pdev->nDevice, tp->pdev->nFunction, TG3PCI_MEM_WIN_BASE_ADDR, 4, 0);
	} else {
		tw32_f(TG3PCI_MEM_WIN_BASE_ADDR, off);
		tw32_f(TG3PCI_MEM_WIN_DATA, val);

		/* Always leave this as zero. */
		tw32_f(TG3PCI_MEM_WIN_BASE_ADDR, 0);
	}
	spin_unlock_irqrestore(&tp->indirect_lock, flags);
}

static void tg3_read_mem(struct tg3 *tp, u32 off, u32 *val)
{
	unsigned long flags;

	spin_lock_irqsave(&tp->indirect_lock, flags);
	if (tp->tg3_flags & TG3_FLAG_SRAM_USE_CONFIG) {
		g_psBus->write_pci_config(tp->pdev->nBus, tp->pdev->nDevice, tp->pdev->nFunction, TG3PCI_MEM_WIN_BASE_ADDR, 4, off);
		*val = g_psBus->read_pci_config(tp->pdev->nBus, tp->pdev->nDevice, tp->pdev->nFunction, TG3PCI_MEM_WIN_DATA, 4);

		/* Always leave this as zero. */
		g_psBus->write_pci_config(tp->pdev->nBus, tp->pdev->nDevice, tp->pdev->nFunction, TG3PCI_MEM_WIN_BASE_ADDR, 4, 0);
	} else {
		tw32_f(TG3PCI_MEM_WIN_BASE_ADDR, off);
		*val = tr32(TG3PCI_MEM_WIN_DATA);

		/* Always leave this as zero. */
		tw32_f(TG3PCI_MEM_WIN_BASE_ADDR, 0);
	}
	spin_unlock_irqrestore(&tp->indirect_lock, flags);
}

static void tg3_disable_ints(struct tg3 *tp)
{
	kerndbg( KERN_DEBUG, "%s\n", __FUNCTION__ );

	tw32(TG3PCI_MISC_HOST_CTRL,
	     (tp->misc_host_ctrl | MISC_HOST_CTRL_MASK_PCI_INT));
	tw32_mailbox_f(MAILBOX_INTERRUPT_0 + TG3_64BIT_REG_LOW, 0x00000001);
}

static inline void tg3_cond_int(struct tg3 *tp)
{
	if (!(tp->tg3_flags & TG3_FLAG_TAGGED_STATUS) &&
	    (tp->hw_status->status & SD_STATUS_UPDATED))
		tw32(GRC_LOCAL_CTRL, tp->grc_local_ctrl | GRC_LCLCTRL_SETINT);
}

static void tg3_enable_ints(struct tg3 *tp)
{
	tp->irq_sync = 0;
	smp_wmb();

	kerndbg( KERN_DEBUG, "%s\n", __FUNCTION__ );

	tw32(TG3PCI_MISC_HOST_CTRL,
	     (tp->misc_host_ctrl & ~MISC_HOST_CTRL_MASK_PCI_INT));
	tw32_mailbox_f(MAILBOX_INTERRUPT_0 + TG3_64BIT_REG_LOW,
		       (tp->last_tag << 24));
	if (tp->tg3_flags2 & TG3_FLG2_1SHOT_MSI)
		tw32_mailbox_f(MAILBOX_INTERRUPT_0 + TG3_64BIT_REG_LOW,
			       (tp->last_tag << 24));
	tg3_cond_int(tp);
}

static inline unsigned int tg3_has_work(struct tg3 *tp)
{
	struct tg3_hw_status *sblk = tp->hw_status;
	unsigned int work_exists = 0;

	/* check for phy events */
	if (!(tp->tg3_flags &
	      (TG3_FLAG_USE_LINKCHG_REG |
	       TG3_FLAG_POLL_SERDES))) {
		if (sblk->status & SD_STATUS_LINK_CHG)
			work_exists = 1;
	}
	/* check for RX/TX work to do */
	if (sblk->idx[0].tx_consumer != tp->tx_cons ||
	    sblk->idx[0].rx_producer != tp->rx_rcb_ptr)
		work_exists = 1;

	return work_exists;
}

/* tg3_restart_ints
 *  similar to tg3_enable_ints, but it accurately determines whether there
 *  is new work pending and can return without flushing the PIO write
 *  which reenables interrupts 
 */
static void tg3_restart_ints(struct tg3 *tp)
{
	kerndbg( KERN_DEBUG, "%s\n", __FUNCTION__ );

	tw32_mailbox(MAILBOX_INTERRUPT_0 + TG3_64BIT_REG_LOW,
		     tp->last_tag << 24);
	smp_wmb();

	/* When doing tagged status, this work check is unnecessary.
	 * The last_tag we write above tells the chip which piece of
	 * work we've completed.
	 */
	if (!(tp->tg3_flags & TG3_FLAG_TAGGED_STATUS) &&
	    tg3_has_work(tp))
		tw32(HOSTCC_MODE, tp->coalesce_mode |
		     (HOSTCC_MODE_ENABLE | HOSTCC_MODE_NOW));
}

static inline void tg3_netif_stop(struct tg3 *tp)
{
	tp->dev->trans_start = jiffies;	/* prevent tx timeout */
}

static inline void tg3_netif_start(struct tg3 *tp)
{
	netif_wake_queue(tp->dev);
	/* NOTE: unconditional netif_wake_queue is only appropriate
	 * so long as all callers are assured to have free tx slots
	 * (such as after tg3_init_hw)
	 */
	tp->hw_status->status |= SD_STATUS_UPDATED;
	tg3_enable_ints(tp);
}

static void tg3_switch_clocks(struct tg3 *tp)
{
	u32 clock_ctrl = tr32(TG3PCI_CLOCK_CTRL);
	u32 orig_clock_ctrl;

	if (tp->tg3_flags2 & TG3_FLG2_5780_CLASS)
		return;

	orig_clock_ctrl = clock_ctrl;
	clock_ctrl &= (CLOCK_CTRL_FORCE_CLKRUN |
		       CLOCK_CTRL_CLKRUN_OENABLE |
		       0x1f);
	tp->pci_clock_ctrl = clock_ctrl;

	if (tp->tg3_flags2 & TG3_FLG2_5705_PLUS) {
		if (orig_clock_ctrl & CLOCK_CTRL_625_CORE) {
			tw32_wait_f(TG3PCI_CLOCK_CTRL,
				    clock_ctrl | CLOCK_CTRL_625_CORE, 40);
		}
	} else if ((orig_clock_ctrl & CLOCK_CTRL_44MHZ_CORE) != 0) {
		tw32_wait_f(TG3PCI_CLOCK_CTRL,
			    clock_ctrl |
			    (CLOCK_CTRL_44MHZ_CORE | CLOCK_CTRL_ALTCLK),
			    40);
		tw32_wait_f(TG3PCI_CLOCK_CTRL,
			    clock_ctrl | (CLOCK_CTRL_ALTCLK),
			    40);
	}
	tw32_wait_f(TG3PCI_CLOCK_CTRL, clock_ctrl, 40);
}

#define PHY_BUSY_LOOPS	5000

static int tg3_readphy(struct tg3 *tp, int reg, u32 *val)
{
	u32 frame_val;
	unsigned int loops;
	int ret;

	if ((tp->mi_mode & MAC_MI_MODE_AUTO_POLL) != 0) {
		tw32_f(MAC_MI_MODE,
		     (tp->mi_mode & ~MAC_MI_MODE_AUTO_POLL));
		udelay(80);
	}

	*val = 0x0;

	frame_val  = ((PHY_ADDR << MI_COM_PHY_ADDR_SHIFT) &
		      MI_COM_PHY_ADDR_MASK);
	frame_val |= ((reg << MI_COM_REG_ADDR_SHIFT) &
		      MI_COM_REG_ADDR_MASK);
	frame_val |= (MI_COM_CMD_READ | MI_COM_START);
	
	tw32_f(MAC_MI_COM, frame_val);

	loops = PHY_BUSY_LOOPS;
	while (loops != 0) {
		udelay(10);
		frame_val = tr32(MAC_MI_COM);

		if ((frame_val & MI_COM_BUSY) == 0) {
			udelay(5);
			frame_val = tr32(MAC_MI_COM);
			break;
		}
		loops -= 1;
	}

	ret = -EBUSY;
	if (loops != 0) {
		*val = frame_val & MI_COM_DATA_MASK;
		ret = 0;
	}

	if ((tp->mi_mode & MAC_MI_MODE_AUTO_POLL) != 0) {
		tw32_f(MAC_MI_MODE, tp->mi_mode);
		udelay(80);
	}

	return ret;
}

static int tg3_writephy(struct tg3 *tp, int reg, u32 val)
{
	u32 frame_val;
	unsigned int loops;
	int ret;

	if ((tp->mi_mode & MAC_MI_MODE_AUTO_POLL) != 0) {
		tw32_f(MAC_MI_MODE,
		     (tp->mi_mode & ~MAC_MI_MODE_AUTO_POLL));
		udelay(80);
	}

	frame_val  = ((PHY_ADDR << MI_COM_PHY_ADDR_SHIFT) &
		      MI_COM_PHY_ADDR_MASK);
	frame_val |= ((reg << MI_COM_REG_ADDR_SHIFT) &
		      MI_COM_REG_ADDR_MASK);
	frame_val |= (val & MI_COM_DATA_MASK);
	frame_val |= (MI_COM_CMD_WRITE | MI_COM_START);
	
	tw32_f(MAC_MI_COM, frame_val);

	loops = PHY_BUSY_LOOPS;
	while (loops != 0) {
		udelay(10);
		frame_val = tr32(MAC_MI_COM);
		if ((frame_val & MI_COM_BUSY) == 0) {
			udelay(5);
			frame_val = tr32(MAC_MI_COM);
			break;
		}
		loops -= 1;
	}

	ret = -EBUSY;
	if (loops != 0)
		ret = 0;

	if ((tp->mi_mode & MAC_MI_MODE_AUTO_POLL) != 0) {
		tw32_f(MAC_MI_MODE, tp->mi_mode);
		udelay(80);
	}

	return ret;
}

static void tg3_phy_set_wirespeed(struct tg3 *tp)
{
	u32 val;

	if (tp->tg3_flags2 & TG3_FLG2_NO_ETH_WIRE_SPEED)
		return;

	if (!tg3_writephy(tp, MII_TG3_AUX_CTRL, 0x7007) &&
	    !tg3_readphy(tp, MII_TG3_AUX_CTRL, &val))
		tg3_writephy(tp, MII_TG3_AUX_CTRL,
			     (val | (1 << 15) | (1 << 4)));
}

static int tg3_bmcr_reset(struct tg3 *tp)
{
	u32 phy_control;
	int limit, err;

	/* OK, reset it, and poll the BMCR_RESET bit until it
	 * clears or we time out.
	 */
	phy_control = BMCR_RESET;
	err = tg3_writephy(tp, MII_BMCR, phy_control);
	if (err != 0)
		return -EBUSY;

	limit = 5000;
	while (limit--) {
		err = tg3_readphy(tp, MII_BMCR, &phy_control);
		if (err != 0)
			return -EBUSY;

		if ((phy_control & BMCR_RESET) == 0) {
			udelay(40);
			break;
		}
		udelay(10);
	}
	if (limit <= 0)
		return -EBUSY;

	return 0;
}

static int tg3_wait_macro_done(struct tg3 *tp)
{
	int limit = 100;

	while (limit--) {
		u32 tmp32;

		if (!tg3_readphy(tp, 0x16, &tmp32)) {
			if ((tmp32 & 0x1000) == 0)
				break;
		}
	}
	if (limit <= 0)
		return -EBUSY;

	return 0;
}

static int tg3_phy_write_and_check_testpat(struct tg3 *tp, int *resetp)
{
	static const u32 test_pat[4][6] = {
	{ 0x00005555, 0x00000005, 0x00002aaa, 0x0000000a, 0x00003456, 0x00000003 },
	{ 0x00002aaa, 0x0000000a, 0x00003333, 0x00000003, 0x0000789a, 0x00000005 },
	{ 0x00005a5a, 0x00000005, 0x00002a6a, 0x0000000a, 0x00001bcd, 0x00000003 },
	{ 0x00002a5a, 0x0000000a, 0x000033c3, 0x00000003, 0x00002ef1, 0x00000005 }
	};
	int chan;

	for (chan = 0; chan < 4; chan++) {
		int i;

		tg3_writephy(tp, MII_TG3_DSP_ADDRESS,
			     (chan * 0x2000) | 0x0200);
		tg3_writephy(tp, 0x16, 0x0002);

		for (i = 0; i < 6; i++)
			tg3_writephy(tp, MII_TG3_DSP_RW_PORT,
				     test_pat[chan][i]);

		tg3_writephy(tp, 0x16, 0x0202);
		if (tg3_wait_macro_done(tp)) {
			*resetp = 1;
			return -EBUSY;
		}

		tg3_writephy(tp, MII_TG3_DSP_ADDRESS,
			     (chan * 0x2000) | 0x0200);
		tg3_writephy(tp, 0x16, 0x0082);
		if (tg3_wait_macro_done(tp)) {
			*resetp = 1;
			return -EBUSY;
		}

		tg3_writephy(tp, 0x16, 0x0802);
		if (tg3_wait_macro_done(tp)) {
			*resetp = 1;
			return -EBUSY;
		}

		for (i = 0; i < 6; i += 2) {
			u32 low, high;

			if (tg3_readphy(tp, MII_TG3_DSP_RW_PORT, &low) ||
			    tg3_readphy(tp, MII_TG3_DSP_RW_PORT, &high) ||
			    tg3_wait_macro_done(tp)) {
				*resetp = 1;
				return -EBUSY;
			}
			low &= 0x7fff;
			high &= 0x000f;
			if (low != test_pat[chan][i] ||
			    high != test_pat[chan][i+1]) {
				tg3_writephy(tp, MII_TG3_DSP_ADDRESS, 0x000b);
				tg3_writephy(tp, MII_TG3_DSP_RW_PORT, 0x4001);
				tg3_writephy(tp, MII_TG3_DSP_RW_PORT, 0x4005);

				return -EBUSY;
			}
		}
	}

	return 0;
}

static int tg3_phy_reset_chanpat(struct tg3 *tp)
{
	int chan;

	for (chan = 0; chan < 4; chan++) {
		int i;

		tg3_writephy(tp, MII_TG3_DSP_ADDRESS,
			     (chan * 0x2000) | 0x0200);
		tg3_writephy(tp, 0x16, 0x0002);
		for (i = 0; i < 6; i++)
			tg3_writephy(tp, MII_TG3_DSP_RW_PORT, 0x000);
		tg3_writephy(tp, 0x16, 0x0202);
		if (tg3_wait_macro_done(tp))
			return -EBUSY;
	}

	return 0;
}

static int tg3_phy_reset_5703_4_5(struct tg3 *tp)
{
	u32 reg32, phy9_orig;
	int retries, do_phy_reset, err;

	retries = 10;
	do_phy_reset = 1;
	do {
		if (do_phy_reset) {
			err = tg3_bmcr_reset(tp);
			if (err)
				return err;
			do_phy_reset = 0;
		}

		/* Disable transmitter and interrupt.  */
		if (tg3_readphy(tp, MII_TG3_EXT_CTRL, &reg32))
			continue;

		reg32 |= 0x3000;
		tg3_writephy(tp, MII_TG3_EXT_CTRL, reg32);

		/* Set full-duplex, 1000 mbps.  */
		tg3_writephy(tp, MII_BMCR,
			     BMCR_FULLDPLX | TG3_BMCR_SPEED1000);

		/* Set to master mode.  */
		if (tg3_readphy(tp, MII_TG3_CTRL, &phy9_orig))
			continue;

		tg3_writephy(tp, MII_TG3_CTRL,
			     (MII_TG3_CTRL_AS_MASTER |
			      MII_TG3_CTRL_ENABLE_AS_MASTER));

		/* Enable SM_DSP_CLOCK and 6dB.  */
		tg3_writephy(tp, MII_TG3_AUX_CTRL, 0x0c00);

		/* Block the PHY control access.  */
		tg3_writephy(tp, MII_TG3_DSP_ADDRESS, 0x8005);
		tg3_writephy(tp, MII_TG3_DSP_RW_PORT, 0x0800);

		err = tg3_phy_write_and_check_testpat(tp, &do_phy_reset);
		if (!err)
			break;
	} while (--retries);

	err = tg3_phy_reset_chanpat(tp);
	if (err)
		return err;

	tg3_writephy(tp, MII_TG3_DSP_ADDRESS, 0x8005);
	tg3_writephy(tp, MII_TG3_DSP_RW_PORT, 0x0000);

	tg3_writephy(tp, MII_TG3_DSP_ADDRESS, 0x8200);
	tg3_writephy(tp, 0x16, 0x0000);

	if (GET_ASIC_REV(tp->pci_chip_rev_id) == ASIC_REV_5703 ||
	    GET_ASIC_REV(tp->pci_chip_rev_id) == ASIC_REV_5704) {
		/* Set Extended packet length bit for jumbo frames */
		tg3_writephy(tp, MII_TG3_AUX_CTRL, 0x4400);
	}
	else {
		tg3_writephy(tp, MII_TG3_AUX_CTRL, 0x0400);
	}

	tg3_writephy(tp, MII_TG3_CTRL, phy9_orig);

	if (!tg3_readphy(tp, MII_TG3_EXT_CTRL, &reg32)) {
		reg32 &= ~0x3000;
		tg3_writephy(tp, MII_TG3_EXT_CTRL, reg32);
	} else if (!err)
		err = -EBUSY;

	return err;
}

static void tg3_link_report(struct tg3 *);

/* This will reset the tigon3 PHY if there is no valid
 * link unless the FORCE argument is non-zero.
 */
static int tg3_phy_reset(struct tg3 *tp)
{
	u32 phy_status;
	int err;

	err  = tg3_readphy(tp, MII_BMSR, &phy_status);
	err |= tg3_readphy(tp, MII_BMSR, &phy_status);
	if (err != 0)
		return -EBUSY;

	if (netif_running(tp->dev) && netif_carrier_ok(tp->dev)) {
		netif_carrier_off(tp->dev);
		tg3_link_report(tp);
	}

	if (GET_ASIC_REV(tp->pci_chip_rev_id) == ASIC_REV_5703 ||
	    GET_ASIC_REV(tp->pci_chip_rev_id) == ASIC_REV_5704 ||
	    GET_ASIC_REV(tp->pci_chip_rev_id) == ASIC_REV_5705) {
		err = tg3_phy_reset_5703_4_5(tp);
		if (err)
			return err;
		goto out;
	}

	err = tg3_bmcr_reset(tp);
	if (err)
		return err;

out:
	if (tp->tg3_flags2 & TG3_FLG2_PHY_ADC_BUG) {
		tg3_writephy(tp, MII_TG3_AUX_CTRL, 0x0c00);
		tg3_writephy(tp, MII_TG3_DSP_ADDRESS, 0x201f);
		tg3_writephy(tp, MII_TG3_DSP_RW_PORT, 0x2aaa);
		tg3_writephy(tp, MII_TG3_DSP_ADDRESS, 0x000a);
		tg3_writephy(tp, MII_TG3_DSP_RW_PORT, 0x0323);
		tg3_writephy(tp, MII_TG3_AUX_CTRL, 0x0400);
	}
	if (tp->tg3_flags2 & TG3_FLG2_PHY_5704_A0_BUG) {
		tg3_writephy(tp, 0x1c, 0x8d68);
		tg3_writephy(tp, 0x1c, 0x8d68);
	}
	if (tp->tg3_flags2 & TG3_FLG2_PHY_BER_BUG) {
		tg3_writephy(tp, MII_TG3_AUX_CTRL, 0x0c00);
		tg3_writephy(tp, MII_TG3_DSP_ADDRESS, 0x000a);
		tg3_writephy(tp, MII_TG3_DSP_RW_PORT, 0x310b);
		tg3_writephy(tp, MII_TG3_DSP_ADDRESS, 0x201f);
		tg3_writephy(tp, MII_TG3_DSP_RW_PORT, 0x9506);
		tg3_writephy(tp, MII_TG3_DSP_ADDRESS, 0x401f);
		tg3_writephy(tp, MII_TG3_DSP_RW_PORT, 0x14e2);
		tg3_writephy(tp, MII_TG3_AUX_CTRL, 0x0400);
	}
	else if (tp->tg3_flags2 & TG3_FLG2_PHY_JITTER_BUG) {
		tg3_writephy(tp, MII_TG3_AUX_CTRL, 0x0c00);
		tg3_writephy(tp, MII_TG3_DSP_ADDRESS, 0x000a);
		tg3_writephy(tp, MII_TG3_DSP_RW_PORT, 0x010b);
		tg3_writephy(tp, MII_TG3_AUX_CTRL, 0x0400);
	}
	/* Set Extended packet length bit (bit 14) on all chips that */
	/* support jumbo frames */
	if ((tp->phy_id & PHY_ID_MASK) == PHY_ID_BCM5401) {
		/* Cannot do read-modify-write on 5401 */
		tg3_writephy(tp, MII_TG3_AUX_CTRL, 0x4c20);
	} else if (tp->tg3_flags2 & TG3_FLG2_JUMBO_CAPABLE) {
		u32 phy_reg;

		/* Set bit 14 with read-modify-write to preserve other bits */
		if (!tg3_writephy(tp, MII_TG3_AUX_CTRL, 0x0007) &&
		    !tg3_readphy(tp, MII_TG3_AUX_CTRL, &phy_reg))
			tg3_writephy(tp, MII_TG3_AUX_CTRL, phy_reg | 0x4000);
	}

	/* Set phy register 0x10 bit 0 to high fifo elasticity to support
	 * jumbo frames transmission.
	 */
	if (tp->tg3_flags2 & TG3_FLG2_JUMBO_CAPABLE) {
		u32 phy_reg;

		if (!tg3_readphy(tp, MII_TG3_EXT_CTRL, &phy_reg))
		    tg3_writephy(tp, MII_TG3_EXT_CTRL,
				 phy_reg | MII_TG3_EXT_CTRL_FIFO_ELASTIC);
	}

	tg3_phy_set_wirespeed(tp);
	return 0;
}

static void tg3_frob_aux_power(struct tg3 *tp)
{
	struct tg3 *tp_peer = tp;

	if ((tp->tg3_flags & TG3_FLAG_EEPROM_WRITE_PROT) != 0)
		return;

/* XXXKV: We'll have to find a way to do this on Syllable */
#if 0
	if ((GET_ASIC_REV(tp->pci_chip_rev_id) == ASIC_REV_5704) ||
	    (GET_ASIC_REV(tp->pci_chip_rev_id) == ASIC_REV_5714)) {
		struct net_device *dev_peer;

		dev_peer = pci_get_drvdata(tp->pdev_peer);
		/* remove_one() may have been run on the peer. */
		if (!dev_peer)
			tp_peer = tp;
		else
			tp_peer = netdev_priv(dev_peer);
	}
#endif

	if ((tp->tg3_flags & TG3_FLAG_WOL_ENABLE) != 0 ||
	    (tp->tg3_flags & TG3_FLAG_ENABLE_ASF) != 0 ||
	    (tp_peer->tg3_flags & TG3_FLAG_WOL_ENABLE) != 0 ||
	    (tp_peer->tg3_flags & TG3_FLAG_ENABLE_ASF) != 0) {
		if (GET_ASIC_REV(tp->pci_chip_rev_id) == ASIC_REV_5700 ||
		    GET_ASIC_REV(tp->pci_chip_rev_id) == ASIC_REV_5701) {
			tw32_wait_f(GRC_LOCAL_CTRL, tp->grc_local_ctrl |
				    (GRC_LCLCTRL_GPIO_OE0 |
				     GRC_LCLCTRL_GPIO_OE1 |
				     GRC_LCLCTRL_GPIO_OE2 |
				     GRC_LCLCTRL_GPIO_OUTPUT0 |
				     GRC_LCLCTRL_GPIO_OUTPUT1),
				    100);
		} else {
			u32 no_gpio2;
			u32 grc_local_ctrl = 0;

			if (tp_peer != tp &&
			    (tp_peer->tg3_flags & TG3_FLAG_INIT_COMPLETE) != 0)
				return;

			/* Workaround to prevent overdrawing Amps. */
			if (GET_ASIC_REV(tp->pci_chip_rev_id) ==
			    ASIC_REV_5714) {
				grc_local_ctrl |= GRC_LCLCTRL_GPIO_OE3;
				tw32_wait_f(GRC_LOCAL_CTRL, tp->grc_local_ctrl |
					    grc_local_ctrl, 100);
			}

			/* On 5753 and variants, GPIO2 cannot be used. */
			no_gpio2 = tp->nic_sram_data_cfg &
				    NIC_SRAM_DATA_CFG_NO_GPIO2;

			grc_local_ctrl |= GRC_LCLCTRL_GPIO_OE0 |
					 GRC_LCLCTRL_GPIO_OE1 |
					 GRC_LCLCTRL_GPIO_OE2 |
					 GRC_LCLCTRL_GPIO_OUTPUT1 |
					 GRC_LCLCTRL_GPIO_OUTPUT2;
			if (no_gpio2) {
				grc_local_ctrl &= ~(GRC_LCLCTRL_GPIO_OE2 |
						    GRC_LCLCTRL_GPIO_OUTPUT2);
			}
			tw32_wait_f(GRC_LOCAL_CTRL, tp->grc_local_ctrl |
						    grc_local_ctrl, 100);

			grc_local_ctrl |= GRC_LCLCTRL_GPIO_OUTPUT0;

			tw32_wait_f(GRC_LOCAL_CTRL, tp->grc_local_ctrl |
						    grc_local_ctrl, 100);

			if (!no_gpio2) {
				grc_local_ctrl &= ~GRC_LCLCTRL_GPIO_OUTPUT2;
				tw32_wait_f(GRC_LOCAL_CTRL, tp->grc_local_ctrl |
					    grc_local_ctrl, 100);
			}
		}
	} else {
		if (GET_ASIC_REV(tp->pci_chip_rev_id) != ASIC_REV_5700 &&
		    GET_ASIC_REV(tp->pci_chip_rev_id) != ASIC_REV_5701) {
			if (tp_peer != tp &&
			    (tp_peer->tg3_flags & TG3_FLAG_INIT_COMPLETE) != 0)
				return;

			tw32_wait_f(GRC_LOCAL_CTRL, tp->grc_local_ctrl |
				    (GRC_LCLCTRL_GPIO_OE1 |
				     GRC_LCLCTRL_GPIO_OUTPUT1), 100);

			tw32_wait_f(GRC_LOCAL_CTRL, tp->grc_local_ctrl |
				    GRC_LCLCTRL_GPIO_OE1, 100);

			tw32_wait_f(GRC_LOCAL_CTRL, tp->grc_local_ctrl |
				    (GRC_LCLCTRL_GPIO_OE1 |
				     GRC_LCLCTRL_GPIO_OUTPUT1), 100);
		}
	}
}

static int tg3_setup_phy(struct tg3 *, int);

#define RESET_KIND_SHUTDOWN	0
#define RESET_KIND_INIT		1
#define RESET_KIND_SUSPEND	2

static void tg3_write_sig_post_reset(struct tg3 *, int);
static int tg3_halt_cpu(struct tg3 *, u32);
static int tg3_nvram_lock(struct tg3 *);
static void tg3_nvram_unlock(struct tg3 *);

static void tg3_power_down_phy(struct tg3 *tp)
{
	/* The PHY should not be powered down on some chips because
	 * of bugs.
	 */
	if (GET_ASIC_REV(tp->pci_chip_rev_id) == ASIC_REV_5700 ||
	    GET_ASIC_REV(tp->pci_chip_rev_id) == ASIC_REV_5704 ||
	    (GET_ASIC_REV(tp->pci_chip_rev_id) == ASIC_REV_5780 &&
	     (tp->tg3_flags2 & TG3_FLG2_MII_SERDES)))
		return;
	tg3_writephy(tp, MII_BMCR, BMCR_PDOWN);
}

static int tg3_set_power_state(struct tg3 *tp, int state)
{
	u32 misc_host_ctrl;
	u16 power_control, power_caps;
	int pm = tp->pm_cap;

	/* Make sure register accesses (indirect or otherwise)
	 * will function correctly.
	 */
	g_psBus->write_pci_config(tp->pdev->nBus, tp->pdev->nDevice, tp->pdev->nFunction,
					TG3PCI_MISC_HOST_CTRL,
					4, tp->misc_host_ctrl);

	power_control = g_psBus->read_pci_config(tp->pdev->nBus, tp->pdev->nDevice, tp->pdev->nFunction,
				pm + PCI_PM_CTRL, 2);
	power_control |= PCI_PM_CTRL_PME_STATUS;
	power_control &= ~(PCI_PM_CTRL_STATE_MASK);
	switch (state) {
	case PCI_D0:
		power_control |= 0;
		g_psBus->write_pci_config(tp->pdev->nBus, tp->pdev->nDevice, tp->pdev->nFunction,
					pm + PCI_PM_CTRL,
					2, power_control);
		udelay(100);	/* Delay after power state change */

		/* Switch out of Vaux if it is not a LOM */
		if (!(tp->tg3_flags & TG3_FLAG_EEPROM_WRITE_PROT))
			tw32_wait_f(GRC_LOCAL_CTRL, tp->grc_local_ctrl, 100);

		return 0;

	case PCI_D1:
		power_control |= 1;
		break;

	case PCI_D2:
		power_control |= 2;
		break;

	case PCI_D3hot:
		power_control |= 3;
		break;

	default:
		kerndbg( KERN_WARNING, "%s: Invalid power state (%d) requested.\n",
		       tp->dev->name, state);
		return -EINVAL;
	};

	power_control |= PCI_PM_CTRL_PME_ENABLE;

	misc_host_ctrl = tr32(TG3PCI_MISC_HOST_CTRL);
	tw32(TG3PCI_MISC_HOST_CTRL,
	     misc_host_ctrl | MISC_HOST_CTRL_MASK_PCI_INT);

	if (tp->link_config.phy_is_low_power == 0) {
		tp->link_config.phy_is_low_power = 1;
		tp->link_config.orig_speed = tp->link_config.speed;
		tp->link_config.orig_duplex = tp->link_config.duplex;
		tp->link_config.orig_autoneg = tp->link_config.autoneg;
	}

	if (!(tp->tg3_flags2 & TG3_FLG2_ANY_SERDES)) {
		tp->link_config.speed = SPEED_10;
		tp->link_config.duplex = DUPLEX_HALF;
		tp->link_config.autoneg = AUTONEG_ENABLE;
		tg3_setup_phy(tp, 0);
	}

	if (!(tp->tg3_flags & TG3_FLAG_ENABLE_ASF)) {
		int i;
		u32 val;

		for (i = 0; i < 200; i++) {
			tg3_read_mem(tp, NIC_SRAM_FW_ASF_STATUS_MBOX, &val);
			if (val == ~NIC_SRAM_FIRMWARE_MBOX_MAGIC1)
				break;
			udelay(100);
		}
	}
	tg3_write_mem(tp, NIC_SRAM_WOL_MBOX, WOL_SIGNATURE |
					     WOL_DRV_STATE_SHUTDOWN |
					     WOL_DRV_WOL | WOL_SET_MAGIC_PKT);

	power_caps = g_psBus->read_pci_config(tp->pdev->nBus, tp->pdev->nDevice, tp->pdev->nFunction, pm + PCI_PM_PMC, 2);

	if (tp->tg3_flags & TG3_FLAG_WOL_ENABLE) {
		u32 mac_mode;

		if (!(tp->tg3_flags2 & TG3_FLG2_PHY_SERDES)) {
			tg3_writephy(tp, MII_TG3_AUX_CTRL, 0x5a);
			udelay(40);

			mac_mode = MAC_MODE_PORT_MODE_MII;

			if (GET_ASIC_REV(tp->pci_chip_rev_id) != ASIC_REV_5700 ||
			    !(tp->tg3_flags & TG3_FLAG_WOL_SPEED_100MB))
				mac_mode |= MAC_MODE_LINK_POLARITY;
		} else {
			mac_mode = MAC_MODE_PORT_MODE_TBI;
		}

		if (!(tp->tg3_flags2 & TG3_FLG2_5750_PLUS))
			tw32(MAC_LED_CTRL, tp->led_ctrl);

		if (((power_caps & PCI_PM_CAP_PME_D3cold) &&
		     (tp->tg3_flags & TG3_FLAG_WOL_ENABLE)))
			mac_mode |= MAC_MODE_MAGIC_PKT_ENABLE;

		tw32_f(MAC_MODE, mac_mode);
		udelay(100);

		tw32_f(MAC_RX_MODE, RX_MODE_ENABLE);
		udelay(10);
	}

	if (!(tp->tg3_flags & TG3_FLAG_WOL_SPEED_100MB) &&
	    (GET_ASIC_REV(tp->pci_chip_rev_id) == ASIC_REV_5700 ||
	     GET_ASIC_REV(tp->pci_chip_rev_id) == ASIC_REV_5701)) {
		u32 base_val;

		base_val = tp->pci_clock_ctrl;
		base_val |= (CLOCK_CTRL_RXCLK_DISABLE |
			     CLOCK_CTRL_TXCLK_DISABLE);

		tw32_wait_f(TG3PCI_CLOCK_CTRL, base_val | CLOCK_CTRL_ALTCLK |
			    CLOCK_CTRL_PWRDOWN_PLL133, 40);
	} else if (tp->tg3_flags2 & TG3_FLG2_5780_CLASS) {
		/* do nothing */
	} else if (!((tp->tg3_flags2 & TG3_FLG2_5750_PLUS) &&
		     (tp->tg3_flags & TG3_FLAG_ENABLE_ASF))) {
		u32 newbits1, newbits2;

		if (GET_ASIC_REV(tp->pci_chip_rev_id) == ASIC_REV_5700 ||
		    GET_ASIC_REV(tp->pci_chip_rev_id) == ASIC_REV_5701) {
			newbits1 = (CLOCK_CTRL_RXCLK_DISABLE |
				    CLOCK_CTRL_TXCLK_DISABLE |
				    CLOCK_CTRL_ALTCLK);
			newbits2 = newbits1 | CLOCK_CTRL_44MHZ_CORE;
		} else if (tp->tg3_flags2 & TG3_FLG2_5705_PLUS) {
			newbits1 = CLOCK_CTRL_625_CORE;
			newbits2 = newbits1 | CLOCK_CTRL_ALTCLK;
		} else {
			newbits1 = CLOCK_CTRL_ALTCLK;
			newbits2 = newbits1 | CLOCK_CTRL_44MHZ_CORE;
		}

		tw32_wait_f(TG3PCI_CLOCK_CTRL, tp->pci_clock_ctrl | newbits1,
			    40);

		tw32_wait_f(TG3PCI_CLOCK_CTRL, tp->pci_clock_ctrl | newbits2,
			    40);

		if (!(tp->tg3_flags2 & TG3_FLG2_5705_PLUS)) {
			u32 newbits3;

			if (GET_ASIC_REV(tp->pci_chip_rev_id) == ASIC_REV_5700 ||
			    GET_ASIC_REV(tp->pci_chip_rev_id) == ASIC_REV_5701) {
				newbits3 = (CLOCK_CTRL_RXCLK_DISABLE |
					    CLOCK_CTRL_TXCLK_DISABLE |
					    CLOCK_CTRL_44MHZ_CORE);
			} else {
				newbits3 = CLOCK_CTRL_44MHZ_CORE;
			}

			tw32_wait_f(TG3PCI_CLOCK_CTRL,
				    tp->pci_clock_ctrl | newbits3, 40);
		}
	}

	if (!(tp->tg3_flags & TG3_FLAG_WOL_ENABLE) &&
	    !(tp->tg3_flags & TG3_FLAG_ENABLE_ASF)) {
		/* Turn off the PHY */
		if (!(tp->tg3_flags2 & TG3_FLG2_PHY_SERDES)) {
			tg3_writephy(tp, MII_TG3_EXT_CTRL,
				     MII_TG3_EXT_CTRL_FORCE_LED_OFF);
			tg3_writephy(tp, MII_TG3_AUX_CTRL, 0x01b2);
			tg3_power_down_phy(tp);
		}
	}

	tg3_frob_aux_power(tp);

	/* Workaround for unstable PLL clock */
	if ((GET_CHIP_REV(tp->pci_chip_rev_id) == CHIPREV_5750_AX) ||
	    (GET_CHIP_REV(tp->pci_chip_rev_id) == CHIPREV_5750_BX)) {
		u32 val = tr32(0x7d00);

		val &= ~((1 << 16) | (1 << 4) | (1 << 2) | (1 << 1) | 1);
		tw32(0x7d00, val);
		if (!(tp->tg3_flags & TG3_FLAG_ENABLE_ASF)) {
			int err;

			err = tg3_nvram_lock(tp);
			tg3_halt_cpu(tp, RX_CPU_BASE);
			if (!err)
				tg3_nvram_unlock(tp);
		}
	}

	tg3_write_sig_post_reset(tp, RESET_KIND_SHUTDOWN);

	/* Finally, set the new power state. */
	g_psBus->write_pci_config(tp->pdev->nBus, tp->pdev->nDevice, tp->pdev->nFunction, pm + PCI_PM_CTRL, 2, power_control);
	udelay(100);	/* Delay after power state change */

	return 0;
}

static void tg3_link_report(struct tg3 *tp)
{
	if (!netif_carrier_ok(tp->dev)) {
		kerndbg( KERN_INFO, "%s: Link is down.\n", tp->dev->name);
	} else {
		kerndbg( KERN_INFO, "%s: Link is up at %d Mbps, %s duplex.\n",
		       tp->dev->name,
		       (tp->link_config.active_speed == SPEED_1000 ?
			1000 :
			(tp->link_config.active_speed == SPEED_100 ?
			 100 : 10)),
		       (tp->link_config.active_duplex == DUPLEX_FULL ?
			"full" : "half"));

		kerndbg( KERN_INFO, "%s: Flow control is %s for TX and "
		       "%s for RX.\n",
		       tp->dev->name,
		       (tp->tg3_flags & TG3_FLAG_TX_PAUSE) ? "on" : "off",
		       (tp->tg3_flags & TG3_FLAG_RX_PAUSE) ? "on" : "off");
	}
}

static void tg3_setup_flow_control(struct tg3 *tp, u32 local_adv, u32 remote_adv)
{
	u32 new_tg3_flags = 0;
	u32 old_rx_mode = tp->rx_mode;
	u32 old_tx_mode = tp->tx_mode;

	if (tp->tg3_flags & TG3_FLAG_PAUSE_AUTONEG) {

		/* Convert 1000BaseX flow control bits to 1000BaseT
		 * bits before resolving flow control.
		 */
		if (tp->tg3_flags2 & TG3_FLG2_MII_SERDES) {
			local_adv &= ~(ADVERTISE_PAUSE_CAP |
				       ADVERTISE_PAUSE_ASYM);
			remote_adv &= ~(LPA_PAUSE_CAP | LPA_PAUSE_ASYM);

			if (local_adv & ADVERTISE_1000XPAUSE)
				local_adv |= ADVERTISE_PAUSE_CAP;
			if (local_adv & ADVERTISE_1000XPSE_ASYM)
				local_adv |= ADVERTISE_PAUSE_ASYM;
			if (remote_adv & LPA_1000XPAUSE)
				remote_adv |= LPA_PAUSE_CAP;
			if (remote_adv & LPA_1000XPAUSE_ASYM)
				remote_adv |= LPA_PAUSE_ASYM;
		}

		if (local_adv & ADVERTISE_PAUSE_CAP) {
			if (local_adv & ADVERTISE_PAUSE_ASYM) {
				if (remote_adv & LPA_PAUSE_CAP)
					new_tg3_flags |=
						(TG3_FLAG_RX_PAUSE |
					 	TG3_FLAG_TX_PAUSE);
				else if (remote_adv & LPA_PAUSE_ASYM)
					new_tg3_flags |=
						(TG3_FLAG_RX_PAUSE);
			} else {
				if (remote_adv & LPA_PAUSE_CAP)
					new_tg3_flags |=
						(TG3_FLAG_RX_PAUSE |
					 	TG3_FLAG_TX_PAUSE);
			}
		} else if (local_adv & ADVERTISE_PAUSE_ASYM) {
			if ((remote_adv & LPA_PAUSE_CAP) &&
		    	(remote_adv & LPA_PAUSE_ASYM))
				new_tg3_flags |= TG3_FLAG_TX_PAUSE;
		}

		tp->tg3_flags &= ~(TG3_FLAG_RX_PAUSE | TG3_FLAG_TX_PAUSE);
		tp->tg3_flags |= new_tg3_flags;
	} else {
		new_tg3_flags = tp->tg3_flags;
	}

	if (new_tg3_flags & TG3_FLAG_RX_PAUSE)
		tp->rx_mode |= RX_MODE_FLOW_CTRL_ENABLE;
	else
		tp->rx_mode &= ~RX_MODE_FLOW_CTRL_ENABLE;

	if (old_rx_mode != tp->rx_mode) {
		tw32_f(MAC_RX_MODE, tp->rx_mode);
	}
	
	if (new_tg3_flags & TG3_FLAG_TX_PAUSE)
		tp->tx_mode |= TX_MODE_FLOW_CTRL_ENABLE;
	else
		tp->tx_mode &= ~TX_MODE_FLOW_CTRL_ENABLE;

	if (old_tx_mode != tp->tx_mode) {
		tw32_f(MAC_TX_MODE, tp->tx_mode);
	}
}

static void tg3_aux_stat_to_speed_duplex(struct tg3 *tp, u32 val, u16 *speed, u8 *duplex)
{
	switch (val & MII_TG3_AUX_STAT_SPDMASK) {
	case MII_TG3_AUX_STAT_10HALF:
		*speed = SPEED_10;
		*duplex = DUPLEX_HALF;
		break;

	case MII_TG3_AUX_STAT_10FULL:
		*speed = SPEED_10;
		*duplex = DUPLEX_FULL;
		break;

	case MII_TG3_AUX_STAT_100HALF:
		*speed = SPEED_100;
		*duplex = DUPLEX_HALF;
		break;

	case MII_TG3_AUX_STAT_100FULL:
		*speed = SPEED_100;
		*duplex = DUPLEX_FULL;
		break;

	case MII_TG3_AUX_STAT_1000HALF:
		*speed = SPEED_1000;
		*duplex = DUPLEX_HALF;
		break;

	case MII_TG3_AUX_STAT_1000FULL:
		*speed = SPEED_1000;
		*duplex = DUPLEX_FULL;
		break;

	default:
		*speed = SPEED_INVALID;
		*duplex = DUPLEX_INVALID;
		break;
	};
}

static void tg3_phy_copper_begin(struct tg3 *tp)
{
	u32 new_adv;
	int i;

	if (tp->link_config.phy_is_low_power) {
		/* Entering low power mode.  Disable gigabit and
		 * 100baseT advertisements.
		 */
		tg3_writephy(tp, MII_TG3_CTRL, 0);

		new_adv = (ADVERTISE_10HALF | ADVERTISE_10FULL |
			   ADVERTISE_CSMA | ADVERTISE_PAUSE_CAP);
		if (tp->tg3_flags & TG3_FLAG_WOL_SPEED_100MB)
			new_adv |= (ADVERTISE_100HALF | ADVERTISE_100FULL);

		tg3_writephy(tp, MII_ADVERTISE, new_adv);
	} else if (tp->link_config.speed == SPEED_INVALID) {
		tp->link_config.advertising =
			(ADVERTISED_10baseT_Half | ADVERTISED_10baseT_Full |
			 ADVERTISED_100baseT_Half | ADVERTISED_100baseT_Full |
			 ADVERTISED_1000baseT_Half | ADVERTISED_1000baseT_Full |
			 ADVERTISED_Autoneg | ADVERTISED_MII);

		if (tp->tg3_flags & TG3_FLAG_10_100_ONLY)
			tp->link_config.advertising &=
				~(ADVERTISED_1000baseT_Half |
				  ADVERTISED_1000baseT_Full);

		new_adv = (ADVERTISE_CSMA | ADVERTISE_PAUSE_CAP);
		if (tp->link_config.advertising & ADVERTISED_10baseT_Half)
			new_adv |= ADVERTISE_10HALF;
		if (tp->link_config.advertising & ADVERTISED_10baseT_Full)
			new_adv |= ADVERTISE_10FULL;
		if (tp->link_config.advertising & ADVERTISED_100baseT_Half)
			new_adv |= ADVERTISE_100HALF;
		if (tp->link_config.advertising & ADVERTISED_100baseT_Full)
			new_adv |= ADVERTISE_100FULL;
		tg3_writephy(tp, MII_ADVERTISE, new_adv);

		if (tp->link_config.advertising &
		    (ADVERTISED_1000baseT_Half | ADVERTISED_1000baseT_Full)) {
			new_adv = 0;
			if (tp->link_config.advertising & ADVERTISED_1000baseT_Half)
				new_adv |= MII_TG3_CTRL_ADV_1000_HALF;
			if (tp->link_config.advertising & ADVERTISED_1000baseT_Full)
				new_adv |= MII_TG3_CTRL_ADV_1000_FULL;
			if (!(tp->tg3_flags & TG3_FLAG_10_100_ONLY) &&
			    (tp->pci_chip_rev_id == CHIPREV_ID_5701_A0 ||
			     tp->pci_chip_rev_id == CHIPREV_ID_5701_B0))
				new_adv |= (MII_TG3_CTRL_AS_MASTER |
					    MII_TG3_CTRL_ENABLE_AS_MASTER);
			tg3_writephy(tp, MII_TG3_CTRL, new_adv);
		} else {
			tg3_writephy(tp, MII_TG3_CTRL, 0);
		}
	} else {
		/* Asking for a specific link mode. */
		if (tp->link_config.speed == SPEED_1000) {
			new_adv = ADVERTISE_CSMA | ADVERTISE_PAUSE_CAP;
			tg3_writephy(tp, MII_ADVERTISE, new_adv);

			if (tp->link_config.duplex == DUPLEX_FULL)
				new_adv = MII_TG3_CTRL_ADV_1000_FULL;
			else
				new_adv = MII_TG3_CTRL_ADV_1000_HALF;
			if (tp->pci_chip_rev_id == CHIPREV_ID_5701_A0 ||
			    tp->pci_chip_rev_id == CHIPREV_ID_5701_B0)
				new_adv |= (MII_TG3_CTRL_AS_MASTER |
					    MII_TG3_CTRL_ENABLE_AS_MASTER);
			tg3_writephy(tp, MII_TG3_CTRL, new_adv);
		} else {
			tg3_writephy(tp, MII_TG3_CTRL, 0);

			new_adv = ADVERTISE_CSMA | ADVERTISE_PAUSE_CAP;
			if (tp->link_config.speed == SPEED_100) {
				if (tp->link_config.duplex == DUPLEX_FULL)
					new_adv |= ADVERTISE_100FULL;
				else
					new_adv |= ADVERTISE_100HALF;
			} else {
				if (tp->link_config.duplex == DUPLEX_FULL)
					new_adv |= ADVERTISE_10FULL;
				else
					new_adv |= ADVERTISE_10HALF;
			}
			tg3_writephy(tp, MII_ADVERTISE, new_adv);
		}
	}

	if (tp->link_config.autoneg == AUTONEG_DISABLE &&
	    tp->link_config.speed != SPEED_INVALID) {
		u32 bmcr, orig_bmcr;

		tp->link_config.active_speed = tp->link_config.speed;
		tp->link_config.active_duplex = tp->link_config.duplex;

		bmcr = 0;
		switch (tp->link_config.speed) {
		default:
		case SPEED_10:
			break;

		case SPEED_100:
			bmcr |= BMCR_SPEED100;
			break;

		case SPEED_1000:
			bmcr |= TG3_BMCR_SPEED1000;
			break;
		};

		if (tp->link_config.duplex == DUPLEX_FULL)
			bmcr |= BMCR_FULLDPLX;

		if (!tg3_readphy(tp, MII_BMCR, &orig_bmcr) &&
		    (bmcr != orig_bmcr)) {
			tg3_writephy(tp, MII_BMCR, BMCR_LOOPBACK);
			for (i = 0; i < 1500; i++) {
				u32 tmp;

				udelay(10);
				if (tg3_readphy(tp, MII_BMSR, &tmp) ||
				    tg3_readphy(tp, MII_BMSR, &tmp))
					continue;
				if (!(tmp & BMSR_LSTATUS)) {
					udelay(40);
					break;
				}
			}
			tg3_writephy(tp, MII_BMCR, bmcr);
			udelay(40);
		}
	} else {
		tg3_writephy(tp, MII_BMCR,
			     BMCR_ANENABLE | BMCR_ANRESTART);
	}
}

static int tg3_init_5401phy_dsp(struct tg3 *tp)
{
	int err;

	/* Turn off tap power management. */
	/* Set Extended packet length bit */
	err  = tg3_writephy(tp, MII_TG3_AUX_CTRL, 0x4c20);

	err |= tg3_writephy(tp, MII_TG3_DSP_ADDRESS, 0x0012);
	err |= tg3_writephy(tp, MII_TG3_DSP_RW_PORT, 0x1804);

	err |= tg3_writephy(tp, MII_TG3_DSP_ADDRESS, 0x0013);
	err |= tg3_writephy(tp, MII_TG3_DSP_RW_PORT, 0x1204);

	err |= tg3_writephy(tp, MII_TG3_DSP_ADDRESS, 0x8006);
	err |= tg3_writephy(tp, MII_TG3_DSP_RW_PORT, 0x0132);

	err |= tg3_writephy(tp, MII_TG3_DSP_ADDRESS, 0x8006);
	err |= tg3_writephy(tp, MII_TG3_DSP_RW_PORT, 0x0232);

	err |= tg3_writephy(tp, MII_TG3_DSP_ADDRESS, 0x201f);
	err |= tg3_writephy(tp, MII_TG3_DSP_RW_PORT, 0x0a20);

	udelay(40);

	return err;
}

static int tg3_copper_is_advertising_all(struct tg3 *tp)
{
	u32 adv_reg, all_mask;

	if (tg3_readphy(tp, MII_ADVERTISE, &adv_reg))
		return 0;

	all_mask = (ADVERTISE_10HALF | ADVERTISE_10FULL |
		    ADVERTISE_100HALF | ADVERTISE_100FULL);
	if ((adv_reg & all_mask) != all_mask)
		return 0;
	if (!(tp->tg3_flags & TG3_FLAG_10_100_ONLY)) {
		u32 tg3_ctrl;

		if (tg3_readphy(tp, MII_TG3_CTRL, &tg3_ctrl))
			return 0;

		all_mask = (MII_TG3_CTRL_ADV_1000_HALF |
			    MII_TG3_CTRL_ADV_1000_FULL);
		if ((tg3_ctrl & all_mask) != all_mask)
			return 0;
	}
	return 1;
}

static int tg3_setup_copper_phy(struct tg3 *tp, int force_reset)
{
	int current_link_up;
	u32 bmsr, dummy;
	u16 current_speed;
	u8 current_duplex;
	int i, err;

	tw32(MAC_EVENT, 0);

	tw32_f(MAC_STATUS,
	     (MAC_STATUS_SYNC_CHANGED |
	      MAC_STATUS_CFG_CHANGED |
	      MAC_STATUS_MI_COMPLETION |
	      MAC_STATUS_LNKSTATE_CHANGED));
	udelay(40);

	tp->mi_mode = MAC_MI_MODE_BASE;
	tw32_f(MAC_MI_MODE, tp->mi_mode);
	udelay(80);

	tg3_writephy(tp, MII_TG3_AUX_CTRL, 0x02);

	/* Some third-party PHYs need to be reset on link going
	 * down.
	 */
	if ((GET_ASIC_REV(tp->pci_chip_rev_id) == ASIC_REV_5703 ||
	     GET_ASIC_REV(tp->pci_chip_rev_id) == ASIC_REV_5704 ||
	     GET_ASIC_REV(tp->pci_chip_rev_id) == ASIC_REV_5705) &&
	    netif_carrier_ok(tp->dev)) {
		tg3_readphy(tp, MII_BMSR, &bmsr);
		if (!tg3_readphy(tp, MII_BMSR, &bmsr) &&
		    !(bmsr & BMSR_LSTATUS))
			force_reset = 1;
	}
	if (force_reset)
		tg3_phy_reset(tp);

	if ((tp->phy_id & PHY_ID_MASK) == PHY_ID_BCM5401) {
		tg3_readphy(tp, MII_BMSR, &bmsr);
		if (tg3_readphy(tp, MII_BMSR, &bmsr) ||
		    !(tp->tg3_flags & TG3_FLAG_INIT_COMPLETE))
			bmsr = 0;

		if (!(bmsr & BMSR_LSTATUS)) {
			err = tg3_init_5401phy_dsp(tp);
			if (err)
				return err;

			tg3_readphy(tp, MII_BMSR, &bmsr);
			for (i = 0; i < 1000; i++) {
				udelay(10);
				if (!tg3_readphy(tp, MII_BMSR, &bmsr) &&
				    (bmsr & BMSR_LSTATUS)) {
					udelay(40);
					break;
				}
			}

			if ((tp->phy_id & PHY_ID_REV_MASK) == PHY_REV_BCM5401_B0 &&
			    !(bmsr & BMSR_LSTATUS) &&
			    tp->link_config.active_speed == SPEED_1000) {
				err = tg3_phy_reset(tp);
				if (!err)
					err = tg3_init_5401phy_dsp(tp);
				if (err)
					return err;
			}
		}
	} else if (tp->pci_chip_rev_id == CHIPREV_ID_5701_A0 ||
		   tp->pci_chip_rev_id == CHIPREV_ID_5701_B0) {
		/* 5701 {A0,B0} CRC bug workaround */
		tg3_writephy(tp, 0x15, 0x0a75);
		tg3_writephy(tp, 0x1c, 0x8c68);
		tg3_writephy(tp, 0x1c, 0x8d68);
		tg3_writephy(tp, 0x1c, 0x8c68);
	}

	/* Clear pending interrupts... */
	tg3_readphy(tp, MII_TG3_ISTAT, &dummy);
	tg3_readphy(tp, MII_TG3_ISTAT, &dummy);

	if (tp->tg3_flags & TG3_FLAG_USE_MI_INTERRUPT)
		tg3_writephy(tp, MII_TG3_IMASK, ~MII_TG3_INT_LINKCHG);
	else
		tg3_writephy(tp, MII_TG3_IMASK, ~0);

	if (GET_ASIC_REV(tp->pci_chip_rev_id) == ASIC_REV_5700 ||
	    GET_ASIC_REV(tp->pci_chip_rev_id) == ASIC_REV_5701) {
		if (tp->led_ctrl == LED_CTRL_MODE_PHY_1)
			tg3_writephy(tp, MII_TG3_EXT_CTRL,
				     MII_TG3_EXT_CTRL_LNK3_LED_MODE);
		else
			tg3_writephy(tp, MII_TG3_EXT_CTRL, 0);
	}

	current_link_up = 0;
	current_speed = SPEED_INVALID;
	current_duplex = DUPLEX_INVALID;

	if (tp->tg3_flags2 & TG3_FLG2_CAPACITIVE_COUPLING) {
		u32 val;

		tg3_writephy(tp, MII_TG3_AUX_CTRL, 0x4007);
		tg3_readphy(tp, MII_TG3_AUX_CTRL, &val);
		if (!(val & (1 << 10))) {
			val |= (1 << 10);
			tg3_writephy(tp, MII_TG3_AUX_CTRL, val);
			goto relink;
		}
	}

	bmsr = 0;
	for (i = 0; i < 100; i++) {
		tg3_readphy(tp, MII_BMSR, &bmsr);
		if (!tg3_readphy(tp, MII_BMSR, &bmsr) &&
		    (bmsr & BMSR_LSTATUS))
			break;
		udelay(40);
	}

	if (bmsr & BMSR_LSTATUS) {
		u32 aux_stat, bmcr;

		tg3_readphy(tp, MII_TG3_AUX_STAT, &aux_stat);
		for (i = 0; i < 2000; i++) {
			udelay(10);
			if (!tg3_readphy(tp, MII_TG3_AUX_STAT, &aux_stat) &&
			    aux_stat)
				break;
		}

		tg3_aux_stat_to_speed_duplex(tp, aux_stat,
					     &current_speed,
					     &current_duplex);

		bmcr = 0;
		for (i = 0; i < 200; i++) {
			tg3_readphy(tp, MII_BMCR, &bmcr);
			if (tg3_readphy(tp, MII_BMCR, &bmcr))
				continue;
			if (bmcr && bmcr != 0x7fff)
				break;
			udelay(10);
		}

		if (tp->link_config.autoneg == AUTONEG_ENABLE) {
			if (bmcr & BMCR_ANENABLE) {
				current_link_up = 1;

				/* Force autoneg restart if we are exiting
				 * low power mode.
				 */
				if (!tg3_copper_is_advertising_all(tp))
					current_link_up = 0;
			} else {
				current_link_up = 0;
			}
		} else {
			if (!(bmcr & BMCR_ANENABLE) &&
			    tp->link_config.speed == current_speed &&
			    tp->link_config.duplex == current_duplex) {
				current_link_up = 1;
			} else {
				current_link_up = 0;
			}
		}

		tp->link_config.active_speed = current_speed;
		tp->link_config.active_duplex = current_duplex;
	}

	if (current_link_up == 1 &&
	    (tp->link_config.active_duplex == DUPLEX_FULL) &&
	    (tp->link_config.autoneg == AUTONEG_ENABLE)) {
		u32 local_adv, remote_adv;

		if (tg3_readphy(tp, MII_ADVERTISE, &local_adv))
			local_adv = 0;
		local_adv &= (ADVERTISE_PAUSE_CAP | ADVERTISE_PAUSE_ASYM);

		if (tg3_readphy(tp, MII_LPA, &remote_adv))
			remote_adv = 0;

		remote_adv &= (LPA_PAUSE_CAP | LPA_PAUSE_ASYM);

		/* If we are not advertising full pause capability,
		 * something is wrong.  Bring the link down and reconfigure.
		 */
		if (local_adv != ADVERTISE_PAUSE_CAP) {
			current_link_up = 0;
		} else {
			tg3_setup_flow_control(tp, local_adv, remote_adv);
		}
	}
relink:
	if (current_link_up == 0 || tp->link_config.phy_is_low_power) {
		u32 tmp;

		tg3_phy_copper_begin(tp);

		tg3_readphy(tp, MII_BMSR, &tmp);
		if (!tg3_readphy(tp, MII_BMSR, &tmp) &&
		    (tmp & BMSR_LSTATUS))
			current_link_up = 1;
	}

	tp->mac_mode &= ~MAC_MODE_PORT_MODE_MASK;
	if (current_link_up == 1) {
		if (tp->link_config.active_speed == SPEED_100 ||
		    tp->link_config.active_speed == SPEED_10)
			tp->mac_mode |= MAC_MODE_PORT_MODE_MII;
		else
			tp->mac_mode |= MAC_MODE_PORT_MODE_GMII;
	} else
		tp->mac_mode |= MAC_MODE_PORT_MODE_GMII;

	tp->mac_mode &= ~MAC_MODE_HALF_DUPLEX;
	if (tp->link_config.active_duplex == DUPLEX_HALF)
		tp->mac_mode |= MAC_MODE_HALF_DUPLEX;

	tp->mac_mode &= ~MAC_MODE_LINK_POLARITY;
	if (GET_ASIC_REV(tp->pci_chip_rev_id) == ASIC_REV_5700) {
		if ((tp->led_ctrl == LED_CTRL_MODE_PHY_2) ||
		    (current_link_up == 1 &&
		     tp->link_config.active_speed == SPEED_10))
			tp->mac_mode |= MAC_MODE_LINK_POLARITY;
	} else {
		if (current_link_up == 1)
			tp->mac_mode |= MAC_MODE_LINK_POLARITY;
	}

	/* ??? Without this setting Netgear GA302T PHY does not
	 * ??? send/receive packets...
	 */
	if ((tp->phy_id & PHY_ID_MASK) == PHY_ID_BCM5411 &&
	    tp->pci_chip_rev_id == CHIPREV_ID_5700_ALTIMA) {
		tp->mi_mode |= MAC_MI_MODE_AUTO_POLL;
		tw32_f(MAC_MI_MODE, tp->mi_mode);
		udelay(80);
	}

	tw32_f(MAC_MODE, tp->mac_mode);
	udelay(40);

	if (tp->tg3_flags & TG3_FLAG_USE_LINKCHG_REG) {
		/* Polled via timer. */
		tw32_f(MAC_EVENT, 0);
	} else {
		tw32_f(MAC_EVENT, MAC_EVENT_LNKSTATE_CHANGED);
	}
	udelay(40);

	if (GET_ASIC_REV(tp->pci_chip_rev_id) == ASIC_REV_5700 &&
	    current_link_up == 1 &&
	    tp->link_config.active_speed == SPEED_1000 &&
	    ((tp->tg3_flags & TG3_FLAG_PCIX_MODE) ||
	     (tp->tg3_flags & TG3_FLAG_PCI_HIGH_SPEED))) {
		udelay(120);
		tw32_f(MAC_STATUS,
		     (MAC_STATUS_SYNC_CHANGED |
		      MAC_STATUS_CFG_CHANGED));
		udelay(40);
		tg3_write_mem(tp,
			      NIC_SRAM_FIRMWARE_MBOX,
			      NIC_SRAM_FIRMWARE_MBOX_MAGIC2);
	}

	if (current_link_up != netif_carrier_ok(tp->dev)) {
		if (current_link_up)
			netif_carrier_on(tp->dev);
		else
			netif_carrier_off(tp->dev);
		tg3_link_report(tp);
	}

	return 0;
}

struct tg3_fiber_aneginfo {
	int state;
#define ANEG_STATE_UNKNOWN		0
#define ANEG_STATE_AN_ENABLE		1
#define ANEG_STATE_RESTART_INIT		2
#define ANEG_STATE_RESTART		3
#define ANEG_STATE_DISABLE_LINK_OK	4
#define ANEG_STATE_ABILITY_DETECT_INIT	5
#define ANEG_STATE_ABILITY_DETECT	6
#define ANEG_STATE_ACK_DETECT_INIT	7
#define ANEG_STATE_ACK_DETECT		8
#define ANEG_STATE_COMPLETE_ACK_INIT	9
#define ANEG_STATE_COMPLETE_ACK		10
#define ANEG_STATE_IDLE_DETECT_INIT	11
#define ANEG_STATE_IDLE_DETECT		12
#define ANEG_STATE_LINK_OK		13
#define ANEG_STATE_NEXT_PAGE_WAIT_INIT	14
#define ANEG_STATE_NEXT_PAGE_WAIT	15

	u32 flags;
#define MR_AN_ENABLE		0x00000001
#define MR_RESTART_AN		0x00000002
#define MR_AN_COMPLETE		0x00000004
#define MR_PAGE_RX		0x00000008
#define MR_NP_LOADED		0x00000010
#define MR_TOGGLE_TX		0x00000020
#define MR_LP_ADV_FULL_DUPLEX	0x00000040
#define MR_LP_ADV_HALF_DUPLEX	0x00000080
#define MR_LP_ADV_SYM_PAUSE	0x00000100
#define MR_LP_ADV_ASYM_PAUSE	0x00000200
#define MR_LP_ADV_REMOTE_FAULT1	0x00000400
#define MR_LP_ADV_REMOTE_FAULT2	0x00000800
#define MR_LP_ADV_NEXT_PAGE	0x00001000
#define MR_TOGGLE_RX		0x00002000
#define MR_NP_RX		0x00004000

#define MR_LINK_OK		0x80000000

	unsigned long link_time, cur_time;

	u32 ability_match_cfg;
	int ability_match_count;

	char ability_match, idle_match, ack_match;

	u32 txconfig, rxconfig;
#define ANEG_CFG_NP		0x00000080
#define ANEG_CFG_ACK		0x00000040
#define ANEG_CFG_RF2		0x00000020
#define ANEG_CFG_RF1		0x00000010
#define ANEG_CFG_PS2		0x00000001
#define ANEG_CFG_PS1		0x00008000
#define ANEG_CFG_HD		0x00004000
#define ANEG_CFG_FD		0x00002000
#define ANEG_CFG_INVAL		0x00001f06

};
#define ANEG_OK		0
#define ANEG_DONE	1
#define ANEG_TIMER_ENAB	2
#define ANEG_FAILED	-1

#define ANEG_STATE_SETTLE_TIME	10000

static int tg3_fiber_aneg_smachine(struct tg3 *tp,
				   struct tg3_fiber_aneginfo *ap)
{
	unsigned long delta;
	u32 rx_cfg_reg;
	int ret;

	if (ap->state == ANEG_STATE_UNKNOWN) {
		ap->rxconfig = 0;
		ap->link_time = 0;
		ap->cur_time = 0;
		ap->ability_match_cfg = 0;
		ap->ability_match_count = 0;
		ap->ability_match = 0;
		ap->idle_match = 0;
		ap->ack_match = 0;
	}
	ap->cur_time++;

	if (tr32(MAC_STATUS) & MAC_STATUS_RCVD_CFG) {
		rx_cfg_reg = tr32(MAC_RX_AUTO_NEG);

		if (rx_cfg_reg != ap->ability_match_cfg) {
			ap->ability_match_cfg = rx_cfg_reg;
			ap->ability_match = 0;
			ap->ability_match_count = 0;
		} else {
			if (++ap->ability_match_count > 1) {
				ap->ability_match = 1;
				ap->ability_match_cfg = rx_cfg_reg;
			}
		}
		if (rx_cfg_reg & ANEG_CFG_ACK)
			ap->ack_match = 1;
		else
			ap->ack_match = 0;

		ap->idle_match = 0;
	} else {
		ap->idle_match = 1;
		ap->ability_match_cfg = 0;
		ap->ability_match_count = 0;
		ap->ability_match = 0;
		ap->ack_match = 0;

		rx_cfg_reg = 0;
	}

	ap->rxconfig = rx_cfg_reg;
	ret = ANEG_OK;

	switch(ap->state) {
	case ANEG_STATE_UNKNOWN:
		if (ap->flags & (MR_AN_ENABLE | MR_RESTART_AN))
			ap->state = ANEG_STATE_AN_ENABLE;

		/* fallthru */
	case ANEG_STATE_AN_ENABLE:
		ap->flags &= ~(MR_AN_COMPLETE | MR_PAGE_RX);
		if (ap->flags & MR_AN_ENABLE) {
			ap->link_time = 0;
			ap->cur_time = 0;
			ap->ability_match_cfg = 0;
			ap->ability_match_count = 0;
			ap->ability_match = 0;
			ap->idle_match = 0;
			ap->ack_match = 0;

			ap->state = ANEG_STATE_RESTART_INIT;
		} else {
			ap->state = ANEG_STATE_DISABLE_LINK_OK;
		}
		break;

	case ANEG_STATE_RESTART_INIT:
		ap->link_time = ap->cur_time;
		ap->flags &= ~(MR_NP_LOADED);
		ap->txconfig = 0;
		tw32(MAC_TX_AUTO_NEG, 0);
		tp->mac_mode |= MAC_MODE_SEND_CONFIGS;
		tw32_f(MAC_MODE, tp->mac_mode);
		udelay(40);

		ret = ANEG_TIMER_ENAB;
		ap->state = ANEG_STATE_RESTART;

		/* fallthru */
	case ANEG_STATE_RESTART:
		delta = ap->cur_time - ap->link_time;
		if (delta > ANEG_STATE_SETTLE_TIME) {
			ap->state = ANEG_STATE_ABILITY_DETECT_INIT;
		} else {
			ret = ANEG_TIMER_ENAB;
		}
		break;

	case ANEG_STATE_DISABLE_LINK_OK:
		ret = ANEG_DONE;
		break;

	case ANEG_STATE_ABILITY_DETECT_INIT:
		ap->flags &= ~(MR_TOGGLE_TX);
		ap->txconfig = (ANEG_CFG_FD | ANEG_CFG_PS1);
		tw32(MAC_TX_AUTO_NEG, ap->txconfig);
		tp->mac_mode |= MAC_MODE_SEND_CONFIGS;
		tw32_f(MAC_MODE, tp->mac_mode);
		udelay(40);

		ap->state = ANEG_STATE_ABILITY_DETECT;
		break;

	case ANEG_STATE_ABILITY_DETECT:
		if (ap->ability_match != 0 && ap->rxconfig != 0) {
			ap->state = ANEG_STATE_ACK_DETECT_INIT;
		}
		break;

	case ANEG_STATE_ACK_DETECT_INIT:
		ap->txconfig |= ANEG_CFG_ACK;
		tw32(MAC_TX_AUTO_NEG, ap->txconfig);
		tp->mac_mode |= MAC_MODE_SEND_CONFIGS;
		tw32_f(MAC_MODE, tp->mac_mode);
		udelay(40);

		ap->state = ANEG_STATE_ACK_DETECT;

		/* fallthru */
	case ANEG_STATE_ACK_DETECT:
		if (ap->ack_match != 0) {
			if ((ap->rxconfig & ~ANEG_CFG_ACK) ==
			    (ap->ability_match_cfg & ~ANEG_CFG_ACK)) {
				ap->state = ANEG_STATE_COMPLETE_ACK_INIT;
			} else {
				ap->state = ANEG_STATE_AN_ENABLE;
			}
		} else if (ap->ability_match != 0 &&
			   ap->rxconfig == 0) {
			ap->state = ANEG_STATE_AN_ENABLE;
		}
		break;

	case ANEG_STATE_COMPLETE_ACK_INIT:
		if (ap->rxconfig & ANEG_CFG_INVAL) {
			ret = ANEG_FAILED;
			break;
		}
		ap->flags &= ~(MR_LP_ADV_FULL_DUPLEX |
			       MR_LP_ADV_HALF_DUPLEX |
			       MR_LP_ADV_SYM_PAUSE |
			       MR_LP_ADV_ASYM_PAUSE |
			       MR_LP_ADV_REMOTE_FAULT1 |
			       MR_LP_ADV_REMOTE_FAULT2 |
			       MR_LP_ADV_NEXT_PAGE |
			       MR_TOGGLE_RX |
			       MR_NP_RX);
		if (ap->rxconfig & ANEG_CFG_FD)
			ap->flags |= MR_LP_ADV_FULL_DUPLEX;
		if (ap->rxconfig & ANEG_CFG_HD)
			ap->flags |= MR_LP_ADV_HALF_DUPLEX;
		if (ap->rxconfig & ANEG_CFG_PS1)
			ap->flags |= MR_LP_ADV_SYM_PAUSE;
		if (ap->rxconfig & ANEG_CFG_PS2)
			ap->flags |= MR_LP_ADV_ASYM_PAUSE;
		if (ap->rxconfig & ANEG_CFG_RF1)
			ap->flags |= MR_LP_ADV_REMOTE_FAULT1;
		if (ap->rxconfig & ANEG_CFG_RF2)
			ap->flags |= MR_LP_ADV_REMOTE_FAULT2;
		if (ap->rxconfig & ANEG_CFG_NP)
			ap->flags |= MR_LP_ADV_NEXT_PAGE;

		ap->link_time = ap->cur_time;

		ap->flags ^= (MR_TOGGLE_TX);
		if (ap->rxconfig & 0x0008)
			ap->flags |= MR_TOGGLE_RX;
		if (ap->rxconfig & ANEG_CFG_NP)
			ap->flags |= MR_NP_RX;
		ap->flags |= MR_PAGE_RX;

		ap->state = ANEG_STATE_COMPLETE_ACK;
		ret = ANEG_TIMER_ENAB;
		break;

	case ANEG_STATE_COMPLETE_ACK:
		if (ap->ability_match != 0 &&
		    ap->rxconfig == 0) {
			ap->state = ANEG_STATE_AN_ENABLE;
			break;
		}
		delta = ap->cur_time - ap->link_time;
		if (delta > ANEG_STATE_SETTLE_TIME) {
			if (!(ap->flags & (MR_LP_ADV_NEXT_PAGE))) {
				ap->state = ANEG_STATE_IDLE_DETECT_INIT;
			} else {
				if ((ap->txconfig & ANEG_CFG_NP) == 0 &&
				    !(ap->flags & MR_NP_RX)) {
					ap->state = ANEG_STATE_IDLE_DETECT_INIT;
				} else {
					ret = ANEG_FAILED;
				}
			}
		}
		break;

	case ANEG_STATE_IDLE_DETECT_INIT:
		ap->link_time = ap->cur_time;
		tp->mac_mode &= ~MAC_MODE_SEND_CONFIGS;
		tw32_f(MAC_MODE, tp->mac_mode);
		udelay(40);

		ap->state = ANEG_STATE_IDLE_DETECT;
		ret = ANEG_TIMER_ENAB;
		break;

	case ANEG_STATE_IDLE_DETECT:
		if (ap->ability_match != 0 &&
		    ap->rxconfig == 0) {
			ap->state = ANEG_STATE_AN_ENABLE;
			break;
		}
		delta = ap->cur_time - ap->link_time;
		if (delta > ANEG_STATE_SETTLE_TIME) {
			/* XXX another gem from the Broadcom driver :( */
			ap->state = ANEG_STATE_LINK_OK;
		}
		break;

	case ANEG_STATE_LINK_OK:
		ap->flags |= (MR_AN_COMPLETE | MR_LINK_OK);
		ret = ANEG_DONE;
		break;

	case ANEG_STATE_NEXT_PAGE_WAIT_INIT:
		/* ??? unimplemented */
		break;

	case ANEG_STATE_NEXT_PAGE_WAIT:
		/* ??? unimplemented */
		break;

	default:
		ret = ANEG_FAILED;
		break;
	};

	return ret;
}

static int fiber_autoneg(struct tg3 *tp, u32 *flags)
{
	int res = 0;
	struct tg3_fiber_aneginfo aninfo;
	int status = ANEG_FAILED;
	unsigned int tick;
	u32 tmp;

	tw32_f(MAC_TX_AUTO_NEG, 0);

	tmp = tp->mac_mode & ~MAC_MODE_PORT_MODE_MASK;
	tw32_f(MAC_MODE, tmp | MAC_MODE_PORT_MODE_GMII);
	udelay(40);

	tw32_f(MAC_MODE, tp->mac_mode | MAC_MODE_SEND_CONFIGS);
	udelay(40);

	memset(&aninfo, 0, sizeof(aninfo));
	aninfo.flags |= MR_AN_ENABLE;
	aninfo.state = ANEG_STATE_UNKNOWN;
	aninfo.cur_time = 0;
	tick = 0;
	while (++tick < 195000) {
		status = tg3_fiber_aneg_smachine(tp, &aninfo);
		if (status == ANEG_DONE || status == ANEG_FAILED)
			break;

		udelay(1);
	}

	tp->mac_mode &= ~MAC_MODE_SEND_CONFIGS;
	tw32_f(MAC_MODE, tp->mac_mode);
	udelay(40);

	*flags = aninfo.flags;

	if (status == ANEG_DONE &&
	    (aninfo.flags & (MR_AN_COMPLETE | MR_LINK_OK |
			     MR_LP_ADV_FULL_DUPLEX)))
		res = 1;

	return res;
}

static void tg3_init_bcm8002(struct tg3 *tp)
{
	u32 mac_status = tr32(MAC_STATUS);
	int i;

	/* Reset when initting first time or we have a link. */
	if ((tp->tg3_flags & TG3_FLAG_INIT_COMPLETE) &&
	    !(mac_status & MAC_STATUS_PCS_SYNCED))
		return;

	/* Set PLL lock range. */
	tg3_writephy(tp, 0x16, 0x8007);

	/* SW reset */
	tg3_writephy(tp, MII_BMCR, BMCR_RESET);

	/* Wait for reset to complete. */
	/* XXX schedule_timeout() ... */
	for (i = 0; i < 500; i++)
		udelay(10);

	/* Config mode; select PMA/Ch 1 regs. */
	tg3_writephy(tp, 0x10, 0x8411);

	/* Enable auto-lock and comdet, select txclk for tx. */
	tg3_writephy(tp, 0x11, 0x0a10);

	tg3_writephy(tp, 0x18, 0x00a0);
	tg3_writephy(tp, 0x16, 0x41ff);

	/* Assert and deassert POR. */
	tg3_writephy(tp, 0x13, 0x0400);
	udelay(40);
	tg3_writephy(tp, 0x13, 0x0000);

	tg3_writephy(tp, 0x11, 0x0a50);
	udelay(40);
	tg3_writephy(tp, 0x11, 0x0a10);

	/* Wait for signal to stabilize */
	/* XXX schedule_timeout() ... */
	for (i = 0; i < 15000; i++)
		udelay(10);

	/* Deselect the channel register so we can read the PHYID
	 * later.
	 */
	tg3_writephy(tp, 0x10, 0x8011);
}

static int tg3_setup_fiber_hw_autoneg(struct tg3 *tp, u32 mac_status)
{
	u32 sg_dig_ctrl, sg_dig_status;
	u32 serdes_cfg, expected_sg_dig_ctrl;
	int workaround, port_a;
	int current_link_up;

	serdes_cfg = 0;
	expected_sg_dig_ctrl = 0;
	workaround = 0;
	port_a = 1;
	current_link_up = 0;

	if (tp->pci_chip_rev_id != CHIPREV_ID_5704_A0 &&
	    tp->pci_chip_rev_id != CHIPREV_ID_5704_A1) {
		workaround = 1;
		if (tr32(TG3PCI_DUAL_MAC_CTRL) & DUAL_MAC_CTRL_ID)
			port_a = 0;

		/* preserve bits 0-11,13,14 for signal pre-emphasis */
		/* preserve bits 20-23 for voltage regulator */
		serdes_cfg = tr32(MAC_SERDES_CFG) & 0x00f06fff;
	}

	sg_dig_ctrl = tr32(SG_DIG_CTRL);

	if (tp->link_config.autoneg != AUTONEG_ENABLE) {
		if (sg_dig_ctrl & (1 << 31)) {
			if (workaround) {
				u32 val = serdes_cfg;

				if (port_a)
					val |= 0xc010000;
				else
					val |= 0x4010000;
				tw32_f(MAC_SERDES_CFG, val);
			}
			tw32_f(SG_DIG_CTRL, 0x01388400);
		}
		if (mac_status & MAC_STATUS_PCS_SYNCED) {
			tg3_setup_flow_control(tp, 0, 0);
			current_link_up = 1;
		}
		goto out;
	}

	/* Want auto-negotiation.  */
	expected_sg_dig_ctrl = 0x81388400;

	/* Pause capability */
	expected_sg_dig_ctrl |= (1 << 11);

	/* Asymettric pause */
	expected_sg_dig_ctrl |= (1 << 12);

	if (sg_dig_ctrl != expected_sg_dig_ctrl) {
		if (workaround)
			tw32_f(MAC_SERDES_CFG, serdes_cfg | 0xc011000);
		tw32_f(SG_DIG_CTRL, expected_sg_dig_ctrl | (1 << 30));
		udelay(5);
		tw32_f(SG_DIG_CTRL, expected_sg_dig_ctrl);

		tp->tg3_flags2 |= TG3_FLG2_PHY_JUST_INITTED;
	} else if (mac_status & (MAC_STATUS_PCS_SYNCED |
				 MAC_STATUS_SIGNAL_DET)) {
		int i;

		/* Giver time to negotiate (~200ms) */
		for (i = 0; i < 40000; i++) {
			sg_dig_status = tr32(SG_DIG_STATUS);
			if (sg_dig_status & (0x3))
				break;
			udelay(5);
		}
		mac_status = tr32(MAC_STATUS);

		if ((sg_dig_status & (1 << 1)) &&
		    (mac_status & MAC_STATUS_PCS_SYNCED)) {
			u32 local_adv, remote_adv;

			local_adv = ADVERTISE_PAUSE_CAP;
			remote_adv = 0;
			if (sg_dig_status & (1 << 19))
				remote_adv |= LPA_PAUSE_CAP;
			if (sg_dig_status & (1 << 20))
				remote_adv |= LPA_PAUSE_ASYM;

			tg3_setup_flow_control(tp, local_adv, remote_adv);
			current_link_up = 1;
			tp->tg3_flags2 &= ~TG3_FLG2_PHY_JUST_INITTED;
		} else if (!(sg_dig_status & (1 << 1))) {
			if (tp->tg3_flags2 & TG3_FLG2_PHY_JUST_INITTED)
				tp->tg3_flags2 &= ~TG3_FLG2_PHY_JUST_INITTED;
			else {
				if (workaround) {
					u32 val = serdes_cfg;

					if (port_a)
						val |= 0xc010000;
					else
						val |= 0x4010000;

					tw32_f(MAC_SERDES_CFG, val);
				}

				tw32_f(SG_DIG_CTRL, 0x01388400);
				udelay(40);

				/* Link parallel detection - link is up */
				/* only if we have PCS_SYNC and not */
				/* receiving config code words */
				mac_status = tr32(MAC_STATUS);
				if ((mac_status & MAC_STATUS_PCS_SYNCED) &&
				    !(mac_status & MAC_STATUS_RCVD_CFG)) {
					tg3_setup_flow_control(tp, 0, 0);
					current_link_up = 1;
				}
			}
		}
	}

out:
	return current_link_up;
}

static int tg3_setup_fiber_by_hand(struct tg3 *tp, u32 mac_status)
{
	int current_link_up = 0;

 	if (!(mac_status & MAC_STATUS_PCS_SYNCED)) {
		tp->tg3_flags &= ~TG3_FLAG_GOT_SERDES_FLOWCTL;
		goto out;
	}

	if (tp->link_config.autoneg == AUTONEG_ENABLE) {
		u32 flags;
		int i;
  
		if (fiber_autoneg(tp, &flags)) {
			u32 local_adv, remote_adv;

			local_adv = ADVERTISE_PAUSE_CAP;
			remote_adv = 0;
			if (flags & MR_LP_ADV_SYM_PAUSE)
				remote_adv |= LPA_PAUSE_CAP;
			if (flags & MR_LP_ADV_ASYM_PAUSE)
				remote_adv |= LPA_PAUSE_ASYM;

			tg3_setup_flow_control(tp, local_adv, remote_adv);

			tp->tg3_flags |= TG3_FLAG_GOT_SERDES_FLOWCTL;
			current_link_up = 1;
		}
		for (i = 0; i < 30; i++) {
			udelay(20);
			tw32_f(MAC_STATUS,
			       (MAC_STATUS_SYNC_CHANGED |
				MAC_STATUS_CFG_CHANGED));
			udelay(40);
			if ((tr32(MAC_STATUS) &
			     (MAC_STATUS_SYNC_CHANGED |
			      MAC_STATUS_CFG_CHANGED)) == 0)
				break;
		}

		mac_status = tr32(MAC_STATUS);
		if (current_link_up == 0 &&
		    (mac_status & MAC_STATUS_PCS_SYNCED) &&
		    !(mac_status & MAC_STATUS_RCVD_CFG))
			current_link_up = 1;
	} else {
		/* Forcing 1000FD link up. */
		current_link_up = 1;
		tp->tg3_flags |= TG3_FLAG_GOT_SERDES_FLOWCTL;

		tw32_f(MAC_MODE, (tp->mac_mode | MAC_MODE_SEND_CONFIGS));
		udelay(40);
	}

out:
	return current_link_up;
}

static int tg3_setup_fiber_phy(struct tg3 *tp, int force_reset)
{
	u32 orig_pause_cfg;
	u16 orig_active_speed;
	u8 orig_active_duplex;
	u32 mac_status;
	int current_link_up;
	int i;

	orig_pause_cfg =
		(tp->tg3_flags & (TG3_FLAG_RX_PAUSE |
				  TG3_FLAG_TX_PAUSE));
	orig_active_speed = tp->link_config.active_speed;
	orig_active_duplex = tp->link_config.active_duplex;

	if (!(tp->tg3_flags2 & TG3_FLG2_HW_AUTONEG) &&
	    netif_carrier_ok(tp->dev) &&
	    (tp->tg3_flags & TG3_FLAG_INIT_COMPLETE)) {
		mac_status = tr32(MAC_STATUS);
		mac_status &= (MAC_STATUS_PCS_SYNCED |
			       MAC_STATUS_SIGNAL_DET |
			       MAC_STATUS_CFG_CHANGED |
			       MAC_STATUS_RCVD_CFG);
		if (mac_status == (MAC_STATUS_PCS_SYNCED |
				   MAC_STATUS_SIGNAL_DET)) {
			tw32_f(MAC_STATUS, (MAC_STATUS_SYNC_CHANGED |
					    MAC_STATUS_CFG_CHANGED));
			return 0;
		}
	}

	tw32_f(MAC_TX_AUTO_NEG, 0);

	tp->mac_mode &= ~(MAC_MODE_PORT_MODE_MASK | MAC_MODE_HALF_DUPLEX);
	tp->mac_mode |= MAC_MODE_PORT_MODE_TBI;
	tw32_f(MAC_MODE, tp->mac_mode);
	udelay(40);

	if (tp->phy_id == PHY_ID_BCM8002)
		tg3_init_bcm8002(tp);

	/* Enable link change event even when serdes polling.  */
	tw32_f(MAC_EVENT, MAC_EVENT_LNKSTATE_CHANGED);
	udelay(40);

	current_link_up = 0;
	mac_status = tr32(MAC_STATUS);

	if (tp->tg3_flags2 & TG3_FLG2_HW_AUTONEG)
		current_link_up = tg3_setup_fiber_hw_autoneg(tp, mac_status);
	else
		current_link_up = tg3_setup_fiber_by_hand(tp, mac_status);

	tp->mac_mode &= ~MAC_MODE_LINK_POLARITY;
	tw32_f(MAC_MODE, tp->mac_mode);
	udelay(40);

	tp->hw_status->status =
		(SD_STATUS_UPDATED |
		 (tp->hw_status->status & ~SD_STATUS_LINK_CHG));

	for (i = 0; i < 100; i++) {
		tw32_f(MAC_STATUS, (MAC_STATUS_SYNC_CHANGED |
				    MAC_STATUS_CFG_CHANGED));
		udelay(5);
		if ((tr32(MAC_STATUS) & (MAC_STATUS_SYNC_CHANGED |
					 MAC_STATUS_CFG_CHANGED)) == 0)
			break;
	}

	mac_status = tr32(MAC_STATUS);
	if ((mac_status & MAC_STATUS_PCS_SYNCED) == 0) {
		current_link_up = 0;
		if (tp->link_config.autoneg == AUTONEG_ENABLE) {
			tw32_f(MAC_MODE, (tp->mac_mode |
					  MAC_MODE_SEND_CONFIGS));
			udelay(1);
			tw32_f(MAC_MODE, tp->mac_mode);
		}
	}

	if (current_link_up == 1) {
		tp->link_config.active_speed = SPEED_1000;
		tp->link_config.active_duplex = DUPLEX_FULL;
		tw32(MAC_LED_CTRL, (tp->led_ctrl |
				    LED_CTRL_LNKLED_OVERRIDE |
				    LED_CTRL_1000MBPS_ON));
	} else {
		tp->link_config.active_speed = SPEED_INVALID;
		tp->link_config.active_duplex = DUPLEX_INVALID;
		tw32(MAC_LED_CTRL, (tp->led_ctrl |
				    LED_CTRL_LNKLED_OVERRIDE |
				    LED_CTRL_TRAFFIC_OVERRIDE));
	}

	if (current_link_up != netif_carrier_ok(tp->dev)) {
		if (current_link_up)
			netif_carrier_on(tp->dev);
		else
			netif_carrier_off(tp->dev);
		tg3_link_report(tp);
	} else {
		u32 now_pause_cfg =
			tp->tg3_flags & (TG3_FLAG_RX_PAUSE |
					 TG3_FLAG_TX_PAUSE);
		if (orig_pause_cfg != now_pause_cfg ||
		    orig_active_speed != tp->link_config.active_speed ||
		    orig_active_duplex != tp->link_config.active_duplex)
			tg3_link_report(tp);
	}

	return 0;
}

static int tg3_setup_fiber_mii_phy(struct tg3 *tp, int force_reset)
{
	int current_link_up, err = 0;
	u32 bmsr, bmcr;
	u16 current_speed;
	u8 current_duplex;

	tp->mac_mode |= MAC_MODE_PORT_MODE_GMII;
	tw32_f(MAC_MODE, tp->mac_mode);
	udelay(40);

	tw32(MAC_EVENT, 0);

	tw32_f(MAC_STATUS,
	     (MAC_STATUS_SYNC_CHANGED |
	      MAC_STATUS_CFG_CHANGED |
	      MAC_STATUS_MI_COMPLETION |
	      MAC_STATUS_LNKSTATE_CHANGED));
	udelay(40);

	if (force_reset)
		tg3_phy_reset(tp);

	current_link_up = 0;
	current_speed = SPEED_INVALID;
	current_duplex = DUPLEX_INVALID;

	err |= tg3_readphy(tp, MII_BMSR, &bmsr);
	err |= tg3_readphy(tp, MII_BMSR, &bmsr);
	if (GET_ASIC_REV(tp->pci_chip_rev_id) == ASIC_REV_5714) {
		if (tr32(MAC_TX_STATUS) & TX_STATUS_LINK_UP)
			bmsr |= BMSR_LSTATUS;
		else
			bmsr &= ~BMSR_LSTATUS;
	}

	err |= tg3_readphy(tp, MII_BMCR, &bmcr);

	if ((tp->link_config.autoneg == AUTONEG_ENABLE) && !force_reset &&
	    (tp->tg3_flags2 & TG3_FLG2_PARALLEL_DETECT)) {
		/* do nothing, just check for link up at the end */
	} else if (tp->link_config.autoneg == AUTONEG_ENABLE) {
		u32 adv, new_adv;

		err |= tg3_readphy(tp, MII_ADVERTISE, &adv);
		new_adv = adv & ~(ADVERTISE_1000XFULL | ADVERTISE_1000XHALF |
				  ADVERTISE_1000XPAUSE |
				  ADVERTISE_1000XPSE_ASYM |
				  ADVERTISE_SLCT);

		/* Always advertise symmetric PAUSE just like copper */
		new_adv |= ADVERTISE_1000XPAUSE;

		if (tp->link_config.advertising & ADVERTISED_1000baseT_Half)
			new_adv |= ADVERTISE_1000XHALF;
		if (tp->link_config.advertising & ADVERTISED_1000baseT_Full)
			new_adv |= ADVERTISE_1000XFULL;

		if ((new_adv != adv) || !(bmcr & BMCR_ANENABLE)) {
			tg3_writephy(tp, MII_ADVERTISE, new_adv);
			bmcr |= BMCR_ANENABLE | BMCR_ANRESTART;
			tg3_writephy(tp, MII_BMCR, bmcr);

			tw32_f(MAC_EVENT, MAC_EVENT_LNKSTATE_CHANGED);
			tp->tg3_flags2 |= TG3_FLG2_PHY_JUST_INITTED;
			tp->tg3_flags2 &= ~TG3_FLG2_PARALLEL_DETECT;

			return err;
		}
	} else {
		u32 new_bmcr;

		bmcr &= ~BMCR_SPEED1000;
		new_bmcr = bmcr & ~(BMCR_ANENABLE | BMCR_FULLDPLX);

		if (tp->link_config.duplex == DUPLEX_FULL)
			new_bmcr |= BMCR_FULLDPLX;

		if (new_bmcr != bmcr) {
			/* BMCR_SPEED1000 is a reserved bit that needs
			 * to be set on write.
			 */
			new_bmcr |= BMCR_SPEED1000;

			/* Force a linkdown */
			if (netif_carrier_ok(tp->dev)) {
				u32 adv;

				err |= tg3_readphy(tp, MII_ADVERTISE, &adv);
				adv &= ~(ADVERTISE_1000XFULL |
					 ADVERTISE_1000XHALF |
					 ADVERTISE_SLCT);
				tg3_writephy(tp, MII_ADVERTISE, adv);
				tg3_writephy(tp, MII_BMCR, bmcr |
							   BMCR_ANRESTART |
							   BMCR_ANENABLE);
				udelay(10);
				netif_carrier_off(tp->dev);
			}
			tg3_writephy(tp, MII_BMCR, new_bmcr);
			bmcr = new_bmcr;
			err |= tg3_readphy(tp, MII_BMSR, &bmsr);
			err |= tg3_readphy(tp, MII_BMSR, &bmsr);
			if (GET_ASIC_REV(tp->pci_chip_rev_id) ==
			    ASIC_REV_5714) {
				if (tr32(MAC_TX_STATUS) & TX_STATUS_LINK_UP)
					bmsr |= BMSR_LSTATUS;
				else
					bmsr &= ~BMSR_LSTATUS;
			}
			tp->tg3_flags2 &= ~TG3_FLG2_PARALLEL_DETECT;
		}
	}

	if (bmsr & BMSR_LSTATUS) {
		current_speed = SPEED_1000;
		current_link_up = 1;
		if (bmcr & BMCR_FULLDPLX)
			current_duplex = DUPLEX_FULL;
		else
			current_duplex = DUPLEX_HALF;

		if (bmcr & BMCR_ANENABLE) {
			u32 local_adv, remote_adv, common;

			err |= tg3_readphy(tp, MII_ADVERTISE, &local_adv);
			err |= tg3_readphy(tp, MII_LPA, &remote_adv);
			common = local_adv & remote_adv;
			if (common & (ADVERTISE_1000XHALF |
				      ADVERTISE_1000XFULL)) {
				if (common & ADVERTISE_1000XFULL)
					current_duplex = DUPLEX_FULL;
				else
					current_duplex = DUPLEX_HALF;

				tg3_setup_flow_control(tp, local_adv,
						       remote_adv);
			}
			else
				current_link_up = 0;
		}
	}

	tp->mac_mode &= ~MAC_MODE_HALF_DUPLEX;
	if (tp->link_config.active_duplex == DUPLEX_HALF)
		tp->mac_mode |= MAC_MODE_HALF_DUPLEX;

	tw32_f(MAC_MODE, tp->mac_mode);
	udelay(40);

	tw32_f(MAC_EVENT, MAC_EVENT_LNKSTATE_CHANGED);

	tp->link_config.active_speed = current_speed;
	tp->link_config.active_duplex = current_duplex;

	if (current_link_up != netif_carrier_ok(tp->dev)) {
		if (current_link_up)
			netif_carrier_on(tp->dev);
		else {
			netif_carrier_off(tp->dev);
			tp->tg3_flags2 &= ~TG3_FLG2_PARALLEL_DETECT;
		}
		tg3_link_report(tp);
	}
	return err;
}

static void tg3_serdes_parallel_detect(struct tg3 *tp)
{
	if (tp->tg3_flags2 & TG3_FLG2_PHY_JUST_INITTED) {
		/* Give autoneg time to complete. */
		tp->tg3_flags2 &= ~TG3_FLG2_PHY_JUST_INITTED;
		return;
	}
	if (!netif_carrier_ok(tp->dev) &&
	    (tp->link_config.autoneg == AUTONEG_ENABLE)) {
		u32 bmcr;

		tg3_readphy(tp, MII_BMCR, &bmcr);
		if (bmcr & BMCR_ANENABLE) {
			u32 phy1, phy2;

			/* Select shadow register 0x1f */
			tg3_writephy(tp, 0x1c, 0x7c00);
			tg3_readphy(tp, 0x1c, &phy1);

			/* Select expansion interrupt status register */
			tg3_writephy(tp, 0x17, 0x0f01);
			tg3_readphy(tp, 0x15, &phy2);
			tg3_readphy(tp, 0x15, &phy2);

			if ((phy1 & 0x10) && !(phy2 & 0x20)) {
				/* We have signal detect and not receiving
				 * config code words, link is up by parallel
				 * detection.
				 */

				bmcr &= ~BMCR_ANENABLE;
				bmcr |= BMCR_SPEED1000 | BMCR_FULLDPLX;
				tg3_writephy(tp, MII_BMCR, bmcr);
				tp->tg3_flags2 |= TG3_FLG2_PARALLEL_DETECT;
			}
		}
	}
	else if (netif_carrier_ok(tp->dev) &&
		 (tp->link_config.autoneg == AUTONEG_ENABLE) &&
		 (tp->tg3_flags2 & TG3_FLG2_PARALLEL_DETECT)) {
		u32 phy2;

		/* Select expansion interrupt status register */
		tg3_writephy(tp, 0x17, 0x0f01);
		tg3_readphy(tp, 0x15, &phy2);
		if (phy2 & 0x20) {
			u32 bmcr;

			/* Config code words received, turn on autoneg. */
			tg3_readphy(tp, MII_BMCR, &bmcr);
			tg3_writephy(tp, MII_BMCR, bmcr | BMCR_ANENABLE);

			tp->tg3_flags2 &= ~TG3_FLG2_PARALLEL_DETECT;

		}
	}
}

static int tg3_setup_phy(struct tg3 *tp, int force_reset)
{
	int err;

	if (tp->tg3_flags2 & TG3_FLG2_PHY_SERDES) {
		err = tg3_setup_fiber_phy(tp, force_reset);
	} else if (tp->tg3_flags2 & TG3_FLG2_MII_SERDES) {
		err = tg3_setup_fiber_mii_phy(tp, force_reset);
	} else {
		err = tg3_setup_copper_phy(tp, force_reset);
	}

	if (tp->link_config.active_speed == SPEED_1000 &&
	    tp->link_config.active_duplex == DUPLEX_HALF)
		tw32(MAC_TX_LENGTHS,
		     ((2 << TX_LENGTHS_IPG_CRS_SHIFT) |
		      (6 << TX_LENGTHS_IPG_SHIFT) |
		      (0xff << TX_LENGTHS_SLOT_TIME_SHIFT)));
	else
		tw32(MAC_TX_LENGTHS,
		     ((2 << TX_LENGTHS_IPG_CRS_SHIFT) |
		      (6 << TX_LENGTHS_IPG_SHIFT) |
		      (32 << TX_LENGTHS_SLOT_TIME_SHIFT)));

	if (!(tp->tg3_flags2 & TG3_FLG2_5705_PLUS)) {
		if (netif_carrier_ok(tp->dev)) {
			tw32(HOSTCC_STAT_COAL_TICKS,
			     DEFAULT_STAT_COAL_TICKS);
		} else {
			tw32(HOSTCC_STAT_COAL_TICKS, 0);
		}
	}

	return err;
}

/* This is called whenever we suspect that the system chipset is re-
 * ordering the sequence of MMIO to the tx send mailbox. The symptom
 * is bogus tx completions. We try to recover by setting the
 * TG3_FLAG_MBOX_WRITE_REORDER flag and resetting the chip later
 * in the workqueue.
 */
static void tg3_tx_recover(struct tg3 *tp)
{
	kerndbg( KERN_WARNING, "%s: The system may be re-ordering memory-"
	       "mapped I/O cycles to the network device, attempting to "
	       "recover. Please report the problem to the driver maintainer "
	       "and include system chipset information.\n", tp->dev->name);

	spin_lock(&tp->lock);
	tp->tg3_flags |= TG3_FLAG_TX_RECOVERY_PENDING;
	spin_unlock(&tp->lock);
}

static inline u32 tg3_tx_avail(struct tg3 *tp)
{
	smp_wmb();
	return (tp->tx_pending -
		((tp->tx_prod - tp->tx_cons) & (TG3_TX_RING_SIZE - 1)));
}

/* Tigon3 never reports partial packet sends.  So we do not
 * need special logic to handle SKBs that have not had all
 * of their frags sent yet, like SunGEM does.
 */
static void tg3_tx(struct tg3 *tp)
{
	u32 hw_idx = tp->hw_status->idx[0].tx_consumer;
	u32 sw_idx = tp->tx_cons;

	kerndbg( KERN_DEBUG, "%s\n", __FUNCTION__ );

	while (sw_idx != hw_idx) {
		struct tx_ring_info *ri = &tp->tx_buffers[sw_idx];
		PacketBuf_s *skb = ri->skb;
		int i, tx_bug = 0;

		if (skb == NULL) {
			tg3_tx_recover(tp);
			return;
		}

		pci_unmap_single(tp->pdev,
				 pci_unmap_addr(ri, mapping),
				 skb_headlen(skb),
				 PCI_DMA_TODEVICE);

		ri->skb = NULL;

		sw_idx = NEXT_TX(sw_idx);

		free_pkt_buffer(skb);

		if (tx_bug) {
			tg3_tx_recover(tp);
			return;
		}
	}

	tp->tx_cons = sw_idx;

	/* Need to make the tx_cons update visible to tg3_start_xmit()
	 * before checking for netif_queue_stopped().  Without the
	 * memory barrier, there is a small possibility that tg3_start_xmit()
	 * will miss it and cause the queue to be stopped forever.
	 */
	smp_wmb();

	if (netif_queue_stopped(tp->dev) &&
		     (tg3_tx_avail(tp) > TG3_TX_WAKEUP_THRESH)) {
		if (netif_queue_stopped(tp->dev) &&
		    (tg3_tx_avail(tp) > TG3_TX_WAKEUP_THRESH))
			netif_wake_queue(tp->dev);
	}
}

/* Returns size of skb allocated or < 0 on error.
 *
 * We only need to fill in the address because the other members
 * of the RX descriptor are invariant, see tg3_init_rings.
 *
 * Note the purposeful assymetry of cpu vs. chip accesses.  For
 * posting buffers we only dirty the first cache line of the RX
 * descriptor (containing the address).  Whereas for the RX status
 * buffers the cpu only reads the last cacheline of the RX descriptor
 * (to fetch the error flags, vlan tag, checksum, and opaque cookie).
 */
static int tg3_alloc_rx_skb(struct tg3 *tp, u32 opaque_key,
			    int src_idx, u32 dest_idx_unmasked)
{
	struct tg3_rx_buffer_desc *desc;
	struct ring_info *map, *src_map;
	PacketBuf_s *skb;
	dma_addr_t mapping;
	int skb_size, dest_idx;

	src_map = NULL;
	switch (opaque_key) {
	case RXD_OPAQUE_RING_STD:
		dest_idx = dest_idx_unmasked % TG3_RX_RING_SIZE;
		desc = &tp->rx_std[dest_idx];
		map = &tp->rx_std_buffers[dest_idx];
		if (src_idx >= 0)
			src_map = &tp->rx_std_buffers[src_idx];
		skb_size = tp->rx_pkt_buf_sz;
		break;

	case RXD_OPAQUE_RING_JUMBO:
		dest_idx = dest_idx_unmasked % TG3_RX_JUMBO_RING_SIZE;
		desc = &tp->rx_jumbo[dest_idx];
		map = &tp->rx_jumbo_buffers[dest_idx];
		if (src_idx >= 0)
			src_map = &tp->rx_jumbo_buffers[src_idx];
		skb_size = RX_JUMBO_PKT_BUF_SZ;
		break;

	default:
		return -EINVAL;
	};

	/* Do not overwrite any of the map or rp information
	 * until we are sure we can commit to a new buffer.
	 *
	 * Callers depend upon this behavior and assume that
	 * we leave everything unchanged if we fail.
	 */
	skb = alloc_pkt_buffer(skb_size);
	if (skb == NULL)
		return -ENOMEM;

	map->skb = skb;

	if (src_map != NULL)
		src_map->skb = NULL;

	desc->addr_hi = ((u64)skb->pb_pData >> 32);
	desc->addr_lo = ((u64)skb->pb_pData & 0xffffffff);

	return skb_size;
}

/* We only need to move over in the address because the other
 * members of the RX descriptor are invariant.  See notes above
 * tg3_alloc_rx_skb for full details.
 */
static void tg3_recycle_rx(struct tg3 *tp, u32 opaque_key,
			   int src_idx, u32 dest_idx_unmasked)
{
	struct tg3_rx_buffer_desc *src_desc, *dest_desc;
	struct ring_info *src_map, *dest_map;
	int dest_idx;

	switch (opaque_key) {
	case RXD_OPAQUE_RING_STD:
		dest_idx = dest_idx_unmasked % TG3_RX_RING_SIZE;
		dest_desc = &tp->rx_std[dest_idx];
		dest_map = &tp->rx_std_buffers[dest_idx];
		src_desc = &tp->rx_std[src_idx];
		src_map = &tp->rx_std_buffers[src_idx];
		break;

	case RXD_OPAQUE_RING_JUMBO:
		dest_idx = dest_idx_unmasked % TG3_RX_JUMBO_RING_SIZE;
		dest_desc = &tp->rx_jumbo[dest_idx];
		dest_map = &tp->rx_jumbo_buffers[dest_idx];
		src_desc = &tp->rx_jumbo[src_idx];
		src_map = &tp->rx_jumbo_buffers[src_idx];
		break;

	default:
		return;
	};

	dest_map->skb = src_map->skb;
	dest_desc->addr_hi = src_desc->addr_hi;
	dest_desc->addr_lo = src_desc->addr_lo;

	src_map->skb = NULL;
}

/* The RX ring scheme is composed of multiple rings which post fresh
 * buffers to the chip, and one special ring the chip uses to report
 * status back to the host.
 *
 * The special ring reports the status of received packets to the
 * host.  The chip does not write into the original descriptor the
 * RX buffer was obtained from.  The chip simply takes the original
 * descriptor as provided by the host, updates the status and length
 * field, then writes this into the next status ring entry.
 *
 * Each ring the host uses to post buffers to the chip is described
 * by a TG3_BDINFO entry in the chips SRAM area.  When a packet arrives,
 * it is first placed into the on-chip ram.  When the packet's length
 * is known, it walks down the TG3_BDINFO entries to select the ring.
 * Each TG3_BDINFO specifies a MAXLEN field and the first TG3_BDINFO
 * which is within the range of the new packet's length is chosen.
 *
 * The "separate ring for rx status" scheme may sound queer, but it makes
 * sense from a cache coherency perspective.  If only the host writes
 * to the buffer post rings, and only the chip writes to the rx status
 * rings, then cache lines never move beyond shared-modified state.
 * If both the host and chip were to write into the same ring, cache line
 * eviction could occur since both entities want it in an exclusive state.
 */
static int tg3_rx(struct tg3 *tp)
{
	u32 work_mask, rx_std_posted = 0;
	u32 sw_idx = tp->rx_rcb_ptr;
	u16 hw_idx;
	int received;

	int budget = 100;	/* XXXKV: This is a placeholder value, we'll adjust it later */

	kerndbg( KERN_DEBUG, "%s: ENTER\n", __FUNCTION__ );

	hw_idx = tp->hw_status->idx[0].rx_producer;
	/*
	 * We need to order the read of hw_idx and the read of
	 * the opaque cookie.
	 */
	smp_rmb();
	work_mask = 0;
	received = 0;
	while (sw_idx != hw_idx && budget > 0) {
		struct tg3_rx_buffer_desc *desc = &tp->rx_rcb[sw_idx];
		unsigned int len;
		PacketBuf_s *skb;
		dma_addr_t dma_addr;
		u32 opaque_key, desc_idx, *post_ptr;

		desc_idx = desc->opaque & RXD_OPAQUE_INDEX_MASK;
		opaque_key = desc->opaque & RXD_OPAQUE_RING_MASK;
		if (opaque_key == RXD_OPAQUE_RING_STD) {
			dma_addr = pci_unmap_addr(&tp->rx_std_buffers[desc_idx],
						  mapping);
			skb = tp->rx_std_buffers[desc_idx].skb;
			post_ptr = &tp->rx_std_ptr;
			rx_std_posted++;
		} else if (opaque_key == RXD_OPAQUE_RING_JUMBO) {
			dma_addr = pci_unmap_addr(&tp->rx_jumbo_buffers[desc_idx],
						  mapping);
			skb = tp->rx_jumbo_buffers[desc_idx].skb;
			post_ptr = &tp->rx_jumbo_ptr;
		}
		else {
			goto next_pkt_nopost;
		}

		work_mask |= opaque_key;

		if ((desc->err_vlan & RXD_ERR_MASK) != 0 &&
		    (desc->err_vlan != RXD_ERR_ODD_NIBBLE_RCVD_MII)) {
		drop_it:
			tg3_recycle_rx(tp, opaque_key,
				       desc_idx, *post_ptr);
		drop_it_no_recycle:
			/* Other statistics kept track of by card. */
			tp->net_stats.rx_dropped++;
			goto next_pkt;
		}

		len = ((desc->idx_len & RXD_LEN_MASK) >> RXD_LEN_SHIFT) - 4; /* omit crc */

		if (len > RX_COPY_THRESHOLD 
			&& tp->rx_offset == 2
			/* rx_offset != 2 iff this is a 5701 card running
			 * in PCI-X mode [see tg3_get_invariants()] */
		) {
			int skb_size;

			skb_size = tg3_alloc_rx_skb(tp, opaque_key,
						    desc_idx, *post_ptr);
			if (skb_size < 0)
				goto drop_it;

			pci_unmap_single(tp->pdev, dma_addr,
					 skb_size - tp->rx_offset,
					 PCI_DMA_FROMDEVICE);

			skb_put(skb, len);
		} else {
			PacketBuf_s *copy_skb;

			tg3_recycle_rx(tp, opaque_key,
				       desc_idx, *post_ptr);

			copy_skb = alloc_pkt_buffer(len + 2);
			if (copy_skb == NULL)
				goto drop_it_no_recycle;

			copy_skb->pb_nSize = 0;
			memcpy( skb_put( copy_skb, len ), skb->pb_pData, len );

			/* We'll reuse the original ring buffer. */
			skb = copy_skb;
		}

		if ( tp->dev->packet_queue != NULL ){
			skb->pb_uMacHdr.pRaw = skb->pb_pData;
			enqueue_packet( tp->dev->packet_queue, skb );
		}
		else
		{
			kerndbg( KERN_DEBUG, "tg3: tg3_rx() recieved packet to downed device!\n" );
			free_pkt_buffer( skb );
		}

		tp->dev->last_rx = jiffies;
		received++;
		budget--;

next_pkt:
		(*post_ptr)++;

		if (rx_std_posted >= tp->rx_std_max_post) {
			u32 idx = *post_ptr % TG3_RX_RING_SIZE;

			tw32_rx_mbox(MAILBOX_RCV_STD_PROD_IDX +
				     TG3_64BIT_REG_LOW, idx);
			work_mask &= ~RXD_OPAQUE_RING_STD;
			rx_std_posted = 0;
		}
next_pkt_nopost:
		sw_idx++;
		sw_idx %= TG3_RX_RCB_RING_SIZE(tp);

		/* Refresh hw_idx to see if there is new work */
		if (sw_idx == hw_idx) {
			hw_idx = tp->hw_status->idx[0].rx_producer;
			smp_rmb();
		}
	}

	/* ACK the status ring. */
	tp->rx_rcb_ptr = sw_idx;
	tw32_rx_mbox(MAILBOX_RCVRET_CON_IDX_0 + TG3_64BIT_REG_LOW, sw_idx);

	/* Refill RX ring(s). */
	if (work_mask & RXD_OPAQUE_RING_STD) {
		sw_idx = tp->rx_std_ptr % TG3_RX_RING_SIZE;
		tw32_rx_mbox(MAILBOX_RCV_STD_PROD_IDX + TG3_64BIT_REG_LOW,
			     sw_idx);
	}
	if (work_mask & RXD_OPAQUE_RING_JUMBO) {
		sw_idx = tp->rx_jumbo_ptr % TG3_RX_JUMBO_RING_SIZE;
		tw32_rx_mbox(MAILBOX_RCV_JUMBO_PROD_IDX + TG3_64BIT_REG_LOW,
			     sw_idx);
	}
	smp_wmb();

	kerndbg( KERN_DEBUG, "%s: EXIT\n", __FUNCTION__ );

	return received;
}

static void tg3_reset_task(struct tg3 *tp);

static int tg3_poll(struct net_device *netdev)
{
	struct tg3 *tp = netdev_priv(netdev);
	struct tg3_hw_status *sblk = tp->hw_status;
	int done;

	kerndbg( KERN_DEBUG, "%s\n", __FUNCTION__ );

	/* handle link change and other phy events */
	if (!(tp->tg3_flags &
	      (TG3_FLAG_USE_LINKCHG_REG |
	       TG3_FLAG_POLL_SERDES))) {
		if (sblk->status & SD_STATUS_LINK_CHG) {
			sblk->status = SD_STATUS_UPDATED |
				(sblk->status & ~SD_STATUS_LINK_CHG);
			kerndbg( KERN_INFO, "tg3: Link status changed.\n" );
			spin_lock(&tp->lock);
			tg3_setup_phy(tp, 0);
			spin_unlock(&tp->lock);
		}
	}

	/* run TX completion thread */
	if (sblk->idx[0].tx_consumer != tp->tx_cons) {
		tg3_tx(tp);
		if (tp->tg3_flags & TG3_FLAG_TX_RECOVERY_PENDING) {
			tg3_reset_task(tp);
			return 0;
		}
	}

	/* run RX thread, within the bounds set by NAPI.
	 * All RX "locking" is done by ensuring outside
	 * code synchronizes with dev->poll()
	 */
	if (sblk->idx[0].rx_producer != tp->rx_rcb_ptr) {
		tg3_rx(tp);
	}

	if (tp->tg3_flags & TG3_FLAG_TAGGED_STATUS) {
		tp->last_tag = sblk->status_tag;
		smp_rmb();
	} else
		sblk->status &= ~SD_STATUS_UPDATED;

	/* if no more work, tell net stack and NIC we're done */
	done = !tg3_has_work(tp);
	if (done) {
		tg3_restart_ints(tp);
	}

	return (done ? 0 : 1);
}

static void tg3_irq_quiesce(struct tg3 *tp)
{
	tp->irq_sync = 1;
}

static inline int tg3_irq_sync(struct tg3 *tp)
{
	return tp->irq_sync;
}

/* Fully shutdown all tg3 driver activity elsewhere in the system.
 * If irq_sync is non-zero, then the IRQ handler must be synchronized
 * with as well.  Most of the time, this is not necessary except when
 * shutting down the device.
 */
static inline void tg3_full_lock(struct tg3 *tp, int irq_sync)
{
	if (irq_sync)
		tg3_irq_quiesce(tp);
	spinlock(&tp->lock);
}

static inline void tg3_full_unlock(struct tg3 *tp)
{
	spinunlock(&tp->lock);
}

/* One-shot MSI handler - Chip automatically disables interrupt
 * after sending MSI so driver doesn't have to do it.
 */
static int tg3_msi_1shot(int irq, void *dev_id, SysCallRegs_s *regs)
{
	struct net_device *dev = dev_id;
	struct tg3 *tp = netdev_priv(dev);

	kerndbg( KERN_DEBUG, "%s\n", __FUNCTION__ );

	prefetch(tp->hw_status);
	prefetch(&tp->rx_rcb[tp->rx_rcb_ptr]);

	if (!tg3_irq_sync(tp))
		tg3_poll(dev);		/* do work */

	return 0;
}

/* MSI ISR - No need to check for interrupt sharing and no need to
 * flush status block and interrupt mailbox. PCI ordering rules
 * guarantee that MSI will arrive after the status block.
 */
static int tg3_msi(int irq, void *dev_id, SysCallRegs_s *regs)
{
	struct net_device *dev = dev_id;
	struct tg3 *tp = netdev_priv(dev);

	kerndbg( KERN_DEBUG, "%s\n", __FUNCTION__ );

	prefetch(tp->hw_status);
	prefetch(&tp->rx_rcb[tp->rx_rcb_ptr]);
	/*
	 * Writing any value to intr-mbox-0 clears PCI INTA# and
	 * chip-internal interrupt pending events.
	 * Writing non-zero to intr-mbox-0 additional tells the
	 * NIC to stop sending us irqs, engaging "in-intr-handler"
	 * event coalescing.
	 */
	tw32_mailbox(MAILBOX_INTERRUPT_0 + TG3_64BIT_REG_LOW, 0x00000001);
	if (!tg3_irq_sync(tp))
		tg3_poll(dev);		/* do work */

	return 1;
}

static int tg3_interrupt(int irq, void *dev_id, SysCallRegs_s *regs)
{
	struct net_device *dev = dev_id;
	struct tg3 *tp = netdev_priv(dev);
	struct tg3_hw_status *sblk = tp->hw_status;
	unsigned int handled = 1;

	kerndbg( KERN_DEBUG, "%s\n", __FUNCTION__ );

	/* In INTx mode, it is possible for the interrupt to arrive at
	 * the CPU before the status block posted prior to the interrupt.
	 * Reading the PCI State register will confirm whether the
	 * interrupt is ours and will flush the status block.
	 */
	if ((sblk->status & SD_STATUS_UPDATED) ||
	    !(tr32(TG3PCI_PCISTATE) & PCISTATE_INT_NOT_ACTIVE)) {
		/*
		 * Writing any value to intr-mbox-0 clears PCI INTA# and
		 * chip-internal interrupt pending events.
		 * Writing non-zero to intr-mbox-0 additional tells the
		 * NIC to stop sending us irqs, engaging "in-intr-handler"
		 * event coalescing.
		 */
		tw32_mailbox(MAILBOX_INTERRUPT_0 + TG3_64BIT_REG_LOW,
			     0x00000001);
		if (tg3_irq_sync(tp))
			goto out;
		sblk->status &= ~SD_STATUS_UPDATED;
		if (tg3_has_work(tp)) {
			prefetch(&tp->rx_rcb[tp->rx_rcb_ptr]);
			tg3_poll(dev);		/* do work */
		} else {
			/* No work, shared interrupt perhaps?  re-enable
			 * interrupts, and flush that PCI write
			 */
			tw32_mailbox_f(MAILBOX_INTERRUPT_0 + TG3_64BIT_REG_LOW,
			     	0x00000000);
		}
	} else {	/* shared interrupt */
		handled = 0;
	}
out:
	return handled;
}

static int tg3_interrupt_tagged(int irq, void *dev_id, SysCallRegs_s *regs)
{
	struct net_device *dev = dev_id;
	struct tg3 *tp = netdev_priv(dev);
	struct tg3_hw_status *sblk = tp->hw_status;
	unsigned int handled = 1;

	kerndbg( KERN_DEBUG, "%s\n", __FUNCTION__ );

	/* In INTx mode, it is possible for the interrupt to arrive at
	 * the CPU before the status block posted prior to the interrupt.
	 * Reading the PCI State register will confirm whether the
	 * interrupt is ours and will flush the status block.
	 */
	if ((sblk->status_tag != tp->last_tag) ||
	    !(tr32(TG3PCI_PCISTATE) & PCISTATE_INT_NOT_ACTIVE)) {
		/*
		 * writing any value to intr-mbox-0 clears PCI INTA# and
		 * chip-internal interrupt pending events.
		 * writing non-zero to intr-mbox-0 additional tells the
		 * NIC to stop sending us irqs, engaging "in-intr-handler"
		 * event coalescing.
		 */
		tw32_mailbox(MAILBOX_INTERRUPT_0 + TG3_64BIT_REG_LOW,
			     0x00000001);
		if (tg3_irq_sync(tp))
			goto out;
		{
			prefetch(&tp->rx_rcb[tp->rx_rcb_ptr]);
			/* Update last_tag to mark that this status has been
			 * seen. Because interrupt may be shared, we may be
			 * racing with tg3_poll(), so only update last_tag
			 * if tg3_poll() is not scheduled.
			 */
			tp->last_tag = sblk->status_tag;
			tg3_poll(dev);
		}
	} else {	/* shared interrupt */
		handled = 0;
	}
out:
	return handled;
}

/* ISR for interrupt test */
static int tg3_test_isr(int irq, void *dev_id,
		SysCallRegs_s *regs)
{
	struct net_device *dev = dev_id;
	struct tg3 *tp = netdev_priv(dev);
	struct tg3_hw_status *sblk = tp->hw_status;

	kerndbg( KERN_DEBUG, "%s\n", __FUNCTION__ );

	if ((sblk->status & SD_STATUS_UPDATED) ||
	    !(tr32(TG3PCI_PCISTATE) & PCISTATE_INT_NOT_ACTIVE)) {
		tw32_mailbox(MAILBOX_INTERRUPT_0 + TG3_64BIT_REG_LOW,
			     0x00000001);
		return 1;
	}
	return 0;
}

static int tg3_init_hw(struct tg3 *, int);
static int tg3_halt(struct tg3 *, int, int);
static int tg3_close(struct net_device *dev);

/* Restart hardware after configuration changes, self-test, etc.
 * Invoked with tp->lock held.
 */
static int tg3_restart_hw(struct tg3 *tp, int reset_phy)
{
	int err;

	err = tg3_init_hw(tp, reset_phy);
	if (err) {
		kerndbg( KERN_PANIC, "%s: Failed to re-initialize device, "
		       "aborting.\n", tp->dev->name);
		tg3_halt(tp, RESET_KIND_SHUTDOWN, 1);
		tg3_full_unlock(tp);
		delete_timer(tp->timer);
		tp->irq_sync = 0;
		tg3_close(tp->dev);
		tg3_full_lock(tp, 0);
	}
	return err;
}

static void tg3_timer(void *data);

static void tg3_reset_task(struct tg3 *tp)
{
	unsigned int restart_timer;

	kerndbg( KERN_DEBUG, "%s\n", __FUNCTION__ );

	tg3_full_lock(tp, 0);
	tp->tg3_flags |= TG3_FLAG_IN_RESET_TASK;

	if (!netif_running(tp->dev)) {
		tp->tg3_flags &= ~TG3_FLAG_IN_RESET_TASK;
		tg3_full_unlock(tp);
		return;
	}

	tg3_full_unlock(tp);

	tg3_netif_stop(tp);

	tg3_full_lock(tp, 1);

	restart_timer = tp->tg3_flags2 & TG3_FLG2_RESTART_TIMER;
	tp->tg3_flags2 &= ~TG3_FLG2_RESTART_TIMER;

	if (tp->tg3_flags & TG3_FLAG_TX_RECOVERY_PENDING) {
		tp->write32_tx_mbox = tg3_write32_tx_mbox;
		tp->write32_rx_mbox = tg3_write_flush_reg32;
		tp->tg3_flags |= TG3_FLAG_MBOX_WRITE_REORDER;
		tp->tg3_flags &= ~TG3_FLAG_TX_RECOVERY_PENDING;
	}

	tg3_halt(tp, RESET_KIND_SHUTDOWN, 0);
	if (tg3_init_hw(tp, 1))
		goto out;

	tg3_netif_start(tp);

	if (restart_timer)
	{
		delete_timer( tp->timer );
		tp->timer = create_timer();
		start_timer(tp->timer, (timer_callback *) &tg3_timer, tp, jiffies + 1, true );
	}

out:
	tp->tg3_flags &= ~TG3_FLAG_IN_RESET_TASK;

	tg3_full_unlock(tp);
}

/* Device interface */
static int tg3_open(struct net_device *dev);
static int tg3_start_xmit(PacketBuf_s *skb, struct net_device *dev);

static status_t tg3_dev_open( void* pNode, uint32 nFlags, void **pCookie )
{
	return 0;
}

static status_t tg3_dev_close( void* pNode, void* pCookie )
{
	return 0;
}

static status_t tg3_dev_ioctl( void* pNode, void* pCookie, uint32 nCommand, void* pArgs, bool bFromKernel )
{
	struct net_device* psNetDev = pNode;
    int nError = 0;

    switch( nCommand )
    {
		case SIOC_ETH_START:
		{
			psNetDev->packet_queue = pArgs;
			tg3_open( psNetDev );
			break;
		}

		case SIOC_ETH_STOP:
		{
			tg3_close( psNetDev );
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
			kerndbg( KERN_WARNING, "tg3_dev_ioctl(): Unknown command %d\n", (int)nCommand );
			nError = -ENOSYS;
			break;
		}
    }

    return nError;
}

static int tg3_dev_write( void* pNode, void* pCookie, off_t nPosition, const void* pBuffer, size_t nSize )
{
	struct net_device* psNetDev = pNode;
	PacketBuf_s* psBuffer = alloc_pkt_buffer( nSize );
	if ( psBuffer != NULL )
	{
		memcpy( psBuffer->pb_pData, pBuffer, nSize );
		psBuffer->pb_nSize = nSize;
		tg3_start_xmit( psBuffer, psNetDev );
	}
	return nSize;
}

static DeviceOperations_s g_sDevOps = {
	tg3_dev_open,
	tg3_dev_close,
	tg3_dev_ioctl,
	NULL,	/* dop_read */
	tg3_dev_write,
	NULL,	/* dop_readv */
	NULL,	/* dop_writev */
	NULL,	/* dop_add_select_req */
	NULL	/* dop_rem_select_req */
};

/* Test for DMA buffers crossing any 4GB boundaries: 4G, 8G, etc */
static inline int tg3_4g_overflow_test(dma_addr_t mapping, int len)
{
	u32 base = (u32) mapping & 0xffffffff;

	return ((base > 0xffffdcc0) &&
		(base + len + 8 < base));
}

/* Test for DMA addresses > 40-bit */
static inline int tg3_40bit_overflow_test(struct tg3 *tp, dma_addr_t mapping,
					  int len)
{
	return 0;
}

static void tg3_set_txd(struct tg3 *, int, dma_addr_t, int, u32, u32);

/* Workaround 4GB and 40-bit hardware DMA bugs. */
static int tigon3_dma_hwbug_workaround(struct tg3 *tp, PacketBuf_s *skb,
				       u32 last_plus_one, u32 *start,
				       u32 base_flags, u32 mss)
{
	PacketBuf_s *new_skb = alloc_pkt_buffer(sizeof(skb));
	dma_addr_t new_addr = 0;
	u32 entry = *start;
	int i, ret = 0;

	/* Clone the skb */
	memcpy(new_skb, skb, sizeof(skb));

	if (!new_skb) {
		ret = -1;
	} else {
		/* New SKB is guaranteed to be linear. */
		entry = *start;
		new_addr = pci_map_single(tp->pdev, new_skb->pb_pData, new_skb->pb_nSize,
					  PCI_DMA_TODEVICE);
		/* Make sure new skb does not cross any 4G boundaries.
		 * Drop the packet if it does.
		 */
		if (tg3_4g_overflow_test(new_addr, new_skb->pb_nSize)) {
			ret = -1;
			free_pkt_buffer(new_skb);
			new_skb = NULL;
		} else {
			tg3_set_txd(tp, entry, new_addr, new_skb->pb_nSize,
				    base_flags, 1 | (mss << 1));
			*start = NEXT_TX(entry);
		}
	}

	/* Now clean up the sw ring entries. */
	i = 0;
	while (entry != last_plus_one) {
		int len;

		if (i == 0)
			len = skb->pb_nSize;
		else
			len = skb->pb_nSize;
		pci_unmap_single(tp->pdev,
				 pci_unmap_addr(&tp->tx_buffers[entry], mapping),
				 len, PCI_DMA_TODEVICE);
		if (i == 0) {
			tp->tx_buffers[entry].skb = new_skb;
			//pci_unmap_addr_set(&tp->tx_buffers[entry], mapping, new_addr);
		} else {
			tp->tx_buffers[entry].skb = NULL;
		}
		entry = NEXT_TX(entry);
		i++;
	}

	free_pkt_buffer(skb);

	return ret;
}

static void tg3_set_txd(struct tg3 *tp, int entry,
			dma_addr_t mapping, int len, u32 flags,
			u32 mss_and_is_end)
{
	struct tg3_tx_buffer_desc *txd = &tp->tx_ring[entry];
	int is_end = (mss_and_is_end & 0x1);
	u32 mss = (mss_and_is_end >> 1);
	u32 vlan_tag = 0;

	if (is_end)
		flags |= TXD_FLAG_END;
	if (flags & TXD_FLAG_VLAN) {
		vlan_tag = flags >> 16;
		flags &= 0xffff;
	}
	vlan_tag |= (mss << TXD_MSS_SHIFT);

	txd->addr_hi = ((u64) mapping >> 32);
	txd->addr_lo = ((u64) mapping & 0xffffffff);
	txd->len_flags = (len << TXD_LEN_SHIFT) | flags;
	txd->vlan_tag = vlan_tag << TXD_VLAN_TAG_SHIFT;
}

/* hard_start_xmit for devices that don't have any bugs and
 * support TG3_FLG2_HW_TSO_2 only.
 */
static int tg3_start_xmit(PacketBuf_s *skb, struct net_device *dev)
{
	struct tg3 *tp = netdev_priv(dev);
	u32 len, entry;

	kerndbg( KERN_DEBUG, "%s(): ENTER\n", __FUNCTION__ );

	len = skb->pb_nSize;
	entry = tp->tx_prod;

	tp->tx_buffers[entry].skb = skb;
	tg3_set_txd(tp, entry, (dma_addr_t)skb->pb_pData, len, 0, 1 );

	entry = NEXT_TX(entry);

	/* Packets are ready, update Tx producer idx local and on card. */
	tw32_tx_mbox((MAILBOX_SNDHOST_PROD_IDX_0 + TG3_64BIT_REG_LOW), entry);
	tp->tx_prod = entry;

out_unlock:
	smp_wmb();

	dev->trans_start = jiffies;

	return 0;
}

/* hard_start_xmit for devices that have the 4G bug and/or 40-bit bug and
 * support TG3_FLG2_HW_TSO_1 or firmware TSO only.
 */
static int tg3_start_xmit_dma_bug(PacketBuf_s *skb, struct net_device *dev)
{
	struct tg3 *tp = netdev_priv(dev);
	dma_addr_t mapping;
	u32 len, entry;
	int would_hit_hwbug;

	kerndbg( KERN_DEBUG, "%s(): ENTER\n", __FUNCTION__ );

	len = skb->pb_nSize;
	entry = tp->tx_prod;

	/* Queue skb data, a.k.a. the main skb fragment. */
	mapping = (dma_addr_t)skb->pb_pData;

	tp->tx_buffers[entry].skb = skb;

	would_hit_hwbug = 0;

	if (tg3_4g_overflow_test(mapping, len))
		would_hit_hwbug = 1;

	tg3_set_txd(tp, entry, mapping, len, 0, 1 );

	entry = NEXT_TX(entry);

	if (would_hit_hwbug) {
		u32 last_plus_one = entry;
		u32 start;

		start = entry;
		start &= (TG3_TX_RING_SIZE - 1);

		/* If the workaround fails due to memory/mapping
		 * failure, silently drop this packet.
		 */
		if (tigon3_dma_hwbug_workaround(tp, skb, last_plus_one,
						&start,0, 0))
			goto out_unlock;

		entry = start;
	}

	/* Packets are ready, update Tx producer idx local and on card. */
	tw32_tx_mbox((MAILBOX_SNDHOST_PROD_IDX_0 + TG3_64BIT_REG_LOW), entry);

	tp->tx_prod = entry;

out_unlock:
	smp_wmb();

	dev->trans_start = jiffies;

	return 0;
}

/* Free up pending packets in all rx/tx rings.
 *
 * The chip has been shut down and the driver detached from
 * the networking, so no interrupts or new tx packets will
 * end up in the driver.  tp->{tx,}lock is not held and we are not
 * in an interrupt context and thus may sleep.
 */
static void tg3_free_rings(struct tg3 *tp)
{
	struct ring_info *rxp;
	int i;

	for (i = 0; i < TG3_RX_RING_SIZE; i++) {
		rxp = &tp->rx_std_buffers[i];

		if (rxp->skb == NULL)
			continue;
		pci_unmap_single(tp->pdev,
				 pci_unmap_addr(rxp, mapping),
				 tp->rx_pkt_buf_sz - tp->rx_offset,
				 PCI_DMA_FROMDEVICE);
		free_pkt_buffer(rxp->skb);
		rxp->skb = NULL;
	}

	for (i = 0; i < TG3_RX_JUMBO_RING_SIZE; i++) {
		rxp = &tp->rx_jumbo_buffers[i];

		if (rxp->skb == NULL)
			continue;
		pci_unmap_single(tp->pdev,
				 pci_unmap_addr(rxp, mapping),
				 RX_JUMBO_PKT_BUF_SZ - tp->rx_offset,
				 PCI_DMA_FROMDEVICE);
		free_pkt_buffer(rxp->skb);
		rxp->skb = NULL;
	}

	for (i = 0; i < TG3_TX_RING_SIZE; ) {
		struct tx_ring_info *txp;
		PacketBuf_s *skb;
		int j;

		txp = &tp->tx_buffers[i];
		skb = txp->skb;

		if (skb == NULL) {
			i++;
			continue;
		}

		pci_unmap_single(tp->pdev,
				 pci_unmap_addr(txp, mapping),
				 skb_headlen(skb),
				 PCI_DMA_TODEVICE);
		txp->skb = NULL;

		i++;

		free_pkt_buffer(skb);
	}
}

/* Initialize tx/rx rings for packet processing.
 *
 * The chip has been shut down and the driver detached from
 * the networking, so no interrupts or new tx packets will
 * end up in the driver.  tp->{tx,}lock are held and thus
 * we may not sleep.
 */
static int tg3_init_rings(struct tg3 *tp)
{
	u32 i;

	/* Free up all the SKBs. */
	tg3_free_rings(tp);

	/* Zero out all descriptors. */
	memset(tp->rx_std, 0, TG3_RX_RING_BYTES);
	memset(tp->rx_jumbo, 0, TG3_RX_JUMBO_RING_BYTES);
	memset(tp->rx_rcb, 0, TG3_RX_RCB_RING_BYTES(tp));
	memset(tp->tx_ring, 0, TG3_TX_RING_BYTES);

	tp->rx_pkt_buf_sz = RX_PKT_BUF_SZ;
	if ((tp->tg3_flags2 & TG3_FLG2_5780_CLASS) &&
	    (tp->dev->mtu > ETH_DATA_LEN))
		tp->rx_pkt_buf_sz = RX_JUMBO_PKT_BUF_SZ;

	/* Initialize invariants of the rings, we only set this
	 * stuff once.  This works because the card does not
	 * write into the rx buffer posting rings.
	 */
	for (i = 0; i < TG3_RX_RING_SIZE; i++) {
		struct tg3_rx_buffer_desc *rxd;

		rxd = &tp->rx_std[i];
		rxd->idx_len = (tp->rx_pkt_buf_sz - tp->rx_offset - 64)
			<< RXD_LEN_SHIFT;
		rxd->type_flags = (RXD_FLAG_END << RXD_FLAGS_SHIFT);
		rxd->opaque = (RXD_OPAQUE_RING_STD |
			       (i << RXD_OPAQUE_INDEX_SHIFT));
	}

	if (tp->tg3_flags & TG3_FLAG_JUMBO_RING_ENABLE) {
		for (i = 0; i < TG3_RX_JUMBO_RING_SIZE; i++) {
			struct tg3_rx_buffer_desc *rxd;

			rxd = &tp->rx_jumbo[i];
			rxd->idx_len = (RX_JUMBO_PKT_BUF_SZ - tp->rx_offset - 64)
				<< RXD_LEN_SHIFT;
			rxd->type_flags = (RXD_FLAG_END << RXD_FLAGS_SHIFT) |
				RXD_FLAG_JUMBO;
			rxd->opaque = (RXD_OPAQUE_RING_JUMBO |
			       (i << RXD_OPAQUE_INDEX_SHIFT));
		}
	}

	/* Now allocate fresh SKBs for each rx ring. */
	for (i = 0; i < tp->rx_pending; i++) {
		if (tg3_alloc_rx_skb(tp, RXD_OPAQUE_RING_STD, -1, i) < 0) {
			kerndbg( KERN_WARNING,
			       "%s: Using a smaller RX standard ring, "
			       "only %d out of %d buffers were allocated "
			       "successfully.\n",
			       tp->dev->name, i, tp->rx_pending);
			if (i == 0)
				return -ENOMEM;
			tp->rx_pending = i;
			break;
		}
	}

	if (tp->tg3_flags & TG3_FLAG_JUMBO_RING_ENABLE) {
		for (i = 0; i < tp->rx_jumbo_pending; i++) {
			if (tg3_alloc_rx_skb(tp, RXD_OPAQUE_RING_JUMBO,
					     -1, i) < 0) {
				kerndbg( KERN_WARNING,
				       "%s: Using a smaller RX jumbo ring, "
				       "only %d out of %d buffers were "
				       "allocated successfully.\n",
				       tp->dev->name, i, tp->rx_jumbo_pending);
				if (i == 0) {
					tg3_free_rings(tp);
					return -ENOMEM;
				}
				tp->rx_jumbo_pending = i;
				break;
			}
		}
	}
	return 0;
}

/*
 * Must not be invoked with interrupt sources disabled and
 * the hardware shutdown down.
 */
static void tg3_free_consistent(struct tg3 *tp)
{
	kfree(tp->rx_std_buffers);
	tp->rx_std_buffers = NULL;
	if (tp->rx_std) {
		pci_free_consistent(tp->pdev, TG3_RX_RING_BYTES,
				    tp->rx_std, tp->rx_std_mapping);
		tp->rx_std = NULL;
	}
	if (tp->rx_jumbo) {
		pci_free_consistent(tp->pdev, TG3_RX_JUMBO_RING_BYTES,
				    tp->rx_jumbo, tp->rx_jumbo_mapping);
		tp->rx_jumbo = NULL;
	}
	if (tp->rx_rcb) {
		pci_free_consistent(tp->pdev, TG3_RX_RCB_RING_BYTES(tp),
				    tp->rx_rcb, tp->rx_rcb_mapping);
		tp->rx_rcb = NULL;
	}
	if (tp->tx_ring) {
		pci_free_consistent(tp->pdev, TG3_TX_RING_BYTES,
			tp->tx_ring, tp->tx_desc_mapping);
		tp->tx_ring = NULL;
	}
	if (tp->hw_status) {
		pci_free_consistent(tp->pdev, TG3_HW_STATUS_SIZE,
				    tp->hw_status, tp->status_mapping);
		tp->hw_status = NULL;
	}
	if (tp->hw_stats) {
		pci_free_consistent(tp->pdev, sizeof(struct tg3_hw_stats),
				    tp->hw_stats, tp->stats_mapping);
		tp->hw_stats = NULL;
	}
}

/*
 * Must not be invoked with interrupt sources disabled and
 * the hardware shutdown down.  Can sleep.
 */
static int tg3_alloc_consistent(struct tg3 *tp)
{
	tp->rx_std_buffers = kmalloc((sizeof(struct ring_info) *
				      (TG3_RX_RING_SIZE +
				       TG3_RX_JUMBO_RING_SIZE)) +
				     (sizeof(struct tx_ring_info) *
				      TG3_TX_RING_SIZE),
				     MEMF_KERNEL);
	if (!tp->rx_std_buffers)
		return -ENOMEM;

	memset(tp->rx_std_buffers, 0,
	       (sizeof(struct ring_info) *
		(TG3_RX_RING_SIZE +
		 TG3_RX_JUMBO_RING_SIZE)) +
	       (sizeof(struct tx_ring_info) *
		TG3_TX_RING_SIZE));

	tp->rx_jumbo_buffers = &tp->rx_std_buffers[TG3_RX_RING_SIZE];
	tp->tx_buffers = (struct tx_ring_info *)
		&tp->rx_jumbo_buffers[TG3_RX_JUMBO_RING_SIZE];

	tp->rx_std = pci_alloc_consistent(tp->pdev, TG3_RX_RING_BYTES,
					  &tp->rx_std_mapping);
	if (!tp->rx_std)
		goto err_out;

	tp->rx_jumbo = pci_alloc_consistent(tp->pdev, TG3_RX_JUMBO_RING_BYTES,
					    &tp->rx_jumbo_mapping);

	if (!tp->rx_jumbo)
		goto err_out;

	tp->rx_rcb = pci_alloc_consistent(tp->pdev, TG3_RX_RCB_RING_BYTES(tp),
					  &tp->rx_rcb_mapping);
	if (!tp->rx_rcb)
		goto err_out;

	tp->tx_ring = pci_alloc_consistent(tp->pdev, TG3_TX_RING_BYTES,
					   &tp->tx_desc_mapping);
	if (!tp->tx_ring)
		goto err_out;

	tp->hw_status = pci_alloc_consistent(tp->pdev,
					     TG3_HW_STATUS_SIZE,
					     &tp->status_mapping);
	if (!tp->hw_status)
		goto err_out;

	tp->hw_stats = pci_alloc_consistent(tp->pdev,
					    sizeof(struct tg3_hw_stats),
					    &tp->stats_mapping);
	if (!tp->hw_stats)
		goto err_out;

	memset(tp->hw_status, 0, TG3_HW_STATUS_SIZE);
	memset(tp->hw_stats, 0, sizeof(struct tg3_hw_stats));

	return 0;

err_out:
	tg3_free_consistent(tp);
	return -ENOMEM;
}

#define MAX_WAIT_CNT 1000

/* To stop a block, clear the enable bit and poll till it
 * clears.  tp->lock is held.
 */
static int tg3_stop_block(struct tg3 *tp, unsigned long ofs, u32 enable_bit, int silent)
{
	unsigned int i;
	u32 val;

	if (tp->tg3_flags2 & TG3_FLG2_5705_PLUS) {
		switch (ofs) {
		case RCVLSC_MODE:
		case DMAC_MODE:
		case MBFREE_MODE:
		case BUFMGR_MODE:
		case MEMARB_MODE:
			/* We can't enable/disable these bits of the
			 * 5705/5750, just say success.
			 */
			return 0;

		default:
			break;
		};
	}

	val = tr32(ofs);
	val &= ~enable_bit;
	tw32_f(ofs, val);

	for (i = 0; i < MAX_WAIT_CNT; i++) {
		udelay(100);
		val = tr32(ofs);
		if ((val & enable_bit) == 0)
			break;
	}

	if (i == MAX_WAIT_CNT && !silent) {
		kerndbg( KERN_WARNING, "tg3_stop_block timed out, "
		       "ofs=%lx enable_bit=%x\n",
		       ofs, enable_bit);
		return -ENODEV;
	}

	return 0;
}

/* tp->lock is held. */
static int tg3_abort_hw(struct tg3 *tp, int silent)
{
	int i, err;

	tg3_disable_ints(tp);

	tp->rx_mode &= ~RX_MODE_ENABLE;
	tw32_f(MAC_RX_MODE, tp->rx_mode);
	udelay(10);

	err  = tg3_stop_block(tp, RCVBDI_MODE, RCVBDI_MODE_ENABLE, silent);
	err |= tg3_stop_block(tp, RCVLPC_MODE, RCVLPC_MODE_ENABLE, silent);
	err |= tg3_stop_block(tp, RCVLSC_MODE, RCVLSC_MODE_ENABLE, silent);
	err |= tg3_stop_block(tp, RCVDBDI_MODE, RCVDBDI_MODE_ENABLE, silent);
	err |= tg3_stop_block(tp, RCVDCC_MODE, RCVDCC_MODE_ENABLE, silent);
	err |= tg3_stop_block(tp, RCVCC_MODE, RCVCC_MODE_ENABLE, silent);

	err |= tg3_stop_block(tp, SNDBDS_MODE, SNDBDS_MODE_ENABLE, silent);
	err |= tg3_stop_block(tp, SNDBDI_MODE, SNDBDI_MODE_ENABLE, silent);
	err |= tg3_stop_block(tp, SNDDATAI_MODE, SNDDATAI_MODE_ENABLE, silent);
	err |= tg3_stop_block(tp, RDMAC_MODE, RDMAC_MODE_ENABLE, silent);
	err |= tg3_stop_block(tp, SNDDATAC_MODE, SNDDATAC_MODE_ENABLE, silent);
	err |= tg3_stop_block(tp, DMAC_MODE, DMAC_MODE_ENABLE, silent);
	err |= tg3_stop_block(tp, SNDBDC_MODE, SNDBDC_MODE_ENABLE, silent);

	tp->mac_mode &= ~MAC_MODE_TDE_ENABLE;
	tw32_f(MAC_MODE, tp->mac_mode);
	udelay(40);

	tp->tx_mode &= ~TX_MODE_ENABLE;
	tw32_f(MAC_TX_MODE, tp->tx_mode);

	for (i = 0; i < MAX_WAIT_CNT; i++) {
		udelay(100);
		if (!(tr32(MAC_TX_MODE) & TX_MODE_ENABLE))
			break;
	}
	if (i >= MAX_WAIT_CNT) {
		kerndbg( KERN_WARNING, "tg3_abort_hw timed out for %s, "
		       "TX_MODE_ENABLE will not clear MAC_TX_MODE=%08x\n",
		       tp->dev->name, tr32(MAC_TX_MODE));
		err |= -ENODEV;
	}

	err |= tg3_stop_block(tp, HOSTCC_MODE, HOSTCC_MODE_ENABLE, silent);
	err |= tg3_stop_block(tp, WDMAC_MODE, WDMAC_MODE_ENABLE, silent);
	err |= tg3_stop_block(tp, MBFREE_MODE, MBFREE_MODE_ENABLE, silent);

	tw32(FTQ_RESET, 0xffffffff);
	tw32(FTQ_RESET, 0x00000000);

	err |= tg3_stop_block(tp, BUFMGR_MODE, BUFMGR_MODE_ENABLE, silent);
	err |= tg3_stop_block(tp, MEMARB_MODE, MEMARB_MODE_ENABLE, silent);

	if (tp->hw_status)
		memset(tp->hw_status, 0, TG3_HW_STATUS_SIZE);
	if (tp->hw_stats)
		memset(tp->hw_stats, 0, sizeof(struct tg3_hw_stats));

	return err;
}

/* tp->lock is held. */
static int tg3_nvram_lock(struct tg3 *tp)
{
	if (tp->tg3_flags & TG3_FLAG_NVRAM) {
		int i;

		if (tp->nvram_lock_cnt == 0) {
			tw32(NVRAM_SWARB, SWARB_REQ_SET1);
			for (i = 0; i < 8000; i++) {
				if (tr32(NVRAM_SWARB) & SWARB_GNT1)
					break;
				udelay(20);
			}
			if (i == 8000) {
				tw32(NVRAM_SWARB, SWARB_REQ_CLR1);
				return -ENODEV;
			}
		}
		tp->nvram_lock_cnt++;
	}
	return 0;
}

/* tp->lock is held. */
static void tg3_nvram_unlock(struct tg3 *tp)
{
	if (tp->tg3_flags & TG3_FLAG_NVRAM) {
		if (tp->nvram_lock_cnt > 0)
			tp->nvram_lock_cnt--;
		if (tp->nvram_lock_cnt == 0)
			tw32_f(NVRAM_SWARB, SWARB_REQ_CLR1);
	}
}

/* tp->lock is held. */
static void tg3_enable_nvram_access(struct tg3 *tp)
{
	if ((tp->tg3_flags2 & TG3_FLG2_5750_PLUS) &&
	    !(tp->tg3_flags2 & TG3_FLG2_PROTECTED_NVRAM)) {
		u32 nvaccess = tr32(NVRAM_ACCESS);

		tw32(NVRAM_ACCESS, nvaccess | ACCESS_ENABLE);
	}
}

/* tp->lock is held. */
static void tg3_disable_nvram_access(struct tg3 *tp)
{
	if ((tp->tg3_flags2 & TG3_FLG2_5750_PLUS) &&
	    !(tp->tg3_flags2 & TG3_FLG2_PROTECTED_NVRAM)) {
		u32 nvaccess = tr32(NVRAM_ACCESS);

		tw32(NVRAM_ACCESS, nvaccess & ~ACCESS_ENABLE);
	}
}

/* tp->lock is held. */
static void tg3_write_sig_pre_reset(struct tg3 *tp, int kind)
{
	tg3_write_mem(tp, NIC_SRAM_FIRMWARE_MBOX,
		      NIC_SRAM_FIRMWARE_MBOX_MAGIC1);

	if (tp->tg3_flags2 & TG3_FLG2_ASF_NEW_HANDSHAKE) {
		switch (kind) {
		case RESET_KIND_INIT:
			tg3_write_mem(tp, NIC_SRAM_FW_DRV_STATE_MBOX,
				      DRV_STATE_START);
			break;

		case RESET_KIND_SHUTDOWN:
			tg3_write_mem(tp, NIC_SRAM_FW_DRV_STATE_MBOX,
				      DRV_STATE_UNLOAD);
			break;

		case RESET_KIND_SUSPEND:
			tg3_write_mem(tp, NIC_SRAM_FW_DRV_STATE_MBOX,
				      DRV_STATE_SUSPEND);
			break;

		default:
			break;
		};
	}
}

/* tp->lock is held. */
static void tg3_write_sig_post_reset(struct tg3 *tp, int kind)
{
	if (tp->tg3_flags2 & TG3_FLG2_ASF_NEW_HANDSHAKE) {
		switch (kind) {
		case RESET_KIND_INIT:
			tg3_write_mem(tp, NIC_SRAM_FW_DRV_STATE_MBOX,
				      DRV_STATE_START_DONE);
			break;

		case RESET_KIND_SHUTDOWN:
			tg3_write_mem(tp, NIC_SRAM_FW_DRV_STATE_MBOX,
				      DRV_STATE_UNLOAD_DONE);
			break;

		default:
			break;
		};
	}
}

/* tp->lock is held. */
static void tg3_write_sig_legacy(struct tg3 *tp, int kind)
{
	if (tp->tg3_flags & TG3_FLAG_ENABLE_ASF) {
		switch (kind) {
		case RESET_KIND_INIT:
			tg3_write_mem(tp, NIC_SRAM_FW_DRV_STATE_MBOX,
				      DRV_STATE_START);
			break;

		case RESET_KIND_SHUTDOWN:
			tg3_write_mem(tp, NIC_SRAM_FW_DRV_STATE_MBOX,
				      DRV_STATE_UNLOAD);
			break;

		case RESET_KIND_SUSPEND:
			tg3_write_mem(tp, NIC_SRAM_FW_DRV_STATE_MBOX,
				      DRV_STATE_SUSPEND);
			break;

		default:
			break;
		};
	}
}

static void tg3_stop_fw(struct tg3 *);

/* tp->lock is held. */
static int tg3_chip_reset(struct tg3 *tp)
{
	u32 val;
	void (*write_op)(struct tg3 *, u32, u32);
	int i;

	tg3_nvram_lock(tp);

	/* No matching tg3_nvram_unlock() after this because
	 * chip reset below will undo the nvram lock.
	 */
	tp->nvram_lock_cnt = 0;

	if (GET_ASIC_REV(tp->pci_chip_rev_id) == ASIC_REV_5752 ||
	    GET_ASIC_REV(tp->pci_chip_rev_id) == ASIC_REV_5755 ||
	    GET_ASIC_REV(tp->pci_chip_rev_id) == ASIC_REV_5787)
		tw32(GRC_FASTBOOT_PC, 0);

	/*
	 * We must avoid the readl() that normally takes place.
	 * It locks machines, causes machine checks, and other
	 * fun things.  So, temporarily disable the 5701
	 * hardware workaround, while we do the reset.
	 */
	write_op = tp->write32;
	if (write_op == tg3_write_flush_reg32)
		tp->write32 = tg3_write32;

	/* do the reset */
	val = GRC_MISC_CFG_CORECLK_RESET;

	if (tp->tg3_flags2 & TG3_FLG2_PCI_EXPRESS) {
		if (tr32(0x7e2c) == 0x60) {
			tw32(0x7e2c, 0x20);
		}
		if (tp->pci_chip_rev_id != CHIPREV_ID_5750_A0) {
			tw32(GRC_MISC_CFG, (1 << 29));
			val |= (1 << 29);
		}
	}

	if (tp->tg3_flags2 & TG3_FLG2_5705_PLUS)
		val |= GRC_MISC_CFG_KEEP_GPHY_POWER;
	tw32(GRC_MISC_CFG, val);

	/* restore 5701 hardware bug workaround write method */
	tp->write32 = write_op;

	/* Unfortunately, we have to delay before the PCI read back.
	 * Some 575X chips even will not respond to a PCI cfg access
	 * when the reset command is given to the chip.
	 *
	 * How do these hardware designers expect things to work
	 * properly if the PCI write is posted for a long period
	 * of time?  It is always necessary to have some method by
	 * which a register read back can occur to push the write
	 * out which does the reset.
	 *
	 * For most tg3 variants the trick below was working.
	 * Ho hum...
	 */
	udelay(120);

	/* Flush PCI posted writes.  The normal MMIO registers
	 * are inaccessible at this time so this is the only
	 * way to make this reliably (actually, this is no longer
	 * the case, see above).  I tried to use indirect
	 * register read/write but this upset some 5701 variants.
	 */
	val = g_psBus->read_pci_config(tp->pdev->nBus, tp->pdev->nDevice, tp->pdev->nFunction, PCI_COMMAND, 4);

	udelay(120);

	if (tp->tg3_flags2 & TG3_FLG2_PCI_EXPRESS) {
		if (tp->pci_chip_rev_id == CHIPREV_ID_5750_A0) {
			int i;
			u32 cfg_val;

			/* Wait for link training to complete.  */
			for (i = 0; i < 5000; i++)
				udelay(100);

			cfg_val = g_psBus->read_pci_config(tp->pdev->nBus, tp->pdev->nDevice, tp->pdev->nFunction, 0xc4, 4);
			g_psBus->write_pci_config(tp->pdev->nBus, tp->pdev->nDevice, tp->pdev->nFunction, 0xc4,
								4, cfg_val | (1 << 15));

		}
		/* Set PCIE max payload size and clear error status.  */
		g_psBus->write_pci_config(tp->pdev->nBus, tp->pdev->nDevice, tp->pdev->nFunction, 0xd8, 4, 0xf5000);
	}

	/* Re-enable indirect register accesses. */
	g_psBus->write_pci_config(tp->pdev->nBus, tp->pdev->nDevice, tp->pdev->nFunction, TG3PCI_MISC_HOST_CTRL,
					4, tp->misc_host_ctrl);

	/* Set MAX PCI retry to zero. */
	val = (PCISTATE_ROM_ENABLE | PCISTATE_ROM_RETRY_ENABLE);
	if (tp->pci_chip_rev_id == CHIPREV_ID_5704_A0 &&
	    (tp->tg3_flags & TG3_FLAG_PCIX_MODE))
		val |= PCISTATE_RETRY_SAME_DMA;
	g_psBus->write_pci_config(tp->pdev->nBus, tp->pdev->nDevice, tp->pdev->nFunction, TG3PCI_PCISTATE, 4, val);

	pci_restore_state(tp->pdev, tp->pci_state);

	/* Make sure PCI-X relaxed ordering bit is clear. */
	val = g_psBus->read_pci_config(tp->pdev->nBus, tp->pdev->nDevice, tp->pdev->nFunction, TG3PCI_X_CAPS, 4);
	val &= ~PCIX_CAPS_RELAXED_ORDERING;
	g_psBus->write_pci_config(tp->pdev->nBus, tp->pdev->nDevice, tp->pdev->nFunction, TG3PCI_X_CAPS, 4, val);

	if (tp->tg3_flags2 & TG3_FLG2_5780_CLASS) {
		u32 val;

		/* Chip reset on 5780 will reset MSI enable bit,
		 * so need to restore it.
		 */
		if (tp->tg3_flags2 & TG3_FLG2_USING_MSI) {
			u16 ctrl;

			ctrl = g_psBus->read_pci_config(tp->pdev->nBus, tp->pdev->nDevice, tp->pdev->nFunction,
							tp->msi_cap + PCI_MSI_FLAGS, 2);
			g_psBus->write_pci_config(tp->pdev->nBus, tp->pdev->nDevice, tp->pdev->nFunction,
							tp->msi_cap + PCI_MSI_FLAGS, 2,
							ctrl | PCI_MSI_FLAGS_ENABLE);
			val = tr32(MSGINT_MODE);
			tw32(MSGINT_MODE, val | MSGINT_MODE_ENABLE);
		}

		val = tr32(MEMARB_MODE);
		tw32(MEMARB_MODE, val | MEMARB_MODE_ENABLE);

	} else
		tw32(MEMARB_MODE, MEMARB_MODE_ENABLE);

	if (tp->pci_chip_rev_id == CHIPREV_ID_5750_A3) {
		tg3_stop_fw(tp);
		tw32(0x5000, 0x400);
	}

	tw32(GRC_MODE, tp->grc_mode);

	if (tp->pci_chip_rev_id == CHIPREV_ID_5705_A0) {
		u32 val = tr32(0xc4);

		tw32(0xc4, val | (1 << 15));
	}

	if ((tp->nic_sram_data_cfg & NIC_SRAM_DATA_CFG_MINI_PCI) != 0 &&
	    GET_ASIC_REV(tp->pci_chip_rev_id) == ASIC_REV_5705) {
		tp->pci_clock_ctrl |= CLOCK_CTRL_CLKRUN_OENABLE;
		if (tp->pci_chip_rev_id == CHIPREV_ID_5705_A0)
			tp->pci_clock_ctrl |= CLOCK_CTRL_FORCE_CLKRUN;
		tw32(TG3PCI_CLOCK_CTRL, tp->pci_clock_ctrl);
	}

	if (tp->tg3_flags2 & TG3_FLG2_PHY_SERDES) {
		tp->mac_mode = MAC_MODE_PORT_MODE_TBI;
		tw32_f(MAC_MODE, tp->mac_mode);
	} else if (tp->tg3_flags2 & TG3_FLG2_MII_SERDES) {
		tp->mac_mode = MAC_MODE_PORT_MODE_GMII;
		tw32_f(MAC_MODE, tp->mac_mode);
	} else
		tw32_f(MAC_MODE, 0);
	udelay(40);

	/* Wait for firmware initialization to complete. */
	for (i = 0; i < 100000; i++) {
		tg3_read_mem(tp, NIC_SRAM_FIRMWARE_MBOX, &val);
		if (val == ~NIC_SRAM_FIRMWARE_MBOX_MAGIC1)
			break;
		udelay(10);
	}

	/* Chip might not be fitted with firmare.  Some Sun onboard
	 * parts are configured like that.  So don't signal the timeout
	 * of the above loop as an error, but do report the lack of
	 * running firmware once.
	 */
	if (i >= 100000 &&
	    !(tp->tg3_flags2 & TG3_FLG2_NO_FWARE_REPORTED)) {
		tp->tg3_flags2 |= TG3_FLG2_NO_FWARE_REPORTED;

		kerndbg( KERN_INFO, "%s: No firmware running.\n",
		       tp->dev->name);
	}

	if ((tp->tg3_flags2 & TG3_FLG2_PCI_EXPRESS) &&
	    tp->pci_chip_rev_id != CHIPREV_ID_5750_A0) {
		u32 val = tr32(0x7c00);

		tw32(0x7c00, val | (1 << 25));
	}

	/* Reprobe ASF enable state.  */
	tp->tg3_flags &= ~TG3_FLAG_ENABLE_ASF;
	tp->tg3_flags2 &= ~TG3_FLG2_ASF_NEW_HANDSHAKE;
	tg3_read_mem(tp, NIC_SRAM_DATA_SIG, &val);
	if (val == NIC_SRAM_DATA_SIG_MAGIC) {
		u32 nic_cfg;

		tg3_read_mem(tp, NIC_SRAM_DATA_CFG, &nic_cfg);
		if (nic_cfg & NIC_SRAM_DATA_CFG_ASF_ENABLE) {
			tp->tg3_flags |= TG3_FLAG_ENABLE_ASF;
			if (tp->tg3_flags2 & TG3_FLG2_5750_PLUS)
				tp->tg3_flags2 |= TG3_FLG2_ASF_NEW_HANDSHAKE;
		}
	}

	return 0;
}

/* tp->lock is held. */
static void tg3_stop_fw(struct tg3 *tp)
{
	if (tp->tg3_flags & TG3_FLAG_ENABLE_ASF) {
		u32 val;
		int i;

		tg3_write_mem(tp, NIC_SRAM_FW_CMD_MBOX, FWCMD_NICDRV_PAUSE_FW);
		val = tr32(GRC_RX_CPU_EVENT);
		val |= (1 << 14);
		tw32(GRC_RX_CPU_EVENT, val);

		/* Wait for RX cpu to ACK the event.  */
		for (i = 0; i < 100; i++) {
			if (!(tr32(GRC_RX_CPU_EVENT) & (1 << 14)))
				break;
			udelay(1);
		}
	}
}

/* tp->lock is held. */
static int tg3_halt(struct tg3 *tp, int kind, int silent)
{
	int err;

	tg3_stop_fw(tp);

	tg3_write_sig_pre_reset(tp, kind);

	tg3_abort_hw(tp, silent);
	err = tg3_chip_reset(tp);

	tg3_write_sig_legacy(tp, kind);
	tg3_write_sig_post_reset(tp, kind);

	if (err)
		return err;

	return 0;
}

#define TG3_FW_RELEASE_MAJOR	0x0
#define TG3_FW_RELASE_MINOR	0x0
#define TG3_FW_RELEASE_FIX	0x0
#define TG3_FW_START_ADDR	0x08000000
#define TG3_FW_TEXT_ADDR	0x08000000
#define TG3_FW_TEXT_LEN		0x9c0
#define TG3_FW_RODATA_ADDR	0x080009c0
#define TG3_FW_RODATA_LEN	0x60
#define TG3_FW_DATA_ADDR	0x08000a40
#define TG3_FW_DATA_LEN		0x20
#define TG3_FW_SBSS_ADDR	0x08000a60
#define TG3_FW_SBSS_LEN		0xc
#define TG3_FW_BSS_ADDR		0x08000a70
#define TG3_FW_BSS_LEN		0x10

static u32 tg3FwText[(TG3_FW_TEXT_LEN / sizeof(u32)) + 1] = {
	0x00000000, 0x10000003, 0x00000000, 0x0000000d, 0x0000000d, 0x3c1d0800,
	0x37bd3ffc, 0x03a0f021, 0x3c100800, 0x26100000, 0x0e000018, 0x00000000,
	0x0000000d, 0x3c1d0800, 0x37bd3ffc, 0x03a0f021, 0x3c100800, 0x26100034,
	0x0e00021c, 0x00000000, 0x0000000d, 0x00000000, 0x00000000, 0x00000000,
	0x27bdffe0, 0x3c1cc000, 0xafbf0018, 0xaf80680c, 0x0e00004c, 0x241b2105,
	0x97850000, 0x97870002, 0x9782002c, 0x9783002e, 0x3c040800, 0x248409c0,
	0xafa00014, 0x00021400, 0x00621825, 0x00052c00, 0xafa30010, 0x8f860010,
	0x00e52825, 0x0e000060, 0x24070102, 0x3c02ac00, 0x34420100, 0x3c03ac01,
	0x34630100, 0xaf820490, 0x3c02ffff, 0xaf820494, 0xaf830498, 0xaf82049c,
	0x24020001, 0xaf825ce0, 0x0e00003f, 0xaf825d00, 0x0e000140, 0x00000000,
	0x8fbf0018, 0x03e00008, 0x27bd0020, 0x2402ffff, 0xaf825404, 0x8f835400,
	0x34630400, 0xaf835400, 0xaf825404, 0x3c020800, 0x24420034, 0xaf82541c,
	0x03e00008, 0xaf805400, 0x00000000, 0x00000000, 0x3c020800, 0x34423000,
	0x3c030800, 0x34633000, 0x3c040800, 0x348437ff, 0x3c010800, 0xac220a64,
	0x24020040, 0x3c010800, 0xac220a68, 0x3c010800, 0xac200a60, 0xac600000,
	0x24630004, 0x0083102b, 0x5040fffd, 0xac600000, 0x03e00008, 0x00000000,
	0x00804821, 0x8faa0010, 0x3c020800, 0x8c420a60, 0x3c040800, 0x8c840a68,
	0x8fab0014, 0x24430001, 0x0044102b, 0x3c010800, 0xac230a60, 0x14400003,
	0x00004021, 0x3c010800, 0xac200a60, 0x3c020800, 0x8c420a60, 0x3c030800,
	0x8c630a64, 0x91240000, 0x00021140, 0x00431021, 0x00481021, 0x25080001,
	0xa0440000, 0x29020008, 0x1440fff4, 0x25290001, 0x3c020800, 0x8c420a60,
	0x3c030800, 0x8c630a64, 0x8f84680c, 0x00021140, 0x00431021, 0xac440008,
	0xac45000c, 0xac460010, 0xac470014, 0xac4a0018, 0x03e00008, 0xac4b001c,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0,
	0x02000008, 0x00000000, 0x0a0001e3, 0x3c0a0001, 0x0a0001e3, 0x3c0a0002,
	0x0a0001e3, 0x00000000, 0x0a0001e3, 0x00000000, 0x0a0001e3, 0x00000000,
	0x0a0001e3, 0x00000000, 0x0a0001e3, 0x00000000, 0x0a0001e3, 0x00000000,
	0x0a0001e3, 0x00000000, 0x0a0001e3, 0x00000000, 0x0a0001e3, 0x00000000,
	0x0a0001e3, 0x3c0a0007, 0x0a0001e3, 0x3c0a0008, 0x0a0001e3, 0x3c0a0009,
	0x0a0001e3, 0x00000000, 0x0a0001e3, 0x00000000, 0x0a0001e3, 0x3c0a000b,
	0x0a0001e3, 0x3c0a000c, 0x0a0001e3, 0x3c0a000d, 0x0a0001e3, 0x00000000,
	0x0a0001e3, 0x00000000, 0x0a0001e3, 0x3c0a000e, 0x0a0001e3, 0x00000000,
	0x0a0001e3, 0x00000000, 0x0a0001e3, 0x00000000, 0x0a0001e3, 0x00000000,
	0x0a0001e3, 0x00000000, 0x0a0001e3, 0x00000000, 0x0a0001e3, 0x00000000,
	0x0a0001e3, 0x00000000, 0x0a0001e3, 0x3c0a0013, 0x0a0001e3, 0x3c0a0014,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0x27bdffe0, 0x00001821, 0x00001021, 0xafbf0018, 0xafb10014, 0xafb00010,
	0x3c010800, 0x00220821, 0xac200a70, 0x3c010800, 0x00220821, 0xac200a74,
	0x3c010800, 0x00220821, 0xac200a78, 0x24630001, 0x1860fff5, 0x2442000c,
	0x24110001, 0x8f906810, 0x32020004, 0x14400005, 0x24040001, 0x3c020800,
	0x8c420a78, 0x18400003, 0x00002021, 0x0e000182, 0x00000000, 0x32020001,
	0x10400003, 0x00000000, 0x0e000169, 0x00000000, 0x0a000153, 0xaf915028,
	0x8fbf0018, 0x8fb10014, 0x8fb00010, 0x03e00008, 0x27bd0020, 0x3c050800,
	0x8ca50a70, 0x3c060800, 0x8cc60a80, 0x3c070800, 0x8ce70a78, 0x27bdffe0,
	0x3c040800, 0x248409d0, 0xafbf0018, 0xafa00010, 0x0e000060, 0xafa00014,
	0x0e00017b, 0x00002021, 0x8fbf0018, 0x03e00008, 0x27bd0020, 0x24020001,
	0x8f836810, 0x00821004, 0x00021027, 0x00621824, 0x03e00008, 0xaf836810,
	0x27bdffd8, 0xafbf0024, 0x1080002e, 0xafb00020, 0x8f825cec, 0xafa20018,
	0x8f825cec, 0x3c100800, 0x26100a78, 0xafa2001c, 0x34028000, 0xaf825cec,
	0x8e020000, 0x18400016, 0x00000000, 0x3c020800, 0x94420a74, 0x8fa3001c,
	0x000221c0, 0xac830004, 0x8fa2001c, 0x3c010800, 0x0e000201, 0xac220a74,
	0x10400005, 0x00000000, 0x8e020000, 0x24420001, 0x0a0001df, 0xae020000,
	0x3c020800, 0x8c420a70, 0x00021c02, 0x000321c0, 0x0a0001c5, 0xafa2001c,
	0x0e000201, 0x00000000, 0x1040001f, 0x00000000, 0x8e020000, 0x8fa3001c,
	0x24420001, 0x3c010800, 0xac230a70, 0x3c010800, 0xac230a74, 0x0a0001df,
	0xae020000, 0x3c100800, 0x26100a78, 0x8e020000, 0x18400028, 0x00000000,
	0x0e000201, 0x00000000, 0x14400024, 0x00000000, 0x8e020000, 0x3c030800,
	0x8c630a70, 0x2442ffff, 0xafa3001c, 0x18400006, 0xae020000, 0x00031402,
	0x000221c0, 0x8c820004, 0x3c010800, 0xac220a70, 0x97a2001e, 0x2442ff00,
	0x2c420300, 0x1440000b, 0x24024000, 0x3c040800, 0x248409dc, 0xafa00010,
	0xafa00014, 0x8fa6001c, 0x24050008, 0x0e000060, 0x00003821, 0x0a0001df,
	0x00000000, 0xaf825cf8, 0x3c020800, 0x8c420a40, 0x8fa3001c, 0x24420001,
	0xaf835cf8, 0x3c010800, 0xac220a40, 0x8fbf0024, 0x8fb00020, 0x03e00008,
	0x27bd0028, 0x27bdffe0, 0x3c040800, 0x248409e8, 0x00002821, 0x00003021,
	0x00003821, 0xafbf0018, 0xafa00010, 0x0e000060, 0xafa00014, 0x8fbf0018,
	0x03e00008, 0x27bd0020, 0x8f82680c, 0x8f85680c, 0x00021827, 0x0003182b,
	0x00031823, 0x00431024, 0x00441021, 0x00a2282b, 0x10a00006, 0x00000000,
	0x00401821, 0x8f82680c, 0x0043102b, 0x1440fffd, 0x00000000, 0x03e00008,
	0x00000000, 0x3c040800, 0x8c840000, 0x3c030800, 0x8c630a40, 0x0064102b,
	0x54400002, 0x00831023, 0x00641023, 0x2c420008, 0x03e00008, 0x38420001,
	0x27bdffe0, 0x00802821, 0x3c040800, 0x24840a00, 0x00003021, 0x00003821,
	0xafbf0018, 0xafa00010, 0x0e000060, 0xafa00014, 0x0a000216, 0x00000000,
	0x8fbf0018, 0x03e00008, 0x27bd0020, 0x00000000, 0x27bdffe0, 0x3c1cc000,
	0xafbf0018, 0x0e00004c, 0xaf80680c, 0x3c040800, 0x24840a10, 0x03802821,
	0x00003021, 0x00003821, 0xafa00010, 0x0e000060, 0xafa00014, 0x2402ffff,
	0xaf825404, 0x3c0200aa, 0x0e000234, 0xaf825434, 0x8fbf0018, 0x03e00008,
	0x27bd0020, 0x00000000, 0x00000000, 0x00000000, 0x27bdffe8, 0xafb00010,
	0x24100001, 0xafbf0014, 0x3c01c003, 0xac200000, 0x8f826810, 0x30422000,
	0x10400003, 0x00000000, 0x0e000246, 0x00000000, 0x0a00023a, 0xaf905428,
	0x8fbf0014, 0x8fb00010, 0x03e00008, 0x27bd0018, 0x27bdfff8, 0x8f845d0c,
	0x3c0200ff, 0x3c030800, 0x8c630a50, 0x3442fff8, 0x00821024, 0x1043001e,
	0x3c0500ff, 0x34a5fff8, 0x3c06c003, 0x3c074000, 0x00851824, 0x8c620010,
	0x3c010800, 0xac230a50, 0x30420008, 0x10400005, 0x00871025, 0x8cc20000,
	0x24420001, 0xacc20000, 0x00871025, 0xaf825d0c, 0x8fa20000, 0x24420001,
	0xafa20000, 0x8fa20000, 0x8fa20000, 0x24420001, 0xafa20000, 0x8fa20000,
	0x8f845d0c, 0x3c030800, 0x8c630a50, 0x00851024, 0x1443ffe8, 0x00851824,
	0x27bd0008, 0x03e00008, 0x00000000, 0x00000000, 0x00000000
};

static u32 tg3FwRodata[(TG3_FW_RODATA_LEN / sizeof(u32)) + 1] = {
	0x35373031, 0x726c7341, 0x00000000, 0x00000000, 0x53774576, 0x656e7430,
	0x00000000, 0x726c7045, 0x76656e74, 0x31000000, 0x556e6b6e, 0x45766e74,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x66617461, 0x6c457272,
	0x00000000, 0x00000000, 0x4d61696e, 0x43707542, 0x00000000, 0x00000000,
	0x00000000
};

#if 0 /* All zeros, don't eat up space with it. */
u32 tg3FwData[(TG3_FW_DATA_LEN / sizeof(u32)) + 1] = {
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000
};
#endif

#define RX_CPU_SCRATCH_BASE	0x30000
#define RX_CPU_SCRATCH_SIZE	0x04000
#define TX_CPU_SCRATCH_BASE	0x34000
#define TX_CPU_SCRATCH_SIZE	0x04000

/* tp->lock is held. */
static int tg3_halt_cpu(struct tg3 *tp, u32 offset)
{
	int i;

	kassertw(!(offset == TX_CPU_BASE &&
	    (tp->tg3_flags2 & TG3_FLG2_5705_PLUS)));

	if (offset == RX_CPU_BASE) {
		for (i = 0; i < 10000; i++) {
			tw32(offset + CPU_STATE, 0xffffffff);
			tw32(offset + CPU_MODE,  CPU_MODE_HALT);
			if (tr32(offset + CPU_MODE) & CPU_MODE_HALT)
				break;
		}

		tw32(offset + CPU_STATE, 0xffffffff);
		tw32_f(offset + CPU_MODE,  CPU_MODE_HALT);
		udelay(10);
	} else {
		for (i = 0; i < 10000; i++) {
			tw32(offset + CPU_STATE, 0xffffffff);
			tw32(offset + CPU_MODE,  CPU_MODE_HALT);
			if (tr32(offset + CPU_MODE) & CPU_MODE_HALT)
				break;
		}
	}

	if (i >= 10000) {
		kerndbg( KERN_WARNING, "tg3_reset_cpu timed out for %s, and %s CPU\n",
		       tp->dev->name,
		       (offset == RX_CPU_BASE ? "RX" : "TX"));
		return -ENODEV;
	}

	/* Clear firmware's nvram arbitration. */
	if (tp->tg3_flags & TG3_FLAG_NVRAM)
		tw32(NVRAM_SWARB, SWARB_REQ_CLR0);
	return 0;
}

struct fw_info {
	unsigned int text_base;
	unsigned int text_len;
	u32 *text_data;
	unsigned int rodata_base;
	unsigned int rodata_len;
	u32 *rodata_data;
	unsigned int data_base;
	unsigned int data_len;
	u32 *data_data;
};

/* tp->lock is held. */
static int tg3_load_firmware_cpu(struct tg3 *tp, u32 cpu_base, u32 cpu_scratch_base,
				 int cpu_scratch_size, struct fw_info *info)
{
	int err, lock_err, i;
	void (*write_op)(struct tg3 *, u32, u32);

	if (cpu_base == TX_CPU_BASE &&
	    (tp->tg3_flags2 & TG3_FLG2_5705_PLUS)) {
		kerndbg( KERN_DEBUG, "tg3_load_firmware_cpu: Trying to load "
		       "TX cpu firmware on %s which is 5705.\n",
		       tp->dev->name);
		return -EINVAL;
	}

	if (tp->tg3_flags2 & TG3_FLG2_5705_PLUS)
		write_op = tg3_write_mem;
	else
		write_op = tg3_write_indirect_reg32;

	/* It is possible that bootcode is still loading at this point.
	 * Get the nvram lock first before halting the cpu.
	 */
	lock_err = tg3_nvram_lock(tp);
	err = tg3_halt_cpu(tp, cpu_base);
	if (!lock_err)
		tg3_nvram_unlock(tp);
	if (err)
		goto out;

	for (i = 0; i < cpu_scratch_size; i += sizeof(u32))
		write_op(tp, cpu_scratch_base + i, 0);
	tw32(cpu_base + CPU_STATE, 0xffffffff);
	tw32(cpu_base + CPU_MODE, tr32(cpu_base+CPU_MODE)|CPU_MODE_HALT);
	for (i = 0; i < (info->text_len / sizeof(u32)); i++)
		write_op(tp, (cpu_scratch_base +
			      (info->text_base & 0xffff) +
			      (i * sizeof(u32))),
			 (info->text_data ?
			  info->text_data[i] : 0));
	for (i = 0; i < (info->rodata_len / sizeof(u32)); i++)
		write_op(tp, (cpu_scratch_base +
			      (info->rodata_base & 0xffff) +
			      (i * sizeof(u32))),
			 (info->rodata_data ?
			  info->rodata_data[i] : 0));
	for (i = 0; i < (info->data_len / sizeof(u32)); i++)
		write_op(tp, (cpu_scratch_base +
			      (info->data_base & 0xffff) +
			      (i * sizeof(u32))),
			 (info->data_data ?
			  info->data_data[i] : 0));

	err = 0;

out:
	return err;
}

/* tp->lock is held. */
static int tg3_load_5701_a0_firmware_fix(struct tg3 *tp)
{
	struct fw_info info;
	int err, i;

	info.text_base = TG3_FW_TEXT_ADDR;
	info.text_len = TG3_FW_TEXT_LEN;
	info.text_data = &tg3FwText[0];
	info.rodata_base = TG3_FW_RODATA_ADDR;
	info.rodata_len = TG3_FW_RODATA_LEN;
	info.rodata_data = &tg3FwRodata[0];
	info.data_base = TG3_FW_DATA_ADDR;
	info.data_len = TG3_FW_DATA_LEN;
	info.data_data = NULL;

	err = tg3_load_firmware_cpu(tp, RX_CPU_BASE,
				    RX_CPU_SCRATCH_BASE, RX_CPU_SCRATCH_SIZE,
				    &info);
	if (err)
		return err;

	err = tg3_load_firmware_cpu(tp, TX_CPU_BASE,
				    TX_CPU_SCRATCH_BASE, TX_CPU_SCRATCH_SIZE,
				    &info);
	if (err)
		return err;

	/* Now startup only the RX cpu. */
	tw32(RX_CPU_BASE + CPU_STATE, 0xffffffff);
	tw32_f(RX_CPU_BASE + CPU_PC,    TG3_FW_TEXT_ADDR);

	for (i = 0; i < 5; i++) {
		if (tr32(RX_CPU_BASE + CPU_PC) == TG3_FW_TEXT_ADDR)
			break;
		tw32(RX_CPU_BASE + CPU_STATE, 0xffffffff);
		tw32(RX_CPU_BASE + CPU_MODE,  CPU_MODE_HALT);
		tw32_f(RX_CPU_BASE + CPU_PC,    TG3_FW_TEXT_ADDR);
		udelay(1000);
	}
	if (i >= 5) {
		kerndbg( KERN_DEBUG, "tg3_load_firmware fails for %s "
		       "to set RX CPU PC, is %08x should be %08x\n",
		       tp->dev->name, tr32(RX_CPU_BASE + CPU_PC),
		       TG3_FW_TEXT_ADDR);
		return -ENODEV;
	}
	tw32(RX_CPU_BASE + CPU_STATE, 0xffffffff);
	tw32_f(RX_CPU_BASE + CPU_MODE,  0x00000000);

	return 0;
}

/* tp->lock is held. */
static void __tg3_set_mac_addr(struct tg3 *tp)
{
	u32 addr_high, addr_low;
	int i;

	addr_high = ((tp->dev->dev_addr[0] << 8) |
		     tp->dev->dev_addr[1]);
	addr_low = ((tp->dev->dev_addr[2] << 24) |
		    (tp->dev->dev_addr[3] << 16) |
		    (tp->dev->dev_addr[4] <<  8) |
		    (tp->dev->dev_addr[5] <<  0));
	for (i = 0; i < 4; i++) {
		tw32(MAC_ADDR_0_HIGH + (i * 8), addr_high);
		tw32(MAC_ADDR_0_LOW + (i * 8), addr_low);
	}

	if (GET_ASIC_REV(tp->pci_chip_rev_id) == ASIC_REV_5703 ||
	    GET_ASIC_REV(tp->pci_chip_rev_id) == ASIC_REV_5704) {
		for (i = 0; i < 12; i++) {
			tw32(MAC_EXTADDR_0_HIGH + (i * 8), addr_high);
			tw32(MAC_EXTADDR_0_LOW + (i * 8), addr_low);
		}
	}

	addr_high = (tp->dev->dev_addr[0] +
		     tp->dev->dev_addr[1] +
		     tp->dev->dev_addr[2] +
		     tp->dev->dev_addr[3] +
		     tp->dev->dev_addr[4] +
		     tp->dev->dev_addr[5]) &
		TX_BACKOFF_SEED_MASK;
	tw32(MAC_TX_BACKOFF_SEED, addr_high);
}

/* tp->lock is held. */
static void tg3_set_bdinfo(struct tg3 *tp, u32 bdinfo_addr,
			   dma_addr_t mapping, u32 maxlen_flags,
			   u32 nic_addr)
{
	tg3_write_mem(tp,
		      (bdinfo_addr + TG3_BDINFO_HOST_ADDR + TG3_64BIT_REG_HIGH),
		      ((u64) mapping >> 32));
	tg3_write_mem(tp,
		      (bdinfo_addr + TG3_BDINFO_HOST_ADDR + TG3_64BIT_REG_LOW),
		      ((u64) mapping & 0xffffffff));
	tg3_write_mem(tp,
		      (bdinfo_addr + TG3_BDINFO_MAXLEN_FLAGS),
		       maxlen_flags);

	if (!(tp->tg3_flags2 & TG3_FLG2_5705_PLUS))
		tg3_write_mem(tp,
			      (bdinfo_addr + TG3_BDINFO_NIC_ADDR),
			      nic_addr);
}

static void __tg3_set_rx_mode(struct net_device *);
static void __tg3_set_coalesce(struct tg3 *tp)
{
	int rx_coalesce_usecs = LOW_RXCOL_TICKS;
	int tx_coalesce_usecs = LOW_TXCOL_TICKS;
	int rx_max_coalesced_frames = LOW_RXMAX_FRAMES;
	int tx_max_coalesced_frames = LOW_TXMAX_FRAMES;
	int rx_coalesce_usecs_irq = DEFAULT_RXCOAL_TICK_INT;
	int tx_coalesce_usecs_irq = DEFAULT_TXCOAL_TICK_INT;
	int rx_max_coalesced_frames_irq = DEFAULT_RXCOAL_MAXF_INT;
	int tx_max_coalesced_frames_irq = DEFAULT_TXCOAL_MAXF_INT;
	int stats_block_coalesce_usecs = DEFAULT_STAT_COAL_TICKS;

	if (tp->coalesce_mode & (HOSTCC_MODE_CLRTICK_RXBD |
				 HOSTCC_MODE_CLRTICK_TXBD)) {
		rx_coalesce_usecs = LOW_RXCOL_TICKS_CLRTCKS;
		rx_coalesce_usecs_irq = DEFAULT_RXCOAL_TICK_INT_CLRTCKS;
		tx_coalesce_usecs = LOW_TXCOL_TICKS_CLRTCKS;
		tx_coalesce_usecs_irq = DEFAULT_TXCOAL_TICK_INT_CLRTCKS;
	}

	if (tp->tg3_flags2 & TG3_FLG2_5705_PLUS) {
		rx_coalesce_usecs_irq = 0;
		tx_coalesce_usecs_irq = 0;
		stats_block_coalesce_usecs = 0;
	}

	tw32(HOSTCC_RXCOL_TICKS, rx_coalesce_usecs);
	tw32(HOSTCC_TXCOL_TICKS, tx_coalesce_usecs);
	tw32(HOSTCC_RXMAX_FRAMES, rx_max_coalesced_frames);
	tw32(HOSTCC_TXMAX_FRAMES, tx_max_coalesced_frames);
	if (!(tp->tg3_flags2 & TG3_FLG2_5705_PLUS)) {
		tw32(HOSTCC_RXCOAL_TICK_INT, rx_coalesce_usecs_irq);
		tw32(HOSTCC_TXCOAL_TICK_INT, tx_coalesce_usecs_irq);
	}
	tw32(HOSTCC_RXCOAL_MAXF_INT, rx_max_coalesced_frames_irq);
	tw32(HOSTCC_TXCOAL_MAXF_INT, tx_max_coalesced_frames_irq);
	if (!(tp->tg3_flags2 & TG3_FLG2_5705_PLUS)) {
		u32 val = stats_block_coalesce_usecs;

		if (!netif_carrier_ok(tp->dev))
			val = 0;

		tw32(HOSTCC_STAT_COAL_TICKS, val);
	}
}

/* tp->lock is held. */
static int tg3_reset_hw(struct tg3 *tp, int reset_phy)
{
	u32 val, rdmac_mode;
	int i, err, limit;

	tg3_disable_ints(tp);

	tg3_stop_fw(tp);

	tg3_write_sig_pre_reset(tp, RESET_KIND_INIT);

	if (tp->tg3_flags & TG3_FLAG_INIT_COMPLETE) {
		tg3_abort_hw(tp, 1);
	}

	if ((tp->tg3_flags2 & TG3_FLG2_MII_SERDES) && reset_phy)
		tg3_phy_reset(tp);

	err = tg3_chip_reset(tp);
	if (err)
		return err;

	tg3_write_sig_legacy(tp, RESET_KIND_INIT);

	/* This works around an issue with Athlon chipsets on
	 * B3 tigon3 silicon.  This bit has no effect on any
	 * other revision.  But do not set this on PCI Express
	 * chips.
	 */
	if (!(tp->tg3_flags2 & TG3_FLG2_PCI_EXPRESS))
		tp->pci_clock_ctrl |= CLOCK_CTRL_DELAY_PCI_GRANT;
	tw32_f(TG3PCI_CLOCK_CTRL, tp->pci_clock_ctrl);

	if (tp->pci_chip_rev_id == CHIPREV_ID_5704_A0 &&
	    (tp->tg3_flags & TG3_FLAG_PCIX_MODE)) {
		val = tr32(TG3PCI_PCISTATE);
		val |= PCISTATE_RETRY_SAME_DMA;
		tw32(TG3PCI_PCISTATE, val);
	}

	if (GET_CHIP_REV(tp->pci_chip_rev_id) == CHIPREV_5704_BX) {
		/* Enable some hw fixes.  */
		val = tr32(TG3PCI_MSI_DATA);
		val |= (1 << 26) | (1 << 28) | (1 << 29);
		tw32(TG3PCI_MSI_DATA, val);
	}

	/* Descriptor ring init may make accesses to the
	 * NIC SRAM area to setup the TX descriptors, so we
	 * can only do this after the hardware has been
	 * successfully reset.
	 */
	err = tg3_init_rings(tp);
	if (err)
		return err;

	/* This value is determined during the probe time DMA
	 * engine test, tg3_test_dma.
	 */
	tw32(TG3PCI_DMA_RW_CTRL, tp->dma_rwctrl);

	tp->grc_mode &= ~(GRC_MODE_HOST_SENDBDS |
			  GRC_MODE_4X_NIC_SEND_RINGS |
			  GRC_MODE_NO_TX_PHDR_CSUM |
			  GRC_MODE_NO_RX_PHDR_CSUM);
	tp->grc_mode |= GRC_MODE_HOST_SENDBDS;

	/* Pseudo-header checksum is done by hardware logic and not
	 * the offload processers, so make the chip do the pseudo-
	 * header checksums on receive.  For transmit it is more
	 * convenient to do the pseudo-header checksum in software
	 * as Linux does that on transmit for us in all cases.
	 */
	tp->grc_mode |= GRC_MODE_NO_TX_PHDR_CSUM;

	tw32(GRC_MODE,
	     tp->grc_mode |
	     (GRC_MODE_IRQ_ON_MAC_ATTN | GRC_MODE_HOST_STACKUP));

	/* Setup the timer prescalar register.  Clock is always 66Mhz. */
	val = tr32(GRC_MISC_CFG);
	val &= ~0xff;
	val |= (65 << GRC_MISC_CFG_PRESCALAR_SHIFT);
	tw32(GRC_MISC_CFG, val);

	/* Initialize MBUF/DESC pool. */
	if (tp->tg3_flags2 & TG3_FLG2_5750_PLUS) {
		/* Do nothing.  */
	} else if (GET_ASIC_REV(tp->pci_chip_rev_id) != ASIC_REV_5705) {
		tw32(BUFMGR_MB_POOL_ADDR, NIC_SRAM_MBUF_POOL_BASE);
		if (GET_ASIC_REV(tp->pci_chip_rev_id) == ASIC_REV_5704)
			tw32(BUFMGR_MB_POOL_SIZE, NIC_SRAM_MBUF_POOL_SIZE64);
		else
			tw32(BUFMGR_MB_POOL_SIZE, NIC_SRAM_MBUF_POOL_SIZE96);
		tw32(BUFMGR_DMA_DESC_POOL_ADDR, NIC_SRAM_DMA_DESC_POOL_BASE);
		tw32(BUFMGR_DMA_DESC_POOL_SIZE, NIC_SRAM_DMA_DESC_POOL_SIZE);
	}
#if TG3_TSO_SUPPORT != 0
	else if (tp->tg3_flags2 & TG3_FLG2_TSO_CAPABLE) {
		int fw_len;

		fw_len = (TG3_TSO5_FW_TEXT_LEN +
			  TG3_TSO5_FW_RODATA_LEN +
			  TG3_TSO5_FW_DATA_LEN +
			  TG3_TSO5_FW_SBSS_LEN +
			  TG3_TSO5_FW_BSS_LEN);
		fw_len = (fw_len + (0x80 - 1)) & ~(0x80 - 1);
		tw32(BUFMGR_MB_POOL_ADDR,
		     NIC_SRAM_MBUF_POOL_BASE5705 + fw_len);
		tw32(BUFMGR_MB_POOL_SIZE,
		     NIC_SRAM_MBUF_POOL_SIZE5705 - fw_len - 0xa00);
	}
#endif

	if (tp->dev->mtu <= ETH_DATA_LEN) {
		tw32(BUFMGR_MB_RDMA_LOW_WATER,
		     tp->bufmgr_config.mbuf_read_dma_low_water);
		tw32(BUFMGR_MB_MACRX_LOW_WATER,
		     tp->bufmgr_config.mbuf_mac_rx_low_water);
		tw32(BUFMGR_MB_HIGH_WATER,
		     tp->bufmgr_config.mbuf_high_water);
	} else {
		tw32(BUFMGR_MB_RDMA_LOW_WATER,
		     tp->bufmgr_config.mbuf_read_dma_low_water_jumbo);
		tw32(BUFMGR_MB_MACRX_LOW_WATER,
		     tp->bufmgr_config.mbuf_mac_rx_low_water_jumbo);
		tw32(BUFMGR_MB_HIGH_WATER,
		     tp->bufmgr_config.mbuf_high_water_jumbo);
	}
	tw32(BUFMGR_DMA_LOW_WATER,
	     tp->bufmgr_config.dma_low_water);
	tw32(BUFMGR_DMA_HIGH_WATER,
	     tp->bufmgr_config.dma_high_water);

	tw32(BUFMGR_MODE, BUFMGR_MODE_ENABLE | BUFMGR_MODE_ATTN_ENABLE);
	for (i = 0; i < 2000; i++) {
		if (tr32(BUFMGR_MODE) & BUFMGR_MODE_ENABLE)
			break;
		udelay(10);
	}
	if (i >= 2000) {
		kerndbg( KERN_WARNING, "tg3_reset_hw cannot enable BUFMGR for %s.\n",
		       tp->dev->name);
		return -ENODEV;
	}

	/* Setup replenish threshold. */
	val = tp->rx_pending / 8;
	if (val == 0)
		val = 1;
	else if (val > tp->rx_std_max_post)
		val = tp->rx_std_max_post;

	tw32(RCVBDI_STD_THRESH, val);

	/* Initialize TG3_BDINFO's at:
	 *  RCVDBDI_STD_BD:	standard eth size rx ring
	 *  RCVDBDI_JUMBO_BD:	jumbo frame rx ring
	 *  RCVDBDI_MINI_BD:	small frame rx ring (??? does not work)
	 *
	 * like so:
	 *  TG3_BDINFO_HOST_ADDR:	high/low parts of DMA address of ring
	 *  TG3_BDINFO_MAXLEN_FLAGS:	(rx max buffer size << 16) |
	 *                              ring attribute flags
	 *  TG3_BDINFO_NIC_ADDR:	location of descriptors in nic SRAM
	 *
	 * Standard receive ring @ NIC_SRAM_RX_BUFFER_DESC, 512 entries.
	 * Jumbo receive ring @ NIC_SRAM_RX_JUMBO_BUFFER_DESC, 256 entries.
	 *
	 * The size of each ring is fixed in the firmware, but the location is
	 * configurable.
	 */
	tw32(RCVDBDI_STD_BD + TG3_BDINFO_HOST_ADDR + TG3_64BIT_REG_HIGH,
	     ((u64) tp->rx_std_mapping >> 32));
	tw32(RCVDBDI_STD_BD + TG3_BDINFO_HOST_ADDR + TG3_64BIT_REG_LOW,
	     ((u64) tp->rx_std_mapping & 0xffffffff));
	tw32(RCVDBDI_STD_BD + TG3_BDINFO_NIC_ADDR,
	     NIC_SRAM_RX_BUFFER_DESC);

	/* Don't even try to program the JUMBO/MINI buffer descriptor
	 * configs on 5705.
	 */
	if (tp->tg3_flags2 & TG3_FLG2_5705_PLUS) {
		tw32(RCVDBDI_STD_BD + TG3_BDINFO_MAXLEN_FLAGS,
		     RX_STD_MAX_SIZE_5705 << BDINFO_FLAGS_MAXLEN_SHIFT);
	} else {
		tw32(RCVDBDI_STD_BD + TG3_BDINFO_MAXLEN_FLAGS,
		     RX_STD_MAX_SIZE << BDINFO_FLAGS_MAXLEN_SHIFT);

		tw32(RCVDBDI_MINI_BD + TG3_BDINFO_MAXLEN_FLAGS,
		     BDINFO_FLAGS_DISABLED);

		/* Setup replenish threshold. */
		tw32(RCVBDI_JUMBO_THRESH, tp->rx_jumbo_pending / 8);

		if (tp->tg3_flags & TG3_FLAG_JUMBO_RING_ENABLE) {
			tw32(RCVDBDI_JUMBO_BD + TG3_BDINFO_HOST_ADDR + TG3_64BIT_REG_HIGH,
			     ((u64) tp->rx_jumbo_mapping >> 32));
			tw32(RCVDBDI_JUMBO_BD + TG3_BDINFO_HOST_ADDR + TG3_64BIT_REG_LOW,
			     ((u64) tp->rx_jumbo_mapping & 0xffffffff));
			tw32(RCVDBDI_JUMBO_BD + TG3_BDINFO_MAXLEN_FLAGS,
			     RX_JUMBO_MAX_SIZE << BDINFO_FLAGS_MAXLEN_SHIFT);
			tw32(RCVDBDI_JUMBO_BD + TG3_BDINFO_NIC_ADDR,
			     NIC_SRAM_RX_JUMBO_BUFFER_DESC);
		} else {
			tw32(RCVDBDI_JUMBO_BD + TG3_BDINFO_MAXLEN_FLAGS,
			     BDINFO_FLAGS_DISABLED);
		}

	}

	/* There is only one send ring on 5705/5750, no need to explicitly
	 * disable the others.
	 */
	if (!(tp->tg3_flags2 & TG3_FLG2_5705_PLUS)) {
		/* Clear out send RCB ring in SRAM. */
		for (i = NIC_SRAM_SEND_RCB; i < NIC_SRAM_RCV_RET_RCB; i += TG3_BDINFO_SIZE)
			tg3_write_mem(tp, i + TG3_BDINFO_MAXLEN_FLAGS,
				      BDINFO_FLAGS_DISABLED);
	}

	tp->tx_prod = 0;
	tp->tx_cons = 0;
	tw32_mailbox(MAILBOX_SNDHOST_PROD_IDX_0 + TG3_64BIT_REG_LOW, 0);
	tw32_tx_mbox(MAILBOX_SNDNIC_PROD_IDX_0 + TG3_64BIT_REG_LOW, 0);

	tg3_set_bdinfo(tp, NIC_SRAM_SEND_RCB,
		       tp->tx_desc_mapping,
		       (TG3_TX_RING_SIZE <<
			BDINFO_FLAGS_MAXLEN_SHIFT),
		       NIC_SRAM_TX_BUFFER_DESC);

	/* There is only one receive return ring on 5705/5750, no need
	 * to explicitly disable the others.
	 */
	if (!(tp->tg3_flags2 & TG3_FLG2_5705_PLUS)) {
		for (i = NIC_SRAM_RCV_RET_RCB; i < NIC_SRAM_STATS_BLK;
		     i += TG3_BDINFO_SIZE) {
			tg3_write_mem(tp, i + TG3_BDINFO_MAXLEN_FLAGS,
				      BDINFO_FLAGS_DISABLED);
		}
	}

	tp->rx_rcb_ptr = 0;
	tw32_rx_mbox(MAILBOX_RCVRET_CON_IDX_0 + TG3_64BIT_REG_LOW, 0);

	tg3_set_bdinfo(tp, NIC_SRAM_RCV_RET_RCB,
		       tp->rx_rcb_mapping,
		       (TG3_RX_RCB_RING_SIZE(tp) <<
			BDINFO_FLAGS_MAXLEN_SHIFT),
		       0);

	tp->rx_std_ptr = tp->rx_pending;
	tw32_rx_mbox(MAILBOX_RCV_STD_PROD_IDX + TG3_64BIT_REG_LOW,
		     tp->rx_std_ptr);

	tp->rx_jumbo_ptr = (tp->tg3_flags & TG3_FLAG_JUMBO_RING_ENABLE) ?
						tp->rx_jumbo_pending : 0;
	tw32_rx_mbox(MAILBOX_RCV_JUMBO_PROD_IDX + TG3_64BIT_REG_LOW,
		     tp->rx_jumbo_ptr);

	/* Initialize MAC address and backoff seed. */
	__tg3_set_mac_addr(tp);

	/* MTU + ethernet header + FCS + optional VLAN tag */
	tw32(MAC_RX_MTU_SIZE, tp->dev->mtu + ETH_HLEN + 8);

	/* The slot time is changed by tg3_setup_phy if we
	 * run at gigabit with half duplex.
	 */
	tw32(MAC_TX_LENGTHS,
	     (2 << TX_LENGTHS_IPG_CRS_SHIFT) |
	     (6 << TX_LENGTHS_IPG_SHIFT) |
	     (32 << TX_LENGTHS_SLOT_TIME_SHIFT));

	/* Receive rules. */
	tw32(MAC_RCV_RULE_CFG, RCV_RULE_CFG_DEFAULT_CLASS);
	tw32(RCVLPC_CONFIG, 0x0181);

	/* Calculate RDMAC_MODE setting early, we need it to determine
	 * the RCVLPC_STATE_ENABLE mask.
	 */
	rdmac_mode = (RDMAC_MODE_ENABLE | RDMAC_MODE_TGTABORT_ENAB |
		      RDMAC_MODE_MSTABORT_ENAB | RDMAC_MODE_PARITYERR_ENAB |
		      RDMAC_MODE_ADDROFLOW_ENAB | RDMAC_MODE_FIFOOFLOW_ENAB |
		      RDMAC_MODE_FIFOURUN_ENAB | RDMAC_MODE_FIFOOREAD_ENAB |
		      RDMAC_MODE_LNGREAD_ENAB);
	if (tp->tg3_flags & TG3_FLAG_SPLIT_MODE)
		rdmac_mode |= RDMAC_MODE_SPLIT_ENABLE;

	/* If statement applies to 5705 and 5750 PCI devices only */
	if ((GET_ASIC_REV(tp->pci_chip_rev_id) == ASIC_REV_5705 &&
	     tp->pci_chip_rev_id != CHIPREV_ID_5705_A0) ||
	    (GET_ASIC_REV(tp->pci_chip_rev_id) == ASIC_REV_5750)) {
		if (tp->tg3_flags2 & TG3_FLG2_TSO_CAPABLE &&
		    (tp->pci_chip_rev_id == CHIPREV_ID_5705_A1 ||
		     tp->pci_chip_rev_id == CHIPREV_ID_5705_A2)) {
			rdmac_mode |= RDMAC_MODE_FIFO_SIZE_128;
		} else if (!(tr32(TG3PCI_PCISTATE) & PCISTATE_BUS_SPEED_HIGH) &&
			   !(tp->tg3_flags2 & TG3_FLG2_IS_5788)) {
			rdmac_mode |= RDMAC_MODE_FIFO_LONG_BURST;
		}
	}

	if (tp->tg3_flags2 & TG3_FLG2_PCI_EXPRESS)
		rdmac_mode |= RDMAC_MODE_FIFO_LONG_BURST;

#if TG3_TSO_SUPPORT != 0
	if (tp->tg3_flags2 & TG3_FLG2_HW_TSO)
		rdmac_mode |= (1 << 27);
#endif

	/* Receive/send statistics. */
	if (tp->tg3_flags2 & TG3_FLG2_5750_PLUS) {
		val = tr32(RCVLPC_STATS_ENABLE);
		val &= ~RCVLPC_STATSENAB_DACK_FIX;
		tw32(RCVLPC_STATS_ENABLE, val);
	} else if ((rdmac_mode & RDMAC_MODE_FIFO_SIZE_128) &&
		   (tp->tg3_flags2 & TG3_FLG2_TSO_CAPABLE)) {
		val = tr32(RCVLPC_STATS_ENABLE);
		val &= ~RCVLPC_STATSENAB_LNGBRST_RFIX;
		tw32(RCVLPC_STATS_ENABLE, val);
	} else {
		tw32(RCVLPC_STATS_ENABLE, 0xffffff);
	}
	tw32(RCVLPC_STATSCTRL, RCVLPC_STATSCTRL_ENABLE);
	tw32(SNDDATAI_STATSENAB, 0xffffff);
	tw32(SNDDATAI_STATSCTRL,
	     (SNDDATAI_SCTRL_ENABLE |
	      SNDDATAI_SCTRL_FASTUPD));

	/* Setup host coalescing engine. */
	tw32(HOSTCC_MODE, 0);
	for (i = 0; i < 2000; i++) {
		if (!(tr32(HOSTCC_MODE) & HOSTCC_MODE_ENABLE))
			break;
		udelay(10);
	}

	__tg3_set_coalesce(tp);

	/* set status block DMA address */
	tw32(HOSTCC_STATUS_BLK_HOST_ADDR + TG3_64BIT_REG_HIGH,
	     ((u64) tp->status_mapping >> 32));
	tw32(HOSTCC_STATUS_BLK_HOST_ADDR + TG3_64BIT_REG_LOW,
	     ((u64) tp->status_mapping & 0xffffffff));

	if (!(tp->tg3_flags2 & TG3_FLG2_5705_PLUS)) {
		/* Status/statistics block address.  See tg3_timer,
		 * the tg3_periodic_fetch_stats call there, and
		 * tg3_get_stats to see how this works for 5705/5750 chips.
		 */
		tw32(HOSTCC_STATS_BLK_HOST_ADDR + TG3_64BIT_REG_HIGH,
		     ((u64) tp->stats_mapping >> 32));
		tw32(HOSTCC_STATS_BLK_HOST_ADDR + TG3_64BIT_REG_LOW,
		     ((u64) tp->stats_mapping & 0xffffffff));
		tw32(HOSTCC_STATS_BLK_NIC_ADDR, NIC_SRAM_STATS_BLK);
		tw32(HOSTCC_STATUS_BLK_NIC_ADDR, NIC_SRAM_STATUS_BLK);
	}

	tw32(HOSTCC_MODE, HOSTCC_MODE_ENABLE | tp->coalesce_mode);

	tw32(RCVCC_MODE, RCVCC_MODE_ENABLE | RCVCC_MODE_ATTN_ENABLE);
	tw32(RCVLPC_MODE, RCVLPC_MODE_ENABLE);
	if (!(tp->tg3_flags2 & TG3_FLG2_5705_PLUS))
		tw32(RCVLSC_MODE, RCVLSC_MODE_ENABLE | RCVLSC_MODE_ATTN_ENABLE);

	/* Clear statistics/status block in chip, and status block in ram. */
	for (i = NIC_SRAM_STATS_BLK;
	     i < NIC_SRAM_STATUS_BLK + TG3_HW_STATUS_SIZE;
	     i += sizeof(u32)) {
		tg3_write_mem(tp, i, 0);
		udelay(40);
	}
	memset(tp->hw_status, 0, TG3_HW_STATUS_SIZE);

	if (tp->tg3_flags2 & TG3_FLG2_MII_SERDES) {
		tp->tg3_flags2 &= ~TG3_FLG2_PARALLEL_DETECT;
		/* reset to prevent losing 1st rx packet intermittently */
		tw32_f(MAC_RX_MODE, RX_MODE_RESET);
		udelay(10);
	}

	tp->mac_mode = MAC_MODE_TXSTAT_ENABLE | MAC_MODE_RXSTAT_ENABLE |
		MAC_MODE_TDE_ENABLE | MAC_MODE_RDE_ENABLE | MAC_MODE_FHDE_ENABLE;
	tw32_f(MAC_MODE, tp->mac_mode | MAC_MODE_RXSTAT_CLEAR | MAC_MODE_TXSTAT_CLEAR);
	udelay(40);

	/* tp->grc_local_ctrl is partially set up during tg3_get_invariants().
	 * If TG3_FLAG_EEPROM_WRITE_PROT is set, we should read the
	 * register to preserve the GPIO settings for LOMs. The GPIOs,
	 * whether used as inputs or outputs, are set by boot code after
	 * reset.
	 */
	if (tp->tg3_flags & TG3_FLAG_EEPROM_WRITE_PROT) {
		u32 gpio_mask;

		gpio_mask = GRC_LCLCTRL_GPIO_OE0 | GRC_LCLCTRL_GPIO_OE2 |
			    GRC_LCLCTRL_GPIO_OUTPUT0 | GRC_LCLCTRL_GPIO_OUTPUT2;

		if (GET_ASIC_REV(tp->pci_chip_rev_id) == ASIC_REV_5752)
			gpio_mask |= GRC_LCLCTRL_GPIO_OE3 |
				     GRC_LCLCTRL_GPIO_OUTPUT3;

		if (GET_ASIC_REV(tp->pci_chip_rev_id) == ASIC_REV_5755)
			gpio_mask |= GRC_LCLCTRL_GPIO_UART_SEL;

		tp->grc_local_ctrl |= tr32(GRC_LOCAL_CTRL) & gpio_mask;

		/* GPIO1 must be driven high for eeprom write protect */
		tp->grc_local_ctrl |= (GRC_LCLCTRL_GPIO_OE1 |
				       GRC_LCLCTRL_GPIO_OUTPUT1);
	}
	tw32_f(GRC_LOCAL_CTRL, tp->grc_local_ctrl);
	udelay(100);

	tw32_mailbox_f(MAILBOX_INTERRUPT_0 + TG3_64BIT_REG_LOW, 0);
	tp->last_tag = 0;

	if (!(tp->tg3_flags2 & TG3_FLG2_5705_PLUS)) {
		tw32_f(DMAC_MODE, DMAC_MODE_ENABLE);
		udelay(40);
	}

	val = (WDMAC_MODE_ENABLE | WDMAC_MODE_TGTABORT_ENAB |
	       WDMAC_MODE_MSTABORT_ENAB | WDMAC_MODE_PARITYERR_ENAB |
	       WDMAC_MODE_ADDROFLOW_ENAB | WDMAC_MODE_FIFOOFLOW_ENAB |
	       WDMAC_MODE_FIFOURUN_ENAB | WDMAC_MODE_FIFOOREAD_ENAB |
	       WDMAC_MODE_LNGREAD_ENAB);

	/* If statement applies to 5705 and 5750 PCI devices only */
	if ((GET_ASIC_REV(tp->pci_chip_rev_id) == ASIC_REV_5705 &&
	     tp->pci_chip_rev_id != CHIPREV_ID_5705_A0) ||
	    GET_ASIC_REV(tp->pci_chip_rev_id) == ASIC_REV_5750) {
		if ((tp->tg3_flags & TG3_FLG2_TSO_CAPABLE) &&
		    (tp->pci_chip_rev_id == CHIPREV_ID_5705_A1 ||
		     tp->pci_chip_rev_id == CHIPREV_ID_5705_A2)) {
			/* nothing */
		} else if (!(tr32(TG3PCI_PCISTATE) & PCISTATE_BUS_SPEED_HIGH) &&
			   !(tp->tg3_flags2 & TG3_FLG2_IS_5788) &&
			   !(tp->tg3_flags2 & TG3_FLG2_PCI_EXPRESS)) {
			val |= WDMAC_MODE_RX_ACCEL;
		}
	}

	/* Enable host coalescing bug fix */
	if ((GET_ASIC_REV(tp->pci_chip_rev_id) == ASIC_REV_5755) ||
	    (GET_ASIC_REV(tp->pci_chip_rev_id) == ASIC_REV_5787))
		val |= (1 << 29);

	tw32_f(WDMAC_MODE, val);
	udelay(40);

	if ((tp->tg3_flags & TG3_FLAG_PCIX_MODE) != 0) {
		val = tr32(TG3PCI_X_CAPS);
		if (GET_ASIC_REV(tp->pci_chip_rev_id) == ASIC_REV_5703) {
			val &= ~PCIX_CAPS_BURST_MASK;
			val |= (PCIX_CAPS_MAX_BURST_CPIOB << PCIX_CAPS_BURST_SHIFT);
		} else if (GET_ASIC_REV(tp->pci_chip_rev_id) == ASIC_REV_5704) {
			val &= ~(PCIX_CAPS_SPLIT_MASK | PCIX_CAPS_BURST_MASK);
			val |= (PCIX_CAPS_MAX_BURST_CPIOB << PCIX_CAPS_BURST_SHIFT);
			if (tp->tg3_flags & TG3_FLAG_SPLIT_MODE)
				val |= (tp->split_mode_max_reqs <<
					PCIX_CAPS_SPLIT_SHIFT);
		}
		tw32(TG3PCI_X_CAPS, val);
	}

	tw32_f(RDMAC_MODE, rdmac_mode);
	udelay(40);

	tw32(RCVDCC_MODE, RCVDCC_MODE_ENABLE | RCVDCC_MODE_ATTN_ENABLE);
	if (!(tp->tg3_flags2 & TG3_FLG2_5705_PLUS))
		tw32(MBFREE_MODE, MBFREE_MODE_ENABLE);
	tw32(SNDDATAC_MODE, SNDDATAC_MODE_ENABLE);
	tw32(SNDBDC_MODE, SNDBDC_MODE_ENABLE | SNDBDC_MODE_ATTN_ENABLE);
	tw32(RCVBDI_MODE, RCVBDI_MODE_ENABLE | RCVBDI_MODE_RCB_ATTN_ENAB);
	tw32(RCVDBDI_MODE, RCVDBDI_MODE_ENABLE | RCVDBDI_MODE_INV_RING_SZ);
	tw32(SNDDATAI_MODE, SNDDATAI_MODE_ENABLE);
#if TG3_TSO_SUPPORT != 0
	if (tp->tg3_flags2 & TG3_FLG2_HW_TSO)
		tw32(SNDDATAI_MODE, SNDDATAI_MODE_ENABLE | 0x8);
#endif
	tw32(SNDBDI_MODE, SNDBDI_MODE_ENABLE | SNDBDI_MODE_ATTN_ENABLE);
	tw32(SNDBDS_MODE, SNDBDS_MODE_ENABLE | SNDBDS_MODE_ATTN_ENABLE);

	if (tp->pci_chip_rev_id == CHIPREV_ID_5701_A0) {
		err = tg3_load_5701_a0_firmware_fix(tp);
		if (err)
			return err;
	}

#if TG3_TSO_SUPPORT != 0
	if (tp->tg3_flags2 & TG3_FLG2_TSO_CAPABLE) {
		err = tg3_load_tso_firmware(tp);
		if (err)
			return err;
	}
#endif

	tp->tx_mode = TX_MODE_ENABLE;
	tw32_f(MAC_TX_MODE, tp->tx_mode);
	udelay(100);

	tp->rx_mode = RX_MODE_ENABLE;
	if (GET_ASIC_REV(tp->pci_chip_rev_id) == ASIC_REV_5755)
		tp->rx_mode |= RX_MODE_IPV6_CSUM_ENABLE;

	tw32_f(MAC_RX_MODE, tp->rx_mode);
	udelay(10);

	if (tp->link_config.phy_is_low_power) {
		tp->link_config.phy_is_low_power = 0;
		tp->link_config.speed = tp->link_config.orig_speed;
		tp->link_config.duplex = tp->link_config.orig_duplex;
		tp->link_config.autoneg = tp->link_config.orig_autoneg;
	}

	tp->mi_mode = MAC_MI_MODE_BASE;
	tw32_f(MAC_MI_MODE, tp->mi_mode);
	udelay(80);

	tw32(MAC_LED_CTRL, tp->led_ctrl);

	tw32(MAC_MI_STAT, MAC_MI_STAT_LNKSTAT_ATTN_ENAB);
	if (tp->tg3_flags2 & TG3_FLG2_PHY_SERDES) {
		tw32_f(MAC_RX_MODE, RX_MODE_RESET);
		udelay(10);
	}
	tw32_f(MAC_RX_MODE, tp->rx_mode);
	udelay(10);

	if (tp->tg3_flags2 & TG3_FLG2_PHY_SERDES) {
		if ((GET_ASIC_REV(tp->pci_chip_rev_id) == ASIC_REV_5704) &&
			!(tp->tg3_flags2 & TG3_FLG2_SERDES_PREEMPHASIS)) {
			/* Set drive transmission level to 1.2V  */
			/* only if the signal pre-emphasis bit is not set  */
			val = tr32(MAC_SERDES_CFG);
			val &= 0xfffff000;
			val |= 0x880;
			tw32(MAC_SERDES_CFG, val);
		}
		if (tp->pci_chip_rev_id == CHIPREV_ID_5703_A1)
			tw32(MAC_SERDES_CFG, 0x616000);
	}

	/* Prevent chip from dropping frames when flow control
	 * is enabled.
	 */
	tw32_f(MAC_LOW_WMARK_MAX_RX_FRAME, 2);

	if (GET_ASIC_REV(tp->pci_chip_rev_id) == ASIC_REV_5704 &&
	    (tp->tg3_flags2 & TG3_FLG2_PHY_SERDES)) {
		/* Use hardware link auto-negotiation */
		tp->tg3_flags2 |= TG3_FLG2_HW_AUTONEG;
	}

	if ((tp->tg3_flags2 & TG3_FLG2_MII_SERDES) &&
	    (GET_ASIC_REV(tp->pci_chip_rev_id) == ASIC_REV_5714)) {
		u32 tmp;

		tmp = tr32(SERDES_RX_CTRL);
		tw32(SERDES_RX_CTRL, tmp | SERDES_RX_SIG_DETECT);
		tp->grc_local_ctrl &= ~GRC_LCLCTRL_USE_EXT_SIG_DETECT;
		tp->grc_local_ctrl |= GRC_LCLCTRL_USE_SIG_DETECT;
		tw32(GRC_LOCAL_CTRL, tp->grc_local_ctrl);
	}

	err = tg3_setup_phy(tp, reset_phy);
	if (err)
		return err;

	if (!(tp->tg3_flags2 & TG3_FLG2_PHY_SERDES)) {
		u32 tmp;

		/* Clear CRC stats. */
		if (!tg3_readphy(tp, 0x1e, &tmp)) {
			tg3_writephy(tp, 0x1e, tmp | 0x8000);
			tg3_readphy(tp, 0x14, &tmp);
		}
	}

	__tg3_set_rx_mode(tp->dev);

	/* Initialize receive rules. */
	tw32(MAC_RCV_RULE_0,  0xc2000000 & RCV_RULE_DISABLE_MASK);
	tw32(MAC_RCV_VALUE_0, 0xffffffff & RCV_RULE_DISABLE_MASK);
	tw32(MAC_RCV_RULE_1,  0x86000004 & RCV_RULE_DISABLE_MASK);
	tw32(MAC_RCV_VALUE_1, 0xffffffff & RCV_RULE_DISABLE_MASK);

	if ((tp->tg3_flags2 & TG3_FLG2_5705_PLUS) &&
	    !(tp->tg3_flags2 & TG3_FLG2_5780_CLASS))
		limit = 8;
	else
		limit = 16;
	if (tp->tg3_flags & TG3_FLAG_ENABLE_ASF)
		limit -= 4;
	switch (limit) {
	case 16:
		tw32(MAC_RCV_RULE_15,  0); tw32(MAC_RCV_VALUE_15,  0);
	case 15:
		tw32(MAC_RCV_RULE_14,  0); tw32(MAC_RCV_VALUE_14,  0);
	case 14:
		tw32(MAC_RCV_RULE_13,  0); tw32(MAC_RCV_VALUE_13,  0);
	case 13:
		tw32(MAC_RCV_RULE_12,  0); tw32(MAC_RCV_VALUE_12,  0);
	case 12:
		tw32(MAC_RCV_RULE_11,  0); tw32(MAC_RCV_VALUE_11,  0);
	case 11:
		tw32(MAC_RCV_RULE_10,  0); tw32(MAC_RCV_VALUE_10,  0);
	case 10:
		tw32(MAC_RCV_RULE_9,  0); tw32(MAC_RCV_VALUE_9,  0);
	case 9:
		tw32(MAC_RCV_RULE_8,  0); tw32(MAC_RCV_VALUE_8,  0);
	case 8:
		tw32(MAC_RCV_RULE_7,  0); tw32(MAC_RCV_VALUE_7,  0);
	case 7:
		tw32(MAC_RCV_RULE_6,  0); tw32(MAC_RCV_VALUE_6,  0);
	case 6:
		tw32(MAC_RCV_RULE_5,  0); tw32(MAC_RCV_VALUE_5,  0);
	case 5:
		tw32(MAC_RCV_RULE_4,  0); tw32(MAC_RCV_VALUE_4,  0);
	case 4:
		/* tw32(MAC_RCV_RULE_3,  0); tw32(MAC_RCV_VALUE_3,  0); */
	case 3:
		/* tw32(MAC_RCV_RULE_2,  0); tw32(MAC_RCV_VALUE_2,  0); */
	case 2:
	case 1:

	default:
		break;
	};

	tg3_write_sig_post_reset(tp, RESET_KIND_INIT);

	return 0;
}

/* Called at device open time to get the chip ready for
 * packet processing.  Invoked with tp->lock held.
 */
static int tg3_init_hw(struct tg3 *tp, int reset_phy)
{
	int err;

	/* Force the chip into D0. */
	err = tg3_set_power_state(tp, PCI_D0);
	if (err)
		goto out;

	tg3_switch_clocks(tp);

	tw32(TG3PCI_MEM_WIN_BASE_ADDR, 0);

	err = tg3_reset_hw(tp, reset_phy);

out:
	return err;
}

#define TG3_STAT_ADD32(PSTAT, REG) \
do {	u32 __val = tr32(REG); \
	(PSTAT)->low += __val; \
	if ((PSTAT)->low < __val) \
		(PSTAT)->high += 1; \
} while (0)

static void tg3_periodic_fetch_stats(struct tg3 *tp)
{
	struct tg3_hw_stats *sp = tp->hw_stats;

	if (!netif_carrier_ok(tp->dev))
		return;

	TG3_STAT_ADD32(&sp->tx_octets, MAC_TX_STATS_OCTETS);
	TG3_STAT_ADD32(&sp->tx_collisions, MAC_TX_STATS_COLLISIONS);
	TG3_STAT_ADD32(&sp->tx_xon_sent, MAC_TX_STATS_XON_SENT);
	TG3_STAT_ADD32(&sp->tx_xoff_sent, MAC_TX_STATS_XOFF_SENT);
	TG3_STAT_ADD32(&sp->tx_mac_errors, MAC_TX_STATS_MAC_ERRORS);
	TG3_STAT_ADD32(&sp->tx_single_collisions, MAC_TX_STATS_SINGLE_COLLISIONS);
	TG3_STAT_ADD32(&sp->tx_mult_collisions, MAC_TX_STATS_MULT_COLLISIONS);
	TG3_STAT_ADD32(&sp->tx_deferred, MAC_TX_STATS_DEFERRED);
	TG3_STAT_ADD32(&sp->tx_excessive_collisions, MAC_TX_STATS_EXCESSIVE_COL);
	TG3_STAT_ADD32(&sp->tx_late_collisions, MAC_TX_STATS_LATE_COL);
	TG3_STAT_ADD32(&sp->tx_ucast_packets, MAC_TX_STATS_UCAST);
	TG3_STAT_ADD32(&sp->tx_mcast_packets, MAC_TX_STATS_MCAST);
	TG3_STAT_ADD32(&sp->tx_bcast_packets, MAC_TX_STATS_BCAST);

	TG3_STAT_ADD32(&sp->rx_octets, MAC_RX_STATS_OCTETS);
	TG3_STAT_ADD32(&sp->rx_fragments, MAC_RX_STATS_FRAGMENTS);
	TG3_STAT_ADD32(&sp->rx_ucast_packets, MAC_RX_STATS_UCAST);
	TG3_STAT_ADD32(&sp->rx_mcast_packets, MAC_RX_STATS_MCAST);
	TG3_STAT_ADD32(&sp->rx_bcast_packets, MAC_RX_STATS_BCAST);
	TG3_STAT_ADD32(&sp->rx_fcs_errors, MAC_RX_STATS_FCS_ERRORS);
	TG3_STAT_ADD32(&sp->rx_align_errors, MAC_RX_STATS_ALIGN_ERRORS);
	TG3_STAT_ADD32(&sp->rx_xon_pause_rcvd, MAC_RX_STATS_XON_PAUSE_RECVD);
	TG3_STAT_ADD32(&sp->rx_xoff_pause_rcvd, MAC_RX_STATS_XOFF_PAUSE_RECVD);
	TG3_STAT_ADD32(&sp->rx_mac_ctrl_rcvd, MAC_RX_STATS_MAC_CTRL_RECVD);
	TG3_STAT_ADD32(&sp->rx_xoff_entered, MAC_RX_STATS_XOFF_ENTERED);
	TG3_STAT_ADD32(&sp->rx_frame_too_long_errors, MAC_RX_STATS_FRAME_TOO_LONG);
	TG3_STAT_ADD32(&sp->rx_jabbers, MAC_RX_STATS_JABBERS);
	TG3_STAT_ADD32(&sp->rx_undersize_packets, MAC_RX_STATS_UNDERSIZE);

	TG3_STAT_ADD32(&sp->rxbds_empty, RCVLPC_NO_RCV_BD_CNT);
	TG3_STAT_ADD32(&sp->rx_discards, RCVLPC_IN_DISCARDS_CNT);
	TG3_STAT_ADD32(&sp->rx_errors, RCVLPC_IN_ERRORS_CNT);
}

static void tg3_timer(void *data)
{
	struct tg3 *tp = (struct tg3 *) data;

	if (tp->irq_sync)
		goto restart_timer;

	spin_lock(&tp->lock);

	if (!(tp->tg3_flags & TG3_FLAG_TAGGED_STATUS)) {
		/* All of this garbage is because when using non-tagged
		 * IRQ status the mailbox/status_block protocol the chip
		 * uses with the cpu is race prone.
		 */
		if (tp->hw_status->status & SD_STATUS_UPDATED) {
			tw32(GRC_LOCAL_CTRL,
			     tp->grc_local_ctrl | GRC_LCLCTRL_SETINT);
		} else {
			tw32(HOSTCC_MODE, tp->coalesce_mode |
			     (HOSTCC_MODE_ENABLE | HOSTCC_MODE_NOW));
		}

		if (!(tr32(WDMAC_MODE) & WDMAC_MODE_ENABLE)) {
			tp->tg3_flags2 |= TG3_FLG2_RESTART_TIMER;
			spin_unlock(&tp->lock);
			tg3_reset_task(tp);
			return;
		}
	}

	/* This part only runs once per second. */
	if (!--tp->timer_counter) {
		if (tp->tg3_flags2 & TG3_FLG2_5705_PLUS)
			tg3_periodic_fetch_stats(tp);

		if (tp->tg3_flags & TG3_FLAG_USE_LINKCHG_REG) {
			u32 mac_stat;
			int phy_event;

			mac_stat = tr32(MAC_STATUS);

			phy_event = 0;
			if (tp->tg3_flags & TG3_FLAG_USE_MI_INTERRUPT) {
				if (mac_stat & MAC_STATUS_MI_INTERRUPT)
					phy_event = 1;
			} else if (mac_stat & MAC_STATUS_LNKSTATE_CHANGED)
				phy_event = 1;

			if (phy_event)
				tg3_setup_phy(tp, 0);
		} else if (tp->tg3_flags & TG3_FLAG_POLL_SERDES) {
			u32 mac_stat = tr32(MAC_STATUS);
			int need_setup = 0;

			if (netif_carrier_ok(tp->dev) &&
			    (mac_stat & MAC_STATUS_LNKSTATE_CHANGED)) {
				need_setup = 1;
			}
			if (! netif_carrier_ok(tp->dev) &&
			    (mac_stat & (MAC_STATUS_PCS_SYNCED |
					 MAC_STATUS_SIGNAL_DET))) {
				need_setup = 1;
			}
			if (need_setup) {
				tw32_f(MAC_MODE,
				     (tp->mac_mode &
				      ~MAC_MODE_PORT_MODE_MASK));
				udelay(40);
				tw32_f(MAC_MODE, tp->mac_mode);
				udelay(40);
				tg3_setup_phy(tp, 0);
			}
		} else if (tp->tg3_flags2 & TG3_FLG2_MII_SERDES)
			tg3_serdes_parallel_detect(tp);

		tp->timer_counter = tp->timer_multiplier;
	}

	/* Heartbeat is only sent once every 2 seconds.  */
	if (!--tp->asf_counter) {
		if (tp->tg3_flags & TG3_FLAG_ENABLE_ASF) {
			u32 val;

			tg3_write_mem(tp, NIC_SRAM_FW_CMD_MBOX,
				      FWCMD_NICDRV_ALIVE2);
			tg3_write_mem(tp, NIC_SRAM_FW_CMD_LEN_MBOX, 4);
			/* 5 seconds timeout */
			tg3_write_mem(tp, NIC_SRAM_FW_CMD_DATA_MBOX, 5);
			val = tr32(GRC_RX_CPU_EVENT);
			val |= (1 << 14);
			tw32(GRC_RX_CPU_EVENT, val);
		}
		tp->asf_counter = tp->asf_multiplier;
	}

	spin_unlock(&tp->lock);

restart_timer:
	start_timer(tp->timer, (timer_callback *) &tg3_timer, tp, (jiffies + tp->timer_offset), true );
}

static int tg3_request_irq(struct tg3 *tp)
{
	int (*fn)(int, void *, SysCallRegs_s *);
	struct net_device *dev = tp->dev;

	kerndbg( KERN_DEBUG, "%s: ENTER\n", __FUNCTION__ );

	if (tp->tg3_flags2 & TG3_FLG2_USING_MSI) {
		fn = tg3_msi;
		kerndbg( KERN_DEBUG, "%s: Using MSI\n", __FUNCTION__ );
		if (tp->tg3_flags2 & TG3_FLG2_1SHOT_MSI)
		{
			fn = tg3_msi_1shot;
			kerndbg( KERN_DEBUG, "%s: Using 1shot MSI\n", __FUNCTION__ );
		}
	} else {
		fn = tg3_interrupt;
		kerndbg( KERN_DEBUG, "%s: Using normal interrupt\n", __FUNCTION__ );
		if (tp->tg3_flags & TG3_FLAG_TAGGED_STATUS)
		{
			fn = tg3_interrupt_tagged;
			kerndbg( KERN_DEBUG, "%s: Using tagged interrupt\n", __FUNCTION__ );
		}
	}

	tp->dev->irq_handle = request_irq(tp->dev->irq, fn, NULL, SA_SHIRQ, dev->name, dev);
	kerndbg( KERN_DEBUG, "%s: IRQ handle is %d\n", __FUNCTION__, tp->dev->irq_handle );
	return ( tp->dev->irq_handle < 0 ? 1 : 0 );
}

static int tg3_test_interrupt(struct tg3 *tp)
{
	struct net_device *dev = tp->dev;
	int err, i, handle;
	u32 int_mbox = 0;

	kerndbg( KERN_DEBUG, "%s: 1\n", __FUNCTION__ );

	if (!netif_running(dev))
		return -ENODEV;

	kerndbg( KERN_DEBUG, "%s: 2\n", __FUNCTION__ );

	tg3_disable_ints(tp);

	release_irq(tp->dev->irq, tp->dev->irq_handle);

	handle = request_irq(tp->dev->irq, &tg3_test_isr,
			  NULL, SA_SHIRQ, dev->name, dev);

	tp->hw_status->status &= ~SD_STATUS_UPDATED;
	tg3_enable_ints(tp);

	tw32_f(HOSTCC_MODE, tp->coalesce_mode | HOSTCC_MODE_ENABLE |
	       HOSTCC_MODE_NOW);

	for (i = 0; i < 5; i++) {
		int_mbox = tr32_mailbox(MAILBOX_INTERRUPT_0 +
					TG3_64BIT_REG_LOW);
		if (int_mbox != 0)
			break;
		udelay(1000);
	}

	kerndbg( KERN_DEBUG, "%s: 3\n", __FUNCTION__ );

	tg3_disable_ints(tp);

	release_irq(tp->dev->irq, handle);

	kerndbg( KERN_DEBUG, "%s: 4\n", __FUNCTION__ );
	
	err = tg3_request_irq(tp);

	kerndbg( KERN_DEBUG, "%s: 5\n", __FUNCTION__ );

	if (err)
		return err;

	kerndbg( KERN_DEBUG, "%s: 6\n", __FUNCTION__ );

	if (int_mbox != 0)
		return 0;

	kerndbg( KERN_DEBUG, "%s: 7\n", __FUNCTION__ );

	return -EIO;
}

/* Returns 0 if MSI test succeeds or MSI test fails and INTx mode is
 * successfully restored
 */
static int tg3_test_msi(struct tg3 *tp)
{
	struct net_device *dev = tp->dev;
	int err;
	u16 pci_cmd;

	if (!(tp->tg3_flags2 & TG3_FLG2_USING_MSI))
		return 0;

	/* Turn off SERR reporting in case MSI terminates with Master
	 * Abort.
	 */
	pci_cmd = g_psBus->read_pci_config(tp->pdev->nBus, tp->pdev->nDevice, tp->pdev->nFunction, PCI_COMMAND, 2);
	g_psBus->write_pci_config(tp->pdev->nBus, tp->pdev->nDevice, tp->pdev->nFunction, PCI_COMMAND,
				2, pci_cmd & ~PCI_COMMAND_SERR);

	err = tg3_test_interrupt(tp);

	g_psBus->write_pci_config(tp->pdev->nBus, tp->pdev->nDevice, tp->pdev->nFunction, PCI_COMMAND,
				2, pci_cmd);

	if (!err)
		return 0;

	/* other failures */
	if (err != -EIO)
		return err;

	/* MSI test failed, go back to INTx mode */
	kerndbg( KERN_WARNING, "%s: No interrupt was generated using MSI, "
	       "switching to INTx mode. Please report this failure to "
	       "the PCI maintainer and include system chipset information.\n",
		       tp->dev->name);

	release_irq(tp->dev->irq, tp->dev->irq_handle);
	pci_disable_msi(tp->pdev);

	tp->tg3_flags2 &= ~TG3_FLG2_USING_MSI;

	err = tg3_request_irq(tp);
	if (err)
		return err;

	/* Need to reset the chip because the MSI cycle may have terminated
	 * with Master Abort.
	 */
	tg3_full_lock(tp, 1);

	tg3_halt(tp, RESET_KIND_SHUTDOWN, 1);
	err = tg3_init_hw(tp, 1);

	tg3_full_unlock(tp);

	if (err)
		release_irq(tp->dev->irq, tp->dev->irq_handle);

	return err;
}

static int tg3_open(struct net_device *dev)
{
	struct tg3 *tp = netdev_priv(dev);
	int err;

	tg3_full_lock(tp, 0);

	err = tg3_set_power_state(tp, PCI_D0);
	if (err)
		return err;

	tg3_disable_ints(tp);
	tp->tg3_flags &= ~TG3_FLAG_INIT_COMPLETE;

	tg3_full_unlock(tp);

	/* The placement of this call is tied
	 * to the setup and use of Host TX descriptors.
	 */
	err = tg3_alloc_consistent(tp);
	if (err)
		return err;
	if ((tp->tg3_flags2 & TG3_FLG2_5750_PLUS) &&
	    (GET_CHIP_REV(tp->pci_chip_rev_id) != CHIPREV_5750_AX) &&
	    (GET_CHIP_REV(tp->pci_chip_rev_id) != CHIPREV_5750_BX) &&
	    !((GET_ASIC_REV(tp->pci_chip_rev_id) == ASIC_REV_5714) &&
	      (tp->pdev_peer == tp->pdev))) {
		/* All MSI supporting chips should support tagged
		 * status.  Assert that this is the case.
		 */
		if (!(tp->tg3_flags & TG3_FLAG_TAGGED_STATUS)) {
			kerndbg( KERN_WARNING, "%s: MSI without TAGGED? "
			       "Not using MSI.\n", tp->dev->name);
		} else if (pci_enable_msi(tp->pdev) == 0) {
			u32 msi_mode;

			msi_mode = tr32(MSGINT_MODE);
			tw32(MSGINT_MODE, msi_mode | MSGINT_MODE_ENABLE);
			tp->tg3_flags2 |= TG3_FLG2_USING_MSI;
		}
	}
	err = tg3_request_irq(tp);

	if (err) {
		if (tp->tg3_flags2 & TG3_FLG2_USING_MSI) {
			pci_disable_msi(tp->pdev);
			tp->tg3_flags2 &= ~TG3_FLG2_USING_MSI;
		}
		tg3_free_consistent(tp);
		return err;
	}

	tg3_full_lock(tp, 0);

	err = tg3_init_hw(tp, 1);
	if (err) {
		tg3_halt(tp, RESET_KIND_SHUTDOWN, 1);
		tg3_free_rings(tp);
	} else {
		if (tp->tg3_flags & TG3_FLAG_TAGGED_STATUS)
			tp->timer_offset = HZ;
		else
			tp->timer_offset = HZ / 10;

		tp->timer_counter = tp->timer_multiplier =
			(HZ / tp->timer_offset);
		tp->asf_counter = tp->asf_multiplier =
			((HZ / tp->timer_offset) * 2);

		tp->timer = create_timer();
	}

	tg3_full_unlock(tp);

	if (err) {
		release_irq(tp->dev->irq, tp->dev->irq_handle);
		if (tp->tg3_flags2 & TG3_FLG2_USING_MSI) {
			pci_disable_msi(tp->pdev);
			tp->tg3_flags2 &= ~TG3_FLG2_USING_MSI;
		}
		tg3_free_consistent(tp);
		return err;
	}

	if (tp->tg3_flags2 & TG3_FLG2_USING_MSI) {
		err = tg3_test_msi(tp);

		if (err) {
			tg3_full_lock(tp, 0);

			if (tp->tg3_flags2 & TG3_FLG2_USING_MSI) {
				pci_disable_msi(tp->pdev);
				tp->tg3_flags2 &= ~TG3_FLG2_USING_MSI;
			}
			tg3_halt(tp, RESET_KIND_SHUTDOWN, 1);
			tg3_free_rings(tp);
			tg3_free_consistent(tp);

			tg3_full_unlock(tp);

			return err;
		}

		if (tp->tg3_flags2 & TG3_FLG2_USING_MSI) {
			if (tp->tg3_flags2 & TG3_FLG2_1SHOT_MSI) {
				u32 val = tr32(0x7c04);

				tw32(0x7c04, val | (1 << 29));
			}
		}
	}

	tg3_full_lock(tp, 0);

	start_timer(tp->timer, (timer_callback *) &tg3_timer, tp, (jiffies + tp->timer_offset), true );
	tp->tg3_flags |= TG3_FLAG_INIT_COMPLETE;
	tg3_enable_ints(tp);

	tg3_full_unlock(tp);

	netif_start_queue(dev);

	return 0;
}

static int tg3_close(struct net_device *dev)
{
	struct tg3 *tp = netdev_priv(dev);

	/* Calling flush_scheduled_work() may deadlock because
	 * linkwatch_event() may be on the workqueue and it will try to get
	 * the rtnl_lock which we are holding.
	 */
	while (tp->tg3_flags & TG3_FLAG_IN_RESET_TASK)
		udelay(100);

	netif_stop_queue(dev);

	delete_timer(tp->timer);

	tg3_full_lock(tp, 1);
#if 0
	tg3_dump_state(tp);
#endif

	tg3_disable_ints(tp);

	tg3_halt(tp, RESET_KIND_SHUTDOWN, 1);
	tg3_free_rings(tp);
	tp->tg3_flags &=
		~(TG3_FLAG_INIT_COMPLETE |
		  TG3_FLAG_GOT_SERDES_FLOWCTL);

	tg3_full_unlock(tp);

	release_irq(tp->dev->irq, tp->dev->irq_handle);
	if (tp->tg3_flags2 & TG3_FLG2_USING_MSI) {
		pci_disable_msi(tp->pdev);
		tp->tg3_flags2 &= ~TG3_FLG2_USING_MSI;
	}

/* XXXKV: We can worry about stats at a later point */
#if 0
	memcpy(&tp->net_stats_prev, tg3_get_stats(tp->dev),
	       sizeof(tp->net_stats_prev));
	memcpy(&tp->estats_prev, tg3_get_estats(tp),
	       sizeof(tp->estats_prev));
#endif

	tg3_free_consistent(tp);

	tg3_set_power_state(tp, PCI_D3hot);

	netif_carrier_off(tp->dev);

	return 0;
}

static void tg3_set_multi(struct tg3 *tp, unsigned int accept_all)
{
	/* accept or reject all multicast frames */
	tw32(MAC_HASH_REG_0, accept_all ? 0xffffffff : 0);
	tw32(MAC_HASH_REG_1, accept_all ? 0xffffffff : 0);
	tw32(MAC_HASH_REG_2, accept_all ? 0xffffffff : 0);
	tw32(MAC_HASH_REG_3, accept_all ? 0xffffffff : 0);
}

static void __tg3_set_rx_mode(struct net_device *dev)
{
	struct tg3 *tp = netdev_priv(dev);
	u32 rx_mode;

	rx_mode = tp->rx_mode & ~(RX_MODE_PROMISC |
				  RX_MODE_KEEP_VLAN_TAG);

	/* When ASF is in use, we always keep the RX_MODE_KEEP_VLAN_TAG
	 * flag clear.
	 */
#if TG3_VLAN_TAG_USED
	if (!tp->vlgrp &&
	    !(tp->tg3_flags & TG3_FLAG_ENABLE_ASF))
		rx_mode |= RX_MODE_KEEP_VLAN_TAG;
#else
	/* By definition, VLAN is disabled always in this
	 * case.
	 */
	if (!(tp->tg3_flags & TG3_FLAG_ENABLE_ASF))
		rx_mode |= RX_MODE_KEEP_VLAN_TAG;
#endif

	if (dev->flags & IFF_PROMISC) {
		/* Promiscuous mode. */
		rx_mode |= RX_MODE_PROMISC;
	} else if (dev->mc_count < 1) {
		/* Reject all multicast. */
		tg3_set_multi (tp, 0);
	} else {
		/* Accept all multicast. */
		tg3_set_multi (tp, 1);
	}

	if (rx_mode != tp->rx_mode) {
		tp->rx_mode = rx_mode;
		tw32_f(MAC_RX_MODE, rx_mode);
		udelay(10);
	}
}

static int tg3_nvram_read(struct tg3 *tp, u32 offset, u32 *val);
static int tg3_nvram_read_swab(struct tg3 *tp, u32 offset, u32 *val);

static void tg3_get_eeprom_size(struct tg3 *tp)
{
	u32 cursize, val, magic;

	tp->nvram_size = EEPROM_CHIP_SIZE;

	if (tg3_nvram_read_swab(tp, 0, &magic) != 0)
		return;

	if ((magic != TG3_EEPROM_MAGIC) && ((magic & 0xff000000) != 0xa5000000))
		return;

	/*
	 * Size the chip by reading offsets at increasing powers of two.
	 * When we encounter our validation signature, we know the addressing
	 * has wrapped around, and thus have our chip size.
	 */
	cursize = 0x10;

	while (cursize < tp->nvram_size) {
		if (tg3_nvram_read_swab(tp, cursize, &val) != 0)
			return;

		if (val == magic)
			break;

		cursize <<= 1;
	}

	tp->nvram_size = cursize;
}
		
static void tg3_get_nvram_size(struct tg3 *tp)
{
	u32 val;

	if (tg3_nvram_read_swab(tp, 0, &val) != 0)
		return;

	/* Selfboot format */
	if (val != TG3_EEPROM_MAGIC) {
		tg3_get_eeprom_size(tp);
		return;
	}

	if (tg3_nvram_read(tp, 0xf0, &val) == 0) {
		if (val != 0) {
			tp->nvram_size = (val >> 16) * 1024;
			return;
		}
	}
	tp->nvram_size = 0x20000;
}

static void tg3_get_nvram_info(struct tg3 *tp)
{
	u32 nvcfg1;

	nvcfg1 = tr32(NVRAM_CFG1);
	if (nvcfg1 & NVRAM_CFG1_FLASHIF_ENAB) {
		tp->tg3_flags2 |= TG3_FLG2_FLASH;
	}
	else {
		nvcfg1 &= ~NVRAM_CFG1_COMPAT_BYPASS;
		tw32(NVRAM_CFG1, nvcfg1);
	}

	if ((GET_ASIC_REV(tp->pci_chip_rev_id) == ASIC_REV_5750) ||
	    (tp->tg3_flags2 & TG3_FLG2_5780_CLASS)) {
		switch (nvcfg1 & NVRAM_CFG1_VENDOR_MASK) {
			case FLASH_VENDOR_ATMEL_FLASH_BUFFERED:
				tp->nvram_jedecnum = JEDEC_ATMEL;
				tp->nvram_pagesize = ATMEL_AT45DB0X1B_PAGE_SIZE;
				tp->tg3_flags |= TG3_FLAG_NVRAM_BUFFERED;
				break;
			case FLASH_VENDOR_ATMEL_FLASH_UNBUFFERED:
				tp->nvram_jedecnum = JEDEC_ATMEL;
                         	tp->nvram_pagesize = ATMEL_AT25F512_PAGE_SIZE;
				break;
			case FLASH_VENDOR_ATMEL_EEPROM:
				tp->nvram_jedecnum = JEDEC_ATMEL;
                         	tp->nvram_pagesize = ATMEL_AT24C512_CHIP_SIZE;
				tp->tg3_flags |= TG3_FLAG_NVRAM_BUFFERED;
				break;
			case FLASH_VENDOR_ST:
				tp->nvram_jedecnum = JEDEC_ST;
				tp->nvram_pagesize = ST_M45PEX0_PAGE_SIZE;
				tp->tg3_flags |= TG3_FLAG_NVRAM_BUFFERED;
				break;
			case FLASH_VENDOR_SAIFUN:
				tp->nvram_jedecnum = JEDEC_SAIFUN;
				tp->nvram_pagesize = SAIFUN_SA25F0XX_PAGE_SIZE;
				break;
			case FLASH_VENDOR_SST_SMALL:
			case FLASH_VENDOR_SST_LARGE:
				tp->nvram_jedecnum = JEDEC_SST;
				tp->nvram_pagesize = SST_25VF0X0_PAGE_SIZE;
				break;
		}
	}
	else {
		tp->nvram_jedecnum = JEDEC_ATMEL;
		tp->nvram_pagesize = ATMEL_AT45DB0X1B_PAGE_SIZE;
		tp->tg3_flags |= TG3_FLAG_NVRAM_BUFFERED;
	}
}

static void tg3_get_5752_nvram_info(struct tg3 *tp)
{
	u32 nvcfg1;

	nvcfg1 = tr32(NVRAM_CFG1);

	/* NVRAM protection for TPM */
	if (nvcfg1 & (1 << 27))
		tp->tg3_flags2 |= TG3_FLG2_PROTECTED_NVRAM;

	switch (nvcfg1 & NVRAM_CFG1_5752VENDOR_MASK) {
		case FLASH_5752VENDOR_ATMEL_EEPROM_64KHZ:
		case FLASH_5752VENDOR_ATMEL_EEPROM_376KHZ:
			tp->nvram_jedecnum = JEDEC_ATMEL;
			tp->tg3_flags |= TG3_FLAG_NVRAM_BUFFERED;
			break;
		case FLASH_5752VENDOR_ATMEL_FLASH_BUFFERED:
			tp->nvram_jedecnum = JEDEC_ATMEL;
			tp->tg3_flags |= TG3_FLAG_NVRAM_BUFFERED;
			tp->tg3_flags2 |= TG3_FLG2_FLASH;
			break;
		case FLASH_5752VENDOR_ST_M45PE10:
		case FLASH_5752VENDOR_ST_M45PE20:
		case FLASH_5752VENDOR_ST_M45PE40:
			tp->nvram_jedecnum = JEDEC_ST;
			tp->tg3_flags |= TG3_FLAG_NVRAM_BUFFERED;
			tp->tg3_flags2 |= TG3_FLG2_FLASH;
			break;
	}

	if (tp->tg3_flags2 & TG3_FLG2_FLASH) {
		switch (nvcfg1 & NVRAM_CFG1_5752PAGE_SIZE_MASK) {
			case FLASH_5752PAGE_SIZE_256:
				tp->nvram_pagesize = 256;
				break;
			case FLASH_5752PAGE_SIZE_512:
				tp->nvram_pagesize = 512;
				break;
			case FLASH_5752PAGE_SIZE_1K:
				tp->nvram_pagesize = 1024;
				break;
			case FLASH_5752PAGE_SIZE_2K:
				tp->nvram_pagesize = 2048;
				break;
			case FLASH_5752PAGE_SIZE_4K:
				tp->nvram_pagesize = 4096;
				break;
			case FLASH_5752PAGE_SIZE_264:
				tp->nvram_pagesize = 264;
				break;
		}
	}
	else {
		/* For eeprom, set pagesize to maximum eeprom size */
		tp->nvram_pagesize = ATMEL_AT24C512_CHIP_SIZE;

		nvcfg1 &= ~NVRAM_CFG1_COMPAT_BYPASS;
		tw32(NVRAM_CFG1, nvcfg1);
	}
}

static void tg3_get_5755_nvram_info(struct tg3 *tp)
{
	u32 nvcfg1;

	nvcfg1 = tr32(NVRAM_CFG1);

	/* NVRAM protection for TPM */
	if (nvcfg1 & (1 << 27))
		tp->tg3_flags2 |= TG3_FLG2_PROTECTED_NVRAM;

	switch (nvcfg1 & NVRAM_CFG1_5752VENDOR_MASK) {
		case FLASH_5755VENDOR_ATMEL_EEPROM_64KHZ:
		case FLASH_5755VENDOR_ATMEL_EEPROM_376KHZ:
			tp->nvram_jedecnum = JEDEC_ATMEL;
			tp->tg3_flags |= TG3_FLAG_NVRAM_BUFFERED;
			tp->nvram_pagesize = ATMEL_AT24C512_CHIP_SIZE;

			nvcfg1 &= ~NVRAM_CFG1_COMPAT_BYPASS;
			tw32(NVRAM_CFG1, nvcfg1);
			break;
		case FLASH_5752VENDOR_ATMEL_FLASH_BUFFERED:
		case FLASH_5755VENDOR_ATMEL_FLASH_1:
		case FLASH_5755VENDOR_ATMEL_FLASH_2:
		case FLASH_5755VENDOR_ATMEL_FLASH_3:
		case FLASH_5755VENDOR_ATMEL_FLASH_4:
			tp->nvram_jedecnum = JEDEC_ATMEL;
			tp->tg3_flags |= TG3_FLAG_NVRAM_BUFFERED;
			tp->tg3_flags2 |= TG3_FLG2_FLASH;
			tp->nvram_pagesize = 264;
			break;
		case FLASH_5752VENDOR_ST_M45PE10:
		case FLASH_5752VENDOR_ST_M45PE20:
		case FLASH_5752VENDOR_ST_M45PE40:
			tp->nvram_jedecnum = JEDEC_ST;
			tp->tg3_flags |= TG3_FLAG_NVRAM_BUFFERED;
			tp->tg3_flags2 |= TG3_FLG2_FLASH;
			tp->nvram_pagesize = 256;
			break;
	}
}

static void tg3_get_5787_nvram_info(struct tg3 *tp)
{
	u32 nvcfg1;

	nvcfg1 = tr32(NVRAM_CFG1);

	switch (nvcfg1 & NVRAM_CFG1_5752VENDOR_MASK) {
		case FLASH_5787VENDOR_ATMEL_EEPROM_64KHZ:
		case FLASH_5787VENDOR_ATMEL_EEPROM_376KHZ:
		case FLASH_5787VENDOR_MICRO_EEPROM_64KHZ:
		case FLASH_5787VENDOR_MICRO_EEPROM_376KHZ:
			tp->nvram_jedecnum = JEDEC_ATMEL;
			tp->tg3_flags |= TG3_FLAG_NVRAM_BUFFERED;
			tp->nvram_pagesize = ATMEL_AT24C512_CHIP_SIZE;

			nvcfg1 &= ~NVRAM_CFG1_COMPAT_BYPASS;
			tw32(NVRAM_CFG1, nvcfg1);
			break;
		case FLASH_5752VENDOR_ATMEL_FLASH_BUFFERED:
		case FLASH_5755VENDOR_ATMEL_FLASH_1:
		case FLASH_5755VENDOR_ATMEL_FLASH_2:
		case FLASH_5755VENDOR_ATMEL_FLASH_3:
			tp->nvram_jedecnum = JEDEC_ATMEL;
			tp->tg3_flags |= TG3_FLAG_NVRAM_BUFFERED;
			tp->tg3_flags2 |= TG3_FLG2_FLASH;
			tp->nvram_pagesize = 264;
			break;
		case FLASH_5752VENDOR_ST_M45PE10:
		case FLASH_5752VENDOR_ST_M45PE20:
		case FLASH_5752VENDOR_ST_M45PE40:
			tp->nvram_jedecnum = JEDEC_ST;
			tp->tg3_flags |= TG3_FLAG_NVRAM_BUFFERED;
			tp->tg3_flags2 |= TG3_FLG2_FLASH;
			tp->nvram_pagesize = 256;
			break;
	}
}

/* Chips other than 5700/5701 use the NVRAM for fetching info. */
static void tg3_nvram_init(struct tg3 *tp)
{
	int j;

	tw32_f(GRC_EEPROM_ADDR,
	     (EEPROM_ADDR_FSM_RESET |
	      (EEPROM_DEFAULT_CLOCK_PERIOD <<
	       EEPROM_ADDR_CLKPERD_SHIFT)));

	/* XXX schedule_timeout() ... */
	for (j = 0; j < 100; j++)
		udelay(10);

	/* Enable seeprom accesses. */
	tw32_f(GRC_LOCAL_CTRL,
	     tr32(GRC_LOCAL_CTRL) | GRC_LCLCTRL_AUTO_SEEPROM);
	udelay(100);

	if (GET_ASIC_REV(tp->pci_chip_rev_id) != ASIC_REV_5700 &&
	    GET_ASIC_REV(tp->pci_chip_rev_id) != ASIC_REV_5701) {
		tp->tg3_flags |= TG3_FLAG_NVRAM;

		if (tg3_nvram_lock(tp)) {
			kerndbg( KERN_WARNING, "%s: Cannot get nvarm lock, tg3_nvram_init failed.\n", tp->dev->name);
			return;
		}
		tg3_enable_nvram_access(tp);

		if (GET_ASIC_REV(tp->pci_chip_rev_id) == ASIC_REV_5752)
			tg3_get_5752_nvram_info(tp);
		else if (GET_ASIC_REV(tp->pci_chip_rev_id) == ASIC_REV_5755)
			tg3_get_5755_nvram_info(tp);
		else if (GET_ASIC_REV(tp->pci_chip_rev_id) == ASIC_REV_5787)
			tg3_get_5787_nvram_info(tp);
		else
			tg3_get_nvram_info(tp);

		tg3_get_nvram_size(tp);

		tg3_disable_nvram_access(tp);
		tg3_nvram_unlock(tp);

	} else {
		tp->tg3_flags &= ~(TG3_FLAG_NVRAM | TG3_FLAG_NVRAM_BUFFERED);

		tg3_get_eeprom_size(tp);
	}
}

static int tg3_nvram_read_using_eeprom(struct tg3 *tp,
					u32 offset, u32 *val)
{
	u32 tmp;
	int i;

	if (offset > EEPROM_ADDR_ADDR_MASK ||
	    (offset % 4) != 0)
		return -EINVAL;

	tmp = tr32(GRC_EEPROM_ADDR) & ~(EEPROM_ADDR_ADDR_MASK |
					EEPROM_ADDR_DEVID_MASK |
					EEPROM_ADDR_READ);
	tw32(GRC_EEPROM_ADDR,
	     tmp |
	     (0 << EEPROM_ADDR_DEVID_SHIFT) |
	     ((offset << EEPROM_ADDR_ADDR_SHIFT) &
	      EEPROM_ADDR_ADDR_MASK) |
	     EEPROM_ADDR_READ | EEPROM_ADDR_START);

	for (i = 0; i < 10000; i++) {
		tmp = tr32(GRC_EEPROM_ADDR);

		if (tmp & EEPROM_ADDR_COMPLETE)
			break;
		udelay(100);
	}
	if (!(tmp & EEPROM_ADDR_COMPLETE))
		return -EBUSY;

	*val = tr32(GRC_EEPROM_DATA);
	return 0;
}

#define NVRAM_CMD_TIMEOUT 10000

static int tg3_nvram_exec_cmd(struct tg3 *tp, u32 nvram_cmd)
{
	int i;

	tw32(NVRAM_CMD, nvram_cmd);
	for (i = 0; i < NVRAM_CMD_TIMEOUT; i++) {
		udelay(10);
		if (tr32(NVRAM_CMD) & NVRAM_CMD_DONE) {
			udelay(10);
			break;
		}
	}
	if (i == NVRAM_CMD_TIMEOUT) {
		return -EBUSY;
	}
	return 0;
}

static u32 tg3_nvram_phys_addr(struct tg3 *tp, u32 addr)
{
	if ((tp->tg3_flags & TG3_FLAG_NVRAM) &&
	    (tp->tg3_flags & TG3_FLAG_NVRAM_BUFFERED) &&
	    (tp->tg3_flags2 & TG3_FLG2_FLASH) &&
	    (tp->nvram_jedecnum == JEDEC_ATMEL))

		addr = ((addr / tp->nvram_pagesize) <<
			ATMEL_AT45DB0X1B_PAGE_POS) +
		       (addr % tp->nvram_pagesize);

	return addr;
}

static u32 tg3_nvram_logical_addr(struct tg3 *tp, u32 addr)
{
	if ((tp->tg3_flags & TG3_FLAG_NVRAM) &&
	    (tp->tg3_flags & TG3_FLAG_NVRAM_BUFFERED) &&
	    (tp->tg3_flags2 & TG3_FLG2_FLASH) &&
	    (tp->nvram_jedecnum == JEDEC_ATMEL))

		addr = ((addr >> ATMEL_AT45DB0X1B_PAGE_POS) *
			tp->nvram_pagesize) +
		       (addr & ((1 << ATMEL_AT45DB0X1B_PAGE_POS) - 1));

	return addr;
}

static int tg3_nvram_read(struct tg3 *tp, u32 offset, u32 *val)
{
	int ret;

	if (!(tp->tg3_flags & TG3_FLAG_NVRAM))
		return tg3_nvram_read_using_eeprom(tp, offset, val);

	offset = tg3_nvram_phys_addr(tp, offset);

	if (offset > NVRAM_ADDR_MSK)
		return -EINVAL;

	ret = tg3_nvram_lock(tp);
	if (ret)
		return ret;

	tg3_enable_nvram_access(tp);

	tw32(NVRAM_ADDR, offset);
	ret = tg3_nvram_exec_cmd(tp, NVRAM_CMD_RD | NVRAM_CMD_GO |
		NVRAM_CMD_FIRST | NVRAM_CMD_LAST | NVRAM_CMD_DONE);

	if (ret == 0)
		*val = swab32(tr32(NVRAM_RDDATA));

	tg3_disable_nvram_access(tp);

	tg3_nvram_unlock(tp);

	return ret;
}

static int tg3_nvram_read_swab(struct tg3 *tp, u32 offset, u32 *val)
{
	int err;
	u32 tmp;

	err = tg3_nvram_read(tp, offset, &tmp);
	*val = swab32(tmp);
	return err;
}

struct subsys_tbl_ent {
	u16 subsys_vendor, subsys_devid;
	u32 phy_id;
};

static struct subsys_tbl_ent subsys_id_to_phy_id[] = {
	/* Broadcom boards. */
	{ PCI_VENDOR_ID_BROADCOM, 0x1644, PHY_ID_BCM5401 }, /* BCM95700A6 */
	{ PCI_VENDOR_ID_BROADCOM, 0x0001, PHY_ID_BCM5701 }, /* BCM95701A5 */
	{ PCI_VENDOR_ID_BROADCOM, 0x0002, PHY_ID_BCM8002 }, /* BCM95700T6 */
	{ PCI_VENDOR_ID_BROADCOM, 0x0003, 0 },		    /* BCM95700A9 */
	{ PCI_VENDOR_ID_BROADCOM, 0x0005, PHY_ID_BCM5701 }, /* BCM95701T1 */
	{ PCI_VENDOR_ID_BROADCOM, 0x0006, PHY_ID_BCM5701 }, /* BCM95701T8 */
	{ PCI_VENDOR_ID_BROADCOM, 0x0007, 0 },		    /* BCM95701A7 */
	{ PCI_VENDOR_ID_BROADCOM, 0x0008, PHY_ID_BCM5701 }, /* BCM95701A10 */
	{ PCI_VENDOR_ID_BROADCOM, 0x8008, PHY_ID_BCM5701 }, /* BCM95701A12 */
	{ PCI_VENDOR_ID_BROADCOM, 0x0009, PHY_ID_BCM5703 }, /* BCM95703Ax1 */
	{ PCI_VENDOR_ID_BROADCOM, 0x8009, PHY_ID_BCM5703 }, /* BCM95703Ax2 */

	/* 3com boards. */
	{ PCI_VENDOR_ID_3COM, 0x1000, PHY_ID_BCM5401 }, /* 3C996T */
	{ PCI_VENDOR_ID_3COM, 0x1006, PHY_ID_BCM5701 }, /* 3C996BT */
	{ PCI_VENDOR_ID_3COM, 0x1004, 0 },		/* 3C996SX */
	{ PCI_VENDOR_ID_3COM, 0x1007, PHY_ID_BCM5701 }, /* 3C1000T */
	{ PCI_VENDOR_ID_3COM, 0x1008, PHY_ID_BCM5701 }, /* 3C940BR01 */

	/* DELL boards. */
	{ PCI_VENDOR_ID_DELL, 0x00d1, PHY_ID_BCM5401 }, /* VIPER */
	{ PCI_VENDOR_ID_DELL, 0x0106, PHY_ID_BCM5401 }, /* JAGUAR */
	{ PCI_VENDOR_ID_DELL, 0x0109, PHY_ID_BCM5411 }, /* MERLOT */
	{ PCI_VENDOR_ID_DELL, 0x010a, PHY_ID_BCM5411 }, /* SLIM_MERLOT */

	/* Compaq boards. */
	{ PCI_VENDOR_ID_COMPAQ, 0x007c, PHY_ID_BCM5701 }, /* BANSHEE */
	{ PCI_VENDOR_ID_COMPAQ, 0x009a, PHY_ID_BCM5701 }, /* BANSHEE_2 */
	{ PCI_VENDOR_ID_COMPAQ, 0x007d, 0 },		  /* CHANGELING */
	{ PCI_VENDOR_ID_COMPAQ, 0x0085, PHY_ID_BCM5701 }, /* NC7780 */
	{ PCI_VENDOR_ID_COMPAQ, 0x0099, PHY_ID_BCM5701 }, /* NC7780_2 */

	/* IBM boards. */
	{ PCI_VENDOR_ID_IBM, 0x0281, 0 } /* IBM??? */
};

static inline struct subsys_tbl_ent *lookup_by_subsys(struct tg3 *tp)
{
	int i;
	uint16 subsystem_vendor, subsystem_device;

	subsystem_vendor = g_psBus->read_pci_config(tp->pdev->nBus, tp->pdev->nDevice, tp->pdev->nFunction, PCI_SUBSYSTEM_VENDOR_ID, 2);
	subsystem_device = g_psBus->read_pci_config(tp->pdev->nBus, tp->pdev->nDevice, tp->pdev->nFunction, PCI_SUBSYSTEM_ID, 2);

	for (i = 0; i < ARRAY_SIZE(subsys_id_to_phy_id); i++) {
		if ((subsys_id_to_phy_id[i].subsys_vendor ==
		     subsystem_vendor) &&
		    (subsys_id_to_phy_id[i].subsys_devid ==
		     subsystem_device))
			return &subsys_id_to_phy_id[i];
	}
	return NULL;
}

static void tg3_get_eeprom_hw_cfg(struct tg3 *tp)
{
	u32 val;
	u16 pmcsr, subsystem_vendor;

	/* On some early chips the SRAM cannot be accessed in D3hot state,
	 * so need make sure we're in D0.
	 */
	pmcsr = g_psBus->read_pci_config(tp->pdev->nBus, tp->pdev->nDevice, tp->pdev->nFunction, tp->pm_cap + PCI_PM_CTRL, 2);
	pmcsr &= ~PCI_PM_CTRL_STATE_MASK;
	g_psBus->write_pci_config(tp->pdev->nBus, tp->pdev->nDevice, tp->pdev->nFunction, tp->pm_cap + PCI_PM_CTRL, 2, pmcsr);
	udelay(100);

	/* Make sure register accesses (indirect or otherwise)
	 * will function correctly.
	 */
	g_psBus->write_pci_config(tp->pdev->nBus, tp->pdev->nDevice, tp->pdev->nFunction, TG3PCI_MISC_HOST_CTRL,
					4, tp->misc_host_ctrl);

	/* The memory arbiter has to be enabled in order for SRAM accesses
	 * to succeed.  Normally on powerup the tg3 chip firmware will make
	 * sure it is enabled, but other entities such as system netboot
	 * code might disable it.
	 */
	val = tr32(MEMARB_MODE);
	tw32(MEMARB_MODE, val | MEMARB_MODE_ENABLE);

	tp->phy_id = PHY_ID_INVALID;
	tp->led_ctrl = LED_CTRL_MODE_PHY_1;

	/* Assume an onboard device by default.  */
	tp->tg3_flags |= TG3_FLAG_EEPROM_WRITE_PROT;

	tg3_read_mem(tp, NIC_SRAM_DATA_SIG, &val);
	if (val == NIC_SRAM_DATA_SIG_MAGIC) {
		u32 nic_cfg, led_cfg;
		u32 nic_phy_id, ver, cfg2 = 0, eeprom_phy_id;
		int eeprom_phy_serdes = 0;

		tg3_read_mem(tp, NIC_SRAM_DATA_CFG, &nic_cfg);
		tp->nic_sram_data_cfg = nic_cfg;

		tg3_read_mem(tp, NIC_SRAM_DATA_VER, &ver);
		ver >>= NIC_SRAM_DATA_VER_SHIFT;
		if ((GET_ASIC_REV(tp->pci_chip_rev_id) != ASIC_REV_5700) &&
		    (GET_ASIC_REV(tp->pci_chip_rev_id) != ASIC_REV_5701) &&
		    (GET_ASIC_REV(tp->pci_chip_rev_id) != ASIC_REV_5703) &&
		    (ver > 0) && (ver < 0x100))
			tg3_read_mem(tp, NIC_SRAM_DATA_CFG_2, &cfg2);

		if ((nic_cfg & NIC_SRAM_DATA_CFG_PHY_TYPE_MASK) ==
		    NIC_SRAM_DATA_CFG_PHY_TYPE_FIBER)
			eeprom_phy_serdes = 1;

		tg3_read_mem(tp, NIC_SRAM_DATA_PHY_ID, &nic_phy_id);
		if (nic_phy_id != 0) {
			u32 id1 = nic_phy_id & NIC_SRAM_DATA_PHY_ID1_MASK;
			u32 id2 = nic_phy_id & NIC_SRAM_DATA_PHY_ID2_MASK;

			eeprom_phy_id  = (id1 >> 16) << 10;
			eeprom_phy_id |= (id2 & 0xfc00) << 16;
			eeprom_phy_id |= (id2 & 0x03ff) <<  0;
		} else
			eeprom_phy_id = 0;

		tp->phy_id = eeprom_phy_id;
		if (eeprom_phy_serdes) {
			if (tp->tg3_flags2 & TG3_FLG2_5780_CLASS)
				tp->tg3_flags2 |= TG3_FLG2_MII_SERDES;
			else
				tp->tg3_flags2 |= TG3_FLG2_PHY_SERDES;
		}

		if (tp->tg3_flags2 & TG3_FLG2_5750_PLUS)
			led_cfg = cfg2 & (NIC_SRAM_DATA_CFG_LED_MODE_MASK |
				    SHASTA_EXT_LED_MODE_MASK);
		else
			led_cfg = nic_cfg & NIC_SRAM_DATA_CFG_LED_MODE_MASK;

		switch (led_cfg) {
		default:
		case NIC_SRAM_DATA_CFG_LED_MODE_PHY_1:
			tp->led_ctrl = LED_CTRL_MODE_PHY_1;
			break;

		case NIC_SRAM_DATA_CFG_LED_MODE_PHY_2:
			tp->led_ctrl = LED_CTRL_MODE_PHY_2;
			break;

		case NIC_SRAM_DATA_CFG_LED_MODE_MAC:
			tp->led_ctrl = LED_CTRL_MODE_MAC;

			/* Default to PHY_1_MODE if 0 (MAC_MODE) is
			 * read on some older 5700/5701 bootcode.
			 */
			if (GET_ASIC_REV(tp->pci_chip_rev_id) ==
			    ASIC_REV_5700 ||
			    GET_ASIC_REV(tp->pci_chip_rev_id) ==
			    ASIC_REV_5701)
				tp->led_ctrl = LED_CTRL_MODE_PHY_1;

			break;

		case SHASTA_EXT_LED_SHARED:
			tp->led_ctrl = LED_CTRL_MODE_SHARED;
			if (tp->pci_chip_rev_id != CHIPREV_ID_5750_A0 &&
			    tp->pci_chip_rev_id != CHIPREV_ID_5750_A1)
				tp->led_ctrl |= (LED_CTRL_MODE_PHY_1 |
						 LED_CTRL_MODE_PHY_2);
			break;

		case SHASTA_EXT_LED_MAC:
			tp->led_ctrl = LED_CTRL_MODE_SHASTA_MAC;
			break;

		case SHASTA_EXT_LED_COMBO:
			tp->led_ctrl = LED_CTRL_MODE_COMBO;
			if (tp->pci_chip_rev_id != CHIPREV_ID_5750_A0)
				tp->led_ctrl |= (LED_CTRL_MODE_PHY_1 |
						 LED_CTRL_MODE_PHY_2);
			break;

		};

		subsystem_vendor = g_psBus->read_pci_config(tp->pdev->nBus, tp->pdev->nDevice, tp->pdev->nFunction, PCI_SUBSYSTEM_VENDOR_ID, 2);
		if ((GET_ASIC_REV(tp->pci_chip_rev_id) == ASIC_REV_5700 ||
		     GET_ASIC_REV(tp->pci_chip_rev_id) == ASIC_REV_5701) &&
		    subsystem_vendor == PCI_VENDOR_ID_DELL)
			tp->led_ctrl = LED_CTRL_MODE_PHY_2;

		if (nic_cfg & NIC_SRAM_DATA_CFG_EEPROM_WP)
			tp->tg3_flags |= TG3_FLAG_EEPROM_WRITE_PROT;
		else
			tp->tg3_flags &= ~TG3_FLAG_EEPROM_WRITE_PROT;

		if (nic_cfg & NIC_SRAM_DATA_CFG_ASF_ENABLE) {
			tp->tg3_flags |= TG3_FLAG_ENABLE_ASF;
			if (tp->tg3_flags2 & TG3_FLG2_5750_PLUS)
				tp->tg3_flags2 |= TG3_FLG2_ASF_NEW_HANDSHAKE;
		}
		if (nic_cfg & NIC_SRAM_DATA_CFG_FIBER_WOL)
			tp->tg3_flags |= TG3_FLAG_SERDES_WOL_CAP;

		if (cfg2 & (1 << 17))
			tp->tg3_flags2 |= TG3_FLG2_CAPACITIVE_COUPLING;

		/* serdes signal pre-emphasis in register 0x590 set by */
		/* bootcode if bit 18 is set */
		if (cfg2 & (1 << 18))
			tp->tg3_flags2 |= TG3_FLG2_SERDES_PREEMPHASIS;
	}
}

static int tg3_phy_probe(struct tg3 *tp)
{
	u32 hw_phy_id_1, hw_phy_id_2;
	u32 hw_phy_id, hw_phy_id_masked;
	int err;

	/* Reading the PHY ID register can conflict with ASF
	 * firwmare access to the PHY hardware.
	 */
	err = 0;
	if (tp->tg3_flags & TG3_FLAG_ENABLE_ASF) {
		hw_phy_id = hw_phy_id_masked = PHY_ID_INVALID;
	} else {
		/* Now read the physical PHY_ID from the chip and verify
		 * that it is sane.  If it doesn't look good, we fall back
		 * to either the hard-coded table based PHY_ID and failing
		 * that the value found in the eeprom area.
		 */
		err |= tg3_readphy(tp, MII_PHYSID1, &hw_phy_id_1);
		err |= tg3_readphy(tp, MII_PHYSID2, &hw_phy_id_2);

		hw_phy_id  = (hw_phy_id_1 & 0xffff) << 10;
		hw_phy_id |= (hw_phy_id_2 & 0xfc00) << 16;
		hw_phy_id |= (hw_phy_id_2 & 0x03ff) <<  0;

		hw_phy_id_masked = hw_phy_id & PHY_ID_MASK;
	}

	if (!err && KNOWN_PHY_ID(hw_phy_id_masked)) {
		tp->phy_id = hw_phy_id;
		if (hw_phy_id_masked == PHY_ID_BCM8002)
			tp->tg3_flags2 |= TG3_FLG2_PHY_SERDES;
		else
			tp->tg3_flags2 &= ~TG3_FLG2_PHY_SERDES;
	} else {
		if (tp->phy_id != PHY_ID_INVALID) {
			/* Do nothing, phy ID already set up in
			 * tg3_get_eeprom_hw_cfg().
			 */
		} else {
			struct subsys_tbl_ent *p;

			/* No eeprom signature?  Try the hardcoded
			 * subsys device table.
			 */
			p = lookup_by_subsys(tp);
			if (!p)
				return -ENODEV;

			tp->phy_id = p->phy_id;
			if (!tp->phy_id ||
			    tp->phy_id == PHY_ID_BCM8002)
				tp->tg3_flags2 |= TG3_FLG2_PHY_SERDES;
		}
	}

	if (!(tp->tg3_flags2 & TG3_FLG2_ANY_SERDES) &&
	    !(tp->tg3_flags & TG3_FLAG_ENABLE_ASF)) {
		u32 bmsr, adv_reg, tg3_ctrl;

		tg3_readphy(tp, MII_BMSR, &bmsr);
		if (!tg3_readphy(tp, MII_BMSR, &bmsr) &&
		    (bmsr & BMSR_LSTATUS))
			goto skip_phy_reset;
		    
		err = tg3_phy_reset(tp);
		if (err)
			return err;

		adv_reg = (ADVERTISE_10HALF | ADVERTISE_10FULL |
			   ADVERTISE_100HALF | ADVERTISE_100FULL |
			   ADVERTISE_CSMA | ADVERTISE_PAUSE_CAP);
		tg3_ctrl = 0;
		if (!(tp->tg3_flags & TG3_FLAG_10_100_ONLY)) {
			tg3_ctrl = (MII_TG3_CTRL_ADV_1000_HALF |
				    MII_TG3_CTRL_ADV_1000_FULL);
			if (tp->pci_chip_rev_id == CHIPREV_ID_5701_A0 ||
			    tp->pci_chip_rev_id == CHIPREV_ID_5701_B0)
				tg3_ctrl |= (MII_TG3_CTRL_AS_MASTER |
					     MII_TG3_CTRL_ENABLE_AS_MASTER);
		}

		if (!tg3_copper_is_advertising_all(tp)) {
			tg3_writephy(tp, MII_ADVERTISE, adv_reg);

			if (!(tp->tg3_flags & TG3_FLAG_10_100_ONLY))
				tg3_writephy(tp, MII_TG3_CTRL, tg3_ctrl);

			tg3_writephy(tp, MII_BMCR,
				     BMCR_ANENABLE | BMCR_ANRESTART);
		}
		tg3_phy_set_wirespeed(tp);

		tg3_writephy(tp, MII_ADVERTISE, adv_reg);
		if (!(tp->tg3_flags & TG3_FLAG_10_100_ONLY))
			tg3_writephy(tp, MII_TG3_CTRL, tg3_ctrl);
	}

skip_phy_reset:
	if ((tp->phy_id & PHY_ID_MASK) == PHY_ID_BCM5401) {
		err = tg3_init_5401phy_dsp(tp);
		if (err)
			return err;
	}

	if (!err && ((tp->phy_id & PHY_ID_MASK) == PHY_ID_BCM5401)) {
		err = tg3_init_5401phy_dsp(tp);
	}

	if (tp->tg3_flags2 & TG3_FLG2_ANY_SERDES)
		tp->link_config.advertising =
			(ADVERTISED_1000baseT_Half |
			 ADVERTISED_1000baseT_Full |
			 ADVERTISED_Autoneg |
			 ADVERTISED_FIBRE);
	if (tp->tg3_flags & TG3_FLAG_10_100_ONLY)
		tp->link_config.advertising &=
			~(ADVERTISED_1000baseT_Half |
			  ADVERTISED_1000baseT_Full);

	return err;
}

static void tg3_read_partno(struct tg3 *tp)
{
	unsigned char vpd_data[256];
	int i;
	u32 magic;

	if (tg3_nvram_read_swab(tp, 0x0, &magic))
		goto out_not_found;

	if (magic == TG3_EEPROM_MAGIC) {
		for (i = 0; i < 256; i += 4) {
			u32 tmp;

			if (tg3_nvram_read(tp, 0x100 + i, &tmp))
				goto out_not_found;

			vpd_data[i + 0] = ((tmp >>  0) & 0xff);
			vpd_data[i + 1] = ((tmp >>  8) & 0xff);
			vpd_data[i + 2] = ((tmp >> 16) & 0xff);
			vpd_data[i + 3] = ((tmp >> 24) & 0xff);
		}
	} else {
		int vpd_cap;

		vpd_cap = g_psBus->get_pci_capability(tp->pdev->nBus, tp->pdev->nDevice, tp->pdev->nFunction, PCI_CAP_ID_VPD);
		for (i = 0; i < 256; i += 4) {
			u32 tmp, j = 0;
			u16 tmp16;

			g_psBus->write_pci_config(tp->pdev->nBus, tp->pdev->nDevice, tp->pdev->nFunction, vpd_cap + PCI_VPD_ADDR, 2, i);
			while (j++ < 100) {
				tmp16 = g_psBus->read_pci_config(tp->pdev->nBus, tp->pdev->nDevice, tp->pdev->nFunction, vpd_cap + PCI_VPD_ADDR, 4);
				if (tmp16 & 0x8000)
					break;
				udelay(100);
			}
			if (!(tmp16 & 0x8000))
				goto out_not_found;

			tmp = g_psBus->read_pci_config(tp->pdev->nBus, tp->pdev->nDevice, tp->pdev->nFunction, vpd_cap + PCI_VPD_DATA, 4);
			tmp = cpu_to_le32(tmp);
			memcpy(&vpd_data[i], &tmp, 4);
		}
	}

	/* Now parse and find the part number. */
	for (i = 0; i < 256; ) {
		unsigned char val = vpd_data[i];
		int block_end;

		if (val == 0x82 || val == 0x91) {
			i = (i + 3 +
			     (vpd_data[i + 1] +
			      (vpd_data[i + 2] << 8)));
			continue;
		}

		if (val != 0x90)
			goto out_not_found;

		block_end = (i + 3 +
			     (vpd_data[i + 1] +
			      (vpd_data[i + 2] << 8)));
		i += 3;
		while (i < block_end) {
			if (vpd_data[i + 0] == 'P' &&
			    vpd_data[i + 1] == 'N') {
				int partno_len = vpd_data[i + 2];

				if (partno_len > 24)
					goto out_not_found;

				memcpy(tp->board_part_number,
				       &vpd_data[i + 3],
				       partno_len);

				/* Success. */
				return;
			}
		}

		/* Part number not found. */
		goto out_not_found;
	}

out_not_found:
	strcpy(tp->board_part_number, "none");
}

static void tg3_read_fw_ver(struct tg3 *tp)
{
	u32 val, offset, start;

	if (tg3_nvram_read_swab(tp, 0, &val))
		return;

	if (val != TG3_EEPROM_MAGIC)
		return;

	if (tg3_nvram_read_swab(tp, 0xc, &offset) ||
	    tg3_nvram_read_swab(tp, 0x4, &start))
		return;

	offset = tg3_nvram_logical_addr(tp, offset);
	if (tg3_nvram_read_swab(tp, offset, &val))
		return;

	if ((val & 0xfc000000) == 0x0c000000) {
		u32 ver_offset, addr;
		int i;

		if (tg3_nvram_read_swab(tp, offset + 4, &val) ||
		    tg3_nvram_read_swab(tp, offset + 8, &ver_offset))
			return;

		if (val != 0)
			return;

		addr = offset + ver_offset - start;
		for (i = 0; i < 16; i += 4) {
			if (tg3_nvram_read(tp, addr + i, &val))
				return;

			val = cpu_to_le32(val);
			memcpy(tp->fw_ver + i, &val, 4);
		}
	}
}

static int tg3_get_invariants(struct tg3 *tp)
{
	static struct pci_device_id write_reorder_chipsets[] = {
		{ PCI_VENDOR_ID_AMD,
		             PCI_DEVICE_ID_AMD_FE_GATE_700C },
		{ PCI_VENDOR_ID_AMD,
		             PCI_DEVICE_ID_AMD_8131_BRIDGE },
		{ PCI_VENDOR_ID_VIA,
			     PCI_DEVICE_ID_VIA_8385_0 },
		{ },
	};
	u32 misc_ctrl_reg;
	u32 cacheline_sz_reg;
	u32 pci_state_reg, grc_misc_cfg;
	u32 val;
	u16 pci_cmd, subsystem_vendor;
	int err;

	/* Force memory write invalidate off.  If we leave it on,
	 * then on 5700_BX chips we have to enable a workaround.
	 * The workaround is to set the TG3PCI_DMA_RW_CTRL boundary
	 * to match the cacheline size.  The Broadcom driver have this
	 * workaround but turns MWI off all the times so never uses
	 * it.  This seems to suggest that the workaround is insufficient.
	 */
	pci_cmd = g_psBus->read_pci_config(tp->pdev->nBus, tp->pdev->nDevice, tp->pdev->nFunction, PCI_COMMAND, 2);
	pci_cmd &= ~PCI_COMMAND_MWI;
	g_psBus->write_pci_config(tp->pdev->nBus, tp->pdev->nDevice, tp->pdev->nFunction, PCI_COMMAND, 2, pci_cmd);

	/* It is absolutely critical that TG3PCI_MISC_HOST_CTRL
	 * has the register indirect write enable bit set before
	 * we try to access any of the MMIO registers.  It is also
	 * critical that the PCI-X hw workaround situation is decided
	 * before that as well.
	 */
	misc_ctrl_reg = g_psBus->read_pci_config(tp->pdev->nBus, tp->pdev->nDevice, tp->pdev->nFunction, TG3PCI_MISC_HOST_CTRL, 4);

	tp->pci_chip_rev_id = (misc_ctrl_reg >>
			       MISC_HOST_CTRL_CHIPREV_SHIFT);

	/* Wrong chip ID in 5752 A0. This code can be removed later
	 * as A0 is not in production.
	 */
	if (tp->pci_chip_rev_id == CHIPREV_ID_5752_A0_HW)
		tp->pci_chip_rev_id = CHIPREV_ID_5752_A0;

	/* If we have 5702/03 A1 or A2 on certain ICH chipsets,
	 * we need to disable memory and use config. cycles
	 * only to access all registers. The 5702/03 chips
	 * can mistakenly decode the special cycles from the
	 * ICH chipsets as memory write cycles, causing corruption
	 * of register and memory space. Only certain ICH bridges
	 * will drive special cycles with non-zero data during the
	 * address phase which can fall within the 5703's address
	 * range. This is not an ICH bug as the PCI spec allows
	 * non-zero address during special cycles. However, only
	 * these ICH bridges are known to drive non-zero addresses
	 * during special cycles.
	 *
	 * Since special cycles do not cross PCI bridges, we only
	 * enable this workaround if the 5703 is on the secondary
	 * bus of these ICH bridges.
	 */
	if ((tp->pci_chip_rev_id == CHIPREV_ID_5703_A1) ||
	    (tp->pci_chip_rev_id == CHIPREV_ID_5703_A2)) {
		static struct tg3_dev_id {
			u32	vendor;
			u32	device;
			u32	rev;
		} ich_chipsets[] = {
			{ PCI_VENDOR_ID_INTEL, PCI_DEVICE_ID_INTEL_82801AA_8,
			  PCI_ANY_ID },
			{ PCI_VENDOR_ID_INTEL, PCI_DEVICE_ID_INTEL_82801AB_8,
			  PCI_ANY_ID },
			{ PCI_VENDOR_ID_INTEL, PCI_DEVICE_ID_INTEL_82801BA_11,
			  0xa },
			{ PCI_VENDOR_ID_INTEL, PCI_DEVICE_ID_INTEL_82801BA_6,
			  PCI_ANY_ID },
			{ },
		};
		struct tg3_dev_id *pci_id = &ich_chipsets[0];
		struct pci_dev *bridge = NULL;

/* XXXKV: On Syllable we'll have to emulate pci_get_device() or find some other way to do it */
#if 0
		while (pci_id->vendor != 0) {
			bridge = pci_get_device(pci_id->vendor, pci_id->device,
						bridge);
			if (!bridge) {
				pci_id++;
				continue;
			}
			if (pci_id->rev != PCI_ANY_ID) {
				u8 rev;

				rev = g_psBus->read_pci_config(tp->pdev->nBus, tp->pdev->nDevice, tp->pdev->nFunction, PCI_REVISION, 1);
				if (rev > pci_id->rev)
					continue;
			}
			if (bridge->subordinate &&
			    (bridge->subordinate->number ==
			     tp->pdev->nBus->number)) {

				tp->tg3_flags2 |= TG3_FLG2_ICH_WORKAROUND;
				pci_dev_put(bridge);
				break;
			}
		}
#endif
	}

	/* The EPB bridge inside 5714, 5715, and 5780 cannot support
	 * DMA addresses > 40-bit. This bridge may have other additional
	 * 57xx devices behind it in some 4-port NIC designs for example.
	 * Any tg3 device found behind the bridge will also need the 40-bit
	 * DMA workaround.
	 */
	if (GET_ASIC_REV(tp->pci_chip_rev_id) == ASIC_REV_5780 ||
	    GET_ASIC_REV(tp->pci_chip_rev_id) == ASIC_REV_5714) {
		tp->tg3_flags2 |= TG3_FLG2_5780_CLASS;
		tp->tg3_flags |= TG3_FLAG_40BIT_DMA_BUG;
		tp->msi_cap = g_psBus->get_pci_capability(tp->pdev->nBus, tp->pdev->nDevice, tp->pdev->nFunction, PCI_CAP_ID_MSI);
	}
	else {

/* XXXKV: On Syllable we'll have to emulate pci_get_device() or find some other way to do it */
#if 0
		struct pci_dev *bridge = NULL;

		do {
			bridge = pci_get_device(PCI_VENDOR_ID_SERVERWORKS,
						PCI_DEVICE_ID_SERVERWORKS_EPB,
						bridge);
			if (bridge && bridge->subordinate &&
			    (bridge->subordinate->number <=
			     tp->pdev->nBus->number) &&
			    (bridge->subordinate->subordinate >=
			     tp->pdev->nBus->number)) {
				tp->tg3_flags |= TG3_FLAG_40BIT_DMA_BUG;
				pci_dev_put(bridge);
				break;
			}
		} while (bridge);
#endif
	}

	/* Initialize misc host control in PCI block. */
	tp->misc_host_ctrl |= (misc_ctrl_reg &
			       MISC_HOST_CTRL_CHIPREV);
	g_psBus->write_pci_config(tp->pdev->nBus, tp->pdev->nDevice, tp->pdev->nFunction, TG3PCI_MISC_HOST_CTRL,
					4, tp->misc_host_ctrl);

	cacheline_sz_reg = g_psBus->read_pci_config(tp->pdev->nBus, tp->pdev->nDevice, tp->pdev->nFunction, TG3PCI_CACHELINESZ, 4);

	tp->pci_cacheline_sz = (cacheline_sz_reg >>  0) & 0xff;
	tp->pci_lat_timer    = (cacheline_sz_reg >>  8) & 0xff;
	tp->pci_hdr_type     = (cacheline_sz_reg >> 16) & 0xff;
	tp->pci_bist         = (cacheline_sz_reg >> 24) & 0xff;

	if (GET_ASIC_REV(tp->pci_chip_rev_id) == ASIC_REV_5750 ||
	    GET_ASIC_REV(tp->pci_chip_rev_id) == ASIC_REV_5752 ||
	    GET_ASIC_REV(tp->pci_chip_rev_id) == ASIC_REV_5755 ||
	    GET_ASIC_REV(tp->pci_chip_rev_id) == ASIC_REV_5787 ||
	    (tp->tg3_flags2 & TG3_FLG2_5780_CLASS))
		tp->tg3_flags2 |= TG3_FLG2_5750_PLUS;

	if ((GET_ASIC_REV(tp->pci_chip_rev_id) == ASIC_REV_5705) ||
	    (tp->tg3_flags2 & TG3_FLG2_5750_PLUS))
		tp->tg3_flags2 |= TG3_FLG2_5705_PLUS;

	if (tp->tg3_flags2 & TG3_FLG2_5750_PLUS) {
		if (GET_ASIC_REV(tp->pci_chip_rev_id) == ASIC_REV_5755 ||
		    GET_ASIC_REV(tp->pci_chip_rev_id) == ASIC_REV_5787) {
			tp->tg3_flags2 |= TG3_FLG2_HW_TSO_2;
			tp->tg3_flags2 |= TG3_FLG2_1SHOT_MSI;
		} else {
			tp->tg3_flags2 |= TG3_FLG2_HW_TSO_1 |
					  TG3_FLG2_HW_TSO_1_BUG;
			if (GET_ASIC_REV(tp->pci_chip_rev_id) ==
				ASIC_REV_5750 &&
	     		    tp->pci_chip_rev_id >= CHIPREV_ID_5750_C2)
				tp->tg3_flags2 &= ~TG3_FLG2_HW_TSO_1_BUG;
		}
	}

	if (GET_ASIC_REV(tp->pci_chip_rev_id) != ASIC_REV_5705 &&
	    GET_ASIC_REV(tp->pci_chip_rev_id) != ASIC_REV_5750 &&
	    GET_ASIC_REV(tp->pci_chip_rev_id) != ASIC_REV_5752 &&
	    GET_ASIC_REV(tp->pci_chip_rev_id) != ASIC_REV_5755 &&
	    GET_ASIC_REV(tp->pci_chip_rev_id) != ASIC_REV_5787)
		tp->tg3_flags2 |= TG3_FLG2_JUMBO_CAPABLE;

	if (g_psBus->get_pci_capability(tp->pdev->nBus, tp->pdev->nDevice, tp->pdev->nFunction, PCI_CAP_ID_EXP) != 0)
		tp->tg3_flags2 |= TG3_FLG2_PCI_EXPRESS;

	/* If we have an AMD 762 or VIA K8T800 chipset, write
	 * reordering to the mailbox registers done by the host
	 * controller can cause major troubles.  We read back from
	 * every mailbox register write to force the writes to be
	 * posted to the chip in order.
	 */
/* XXXKV: On Syllable we'll have to emulate pci_dev_present() or find some other way to do it */
#if 0
	if (pci_dev_present(write_reorder_chipsets) &&
	    !(tp->tg3_flags2 & TG3_FLG2_PCI_EXPRESS))
		tp->tg3_flags |= TG3_FLAG_MBOX_WRITE_REORDER;
#endif

	if (GET_ASIC_REV(tp->pci_chip_rev_id) == ASIC_REV_5703 &&
	    tp->pci_lat_timer < 64) {
		tp->pci_lat_timer = 64;

		cacheline_sz_reg  = ((tp->pci_cacheline_sz & 0xff) <<  0);
		cacheline_sz_reg |= ((tp->pci_lat_timer    & 0xff) <<  8);
		cacheline_sz_reg |= ((tp->pci_hdr_type     & 0xff) << 16);
		cacheline_sz_reg |= ((tp->pci_bist         & 0xff) << 24);

		g_psBus->write_pci_config(tp->pdev->nBus, tp->pdev->nDevice, tp->pdev->nFunction, TG3PCI_CACHELINESZ, 4,
						cacheline_sz_reg);
	}

	pci_state_reg = g_psBus->read_pci_config(tp->pdev->nBus, tp->pdev->nDevice, tp->pdev->nFunction, TG3PCI_PCISTATE, 4);

	if ((pci_state_reg & PCISTATE_CONV_PCI_MODE) == 0) {
		tp->tg3_flags |= TG3_FLAG_PCIX_MODE;

		/* If this is a 5700 BX chipset, and we are in PCI-X
		 * mode, enable register write workaround.
		 *
		 * The workaround is to use indirect register accesses
		 * for all chip writes not to mailbox registers.
		 */
		if (GET_CHIP_REV(tp->pci_chip_rev_id) == CHIPREV_5700_BX) {
			u32 pm_reg;
			u16 pci_cmd;

			tp->tg3_flags |= TG3_FLAG_PCIX_TARGET_HWBUG;

			/* The chip can have it's power management PCI config
			 * space registers clobbered due to this bug.
			 * So explicitly force the chip into D0 here.
			 */
			pm_reg = g_psBus->read_pci_config(tp->pdev->nBus, tp->pdev->nDevice, tp->pdev->nFunction, TG3PCI_PM_CTRL_STAT, 4);
			pm_reg &= ~PCI_PM_CTRL_STATE_MASK;
			pm_reg |= PCI_PM_CTRL_PME_ENABLE | 0 /* D0 */;
			g_psBus->write_pci_config(tp->pdev->nBus, tp->pdev->nDevice, tp->pdev->nFunction, TG3PCI_PM_CTRL_STAT, 4,
							pm_reg);

			/* Also, force SERR#/PERR# in PCI command. */
			pci_cmd = g_psBus->read_pci_config(tp->pdev->nBus, tp->pdev->nDevice, tp->pdev->nFunction, PCI_COMMAND, 2);
			pci_cmd |= PCI_COMMAND_PARITY | PCI_COMMAND_SERR;
			g_psBus->write_pci_config(tp->pdev->nBus, tp->pdev->nDevice, tp->pdev->nFunction, PCI_COMMAND, 2, pci_cmd);
		}
	}

	/* 5700 BX chips need to have their TX producer index mailboxes
	 * written twice to workaround a bug.
	 */
	if (GET_CHIP_REV(tp->pci_chip_rev_id) == CHIPREV_5700_BX)
		tp->tg3_flags |= TG3_FLAG_TXD_MBOX_HWBUG;

	/* Back to back register writes can cause problems on this chip,
	 * the workaround is to read back all reg writes except those to
	 * mailbox regs.  See tg3_write_indirect_reg32().
	 *
	 * PCI Express 5750_A0 rev chips need this workaround too.
	 */
	if (GET_ASIC_REV(tp->pci_chip_rev_id) == ASIC_REV_5701 ||
	    ((tp->tg3_flags2 & TG3_FLG2_PCI_EXPRESS) &&
	     tp->pci_chip_rev_id == CHIPREV_ID_5750_A0))
		tp->tg3_flags |= TG3_FLAG_5701_REG_WRITE_BUG;

	if ((pci_state_reg & PCISTATE_BUS_SPEED_HIGH) != 0)
		tp->tg3_flags |= TG3_FLAG_PCI_HIGH_SPEED;
	if ((pci_state_reg & PCISTATE_BUS_32BIT) != 0)
		tp->tg3_flags |= TG3_FLAG_PCI_32BIT;

	/* Chip-specific fixup from Broadcom driver */
	if ((tp->pci_chip_rev_id == CHIPREV_ID_5704_A0) &&
	    (!(pci_state_reg & PCISTATE_RETRY_SAME_DMA))) {
		pci_state_reg |= PCISTATE_RETRY_SAME_DMA;
		g_psBus->write_pci_config(tp->pdev->nBus, tp->pdev->nDevice, tp->pdev->nFunction, TG3PCI_PCISTATE, 4, pci_state_reg);
	}

	/* Default fast path register access methods */
	tp->read32 = tg3_read32;
	tp->write32 = tg3_write32;
	tp->read32_mbox = tg3_read32;
	tp->write32_mbox = tg3_write32;
	tp->write32_tx_mbox = tg3_write32;
	tp->write32_rx_mbox = tg3_write32;

	/* Various workaround register access methods */
	if (tp->tg3_flags & TG3_FLAG_PCIX_TARGET_HWBUG)
		tp->write32 = tg3_write_indirect_reg32;
	else if (tp->tg3_flags & TG3_FLAG_5701_REG_WRITE_BUG)
		tp->write32 = tg3_write_flush_reg32;

	if ((tp->tg3_flags & TG3_FLAG_TXD_MBOX_HWBUG) ||
	    (tp->tg3_flags & TG3_FLAG_MBOX_WRITE_REORDER)) {
		tp->write32_tx_mbox = tg3_write32_tx_mbox;
		if (tp->tg3_flags & TG3_FLAG_MBOX_WRITE_REORDER)
			tp->write32_rx_mbox = tg3_write_flush_reg32;
	}

	if (tp->tg3_flags2 & TG3_FLG2_ICH_WORKAROUND) {
		tp->read32 = tg3_read_indirect_reg32;
		tp->write32 = tg3_write_indirect_reg32;
		tp->read32_mbox = tg3_read_indirect_mbox;
		tp->write32_mbox = tg3_write_indirect_mbox;
		tp->write32_tx_mbox = tg3_write_indirect_mbox;
		tp->write32_rx_mbox = tg3_write_indirect_mbox;

		delete_area(tp->reg_area);
		tp->regs = NULL;

		pci_cmd = g_psBus->read_pci_config(tp->pdev->nBus, tp->pdev->nDevice, tp->pdev->nFunction, PCI_COMMAND, 2);
		pci_cmd &= ~PCI_COMMAND_MEMORY;
		g_psBus->write_pci_config(tp->pdev->nBus, tp->pdev->nDevice, tp->pdev->nFunction, PCI_COMMAND, 2, pci_cmd);
	}

	if (tp->write32 == tg3_write_indirect_reg32 ||
	    ((tp->tg3_flags & TG3_FLAG_PCIX_MODE) &&
	     (GET_ASIC_REV(tp->pci_chip_rev_id) == ASIC_REV_5700 ||
	      GET_ASIC_REV(tp->pci_chip_rev_id) == ASIC_REV_5701)))
		tp->tg3_flags |= TG3_FLAG_SRAM_USE_CONFIG;

	/* Get eeprom hw config before calling tg3_set_power_state().
	 * In particular, the TG3_FLAG_EEPROM_WRITE_PROT flag must be
	 * determined before calling tg3_set_power_state() so that
	 * we know whether or not to switch out of Vaux power.
	 * When the flag is set, it means that GPIO1 is used for eeprom
	 * write protect and also implies that it is a LOM where GPIOs
	 * are not used to switch power.
	 */ 
	tg3_get_eeprom_hw_cfg(tp);

	/* Set up tp->grc_local_ctrl before calling tg3_set_power_state().
	 * GPIO1 driven high will bring 5700's external PHY out of reset.
	 * It is also used as eeprom write protect on LOMs.
	 */
	tp->grc_local_ctrl = GRC_LCLCTRL_INT_ON_ATTN | GRC_LCLCTRL_AUTO_SEEPROM;
	if ((GET_ASIC_REV(tp->pci_chip_rev_id) == ASIC_REV_5700) ||
	    (tp->tg3_flags & TG3_FLAG_EEPROM_WRITE_PROT))
		tp->grc_local_ctrl |= (GRC_LCLCTRL_GPIO_OE1 |
				       GRC_LCLCTRL_GPIO_OUTPUT1);
	/* Unused GPIO3 must be driven as output on 5752 because there
	 * are no pull-up resistors on unused GPIO pins.
	 */
	else if (GET_ASIC_REV(tp->pci_chip_rev_id) == ASIC_REV_5752)
		tp->grc_local_ctrl |= GRC_LCLCTRL_GPIO_OE3;

	if (GET_ASIC_REV(tp->pci_chip_rev_id) == ASIC_REV_5755)
		tp->grc_local_ctrl |= GRC_LCLCTRL_GPIO_UART_SEL;

	/* Force the chip into D0. */
	err = tg3_set_power_state(tp, PCI_D0);
	if (err) {
		kerndbg( KERN_WARNING, "(%s) transition to D0 failed\n",
		       tp->dev->name);
		return err;
	}

	/* 5700 B0 chips do not support checksumming correctly due
	 * to hardware bugs.
	 */
	if (tp->pci_chip_rev_id == CHIPREV_ID_5700_B0)
		tp->tg3_flags |= TG3_FLAG_BROKEN_CHECKSUMS;

	/* Derive initial jumbo mode from MTU assigned in
	 * ether_setup() via the alloc_etherdev() call
	 */
	if (tp->dev->mtu > ETH_DATA_LEN &&
	    !(tp->tg3_flags2 & TG3_FLG2_5780_CLASS))
		tp->tg3_flags |= TG3_FLAG_JUMBO_RING_ENABLE;

	/* Determine WakeOnLan speed to use. */
	if (GET_ASIC_REV(tp->pci_chip_rev_id) == ASIC_REV_5700 ||
	    tp->pci_chip_rev_id == CHIPREV_ID_5701_A0 ||
	    tp->pci_chip_rev_id == CHIPREV_ID_5701_B0 ||
	    tp->pci_chip_rev_id == CHIPREV_ID_5701_B2) {
		tp->tg3_flags &= ~(TG3_FLAG_WOL_SPEED_100MB);
	} else {
		tp->tg3_flags |= TG3_FLAG_WOL_SPEED_100MB;
	}

	/* A few boards don't want Ethernet@WireSpeed phy feature */
	if ((GET_ASIC_REV(tp->pci_chip_rev_id) == ASIC_REV_5700) ||
	    ((GET_ASIC_REV(tp->pci_chip_rev_id) == ASIC_REV_5705) &&
	     (tp->pci_chip_rev_id != CHIPREV_ID_5705_A0) &&
	     (tp->pci_chip_rev_id != CHIPREV_ID_5705_A1)) ||
	    (tp->tg3_flags2 & TG3_FLG2_ANY_SERDES))
		tp->tg3_flags2 |= TG3_FLG2_NO_ETH_WIRE_SPEED;

	if (GET_CHIP_REV(tp->pci_chip_rev_id) == CHIPREV_5703_AX ||
	    GET_CHIP_REV(tp->pci_chip_rev_id) == CHIPREV_5704_AX)
		tp->tg3_flags2 |= TG3_FLG2_PHY_ADC_BUG;
	if (tp->pci_chip_rev_id == CHIPREV_ID_5704_A0)
		tp->tg3_flags2 |= TG3_FLG2_PHY_5704_A0_BUG;

	if (tp->tg3_flags2 & TG3_FLG2_5705_PLUS) {
		if (GET_ASIC_REV(tp->pci_chip_rev_id) == ASIC_REV_5755 ||
		    GET_ASIC_REV(tp->pci_chip_rev_id) == ASIC_REV_5787)
			tp->tg3_flags2 |= TG3_FLG2_PHY_JITTER_BUG;
		else
			tp->tg3_flags2 |= TG3_FLG2_PHY_BER_BUG;
	}

	tp->coalesce_mode = 0;
	if (GET_CHIP_REV(tp->pci_chip_rev_id) != CHIPREV_5700_AX &&
	    GET_CHIP_REV(tp->pci_chip_rev_id) != CHIPREV_5700_BX)
		tp->coalesce_mode |= HOSTCC_MODE_32BYTE;

	/* Initialize MAC MI mode, polling disabled. */
	tw32_f(MAC_MI_MODE, tp->mi_mode);
	udelay(80);

	/* Initialize data/descriptor byte/word swapping. */
	val = tr32(GRC_MODE);
	val &= GRC_MODE_HOST_STACKUP;
	tw32(GRC_MODE, val | tp->grc_mode);

	tg3_switch_clocks(tp);

	/* Clear this out for sanity. */
	tw32(TG3PCI_MEM_WIN_BASE_ADDR, 0);

	pci_state_reg = g_psBus->read_pci_config(tp->pdev->nBus, tp->pdev->nDevice, tp->pdev->nFunction, TG3PCI_PCISTATE, 4);

	if ((pci_state_reg & PCISTATE_CONV_PCI_MODE) == 0 &&
	    (tp->tg3_flags & TG3_FLAG_PCIX_TARGET_HWBUG) == 0) {
		u32 chiprevid = GET_CHIP_REV_ID(tp->misc_host_ctrl);

		if (chiprevid == CHIPREV_ID_5701_A0 ||
		    chiprevid == CHIPREV_ID_5701_B0 ||
		    chiprevid == CHIPREV_ID_5701_B2 ||
		    chiprevid == CHIPREV_ID_5701_B5) {
			void *sram_base;

			/* Write some dummy words into the SRAM status block
			 * area, see if it reads back correctly.  If the return
			 * value is bad, force enable the PCIX workaround.
			 */
			sram_base = tp->regs + NIC_SRAM_WIN_BASE + NIC_SRAM_STATS_BLK;

			writel(0x00000000, sram_base);
			writel(0x00000000, sram_base + 4);
			writel(0xffffffff, sram_base + 4);
			if (readl(sram_base) != 0x00000000)
				tp->tg3_flags |= TG3_FLAG_PCIX_TARGET_HWBUG;
		}
	}

	udelay(50);
	tg3_nvram_init(tp);

	grc_misc_cfg = tr32(GRC_MISC_CFG);
	grc_misc_cfg &= GRC_MISC_CFG_BOARD_ID_MASK;

	/* Broadcom's driver says that CIOBE multisplit has a bug */
#if 0
	if (GET_ASIC_REV(tp->pci_chip_rev_id) == ASIC_REV_5704 &&
	    grc_misc_cfg == GRC_MISC_CFG_BOARD_ID_5704CIOBE) {
		tp->tg3_flags |= TG3_FLAG_SPLIT_MODE;
		tp->split_mode_max_reqs = SPLIT_MODE_5704_MAX_REQ;
	}
#endif
	if (GET_ASIC_REV(tp->pci_chip_rev_id) == ASIC_REV_5705 &&
	    (grc_misc_cfg == GRC_MISC_CFG_BOARD_ID_5788 ||
	     grc_misc_cfg == GRC_MISC_CFG_BOARD_ID_5788M))
		tp->tg3_flags2 |= TG3_FLG2_IS_5788;

	if (!(tp->tg3_flags2 & TG3_FLG2_IS_5788) &&
	    (GET_ASIC_REV(tp->pci_chip_rev_id) != ASIC_REV_5700))
		tp->tg3_flags |= TG3_FLAG_TAGGED_STATUS;
	if (tp->tg3_flags & TG3_FLAG_TAGGED_STATUS) {
		tp->coalesce_mode |= (HOSTCC_MODE_CLRTICK_RXBD |
				      HOSTCC_MODE_CLRTICK_TXBD);

		tp->misc_host_ctrl |= MISC_HOST_CTRL_TAGGED_STATUS;
		g_psBus->write_pci_config(tp->pdev->nBus, tp->pdev->nDevice, tp->pdev->nFunction, TG3PCI_MISC_HOST_CTRL,
						4, tp->misc_host_ctrl);
	}

	/* these are limited to 10/100 only */
	if ((GET_ASIC_REV(tp->pci_chip_rev_id) == ASIC_REV_5703 &&
	     (grc_misc_cfg == 0x8000 || grc_misc_cfg == 0x4000)) ||
	    (GET_ASIC_REV(tp->pci_chip_rev_id) == ASIC_REV_5705 &&
	     tp->pdev->nVendorID == PCI_VENDOR_ID_BROADCOM &&
	     (tp->pdev->nDeviceID == PCI_DEVICE_ID_TIGON3_5901 ||
	      tp->pdev->nDeviceID == PCI_DEVICE_ID_TIGON3_5901_2 ||
	      tp->pdev->nDeviceID == PCI_DEVICE_ID_TIGON3_5705F)) ||
	    (tp->pdev->nVendorID == PCI_VENDOR_ID_BROADCOM &&
	     (tp->pdev->nDeviceID == PCI_DEVICE_ID_TIGON3_5751F ||
	      tp->pdev->nDeviceID == PCI_DEVICE_ID_TIGON3_5753F)))
		tp->tg3_flags |= TG3_FLAG_10_100_ONLY;

	err = tg3_phy_probe(tp);
	if (err) {
		kerndbg( KERN_WARNING, "(%s) phy probe failed, err %d\n",
		       tp->dev->name, err);
		/* ... but do not return immediately ... */
	}

	tg3_read_partno(tp);
	tg3_read_fw_ver(tp);

	if (tp->tg3_flags2 & TG3_FLG2_PHY_SERDES) {
		tp->tg3_flags &= ~TG3_FLAG_USE_MI_INTERRUPT;
	} else {
		if (GET_ASIC_REV(tp->pci_chip_rev_id) == ASIC_REV_5700)
			tp->tg3_flags |= TG3_FLAG_USE_MI_INTERRUPT;
		else
			tp->tg3_flags &= ~TG3_FLAG_USE_MI_INTERRUPT;
	}

	/* 5700 {AX,BX} chips have a broken status block link
	 * change bit implementation, so we must use the
	 * status register in those cases.
	 */
	if (GET_ASIC_REV(tp->pci_chip_rev_id) == ASIC_REV_5700)
		tp->tg3_flags |= TG3_FLAG_USE_LINKCHG_REG;
	else
		tp->tg3_flags &= ~TG3_FLAG_USE_LINKCHG_REG;

	/* The led_ctrl is set during tg3_phy_probe, here we might
	 * have to force the link status polling mechanism based
	 * upon subsystem IDs.
	 */
	subsystem_vendor = g_psBus->read_pci_config(tp->pdev->nBus, tp->pdev->nDevice, tp->pdev->nFunction, PCI_SUBSYSTEM_VENDOR_ID, 2);
	if (subsystem_vendor == PCI_VENDOR_ID_DELL &&
	    !(tp->tg3_flags2 & TG3_FLG2_PHY_SERDES)) {
		tp->tg3_flags |= (TG3_FLAG_USE_MI_INTERRUPT |
				  TG3_FLAG_USE_LINKCHG_REG);
	}

	/* For all SERDES we poll the MAC status register. */
	if (tp->tg3_flags2 & TG3_FLG2_PHY_SERDES)
		tp->tg3_flags |= TG3_FLAG_POLL_SERDES;
	else
		tp->tg3_flags &= ~TG3_FLAG_POLL_SERDES;

	/* All chips before 5787 can get confused if TX buffers
	 * straddle the 4GB address boundary in some cases.
	 */
	if (GET_ASIC_REV(tp->pci_chip_rev_id) == ASIC_REV_5755 ||
	    GET_ASIC_REV(tp->pci_chip_rev_id) == ASIC_REV_5787)
		tp->dev->hard_start_xmit = tg3_start_xmit;
	else
		tp->dev->hard_start_xmit = tg3_start_xmit_dma_bug;

	tp->rx_offset = 2;
	if (GET_ASIC_REV(tp->pci_chip_rev_id) == ASIC_REV_5701 &&
	    (tp->tg3_flags & TG3_FLAG_PCIX_MODE) != 0)
		tp->rx_offset = 0;

	tp->rx_std_max_post = TG3_RX_RING_SIZE;

	/* Increment the rx prod index on the rx std ring by at most
	 * 8 for these chips to workaround hw errata.
	 */
	if (GET_ASIC_REV(tp->pci_chip_rev_id) == ASIC_REV_5750 ||
	    GET_ASIC_REV(tp->pci_chip_rev_id) == ASIC_REV_5752 ||
	    GET_ASIC_REV(tp->pci_chip_rev_id) == ASIC_REV_5755)
		tp->rx_std_max_post = 8;

	/* By default, disable wake-on-lan.  User can change this
	 * using ETHTOOL_SWOL.
	 */
	tp->tg3_flags &= ~TG3_FLAG_WOL_ENABLE;

	return err;
}

static int tg3_get_device_address(struct tg3 *tp)
{
	struct net_device *dev = tp->dev;
	u32 hi, lo, mac_offset;
	int addr_ok = 0;

	mac_offset = 0x7c;
	if ((GET_ASIC_REV(tp->pci_chip_rev_id) == ASIC_REV_5704) ||
	    (tp->tg3_flags2 & TG3_FLG2_5780_CLASS)) {
		if (tr32(TG3PCI_DUAL_MAC_CTRL) & DUAL_MAC_CTRL_ID)
			mac_offset = 0xcc;
		if (tg3_nvram_lock(tp))
			tw32_f(NVRAM_CMD, NVRAM_CMD_RESET);
		else
			tg3_nvram_unlock(tp);
	}

	/* First try to get it from MAC address mailbox. */
	tg3_read_mem(tp, NIC_SRAM_MAC_ADDR_HIGH_MBOX, &hi);
	if ((hi >> 16) == 0x484b) {
		dev->dev_addr[0] = (hi >>  8) & 0xff;
		dev->dev_addr[1] = (hi >>  0) & 0xff;

		tg3_read_mem(tp, NIC_SRAM_MAC_ADDR_LOW_MBOX, &lo);
		dev->dev_addr[2] = (lo >> 24) & 0xff;
		dev->dev_addr[3] = (lo >> 16) & 0xff;
		dev->dev_addr[4] = (lo >>  8) & 0xff;
		dev->dev_addr[5] = (lo >>  0) & 0xff;

		/* Some old bootcode may report a 0 MAC address in SRAM */
		addr_ok = is_valid_ether_addr(&dev->dev_addr[0]);
	}
	if (!addr_ok) {
		/* Next, try NVRAM. */
		if (!tg3_nvram_read(tp, mac_offset + 0, &hi) &&
		    !tg3_nvram_read(tp, mac_offset + 4, &lo)) {
			dev->dev_addr[0] = ((hi >> 16) & 0xff);
			dev->dev_addr[1] = ((hi >> 24) & 0xff);
			dev->dev_addr[2] = ((lo >>  0) & 0xff);
			dev->dev_addr[3] = ((lo >>  8) & 0xff);
			dev->dev_addr[4] = ((lo >> 16) & 0xff);
			dev->dev_addr[5] = ((lo >> 24) & 0xff);
		}
		/* Finally just fetch it out of the MAC control regs. */
		else {
			hi = tr32(MAC_ADDR_0_HIGH);
			lo = tr32(MAC_ADDR_0_LOW);

			dev->dev_addr[5] = lo & 0xff;
			dev->dev_addr[4] = (lo >> 8) & 0xff;
			dev->dev_addr[3] = (lo >> 16) & 0xff;
			dev->dev_addr[2] = (lo >> 24) & 0xff;
			dev->dev_addr[1] = hi & 0xff;
			dev->dev_addr[0] = (hi >> 8) & 0xff;
		}
	}

	if (!is_valid_ether_addr(&dev->dev_addr[0])) {
		return -EINVAL;
	}
	memcpy(dev->perm_addr, dev->dev_addr, dev->addr_len);
	return 0;
}

#define BOUNDARY_SINGLE_CACHELINE	1
#define BOUNDARY_MULTI_CACHELINE	2

static u32 tg3_calc_dma_bndry(struct tg3 *tp, u32 val)
{
	int cacheline_size;
	u8 byte;
	int goal;

	byte = g_psBus->read_pci_config(tp->pdev->nBus, tp->pdev->nDevice, tp->pdev->nFunction, PCI_LINE_SIZE, 1);
	if (byte == 0)
		cacheline_size = 1024;
	else
		cacheline_size = (int) byte * 4;

	/* On 5703 and later chips, the boundary bits have no
	 * effect.
	 */
	if (GET_ASIC_REV(tp->pci_chip_rev_id) != ASIC_REV_5700 &&
	    GET_ASIC_REV(tp->pci_chip_rev_id) != ASIC_REV_5701 &&
	    !(tp->tg3_flags2 & TG3_FLG2_PCI_EXPRESS))
		goto out;

	goal = 0;

	if (!goal)
		goto out;

	/* PCI controllers on most RISC systems tend to disconnect
	 * when a device tries to burst across a cache-line boundary.
	 * Therefore, letting tg3 do so just wastes PCI bandwidth.
	 *
	 * Unfortunately, for PCI-E there are only limited
	 * write-side controls for this, and thus for reads
	 * we will still get the disconnects.  We'll also waste
	 * these PCI cycles for both read and write for chips
	 * other than 5700 and 5701 which do not implement the
	 * boundary bits.
	 */
	if ((tp->tg3_flags & TG3_FLAG_PCIX_MODE) &&
	    !(tp->tg3_flags2 & TG3_FLG2_PCI_EXPRESS)) {
		switch (cacheline_size) {
		case 16:
		case 32:
		case 64:
		case 128:
			if (goal == BOUNDARY_SINGLE_CACHELINE) {
				val |= (DMA_RWCTRL_READ_BNDRY_128_PCIX |
					DMA_RWCTRL_WRITE_BNDRY_128_PCIX);
			} else {
				val |= (DMA_RWCTRL_READ_BNDRY_384_PCIX |
					DMA_RWCTRL_WRITE_BNDRY_384_PCIX);
			}
			break;

		case 256:
			val |= (DMA_RWCTRL_READ_BNDRY_256_PCIX |
				DMA_RWCTRL_WRITE_BNDRY_256_PCIX);
			break;

		default:
			val |= (DMA_RWCTRL_READ_BNDRY_384_PCIX |
				DMA_RWCTRL_WRITE_BNDRY_384_PCIX);
			break;
		};
	} else if (tp->tg3_flags2 & TG3_FLG2_PCI_EXPRESS) {
		switch (cacheline_size) {
		case 16:
		case 32:
		case 64:
			if (goal == BOUNDARY_SINGLE_CACHELINE) {
				val &= ~DMA_RWCTRL_WRITE_BNDRY_DISAB_PCIE;
				val |= DMA_RWCTRL_WRITE_BNDRY_64_PCIE;
				break;
			}
			/* fallthrough */
		case 128:
		default:
			val &= ~DMA_RWCTRL_WRITE_BNDRY_DISAB_PCIE;
			val |= DMA_RWCTRL_WRITE_BNDRY_128_PCIE;
			break;
		};
	} else {
		switch (cacheline_size) {
		case 16:
			if (goal == BOUNDARY_SINGLE_CACHELINE) {
				val |= (DMA_RWCTRL_READ_BNDRY_16 |
					DMA_RWCTRL_WRITE_BNDRY_16);
				break;
			}
			/* fallthrough */
		case 32:
			if (goal == BOUNDARY_SINGLE_CACHELINE) {
				val |= (DMA_RWCTRL_READ_BNDRY_32 |
					DMA_RWCTRL_WRITE_BNDRY_32);
				break;
			}
			/* fallthrough */
		case 64:
			if (goal == BOUNDARY_SINGLE_CACHELINE) {
				val |= (DMA_RWCTRL_READ_BNDRY_64 |
					DMA_RWCTRL_WRITE_BNDRY_64);
				break;
			}
			/* fallthrough */
		case 128:
			if (goal == BOUNDARY_SINGLE_CACHELINE) {
				val |= (DMA_RWCTRL_READ_BNDRY_128 |
					DMA_RWCTRL_WRITE_BNDRY_128);
				break;
			}
			/* fallthrough */
		case 256:
			val |= (DMA_RWCTRL_READ_BNDRY_256 |
				DMA_RWCTRL_WRITE_BNDRY_256);
			break;
		case 512:
			val |= (DMA_RWCTRL_READ_BNDRY_512 |
				DMA_RWCTRL_WRITE_BNDRY_512);
			break;
		case 1024:
		default:
			val |= (DMA_RWCTRL_READ_BNDRY_1024 |
				DMA_RWCTRL_WRITE_BNDRY_1024);
			break;
		};
	}

out:
	return val;
}

static int tg3_do_test_dma(struct tg3 *tp, u32 *buf, dma_addr_t buf_dma, int size, int to_device)
{
	struct tg3_internal_buffer_desc test_desc;
	u32 sram_dma_descs;
	int i, ret;

	sram_dma_descs = NIC_SRAM_DMA_DESC_POOL_BASE;

	tw32(FTQ_RCVBD_COMP_FIFO_ENQDEQ, 0);
	tw32(FTQ_RCVDATA_COMP_FIFO_ENQDEQ, 0);
	tw32(RDMAC_STATUS, 0);
	tw32(WDMAC_STATUS, 0);

	tw32(BUFMGR_MODE, 0);
	tw32(FTQ_RESET, 0);

	test_desc.addr_hi = ((u64) buf_dma) >> 32;
	test_desc.addr_lo = buf_dma & 0xffffffff;
	test_desc.nic_mbuf = 0x00002100;
	test_desc.len = size;

	/*
	 * HP ZX1 was seeing test failures for 5701 cards running at 33Mhz
	 * the *second* time the tg3 driver was getting loaded after an
	 * initial scan.
	 *
	 * Broadcom tells me:
	 *   ...the DMA engine is connected to the GRC block and a DMA
	 *   reset may affect the GRC block in some unpredictable way...
	 *   The behavior of resets to individual blocks has not been tested.
	 *
	 * Broadcom noted the GRC reset will also reset all sub-components.
	 */
	if (to_device) {
		test_desc.cqid_sqid = (13 << 8) | 2;

		tw32_f(RDMAC_MODE, RDMAC_MODE_ENABLE);
		udelay(40);
	} else {
		test_desc.cqid_sqid = (16 << 8) | 7;

		tw32_f(WDMAC_MODE, WDMAC_MODE_ENABLE);
		udelay(40);
	}
	test_desc.flags = 0x00000005;

	for (i = 0; i < (sizeof(test_desc) / sizeof(u32)); i++) {
		u32 val;

		val = *(((u32 *)&test_desc) + i);
		g_psBus->write_pci_config(tp->pdev->nBus, tp->pdev->nDevice, tp->pdev->nFunction, TG3PCI_MEM_WIN_BASE_ADDR,
						4, sram_dma_descs + (i * sizeof(u32)));
		g_psBus->write_pci_config(tp->pdev->nBus, tp->pdev->nDevice, tp->pdev->nFunction, TG3PCI_MEM_WIN_DATA, 4, val);

	}
	g_psBus->write_pci_config(tp->pdev->nBus, tp->pdev->nDevice, tp->pdev->nFunction, TG3PCI_MEM_WIN_BASE_ADDR, 4, 0);

	if (to_device) {
		tw32(FTQ_DMA_HIGH_READ_FIFO_ENQDEQ, sram_dma_descs);
	} else {
		tw32(FTQ_DMA_HIGH_WRITE_FIFO_ENQDEQ, sram_dma_descs);
	}

	ret = -ENODEV;
	for (i = 0; i < 40; i++) {
		u32 val;

		if (to_device)
			val = tr32(FTQ_RCVBD_COMP_FIFO_ENQDEQ);
		else
			val = tr32(FTQ_RCVDATA_COMP_FIFO_ENQDEQ);
		if ((val & 0xffff) == sram_dma_descs) {
			ret = 0;
			break;
		}

		udelay(100);
	}

	return ret;
}

#define TEST_BUFFER_SIZE	0x2000

static int tg3_test_dma(struct tg3 *tp)
{
	dma_addr_t buf_dma;
	u32 *buf, saved_dma_rwctrl;
	int ret;

	buf = pci_alloc_consistent(tp->pdev, TEST_BUFFER_SIZE, &buf_dma);
	if (!buf) {
		ret = -ENOMEM;
		goto out_nofree;
	}

	tp->dma_rwctrl = ((0x7 << DMA_RWCTRL_PCI_WRITE_CMD_SHIFT) |
			  (0x6 << DMA_RWCTRL_PCI_READ_CMD_SHIFT));

	tp->dma_rwctrl = tg3_calc_dma_bndry(tp, tp->dma_rwctrl);

	if (tp->tg3_flags2 & TG3_FLG2_PCI_EXPRESS) {
		/* DMA read watermark not used on PCIE */
		tp->dma_rwctrl |= 0x00180000;
	} else if (!(tp->tg3_flags & TG3_FLAG_PCIX_MODE)) {
		if (GET_ASIC_REV(tp->pci_chip_rev_id) == ASIC_REV_5705 ||
		    GET_ASIC_REV(tp->pci_chip_rev_id) == ASIC_REV_5750)
			tp->dma_rwctrl |= 0x003f0000;
		else
			tp->dma_rwctrl |= 0x003f000f;
	} else {
		if (GET_ASIC_REV(tp->pci_chip_rev_id) == ASIC_REV_5703 ||
		    GET_ASIC_REV(tp->pci_chip_rev_id) == ASIC_REV_5704) {
			u32 ccval = (tr32(TG3PCI_CLOCK_CTRL) & 0x1f);

			/* If the 5704 is behind the EPB bridge, we can
			 * do the less restrictive ONE_DMA workaround for
			 * better performance.
			 */
			if ((tp->tg3_flags & TG3_FLAG_40BIT_DMA_BUG) &&
			    GET_ASIC_REV(tp->pci_chip_rev_id) == ASIC_REV_5704)
				tp->dma_rwctrl |= 0x8000;
			else if (ccval == 0x6 || ccval == 0x7)
				tp->dma_rwctrl |= DMA_RWCTRL_ONE_DMA;

			/* Set bit 23 to enable PCIX hw bug fix */
			tp->dma_rwctrl |= 0x009f0000;
		} else if (GET_ASIC_REV(tp->pci_chip_rev_id) == ASIC_REV_5780) {
			/* 5780 always in PCIX mode */
			tp->dma_rwctrl |= 0x00144000;
		} else if (GET_ASIC_REV(tp->pci_chip_rev_id) == ASIC_REV_5714) {
			/* 5714 always in PCIX mode */
			tp->dma_rwctrl |= 0x00148000;
		} else {
			tp->dma_rwctrl |= 0x001b000f;
		}
	}

	if (GET_ASIC_REV(tp->pci_chip_rev_id) == ASIC_REV_5703 ||
	    GET_ASIC_REV(tp->pci_chip_rev_id) == ASIC_REV_5704)
		tp->dma_rwctrl &= 0xfffffff0;

	if (GET_ASIC_REV(tp->pci_chip_rev_id) == ASIC_REV_5700 ||
	    GET_ASIC_REV(tp->pci_chip_rev_id) == ASIC_REV_5701) {
		/* Remove this if it causes problems for some boards. */
		tp->dma_rwctrl |= DMA_RWCTRL_USE_MEM_READ_MULT;

		/* On 5700/5701 chips, we need to set this bit.
		 * Otherwise the chip will issue cacheline transactions
		 * to streamable DMA memory with not all the byte
		 * enables turned on.  This is an error on several
		 * RISC PCI controllers, in particular sparc64.
		 *
		 * On 5703/5704 chips, this bit has been reassigned
		 * a different meaning.  In particular, it is used
		 * on those chips to enable a PCI-X workaround.
		 */
		tp->dma_rwctrl |= DMA_RWCTRL_ASSERT_ALL_BE;
	}

	tw32(TG3PCI_DMA_RW_CTRL, tp->dma_rwctrl);

#if 0
	/* Unneeded, already done by tg3_get_invariants.  */
	tg3_switch_clocks(tp);
#endif

	ret = 0;
	if (GET_ASIC_REV(tp->pci_chip_rev_id) != ASIC_REV_5700 &&
	    GET_ASIC_REV(tp->pci_chip_rev_id) != ASIC_REV_5701)
		goto out;

	/* It is best to perform DMA test with maximum write burst size
	 * to expose the 5700/5701 write DMA bug.
	 */
	saved_dma_rwctrl = tp->dma_rwctrl;
	tp->dma_rwctrl &= ~DMA_RWCTRL_WRITE_BNDRY_MASK;
	tw32(TG3PCI_DMA_RW_CTRL, tp->dma_rwctrl);

	while (1) {
		u32 *p = buf, i;

		for (i = 0; i < TEST_BUFFER_SIZE / sizeof(u32); i++)
			p[i] = i;

		/* Send the buffer to the chip. */
		ret = tg3_do_test_dma(tp, buf, buf_dma, TEST_BUFFER_SIZE, 1);
		if (ret) {
			kerndbg( KERN_WARNING, "tg3_test_dma() Write the buffer failed %d\n", ret);
			break;
		}

#if 0
		/* validate data reached card RAM correctly. */
		for (i = 0; i < TEST_BUFFER_SIZE / sizeof(u32); i++) {
			u32 val;
			tg3_read_mem(tp, 0x2100 + (i*4), &val);
			if (le32_to_cpu(val) != p[i]) {
				printk(KERN_ERR "  tg3_test_dma()  Card buffer corrupted on write! (%d != %d)\n", val, i);
				/* ret = -ENODEV here? */
			}
			p[i] = 0;
		}
#endif
		/* Now read it back. */
		ret = tg3_do_test_dma(tp, buf, buf_dma, TEST_BUFFER_SIZE, 0);
		if (ret) {
			kerndbg( KERN_WARNING, "tg3_test_dma() Read the buffer failed %d\n", ret);

			break;
		}

		/* Verify it. */
		for (i = 0; i < TEST_BUFFER_SIZE / sizeof(u32); i++) {
			if (p[i] == i)
				continue;

			if ((tp->dma_rwctrl & DMA_RWCTRL_WRITE_BNDRY_MASK) !=
			    DMA_RWCTRL_WRITE_BNDRY_16) {
				tp->dma_rwctrl &= ~DMA_RWCTRL_WRITE_BNDRY_MASK;
				tp->dma_rwctrl |= DMA_RWCTRL_WRITE_BNDRY_16;
				tw32(TG3PCI_DMA_RW_CTRL, tp->dma_rwctrl);
				break;
			} else {
				kerndbg( KERN_WARNING, "tg3_test_dma() buffer corrupted on read back! (%d != %d)\n", p[i], i);
				ret = -ENODEV;
				goto out;
			}
		}

		if (i == (TEST_BUFFER_SIZE / sizeof(u32))) {
			/* Success. */
			ret = 0;
			break;
		}
	}
	if ((tp->dma_rwctrl & DMA_RWCTRL_WRITE_BNDRY_MASK) !=
	    DMA_RWCTRL_WRITE_BNDRY_16) {
		static struct pci_device_id dma_wait_state_chipsets[] = {
			{ PCI_VENDOR_ID_APPLE,
				     PCI_DEVICE_ID_APPLE_UNI_N_PCI15 },
			{ },
		};

		/* DMA test passed without adjusting DMA boundary,
		 * now look for chipsets that are known to expose the
		 * DMA bug without failing the test.
		 */

/* XXXKV: On Syllable we'll have to emulate pci_dev_present() or find some other way to do it */
#if 0
		if (pci_dev_present(dma_wait_state_chipsets)) {
			tp->dma_rwctrl &= ~DMA_RWCTRL_WRITE_BNDRY_MASK;
			tp->dma_rwctrl |= DMA_RWCTRL_WRITE_BNDRY_16;
		}
		else
#endif
			/* Safe to use the calculated DMA boundary. */
			tp->dma_rwctrl = saved_dma_rwctrl;

		tw32(TG3PCI_DMA_RW_CTRL, tp->dma_rwctrl);
	}

out:
	pci_free_consistent(tp->pdev, TEST_BUFFER_SIZE, buf, buf_dma);
out_nofree:
	return ret;
}

static void tg3_init_link_config(struct tg3 *tp)
{
	tp->link_config.advertising =
		(ADVERTISED_10baseT_Half | ADVERTISED_10baseT_Full |
		 ADVERTISED_100baseT_Half | ADVERTISED_100baseT_Full |
		 ADVERTISED_1000baseT_Half | ADVERTISED_1000baseT_Full |
		 ADVERTISED_Autoneg | ADVERTISED_MII);
	tp->link_config.speed = SPEED_INVALID;
	tp->link_config.duplex = DUPLEX_INVALID;
	tp->link_config.autoneg = AUTONEG_ENABLE;
	tp->link_config.active_speed = SPEED_INVALID;
	tp->link_config.active_duplex = DUPLEX_INVALID;
	tp->link_config.phy_is_low_power = 0;
	tp->link_config.orig_speed = SPEED_INVALID;
	tp->link_config.orig_duplex = DUPLEX_INVALID;
	tp->link_config.orig_autoneg = AUTONEG_INVALID;
}

static void tg3_init_bufmgr_config(struct tg3 *tp)
{
	if (tp->tg3_flags2 & TG3_FLG2_5705_PLUS) {
		tp->bufmgr_config.mbuf_read_dma_low_water =
			DEFAULT_MB_RDMA_LOW_WATER_5705;
		tp->bufmgr_config.mbuf_mac_rx_low_water =
			DEFAULT_MB_MACRX_LOW_WATER_5705;
		tp->bufmgr_config.mbuf_high_water =
			DEFAULT_MB_HIGH_WATER_5705;

		tp->bufmgr_config.mbuf_read_dma_low_water_jumbo =
			DEFAULT_MB_RDMA_LOW_WATER_JUMBO_5780;
		tp->bufmgr_config.mbuf_mac_rx_low_water_jumbo =
			DEFAULT_MB_MACRX_LOW_WATER_JUMBO_5780;
		tp->bufmgr_config.mbuf_high_water_jumbo =
			DEFAULT_MB_HIGH_WATER_JUMBO_5780;
	} else {
		tp->bufmgr_config.mbuf_read_dma_low_water =
			DEFAULT_MB_RDMA_LOW_WATER;
		tp->bufmgr_config.mbuf_mac_rx_low_water =
			DEFAULT_MB_MACRX_LOW_WATER;
		tp->bufmgr_config.mbuf_high_water =
			DEFAULT_MB_HIGH_WATER;

		tp->bufmgr_config.mbuf_read_dma_low_water_jumbo =
			DEFAULT_MB_RDMA_LOW_WATER_JUMBO;
		tp->bufmgr_config.mbuf_mac_rx_low_water_jumbo =
			DEFAULT_MB_MACRX_LOW_WATER_JUMBO;
		tp->bufmgr_config.mbuf_high_water_jumbo =
			DEFAULT_MB_HIGH_WATER_JUMBO;
	}

	tp->bufmgr_config.dma_low_water = DEFAULT_DMA_LOW_WATER;
	tp->bufmgr_config.dma_high_water = DEFAULT_DMA_HIGH_WATER;
}

static char * tg3_phy_string(struct tg3 *tp)
{
	switch (tp->phy_id & PHY_ID_MASK) {
	case PHY_ID_BCM5400:	return "5400";
	case PHY_ID_BCM5401:	return "5401";
	case PHY_ID_BCM5411:	return "5411";
	case PHY_ID_BCM5701:	return "5701";
	case PHY_ID_BCM5703:	return "5703";
	case PHY_ID_BCM5704:	return "5704";
	case PHY_ID_BCM5705:	return "5705";
	case PHY_ID_BCM5750:	return "5750";
	case PHY_ID_BCM5752:	return "5752";
	case PHY_ID_BCM5714:	return "5714";
	case PHY_ID_BCM5780:	return "5780";
	case PHY_ID_BCM5755:	return "5755";
	case PHY_ID_BCM5787:	return "5787";
	case PHY_ID_BCM8002:	return "8002/serdes";
	case 0:			return "serdes";
	default:		return "unknown";
	};
}

static char * tg3_bus_string(struct tg3 *tp, char *str)
{
	if (tp->tg3_flags2 & TG3_FLG2_PCI_EXPRESS) {
		strcpy(str, "PCI Express");
		return str;
	} else if (tp->tg3_flags & TG3_FLAG_PCIX_MODE) {
		u32 clock_ctrl = tr32(TG3PCI_CLOCK_CTRL) & 0x1f;

		strcpy(str, "PCIX:");

		if ((clock_ctrl == 7) ||
		    ((tr32(GRC_MISC_CFG) & GRC_MISC_CFG_BOARD_ID_MASK) ==
		     GRC_MISC_CFG_BOARD_ID_5704CIOBE))
			strcat(str, "133MHz");
		else if (clock_ctrl == 0)
			strcat(str, "33MHz");
		else if (clock_ctrl == 2)
			strcat(str, "50MHz");
		else if (clock_ctrl == 4)
			strcat(str, "66MHz");
		else if (clock_ctrl == 6)
			strcat(str, "100MHz");
	} else {
		strcpy(str, "PCI:");
		if (tp->tg3_flags & TG3_FLAG_PCI_HIGH_SPEED)
			strcat(str, "66MHz");
		else
			strcat(str, "33MHz");
	}
	if (tp->tg3_flags & TG3_FLAG_PCI_32BIT)
		strcat(str, ":32-bit");
	else
		strcat(str, ":64-bit");
	return str;
}

static PCI_Info_s * tg3_find_peer(struct tg3 *tp)
{
	/* Not yet ported */
	return NULL;
}

static int tg3_init_one( PCI_Info_s *pDev, struct net_device *pNetDev )
{
	int i, nError = 0;
	struct tg3 *tp;
	unsigned long tg3reg_base, tg3reg_len;
	uint8 pm_cap;
	char str[40];

	/* Enable device */
	uint16 nOldCommand, nNewCommand;

	nOldCommand = g_psBus->read_pci_config( pDev->nBus, pDev->nDevice, pDev->nFunction, PCI_COMMAND, 2 );
	nNewCommand = nOldCommand | ( PCI_COMMAND_MEMORY & 7);
	if( nOldCommand != nNewCommand )
		g_psBus->write_pci_config( pDev->nBus, pDev->nDevice, pDev->nFunction, PCI_COMMAND, 2, nNewCommand );

	g_psBus->enable_pci_master( pDev->nBus, pDev->nDevice, pDev->nFunction );

	/* Find power-management capability. */
	pm_cap = g_psBus->get_pci_capability( pDev->nBus, pDev->nDevice, pDev->nFunction, PCI_CAP_ID_PM );
	if( pm_cap == 0 )
	{
		kerndbg( KERN_WARNING, "tg3: Cannot find PowerManagement capability, aborting.\n");
		nError = -EIO;
		goto err_out;
	}

	tg3reg_base = pDev->u.h0.nBase0 & PCI_ADDRESS_MEMORY_32_MASK;
	tg3reg_len = pci_resource_len( pDev, 0 );

	tp = netdev_priv(pNetDev);
	tp->pdev = pDev;
	tp->dev = pNetDev;
	tp->pm_cap = pm_cap;
	tp->mac_mode = TG3_DEF_MAC_MODE;
	tp->rx_mode = TG3_DEF_RX_MODE;
	tp->tx_mode = TG3_DEF_TX_MODE;
	tp->mi_mode = MAC_MI_MODE_BASE;

	/* The word/byte swap controls here control register access byte
	 * swapping.  DMA data byte swapping is controlled in the GRC_MODE
	 * setting below.
	 */
	tp->misc_host_ctrl =
		MISC_HOST_CTRL_MASK_PCI_INT |
		MISC_HOST_CTRL_WORD_SWAP |
		MISC_HOST_CTRL_INDIR_ACCESS |
		MISC_HOST_CTRL_PCISTATE_RW;

	/* The NONFRM (non-frame) byte/word swap controls take effect
	 * on descriptor entries, anything which isn't packet data.
	 *
	 * The StrongARM chips on the board (one for tx, one for rx)
	 * are running in big-endian mode.
	 */
	tp->grc_mode = (GRC_MODE_WSWAP_DATA | GRC_MODE_BSWAP_DATA |
			GRC_MODE_WSWAP_NONFRM_DATA);
	spinlock_init(&tp->lock, "tg3");
	spinlock_init(&tp->indirect_lock, "tg3_indirect");

	/* Map the device registers into memory. The address is page-aligned. */
	tp->reg_area = create_area ("tg3_register", (void **)&tp->regs, tg3reg_len, tg3reg_len, AREA_KERNEL | AREA_ANY_ADDRESS, AREA_NO_LOCK );
	if( tp->reg_area < 0 )
	{
		kerndbg ( KERN_DEBUG, "tg3: failed to create register area (%d)\n", tp->reg_area );
		nError = -EIO;
		goto err_out;
	}

	if( remap_area (tp->reg_area, (void *)(tg3reg_base & PAGE_MASK) ) < 0 )
	{
		kerndbg( KERN_DEBUG, "tg3: failed to remap register area (%d)\n", tp->reg_area );
		nError = -EIO;
		goto err_out;
	}

	tp->regs = (void*)( (uint32)tp->regs + ( tg3reg_base - ( tg3reg_base & PAGE_MASK ) ) );

	tg3_init_link_config(tp);

	tp->rx_pending = TG3_DEF_RX_RING_PENDING;
	tp->rx_jumbo_pending = TG3_DEF_RX_JUMBO_RING_PENDING;
	tp->tx_pending = TG3_DEF_TX_RING_PENDING;

	nError = tg3_get_invariants(tp);
	if( nError )
	{
		kerndbg(KERN_WARNING, "tg3: Problem fetching invariants of chip, aborting.\n");
		goto err_out_iounmap;
	}

	tg3_init_bufmgr_config(tp);

	/* Always switch off hardware TSO */
	tp->tg3_flags2 &= ~TG3_FLG2_TSO_CAPABLE;

	if (tp->pci_chip_rev_id == CHIPREV_ID_5705_A1 &&
	    !(tp->tg3_flags2 & TG3_FLG2_TSO_CAPABLE) &&
	    !(tr32(TG3PCI_PCISTATE) & PCISTATE_BUS_SPEED_HIGH)) {
		tp->tg3_flags2 |= TG3_FLG2_MAX_RXPEND_64;
		tp->rx_pending = 63;
	}

	if ((GET_ASIC_REV(tp->pci_chip_rev_id) == ASIC_REV_5704) ||
	    (GET_ASIC_REV(tp->pci_chip_rev_id) == ASIC_REV_5714))
		tp->pdev_peer = tg3_find_peer(tp);

	nError = tg3_get_device_address(tp);
	if( nError )
	{
		kerndbg( KERN_WARNING, "tg3: Could not obtain valid ethernet address, aborting.\n");
		goto err_out_iounmap;
	}

	/*
	 * Reset chip in case UNDI or EFI driver did not shutdown
	 * DMA self test will enable WDMAC and we'll see (spurious)
	 * pending DMA on the PCI bus at that point.
	 */
	if ((tr32(HOSTCC_MODE) & HOSTCC_MODE_ENABLE) ||
	    (tr32(WDMAC_MODE) & WDMAC_MODE_ENABLE)) {
		pci_save_state(tp->pdev, tp->pci_state);
		tw32(MEMARB_MODE, MEMARB_MODE_ENABLE);
		tg3_halt(tp, RESET_KIND_SHUTDOWN, 1);
	}

	nError = tg3_test_dma(tp);
	if( nError )
	{
		kerndbg( KERN_WARNING, "tg3: DMA engine test failed, aborting.\n");
		goto err_out_iounmap;
	}

	/* Always switch off hardware checksums */
	tp->tg3_flags &= ~TG3_FLAG_RX_CHECKSUMS;

	/* flow control autonegotiation is default behavior */
	tp->tg3_flags |= TG3_FLAG_PAUSE_AUTONEG;

	/* Now that we have fully setup the chip, save away a snapshot
	 * of the PCI config space.  We need to restore this after
	 * GRC_MISC_CFG core clock resets and some resume events.
	 */
	pci_save_state(tp->pdev, tp->pci_state);

	printk( "%s: Tigon3 [partno(%s) rev %04x PHY(%s)] (%s) %sBaseT Ethernet\n",
	       pNetDev->name,
	       tp->board_part_number,
	       tp->pci_chip_rev_id,
	       tg3_phy_string(tp),
	       tg3_bus_string(tp, str),
	       (tp->tg3_flags & TG3_FLAG_10_100_ONLY) ? "10/100" : "10/100/1000");

	printk( "%s: MAC: %2.2x:%2.2x:%2.2x:%2.2x:%2.2x:%2.2x\n",
		pNetDev->name,
		pNetDev->dev_addr[0],
		pNetDev->dev_addr[1],
		pNetDev->dev_addr[2],
		pNetDev->dev_addr[3],
		pNetDev->dev_addr[4],
		pNetDev->dev_addr[5] );

	printk( "%s: RXcsums[%d] LinkChgREG[%d] "
	       "MIirq[%d] ASF[%d] Split[%d] WireSpeed[%d] "
	       "TSOcap[%d] \n",
	       pNetDev->name,
	       (tp->tg3_flags & TG3_FLAG_RX_CHECKSUMS) != 0,
	       (tp->tg3_flags & TG3_FLAG_USE_LINKCHG_REG) != 0,
	       (tp->tg3_flags & TG3_FLAG_USE_MI_INTERRUPT) != 0,
	       (tp->tg3_flags & TG3_FLAG_ENABLE_ASF) != 0,
	       (tp->tg3_flags & TG3_FLAG_SPLIT_MODE) != 0,
	       (tp->tg3_flags2 & TG3_FLG2_NO_ETH_WIRE_SPEED) == 0,
	       (tp->tg3_flags2 & TG3_FLG2_TSO_CAPABLE) != 0);
	printk( "%s: dma_rwctrl[%08x]\n",
	       pNetDev->name, tp->dma_rwctrl);

	netif_carrier_off(tp->dev);

	return 0;

err_out_iounmap:
	if( tp->reg_area )
		delete_area( tp->reg_area );
	tp->regs = NULL;

err_out:
	return nError;
}

static int tg3_probe( int nDeviceID )
{
	int nCardsFound = 0;
    PCI_Info_s sInfo;
    int i, j;
   
	for( i = 0; g_psBus->get_pci_info( &sInfo, i ) == 0; ++i )
	{
		for( j = 0; tg3_pci_tbl[j].vendor_id != 0; j++ )
		{
			if ( sInfo.nVendorID == tg3_pci_tbl[j].vendor_id &&
				 sInfo.nDeviceID == tg3_pci_tbl[j].device_id)
			{
				struct net_device *pNetDev = NULL;
				struct tg3 *pPrivate = NULL;

				if( claim_device( nDeviceID, sInfo.nHandle, "Broadcom NetExtreme", DEVICE_NET ) < 0 )
					continue;

				pNetDev = kmalloc( sizeof( *pNetDev ), MEMF_KERNEL | MEMF_CLEAR );
				if( NULL == pNetDev )
				{
					kerndbg( KERN_WARNING, "tg3_probe(): Could not allocate memory for device.\n" );
					continue;
				}
				pNetDev->name = "tg3";
				pNetDev->device_handle = nDeviceID;
				pNetDev->irq = sInfo.u.h0.nInterruptLine;

				/* This driver relies on this having a sensible value */
				pNetDev->mtu = 1518;

				pPrivate = kmalloc( sizeof( *pPrivate ), MEMF_KERNEL | MEMF_CLEAR );
				if( NULL == pPrivate )
				{
					kerndbg( KERN_WARNING, "tg3_probe(): Could not allocate memory for device.\n" );
					kfree( pNetDev );
					continue;
				}
				pNetDev->priv = pPrivate;

				set_device_data( sInfo.nHandle, pNetDev );

				if( tg3_init_one( &sInfo, pNetDev ) == 0 )
				{
					char zNodePath[64];
					sprintf( zNodePath, "net/eth/tg3-%d", nCardsFound );

					kerndbg( KERN_DEBUG, "tg3_probe(): Create node %s\n", zNodePath );

					pNetDev->node_handle = create_device_node( nDeviceID, sInfo.nHandle, zNodePath, &g_sDevOps, pNetDev );
					nCardsFound++;
				}
			}
		}
	}

	if( nCardsFound == 0 )
		disable_device( nDeviceID );

	return nCardsFound ? 0 : -ENODEV;
}

/* Power management */
status_t device_suspend( int nDeviceID, int nDeviceHandle, void* pPrivateData )
{
	struct net_device *dev = pPrivateData;
	struct tg3 *tp = netdev_priv(dev);
	int err;

	if (!netif_running(dev))
		return 0;

	tg3_netif_stop(tp);

	delete_timer(tp->timer);

	tg3_full_lock(tp, 1);
	tg3_disable_ints(tp);
	tg3_full_unlock(tp);

	tg3_full_lock(tp, 0);
	tg3_halt(tp, RESET_KIND_SHUTDOWN, 1);
	tp->tg3_flags &= ~TG3_FLAG_INIT_COMPLETE;
	tg3_full_unlock(tp);

	err = tg3_set_power_state(tp, PCI_D3hot );
	if (err) {
		tg3_full_lock(tp, 0);

		tp->tg3_flags |= TG3_FLAG_INIT_COMPLETE;
		if (tg3_restart_hw(tp, 1))
			goto out;

		tp->timer = create_timer();
		start_timer(tp->timer, (timer_callback *) &tg3_timer, tp, (jiffies + tp->timer_offset), true );

		tg3_netif_start(tp);

out:
		tg3_full_unlock(tp);
	}

	return err;
}

status_t device_resume( int nDeviceID, int nDeviceHandle, void* pPrivateData )
{
	struct net_device *dev = pPrivateData;
	struct tg3 *tp = netdev_priv(dev);
	int err;

	if (!netif_running(dev))
		return 0;

	pci_restore_state(tp->pdev, tp->pci_state);

	err = tg3_set_power_state(tp, PCI_D0);
	if (err)
		return err;

	tg3_full_lock(tp, 0);

	tp->tg3_flags |= TG3_FLAG_INIT_COMPLETE;
	err = tg3_restart_hw(tp, 1);
	if (err)
		goto out;

	tp->timer = create_timer();
	start_timer(tp->timer, (timer_callback *) &tg3_timer, tp, (jiffies + tp->timer_offset), true );

	tg3_netif_start(tp);

out:
	tg3_full_unlock(tp);

	return err;
}

/* Driver management */
status_t device_init( int nDeviceID )
{
	/* Get PCI bus */
	g_psBus = get_busmanager( PCI_BUS_NAME, PCI_BUS_VERSION );
	if( g_psBus == NULL )
		return -ENODEV;

	return tg3_probe( nDeviceID );
}

void device_release( int nDeviceID, int nDeviceHandle, void* pPrivateData )
{
	struct net_device *pNetDev = pPrivateData;
	struct tg3 *pPrivate = netdev_priv( pNetDev );

	set_device_data( nDeviceHandle, NULL );

	release_irq( pNetDev->irq, pNetDev->irq_handle );
	delete_timer( pPrivate->timer );

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


