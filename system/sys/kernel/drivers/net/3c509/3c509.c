/*  3c509.c : driver for the 3COM EtherLink III series ethernet adapter cards.
 *   
 *  Copyright (C) 2000 Steven Huang
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *
 *  Acknowledgements
 *  ----------------
 *  This driver makes extensive use of the EtherLink III driver code for Linux
 *  written by Donald Becker, as well as the NE2000 driver code for Atheos, 
 *  written by Kurt Skauen.
 *
 *  Version History
 *  ---------------
 *  v0.50 October 9, 2000    First alpha release.
 *  v0.51 October 10, 2000   Small fix to check for packet allocation error
 *                           add note: only use with kernel version > 0.20.
 *  v0.60 October 10, 2000   Removed ping pong and rx_thread implementations
 *                           made the code a little "prettier"
 *
 */
    
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


#define EL3_DEBUG   0

#define KERN_ERR "Error: "
#define KERN_DEBUG "Debug: "
#define KERN_WARNING "Warning: "
#define KERN_INFO "Info: "

static char *version = "3c509.c:0.51 October 9, 2000\n";

/*  This should probably be in some common header file
    somewhere. For now, I just copied it from 8390.h.   */
struct net_device_stats
{
    int tx_packets;
    int tx_bytes;
    int tx_errors;
    int tx_aborted_errors;
    int tx_carrier_errors;
    int tx_fifo_errors;
    int tx_heartbeat_errors;
    int tx_window_errors;
    int rx_errors;
    int rx_length_errors;
    int rx_fifo_errors;
    int rx_over_errors;
    int rx_frame_errors;
    int rx_crc_errors;
    int rx_missed_errors;
    int collisions;
};


typedef struct
{
    int el3_nDeviceID;
    int el3_nIoBase;
    bigtime_t   el3_nTransStart;
    uint32 el3_nFlags;
    char*   el3_pzName;
    uint8   el3_anEthrAddress[6];
    bool el3_bStarted;
    bool el3_bTxBusy;
    bool el3_bIrqActive;
    sem_id el3_hTxSem;
//    WaitQueue_s* el3_psTxWaitQueue;
    int el3_nIrq;
    int el3_nIRQHandle;
    int el3_nInodeHandle;
    NetQueue_s* el3_psPktQueue;
    SpinLock_s el3_nPageLock;
    uint8 el3_anMCFilter[8];
    unsigned el3_nWord16:1;             /* We have the 16-bit (vs 8-bit) version of the card. */
    unsigned el3_nTXing:1;              /* Transmit Active */
    unsigned el3_nIRQLock:1;            /* 8390's intrs disabled when '1'. */
    unsigned el3_nDMAing:1;             /* Remote DMA Active */

    uint8 el3_nInterfaceNum;            /* Net port (AUI, 10bT.) to use. */
    struct net_device_stats el3_sStat;  /* The new statistics table. */
    
} el3Dev_s;


static int id_port = 0x110;     /*  id_port for initialization, 
                                    start with 0x110 to avoid new sound cards.*/

/* 3c509 register offsets */
#define EL3_DATA    0x00
#define EL3_CMD     0x0e
#define EL3_STATUS  0x0e
#define EEPROM_READ 0x80

/* 3c509 has a 16 byte register window */
#define EL3_IO_EXTENT   16

/* macro to switch register windows */
#define EL3WINDOW(win_num) outw(SelectWindow + (win_num), ioaddr + EL3_CMD)

/* maximum number of interrupts to service per request */
#define MAX_SERVICE 12

#define TX_TIMEOUT  400000

/* The top five bits written to EL3_CMD are a command, the lower
   11 bits are the parameter, if applicable. */
enum c509cmd {
    TotalReset = 0<<11, SelectWindow = 1<<11, StartCoax = 2<<11,
    RxDisable = 3<<11, RxEnable = 4<<11, RxReset = 5<<11, RxDiscard = 8<<11,
    TxEnable = 9<<11, TxDisable = 10<<11, TxReset = 11<<11,
    FakeIntr = 12<<11, AckIntr = 13<<11, SetIntrEnb = 14<<11,
    SetStatusEnb = 15<<11, SetRxFilter = 16<<11, SetRxThreshold = 17<<11,
    SetTxThreshold = 18<<11, SetTxStart = 19<<11, StatsEnable = 21<<11,
    StatsDisable = 22<<11, StopCoax = 23<<11,};

enum c509status {
    IntLatch = 0x0001, AdapterFailure = 0x0002, TxComplete = 0x0004,
    TxAvailable = 0x0008, RxComplete = 0x0010, RxEarly = 0x0020,
    IntReq = 0x0040, StatsFull = 0x0080, CmdBusy = 0x1000, };

