#include <atheos/kernel.h>
#include <atheos/irq.h>
#include <atheos/isa_io.h>
#include <atheos/udelay.h>
#include <atheos/time.h>
#include <atheos/pci.h>
#include <atheos/semaphore.h>
#include <atheos/spinlock.h>
#include <atheos/ctype.h>
#include <atheos/device.h>

#include <posix/unistd.h>
#include <posix/errno.h>
#include <posix/signal.h>
#include <net/net.h>
#include <net/sockios.h>

#include "8390.h"

#define KERN_ERR "Error: "
#define KERN_DEBUG "Debug: "
#define KERN_WARNING "Warning: "
#define KERN_INFO "Info: "

#define MAX_PACKET_SIZE 1518


#define VERBOSE_ERROR_DUMP

//#define NE_SANITY_CHECK

#define CRYNWR_WAY
//#define SUPPORT_NE_BAD_CLONES
#define EI_PINGPONG

static unsigned int netcard_portlist[]  = { 
  0x300, 0x280, 0x320, 0x340, 0x360, 0x380, 0
};

#define NE_CMD	 	0x00
#define NE_DATAPORT	0x10	/* NatSemi-defined port window offset. */
#define NE_RESET	0x1f	/* Issue a read to reset, a write to clear. */
#define NE_IO_EXTENT	0x20

#define NE1SM_START_PG	0x20	/* First page of TX buffer */
#define NE1SM_STOP_PG 	0x40	/* Last page +1 of RX ring */
#define NESM_START_PG	0x40	/* First page of TX buffer */
#define NESM_STOP_PG	0x80	/* Last page +1 of RX ring */


typedef struct _NEPacketBuf NEPacketBuf_s;

struct _NEPacketBuf
{
    NEPacketBuf_s* psNext;
    NEPacketBuf_s* psPrev;
    uint8*       pBuffer;
    int	       nSize;
};

typedef struct
{
    NEPacketBuf_s* psHead;
    NEPacketBuf_s* psTail;
    int	       nCount;
} BufferList_s;

typedef struct
{
    int		 n2_nDeviceID;
    int		 n2_nIoBase;
    bigtime_t	 n2_nTransStart;
    uint32	 n2_nFlags;
    char*	 n2_pzName;
    uint8	 n2_anEthrAddress[6];
    bool	 n2_bStarted;
    bool	 n2_bTxBusy;
    bool	 n2_bIrqActive;
    thread_id	 n2_hRxThread;
    sem_id	 n2_hRxSemIrq;
    sem_id	 n2_hRxSem;
    sem_id	 n2_hTxSem;
//    WaitQueue_s* n2_psTxWaitQueue;
    BufferList_s n2_sRxIrqQueue;
    BufferList_s n2_sRxQueue;
    BufferList_s n2_sRxFreeBuffers;
    BufferList_s n2_sTxPending;
    int		 n2_nIrq;
    int		 n2_nIRQHandle;
    int		 ne_nInodeHandle;
    NetQueue_s*	 n2_psPktQueue;
    SpinLock_s	 n2_nPageLock;
    uint8 	 n2_anMCFilter[8];
    unsigned	 n2_nWord16:1;  	/* We have the 16-bit (vs 8-bit) version of the card. */
    unsigned 	 n2_nTXing:1;		/* Transmit Active */
    unsigned	 n2_nIRQLock:1;		/* 8390's intrs disabled when '1'. */
    unsigned	 n2_nDMAing:1;		/* Remote DMA Active */

    uint8	 n2_nTXStartPage;
    uint8	 n2_nRXStartPage;
    uint8	 n2_nStopPage;
    uint8	 n2_nCurrentPage;	/* Read pointer in buffer  */
    uint8	 n2_nInterfaceNum;	/* Net port (AUI, 10bT.) to use. */
    uint8	 n2_nTXQueue;		/* Tx Packet buffer queue length. */
    int16 	 n2_nTX1;
    int16	 n2_nTX2;			/* Packet lengths for ping-pong tx. */
    int16	 n2_nLastTX;			/* Alpha version consistency check. */
    struct net_device_stats n2_sStat;	/* The new statistics table. */
    
} NE2Dev_s;

//NE2Dev_s* g_psDev;


/* Ack! People are making PCI ne2000 clones! Oh the horror, the horror... */
static struct { unsigned short vendor, dev_id; char *name; }
pci_clone_list[] = {
    {PCI_VENDOR_ID_REALTEK,		PCI_DEVICE_ID_REALTEK_8029,		"Realtek 8029" },
    {PCI_VENDOR_ID_WINBOND2,	PCI_DEVICE_ID_WINBOND2_89C940,		"Winbond 89C940" },
    {PCI_VENDOR_ID_COMPEX,		PCI_DEVICE_ID_COMPEX_RL2000,		"Compex ReadyLink 2000" },
    {PCI_VENDOR_ID_KTI,		PCI_DEVICE_ID_KTI_ET32P2,		"KTI ET32P2" },
    {PCI_VENDOR_ID_NETVIN,		PCI_DEVICE_ID_NETVIN_NV5000SC,		"NetVin NV5000" },
    {PCI_VENDOR_ID_VIA,		PCI_DEVICE_ID_VIA_82C926,		"VIA 82C926 Amazon" },
    {PCI_VENDOR_ID_SURECOM,		PCI_DEVICE_ID_SURECOM_NE34,		"SureCom NE34"},
    {0,}
};