/* The SetRxFilter command accepts the following classes: */
enum RxFilter {
    RxStation = 1, RxMulticast = 2, RxBroadcast = 4, RxProm = 8 };

/* Register window 1 offsets, the window used in normal operation. */
#define TX_FIFO     0x00
#define RX_FIFO     0x00
#define RX_STATUS   0x08
#define TX_STATUS   0x0B
#define TX_FREE     0x0C        /* Remaining free bytes in Tx buffer. */

#define WN0_IRQ     0x08        /* Window 0: Set IRQ line in bits 12-15. */
#define WN4_MEDIA   0x0A        /* Window 4: Various transcvr/media bits. */
#define MEDIA_TP    0x00C0      /* Enable link beat and jabber for 10baseT. */

#define MAX_PACKET_SIZE 1518


/* function definitions */
static int el3_init(el3Dev_s* dev);
static unsigned short id_read_eeprom(int index);
//static unsigned short read_eeprom(int ioaddr, int index);
static void update_stats(el3Dev_s* dev);


static int el3_receive(el3Dev_s* psDev)
{
    struct net_device_stats* stats = &psDev->el3_sStat;
    int ioaddr = psDev->el3_nIoBase;
    short rx_status;
    
#if EL3_DEBUG
    printk( "el3_receive() called\n" );
#endif
    while ((rx_status = inw(ioaddr + RX_STATUS)) > 0)
    {
        if (rx_status & 0x4000)
        {
            /* Error, update stats. */ 
            short error = rx_status & 0x3800;

            outw(RxDiscard, ioaddr + EL3_CMD);
            stats->rx_errors++;
            switch (error) {
            case 0x0000: stats->rx_over_errors++; break;
            case 0x0800: stats->rx_length_errors++; break;
            case 0x1000: stats->rx_frame_errors++; break;
            case 0x1800: stats->rx_length_errors++; break;
            case 0x2000: stats->rx_frame_errors++; break;
            case 0x2800: stats->rx_crc_errors++; break;
            }
        } else {
            short pkt_len = rx_status & 0x7ff;

            if (pkt_len < 60  ||  pkt_len > 1518) 
            {
                printk( KERN_DEBUG "%s: bogus packet size: %d.\n", psDev->el3_pzName, pkt_len );

                psDev->el3_sStat.rx_errors++;
                psDev->el3_sStat.rx_length_errors++;
            }

            {
                /* send the packet off */
                PacketBuf_s* psBuf = alloc_pkt_buffer( 2048 );
                if ( psBuf == NULL ) {
                    printk( KERN_WARNING " el3_receive() could not allocate packet buffer, discarding packet\n" );
                    stats->rx_missed_errors++;
                    outw(RxDiscard, ioaddr + EL3_CMD);
                    while (inw(ioaddr + EL3_STATUS) & 0x1000)
                        printk(KERN_DEBUG " Waiting for 3c509 to discard packet, status %x.\n", inw(ioaddr + EL3_STATUS) );
                    break;
                }
                psBuf->pb_nSize = pkt_len;
                psBuf->pb_uMacHdr.pRaw = psBuf->pb_pData;
                
                insl( ioaddr + RX_FIFO, psBuf->pb_uMacHdr.pRaw, (pkt_len + 3) >> 2);
       
                if ( psDev->el3_psPktQueue != NULL && psDev->el3_psPktQueue->nq_nCount < 200 ) {
                    enqueue_packet( psDev->el3_psPktQueue, psBuf );
                } else {
                    free_pkt_buffer( psBuf );
                }                
            }
        }
        outw(RxDiscard, ioaddr + EL3_CMD);
//      inw(ioaddr + EL3_STATUS);               /* Delay. */
        while (inw(ioaddr + EL3_STATUS) & 0x1000)
            printk(KERN_DEBUG " Waiting for 3c509 to discard packet, status %x.\n", inw(ioaddr + EL3_STATUS) );
    }
    return 0;
}


static bool el3_tx_intr( el3Dev_s* psDev )
{
    psDev->el3_nTXing = 0;
    psDev->el3_bTxBusy = 0;

    /* Minimize Tx latency: update the statistics after we restart TXing. */
/*
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
*/
    return( wakeup_sem( psDev->el3_hTxSem, false ) > 0 );

//  mark_bh (NET_BH);
}


static int el3_interrupt( int nIrqNum, void* pData, SysCallRegs_s* psRegs )
{
    el3Dev_s* psDev = pData;
    int ioaddr;
    int status;
    int nr_serviced = 0;
  
    ioaddr = psDev->el3_nIoBase;

    spinlock(&psDev->el3_nPageLock);

    /* This might just be an interrupt for a PCI device sharing this line */
    /* The "irqlock" check is only for testing. */
    if (psDev->el3_bIrqActive || psDev->el3_nIRQLock) 
    {
        printk(psDev->el3_nIRQLock
            ? "%s: Interrupted while interrupts are masked! isr=%#2x.\n"
            : "%s: Reentering the interrupt handler! isr=%#2x.\n",
            psDev->el3_pzName, inb_p(ioaddr + EL3_STATUS));

        spinunlock(&psDev->el3_nPageLock);
        return(0);
    }
    psDev->el3_bIrqActive = 1;
    if (psDev->el3_bStarted == 0) 
    {
        printk(KERN_WARNING "%s: interrupt from stopped card\n", psDev->el3_pzName );
        status = 0;
    }

    while ( ( status = inw( ioaddr + EL3_STATUS ) & 0xFF ) && ++nr_serviced < MAX_SERVICE )
    {

#if EL3_DEBUG
        printk( "el3_interrupt() processing interrupt %#4.4x\n", status );
#endif
        /* process a RxComplete interrupt (full packet has been received) */
        if (status & RxComplete)
        {
#if EL3_DEBUG           
            printk ( " el3_interrupt() processing RxComplete interrupt\n" );
#endif            
            el3_receive( psDev );
        }
        /*  process a transmit available interrupt -- 
            adapter has room for another packet to be sent */
        if (status & TxAvailable) {
#if EL3_DEBUG
            printk ( " el3_interrupt() processing TxAvailable interrupt\n" );           
#endif
            el3_tx_intr( psDev );
            outw(AckIntr | TxAvailable, ioaddr + EL3_CMD);          

        }
        
        /*  process all other (uncommon) interrupts */
        if ( status & ( RxEarly | TxComplete | StatsFull | AdapterFailure ) ) 
        {
            
            if (status & RxEarly) {     /* Rx early is unused. */
#if EL3_DEBUG
                printk ( KERN_WARNING " el3_interrupt() processing RxEarly interrupt (huh?)\n" );
#endif          
                el3_receive( psDev );
                outw(AckIntr | RxEarly, ioaddr + EL3_CMD);  /* ack RxEarly interrupt */
            }       

            if ( status & TxComplete )  /* this is an error condition */
            {
                struct net_device_stats stat = psDev->el3_sStat;
                short tx_status;
                int i = 4;
#if EL3_DEBUG    
                printk ( KERN_WARNING "el3_interrupt() processing TxComplete interrupt\n" );
#endif                
                while (--i>0 && (tx_status = inb(ioaddr + TX_STATUS)) > 0) {
                    if (tx_status & 0x38) stat.tx_aborted_errors++;
                    if (tx_status & 0x30) outw(TxReset, ioaddr + EL3_CMD);
                    if (tx_status & 0x3C) outw(TxEnable, ioaddr + EL3_CMD);
                    outb(0x00, ioaddr + TX_STATUS); /* Pop the status stack. */
                }
                /*
                ei_tx_err( psDev );
                */
            }

            if (status & StatsFull) 
            {
                update_stats( psDev );
                /*  ack StatsFull interrupt */
                /*  the linux driver doesn't do this, as far as I can tell,
                    but if not acked, update_stats keeps getting called
                    I must be missing something...                          */
                outw( AckIntr | StatsFull, ioaddr + EL3_CMD );  
            }

            if (status & AdapterFailure)
            {
                /* Adapter failure requires Rx reset and reinit. */
#if EL3_DEBUG
                printk( "el3_interrupt() processing adapter failure interrupt\n" );
#endif
                outw(RxReset, ioaddr + EL3_CMD);
            /* Set the Rx filter to the current state. */
/*
                outw(SetRxFilter | RxStation | RxBroadcast
                     | (psDev->el3_nFlags & IFF_ALLMULTI ? RxMulticast : 0)
                     | (psDev->el3_nFlags & IFF_PROMISC ? RxProm : 0),
                     ioaddr + EL3_CMD);
*/
                outw(RxEnable, ioaddr + EL3_CMD); /* Re-enable the receiver. */
                outw(AckIntr | AdapterFailure, ioaddr + EL3_CMD);
            }
        }

        outw(AckIntr | IntReq | IntLatch, ioaddr + EL3_CMD); /* Ack IRQ */
    }

    if ( status )
    {
        if (nr_serviced >= MAX_SERVICE) 
        {
            printk(KERN_WARNING "%s: Too much work at interrupt, status %#4.4x\n",
                psDev->el3_pzName, status);
            outw(AckIntr | 0xFF, ioaddr + EL3_CMD);     /* Ack all interrupts */
        } else {
            printk(KERN_WARNING "%s: unknown interrupt %#4.4x\n", psDev->el3_pzName, status);
            outw(AckIntr | 0xFF, ioaddr + EL3_CMD);     /* Ack all interrupts */
        }
    }

    psDev->el3_bIrqActive = 0;
    spinunlock(&psDev->el3_nPageLock);
    return(0);
}