static int pci_irq_line = 0;
static int g_nISAIRQ = 0;
static int g_nHandle;
/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static void add_buffer( BufferList_s* psList, NEPacketBuf_s* psBuf )
{
    if ( NULL != psBuf->psNext || NULL != psBuf->psPrev ) {
	printk( KERN_ERR "NEx000 add_buffer() Attempt to add entry twice!\n" );
    }

    if ( psList->psTail != NULL ) {
	psList->psTail->psNext = psBuf;
    }
    if ( psList->psHead == NULL ) {
	psList->psHead = psBuf;
    }
  
    psBuf->psNext = NULL;
    psBuf->psPrev = psList->psTail;
    psList->psTail = psBuf;
    psList->nCount++;
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static void rem_buffer( BufferList_s* psList, NEPacketBuf_s* psBuf )
{
    if ( NULL != psBuf->psNext ) {
	psBuf->psNext->psPrev = psBuf->psPrev;
    }
    if ( NULL != psBuf->psPrev ) {
	psBuf->psPrev->psNext = psBuf->psNext;
    }

    if ( psList->psHead == psBuf ) {
	psList->psHead = psBuf->psNext;
    }
    if ( psList->psTail == psBuf ) {
	psList->psTail = psBuf->psPrev;
    }

    psBuf->psNext = NULL;
    psBuf->psPrev = NULL;
    psList->nCount--;
  
    if ( psList->nCount < 0 ) {
	panic( "NEx000 : rem_buffer() packet buffer list %p got count of %d\n", psList, psList->nCount );
    }
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

NEPacketBuf_s* alloc_rcv_packet( NE2Dev_s* psDev, int nSize )
{
    NEPacketBuf_s* psBuf;
  
    if ( nSize > 1518 ) {
	printk( KERN_ERR "alloc_rcv_packet() packet to big: %d\n", nSize );
	return( NULL );
    }
    if ( psDev->n2_sRxFreeBuffers.psHead != NULL ) {
	psBuf = psDev->n2_sRxFreeBuffers.psHead;
	rem_buffer( &psDev->n2_sRxFreeBuffers, psBuf );
	psBuf->nSize = nSize;
	return( psBuf );
    }
    return( NULL );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

void add_rcv_buffer( NE2Dev_s* psDev, NEPacketBuf_s* psBuf )
{
    add_buffer( &psDev->n2_sRxIrqQueue, psBuf );
    UNLOCK( psDev->n2_hRxSemIrq );
}

/*****************************************************************************
 * NAME:
 * DESC:
 *	Set or clear the multicast filter for this adaptor. May be called
 *	from a BH in 2.1.x. 
 * NOTE:
 *	Must be called with lock held. 
 * SEE ALSO:
 ****************************************************************************/
 
static void do_set_multicast_list( NE2Dev_s* psDev )
{
    int e8390_base = psDev->n2_nIoBase;
    int i;

#ifndef __ATHEOS__  
    if (!(psDev->n2_nFlags & (IFF_PROMISC|IFF_ALLMULTI))) 
    {
	memset(psDev->n2_anMCFilter, 0, 8);
	if (dev->mc_list)
	    make_mc_bits(psDev->n2_anMCFilter, dev);
    }
    else
#endif
	memset(psDev->n2_anMCFilter, 0xFF, 8);	/* mcast set to accept-all */
  
      /* 
       * DP8390 manuals don't specify any magic sequence for altering
       * the multicast regs on an already running card. To be safe, we
       * ensure multicast mode is off prior to loading up the new hash
       * table. If this proves to be not enough, we can always resort
       * to stopping the NIC, loading the table and then restarting.
       *
       * Bug Alert!  The MC regs on the SMC 83C690 (SMC Elite and SMC 
       * Elite16) appear to be write-only. The NS 8390 data sheet lists
       * them as r/w so this is a bug.  The SMC 83C790 (SMC Ultra and
       * Ultra32 EISA) appears to have this bug fixed.
       */
	 
    if (psDev->n2_bStarted)
	outb_p(E8390_RXCONFIG, e8390_base + EN0_RXCR);
  
    outb_p(E8390_NODMA + E8390_PAGE1, e8390_base + E8390_CMD);
    for(i = 0; i < 8; i++) 
    {
	outb_p(psDev->n2_anMCFilter[i], e8390_base + EN1_MULT_SHIFT(i));
#ifndef BUG_83C690
	if(inb_p(e8390_base + EN1_MULT_SHIFT(i))!=psDev->n2_anMCFilter[i])
	    printk(KERN_ERR "Multicast filter read/write mismap %d\n",i);
#endif
    }
    outb_p(E8390_NODMA + E8390_PAGE0, e8390_base + E8390_CMD);
  
#ifndef __ATHEOS__
    if(dev->flags&IFF_PROMISC)
	outb_p(E8390_RXCONFIG | 0x18, e8390_base + EN0_RXCR);
    else if(dev->flags&IFF_ALLMULTI || dev->mc_list)
	outb_p(E8390_RXCONFIG | 0x08, e8390_base + EN0_RXCR);
    else
#endif    
	outb_p(E8390_RXCONFIG, e8390_base + EN0_RXCR);
}

/*****************************************************************************
 * NAME:
 * DESC:
 *	Called without lock held. This is invoked from user context and may
 *	be parallel to just about everything else. Its also fairly quick and
 *	not called too often. Must protect against both bh and irq users
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/
/*
static void set_multicast_list( NE2Dev_s* psDev )
{
  unsigned long flags;
  struct ei_device *ei_local = &psDev->n2_sCardInfo; // (struct ei_device*)dev->priv;
	
  spinlock_cli(&psDev->n2_nPageLock, flags);
  do_set_multicast_list( psDev );
  spinunlock_restore(&psDev->n2_nPageLock, flags);
}	
*/
/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

void NS8390_init( NE2Dev_s* psDev, int startp )
{
    int e8390_base = psDev->n2_nIoBase;
    int i;
    int endcfg = psDev->n2_nWord16 ? (0x48 | ENDCFG_WTS) : 0x48;
    
    if(sizeof(struct e8390_pkt_hdr)!=4)
	panic("8390.c: header struct mispacked\n");    
      /* Follow National Semi's recommendations for initing the DP83902. */
    outb_p(E8390_NODMA+E8390_PAGE0+E8390_STOP, e8390_base+E8390_CMD); /* 0x21 */
    outb_p(endcfg, e8390_base + EN0_DCFG);	/* 0x48 or 0x49 */
      /* Clear the remote byte count registers. */
    outb_p(0x00,  e8390_base + EN0_RCNTLO);
    outb_p(0x00,  e8390_base + EN0_RCNTHI);
      /* Set to monitor and loopback mode -- this is vital!. */
    outb_p(E8390_RXOFF, e8390_base + EN0_RXCR); /* 0x20 */
    outb_p(E8390_TXOFF, e8390_base + EN0_TXCR); /* 0x02 */
      /* Set the transmit page and receive ring. */
    outb_p(psDev->n2_nTXStartPage, e8390_base + EN0_TPSR);
    psDev->n2_nTX1 = psDev->n2_nTX2 = 0;
    outb_p(psDev->n2_nRXStartPage, e8390_base + EN0_STARTPG);
    outb_p(psDev->n2_nStopPage-1, e8390_base + EN0_BOUNDARY);	/* 3c503 says 0x3f,NS0x26*/
    psDev->n2_nCurrentPage = psDev->n2_nRXStartPage;		/* assert boundary+1 */
    outb_p(psDev->n2_nStopPage, e8390_base + EN0_STOPPG);
      /* Clear the pending interrupts and mask. */
    outb_p(0xFF, e8390_base + EN0_ISR);
    outb_p(0x00,  e8390_base + EN0_IMR);
    
      /* Copy the station address into the DS8390 registers. */

    outb_p(E8390_NODMA + E8390_PAGE1 + E8390_STOP, e8390_base+E8390_CMD); /* 0x61 */
    for(i = 0; i < 6; i++) 
    {
	outb_p( psDev->n2_anEthrAddress[i], e8390_base + EN1_PHYS_SHIFT(i));
	if(inb_p(e8390_base + EN1_PHYS_SHIFT(i)) != psDev->n2_anEthrAddress[i]) {
	    printk(KERN_ERR "Hw. address read/write mismap %d (%02x-%02x)\n",
		   i, inb_p(e8390_base + EN1_PHYS_SHIFT(i)), psDev->n2_anEthrAddress[i] );
	} else {
//      printk( "Hw address %d - %02x\n", i, inb_p(e8390_base + EN1_PHYS_SHIFT(i)) );
	}
    }

    outb_p(psDev->n2_nRXStartPage, e8390_base + EN1_CURPAG);
    outb_p(E8390_NODMA+E8390_PAGE0+E8390_STOP, e8390_base+E8390_CMD);

    psDev->n2_bTxBusy = 0;
    psDev->n2_bIrqActive = 0;
    psDev->n2_nTX1 = psDev->n2_nTX2 = 0;
    psDev->n2_nTXing = 0;

    if (startp) 
    {
	outb_p(0xff,  e8390_base + EN0_ISR);
	outb_p(ENISR_ALL,  e8390_base + EN0_IMR);
	outb_p(E8390_NODMA+E8390_PAGE0+E8390_START, e8390_base+E8390_CMD);
	outb_p(E8390_TXCONFIG, e8390_base + EN0_TXCR); /* xmit on. */
	  /* 3c503 TechMan says rxconfig only after the NIC is started. */
	outb_p(E8390_RXCONFIG, e8390_base + EN0_RXCR); /* rx on,  */
	do_set_multicast_list( psDev );	/* (re)load the mcast table */
    }
    return;
}

/*****************************************************************************
 * NAME:
 * DESC:
 *	Trigger a transmit start, assuming the length is valid. 
 *	Always called with the page lock held
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

   
static void NS8390_trigger_send( NE2Dev_s* psDev, uint32 length, int start_page )
{
    int e8390_base = psDev->n2_nIoBase;
   
    outb_p(E8390_NODMA+E8390_PAGE0, e8390_base+E8390_CMD);
    
    if (inb_p(e8390_base) & E8390_TRANS) 
    {
	printk(KERN_WARNING "%s: trigger_send() called with the transmitter busy.\n", psDev->n2_pzName );
	return;
    }
    outb_p(length & 0xff, e8390_base + EN0_TCNTLO);
    outb_p(length >> 8, e8390_base + EN0_TCNTHI);
    outb_p(start_page, e8390_base + EN0_TPSR);
    outb_p(E8390_NODMA+E8390_TRANS+E8390_START, e8390_base+E8390_CMD);
}

/*****************************************************************************
 * NAME:
 * DESC:
 *	We have finished a transmit: check for errors and then trigger the next
 *	packet to be sent. Called with lock held
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/


static bool ei_tx_intr( NE2Dev_s* psDev )
{
    int e8390_base = psDev->n2_nIoBase;
    int status = inb(e8390_base + EN0_TSR);
    
    outb_p(ENISR_TX, e8390_base + EN0_ISR); /* Ack intr. */


#ifdef EI_PINGPONG

      /*
       * There are two Tx buffers, see which one finished, and trigger
       * the send of another one if it exists.
       */
    psDev->n2_nTXQueue--;

    if (psDev->n2_nTX1 < 0) 
    {
	if (psDev->n2_nLastTX != 1 && psDev->n2_nLastTX != -1)
	    printk(KERN_ERR "%s: bogus last_tx_buffer %d, tx1=%d.\n",
		   psDev->n2_pzName, psDev->n2_nLastTX, psDev->n2_nTX1);
	psDev->n2_nTX1 = 0;
//    dev->tbusy = 0;
	psDev->n2_bTxBusy = 0;
	if (psDev->n2_nTX2 > 0) 
	{
	    psDev->n2_nTXing = 1;
	    NS8390_trigger_send( psDev, psDev->n2_nTX2, psDev->n2_nTXStartPage + 6);
	    psDev->n2_nTransStart = get_real_time();
	    psDev->n2_nTX2 = -1,
		psDev->n2_nLastTX = 2;
	}
	else psDev->n2_nLastTX = 20, psDev->n2_nTXing = 0;	
    }
    else if (psDev->n2_nTX2 < 0) 
    {
	if (psDev->n2_nLastTX != 2  &&  psDev->n2_nLastTX != -2)
	    printk("%s: bogus last_tx_buffer %d, tx2=%d.\n", psDev->n2_pzName, psDev->n2_nLastTX, psDev->n2_nTX2 );
	psDev->n2_nTX2 = 0;
	psDev->n2_bTxBusy = 0;
//    dev->tbusy = 0;
	if (psDev->n2_nTX1 > 0) 
	{
	    psDev->n2_nTXing = 1;
	    NS8390_trigger_send( psDev, psDev->n2_nTX1, psDev->n2_nTXStartPage);
	    psDev->n2_nTransStart = get_real_time();
//      dev->trans_start = jiffies;
	    psDev->n2_nTX1 = -1;
	    psDev->n2_nLastTX = 1;
	}
	else
	    psDev->n2_nLastTX = 10, psDev->n2_nTXing = 0;
    }
    else printk(KERN_WARNING "%s: unexpected TX-done interrupt, lasttx=%d.\n", psDev->n2_pzName, psDev->n2_nLastTX);

#else	/* EI_PINGPONG */
      /*
       *  Single Tx buffer: mark it free so another packet can be loaded.
       */
    psDev->n2_nTXing = 0;
    psDev->n2_bTxBusy = 0;
//  dev->tbusy = 0;
#endif

      /* Minimize Tx latency: update the statistics after we restart TXing. */
  
    if (status & ENTSR_COL)
	psDev->n2_sStat.collisions++;
    if (status & ENTSR_PTX)
	psDev->n2_sStat.tx_packets++;
    else 
    {
	psDev->n2_sStat.tx_errors++;
	if (status & ENTSR_ABT) 
	{
	    psDev->n2_sStat.tx_aborted_errors++;
	    psDev->n2_sStat.collisions += 16;
	}
	if (status & ENTSR_CRS) 
	    psDev->n2_sStat.tx_carrier_errors++;
	if (status & ENTSR_FU) 
	    psDev->n2_sStat.tx_fifo_errors++;
	if (status & ENTSR_CDH)
	    psDev->n2_sStat.tx_heartbeat_errors++;
	if (status & ENTSR_OWC)
	    psDev->n2_sStat.tx_window_errors++;
    }

    return( wakeup_sem( psDev->n2_hTxSem, false ) > 0 );

//  mark_bh (NET_BH);
}

/*****************************************************************************
 * NAME:
 * DESC:
 *	Hard reset the card.  This used to pause for the same period that a
 *	8390 reset command required, but that shouldn't be necessary.
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static void ei_reset_8390( NE2Dev_s* psDev )
{
    int nIoBase = psDev->n2_nIoBase;
    bigtime_t reset_start_time = get_real_time();

    if (ei_debug > 1) 
	printk("resetting the 8390 t=%Ld...", get_real_time() );

      /* DON'T change these to inb_p/outb_p or reset will fail on clones. */
    outb(inb(nIoBase + NE_RESET), nIoBase + NE_RESET);

    psDev->n2_nTXing = 0;
    psDev->n2_nDMAing = 0;

      /* This check _should_not_ be necessary, omit eventually. */
    while ((inb_p(nIoBase+EN0_ISR) & ENISR_RESET) == 0)
    {
	if (get_real_time() - reset_start_time > 20000 ) {
	    printk("%s: ei_reset_8390() did not complete.\n", psDev->n2_pzName );
	    break;
	}
    }
    outb_p(ENISR_RESET, nIoBase + EN0_ISR);	/* Ack intr. */
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static void ei_block_input( NE2Dev_s* psDev, int count, char* buf/* struct sk_buff *skb*/, int ring_offset)
{
#ifdef NE_SANITY_CHECK
    int xfer_count = count;
#endif
    int nic_base = psDev->n2_nIoBase; // dev->base_addr;
//  char *buf = skb->data;

      /* This *shouldn't* happen. 
	 If it does, it's the last thing you'll see */
    if (psDev->n2_nDMAing) {
	printk("%s: DMAing conflict in ne_block_input "
	       "[DMAstat:%d][irqlock:%d][intr:%d].\n",
	       psDev->n2_pzName, psDev->n2_nDMAing, psDev->n2_nIRQLock,
	       psDev->n2_bIrqActive);
	return;
    }
    psDev->n2_nDMAing |= 0x01;
    outb_p(E8390_NODMA+E8390_PAGE0+E8390_START, nic_base+ NE_CMD);
    outb_p(count & 0xff, nic_base + EN0_RCNTLO);
    outb_p(count >> 8, nic_base + EN0_RCNTHI);
    outb_p(ring_offset & 0xff, nic_base + EN0_RSARLO);
    outb_p(ring_offset >> 8, nic_base + EN0_RSARHI);
    outb_p(E8390_RREAD+E8390_START, nic_base + NE_CMD);
    if (psDev->n2_nWord16) {
	insw(nic_base + NE_DATAPORT,buf,count>>1);
	if (count & 0x01) {
	    buf[count-1] = inb(nic_base + NE_DATAPORT);
#ifdef NE_SANITY_CHECK
	    xfer_count++;
#endif
	}
    } else {
	insb(nic_base + NE_DATAPORT, buf, count);
    }

#ifdef NE_SANITY_CHECK
      /* This was for the ALPHA version only, but enough people have
	 been encountering problems so it is still here.  If you see
	 this message you either 1) have a slightly incompatible clone
	 or 2) have noise/speed problems with your bus. */
    if (ei_debug > 1) {	/* DMA termination address check... */
	int addr, tries = 20;
	do {
	      /* DON'T check for 'inb_p(EN0_ISR) & ENISR_RDC' here
		 -- it's broken for Rx on some cards! */
	    int high = inb_p(nic_base + EN0_RSARHI);
	    int low = inb_p(nic_base + EN0_RSARLO);
	    addr = (high << 8) + low;
	    if (((ring_offset + xfer_count) & 0xff) == low)
		break;
	} while (--tries > 0);
	if (tries <= 0)
	    printk("%s: RX transfer address mismatch,"
		   "%#4.4x (expected) vs. %#4.4x (actual).\n",
		   psDev->n2_pzName, ring_offset + xfer_count, addr);
    }
#endif
    outb_p(ENISR_RDC, nic_base + EN0_ISR);	/* Ack intr. */
    psDev->n2_nDMAing &= ~0x01;
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static void ei_block_output( NE2Dev_s* psDev, int count,
		const unsigned char *buf, const int start_page)
{
    int nic_base = psDev->n2_nIoBase;
//  unsigned long dma_start;
    bigtime_t	dma_start;
#ifdef NE_SANITY_CHECK
    int retries = 0;
#endif

      /* Round the count up for word writes. Do we need to do this?
	 What effect will an odd byte count have on the 8390?
	 I should check someday. */
    if (psDev->n2_nWord16 && (count & 0x01))
	count++;

      /* This *shouldn't* happen. 
	 If it does, it's the last thing you'll see */
    if (psDev->n2_nDMAing) {
	printk("%s: DMAing conflict in ne_block_output."
	       "[DMAstat:%d][irqlock:%d][intr:%d]\n",
	       psDev->n2_pzName, psDev->n2_nDMAing, psDev->n2_nIRQLock,
	       psDev->n2_bIrqActive);
	return;
    }
    psDev->n2_nDMAing |= 0x01;
      /* We should already be in page 0, but to be safe... */
    outb_p(E8390_PAGE0+E8390_START+E8390_NODMA, nic_base + NE_CMD);

#ifdef NE_SANITY_CHECK
retry:
#endif

#ifdef NE8390_RW_BUGFIX
      /* Handle the read-before-write bug the same way as the
	 Crynwr packet driver -- the NatSemi method doesn't work.
	 Actually this doesn't always work either, but if you have
	 problems with your NEx000 this is better than nothing! */
    outb_p(0x42, nic_base + EN0_RCNTLO);
    outb_p(0x00, nic_base + EN0_RCNTHI);
    outb_p(0x42, nic_base + EN0_RSARLO);
    outb_p(0x00, nic_base + EN0_RSARHI);
    outb_p(E8390_RREAD+E8390_START, nic_base + NE_CMD);
      /* Make certain that the dummy read has occurred. */
    SLOW_DOWN_IO;
    SLOW_DOWN_IO;
    SLOW_DOWN_IO;
#endif

    outb_p(ENISR_RDC, nic_base + EN0_ISR);

      /* Now the normal output. */
    outb_p(count & 0xff, nic_base + EN0_RCNTLO);
    outb_p(count >> 8,   nic_base + EN0_RCNTHI);
    outb_p(0x00, nic_base + EN0_RSARLO);
    outb_p(start_page, nic_base + EN0_RSARHI);

    outb_p(E8390_RWRITE+E8390_START, nic_base + NE_CMD);
    if (psDev->n2_nWord16) {
	outsw(nic_base + NE_DATAPORT, buf, count>>1);
    } else {
	outsb(nic_base + NE_DATAPORT, buf, count);
    }

//  dma_start = jiffies;
    dma_start = get_real_time();

#ifdef NE_SANITY_CHECK
      /* This was for the ALPHA version only, but enough people have
	 been encountering problems so it is still here. */

    if (ei_debug > 1) {		/* DMA termination address check... */
	int addr, tries = 20;
	do {
	    int high = inb_p(nic_base + EN0_RSARHI);
	    int low = inb_p(nic_base + EN0_RSARLO);
	    addr = (high << 8) + low;
	    if ((start_page << 8) + count == addr)
		break;
	} while (--tries > 0);
	if (tries <= 0) {
	    printk("%s: Tx packet transfer address mismatch,"
		   "%#4.4x (expected) vs. %#4.4x (actual).\n",
		   psDev->n2_pzName, (start_page << 8) + count, addr);
	    if (retries++ == 0)
		goto retry;
	}
    }
#endif

    while ((inb_p(nic_base + EN0_ISR) & ENISR_RDC) == 0)
//    if (jiffies - dma_start > 2*HZ/100) {		/* 20ms */
	if ( get_real_time() - dma_start > 20000LL ) {	/* 20ms */
	    printk("%s: timeout waiting for Tx RDC.\n", psDev->n2_pzName );
	    ei_reset_8390( psDev );
	    NS8390_init( psDev, 1);
	    break;
	}

    outb_p(ENISR_RDC, nic_base + EN0_ISR);	/* Ack intr. */
    psDev->n2_nDMAing &= ~0x01;
    return;
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int ei_start_xmit( NE2Dev_s* psDev, const void* pBuffer, int length /*struct sk_buff *skb, struct device *dev */ )
{
    int e8390_base = psDev->n2_nIoBase;
    int /*length,*/ send_length, output_page;
    unsigned long flags;

      /*
       *  We normally shouldn't be called if dev->tbusy is set, but the
       *  existing code does anyway. If it has been too long since the
       *  last Tx, we assume the board has died and kick it. We are
       *  bh_atomic here.
       */
 
    if (psDev->n2_bTxBusy) 
    {	/* Do timeouts, just like the 8003 driver. */
	int txsr;
	int isr;
	bigtime_t tickssofar = get_real_time() - psDev->n2_nTransStart;

	  /*
	   *	Need the page lock. Now see what went wrong. This bit is
	   *	fast.
	   */
		 		
	spinlock_cli(&psDev->n2_nPageLock, flags);
	txsr = inb(e8390_base+EN0_TSR);
	if (tickssofar < TX_TIMEOUT ||	(tickssofar < (TX_TIMEOUT+50000) && ! (txsr & ENTSR_PTX))) 
	{
	    spinunlock_restore(&psDev->n2_nPageLock, flags);
	    return 1;
	}

	psDev->n2_sStat.tx_errors++;
	isr = inb(e8390_base+EN0_ISR);
	if ( psDev->n2_bStarted == 0) 
	{
	    spinunlock_restore(&psDev->n2_nPageLock, flags);
	    printk(KERN_WARNING "%s: xmit on stopped card\n", psDev->n2_pzName );
	    return 1;
	}

	  /*
	   * Note that if the Tx posted a TX_ERR interrupt, then the
	   * error will have been handled from the interrupt handler
	   * and not here. Error statistics are handled there as well.
	   */

	printk(KERN_DEBUG "%s: Tx timed out, %s TSR=%#2x, ISR=%#2x, t=%Ld.\n",
	       psDev->n2_pzName, (txsr & ENTSR_ABT) ? "excess collisions." :
	       (isr) ? "lost interrupt?" : "cable problem?", txsr, isr, tickssofar);

	if (!isr && !psDev->n2_sStat.tx_packets)
	{
	      /* The 8390 probably hasn't gotten on the cable yet. */
	    psDev->n2_nInterfaceNum ^= 1;   /* Try a different xcvr.  */
	}

	  /*
	   *	Play shuffle the locks, a reset on some chips takes a few
	   *	mS. We very rarely hit this point.
	   */
		 
	spinunlock_restore(&psDev->n2_nPageLock, flags);

	  /* Ugly but a reset can be slow, yet must be protected */
		
	disable_irq_nosync( psDev->n2_nIrq );
	spinlock_cli(&psDev->n2_nPageLock, flags);
		
	  /* Try to restart the card.  Perhaps the user has fixed something. */
	ei_reset_8390( psDev );
	NS8390_init( psDev, 1 );
		
	spinunlock_restore(&psDev->n2_nPageLock, flags);
	enable_irq( psDev->n2_nIrq );
	psDev->n2_nTransStart = get_real_time();
    }
    
//  length = skb->len;

      /* Mask interrupts from the ethercard. 
	 SMP: We have to grab the lock here otherwise the IRQ handler
	 on another CPU can flip window and race the IRQ mask set. We end
	 up trashing the mcast filter not disabling irqs if we dont lock */
	   
    spinlock_cli(&psDev->n2_nPageLock, flags);
    outb_p(0x00, e8390_base + EN0_IMR);
    spinunlock_restore(&psDev->n2_nPageLock, flags);
	
	
      /*
       *	Slow phase with lock held.
       */
	 
    disable_irq_nosync( psDev->n2_nIrq );
	
    spinlock_cli(&psDev->n2_nPageLock, flags);
/*	
	if (dev->interrupt) 
	{
	printk(KERN_WARNING "%s: Tx request while isr active.\n", psDev->n2_pzName );
	outb_p(ENISR_ALL, e8390_base + EN0_IMR);
	spin_unlock(&psDev->n2_nPageLock);
	enable_irq(dev->n2_nIrq);
	psDev->n2_sStat.tx_errors++;
	dev_kfree_skb(skb);
	return 0;
	}
	*/  
    psDev->n2_nIRQLock = 1;

    send_length = ETH_ZLEN < length ? length : ETH_ZLEN;
    
#ifdef EI_PINGPONG

      /*
       * We have two Tx slots available for use. Find the first free
       * slot, and then perform some sanity checks. With two Tx bufs,
       * you get very close to transmitting back-to-back packets. With
       * only one Tx buf, the transmitter sits idle while you reload the
       * card, leaving a substantial gap between each transmitted packet.
       */

    if (psDev->n2_nTX1 == 0) 
    {
	output_page = psDev->n2_nTXStartPage;
	psDev->n2_nTX1 = send_length;
	if (ei_debug  &&  psDev->n2_nTX2 > 0)
	    printk(KERN_DEBUG "%s: idle transmitter tx2=%d, lasttx=%d, txing=%d.\n",
		   psDev->n2_pzName, psDev->n2_nTX2, psDev->n2_nLastTX, psDev->n2_nTXing);
    }
    else if (psDev->n2_nTX2 == 0) 
    {
	output_page = psDev->n2_nTXStartPage + TX_1X_PAGES;
	psDev->n2_nTX2 = send_length;
	if (ei_debug  &&  psDev->n2_nTX1 > 0)
	    printk(KERN_DEBUG "%s: idle transmitter, tx1=%d, lasttx=%d, txing=%d.\n",
		   psDev->n2_pzName, psDev->n2_nTX1, psDev->n2_nLastTX, psDev->n2_nTXing);
    }
    else
    {	/* We should never get here. */
	if (ei_debug)
	    printk(KERN_DEBUG "%s: No Tx buffers free! irq=%d tx1=%d tx2=%d last=%d\n",
		   psDev->n2_pzName, psDev->n2_bIrqActive/*dev->interrupt*/, psDev->n2_nTX1, psDev->n2_nTX2, psDev->n2_nLastTX);
	psDev->n2_nIRQLock = 0;
	psDev->n2_bTxBusy = 1;
	outb_p(ENISR_ALL, e8390_base + EN0_IMR);
	spinunlock_restore(&psDev->n2_nPageLock, flags );
	enable_irq( psDev->n2_nIrq );
	psDev->n2_sStat.tx_errors++;
	return 1;
    }

      /*
       * Okay, now upload the packet and trigger a send if the transmitter
       * isn't already sending. If it is busy, the interrupt handler will
       * trigger the send later, upon receiving a Tx done interrupt.
       */

    ei_block_output( psDev, length, pBuffer/* skb->data*/, output_page);
    if (! psDev->n2_nTXing) 
    {
	psDev->n2_nTXing = 1;
	NS8390_trigger_send( psDev, send_length, output_page);
	psDev->n2_nTransStart = get_real_time();
	if (output_page == psDev->n2_nTXStartPage) 
	{
	    psDev->n2_nTX1 = -1;
	    psDev->n2_nLastTX = -1;
	}
	else 
	{
	    psDev->n2_nTX2 = -1;
	    psDev->n2_nLastTX = -2;
	}
    }
    else psDev->n2_nTXQueue++;

    psDev->n2_bTxBusy = (psDev->n2_nTX1  &&  psDev->n2_nTX2);

#else	/* EI_PINGPONG */

      /*
       * Only one Tx buffer in use. You need two Tx bufs to come close to
       * back-to-back transmits. Expect a 20 -> 25% performance hit on
       * reasonable hardware if you only use one Tx buffer.
       */

    ei_block_output( psDev, length, pBuffer /*skb->data*/, psDev->n2_nTXStartPage);
    psDev->n2_nTXing = 1;
    NS8390_trigger_send( psDev, send_length, psDev->n2_nTXStartPage);
    psDev->n2_nTransStart = get_real_time();
    psDev->n2_bTxBusy = 1;

#endif	/* EI_PINGPONG */

      /* Turn 8390 interrupts back on. */
    psDev->n2_nIRQLock = 0;
    outb_p(ENISR_ALL, e8390_base + EN0_IMR);
	
    spinunlock_restore(&psDev->n2_nPageLock, flags );
    enable_irq( psDev->n2_nIrq );

//  dev_kfree_skb (skb);
  psDev->n2_sStat.tx_bytes += send_length;
    
    return 0;
}

/*****************************************************************************
 * NAME:
 * DESC:
 *	Grab the 8390 specific header. Similar to the block_input routine, but
 *	we don't need to be concerned with ring wrap as the header will be at
 *	the start of a page, so we optimize accordingly. 
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/


static void ei_get_8390_hdr( NE2Dev_s* psDev, struct e8390_pkt_hdr *hdr, 
		int ring_page)
{
    int nic_base = psDev->n2_nIoBase;

      /* This *shouldn't* happen. 
	 If it does, it's the last thing you'll see */
    if (psDev->n2_nDMAing) {
	printk("%s: DMAing conflict in ne_get_8390_hdr "
	       "[DMAstat:%d][irqlock:%d][intr:%d].\n",
	       psDev->n2_pzName, psDev->n2_nDMAing, psDev->n2_nIRQLock,
	       psDev->n2_bIrqActive);
	return;
    }

    psDev->n2_nDMAing |= 0x01;
    outb_p(E8390_NODMA+E8390_PAGE0+E8390_START, nic_base+ NE_CMD);
    outb_p(sizeof(struct e8390_pkt_hdr), nic_base + EN0_RCNTLO);
    outb_p(0, nic_base + EN0_RCNTHI);
    outb_p(0, nic_base + EN0_RSARLO);		/* On page boundary */
    outb_p(ring_page, nic_base + EN0_RSARHI);
    outb_p(E8390_RREAD+E8390_START, nic_base + NE_CMD);

    if (psDev->n2_nWord16)
	insw(nic_base + NE_DATAPORT, hdr, 
	     sizeof(struct e8390_pkt_hdr)>>1);
    else
	insb(nic_base + NE_DATAPORT, hdr, 
	     sizeof(struct e8390_pkt_hdr));

    outb_p(ENISR_RDC, nic_base + EN0_ISR);	/* Ack intr. */
    psDev->n2_nDMAing &= ~0x01;
}


/*****************************************************************************
 * NAME:
 * DESC:
 *	A transmitter error has happened. Most likely excess collisions (which
 *	is a fairly normal condition). If the error is one where the Tx will
 *	have been aborted, we try and send another one right away, instead of
 *	letting the failed packet sit and collect dust in the Tx buffer. This
 *	is a much better solution as it avoids kernel based Tx timeouts, and
 *	an unnecessary card reset.
 *
 * NOTE:
 *	Called with lock held
 * SEE ALSO:
 ****************************************************************************/

static void ei_tx_err( NE2Dev_s* psDev )
{
    int e8390_base = psDev->n2_nIoBase;
//  struct ei_device *ei_local = &g_sDev; // (struct ei_device *) dev->priv;
    unsigned char txsr = inb_p(e8390_base+EN0_TSR);
    unsigned char tx_was_aborted = txsr & (ENTSR_ABT+ENTSR_FU);

#ifdef VERBOSE_ERROR_DUMP
    printk(KERN_DEBUG "%s: transmitter error (%#2x): ", psDev->n2_pzName, txsr);
    if (txsr & ENTSR_ABT)
	printk("excess-collisions ");
    if (txsr & ENTSR_ND)
	printk("non-deferral ");
    if (txsr & ENTSR_CRS)
	printk("lost-carrier ");
    if (txsr & ENTSR_FU)
	printk("FIFO-underrun ");
    if (txsr & ENTSR_CDH)
	printk("lost-heartbeat ");
    printk("\n");
#endif

    outb_p(ENISR_TX_ERR, e8390_base + EN0_ISR); /* Ack intr. */

    if (tx_was_aborted)
	ei_tx_intr(  psDev );
    else 
    {
	psDev->n2_sStat.tx_errors++;
	if (txsr & ENTSR_CRS) psDev->n2_sStat.tx_carrier_errors++;
	if (txsr & ENTSR_CDH) psDev->n2_sStat.tx_heartbeat_errors++;
	if (txsr & ENTSR_OWC) psDev->n2_sStat.tx_window_errors++;
    }
}



/*****************************************************************************
 * NAME:
 * DESC:
 *	We have a good packet(s), get it/them out of the buffers. 
 * NOTE:
 *	Called with lock held
 * SEE ALSO:
 ****************************************************************************/

static void ei_receive( NE2Dev_s* psDev )
{
    int e8390_base = psDev->n2_nIoBase;
    unsigned char rxing_page, this_frame, next_frame;
    unsigned short current_offset;
    int rx_pkt_count = 0;
    struct e8390_pkt_hdr rx_frame;
    int num_rx_pages = psDev->n2_nStopPage-psDev->n2_nRXStartPage;

    while (++rx_pkt_count < 10) 
    {
	int pkt_len, pkt_stat;
		
	  /* Get the rx page (incoming packet pointer). */
	outb_p(E8390_NODMA+E8390_PAGE1, e8390_base + E8390_CMD);
	rxing_page = inb_p(e8390_base + EN1_CURPAG);
	outb_p(E8390_NODMA+E8390_PAGE0, e8390_base + E8390_CMD);
		
	  /* Remove one frame from the ring.  Boundary is always a page behind. */
	this_frame = inb_p(e8390_base + EN0_BOUNDARY) + 1;
	if (this_frame >= psDev->n2_nStopPage)
	    this_frame = psDev->n2_nRXStartPage;
		
	  /* Someday we'll omit the previous, iff we never get this message.
	     (There is at least one clone claimed to have a problem.)  */
	if (ei_debug > 0  &&  this_frame != psDev->n2_nCurrentPage)
	    printk(KERN_ERR "%s: mismatched read page pointers %2x vs %2x.\n",
		   psDev->n2_pzName, this_frame, psDev->n2_nCurrentPage);
		
	if (this_frame == rxing_page)	/* Read all the frames? */
	    break;				/* Done for now */
		
	current_offset = this_frame << 8;
	ei_get_8390_hdr( psDev, &rx_frame, this_frame);
		
	pkt_len = rx_frame.count - sizeof(struct e8390_pkt_hdr);
	pkt_stat = rx_frame.status;
		
	next_frame = this_frame + 1 + ((pkt_len+4)>>8);
		
	  /* Check for bogosity warned by 3c503 book: the status byte is never
	     written.  This happened a lot during testing! This code should be
	     cleaned up someday. */
	if (rx_frame.next != next_frame
	    && rx_frame.next != next_frame + 1
	    && rx_frame.next != next_frame - num_rx_pages
	    && rx_frame.next != next_frame + 1 - num_rx_pages) {
	    psDev->n2_nCurrentPage = rxing_page;
	    outb(psDev->n2_nCurrentPage-1, e8390_base+EN0_BOUNDARY);
	    printk( KERN_DEBUG "ei_receive() receive error\n" );
	    psDev->n2_sStat.rx_errors++;
	    continue;
	}

	if (pkt_len < 60  ||  pkt_len > 1518) 
	{
	    if (ei_debug)
		printk(KERN_DEBUG "%s: bogus packet size: %d, status=%#2x nxpg=%#2x.\n",
		       psDev->n2_pzName, rx_frame.count, rx_frame.status,
		       rx_frame.next);
	    psDev->n2_sStat.rx_errors++;
	    psDev->n2_sStat.rx_length_errors++;
	}
	else if ((pkt_stat & 0x0F) == ENRSR_RXOK) 
	{
//      static uint8 zBuf[4096];
//      char	zLine[128];
//      int	nPos;

	    NEPacketBuf_s* psBuf = alloc_rcv_packet( psDev, pkt_len );

	    if ( NULL == psBuf ) {
		printk( KERN_DEBUG "%s could not allocate a receive packet buffer of %d bytes\n", psDev->n2_pzName, pkt_len );
		break;
	    }

	    ei_block_input( psDev, pkt_len, psBuf->pBuffer, current_offset + sizeof(rx_frame) );

	    add_rcv_buffer( psDev, psBuf );
#ifndef __ATHEOS__
	    struct sk_buff *skb;
			
	    skb = dev_alloc_skb(pkt_len+2);
	    if (skb == NULL) 
	    {
		if (ei_debug > 1)
		    printk(KERN_DEBUG "%s: Couldn't allocate a sk_buff of size %d.\n",
			   psDev->n2_pzName, pkt_len);
		psDev->n2_sStat.rx_dropped++;
		break;
	    }
	    else
	    {
		skb_reserve(skb,2);	/* IP headers on 16 byte boundaries */
		skb->dev = dev;
		skb_put(skb, pkt_len);	/* Make room */
		ei_block_input(dev, pkt_len, skb, current_offset + sizeof(rx_frame));
		skb->protocol=eth_type_trans(skb,dev);
		netif_rx(skb);
		psDev->n2_sStat.rx_packets++;
		psDev->n2_sStat.rx_bytes += pkt_len;
		if (pkt_stat & ENRSR_PHY)
		    psDev->n2_sStat.multicast++;
	    }
#endif	
	} 
	else 
	{
	    if (ei_debug)
		printk(KERN_DEBUG "%s: bogus packet: status=%#2x nxpg=%#2x size=%d\n",
		       psDev->n2_pzName, rx_frame.status, rx_frame.next,
		       rx_frame.count);
	    psDev->n2_sStat.rx_errors++;
	      /* NB: The NIC counts CRC, frame and missed errors. */
//      if (pkt_stat & ENRSR_FO)
	    psDev->n2_sStat.rx_fifo_errors++;
	}
	next_frame = rx_frame.next;
		
	  /* This _should_ never happen: it's here for avoiding bad clones. */
	if (next_frame >= psDev->n2_nStopPage) {
	    printk("%s: next frame inconsistency, %#2x\n", psDev->n2_pzName, next_frame);
	    next_frame = psDev->n2_nRXStartPage;
	}
	psDev->n2_nCurrentPage = next_frame;
	outb_p(next_frame-1, e8390_base+EN0_BOUNDARY);
    }

      /* We used to also ack ENISR_OVER here, but that would sometimes mask
	 a real overrun, leaving the 8390 in a stopped state with rec'vr off. */
    outb_p(ENISR_RX+ENISR_RX_ERR, e8390_base+EN0_ISR);
    return;
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

/* 
 * We have a receiver overrun: we have to kick the 8390 to get it started
 * again. Problem is that you have to kick it exactly as NS prescribes in
 * the updated datasheets, or "the NIC may act in an unpredictable manner."
 * This includes causing "the NIC to defer indefinitely when it is stopped
 * on a busy network."  Ugh.
 * Called with lock held. Don't call this with the interrupts off or your
 * computer will hate you - it takes 10mS or so. 
 */

static void ei_rx_overrun( NE2Dev_s* psDev )
{
    int e8390_base = psDev->n2_nIoBase;
    unsigned char was_txing, must_resend = 0;
//  struct ei_device *ei_local = &g_sDev; // (struct ei_device *) dev->priv;
    
      /*
       * Record whether a Tx was in progress and then issue the
       * stop command.
       */
    was_txing = inb_p(e8390_base+E8390_CMD) & E8390_TRANS;
    outb_p(E8390_NODMA+E8390_PAGE0+E8390_STOP, e8390_base+E8390_CMD);
    
    if (ei_debug > 1)
	printk(KERN_DEBUG "%s: Receiver overrun.\n", psDev->n2_pzName );
    psDev->n2_sStat.rx_over_errors++;
    
      /* 
       * Wait a full Tx time (1.2ms) + some guard time, NS says 1.6ms total.
       * Early datasheets said to poll the reset bit, but now they say that
       * it "is not a reliable indicator and subsequently should be ignored."
       * We wait at least 10ms.
       */

    udelay(10*1000);

      /*
       * Reset RBCR[01] back to zero as per magic incantation.
       */
    outb_p(0x00, e8390_base+EN0_RCNTLO);
    outb_p(0x00, e8390_base+EN0_RCNTHI);

      /*
       * See if any Tx was interrupted or not. According to NS, this
       * step is vital, and skipping it will cause no end of havoc.
       */

    if (was_txing)
    { 
	unsigned char tx_completed = inb_p(e8390_base+EN0_ISR) & (ENISR_TX+ENISR_TX_ERR);
	if (!tx_completed)
	    must_resend = 1;
    }

      /*
       * Have to enter loopback mode and then restart the NIC before
       * you are allowed to slurp packets up off the ring.
       */
    outb_p(E8390_TXOFF, e8390_base + EN0_TXCR);
    outb_p(E8390_NODMA + E8390_PAGE0 + E8390_START, e8390_base + E8390_CMD);

      /*
       * Clear the Rx ring of all the debris, and ack the interrupt.
       */
    ei_receive( psDev );
    outb_p(ENISR_OVER, e8390_base+EN0_ISR);

      /*
       * Leave loopback mode, and resend any packet that got stopped.
       */
    outb_p(E8390_TXCONFIG, e8390_base + EN0_TXCR); 
    if (must_resend)
	outb_p(E8390_NODMA + E8390_PAGE0 + E8390_START + E8390_TRANS, e8390_base + E8390_CMD);
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

/* The typical workload of the driver:
   Handle the ether interface interrupts. */

static int ei_interrupt( int nIrqNum, void* pData, SysCallRegs_s* psRegs )
{
    NE2Dev_s* psDev = pData;
//	struct device *dev = dev_id;
    int e8390_base;
    int interrupts, nr_serviced = 0;
    bool bSchedule = false;
  
/*    
      if (dev == NULL) 
      {
      printk ("net_interrupt(): irq %d for unknown device.\n", irq);
      return;
      }
      */  
    e8390_base = psDev->n2_nIoBase;

//  printk( "Got IRQ %d\n", nIrqNum );
      /*
       *	Protect the irq test too.
       */
	 
    spinlock(&psDev->n2_nPageLock);

    if (psDev->n2_bIrqActive || psDev->n2_nIRQLock) 
    {
#if 1 /* This might just be an interrupt for a PCI device sharing this line */
	  /* The "irqlock" check is only for testing. */
	printk(psDev->n2_nIRQLock
	       ? "%s: Interrupted while interrupts are masked! isr=%#2x imr=%#2x.\n"
	       : "%s: Reentering the interrupt handler! isr=%#2x imr=%#2x.\n",
	       psDev->n2_pzName, inb_p(e8390_base + EN0_ISR),
	       inb_p(e8390_base + EN0_IMR));
#endif
	spinunlock(&psDev->n2_nPageLock);
	return(0);
    }
	
    psDev->n2_bIrqActive = 1;
    
      /* Change to page 0 and read the intr status reg. */
    outb_p(E8390_NODMA+E8390_PAGE0, e8390_base + E8390_CMD);
    if (ei_debug > 3)
	printk(KERN_DEBUG "%s: interrupt(isr=%#2.2x).\n", psDev->n2_pzName,
	       inb_p(e8390_base + EN0_ISR));
    
      /* !!Assumption!! -- we stay in page 0.	 Don't break this. */
    while ((interrupts = inb_p(e8390_base + EN0_ISR)) != 0
	   && ++nr_serviced < MAX_SERVICE) 
    {
	if (psDev->n2_bStarted == 0) 
	{
	    printk(KERN_WARNING "%s: interrupt from stopped card\n", psDev->n2_pzName );
	    interrupts = 0;
	    break;
	}
	if (interrupts & ENISR_OVER) 
	    ei_rx_overrun( psDev );
	else if (interrupts & (ENISR_RX+ENISR_RX_ERR)) 
	{
	      /* Got a good (?) packet. */
	    ei_receive( psDev );
	}
	  /* Push the next to-transmit packet through. */
	if (interrupts & ENISR_TX) {
	    if ( ei_tx_intr( psDev ) ) {
		bSchedule = true;
	    }
	}
	else if (interrupts & ENISR_TX_ERR)
	    ei_tx_err( psDev );

	if (interrupts & ENISR_COUNTERS) 
	{
	    psDev->n2_sStat.rx_frame_errors += inb_p(e8390_base + EN0_COUNTER0);
	    psDev->n2_sStat.rx_crc_errors   += inb_p(e8390_base + EN0_COUNTER1);
	    psDev->n2_sStat.rx_missed_errors+= inb_p(e8390_base + EN0_COUNTER2);
	    outb_p(ENISR_COUNTERS, e8390_base + EN0_ISR); /* Ack intr. */
	}
		
	  /* Ignore any RDC interrupts that make it back to here. */
	if (interrupts & ENISR_RDC) 
	{
	    outb_p(ENISR_RDC, e8390_base + EN0_ISR);
	}
	outb_p(E8390_NODMA+E8390_PAGE0+E8390_START, e8390_base + E8390_CMD);
    }
    
    if (interrupts && ei_debug) 
    {
	outb_p(E8390_NODMA+E8390_PAGE0+E8390_START, e8390_base + E8390_CMD);
	if (nr_serviced >= MAX_SERVICE) 
	{
	    printk(KERN_WARNING "%s: Too much work at interrupt, status %#2.2x\n",
		   psDev->n2_pzName, interrupts);
	    outb_p(ENISR_ALL, e8390_base + EN0_ISR); /* Ack. most intrs. */
	} else {
	    printk(KERN_WARNING "%s: unknown interrupt %#2x\n", psDev->n2_pzName, interrupts);
	    outb_p(0xff, e8390_base + EN0_ISR); /* Ack. all intrs. */
	}
    }
    psDev->n2_bIrqActive = 0;
    spinunlock(&psDev->n2_nPageLock);
    return(0);
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int ethdev_init( NE2Dev_s* psDev )
{
    spinlock_init( &psDev->n2_nPageLock, "NE*000" );
    return 0;
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int ne_probe1( NE2Dev_s* psDev, int ioaddr)
{
    int i;
    unsigned char SA_prom[32];
    char	zEtherAddr[32];
    int wordlength = 2;
    const char *name = NULL;
    int start_page, stop_page;
    int neX000, ctron, copam, bad_card;
    int reg0 = inb_p(ioaddr);

//    printk(KERN_INFO "NE*000 check first port at %#3x:\n", ioaddr);
    if (reg0 == 0xFF)
	return ENODEV;

//    printk(KERN_INFO "NE*000 check signature at %#3x:\n", ioaddr);
  
      /* Do a preliminary verification that we have a 8390. */
    {
	int regd;
	outb_p(E8390_NODMA+E8390_PAGE1+E8390_STOP, ioaddr + E8390_CMD);
	regd = inb_p(ioaddr + 0x0d);
	outb_p(0xff, ioaddr + 0x0d);
	outb_p(E8390_NODMA+E8390_PAGE0, ioaddr + E8390_CMD);
	inb_p(ioaddr + EN0_COUNTER0); /* Clear the counter by reading. */
	if (inb_p(ioaddr + EN0_COUNTER0) != 0) {
	    outb_p(reg0, ioaddr);
	    outb_p(regd, ioaddr + 0x0d);	/* Restore the old values. */
	    return ENODEV;
	}
    }

//    printk(KERN_INFO "NE*000 ethercard probe at %#3x:\n", ioaddr);

      /* A user with a poor card that fails to ack the reset, or that
	 does not have a valid 0x57,0x57 signature can still use this
	 without having to recompile. Specifying an i/o address along
	 with an otherwise unused dev->mem_end value of "0xBAD" will
	 cause the driver to skip these parts of the probe. */

    bad_card = 1; // ((dev->base_addr != 0) && (dev->mem_end == 0xbad));

      /* Reset card. Who knows what dain-bramaged state it was left in. */

    {
//    unsigned long reset_start_time = jiffies;
	bigtime_t reset_start_time = get_real_time();

	  /* DON'T change these to inb_p/outb_p or reset will fail on clones. */
	outb(inb(ioaddr + NE_RESET), ioaddr + NE_RESET);

	while ((inb_p(ioaddr + EN0_ISR) & ENISR_RESET) == 0)
	{
//      if (jiffies - reset_start_time > 2*HZ/100) {
	    if ( get_real_time() - reset_start_time > 20000 ) {
		if (bad_card) {
		    printk(" (warning: no reset ack)\n");
		    break;
		} else {
		    printk(" not found (no reset ack).\n");
		    return ENODEV;
		}
	    }
	}
	outb_p(0xff, ioaddr + EN0_ISR);		/* Ack all intr. */
    }

      /* Read the 16 bytes of station address PROM.
	 We must first initialize registers, similar to NS8390_init(eifdev, 0).
	 We can't reliably read the SAPROM address without this.
	 (I learned the hard way!). */
    {
	struct {unsigned char value, offset; } program_seq[] = 
					       {
						   {E8390_NODMA+E8390_PAGE0+E8390_STOP, E8390_CMD}, /* Select page 0*/
						   {0x48,	EN0_DCFG},	/* Set byte-wide (0x48) access. */
						   {0x00,	EN0_RCNTLO},	/* Clear the count regs. */
						   {0x00,	EN0_RCNTHI},
						   {0x00,	EN0_IMR},	/* Mask completion irq. */
						   {0xFF,	EN0_ISR},
						   {E8390_RXOFF, EN0_RXCR},	/* 0x20  Set to monitor */
						   {E8390_TXOFF, EN0_TXCR},	/* 0x02  and loopback mode. */
						   {32,	EN0_RCNTLO},
						   {0x00,	EN0_RCNTHI},
						   {0x00,	EN0_RSARLO},	/* DMA starting at 0x0000. */
						   {0x00,	EN0_RSARHI},
						   {E8390_RREAD+E8390_START, E8390_CMD},
					       };

	for (i = 0; i < sizeof(program_seq)/sizeof(program_seq[0]); i++)
	    outb_p( program_seq[i].value, ioaddr + program_seq[i].offset );

    }
    for( i = 0 ; i < 32 /*sizeof(SA_prom)*/ ; i+=2 ) {
	SA_prom[i] = inb( ioaddr + NE_DATAPORT );
	SA_prom[i+1] = inb( ioaddr + NE_DATAPORT );
	if (SA_prom[i] != SA_prom[i+1])
	    wordlength = 1;
    }

      /* At this point, wordlength *only* tells us if the SA_prom is doubled
	 up or not because some broken PCI cards don't respect the byte-wide
	 request in program_seq above, and hence don't have doubled up values.
	 These broken cards would otherwise be detected as an ne1000.  */

  
    if (wordlength == 2)
	for (i = 0; i < 16; i++)
	    SA_prom[i] = SA_prom[i+i];

  
    if (pci_irq_line || ioaddr >= 0x400)
	wordlength = 2;		/* Catch broken PCI cards mentioned above. */

    if (wordlength == 2) 
    {
	  /* We must set the 8390 for word mode. */
	outb_p(0x49, ioaddr + EN0_DCFG);
	start_page = NESM_START_PG;
	stop_page = NESM_STOP_PG;
    } else {
	start_page = NE1SM_START_PG;
	stop_page = NE1SM_STOP_PG;
    }

    neX000 = (SA_prom[14] == 0x57  &&  SA_prom[15] == 0x57);
    ctron =  (SA_prom[0] == 0x00 && SA_prom[1] == 0x00 && SA_prom[2] == 0x1d);
    copam =  (SA_prom[14] == 0x49 && SA_prom[15] == 0x00);
	

      /* Set up the rest of the parameters. */
    if (neX000 || bad_card || copam) {
	name = (wordlength == 2) ? "NE2000" : "NE1000";
    }
    else if (ctron) 
    {
	name = (wordlength == 2) ? "Ctron-8" : "Ctron-16";
	start_page = 0x01;
	stop_page = (wordlength == 2) ? 0x40 : 0x20;
    }
    else
    {
#ifdef SUPPORT_NE_BAD_CLONES
	  /* Ack!  Well, there might be a *bad* NE*000 clone there.
	     Check for total bogus addresses. */
	for (i = 0; bad_clone_list[i].name8; i++) 
	{
	    if (SA_prom[0] == bad_clone_list[i].SAprefix[0] &&
		SA_prom[1] == bad_clone_list[i].SAprefix[1] &&
		SA_prom[2] == bad_clone_list[i].SAprefix[2]) 
	    {
		if (wordlength == 2) 
		{
		    name = bad_clone_list[i].name16;
		} else {
		    name = bad_clone_list[i].name8;
		}
		break;
	    }
	}
	if (bad_clone_list[i].name8 == NULL) 
	{
	    printk(" not found (invalid signature %2.2x %2.2x).\n",
		   SA_prom[14], SA_prom[15]);
	    return ENXIO;
	}
#else
	return ENXIO;
#endif
    }
    psDev->n2_nIrq = g_nISAIRQ;
    if (pci_irq_line)
	psDev->n2_nIrq = pci_irq_line;
    psDev->n2_nIoBase = ioaddr;
  
    zEtherAddr[0] = '\0';
    for(i = 0; i < ETHER_ADDR_LEN; i++) {
	char zBuf[8];
	sprintf( zBuf, " %2.2x", SA_prom[i] );
	strcat( zEtherAddr, zBuf );
	psDev->n2_anEthrAddress[i] = SA_prom[i];
    }
    printk( "Ethernet addr: %s\n", zEtherAddr );
  
    printk("%s: %s found at %#x, using IRQ %d.\n",
	   psDev->n2_pzName, name, ioaddr, psDev->n2_nIrq );

    psDev->n2_nTXStartPage = start_page;
    psDev->n2_nStopPage = stop_page;
    psDev->n2_nWord16 = (wordlength == 2);

    psDev->n2_nRXStartPage = start_page + TX_PAGES;
#ifdef PACKETBUF_MEMSIZE
      /* Allow the packet buffer size to be overridden by know-it-alls. */
    psDev->n2_nStopPage = psDev->n2_nTXStartPage + PACKETBUF_MEMSIZE;
#endif

    NS8390_init(  psDev, 0);
    
    return 0;
}

static int ne_probe_pci( int nDeviceID, NE2Dev_s* psDev )
{
    int i;
    PCI_Info_s sInfo;
    PCI_bus_s* psBus = get_busmanager( PCI_BUS_NAME, PCI_BUS_VERSION );
    if( psBus == NULL ) 
    {
    	return( -ENODEV );
    }
  
    for ( i = 0 ; psBus->get_pci_info( &sInfo, i ) == 0 ; ++i ) {
	int j;
	for (j = 0; pci_clone_list[j].vendor != 0; j++) {
	    if ( sInfo.nVendorID == pci_clone_list[j].vendor && sInfo.nDeviceID == pci_clone_list[j].dev_id ) {
		uint32 pci_ioaddr;
	
		pci_irq_line = sInfo.u.h0.nInterruptLine;
		pci_ioaddr   = sInfo.u.h0.nBase0 & PCI_ADDRESS_IO_MASK;

		if ( pci_irq_line != 0 ) {
		    printk(KERN_INFO "ne.c: PCI BIOS reports %s at i/o %#lx, irq %d.\n",
			   pci_clone_list[j].name,
			   pci_ioaddr, pci_irq_line);
	
		} else {
		    printk( "PCI ne2000 card %s has no IRQ\n", pci_clone_list[j].name );
		    continue;
		}
	
		if (ne_probe1( psDev, pci_ioaddr) != 0) {	/* Shouldn't happen. */
		    printk(KERN_ERR "ne.c: Probe of PCI card at %#lx failed.\n", pci_ioaddr);
		    pci_irq_line = 0;
		    return -ENXIO;
		}
		g_nHandle = sInfo.nHandle;
		if( claim_device( nDeviceID, sInfo.nHandle, "NE2000 PCI", DEVICE_NET ) != 0 ) {
        	printk( "Could not claim device!\n" );
        	continue;
        }
		return( 0 ); // FIXME: Might more than one card out there
	    }
	}
    }
    return -ENODEV;
}

int ne_probe( int nDeviceID, NE2Dev_s* psDev )
{
    int base_addr = 0; //dev ? dev->base_addr : 0;
    int i;
    
      /* First check any supplied i/o locations. User knows best. <cough> */
    if (base_addr > 0x1ff)	/* Check a single specified location. */
	return ne_probe1( psDev, base_addr);
    else if (base_addr != 0)	/* Don't probe at all. */
	return ENXIO;

    for ( i = 0 ; netcard_portlist[i] != 0 ; ++i ) {
	if ( ne_probe1( psDev, netcard_portlist[i] ) == 0 ) {
		g_nHandle = register_device( "", "isa" );
    	claim_device( nDeviceID, g_nHandle, "NE2000 ISA", DEVICE_NET );
	    return( 0 );
	}
    }
    
      /* Then look for any installed PCI clones */
    if ( ne_probe_pci( nDeviceID, psDev ) == 0 ) {
	return 0;
    }
    return ENODEV;
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int rx_thread( void* pData )
{
    NE2Dev_s* psDev = pData;

    for (;;)
    {
	NEPacketBuf_s* psCurBuf;
	PacketBuf_s* psNewBuf;
	int		 nFlags;
    
	LOCK( psDev->n2_hRxSemIrq );

	if ( psDev->n2_bStarted == false ) {
	    return(0);
	}
	
	if ( psDev->n2_sRxIrqQueue.psHead == NULL ) {
	    printk( KERN_ERR "rx_thread() sem released without any ready packets\n" );
	    continue;
	}

	spinlock_cli( &psDev->n2_nPageLock, nFlags );
	psCurBuf = psDev->n2_sRxIrqQueue.psHead;
	rem_buffer( &psDev->n2_sRxIrqQueue, psCurBuf );
	spinunlock_restore( &psDev->n2_nPageLock, nFlags );

	if ( psDev->n2_sRxQueue.nCount > 1000 )
	{
//      printk( "rx_thread() NEx000 rx buffer overflow\n" );
      
	    spinlock_cli( &psDev->n2_nPageLock, nFlags );
	    add_buffer( &psDev->n2_sRxFreeBuffers, psCurBuf );
	    spinunlock_restore( &psDev->n2_nPageLock, nFlags );
	    continue;
	}
    
	psNewBuf = alloc_pkt_buffer( 2048 );
    
	if ( psNewBuf == NULL ) {
	    printk( KERN_ERR "rx_thread() failed to alloc packet buffer\n" );
      
	    spinlock_cli( &psDev->n2_nPageLock, nFlags );
	    add_buffer( &psDev->n2_sRxFreeBuffers, psCurBuf );
	    spinunlock_restore( &psDev->n2_nPageLock, nFlags );
	    continue;
	}
    
	psNewBuf->pb_nSize = psCurBuf->nSize;
	psNewBuf->pb_uMacHdr.pRaw = psNewBuf->pb_pData;
    
	memcpy( psNewBuf->pb_uMacHdr.pRaw, psCurBuf->pBuffer, psCurBuf->nSize );
    
	spinlock_cli( &psDev->n2_nPageLock, nFlags );
	add_buffer( &psDev->n2_sRxFreeBuffers, psCurBuf );
	spinunlock_restore( &psDev->n2_nPageLock, nFlags );
	UNLOCK( psDev->n2_hRxSem );

	if ( psDev->n2_psPktQueue != NULL &&  psDev->n2_psPktQueue->nq_nCount < 200 ) {
	    enqueue_packet( psDev->n2_psPktQueue, psNewBuf );
	} else {
	    free_pkt_buffer( psNewBuf );
	}
    
    }
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int ne_send( void* pDev, const void* pBuffer, int nSize )
{
//    Thread_s* psMyThread = CURRENT_THREAD;
//    thread_id hMyThread  = psMyThread->tr_hThreadID;
    bigtime_t nStartTime = get_real_time();
    NE2Dev_s* psDev = pDev;
    uint32    nFlags;

    if ( psDev->n2_bStarted == false ) {
	return( -ENETDOWN );
    }
  
    nSize = min( nSize, MAX_PACKET_SIZE );
    nSize = max( 60, nSize );
  
    spinlock_cli( &psDev->n2_nPageLock, nFlags );
    
    while ( psDev->n2_bTxBusy && get_real_time() - nStartTime < 100000LL )
    {
//	WaitQueue_s sWaitNode;
//	WaitQueue_s sSleepNode;

//	sWaitNode.wq_hThread = hMyThread;
//	sSleepNode.wq_hThread = hMyThread;
//	sSleepNode.wq_nResumeTime = nStartTime + 100000LL;

//	psMyThread->tr_nState = TS_SLEEP;
    
//	add_to_sleeplist( &sSleepNode );
//	add_to_waitlist( &psDev->n2_psTxWaitQueue, &sWaitNode );
    
//	spinunlock_restore( &psDev->n2_nPageLock, nFlags );

	if ( spinunlock_and_suspend( psDev->n2_hTxSem, &psDev->n2_nPageLock, nFlags, 100000LL ) < 0 ) {
	    printk( "Error: ne_send() timed out\n" );
	    spinlock_cli( &psDev->n2_nPageLock, nFlags );
	    break;
	}
	
	spinlock_cli( &psDev->n2_nPageLock, nFlags );
//	Schedule();
//	spinlock_cli( &psDev->n2_nPageLock, nFlags );
    
//	remove_from_waitlist( &psDev->n2_psTxWaitQueue, &sWaitNode );
//	remove_from_sleeplist( &sSleepNode );
    }
    ei_start_xmit( psDev, pBuffer, nSize );
    spinunlock_restore( &psDev->n2_nPageLock, nFlags );
    return( nSize );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static void read_config()
{
    char anBuffer[1024];
    int nSize;
    int	nStart = 0;
    int nPathBase;
    int nFile;
    int	i;
    
    get_system_path( anBuffer, 256 );

    nPathBase = strlen( anBuffer );
	
    if ( '/' != anBuffer[ nPathBase - 1 ] ) {
      anBuffer[ nPathBase ] = '/';
      anBuffer[ nPathBase + 1 ] = '\0';
    }

    strcat( anBuffer, "sys/config/dev/ne2000.cfg" );
    
    nFile = open( anBuffer, O_RDONLY );
    if ( nFile < 0 ) {
	return;
    }
    
    nSize = read( nFile, anBuffer, 1024 );
    if ( nSize < 0 ) {
	printk( "NE2000: failed to read config file: %d\n", nSize );
	close( nFile );
	return;
    }
  
    for ( i = 0 ; i < nSize ; ++i ) {
	if ( '\n' == anBuffer[i] || '\r' == anBuffer[i] ) {
	    int nEnd = i;

	    while( isspace( anBuffer[ nStart ] ) && nStart < nEnd ) {
		nStart++;
	    }

	    if ( nStart < nEnd ) {
		if ( strncmp( "ISA_IRQ=", &anBuffer[nStart], 8 ) == 0 ) {
		    nStart += 8;	/* Skip command + "=" */
		    if ( nStart < nEnd ) {
			g_nISAIRQ = atol( &anBuffer[nStart] );
			printk( "NE2000: ISA_IRQ=%d\n", g_nISAIRQ );
		    }
		}
	    }
	    nStart = nEnd + 1;
	}
    }
    close( nFile );
}

status_t ne2_open( void* pNode, uint32 nFlags, void **pCookie )
{
    return( 0 );
}

status_t ne2_close( void* pNode, void* pCookie )
{
    return( 0 );
}

status_t ne2_ioctl( void* pNode, void* pCookie, uint32 nCommand, void* pArgs, bool bFromKernel )
{
    NE2Dev_s* psDev = pNode;
    int nError = 0;

    switch( nCommand )
    {
	case SIOC_ETH_START:
	{
	    read_config();

	    if ( pci_irq_line == 0 ) {
		psDev->n2_nIrq = g_nISAIRQ;
	    }
	    
	    psDev->n2_psPktQueue = pArgs;

	    psDev->n2_nIRQHandle = request_irq( psDev->n2_nIrq, ei_interrupt, NULL, pci_irq_line ? SA_SHIRQ : 0, "ne2000_device", psDev );
	    psDev->n2_bStarted = true;

	    NS8390_init( psDev, true );

	    psDev->n2_hRxThread = spawn_kernel_thread( "ne_rx", rx_thread, 200, 0, psDev );
	    wakeup_thread( psDev->n2_hRxThread, false );
	    break;
	}
	case SIOC_ETH_STOP:
	{
	    NS8390_init( psDev, false );
	    release_irq( psDev->n2_nIrq, psDev->n2_nIRQHandle );
	    psDev->n2_bStarted = false;
	    UNLOCK( psDev->n2_hRxSemIrq );
	    while ( sys_wait_for_thread( psDev->n2_hRxThread ) == -EINTR );
	    psDev->n2_hRxThread = -1;
	    psDev->n2_psPktQueue = NULL;
	    break;
	}
	case SIOCSIFHWADDR:
	    nError = -ENOSYS;
	    break;
	case SIOCGIFHWADDR:
	    memcpy( ((struct ifreq*)pArgs)->ifr_hwaddr.sa_data, psDev->n2_anEthrAddress, 6 );
	    break;
	default:
	    printk( "Error: ne2_ioctl() unknown command %ld\n", nCommand );
	    nError = -ENOSYS;
	    break;
    }
    return( nError );
}

int  ne2_read( void* pNode, void* pCookie, off_t nPosition, void* pBuffer, size_t nSize )
{
    return( -ENOSYS );
}

int  ne2_write( void* pNode, void* pCookie, off_t nPosition, const void* pBuffer, size_t nSize )
{
    return( ne_send( pNode, pBuffer, nSize ) );
}



DeviceOperations_s g_sDevOps = {
    ne2_open,
    ne2_close,
    ne2_ioctl,
    ne2_read,
    ne2_write,
    NULL,	// dop_readv
    NULL,	// dop_writev
    NULL,	// dop_add_select_req
    NULL	// dop_rem_select_req
};

status_t device_init( int nDeviceID )
{
    int nError;
    NE2Dev_s* psDev;
    int	i;
  
    g_nISAIRQ = 10; // nIRQ;

    read_config();
    
    psDev = kmalloc( sizeof( NE2Dev_s ), MEMF_KERNEL | MEMF_CLEAR );
  
    if ( psDev == NULL ) {
	printk( KERN_ERR "init_n2k() Failed to alloc device struct\n" );
	return( -ENOMEM );
    }
    psDev->n2_nDeviceID = nDeviceID;
    
    for ( i = 0 ; i < 100 ; ++i )
    {
	NEPacketBuf_s* psNewBuf = kmalloc( sizeof( NEPacketBuf_s ) + MAX_PACKET_SIZE, MEMF_KERNEL | MEMF_CLEAR );

	if ( psNewBuf == NULL ) {
	    printk( KERN_ERR "init_n2k() failed to alloc packet buffer\n" );
	    break;
	}
	psNewBuf->pBuffer = (uint8*)(psNewBuf + 1);
	add_buffer( &psDev->n2_sRxFreeBuffers, psNewBuf );
    }
    psDev->n2_pzName = "Nx000_drv";
  
    psDev->n2_hRxSemIrq = create_semaphore( "ne2_rx_irq", 0, 0 );
    psDev->n2_hRxSem    = create_semaphore( "ne2_rx", 0, 0 );
    psDev->n2_hTxSem    = create_semaphore( "ne2_tx", 0, 0 );

    if ( ne_probe( nDeviceID, psDev ) != 0 ) {
	return( -ENOENT );
    }

    psDev->ne_nInodeHandle = create_device_node( nDeviceID, g_nHandle, "net/eth/8390-0", &g_sDevOps, psDev );
    nError = 0;
    if ( psDev->ne_nInodeHandle < 0 ) {
	printk( "Failed to create device node %d\n", nError );
	nError = psDev->ne_nInodeHandle;
    }
  
    return( nError );
}

status_t device_uninit( int nDeviceID )
{
    return( 0 );
}