/*  Update statistics.  We change to register window 6, so this should be run
    single-threaded if the device is active. This is expected to be a rare
    operation, and it's simpler for the rest of the driver to assume that
    window 1 is always valid rather than use a special window-state variable.   */
static void update_stats(el3Dev_s* dev)
{
    int ioaddr = dev->el3_nIoBase;
    struct net_device_stats stat = dev->el3_sStat;

#if EL3_DEBUG
    printk("   Updating 3c509 statistics.\n");
#endif
    /* Turn off statistics updates while reading. */
    outw(StatsDisable, ioaddr + EL3_CMD);
    /*  Switch to the stats window, and read everything 
        (registers are cleared when read)               */
    EL3WINDOW(6);
    stat.tx_carrier_errors      += inb(ioaddr + 0);
    stat.tx_heartbeat_errors    += inb(ioaddr + 1);
                                   inb(ioaddr + 2); /* Multiple collisions */
    stat.collisions             += inb(ioaddr + 3);
    stat.tx_window_errors       += inb(ioaddr + 4);
    stat.rx_fifo_errors         += inb(ioaddr + 5);
    stat.tx_packets             += inb(ioaddr + 6);
                                   inb(ioaddr + 7); /* Rx packets */
                                   inb(ioaddr + 8); /* Tx deferrals */
    
                                   inw(ioaddr + 10);/* Total Rx and Tx octets. */
                                   inw(ioaddr + 12);

    /* Back to window 1, and turn statistics back on. */
    EL3WINDOW(1);
    outw(StatsEnable, ioaddr + EL3_CMD);
    return;
}


int el3_probe(el3Dev_s* dev)
{
    short lrs_state = 0xff, i;
    int ioaddr, irq, if_port;
    uint16 phys_addr[3];
    static int current_tag = 0;

#if 0
    int mca_slot = -1;

    /* First check all slots of the EISA bus.  The next slot address to
       probe is kept in 'eisa_addr' to support multiple probe() calls. */
    if (EISA_bus) {
        static int eisa_addr = 0x1000;
        while (eisa_addr < 0x9000) {
            ioaddr = eisa_addr;
            eisa_addr += 0x1000;

            /* Check the standard EISA ID register for an encoded '3Com'. */
            if (inw(ioaddr + 0xC80) != 0x6d50)
                continue;

            /* Change the register set to the configuration window 0. */
            outw(SelectWindow | 0, ioaddr + 0xC80 + EL3_CMD);

            irq = inw(ioaddr + WN0_IRQ) >> 12;
            if_port = inw(ioaddr + 6)>>14;
            for (i = 0; i < 3; i++)
                phys_addr[i] = htons(read_eeprom(ioaddr, i));

            /* Restore the "Product ID" to the EEPROM read register. */
            read_eeprom(ioaddr, 3);

            /* Was the EISA code an add-on hack?  Nahhhhh... */
            goto found;
        }
    }

#ifdef CONFIG_MCA
    /* Based on Erik Nygren's (nygren@mit.edu) 3c529 patch, heavily
     * modified by Chris Beauregard (cpbeaure@csclub.uwaterloo.ca)
     * to support standard MCA probing.
     *
     * redone for multi-card detection by ZP Gu (zpg@castle.net)
     * now works as a module
     */

    if( MCA_bus ) {
        int slot, j;
        u_char pos4, pos5;

        for( j = 0; el3_mca_adapters[j].name != NULL; j ++ ) {
            slot = 0;
            while( slot != MCA_NOTFOUND ) {
                slot = mca_find_unused_adapter(
                    el3_mca_adapters[j].id, slot );
                if( slot == MCA_NOTFOUND ) break;

                /* if we get this far, an adapter has been
                 * detected and is enabled
                 */

                pos4 = mca_read_stored_pos( slot, 4 );
                pos5 = mca_read_stored_pos( slot, 5 );

                ioaddr = ((short)((pos4&0xfc)|0x02)) << 8;
                irq = pos5 & 0x0f;

                /* probing for a card at a particular IO/IRQ */
                if(dev && ((dev->el3_nIrq >= 1 && dev->el3_nIrq != irq) ||
                (dev->el3_nIoBase >= 1 && dev->el3_nIoBase != ioaddr))) {
                    slot++;         /* probing next slot */
                    continue;
                }

                printk("3c509: found %s at slot %d\n",
                    el3_mca_adapters[j].name, slot + 1 );

                /* claim the slot */
                mca_set_adapter_name(slot, el3_mca_adapters[j].name);
                mca_set_adapter_procfn(slot, NULL, NULL);
                mca_mark_as_used(slot);

                if_port = pos4 & 0x03;
                if (el3_debug > 2) {
                    printk("3c529: irq %d  ioaddr 0x%x  ifport %d\n", irq, ioaddr, if_port);
                }
                for (i = 0; i < 3; i++) {
                    phys_addr[i] = htons(read_eeprom(ioaddr, i));
                }
                
                mca_slot = slot;

                goto found;
            }
        }
        /* if we get here, we didn't find an MCA adapter */
        return -ENODEV;
    }
#endif
#endif

    /* Reset the ISA PnP mechanism on 3c509b. */
#if EL3_DEBUG   
    printk( "Resnet ISA PnP mechanism\n" );
#endif  
    outb(0x02, 0x279);           /* Select PnP config control register. */
    outb(0x02, 0xA79);           /* Return to WaitForKey state. */

    /* Select an open I/O location at 0x1*0 to do contention select. */
#if EL3_DEBUG
    printk( "Select an open I/O location at 0x1*0 to do contention select\n" );
#endif
    for ( ; id_port < 0x200; id_port += 0x10)
    {
/*
        if (check_region(id_port, 1))
            continue;
*/
        outb(0x00, id_port);
        outb(0xff, id_port);
        if (inb(id_port) & 0x01)
            break;
    }
    
#if EL3_DEBUG
    printk( "Got it, id_port = %#3.3lx\n", id_port );
#endif

    if (id_port >= 0x200)
    {

        /* Rare -- do we really need a warning? */
        printk( KERN_WARNING "No I/O port available for 3c509 activation.\n");
        return -ENODEV;
    }

    /* Next check for all ISA bus boards by sending the ID sequence to the
       ID_PORT.  We find cards past the first by setting the 'current_tag'
       on cards as they are found.  Cards with their tag set will not
       respond to subsequent ID sequences. */

#if EL3_DEBUG
    printk( "Sending ID Sequence\n" );
#endif  
    outb(0x00, id_port);
    outb(0x00, id_port);
    for(i = 0; i < 255; i++)
    {
        outb(lrs_state, id_port);
        lrs_state <<= 1;
        lrs_state = lrs_state & 0x100 ? lrs_state ^ 0xcf : lrs_state;
    }

#if EL3_DEBUG
    printk( "Clearing tag registers\n" );
#endif    
    /* For the first probe, clear all board's tag registers. */
    if (current_tag == 0)
        outb(0xd0, id_port);
    else                /* Otherwise kill off already-found boards. */
        outb(0xd8, id_port);

#if EL3_DEBUG
    printk( "Reading manufacturer's ID\n" );
#endif    
    if (id_read_eeprom(7) != 0x6d50)
    {
        return -ENODEV;
    }

    /* Read in EEPROM data, which does contention-select.
       Only the lowest address board will stay "on-line".
       3Com got the byte order backwards. */
    
#if EL3_DEBUG
    printk( "Getting hardware address\n" );
#endif  
    for (i = 0; i < 3; i++)
    {
        phys_addr[i] = htons(id_read_eeprom(i));
    }

    {
        unsigned int iobase = id_read_eeprom(8);
        if_port = iobase >> 14;
        ioaddr = 0x200 + ((iobase & 0x1f) << 4);
    }
    irq = id_read_eeprom(9) >> 12;

#if 0   // user specified parameters not supported yet
    if (dev)                    /* Set passed-in IRQ or I/O Addr. */
    {
        if (dev->el3_nIrq > 1  &&  dev->el3_nIrq < 16)
            irq = dev->el3_nIrq;

        if (dev->el3_nIOBase)
        {
            if (dev->mem_end == 0x3c509             /* Magic key */
                && dev->el3_nIoBase >= 0x200  &&  dev->el3_nIoBase <= 0x3e0)
                ioaddr = dev->el3_nIoBase & 0x3f0;
            else if (dev->el3_nIoBase != ioaddr)
                return -ENODEV;
        }
    }
#endif

    /* Set the adaptor tag so that the next card can be found. */
    outb(0xd0 + ++current_tag, id_port);

    /* Activate the adaptor at the EEPROM location. */
    outb((ioaddr >> 4) | 0xe0, id_port);

    EL3WINDOW(0);
    if (inw(ioaddr) != 0x6d50)
        return -ENODEV;

    /* Free the interrupt so that some other card can use it. */
    outw(0x0f00, ioaddr + WN0_IRQ);
    memcpy(dev->el3_anEthrAddress, phys_addr, sizeof(phys_addr));
    dev->el3_nIoBase = ioaddr;
    dev->el3_nIrq = irq;
//  dev->el3_nInterfaceNum = (dev->mem_start & 0x1f) ? dev->mem_start & 3 : if_port;
    dev->el3_nInterfaceNum = if_port;

    {
        const char *if_names[] = {"10baseT", "AUI", "undefined", "BNC"};    
        printk("%s: 3c509 at %#3.3x, IRQ %d, tag %d, %s port, address %2.2x:%2.2x:%2.2x:%2.2x:%2.2x:%2.2x\n", 
            dev->el3_pzName, 
            dev->el3_nIoBase,
            dev->el3_nIrq,
            current_tag, 
            if_names[dev->el3_nInterfaceNum],
            dev->el3_anEthrAddress[0],
            dev->el3_anEthrAddress[1],
            dev->el3_anEthrAddress[2],
            dev->el3_anEthrAddress[3],
            dev->el3_anEthrAddress[4],
            dev->el3_anEthrAddress[5]
        );
    }

    return 0;
}


/* Read a word from the EEPROM using the regular EEPROM access register.
   Assume that we are in register window zero.
 */
#if 0
static unsigned short read_eeprom(int ioaddr, int index)
{
    outw(EEPROM_READ + index, ioaddr + 10);
    /* Pause for at least 162 us. for the read to take place. */
    udelay (500);
    return inw(ioaddr + 12);
}
#endif

/* Read a word from the EEPROM when in the ISA ID probe state. */
static unsigned short id_read_eeprom(int index)
{
    int bit, word = 0;

    /* Issue read command, and pause for at least 162 us. for it to complete.
       Assume extra-fast 16Mhz bus. */
    outb(EEPROM_READ + index, id_port);
    /* Pause for at least 162 us. for the read to take place. */
    udelay (500);
    
    for (bit = 15; bit >= 0; bit--)
        word = (word << 1) + (inb(id_port) & 0x01);

/*
    if (el3_debug > 3)
        printk("  3c509 EEPROM word %d %#4.4x.\n", index, word);
*/
    return word;
}


static int el3_init(el3Dev_s* dev)
{
    int ioaddr = dev->el3_nIoBase;
    int i;

    outw(TxReset, ioaddr + EL3_CMD);
    outw(RxReset, ioaddr + EL3_CMD);
    outw(SetStatusEnb | 0x00, ioaddr + EL3_CMD);

    EL3WINDOW(0);

    /* Activate board: this is probably unnecessary. */
    outw(0x0001, ioaddr + 4);

    /* Set the IRQ line. */
    outw((dev->el3_nIrq << 12) | 0x0f00, ioaddr + WN0_IRQ);

    /* Set the station address in window 2 each time opened. */
    EL3WINDOW(2);

    for (i = 0; i < 6; i++)
        outb(dev->el3_anEthrAddress[i], ioaddr + i);

    if (dev->el3_nInterfaceNum == 3)
        /* Start the thinnet transceiver. We should really wait 50ms...*/
        outw(StartCoax, ioaddr + EL3_CMD);
    else if (dev->el3_nInterfaceNum == 0) {
        /* 10baseT interface, enabled link beat and jabber check. */
        EL3WINDOW(4);
        outw(inw(ioaddr + WN4_MEDIA) | MEDIA_TP, ioaddr + WN4_MEDIA);
    }

    /* Switch to the stats window, and clear all stats by reading. */
    outw(StatsDisable, ioaddr + EL3_CMD);
    EL3WINDOW(6);
    for (i = 0; i < 9; i++)
        inb(ioaddr + i);
    inw(ioaddr + 10);
    inw(ioaddr + 12);

    /* Switch to register set 1 for normal use. */
    EL3WINDOW(1);

    /* Accept b-case and phys addr only. */
    outw(SetRxFilter | RxStation | RxBroadcast, ioaddr + EL3_CMD);
    outw(StatsEnable, ioaddr + EL3_CMD); /* Turn on statistics. */

    dev->el3_bIrqActive = 0;
    dev->el3_bTxBusy = 0;
    dev->el3_bStarted = 1;

    outw(RxEnable, ioaddr + EL3_CMD); /* Enable the receiver. */
    outw(TxEnable, ioaddr + EL3_CMD); /* Enable transmitter. */
    /* Allow status bits to be seen. */
    outw(SetStatusEnb | 0xff, ioaddr + EL3_CMD);
    /* Ack all pending events, and set active indiator mask. */
    outw(AckIntr | IntLatch | TxAvailable | RxEarly | IntReq, ioaddr + EL3_CMD);
    outw(SetIntrEnb | IntLatch|TxAvailable|TxComplete|RxComplete|StatsFull, ioaddr + EL3_CMD);
    dev->el3_nIRQHandle = request_irq( dev->el3_nIrq, el3_interrupt, NULL, 0, "3c509_device", dev );
    return 0;                   /* Always succeed */
}


static int el3_start_xmit(el3Dev_s* dev, const void* pBuffer, int length)
{
    struct net_device_stats* stats = &dev->el3_sStat;
    int ioaddr = dev->el3_nIoBase;
    unsigned long flags;

#if EL3_DEBUG
    printk ( "el3_start_xmit() called\n" );
#endif
    /* Transmitter timeout, serious problems. */
    if (dev->el3_bTxBusy) {
        bigtime_t tickssofar = get_real_time() - dev->el3_nTransStart;
        if (tickssofar < TX_TIMEOUT)
            return 1;
            
        /* Reset the adapter */
        printk("%s: transmit timed out, Tx_status %2.2x status %4.4x "
               "Tx FIFO room %d.\n",
               dev->el3_pzName, inb(ioaddr + TX_STATUS), inw(ioaddr + EL3_STATUS),
               inw(ioaddr + TX_FREE));
        stats->tx_errors++;
        dev->el3_nTransStart = get_real_time();
        /* Issue TX_RESET and TX_START commands. */
        outw(TxReset, ioaddr + EL3_CMD);
        outw(TxEnable, ioaddr + EL3_CMD);
        dev->el3_bTxBusy = 0;
    }

    stats->tx_bytes += length;
    
    /* Avoid timer-based retransmission conflicts. */
    if (dev->el3_bTxBusy)
//  if (test_and_set_bit(0, (void*)&dev->el3_bTxBusy) != 0)
        printk("%s: Transmitter access conflict.\n", dev->el3_pzName);
    else {
        /*
         *  We lock the driver against other processors. Note
         *  we don't need to lock versus the IRQ as we suspended
         *  that. This means that we lose the ability to take
         *  an RX during a TX upload. That sucks a bit with SMP
         *  on an original 3c509 (2K buffer)
         *
         *  Using disable_irq stops us crapping on other
         *  time sensitive devices.
         */
        dev->el3_bTxBusy = 1;
        disable_irq_nosync(dev->el3_nIrq);
        spinlock_cli(&dev->el3_nPageLock, flags);
        
        /* Put out the doubleword header... */
        outw(length, ioaddr + TX_FIFO);
        outw(0x00, ioaddr + TX_FIFO);
        /* ... and the packet rounded to a doubleword. */
        outsl(ioaddr + TX_FIFO, pBuffer, (length + 3) >> 2);


        dev->el3_nTransStart = get_real_time();
        if (inw(ioaddr + TX_FREE) > 1536) {
            dev->el3_bTxBusy = 0;
        } else
            /* Interrupt us when the FIFO has room for max-sized packet. */
            outw(SetTxThreshold + 1536, ioaddr + EL3_CMD);

        spinunlock_restore(&dev->el3_nPageLock, flags);
        enable_irq(dev->el3_nIrq);
    }

    /* Clear the Tx status stack. */
    {
        short tx_status;
        int i = 4;

        while (--i > 0  &&  (tx_status = inb(ioaddr + TX_STATUS)) > 0) {
            if (tx_status & 0x38) stats->tx_aborted_errors++;
            if (tx_status & 0x30) outw(TxReset, ioaddr + EL3_CMD);
            if (tx_status & 0x3C) outw(TxEnable, ioaddr + EL3_CMD);
            outb(0x00, ioaddr + TX_STATUS); /* Pop the status stack. */
        }
    }
    return 0;
}


static int el3_send( void* pDev, const void* pBuffer, int nSize )
{
//    Thread_s* psMyThread = CURRENT_THREAD;
//    thread_id hMyThread  = psMyThread->tr_hThreadID;
    bigtime_t nStartTime = get_real_time();
    el3Dev_s* psDev = pDev;
    uint32    nFlags;

#if EL3_DEBUG
    printk ( "el3_send() called\n" );
#endif

    if ( psDev->el3_bStarted == false ) {
        return( -ENETDOWN );
    }
  
    nSize = min( nSize, MAX_PACKET_SIZE );
    nSize = max( 60, nSize );
  
    spinlock_cli( &psDev->el3_nPageLock, nFlags );
    
    /* wait until 3c509 is available */
    while ( psDev->el3_bTxBusy && get_real_time() - nStartTime < 100000LL )
    {
//        WaitQueue_s sWaitNode;
//        WaitQueue_s sSleepNode;
    
//        sWaitNode.wq_hThread = hMyThread;
//        sSleepNode.wq_hThread = hMyThread;
//        sSleepNode.wq_nResumeTime = nStartTime + 100000LL;
    
//        psMyThread->tr_nState = TS_SLEEP;
        
//        add_to_sleeplist( &sSleepNode );
//        add_to_waitlist( &psDev->el3_psTxWaitQueue, &sWaitNode );
        
//        spinunlock_restore( &psDev->el3_nPageLock, nFlags );
        
//        Schedule();
	if( spinunlock_and_suspend( psDev->el3_hTxSem, &psDev->el3_nPageLock, nFlags, 100000LL ) < 0 )
	{
	    printk( "Error: el3_send() timed out\n" );
	    spinlock_cli( &psDev->el3_nPageLock, nFlags );
	    break;
	}        
        spinlock_cli( &psDev->el3_nPageLock, nFlags );
        
//        remove_from_waitlist( &psDev->el3_psTxWaitQueue, &sWaitNode );
//        remove_from_sleeplist( &sSleepNode );
    }
    el3_start_xmit( psDev, pBuffer, nSize );
    spinunlock_restore( &psDev->el3_nPageLock, nFlags );
    return( nSize );
}


status_t el3_open( void* pNode, uint32 nFlags, void **pCookie )
{
    return( 0 );
}



status_t el3_close( void* pNode, void* pCookie )
{
    return( 0 );
}


int  el3_read( void* pNode, void* pCookie, off_t nPosition, void* pBuffer, size_t nSize )
{
    return( -ENOSYS );
}


int  el3_write( void* pNode, void* pCookie, off_t nPosition, const void* pBuffer, size_t nSize )
{
    return( el3_send( pNode, pBuffer, nSize ) );
}


status_t el3_ioctl( void* pNode, void* pCookie, uint32 nCommand, void* pArgs, bool bFromKernel )
{
    el3Dev_s* psDev = pNode;
    int nError = 0;

    switch( nCommand )
    {
    case SIOC_ETH_START:
    {
#if EL3_DEBUG
        printk( "el3_ioctl() SIOC_ETH_START command received\n" );
#endif      
/*
        if ( pci_irq_line == 0 ) {
        psDev->n2_nIrq = g_nISAIRQ;
        }
*/      
        psDev->el3_psPktQueue = pArgs;
        el3_init( psDev );
        break;
    }
    case SIOC_ETH_STOP:
    {
#if EL3_DEBUG
        printk( "el3_ioctl() SIOC_ETH_STOP command received\n" );
#endif        
        release_irq( psDev->el3_nIrq, psDev->el3_nIRQHandle );
        psDev->el3_bStarted = false;
        psDev->el3_psPktQueue = NULL;
        break;
    }
    case SIOCSIFHWADDR:
        printk( "el3_ioctl() SIOCSIFHWADDR command received\n" );   
        nError = -ENOSYS;
        break;
    case SIOCGIFHWADDR:
        printk( "el3_ioctl() SIOCGIFHWADDR command received\n" );
        memcpy( ((struct ifreq*)pArgs)->ifr_hwaddr.sa_data, psDev->el3_anEthrAddress, 6 );
        break;
    default:
        printk( "Error: el3_ioctl() unknown command %ld\n", nCommand );
        nError = -ENOSYS;
        break;
    }
    return( nError );
}


DeviceOperations_s g_sDevOps = {

    el3_open,
    el3_close,
    el3_ioctl,
    el3_read,
    el3_write,
    NULL,   // dop_readv
    NULL,   // dop_writev
    NULL,   // dop_add_select_req
    NULL    // dop_rem_select_req
};


status_t device_init( int nDeviceID )
{
    int nError;
    int nHandle;
    el3Dev_s* psDev;
  
    printk( version );

//    read_config();
    
    psDev = kmalloc( sizeof( el3Dev_s ), MEMF_KERNEL | MEMF_CLEAR );
  
    if ( psDev == NULL ) {
        printk( KERN_ERR "device_init() Failed to alloc device struct\n" );
        return( -ENOMEM );
    }
    
    psDev->el3_nDeviceID = nDeviceID;
    psDev->el3_pzName = "3c509_drv";    


#if EL3_DEBUG
    printk( "Calling el3_probe()\n" );
#endif    
    if ( el3_probe( psDev ) != 0 ) {
          printk( "***NO 3c509 CARD FOUND***\n" );
          return( -ENOENT );
    }
    
    nHandle = register_device( "", "isa" );
    claim_device( nDeviceID, nHandle, "3Com EtherLink III", DEVICE_NET );

#if EL3_DEBUG    
    printk( "Creating device node: net/eth/3c509\n" );
#endif    
    psDev->el3_nInodeHandle = create_device_node( nDeviceID, nHandle, "net/eth/3c509", &g_sDevOps, psDev );
    nError = 0;
    if ( psDev->el3_nInodeHandle < 0 ) {
          printk( KERN_ERR "Failed to create device node %d\n", nError );
          nError = psDev->el3_nInodeHandle;
    }
  
    return( nError );
}


status_t device_uninit( int nDeviceID )
{
    return( 0 );
}
