/* 
 * Universal Host Controller Interface driver for USB (take II).
 *
 * (c) 1999-2001 Georg Acher, acher@in.tum.de (executive slave) (base guitar)
 *               Deti Fliegl, deti@fliegl.de (executive slave) (lead voice)
 *               Thomas Sailer, sailer@ife.ee.ethz.ch (chief consultant) (cheer leader)
 *               Roman Weissgaerber, weissg@vienna.at (virt root hub) (studio porter)
 * (c) 2000      Yggdrasil Computing, Inc. (port of new PCI interface support
 *               from usb-ohci.c by Adam Richter, adam@yggdrasil.com).
 * (C) 2000      David Brownell, david-b@pacbell.net (usb-ohci.c)
 *          
 * HW-initalization based on material of
 *
 * (C) Copyright 1999 Linus Torvalds
 * (C) Copyright 1999 Johannes Erdfelt
 * (C) Copyright 1999 Randy Dunlap
 * (C) Copyright 1999 Gregory P. Smith
 *
 * $Id$
 */
 
//#define DEBUG_LIMIT KERN_DEBUG

#include <atheos/udelay.h>
#include <atheos/isa_io.h>
#include <posix/errno.h>
#include <posix/ioctls.h>
#include <posix/fcntl.h>
#include <posix/termios.h>
#include <posix/signal.h>
#include <macros.h>
#include "bitops.h"

/* This enables more detailed sanity checks in submit_iso */
//#define ISO_SANITY_CHECK

/* This enables all symbols to be exported, to ease debugging oopses */
//#define DEBUG_SYMBOLS

//#define DEBUG

#define VERSTR "$Revision$ time " __TIME__ " " __DATE__

#include "usb-uhci.h"
#include "usb-uhci-debug.h"

#define queue_dbg dbg //err
#define async_dbg dbg //err

#define KMALLOC_FLAG  (MEMF_KERNEL)

/* CONFIG_USB_UHCI_HIGH_BANDWITH turns on Full Speed Bandwidth
 * Reclamation: feature that puts loop on descriptor loop when
 * there's some transfer going on. With FSBR, USB performance
 * is optimal, but PCI can be slowed down up-to 5 times, slowing down
 * system performance (eg. framebuffer devices).
 */
#define CONFIG_USB_UHCI_HIGH_BANDWIDTH 

/* *_DEPTH_FIRST puts descriptor in depth-first mode. This has
 * somehow similar effect to FSBR (higher speed), but does not
 * slow PCI down. OTOH USB performace is slightly slower than
 * in FSBR case and single device could hog whole USB, starving
 * other devices.
 */
#define USE_CTRL_DEPTH_FIRST 0  // 0: Breadth first, 1: Depth first
#define USE_BULK_DEPTH_FIRST 0  // 0: Breadth first, 1: Depth first

/* Turning off both CONFIG_USB_UHCI_HIGH_BANDWITH and *_DEPTH_FIRST
 * will lead to <64KB/sec performance over USB for bulk transfers targeting
 * one device's endpoint. You probably do not want to do that.
 */

// stop bandwidth reclamation after (roughly) 50ms
#define IDLE_TIMEOUT  (1000/20)

// Suppress HC interrupt error messages for 5s
#define ERROR_SUPPRESSION_TIME (1000*5)

static int rh_submit_urb (USB_packet_s *urb);
static int rh_unlink_urb (USB_packet_s *urb);
static int delete_qh (uhci_t *s, uhci_desc_t *qh);
static int process_transfer (uhci_t *s, USB_packet_s *urb, int mode);
static int process_interrupt (uhci_t *s, USB_packet_s *urb);
static int process_iso (uhci_t *s, USB_packet_s *urb, int force);

// How much URBs with ->next are walked
#define MAX_NEXT_COUNT 2048

static uhci_t *devs = NULL;

/* used by userspace UHCI data structure dumper */
uhci_t **uhci_devices = &devs;

static USB_bus_s* g_psBus;
static PCI_bus_s* g_psPCIBus;

/*-------------------------------------------------------------------*/
// Cleans up collected QHs, but not more than 100 in one go
void clean_descs(uhci_t *s, int force)
{
	struct list_head *q;
	uhci_desc_t *qh;
	int now=UHCI_GET_CURRENT_FRAME(s), n=0;

	q=s->free_desc.prev;

	while (q != &s->free_desc && (force || n<100)) {
		qh = list_entry (q, uhci_desc_t, horizontal);		
		q=qh->horizontal.prev;

		if ((qh->last_used!=now) || force)
			delete_qh(s,qh);
		n++;
	}
}
/*-------------------------------------------------------------------*/
static void uhci_switch_timer_int(uhci_t *s)
{

	if (!list_empty(&s->urb_unlinked))
		set_td_ioc(s->td1ms);
	else
		clr_td_ioc(s->td1ms);

	if (s->timeout_urbs)
		set_td_ioc(s->td32ms);
	else
		clr_td_ioc(s->td32ms);
	wmb();
}
/*-------------------------------------------------------------------*/
#ifdef CONFIG_USB_UHCI_HIGH_BANDWIDTH
static void enable_desc_loop(uhci_t *s, USB_packet_s *urb)
{
	int flags;

	if (urb->nTransferFlags & USB_NO_FSBR)
		return;

	spinlock_cli (&s->qh_lock, flags);
	s->chain_end->hw.qh.head&=cpu_to_le32(~UHCI_PTR_TERM);
	mb();
	s->loop_usage++;
	((urb_priv_t*)urb->pHCPrivate)->use_loop=1;
	spinunlock_restore (&s->qh_lock, flags);
}
/*-------------------------------------------------------------------*/
static void disable_desc_loop(uhci_t *s, USB_packet_s *urb)
{
	int flags;

	if (urb->nTransferFlags & USB_NO_FSBR)
		return;

	spinlock_cli (&s->qh_lock, flags);
	if (((urb_priv_t*)urb->pHCPrivate)->use_loop) {
		s->loop_usage--;

		if (!s->loop_usage) {
			s->chain_end->hw.qh.head|=cpu_to_le32(UHCI_PTR_TERM);
			mb();
		}
		((urb_priv_t*)urb->pHCPrivate)->use_loop=0;
	}
	spinunlock_restore (&s->qh_lock, flags);
}
#endif
/*-------------------------------------------------------------------*/
static void queue_urb_unlocked (uhci_t *s, USB_packet_s *urb)
{
	struct list_head *p=&urb->lPacketList;
#ifdef CONFIG_USB_UHCI_HIGH_BANDWIDTH
	{
		int type;
		type=usb_pipetype (urb->nPipe);

		if ((type == USB_PIPE_BULK) || (type == USB_PIPE_CONTROL))
			enable_desc_loop(s, urb);
	}
#endif
	urb->nStatus = -EINPROGRESS;
	((urb_priv_t*)urb->pHCPrivate)->started=get_system_time();
	list_add (p, &s->urb_list);
	if (urb->nTimeOut)
		s->timeout_urbs++;
	uhci_switch_timer_int(s);
}
/*-------------------------------------------------------------------*/
static void queue_urb (uhci_t *s, USB_packet_s *urb)
{
	unsigned long flags=0;

	spinlock_cli (&s->urb_list_lock, flags);
	queue_urb_unlocked(s,urb);
	spinunlock_restore (&s->urb_list_lock, flags);
}
/*-------------------------------------------------------------------*/
static void dequeue_urb (uhci_t *s, USB_packet_s *urb)
{
#ifdef CONFIG_USB_UHCI_HIGH_BANDWIDTH
	int type;

	type=usb_pipetype (urb->nPipe);

	if ((type == USB_PIPE_BULK) || (type == USB_PIPE_CONTROL))
		disable_desc_loop(s, urb);
#endif

	list_del (&urb->lPacketList);
	if (urb->nTimeOut && s->timeout_urbs)
		s->timeout_urbs--;

}
/*-------------------------------------------------------------------*/
static int alloc_td (uhci_t *s, uhci_desc_t ** new, int flags)
{
	void* buffer;

	buffer = kmalloc( sizeof( uhci_desc_t ) + 16, MEMF_KERNEL );
	if (!buffer)
		return -ENOMEM;
	*new = ( uhci_desc_t* )( ( (uint32)buffer + 16 ) & ~15 );
	(*new)->pReal = buffer;
	memset (*new, 0, sizeof (uhci_desc_t));
	(*new)->dma_addr = (uint32)*new;
	set_td_link((*new), UHCI_PTR_TERM | (flags & UHCI_PTR_BITS));	// last by default
	(*new)->type = TD_TYPE;
	mb();
	INIT_LIST_HEAD (&(*new)->vertical);
	INIT_LIST_HEAD (&(*new)->horizontal);
	
	return 0;
}
/*-------------------------------------------------------------------*/
// append a qh to td.link physically, the SW linkage is not affected
static void append_qh(uhci_t *s, uhci_desc_t *td, uhci_desc_t* qh, int  flags)
{
	unsigned long xxx;
	
	spinlock_cli (&s->td_lock, xxx);

	set_td_link(td, qh->dma_addr | (flags & UHCI_PTR_DEPTH) | UHCI_PTR_QH);
       
	mb();
	spinunlock_restore (&s->td_lock, xxx);
}
/*-------------------------------------------------------------------*/
/* insert td at last position in td-list of qh (vertical) */
static int insert_td (uhci_t *s, uhci_desc_t *qh, uhci_desc_t* new, int flags)
{
	uhci_desc_t *prev;
	unsigned long xxx;
	
	spinlock_cli (&s->td_lock, xxx);

	list_add_tail (&new->vertical, &qh->vertical);

	prev = list_entry (new->vertical.prev, uhci_desc_t, vertical);

	if (qh == prev ) {
		// virgin qh without any tds
		set_qh_element(qh, new->dma_addr | UHCI_PTR_TERM);
	}
	else {
		// already tds inserted, implicitely remove TERM bit of prev
		set_td_link(prev, new->dma_addr | (flags & UHCI_PTR_DEPTH));
	}
	mb();
	spinunlock_restore (&s->td_lock, xxx);
	
	return 0;
}
/*-------------------------------------------------------------------*/
/* insert new_td after td (horizontal) */
static int insert_td_horizontal (uhci_t *s, uhci_desc_t *td, uhci_desc_t* new)
{
	uhci_desc_t *next;
	unsigned long flags;
	
	spinlock_cli (&s->td_lock, flags);

	next = list_entry (td->horizontal.next, uhci_desc_t, horizontal);
	list_add (&new->horizontal, &td->horizontal);
	new->hw.td.link = td->hw.td.link;
	set_td_link(td, new->dma_addr);
	mb();
	spinunlock_restore (&s->td_lock, flags);	
	
	return 0;
}
/*-------------------------------------------------------------------*/
static int unlink_td (uhci_t *s, uhci_desc_t *element, int phys_unlink)
{
	uhci_desc_t *next, *prev;
	int dir = 0;
	unsigned long flags;
	
	spinlock_cli (&s->td_lock, flags);
	
	next = list_entry (element->vertical.next, uhci_desc_t, vertical);
	
	if (next == element) {
		dir = 1;
		prev = list_entry (element->horizontal.prev, uhci_desc_t, horizontal);
	}
	else 
		prev = list_entry (element->vertical.prev, uhci_desc_t, vertical);
	
	if (phys_unlink) {
		// really remove HW linking
		if (prev->type == TD_TYPE)
			prev->hw.td.link = element->hw.td.link;
		else
			prev->hw.qh.element = element->hw.td.link;
	}

	mb ();

	if (dir == 0)
		list_del (&element->vertical);
	else
		list_del (&element->horizontal);
	
	spinunlock_restore (&s->td_lock, flags);	
	
	return 0;
}

/*-------------------------------------------------------------------*/
static int delete_desc (uhci_t *s, uhci_desc_t *element)
{
	kfree( element->pReal );
	//pci_pool_free(s->desc_pool, element, element->dma_addr);
	return 0;
}
/*-------------------------------------------------------------------*/
// Allocates qh element
static int alloc_qh (uhci_t *s, uhci_desc_t ** new)
{
	void* buffer;

	buffer = kmalloc( sizeof( uhci_desc_t ) + 16, MEMF_KERNEL | MEMF_OKTOFAILHACK );
	if (!buffer)
		return -ENOMEM;
	*new = ( uhci_desc_t* )( ( (uint32)buffer + 16 ) & ~15 );
	(*new)->pReal = buffer;
	
	memset (*new, 0, sizeof (uhci_desc_t));
	(*new)->dma_addr = (uint32)*new;
	set_qh_head(*new, UHCI_PTR_TERM);
	set_qh_element(*new, UHCI_PTR_TERM);
	(*new)->type = QH_TYPE;
	
	mb();
	INIT_LIST_HEAD (&(*new)->horizontal);
	INIT_LIST_HEAD (&(*new)->vertical);
	
	dbg("Allocated qh @ %p\n", *new);
	
	return 0;
}
/*-------------------------------------------------------------------*/
// inserts new qh before/after the qh at pos
// flags: 0: insert before pos, 1: insert after pos (for low speed transfers)
static int insert_qh (uhci_t *s, uhci_desc_t *pos, uhci_desc_t *new, int order)
{
	uhci_desc_t *old;
	unsigned long flags;

	spinlock_cli (&s->qh_lock, flags);

	if (!order) {
		// (OLD) (POS) -> (OLD) (NEW) (POS)
		old = list_entry (pos->horizontal.prev, uhci_desc_t, horizontal);
		list_add_tail (&new->horizontal, &pos->horizontal);
		set_qh_head(new, MAKE_QH_ADDR (pos)) ;
		if (!(old->hw.qh.head & cpu_to_le32(UHCI_PTR_TERM)))
			set_qh_head(old, MAKE_QH_ADDR (new)) ;
	}
	else {
		// (POS) (OLD) -> (POS) (NEW) (OLD)
		old = list_entry (pos->horizontal.next, uhci_desc_t, horizontal);
		list_add (&new->horizontal, &pos->horizontal);
		set_qh_head(new, MAKE_QH_ADDR (old));
		set_qh_head(pos, MAKE_QH_ADDR (new)) ;
	}

	mb ();
	
	spinunlock_restore (&s->qh_lock, flags);

	return 0;
}

/*-------------------------------------------------------------------*/
static int unlink_qh (uhci_t *s, uhci_desc_t *element)
{
	uhci_desc_t  *prev;
	unsigned long flags;

	spinlock_cli (&s->qh_lock, flags);
	
	prev = list_entry (element->horizontal.prev, uhci_desc_t, horizontal);
	prev->hw.qh.head = element->hw.qh.head;

	dbg("unlink qh %p, pqh %p, nxqh %p, to %08x\n", element, prev, 
	    list_entry (element->horizontal.next, uhci_desc_t, horizontal),le32_to_cpu(element->hw.qh.head) &~15);
	
	list_del(&element->horizontal);

	mb ();
	spinunlock_restore (&s->qh_lock, flags);
	
	return 0;
}
/*-------------------------------------------------------------------*/
static int delete_qh (uhci_t *s, uhci_desc_t *qh)
{
	uhci_desc_t *td;
	struct list_head *p;
	
	list_del (&qh->horizontal);

	while ((p = qh->vertical.next) != &qh->vertical) {
		td = list_entry (p, uhci_desc_t, vertical);
		dbg("unlink td @ %p\n",td);
		unlink_td (s, td, 0); // no physical unlink
		delete_desc (s, td);
	}

	delete_desc (s, qh);
	
	return 0;
}
/*-------------------------------------------------------------------*/
static void clean_td_chain (uhci_t *s, uhci_desc_t *td)
{
	struct list_head *p;
	uhci_desc_t *td1;

	if (!td)
		return;
	
	while ((p = td->horizontal.next) != &td->horizontal) {
		td1 = list_entry (p, uhci_desc_t, horizontal);
		delete_desc (s, td1);
	}
	
	delete_desc (s, td);
}

/*-------------------------------------------------------------------*/
static void fill_td (uhci_desc_t *td, int status, int info, __u32 buffer)
{
	td->hw.td.status = cpu_to_le32(status);
	td->hw.td.info = cpu_to_le32(info);
	td->hw.td.buffer = cpu_to_le32(buffer);
}
/*-------------------------------------------------------------------*/
// Removes ALL qhs in chain (paranoia!)
static void cleanup_skel (uhci_t *s)
{
	unsigned int n;
	uhci_desc_t *td;

	dbg("cleanup_skel\n");

	clean_descs(s,1);

	
	if (s->td32ms) {
	
		unlink_td(s,s->td32ms,1);
		delete_desc(s, s->td32ms);
	}

	for (n = 0; n < 8; n++) {
		td = s->int_chain[n];
		clean_td_chain (s, td);
	}

	if (s->iso_td) {
		for (n = 0; n < 1024; n++) {
			td = s->iso_td[n];
			clean_td_chain (s, td);
		}
		kfree (s->iso_td);
	}

	if (s->framelist_real)
		kfree( s->framelist_real );		

	if (s->control_chain) {
		// completed init_skel?
		struct list_head *p;
		uhci_desc_t *qh, *qh1;

		qh = s->control_chain;
		while ((p = qh->horizontal.next) != &qh->horizontal) {
			qh1 = list_entry (p, uhci_desc_t, horizontal);
			delete_qh (s, qh1);
		}

		delete_qh (s, qh);
	}
	else {
		if (s->ls_control_chain)
			delete_desc (s, s->ls_control_chain);
		if (s->control_chain)
			delete_desc (s, s->control_chain);
		if (s->bulk_chain)
			delete_desc (s, s->bulk_chain);
		if (s->chain_end)
			delete_desc (s, s->chain_end);
	}

/*	if (s->desc_pool) {
		pci_pool_destroy(s->desc_pool);
		s->desc_pool = NULL;
	}*/

	dbg("cleanup_skel finished\n");	
}
/*-------------------------------------------------------------------*/
// allocates framelist and qh-skeletons
// only HW-links provide continous linking, SW-links stay in their domain (ISO/INT)
static int init_skel (uhci_t *s)
{
	int n, ret;
	uhci_desc_t *qh, *td;
	
	dbg("init_skel\n");
	
	s->framelist_real = kmalloc( PAGE_SIZE * 2, MEMF_KERNEL );
	
	
	if (!s->framelist_real)
		return -ENOMEM;
	s->framelist = (uint32*)( PAGE_ALIGN( (uint32)s->framelist_real ) );
	
	memset (s->framelist, 0, PAGE_SIZE);

	dbg("creating descriptor pci_pool\n");

/*	s->desc_pool = pci_pool_create("uhci_desc", s->uhci_pci,
				       sizeof(uhci_desc_t), 16, 0,
				       GFP_DMA | GFP_ATOMIC);	
	if (!s->desc_pool)
		goto init_skel_cleanup;*/

	dbg("allocating iso desc pointer list\n");
	s->iso_td = (uhci_desc_t **) kmalloc (1024 * sizeof (uhci_desc_t*), MEMF_KERNEL);
	
	if (!s->iso_td)
		goto init_skel_cleanup;

	s->ls_control_chain = NULL;
	s->control_chain = NULL;
	s->bulk_chain = NULL;
	s->chain_end = NULL;

	dbg("allocating iso descs\n");
	for (n = 0; n < 1024; n++) {
	 	// allocate skeleton iso/irq-tds
		if (alloc_td (s, &td, 0))
			goto init_skel_cleanup;

		s->iso_td[n] = td;
		s->framelist[n] = cpu_to_le32((__u32) td->dma_addr);
	}

	dbg("allocating qh: chain_end\n");
	if (alloc_qh (s, &qh))	
		goto init_skel_cleanup;
				
	s->chain_end = qh;

	if (alloc_td (s, &td, 0))
		goto init_skel_cleanup;
	
	fill_td (td, 0 * TD_CTRL_IOC, 0, 0); // generate 1ms interrupt (enabled on demand)
	insert_td (s, qh, td, 0);
	qh->hw.qh.element &= cpu_to_le32(~UHCI_PTR_TERM); // remove TERM bit
	s->td1ms=td;

	dbg("allocating qh: bulk_chain\n");
	if (alloc_qh (s, &qh))
		goto init_skel_cleanup;
	
	insert_qh (s, s->chain_end, qh, 0);
	s->bulk_chain = qh;

	dbg("allocating qh: control_chain\n");
	ret = alloc_qh (s, &qh);
	if (ret)
		goto init_skel_cleanup;
	
	insert_qh (s, s->bulk_chain, qh, 0);
	s->control_chain = qh;

#ifdef	CONFIG_USB_UHCI_HIGH_BANDWIDTH
	// disabled reclamation loop
	set_qh_head(s->chain_end, s->control_chain->dma_addr | UHCI_PTR_QH | UHCI_PTR_TERM);
#endif

	dbg("allocating qh: ls_control_chain\n");
	if (alloc_qh (s, &qh))
		goto init_skel_cleanup;
	
	insert_qh (s, s->control_chain, qh, 0);
	s->ls_control_chain = qh;

	for (n = 0; n < 8; n++)
		s->int_chain[n] = 0;

	dbg("allocating skeleton INT-TDs\n");
	
	for (n = 0; n < 8; n++) {
		uhci_desc_t *td;

		if (alloc_td (s, &td, 0))
			goto init_skel_cleanup;

		s->int_chain[n] = td;
		if (n == 0) {
			set_td_link(s->int_chain[0], s->ls_control_chain->dma_addr | UHCI_PTR_QH);
		}
		else {
			set_td_link(s->int_chain[n], s->int_chain[0]->dma_addr);
		}
	}

	dbg("Linking skeleton INT-TDs\n");
	
	for (n = 0; n < 1024; n++) {
		// link all iso-tds to the interrupt chains
		int m, o;
		//dbg("framelist[%i]=%x\n",n,le32_to_cpu(s->framelist[n]));
		if ((n&127)==127) 
			((uhci_desc_t*) s->iso_td[n])->hw.td.link = cpu_to_le32(s->int_chain[0]->dma_addr);
		else 
			for (o = 1, m = 2; m <= 128; o++, m += m)
				if ((n & (m - 1)) == ((m - 1) / 2))
					set_td_link(((uhci_desc_t*) s->iso_td[n]), s->int_chain[o]->dma_addr);
	}

	if (alloc_td (s, &td, 0))
		goto init_skel_cleanup;
	
	fill_td (td, 0 * TD_CTRL_IOC, 0, 0); // generate 32ms interrupt (activated later)
	s->td32ms=td;

	insert_td_horizontal (s, s->int_chain[5], td);

	mb();
	//uhci_show_queue(s->control_chain);   
	dbg("init_skel exit\n");
	return 0;

      init_skel_cleanup:
	cleanup_skel (s);
	return -ENOMEM;
}

/*-------------------------------------------------------------------*/
//                         LOW LEVEL STUFF
//          assembles QHs und TDs for control, bulk and iso
/*-------------------------------------------------------------------*/
static int uhci_submit_control_urb (USB_packet_s *urb)
{
	uhci_desc_t *qh, *td;
	uhci_t *s = (uhci_t*) urb->psDevice->psBus->pHCPrivate;
	urb_priv_t *urb_priv = urb->pHCPrivate;
	unsigned long destination, status;
	int maxsze = usb_maxpacket (urb->psDevice, urb->nPipe, usb_pipeout (urb->nPipe));
	unsigned long len;
	char *data;
	int depth_first=USE_CTRL_DEPTH_FIRST;  // UHCI descriptor chasing method

	dbg("uhci_submit_control start\n");
	if (alloc_qh (s, &qh))		// alloc qh for this request
		return -ENOMEM;

	if (alloc_td (s, &td, UHCI_PTR_DEPTH * depth_first))		// get td for setup stage
	{
		delete_qh (s, qh);
		return -ENOMEM;
	}

	/* The "pipe" thing contains the destination in bits 8--18 */
	destination = (urb->nPipe & USB_PIPE_DEVEP_MASK) | USB_PID_SETUP;

	/* 3 errors */
	status = (urb->nPipe & TD_CTRL_LS) | TD_CTRL_ACTIVE |
		(urb->nTransferFlags & USB_DISABLE_SPD ? 0 : TD_CTRL_SPD) | (3 << 27);

	/*  Build the TD for the control request, try forever, 8 bytes of data */
	fill_td (td, status, destination | (7 << 21), urb_priv->setup_packet_dma);

	insert_td (s, qh, td, 0);	// queue 'setup stage'-td in qh
#if 0
	{
		char *sp=urb->setup_packet;
		dbg("SETUP to pipe %x: %x %x %x %x %x %x %x %x\n", urb->pipe,
		    sp[0],sp[1],sp[2],sp[3],sp[4],sp[5],sp[6],sp[7]);
	}
	//uhci_show_td(td);
#endif

	len = urb->nBufferLength;
	data = urb->pBuffer;

	/* If direction is "send", change the frame from SETUP (0x2D)
	   to OUT (0xE1). Else change it from SETUP to IN (0x69). */

	destination = (urb->nPipe & USB_PIPE_DEVEP_MASK) | (usb_pipeout (urb->nPipe)?USB_PID_OUT:USB_PID_IN);

	while (len > 0) {
		int pktsze = len;

		if (alloc_td (s, &td, UHCI_PTR_DEPTH * depth_first))
			goto fail_unmap_enomem;

		if (pktsze > maxsze)
			pktsze = maxsze;

		destination ^= 1 << TD_TOKEN_TOGGLE;	// toggle DATA0/1

		// Status, pktsze bytes of data
		fill_td (td, status, destination | ((pktsze - 1) << 21),
			 urb_priv->transfer_buffer_dma + (data - (char *)urb->pBuffer));

		insert_td (s, qh, td, UHCI_PTR_DEPTH * depth_first);	// queue 'data stage'-td in qh

		data += pktsze;
		len -= pktsze;
	}

	/* Build the final TD for control status */
	/* It's only IN if the pipe is out AND we aren't expecting data */

	destination &= ~UHCI_PID;

	if (usb_pipeout (urb->nPipe) || (urb->nBufferLength == 0))
		destination |= USB_PID_IN;
	else
		destination |= USB_PID_OUT;

	destination |= 1 << TD_TOKEN_TOGGLE;	/* End in Data1 */

	if (alloc_td (s, &td, UHCI_PTR_DEPTH))
		goto fail_unmap_enomem;

	status &=~TD_CTRL_SPD;

	/* no limit on errors on final packet , 0 bytes of data */
	fill_td (td, status | TD_CTRL_IOC, destination | (UHCI_NULL_DATA_SIZE << 21),
		 0);

	insert_td (s, qh, td, UHCI_PTR_DEPTH * depth_first);	// queue status td

	list_add (&qh->desc_list, &urb_priv->desc_list);

	queue_urb (s, urb);	// queue before inserting in desc chain

	qh->hw.qh.element &= cpu_to_le32(~UHCI_PTR_TERM);

	//uhci_show_queue(qh);
	/* Start it up... put low speed first */
	if (urb->nPipe & TD_CTRL_LS)
		insert_qh (s, s->control_chain, qh, 0);
	else
		insert_qh (s, s->bulk_chain, qh, 0);

	dbg("uhci_submit_control end\n");
	return 0;

fail_unmap_enomem:
	delete_qh(s, qh);
	return -ENOMEM;
}
/*-------------------------------------------------------------------*/
// For queued bulk transfers, two additional QH helpers are allocated (nqh, bqh)
// Due to the linking with other bulk urbs, it has to be locked with urb_list_lock!

static int uhci_submit_bulk_urb (USB_packet_s *urb, USB_packet_s *bulk_urb)
{
	uhci_t *s = (uhci_t*) urb->psDevice->psBus->pHCPrivate;
	urb_priv_t *urb_priv = urb->pHCPrivate, *upriv, *bpriv=NULL;
	uhci_desc_t *qh, *td, *nqh=NULL, *bqh=NULL, *first_td=NULL;
	unsigned long destination, status;
	char *data;
	unsigned int pipe = urb->nPipe;
	int maxsze = usb_maxpacket (urb->psDevice, pipe, usb_pipeout (pipe));
	int info, len, last;
	int depth_first=USE_BULK_DEPTH_FIRST;  // UHCI descriptor chasing method

	if (usb_endpoint_halted (urb->psDevice, usb_pipeendpoint (pipe), usb_pipeout (pipe)))
		return -EPIPE;

	queue_dbg("uhci_submit_bulk_urb: urb %p, old %p, pipe %08x, len %i\n",
		  urb,bulk_urb,urb->nPipe,urb->nBufferLength);

	upriv = (urb_priv_t*)urb->pHCPrivate;

	if (!bulk_urb) {
		if (alloc_qh (s, &qh))		// get qh for this request
			return -ENOMEM;

		if (urb->nTransferFlags & USB_QUEUE_BULK) {
			if (alloc_qh(s, &nqh)) // placeholder for clean unlink
			{
				delete_desc (s, qh);
				return -ENOMEM;
			}
			upriv->next_qh = nqh;
			queue_dbg("new next qh %p\n",nqh);
		}
	}
	else { 
		bpriv = (urb_priv_t*)bulk_urb->pHCPrivate;
		qh = bpriv->bottom_qh;  // re-use bottom qh and next qh
		nqh = bpriv->next_qh;
		upriv->next_qh=nqh;	
		upriv->prev_queued_urb=bulk_urb;
	}

	if (urb->nTransferFlags & USB_QUEUE_BULK) {
		if (alloc_qh (s, &bqh))  // "bottom" QH
		{
			if (!bulk_urb) { 
				delete_desc(s, qh);
				delete_desc(s, nqh);
			}
			return -ENOMEM;
		}
		set_qh_element(bqh, UHCI_PTR_TERM);
		set_qh_head(bqh, nqh->dma_addr | UHCI_PTR_QH); // element
		upriv->bottom_qh = bqh;
	}
	queue_dbg("uhci_submit_bulk: qh %p bqh %p nqh %p\n",qh, bqh, nqh);

	/* The "pipe" thing contains the destination in bits 8--18. */
	destination = (pipe & USB_PIPE_DEVEP_MASK) | usb_packetid (pipe);

	/* 3 errors */
	status = (pipe & TD_CTRL_LS) | TD_CTRL_ACTIVE |
		((urb->nTransferFlags & USB_DISABLE_SPD) ? 0 : TD_CTRL_SPD) | (3 << 27);

	/* Build the TDs for the bulk request */
	len = urb->nBufferLength;
	data = urb->pBuffer;
	
	do {					// TBD: Really allow zero-length packets?
		int pktsze = len;

		if (alloc_td (s, &td, UHCI_PTR_DEPTH * depth_first))
		{
			delete_qh (s, qh);
			return -ENOMEM;
		}

		if (pktsze > maxsze)
			pktsze = maxsze;

		// pktsze bytes of data 
		info = destination | (((pktsze - 1)&UHCI_NULL_DATA_SIZE) << 21) |
			(usb_gettoggle (urb->psDevice, usb_pipeendpoint (pipe), usb_pipeout (pipe)) << TD_TOKEN_TOGGLE);

		fill_td (td, status, info,
			 urb_priv->transfer_buffer_dma + (data - (char *)urb->pBuffer));
			 
		data += pktsze;
		len -= pktsze;
		// Use USB_ZERO_PACKET to finish bulk OUTs always with a zero length packet
		last = (len == 0 && (usb_pipein(pipe) || pktsze < maxsze || !(urb->nTransferFlags & USB_ZERO_PACKET)));

		if (last)
			set_td_ioc(td);	// last one generates INT

		insert_td (s, qh, td, UHCI_PTR_DEPTH * depth_first);
		if (!first_td)
			first_td=td;
		usb_dotoggle (urb->psDevice, usb_pipeendpoint (pipe), usb_pipeout (pipe));

	} while (!last);

	if (bulk_urb && bpriv)   // everything went OK, link with old bulk URB
		bpriv->next_queued_urb=urb;

	list_add (&qh->desc_list, &urb_priv->desc_list);

	if (urb->nTransferFlags & USB_QUEUE_BULK)
		append_qh(s, td, bqh, UHCI_PTR_DEPTH * depth_first);

	queue_urb_unlocked (s, urb);
	
	if (urb->nTransferFlags & USB_QUEUE_BULK)
		set_qh_element(qh, first_td->dma_addr);
	else
		qh->hw.qh.element &= cpu_to_le32(~UHCI_PTR_TERM);    // arm QH

	if (!bulk_urb) { 					// new bulk queue	
		if (urb->nTransferFlags & USB_QUEUE_BULK) {
			spinlock (&s->td_lock);		// both QHs in one go
			insert_qh (s, s->chain_end, qh, 0);	// Main QH
			insert_qh (s, s->chain_end, nqh, 0);	// Helper QH
			spinunlock (&s->td_lock);
		}
		else
			insert_qh (s, s->chain_end, qh, 0);
	}
	
	//uhci_show_queue(s->bulk_chain);
	//dbg("uhci_submit_bulk_urb: exit\n");
	return 0;
}
/*-------------------------------------------------------------------*/
static void uhci_clean_iso_step1(uhci_t *s, urb_priv_t *urb_priv)
{
	struct list_head *p;
	uhci_desc_t *td;

	for (p = urb_priv->desc_list.next; p != &urb_priv->desc_list; p = p->next) {
				td = list_entry (p, uhci_desc_t, desc_list);
				unlink_td (s, td, 1);
	}
}
/*-------------------------------------------------------------------*/
static void uhci_clean_iso_step2(uhci_t *s, urb_priv_t *urb_priv)
{
	struct list_head *p;
	uhci_desc_t *td;

	while ((p = urb_priv->desc_list.next) != &urb_priv->desc_list) {
				td = list_entry (p, uhci_desc_t, desc_list);
				list_del (p);
				delete_desc (s, td);
	}
}
/*-------------------------------------------------------------------*/
/* mode: CLEAN_TRANSFER_NO_DELETION: unlink but no deletion mark (step 1 of async_unlink)
         CLEAN_TRANSFER_REGULAR: regular (unlink/delete-mark)
         CLEAN_TRANSFER_DELETION_MARK: deletion mark for QH (step 2 of async_unlink)
 looks a bit complicated because of all the bulk queueing goodies
*/

static void uhci_clean_transfer (uhci_t *s, USB_packet_s *urb, uhci_desc_t *qh, int mode)
{
	uhci_desc_t *bqh, *nqh, *prevqh, *prevtd;
	int now;
	urb_priv_t *priv=(urb_priv_t*)urb->pHCPrivate;

	now=UHCI_GET_CURRENT_FRAME(s);

	bqh=priv->bottom_qh;	
	
	if (!priv->next_queued_urb)  { // no more appended bulk queues

		queue_dbg("uhci_clean_transfer: No more bulks for urb %p, qh %p, bqh %p, nqh %p\n", urb, qh, bqh, priv->next_qh);	
	
		if (priv->prev_queued_urb && mode != CLEAN_TRANSFER_DELETION_MARK) {  // qh not top of the queue
				unsigned long flags; 
				urb_priv_t* ppriv=(urb_priv_t*)priv->prev_queued_urb->pHCPrivate;

				spinlock_cli (&s->qh_lock, flags);
				prevqh = list_entry (ppriv->desc_list.next, uhci_desc_t, desc_list);
				prevtd = list_entry (prevqh->vertical.prev, uhci_desc_t, vertical);
				set_td_link(prevtd, priv->bottom_qh->dma_addr | UHCI_PTR_QH); // skip current qh
				mb();
				queue_dbg("uhci_clean_transfer: relink pqh %p, ptd %p\n",prevqh, prevtd);
				spinunlock_restore (&s->qh_lock, flags);

				ppriv->bottom_qh = priv->bottom_qh;
				ppriv->next_queued_urb = NULL;
			}
		else {   // queue is dead, qh is top of the queue
			
			if (mode != CLEAN_TRANSFER_DELETION_MARK) 				
				unlink_qh(s, qh); // remove qh from horizontal chain

			if (bqh) {  // remove remainings of bulk queue
				nqh=priv->next_qh;

				if (mode != CLEAN_TRANSFER_DELETION_MARK) 
					unlink_qh(s, nqh);  // remove nqh from horizontal chain
				
				if (mode != CLEAN_TRANSFER_NO_DELETION) {  // add helper QHs to free desc list
					nqh->last_used = bqh->last_used = now;
					list_add_tail (&nqh->horizontal, &s->free_desc);
					list_add_tail (&bqh->horizontal, &s->free_desc);
				}			
			}
		}
	}
	else { // there are queued urbs following
	
	  queue_dbg("uhci_clean_transfer: urb %p, prevurb %p, nexturb %p, qh %p, bqh %p, nqh %p\n",
		       urb, priv->prev_queued_urb,  priv->next_queued_urb, qh, bqh, priv->next_qh);	
       	
		if (mode != CLEAN_TRANSFER_DELETION_MARK) {	// no work for cleanup at unlink-completion
			USB_packet_s *nurb;
			unsigned long flags;

			nurb = priv->next_queued_urb;
			spinlock_cli (&s->qh_lock, flags);		

			if (!priv->prev_queued_urb) { // top QH
				
				prevqh = list_entry (qh->horizontal.prev, uhci_desc_t, horizontal);
				set_qh_head(prevqh, bqh->dma_addr | UHCI_PTR_QH);
				list_del (&qh->horizontal);  // remove this qh form horizontal chain
				list_add (&bqh->horizontal, &prevqh->horizontal); // insert next bqh in horizontal chain
			}
			else {		// intermediate QH
				urb_priv_t* ppriv=(urb_priv_t*)priv->prev_queued_urb->pHCPrivate;
				urb_priv_t* npriv=(urb_priv_t*)nurb->pHCPrivate;
				uhci_desc_t * bnqh;
				
				bnqh = list_entry (npriv->desc_list.next, uhci_desc_t, desc_list);
				ppriv->bottom_qh = bnqh;
				ppriv->next_queued_urb = nurb;				
				prevqh = list_entry (ppriv->desc_list.next, uhci_desc_t, desc_list);
				set_qh_head(prevqh, bqh->dma_addr | UHCI_PTR_QH);
			}

			mb();
			((urb_priv_t*)nurb->pHCPrivate)->prev_queued_urb=priv->prev_queued_urb;
			spinunlock_restore (&s->qh_lock, flags);
		}		
	}

	if (mode != CLEAN_TRANSFER_NO_DELETION) {
		qh->last_used = now;	
		list_add_tail (&qh->horizontal, &s->free_desc); // mark qh for later deletion/kfree
	}
}
/*-------------------------------------------------------------------*/
// Release bandwidth for Interrupt or Isoc. transfers 
static void uhci_release_bandwidth(USB_packet_s *urb)
{       
	if (urb->nBandWidth) {
		switch (usb_pipetype(urb->nPipe)) {
		case USB_PIPE_INTERRUPT:
			g_psBus->release_bandwidth (urb->psDevice, urb, 0);
			break;
		case USB_PIPE_ISOCHRONOUS:
			g_psBus->release_bandwidth (urb->psDevice, urb, 1);
			break;
		default:
			break;
		}
	}	
}

static void uhci_urb_dma_sync(uhci_t *s, USB_packet_s *urb, urb_priv_t *urb_priv)
{
	/*if (urb_priv->setup_packet_dma)
		pci_dma_sync_single(s->uhci_pci, urb_priv->setup_packet_dma,
				    sizeof(devrequest), PCI_DMA_TODEVICE);

	if (urb_priv->transfer_buffer_dma)
		pci_dma_sync_single(s->uhci_pci, urb_priv->transfer_buffer_dma,
				    urb->transfer_buffer_length,
				    usb_pipein(urb->pipe) ?
				    PCI_DMA_FROMDEVICE :
				    PCI_DMA_TODEVICE);*/
}

static void uhci_urb_dma_unmap(uhci_t *s, USB_packet_s *urb, urb_priv_t *urb_priv)
{
	/*if (urb_priv->setup_packet_dma) {
		pci_unmap_single(s->uhci_pci, urb_priv->setup_packet_dma,
				 sizeof(devrequest), PCI_DMA_TODEVICE);
		urb_priv->setup_packet_dma = 0;
	}
	if (urb_priv->transfer_buffer_dma) {
		pci_unmap_single(s->uhci_pci, urb_priv->transfer_buffer_dma,
				 urb->transfer_buffer_length,
				 usb_pipein(urb->pipe) ?
				 PCI_DMA_FROMDEVICE :
				 PCI_DMA_TODEVICE);
		urb_priv->transfer_buffer_dma = 0;
	}*/
}
/*-------------------------------------------------------------------*/
/* needs urb_list_lock!
   mode: UNLINK_ASYNC_STORE_URB: unlink and move URB into unlinked list
         UNLINK_ASYNC_DONT_STORE: unlink, don't move URB into unlinked list
*/
static int uhci_unlink_urb_async (uhci_t *s,USB_packet_s *urb, int mode)
{
	uhci_desc_t *qh;
	urb_priv_t *urb_priv;
	
	async_dbg("unlink_urb_async called %p\n",urb);

	if ((urb->nStatus == -EINPROGRESS) ||
	    ((usb_pipetype (urb->nPipe) ==  USB_PIPE_INTERRUPT) && ((urb_priv_t*)urb->pHCPrivate)->flags))
	{
		((urb_priv_t*)urb->pHCPrivate)->started = ~0;  // mark
		dequeue_urb (s, urb);

		if (mode==UNLINK_ASYNC_STORE_URB)
			list_add_tail (&urb->lPacketList, &s->urb_unlinked); // store urb

		uhci_switch_timer_int(s);
       		s->unlink_urb_done = 1;
		uhci_release_bandwidth(urb);

		urb->nStatus = -ECONNABORTED;	// mark urb as "waiting to be killed"	
		urb_priv = (urb_priv_t*)urb->pHCPrivate;

		switch (usb_pipetype (urb->nPipe)) {
		case USB_PIPE_INTERRUPT:
			usb_dotoggle (urb->psDevice, usb_pipeendpoint (urb->nPipe), usb_pipeout (urb->nPipe));

		case USB_PIPE_ISOCHRONOUS:
			uhci_clean_iso_step1 (s, urb_priv);
			break;

		case USB_PIPE_BULK:
		case USB_PIPE_CONTROL:
			qh = list_entry (urb_priv->desc_list.next, uhci_desc_t, desc_list);
			uhci_clean_transfer (s, urb, qh, CLEAN_TRANSFER_NO_DELETION);
			break;
		}
		((urb_priv_t*)urb->pHCPrivate)->started = UHCI_GET_CURRENT_FRAME(s);
		return -EINPROGRESS;  // completion will follow
	}		

	return 0;    // URB already dead
}
/*-------------------------------------------------------------------*/
// kills an urb by unlinking descriptors and waiting for at least one frame
static int uhci_unlink_urb_sync (uhci_t *s, USB_packet_s *urb)
{
	uhci_desc_t *qh;
	urb_priv_t *urb_priv;
	unsigned long flags=0;
	USB_device_s *usb_dev;

	spinlock_cli (&s->urb_list_lock, flags);

	if (urb->nStatus == -EINPROGRESS) {

		// move descriptors out the the running chains, dequeue urb
		uhci_unlink_urb_async(s, urb, UNLINK_ASYNC_DONT_STORE);

		urb_priv = urb->pHCPrivate;
		urb->nStatus = -ENOENT;	// prevent from double deletion after unlock		
		spinunlock_restore (&s->urb_list_lock, flags);
		
		// cleanup the rest
		switch (usb_pipetype (urb->nPipe)) {

		case USB_PIPE_ISOCHRONOUS:
			uhci_wait_ms(1);
			uhci_clean_iso_step2(s, urb_priv);
			break;

		case USB_PIPE_BULK:
		case USB_PIPE_CONTROL:
			qh = list_entry (urb_priv->desc_list.next, uhci_desc_t, desc_list);
			uhci_clean_transfer(s, urb, qh, CLEAN_TRANSFER_DELETION_MARK);
			uhci_wait_ms(1);
		}
		urb->nStatus = -ENOENT;	// mark urb as killed		
					
		uhci_urb_dma_unmap(s, urb, urb->pHCPrivate);

#ifdef DEBUG_SLAB
		kmem_cache_free (urb_priv_kmem, urb->hcpriv);
#else
		kfree (urb->pHCPrivate);
#endif
		usb_dev = urb->psDevice;
		if (urb->pComplete) {
			dbg("unlink_urb: calling completion\n");
			urb->psDevice = NULL;
			urb->pComplete (urb);
		}
		atomic_dec( &usb_dev->nRefCount );
		//usb_dec_dev_use (usb_dev);
	}
	else
		spinunlock_restore (&s->urb_list_lock, flags);

	return 0;
}
/*-------------------------------------------------------------------*/
// async unlink_urb completion/cleanup work
// has to be protected by urb_list_lock!
// features: if set in transfer_flags, the resulting status of the killed
// transaction is not overwritten

static void uhci_cleanup_unlink(uhci_t *s, int force)
{
	struct list_head *q;
	USB_packet_s *urb;
	USB_device_s *dev;
	int now, type;
	urb_priv_t *urb_priv;

	q=s->urb_unlinked.next;
	now=UHCI_GET_CURRENT_FRAME(s);

	while (q != &s->urb_unlinked) {

		urb = list_entry (q, USB_packet_s, lPacketList);

		urb_priv = (urb_priv_t*)urb->pHCPrivate;
		q = urb->lPacketList.next;
		
		if (!urb_priv) // avoid crash when URB is corrupted
			break;
			
		if (force || ((urb_priv->started != ~0) && (urb_priv->started != now))) {
			async_dbg("async cleanup %p\n",urb);
			type=usb_pipetype (urb->nPipe);

			switch (type) { // process descriptors
			case USB_PIPE_CONTROL:
				process_transfer (s, urb, CLEAN_TRANSFER_DELETION_MARK);  // don't unlink (already done)
				break;
			case USB_PIPE_BULK:
				if (!atomic_read( &s->avoid_bulk ))
					process_transfer (s, urb, CLEAN_TRANSFER_DELETION_MARK); // don't unlink (already done)
				else
					continue;
				break;
			case USB_PIPE_ISOCHRONOUS:
				process_iso (s, urb, PROCESS_ISO_FORCE); // force, don't unlink
				break;
			case USB_PIPE_INTERRUPT:
				process_interrupt (s, urb);
				break;
			}

			if (!(urb->nTransferFlags & USB_TIMEOUT_KILLED))
		  		urb->nStatus = -ECONNRESET; // mark as asynchronously killed

			dev = urb->psDevice;	// completion may destroy all...
			urb_priv = urb->pHCPrivate;
			list_del (&urb->lPacketList);
			
			uhci_urb_dma_sync(s, urb, urb_priv);
			if (urb->pComplete) {
				spinunlock(&s->urb_list_lock);
				urb->psDevice = NULL;
				urb->pComplete (urb);
				spinlock(&s->urb_list_lock);
			}

			if (!(urb->nTransferFlags & USB_TIMEOUT_KILLED))
				urb->nStatus = -ENOENT;  // now the urb is really dead

			switch (type) {
			case USB_PIPE_ISOCHRONOUS:
			case USB_PIPE_INTERRUPT:
				uhci_clean_iso_step2(s, urb_priv);
				break;
			}
	
			uhci_urb_dma_unmap(s, urb, urb_priv);

			atomic_dec( &dev->nRefCount );
			//usb_dec_dev_use (dev);
#ifdef DEBUG_SLAB
			kmem_cache_free (urb_priv_kmem, urb_priv);
#else
			kfree (urb_priv);
#endif

		}
	}
}
 
/*-------------------------------------------------------------------*/
static int uhci_unlink_urb (USB_packet_s *urb)
{
	uhci_t *s;
	unsigned long flags=0;
	dbg("uhci_unlink_urb called for %p\n",urb);
	if (!urb || !urb->psDevice)		// you never know...
		return -EINVAL;
	
	s = (uhci_t*) urb->psDevice->psBus->pHCPrivate;

	if (usb_pipedevice (urb->nPipe) == s->rh.devnum)
		return rh_unlink_urb (urb);

	if (!urb->pHCPrivate)
		return -EINVAL;

	if (urb->nTransferFlags & USB_ASYNC_UNLINK) {
		int ret;
       		spinlock_cli (&s->urb_list_lock, flags);
       		
		uhci_release_bandwidth(urb);
		ret = uhci_unlink_urb_async(s, urb, UNLINK_ASYNC_STORE_URB);

		spinunlock_restore (&s->urb_list_lock, flags);	
		return ret;
	}
	else
		return uhci_unlink_urb_sync(s, urb);
}
/*-------------------------------------------------------------------*/
// In case of ASAP iso transfer, search the URB-list for already queued URBs
// for this EP and calculate the earliest start frame for the new
// URB (easy seamless URB continuation!)
static int find_iso_limits (USB_packet_s *urb, unsigned int *start, unsigned int *end)
{
	USB_packet_s *u, *last_urb = NULL;
	uhci_t *s = (uhci_t*) urb->psDevice->psBus->pHCPrivate;
	struct list_head *p;
	int ret=-1;
	unsigned long flags;
	
	spinlock_cli (&s->urb_list_lock, flags);
	p=s->urb_list.prev;

	for (; p != &s->urb_list; p = p->prev) {
		u = list_entry (p, USB_packet_s, lPacketList);
		// look for pending URBs with identical pipe handle
		// works only because iso doesn't toggle the data bit!
		if ((urb->nPipe == u->nPipe) && (urb->psDevice == u->psDevice) && (u->nStatus == -EINPROGRESS)) {
			if (!last_urb)
				*start = u->nStartFrame;
			last_urb = u;
		}
	}
	
	if (last_urb) {
		*end = (last_urb->nStartFrame + last_urb->nPacketNum) & 1023;
		ret=0;
	}
	
	spinunlock_restore(&s->urb_list_lock, flags);
	
	return ret;
}
/*-------------------------------------------------------------------*/
// adjust start_frame according to scheduling constraints (ASAP etc)

static int iso_find_start (USB_packet_s *urb)
{
	uhci_t *s = (uhci_t*) urb->psDevice->psBus->pHCPrivate;
	unsigned int now;
	unsigned int start_limit = 0, stop_limit = 0, queued_size;
	int limits;

	now = UHCI_GET_CURRENT_FRAME (s) & 1023;

	if ((unsigned) urb->nPacketNum > 900)
		return -EFBIG;
	
	limits = find_iso_limits (urb, &start_limit, &stop_limit);
	queued_size = (stop_limit - start_limit) & 1023;

	if (urb->nTransferFlags & USB_ISO_ASAP) {
		// first iso
		if (limits) {
			// 10ms setup should be enough //FIXME!
			urb->nStartFrame = (now + 10) & 1023;
		}
		else {
			urb->nStartFrame = stop_limit;		//seamless linkage

			if (((now - urb->nStartFrame) & 1023) <= (unsigned) urb->nPacketNum) {
				//info("iso_find_start: gap in seamless isochronous scheduling");
				dbg("iso_find_start: now %u start_frame %u number_of_packets %u pipe 0x%08x\n",
					now, urb->nStartFrame, urb->nPacketNum, urb->nPipe);
				urb->nStartFrame = (now + 5) & 1023;	// 5ms setup should be enough //FIXME!
			}
		}
	}
	else {
		urb->nStartFrame &= 1023;
		if (((now - urb->nStartFrame) & 1023) < (unsigned) urb->nPacketNum) {
			dbg("iso_find_start: now between start_frame and end\n");
			return -EAGAIN;
		}
	}

	/* check if either start_frame or start_frame+number_of_packets-1 lies between start_limit and stop_limit */
	if (limits)
		return 0;

	if (((urb->nStartFrame - start_limit) & 1023) < queued_size ||
	    ((urb->nStartFrame + urb->nPacketNum - 1 - start_limit) & 1023) < queued_size) {
		dbg("iso_find_start: start_frame %u number_of_packets %u start_limit %u stop_limit %u\n",
			urb->nStartFrame, urb->nPacketNum, start_limit, stop_limit);
		return -EAGAIN;
	}

	return 0;
}
/*-------------------------------------------------------------------*/
// submits USB interrupt (ie. polling ;-) 
// ASAP-flag set implicitely
// if period==0, the transfer is only done once

static int uhci_submit_int_urb (USB_packet_s *urb)
{
	uhci_t *s = (uhci_t*) urb->psDevice->psBus->pHCPrivate;
	urb_priv_t *urb_priv = urb->pHCPrivate;
	int nint, n;
	uhci_desc_t *td;
	int status, destination;
	int info;
	unsigned int pipe = urb->nPipe;

	if (urb->nInterval < 0 || urb->nInterval >= 256)
		return -EINVAL;

	if (urb->nInterval == 0)
		nint = 0;
	else {
		for (nint = 0, n = 1; nint <= 8; nint++, n += n)	// round interval down to 2^n
		 {
			if (urb->nInterval < n) {
				urb->nInterval = n / 2;
				break;
			}
		}
		nint--;
	}

	dbg("Rounded interval to %i, chain  %i\n", urb->nInterval, nint);

	urb->nStartFrame = UHCI_GET_CURRENT_FRAME (s) & 1023;	// remember start frame, just in case...

	urb->nPacketNum = 1;

	// INT allows only one packet
	if (urb->nBufferLength > usb_maxpacket (urb->psDevice, pipe, usb_pipeout (pipe)))
		return -EINVAL;

	if (alloc_td (s, &td, UHCI_PTR_DEPTH))
		return -ENOMEM;

	status = (pipe & TD_CTRL_LS) | TD_CTRL_ACTIVE | TD_CTRL_IOC |
		(urb->nTransferFlags & USB_DISABLE_SPD ? 0 : TD_CTRL_SPD) | (3 << 27);

	destination = (urb->nPipe & USB_PIPE_DEVEP_MASK) | usb_packetid (urb->nPipe) |
		(((urb->nBufferLength - 1) & 0x7ff) << 21);


	info = destination | (usb_gettoggle (urb->psDevice, usb_pipeendpoint (pipe), usb_pipeout (pipe)) << TD_TOKEN_TOGGLE);

	fill_td (td, status, info, urb_priv->transfer_buffer_dma);
	list_add_tail (&td->desc_list, &urb_priv->desc_list);

	queue_urb (s, urb);

	insert_td_horizontal (s, s->int_chain[nint], td);	// store in INT-TDs

	usb_dotoggle (urb->psDevice, usb_pipeendpoint (pipe), usb_pipeout (pipe));

	return 0;
}
/*-------------------------------------------------------------------*/
static int uhci_submit_iso_urb (USB_packet_s *urb)
{
	uhci_t *s = (uhci_t*) urb->psDevice->psBus->pHCPrivate;
	urb_priv_t *urb_priv = urb->pHCPrivate;
#ifdef ISO_SANITY_CHECK
	int pipe=urb->pipe;
	int maxsze = usb_maxpacket (urb->dev, pipe, usb_pipeout (pipe));
#endif
	int n, ret, last=0;
	uhci_desc_t *td, **tdm;
	int status, destination;
	unsigned long flags;

	flags = cli();/*__save_flags(flags);
	__cli();		*/      // Disable IRQs to schedule all ISO-TDs in time
	ret = iso_find_start (urb);	// adjusts urb->start_frame for later use
	
	if (ret)
		goto err;

	tdm = (uhci_desc_t **) kmalloc (urb->nPacketNum * sizeof (uhci_desc_t*), MEMF_KERNEL);

	if (!tdm) {
		ret = -ENOMEM;
		goto err;
	}

	memset(tdm, 0, urb->nPacketNum * sizeof (uhci_desc_t*));

	// First try to get all TDs. Cause: Removing already inserted TDs can only be done 
	// racefree in three steps: unlink TDs, wait one frame, delete TDs. 
	// So, this solutions seems simpler...

	for (n = 0; n < urb->nPacketNum; n++) {
		dbg("n:%d urb->iso_frame_desc[n].length:%d\n", n, urb->sISOFrame[n].nLength);
		if (!urb->sISOFrame[n].nLength)
			continue;  // allows ISO striping by setting length to zero in iso_descriptor


#ifdef ISO_SANITY_CHECK
		if(urb->sISOFrame[n].nLength > maxsze) {

			err("submit_iso: urb->iso_frame_desc[%d].length(%d)>%d",n , urb->sISOFrame[n].nLength, maxsze);
			ret=-EINVAL;		
		}
		else
#endif
		if (alloc_td (s, &td, UHCI_PTR_DEPTH)) {
			int i;	// Cleanup allocated TDs

			for (i = 0; i < n; n++)
				if (tdm[i])
					 delete_desc(s, tdm[i]);
			kfree (tdm);
			goto err;
		}
		last=n;
		tdm[n] = td;
	}

	status = TD_CTRL_ACTIVE | TD_CTRL_IOS;

	destination = (urb->nPipe & USB_PIPE_DEVEP_MASK) | usb_packetid (urb->nPipe);

	// Queue all allocated TDs
	for (n = 0; n < urb->nPacketNum; n++) {
		td = tdm[n];
		if (!td)
			continue;
			
		if (n  == last) {
			status |= TD_CTRL_IOC;
			queue_urb (s, urb);
		}

		fill_td (td, status, destination | (((urb->sISOFrame[n].nLength - 1) & 0x7ff) << 21),
			 urb_priv->transfer_buffer_dma + urb->sISOFrame[n].nOffset);
		list_add_tail (&td->desc_list, &urb_priv->desc_list);
	
		insert_td_horizontal (s, s->iso_td[(urb->nStartFrame + n) & 1023], td);	// store in iso-tds
	}

	kfree (tdm);
	dbg("ISO-INT# %i, start %i, now %i\n", urb->nPacketNum, urb->nStartFrame, UHCI_GET_CURRENT_FRAME (s) & 1023);
	ret = 0;

      err:
	put_cpu_flags( flags );
	return ret;
}
/*-------------------------------------------------------------------*/
// returns: 0 (no transfer queued), urb* (this urb already queued)
 
static USB_packet_s* search_dev_ep (uhci_t *s, USB_packet_s *urb)
{
	struct list_head *p;
	USB_packet_s *tmp;
	unsigned int mask = usb_pipecontrol(urb->nPipe) ? (~USB_DIR_IN) : (~0);

	dbg("search_dev_ep:\n");

	p=s->urb_list.next;

	for (; p != &s->urb_list; p = p->next) {
		tmp = list_entry (p, USB_packet_s, lPacketList);
		dbg("urb: %p\n", tmp);
		// we can accept this urb if it is not queued at this time 
		// or if non-iso transfer requests should be scheduled for the same device and pipe
		if ((!usb_pipeisoc(urb->nPipe) && (tmp->psDevice == urb->psDevice) && !((tmp->nPipe ^ urb->nPipe) & mask)) ||
		    (urb == tmp)) {
			return tmp;	// found another urb already queued for processing
		}
	}

	return 0;
}
/*-------------------------------------------------------------------*/
static int uhci_submit_urb (USB_packet_s *urb)
{
	uhci_t *s;
	urb_priv_t *urb_priv;
	int ret = 0, type;
	unsigned long flags;
	USB_packet_s *queued_urb=NULL;
	int bustime;
	
	
		
	if (!urb->psDevice || !urb->psDevice->psBus)
		return -ENODEV;

	s = (uhci_t*) urb->psDevice->psBus->pHCPrivate;
	dbg("submit_urb: %p type %d\n",urb,usb_pipetype(urb->nPipe));
	
	if (!s->running)
		return -ENODEV;
	
	type = usb_pipetype (urb->nPipe);
	
	dbg( "Submitting urb to %i Hub: %i\n", usb_pipedevice (urb->nPipe), s->rh.devnum );

	//if (usb_pipedevice (urb->nPipe) == s->rh.devnum)
	if( urb->psDevice == s->bus->psRootHUB )
		return rh_submit_urb (urb);	/* virtual root hub */

	// Sanity checks
	if (usb_maxpacket (urb->psDevice, urb->nPipe, usb_pipeout (urb->nPipe)) <= 0) {		
		printk("uhci_submit_urb: pipesize for pipe %x is zero\n", urb->nPipe);
		return -EMSGSIZE;
	}

	if (urb->nBufferLength < 0 && type != USB_PIPE_ISOCHRONOUS) {
		printk("uhci_submit_urb: Negative transfer length for urb %p\n", urb);
		return -EINVAL;
	}
	

	atomic_inc( &urb->psDevice->nRefCount );
	//usb_inc_dev_use (urb->dev);

	spinlock_cli (&s->urb_list_lock, flags);

	queued_urb = search_dev_ep (s, urb); // returns already queued urb for that pipe

	if (queued_urb) {

		queue_dbg("found bulk urb %p\n", queued_urb);

		if (( type != USB_PIPE_BULK) ||
		    ((type == USB_PIPE_BULK) &&
		     (!(urb->nTransferFlags & USB_QUEUE_BULK) || !(queued_urb->nTransferFlags & USB_QUEUE_BULK)))) {
			spinunlock_restore (&s->urb_list_lock, flags);
			atomic_dec( &urb->psDevice->nRefCount );
			//usb_dec_dev_use (urb->dev);
			printk("UHCI: ENXIO %08x, flags %x, urb %p, burb %p\n",urb->nPipe,urb->nTransferFlags,urb,queued_urb);
			return -ENXIO;	// urb already queued
		}
	}
	

#ifdef DEBUG_SLAB
	urb_priv = kmem_cache_alloc(urb_priv_kmem, SLAB_FLAG);
#else
	urb_priv = kmalloc (sizeof (urb_priv_t), MEMF_KERNEL);
#endif
	if (!urb_priv) {
		atomic_dec( &urb->psDevice->nRefCount );
		spinunlock_restore (&s->urb_list_lock, flags);
		return -ENOMEM;
	}

	memset(urb_priv, 0, sizeof(urb_priv_t));
	urb->pHCPrivate = urb_priv;
	INIT_LIST_HEAD (&urb_priv->desc_list);

	dbg("submit_urb: scheduling %p\n", urb);
	
	if (type == USB_PIPE_CONTROL)
		urb_priv->setup_packet_dma = (uint32)urb->pSetupPacket;
	if (urb->nBufferLength)
		urb_priv->transfer_buffer_dma =  (uint32)urb->pBuffer;
	
	/*if (type == PIPE_CONTROL)
		urb_priv->setup_packet_dma = pci_map_single(s->uhci_pci, urb->setup_packet,
							    sizeof(devrequest), PCI_DMA_TODEVICE);

	if (urb->transfer_buffer_length)
		urb_priv->transfer_buffer_dma = pci_map_single(s->uhci_pci,
							       urb->transfer_buffer,
							       urb->transfer_buffer_length,
							       usb_pipein(urb->pipe) ?
							       PCI_DMA_FROMDEVICE :
							       PCI_DMA_TODEVICE);*/

	if (type == USB_PIPE_BULK) {
	
		if (queued_urb) {
			while (((urb_priv_t*)queued_urb->pHCPrivate)->next_queued_urb)  // find last queued bulk
				queued_urb=((urb_priv_t*)queued_urb->pHCPrivate)->next_queued_urb;
			
			((urb_priv_t*)queued_urb->pHCPrivate)->next_queued_urb=urb;
		}
		atomic_inc(&s->avoid_bulk);
		ret = uhci_submit_bulk_urb (urb, queued_urb);
		atomic_dec(&s->avoid_bulk);
		spinunlock_restore (&s->urb_list_lock, flags);
	}
	else {
		spinunlock_restore (&s->urb_list_lock, flags);
		switch (type) {
		case USB_PIPE_ISOCHRONOUS:			
			if (urb->nBandWidth == 0) {      /* not yet checked/allocated */
				if (urb->nPacketNum <= 0) {
					ret = -EINVAL;
					break;
				}

				bustime = g_psBus->check_bandwidth (urb->psDevice, urb);
				if (bustime < 0) 
					ret = bustime;
				else {
					ret = uhci_submit_iso_urb(urb);
					if (ret == 0)
						g_psBus->claim_bandwidth (urb->psDevice, urb, bustime, 1);
				}
			} else {        /* bandwidth is already set */
				ret = uhci_submit_iso_urb(urb);
			}
			break;
		case USB_PIPE_INTERRUPT:
			if (urb->nBandWidth == 0) {      /* not yet checked/allocated */
				bustime = g_psBus->check_bandwidth (urb->psDevice, urb);
				if (bustime < 0)
					ret = bustime;
				else {
					ret = uhci_submit_int_urb(urb);
					if (ret == 0)
						g_psBus->claim_bandwidth (urb->psDevice, urb, bustime, 0);
				}
			} else {        /* bandwidth is already set */
				ret = uhci_submit_int_urb(urb);
			}
			break;
		case USB_PIPE_CONTROL:
			ret = uhci_submit_control_urb (urb);
			break;
		default:
			ret = -EINVAL;
		}
	
	}
	dbg("submit_urb: scheduled with ret: %d\n", ret);
	
	if (ret != 0) {
		uhci_urb_dma_unmap(s, urb, urb_priv);
		atomic_dec( &urb->psDevice->nRefCount );
		//usb_dec_dev_use (urb->psDevice);
#ifdef DEBUG_SLAB
		kmem_cache_free(urb_priv_kmem, urb_priv);
#else
		kfree (urb_priv);
#endif
		return ret;
	}

	return 0;
}

// Checks for URB timeout and removes bandwidth reclamation if URB idles too long
static void uhci_check_timeouts(uhci_t *s)
{
	struct list_head *p,*p2;
	USB_packet_s *urb;
	int type;	

	p = s->urb_list.prev;	

	while (p != &s->urb_list) {
		urb_priv_t *hcpriv;

		p2 = p;
		p = p->prev;
		urb = list_entry (p2, USB_packet_s, lPacketList);
		type = usb_pipetype (urb->nPipe);

		hcpriv = (urb_priv_t*)urb->pHCPrivate;
				
		if ( urb->nTimeOut && 
			((hcpriv->started + urb->nTimeOut) < get_system_time())) {
			urb->nTransferFlags |= USB_TIMEOUT_KILLED | USB_ASYNC_UNLINK;
			async_dbg("uhci_check_timeout: timeout for %p\n",urb);
			uhci_unlink_urb_async(s, urb, UNLINK_ASYNC_STORE_URB);
		}
#ifdef CONFIG_USB_UHCI_HIGH_BANDWIDTH
		else if (((type == USB_PIPE_BULK) || (type == USB_PIPE_CONTROL)) &&  
		     (hcpriv->use_loop) &&
		     ((hcpriv->started + IDLE_TIMEOUT * 1000) < get_system_time()))
			disable_desc_loop(s, urb);
#endif

	}
	s->timeout_check=get_system_time();
}

/*-------------------------------------------------------------------
 Virtual Root Hub
 -------------------------------------------------------------------*/

static __u8 root_hub_dev_des[] =
{
	0x12,			/*  __u8  bLength; */
	0x01,			/*  __u8  bDescriptorType; Device */
	0x00,			/*  __u16 bcdUSB; v1.0 */
	0x01,
	0x09,			/*  __u8  bDeviceClass; HUB_CLASSCODE */
	0x00,			/*  __u8  bDeviceSubClass; */
	0x00,			/*  __u8  bDeviceProtocol; */
	0x08,			/*  __u8  bMaxPacketSize0; 8 Bytes */
	0x00,			/*  __u16 idVendor; */
	0x00,
	0x00,			/*  __u16 idProduct; */
	0x00,
	0x00,			/*  __u16 bcdDevice; */
	0x00,
	0x00,			/*  __u8  iManufacturer; */
	0x02,			/*  __u8  iProduct; */
	0x01,			/*  __u8  iSerialNumber; */
	0x01			/*  __u8  bNumConfigurations; */
};


/* Configuration descriptor */
static __u8 root_hub_config_des[] =
{
	0x09,			/*  __u8  bLength; */
	0x02,			/*  __u8  bDescriptorType; Configuration */
	0x19,			/*  __u16 wTotalLength; */
	0x00,
	0x01,			/*  __u8  bNumInterfaces; */
	0x01,			/*  __u8  bConfigurationValue; */
	0x00,			/*  __u8  iConfiguration; */
	0x40,			/*  __u8  bmAttributes; 
				   Bit 7: Bus-powered, 6: Self-powered, 5 Remote-wakwup, 4..0: resvd */
	0x00,			/*  __u8  MaxPower; */

     /* interface */
	0x09,			/*  __u8  if_bLength; */
	0x04,			/*  __u8  if_bDescriptorType; Interface */
	0x00,			/*  __u8  if_bInterfaceNumber; */
	0x00,			/*  __u8  if_bAlternateSetting; */
	0x01,			/*  __u8  if_bNumEndpoints; */
	0x09,			/*  __u8  if_bInterfaceClass; HUB_CLASSCODE */
	0x00,			/*  __u8  if_bInterfaceSubClass; */
	0x00,			/*  __u8  if_bInterfaceProtocol; */
	0x00,			/*  __u8  if_iInterface; */

     /* endpoint */
	0x07,			/*  __u8  ep_bLength; */
	0x05,			/*  __u8  ep_bDescriptorType; Endpoint */
	0x81,			/*  __u8  ep_bEndpointAddress; IN Endpoint 1 */
	0x03,			/*  __u8  ep_bmAttributes; Interrupt */
	0x08,			/*  __u16 ep_wMaxPacketSize; 8 Bytes */
	0x00,
	0xff			/*  __u8  ep_bInterval; 255 ms */
};


static __u8 root_hub_hub_des[] =
{
	0x09,			/*  __u8  bLength; */
	0x29,			/*  __u8  bDescriptorType; Hub-descriptor */
	0x02,			/*  __u8  bNbrPorts; */
	0x00,			/* __u16  wHubCharacteristics; */
	0x00,
	0x01,			/*  __u8  bPwrOn2pwrGood; 2ms */
	0x00,			/*  __u8  bHubContrCurrent; 0 mA */
	0x00,			/*  __u8  DeviceRemovable; *** 7 Ports max *** */
	0xff			/*  __u8  PortPwrCtrlMask; *** 7 ports max *** */
};

/*-------------------------------------------------------------------------*/
/* prepare Interrupt pipe transaction data; HUB INTERRUPT ENDPOINT */
static int rh_send_irq (USB_packet_s *urb)
{
	int len = 1;
	int i;
	uhci_t *uhci = urb->psDevice->psBus->pHCPrivate;
	unsigned int io_addr = uhci->io_addr;
	__u16 data = 0;

	for (i = 0; i < uhci->rh.numports; i++) {
		data |= ((inw (io_addr + USBPORTSC1 + i * 2) & 0xa) > 0 ? (1 << (i + 1)) : 0);
		len = (i + 1) / 8 + 1;
	}

	*(__u16 *) urb->pBuffer = cpu_to_le16 (data);
	urb->nActualLength = len;
	urb->nStatus = 0;
	
	if ((data > 0) && (uhci->rh.send != 0)) {
		dbg("Root-Hub INT complete: port1: %x port2: %x data: %x\n",
		     inw (io_addr + USBPORTSC1), inw (io_addr + USBPORTSC2), data);
		urb->pComplete (urb);
	}
	return 0;
}

/*-------------------------------------------------------------------------*/
/* Virtual Root Hub INTs are polled by this timer every "intervall" ms */
static int rh_init_int_timer (USB_packet_s *urb);

static void rh_int_timer_do (void* ptr)
{
	int len;
	USB_packet_s *urb = (USB_packet_s*) ptr;
	uhci_t *uhci = urb->psDevice->psBus->pHCPrivate;
	
	delete_timer( uhci->rh.rh_int_timer );

	if (uhci->rh.send) {
		len = rh_send_irq (urb);
		if (len > 0) {
			urb->nActualLength = len;
			if (urb->pComplete)
				urb->pComplete (urb);
		}
	}
	rh_init_int_timer (urb);
}

/*-------------------------------------------------------------------------*/
/* Root Hub INTs are polled by this timer, polling interval 20ms */

static int rh_init_int_timer (USB_packet_s *urb)
{
	uhci_t *uhci = urb->psDevice->psBus->pHCPrivate;

	uhci->rh.interval = urb->nInterval;
	uhci->rh.rh_int_timer = create_timer();
	
	start_timer( uhci->rh.rh_int_timer, rh_int_timer_do, (void*)urb, 20 * 1000, true );
	
	/*init_timer (&uhci->rh.rh_int_timer);
	uhci->rh.rh_int_timer.function = rh_int_timer_do;
	uhci->rh.rh_int_timer.data = (unsigned long) urb;
	uhci->rh.rh_int_timer.expires = jiffies + (HZ * 20) / 1000;
	add_timer (&uhci->rh.rh_int_timer);*/

	return 0;
}

/*-------------------------------------------------------------------------*/
#define OK(x) 			len = (x); break

#define CLR_RH_PORTSTAT(x) \
		status = inw(io_addr+USBPORTSC1+2*(wIndex-1)); \
		status = (status & 0xfff5) & ~(x); \
		outw(status, io_addr+USBPORTSC1+2*(wIndex-1))

#define SET_RH_PORTSTAT(x) \
		status = inw(io_addr+USBPORTSC1+2*(wIndex-1)); \
		status = (status & 0xfff5) | (x); \
		outw(status, io_addr+USBPORTSC1+2*(wIndex-1))


/*-------------------------------------------------------------------------*/
/****
 ** Root Hub Control Pipe
 *************************/


#define min_t(type,x,y) \
	({ type __x = (x); type __y = (y); __x < __y ? __x: __y; })

static int rh_submit_urb (USB_packet_s *urb)
{
	USB_device_s *usb_dev = urb->psDevice;
	uhci_t *uhci = usb_dev->psBus->pHCPrivate;
	unsigned int pipe = urb->nPipe;
	USB_request_s *cmd = (USB_request_s *) urb->pSetupPacket;
	void *data = urb->pBuffer;
	int leni = urb->nBufferLength;
	int len = 0;
	int status = 0;
	int stat = 0;
	int i;
	unsigned int io_addr = uhci->io_addr;
	__u16 cstatus;

	__u16 bmRType_bReq;
	__u16 wValue;
	__u16 wIndex;
	__u16 wLength;

	if (usb_pipetype (pipe) == USB_PIPE_INTERRUPT) {
		dbg("Root-Hub submit IRQ: every %d ms\n", urb->nInterval);
		uhci->rh.urb = urb;
		uhci->rh.send = 1;
		uhci->rh.interval = urb->nInterval;
		rh_init_int_timer (urb);

		return 0;
	}


	bmRType_bReq = cmd->nRequestType | cmd->nRequest << 8;
	wValue = le16_to_cpu (cmd->nValue);
	wIndex = le16_to_cpu (cmd->nIndex);
	wLength = le16_to_cpu (cmd->nLength);

	for (i = 0; i < 8; i++)
		uhci->rh.c_p_r[i] = 0;

	dbg("Root-Hub: adr: %2x cmd(%1x): %04x %04x %04x %04x\n",
	     uhci->rh.devnum, 8, bmRType_bReq, wValue, wIndex, wLength);

	switch (bmRType_bReq) {
		/* Request Destination:
		   without flags: Device, 
		   RH_INTERFACE: interface, 
		   RH_ENDPOINT: endpoint,
		   RH_CLASS means HUB here, 
		   RH_OTHER | RH_CLASS  almost ever means HUB_PORT here 
		 */

	case RH_GET_STATUS:
		*(__u16 *) data = cpu_to_le16 (1);
		OK (2);
	case RH_GET_STATUS | RH_INTERFACE:
		*(__u16 *) data = cpu_to_le16 (0);
		OK (2);
	case RH_GET_STATUS | RH_ENDPOINT:
		*(__u16 *) data = cpu_to_le16 (0);
		OK (2);
	case RH_GET_STATUS | RH_CLASS:
		*(__u32 *) data = cpu_to_le32 (0);
		OK (4);		/* hub power ** */
	case RH_GET_STATUS | RH_OTHER | RH_CLASS:
		status = inw (io_addr + USBPORTSC1 + 2 * (wIndex - 1));
		cstatus = ((status & USBPORTSC_CSC) >> (1 - 0)) |
			((status & USBPORTSC_PEC) >> (3 - 1)) |
			(uhci->rh.c_p_r[wIndex - 1] << (0 + 4));
		status = (status & USBPORTSC_CCS) |
			((status & USBPORTSC_PE) >> (2 - 1)) |
			((status & USBPORTSC_SUSP) >> (12 - 2)) |
			((status & USBPORTSC_PR) >> (9 - 4)) |
			(1 << 8) |	/* power on ** */
			((status & USBPORTSC_LSDA) << (-8 + 9));

		*(__u16 *) data = cpu_to_le16 (status);
		*(__u16 *) (data + 2) = cpu_to_le16 (cstatus);
		OK (4);

	case RH_CLEAR_FEATURE | RH_ENDPOINT:
		switch (wValue) {
		case (RH_ENDPOINT_STALL):
			OK (0);
		}
		break;

	case RH_CLEAR_FEATURE | RH_CLASS:
		switch (wValue) {
		case (RH_C_HUB_OVER_CURRENT):
			OK (0);	/* hub power over current ** */
		}
		break;

	case RH_CLEAR_FEATURE | RH_OTHER | RH_CLASS:
		switch (wValue) {
		case (RH_PORT_ENABLE):
			CLR_RH_PORTSTAT (USBPORTSC_PE);
			OK (0);
		case (RH_PORT_SUSPEND):
			CLR_RH_PORTSTAT (USBPORTSC_SUSP);
			OK (0);
		case (RH_PORT_POWER):
			OK (0);	/* port power ** */
		case (RH_C_PORT_CONNECTION):
			SET_RH_PORTSTAT (USBPORTSC_CSC);
			OK (0);
		case (RH_C_PORT_ENABLE):
			SET_RH_PORTSTAT (USBPORTSC_PEC);
			OK (0);
		case (RH_C_PORT_SUSPEND):
/*** WR_RH_PORTSTAT(RH_PS_PSSC); */
			OK (0);
		case (RH_C_PORT_OVER_CURRENT):
			OK (0);	/* port power over current ** */
		case (RH_C_PORT_RESET):
			uhci->rh.c_p_r[wIndex - 1] = 0;
			OK (0);
		}
		break;

	case RH_SET_FEATURE | RH_OTHER | RH_CLASS:
		switch (wValue) {
		case (RH_PORT_SUSPEND):
			SET_RH_PORTSTAT (USBPORTSC_SUSP);
			OK (0);
		case (RH_PORT_RESET):
			SET_RH_PORTSTAT (USBPORTSC_PR);
			uhci_wait_ms (10);
			uhci->rh.c_p_r[wIndex - 1] = 1;
			CLR_RH_PORTSTAT (USBPORTSC_PR);
			udelay (10);
			SET_RH_PORTSTAT (USBPORTSC_PE);
			uhci_wait_ms (10);
			SET_RH_PORTSTAT (0xa);
			OK (0);
		case (RH_PORT_POWER):
			OK (0);	/* port power ** */
		case (RH_PORT_ENABLE):
			SET_RH_PORTSTAT (USBPORTSC_PE);
			OK (0);
		}
		break;

	case RH_SET_ADDRESS:
		uhci->rh.devnum = wValue;
		OK (0);

	case RH_GET_DESCRIPTOR:
		switch ((wValue & 0xff00) >> 8) {
		case (0x01):	/* device descriptor */
			len = min_t(unsigned int, leni,
				  min_t(unsigned int,
				      sizeof (root_hub_dev_des), wLength));
			memcpy (data, root_hub_dev_des, len);
			OK (len);
		case (0x02):	/* configuration descriptor */
			len = min_t(unsigned int, leni,
				  min_t(unsigned int,
				      sizeof (root_hub_config_des), wLength));
			memcpy (data, root_hub_config_des, len);
			OK (len);
		case (0x03):	/* string descriptors */
			len = g_psBus->root_hub_string (wValue & 0xff,
			        uhci->io_addr, "UHCI",
				data, wLength);
			if (len > 0) {
				OK(min_t(int, leni, len));
			} else 
				stat = -EPIPE;
		}
		break;

	case RH_GET_DESCRIPTOR | RH_CLASS:
		root_hub_hub_des[2] = uhci->rh.numports;
		len = min_t(unsigned int, leni,
			  min_t(unsigned int, sizeof (root_hub_hub_des), wLength));
		memcpy (data, root_hub_hub_des, len);
		OK (len);

	case RH_GET_CONFIGURATION:
		*(__u8 *) data = 0x01;
		OK (1);

	case RH_SET_CONFIGURATION:
		OK (0);
	default:
		stat = -EPIPE;
	}

	dbg("Root-Hub stat port1: %x port2: %x\n",
	     inw (io_addr + USBPORTSC1), inw (io_addr + USBPORTSC2));

	urb->nActualLength = len;
	urb->nStatus = stat;
	urb->psDevice=NULL;
	if (urb->pComplete)
		urb->pComplete (urb);
	return 0;
}
/*-------------------------------------------------------------------------*/

static int rh_unlink_urb (USB_packet_s *urb)
{
	uhci_t *uhci = urb->psDevice->psBus->pHCPrivate;

	if (uhci->rh.urb==urb) {
		dbg("Root-Hub unlink IRQ\n");
		uhci->rh.send = 0;
	}
	return 0;
}
/*-------------------------------------------------------------------*/

/*
 * Map status to standard result codes
 *
 * <status> is (td->status & 0xFE0000) [a.k.a. uhci_status_bits(td->status)
 * <dir_out> is True for output TDs and False for input TDs.
 */
static int uhci_map_status (int status, int dir_out)
{
	if (!status)
		return 0;
	if (status & TD_CTRL_BITSTUFF)	/* Bitstuff error */
		return -EPROTO;
	if (status & TD_CTRL_CRCTIMEO) {	/* CRC/Timeout */
		if (dir_out)
			return -ETIMEDOUT;
		else
			return -EILSEQ;
	}
	if (status & TD_CTRL_NAK)	/* NAK */
		return -ETIMEDOUT;
	if (status & TD_CTRL_BABBLE)	/* Babble */
		return -EOVERFLOW;
	if (status & TD_CTRL_DBUFERR)	/* Buffer error */
		return -ENOSR;
	if (status & TD_CTRL_STALLED)	/* Stalled */
		return -EPIPE;
	if (status & TD_CTRL_ACTIVE)	/* Active */
		return 0;

	return -EPROTO;
}

/*
 * Only the USB core should call uhci_alloc_dev and uhci_free_dev
 */
static int uhci_alloc_dev (USB_device_s *usb_dev)
{
	return 0;
}

static void uhci_unlink_urbs(uhci_t *s, USB_device_s *usb_dev, int remove_all)
{
	unsigned long flags;
	struct list_head *p;
	struct list_head *p2;
	USB_packet_s *urb;

	spinlock_cli (&s->urb_list_lock, flags);
	p = s->urb_list.prev;	
	while (p != &s->urb_list) {
		p2 = p;
		p = p->prev ;
		urb = list_entry (p2, USB_packet_s, lPacketList);
		dbg("urb: %p, dev %p, %p\n", urb, usb_dev,urb->psDevice);
		
		//urb->transfer_flags |=USB_ASYNC_UNLINK; 
			
		if (remove_all || (usb_dev == urb->psDevice)) {
			spinunlock_restore (&s->urb_list_lock, flags);
			printk("UHCI: forced removing of queued URB %p due to disconnect\n",urb);
			uhci_unlink_urb(urb);
			urb->psDevice = NULL; // avoid further processing of this URB
			spinlock_cli (&s->urb_list_lock, flags);
			p = s->urb_list.prev;	
		}
	}
	spinunlock_restore (&s->urb_list_lock, flags);
}

static int uhci_free_dev (USB_device_s *usb_dev)
{
	uhci_t *s;
	

	if(!usb_dev || !usb_dev->psBus || !usb_dev->psBus->pHCPrivate)
		return -EINVAL;
	
	s=(uhci_t*) usb_dev->psBus->pHCPrivate;	
	uhci_unlink_urbs(s, usb_dev, 0);

	return 0;
}

/*
 * uhci_get_current_frame_number()
 *
 * returns the current frame number for a USB bus/controller.
 */
static int uhci_get_current_frame_number (USB_device_s *usb_dev)
{
	return UHCI_GET_CURRENT_FRAME ((uhci_t*) usb_dev->psBus->pHCPrivate);
}


static void correct_data_toggles(USB_packet_s *urb)
{
	usb_settoggle (urb->psDevice, usb_pipeendpoint (urb->nPipe), usb_pipeout (urb->nPipe), 
		       !usb_gettoggle (urb->psDevice, usb_pipeendpoint (urb->nPipe), usb_pipeout (urb->nPipe)));

	while(urb) {
		urb_priv_t *priv=urb->pHCPrivate;		
		uhci_desc_t *qh = list_entry (priv->desc_list.next, uhci_desc_t, desc_list);
		struct list_head *p = qh->vertical.next;
		uhci_desc_t *td;
		dbg("URB to correct %p\n", urb);
	
		for (; p != &qh->vertical; p = p->next) {
			td = list_entry (p, uhci_desc_t, vertical);
			td->hw.td.info^=cpu_to_le32(1<<TD_TOKEN_TOGGLE);
		}
		urb=priv->next_queued_urb;
	}
}

/* 
 * For IN-control transfers, process_transfer gets a bit more complicated,
 * since there are devices that return less data (eg. strings) than they
 * have announced. This leads to a queue abort due to the short packet,
 * the status stage is not executed. If this happens, the status stage
 * is manually re-executed.
 * mode: PROCESS_TRANSFER_REGULAR: regular (unlink QH)
 *       PROCESS_TRANSFER_DONT_UNLINK: QHs already unlinked (for async unlink_urb)
 */

static int process_transfer (uhci_t *s, USB_packet_s *urb, int mode)
{
	int ret = 0;
	urb_priv_t *urb_priv = urb->pHCPrivate;
	struct list_head *qhl = urb_priv->desc_list.next;
	uhci_desc_t *qh = list_entry (qhl, uhci_desc_t, desc_list);
	struct list_head *p = qh->vertical.next;
	uhci_desc_t *desc= list_entry (urb_priv->desc_list.prev, uhci_desc_t, desc_list);
	uhci_desc_t *last_desc = list_entry (desc->vertical.prev, uhci_desc_t, vertical);
	int data_toggle = usb_gettoggle (urb->psDevice, usb_pipeendpoint (urb->nPipe), usb_pipeout (urb->nPipe));	// save initial data_toggle
	int maxlength; 	// extracted and remapped info from TD
	int actual_length;
	int status = 0;

	dbg("process_transfer: urb %p, urb_priv %p, qh %p last_desc %p\n",urb,urb_priv, qh, last_desc);

	/* if the status phase has been retriggered and the
	   queue is empty or the last status-TD is inactive, the retriggered
	   status stage is completed
	 */

	if (urb_priv->flags && 
	    ((qh->hw.qh.element == cpu_to_le32(UHCI_PTR_TERM)) || !is_td_active(desc)))
		goto transfer_finished;

	urb->nActualLength=0;

	for (; p != &qh->vertical; p = p->next) {
		desc = list_entry (p, uhci_desc_t, vertical);

		if (is_td_active(desc)) {	// do not process active TDs
			if (mode == CLEAN_TRANSFER_DELETION_MARK) // if called from async_unlink
				uhci_clean_transfer(s, urb, qh, CLEAN_TRANSFER_DELETION_MARK);
			return ret;
		}
	
		actual_length = uhci_actual_length(le32_to_cpu(desc->hw.td.status));		// extract transfer parameters from TD
		maxlength = (((le32_to_cpu(desc->hw.td.info) >> 21) & 0x7ff) + 1) & 0x7ff;
		
		status = uhci_map_status (uhci_status_bits (le32_to_cpu(desc->hw.td.status)), usb_pipeout (urb->nPipe));

		if (status == -EPIPE) { 		// see if EP is stalled
			// set up stalled condition
			//printk( "UHCI: Endpoint halted\n" );
			usb_endpoint_halt (urb->psDevice, usb_pipeendpoint (urb->nPipe), usb_pipeout (urb->nPipe));
		}

		if (status && (status != -EPIPE)) {	// if any error occurred stop processing of further TDs
			// only set ret if status returned an error
  is_error:
			ret = status;
			urb->nErrorCount++;
			break;
		}
		else if ((le32_to_cpu(desc->hw.td.info) & 0xff) != USB_PID_SETUP)
			urb->nActualLength += actual_length;

		// got less data than requested
		if ( (actual_length < maxlength)) {
			if (urb->nTransferFlags & USB_DISABLE_SPD) {
				status = -EREMOTEIO;	// treat as real error
				dbg("process_transfer: SPD!!\n");
				break;	// exit after this TD because SP was detected
			}

			// short read during control-IN: re-start status stage
			if ((usb_pipetype (urb->nPipe) == USB_PIPE_CONTROL)) {
				if (uhci_packetid(le32_to_cpu(last_desc->hw.td.info)) == USB_PID_OUT) {
			
					set_qh_element(qh, last_desc->dma_addr);  // re-trigger status stage
					dbg("short packet during control transfer, retrigger status stage @ %p\n",last_desc);
					urb_priv->flags = 1; // mark as short control packet
					return 0;
				}
			}
			// all other cases: short read is OK
			data_toggle = uhci_toggle (le32_to_cpu(desc->hw.td.info));
			break;
		}
		else if (status)
			goto is_error;

		data_toggle = uhci_toggle (le32_to_cpu(desc->hw.td.info));
		queue_dbg("process_transfer: len:%d status:%x mapped:%x toggle:%d\n", actual_length, le32_to_cpu(desc->hw.td.status),status, data_toggle);      

	}

	if (usb_pipetype (urb->nPipe) == USB_PIPE_BULK ) {  /* toggle correction for short bulk transfers (nonqueued/queued) */

		urb_priv_t *priv=(urb_priv_t*)urb->pHCPrivate;
		USB_packet_s *next_queued_urb=priv->next_queued_urb;

		if (next_queued_urb) {
			urb_priv_t *next_priv=(urb_priv_t*)next_queued_urb->pHCPrivate;
			uhci_desc_t *qh = list_entry (next_priv->desc_list.next, uhci_desc_t, desc_list);
			uhci_desc_t *first_td=list_entry (qh->vertical.next, uhci_desc_t, vertical);

			if (data_toggle == uhci_toggle (le32_to_cpu(first_td->hw.td.info))) {
				printk("UHCI: process_transfer(): fixed toggle\n");
				correct_data_toggles(next_queued_urb);
			}						
		}
		else
			usb_settoggle (urb->psDevice, usb_pipeendpoint (urb->nPipe), usb_pipeout (urb->nPipe), !data_toggle);		
	}

 transfer_finished:
	
	uhci_clean_transfer(s, urb, qh, mode);

	urb->nStatus = status;

#ifdef CONFIG_USB_UHCI_HIGH_BANDWIDTH	
	disable_desc_loop(s,urb);
#endif	

	dbg("process_transfer: (end) urb %p, wanted len %d, len %d status %x err %d\n",
		urb,urb->nBufferLength,urb->nActualLength, urb->nStatus, urb->nErrorCount);
	return ret;
}

static int process_interrupt (uhci_t *s, USB_packet_s *urb)
{
	int i, ret = -EINPROGRESS;
	urb_priv_t *urb_priv = urb->pHCPrivate;
	struct list_head *p = urb_priv->desc_list.next;
	uhci_desc_t *desc = list_entry (urb_priv->desc_list.prev, uhci_desc_t, desc_list);

	int actual_length;
	int status = 0;

	//dbg("urb contains interrupt request");

	for (i = 0; p != &urb_priv->desc_list; p = p->next, i++)	// Maybe we allow more than one TD later ;-)
	{
		desc = list_entry (p, uhci_desc_t, desc_list);

		if (is_td_active(desc)) {
			// do not process active TDs
			//dbg("TD ACT Status @%p %08x",desc,le32_to_cpu(desc->hw.td.status));
			break;
		}

		if (!desc->hw.td.status & cpu_to_le32(TD_CTRL_IOC)) {
			// do not process one-shot TDs, no recycling
			break;
		}
		// extract transfer parameters from TD

		actual_length = uhci_actual_length(le32_to_cpu(desc->hw.td.status));
		status = uhci_map_status (uhci_status_bits (le32_to_cpu(desc->hw.td.status)), usb_pipeout (urb->nPipe));

		// see if EP is stalled
		if (status == -EPIPE) {
			// set up stalled condition
			//printk( "UHCI: Endpoint halted\n" );
			usb_endpoint_halt (urb->psDevice, usb_pipeendpoint (urb->nPipe), usb_pipeout (urb->nPipe));
		}

		// if any error occurred: ignore this td, and continue
		if (status != 0) {
			//uhci_show_td (desc);
			urb->nErrorCount++;
			goto recycle;
		}
		else
			urb->nActualLength = actual_length;

	recycle:
		uhci_urb_dma_sync(s, urb, urb->pHCPrivate);
		if (urb->pComplete) {
			//dbg("process_interrupt: calling completion, status %i",status);
			urb->nStatus = status;
			((urb_priv_t*)urb->pHCPrivate)->flags=1; // if unlink_urb is called during completion

			spinunlock(&s->urb_list_lock);
			
			urb->pComplete (urb);
			
			spinlock(&s->urb_list_lock);

			((urb_priv_t*)urb->pHCPrivate)->flags=0;		       			
		}
		
		if ((urb->nStatus != -ECONNABORTED) && (urb->nStatus != ECONNRESET) &&
			    (urb->nStatus != -ENOENT)) {

			urb->nStatus = -EINPROGRESS;

			// Recycle INT-TD if interval!=0, else mark TD as one-shot
			if (urb->nInterval) {
				
				desc->hw.td.info &= cpu_to_le32(~(1 << TD_TOKEN_TOGGLE));
				if (status==0) {
					((urb_priv_t*)urb->pHCPrivate)->started=get_system_time();
					desc->hw.td.info |= cpu_to_le32((usb_gettoggle (urb->psDevice, usb_pipeendpoint (urb->nPipe),
									    usb_pipeout (urb->nPipe)) << TD_TOKEN_TOGGLE));
					usb_dotoggle (urb->psDevice, usb_pipeendpoint (urb->nPipe), usb_pipeout (urb->nPipe));
				} else {
					desc->hw.td.info |= cpu_to_le32((!usb_gettoggle (urb->psDevice, usb_pipeendpoint (urb->nPipe),
									     usb_pipeout (urb->nPipe)) << TD_TOKEN_TOGGLE));
				}
				desc->hw.td.status= cpu_to_le32((urb->nPipe & TD_CTRL_LS) | TD_CTRL_ACTIVE | TD_CTRL_IOC |
					(urb->nTransferFlags & USB_DISABLE_SPD ? 0 : TD_CTRL_SPD) | (3 << 27));
				mb();
			}
			else {
				uhci_unlink_urb_async(s, urb, UNLINK_ASYNC_STORE_URB);
				clr_td_ioc(desc); // inactivate TD
			}
		}
	}

	return ret;
}

// mode: PROCESS_ISO_REGULAR: processing only for done TDs, unlink TDs
// mode: PROCESS_ISO_FORCE: force processing, don't unlink TDs (already unlinked)

static int process_iso (uhci_t *s, USB_packet_s *urb, int mode)
{
	int i;
	int ret = 0;
	urb_priv_t *urb_priv = urb->pHCPrivate;
	struct list_head *p = urb_priv->desc_list.next, *p_tmp;
	uhci_desc_t *desc = list_entry (urb_priv->desc_list.prev, uhci_desc_t, desc_list);

	dbg("urb contains iso request\n");
	if (is_td_active(desc) && mode==PROCESS_ISO_REGULAR)
		return -EXDEV;	// last TD not finished

	urb->nErrorCount = 0;
	urb->nActualLength = 0;
	urb->nStatus = 0;
	dbg("process iso urb %p, %li, %i, %i, %i %08x\n",urb,get_system_time(),UHCI_GET_CURRENT_FRAME(s),
	    urb->nPacketNum,mode,le32_to_cpu(desc->hw.td.status));

	for (i = 0; p != &urb_priv->desc_list;  i++) {
		desc = list_entry (p, uhci_desc_t, desc_list);
		
		//uhci_show_td(desc);
		if (is_td_active(desc)) {
			// means we have completed the last TD, but not the TDs before
			desc->hw.td.status &= cpu_to_le32(~TD_CTRL_ACTIVE);
			dbg("TD still active (%x)- grrr. paranoia!\n", le32_to_cpu(desc->hw.td.status));
			ret = -EXDEV;
			urb->sISOFrame[i].nStatus = ret;
			unlink_td (s, desc, 1);
			// FIXME: immediate deletion may be dangerous
			goto err;
		}

		if (mode == PROCESS_ISO_REGULAR)
			unlink_td (s, desc, 1);

		if (urb->nPacketNum <= i) {
			dbg("urb->number_of_packets (%d)<=(%d)\n", urb->nPacketNum, i);
			ret = -EINVAL;
			goto err;
		}

		urb->sISOFrame[i].nActualLength = uhci_actual_length(le32_to_cpu(desc->hw.td.status));
		urb->sISOFrame[i].nStatus = uhci_map_status (uhci_status_bits (le32_to_cpu(desc->hw.td.status)), usb_pipeout (urb->nPipe));
		urb->nActualLength += urb->sISOFrame[i].nActualLength;

	      err:

		if (urb->sISOFrame[i].nStatus != 0) {
			urb->nErrorCount++;
			urb->nStatus = urb->sISOFrame[i].nStatus;
		}
		dbg("process_iso: %i: len:%d %08x status:%x\n",
		     i, urb->sISOFrame[i].nActualLength, le32_to_cpu(desc->hw.td.status),urb->sISOFrame[i].nStatus);

		p_tmp = p;
		p = p->next;
		list_del (p_tmp);
		delete_desc (s, desc);
	}
	
	dbg("process_iso: exit %i (%d), actual_len %i\n", i, ret,urb->nActualLength);
	return ret;
}


static int process_urb (uhci_t *s, struct list_head *p)
{
	int ret = 0;
	USB_packet_s *urb;

	urb=list_entry (p, USB_packet_s, lPacketList);
	dbg("process_urb: found queued urb: %p", urb);

	switch (usb_pipetype (urb->nPipe)) {
	case USB_PIPE_CONTROL:
		ret = process_transfer (s, urb, CLEAN_TRANSFER_REGULAR);
		break;
	case USB_PIPE_BULK:
		if (!atomic_read( &s->avoid_bulk ))
			ret = process_transfer (s, urb, CLEAN_TRANSFER_REGULAR);
		else
			return 0;
		break;
	case USB_PIPE_ISOCHRONOUS:
		ret = process_iso (s, urb, PROCESS_ISO_REGULAR);
		break;
	case USB_PIPE_INTERRUPT:
		ret = process_interrupt (s, urb);
		break;
	}

	if (urb->nStatus != -EINPROGRESS) {
		urb_priv_t *urb_priv;
		USB_device_s *usb_dev;
		
		usb_dev=urb->psDevice;

		/* Release bandwidth for Interrupt or Iso transfers */
		if (urb->nBandWidth) {
			if (usb_pipetype(urb->nPipe)==USB_PIPE_ISOCHRONOUS)
				g_psBus->release_bandwidth (urb->psDevice, urb, 1);
			else if (usb_pipetype(urb->nPipe)==USB_PIPE_INTERRUPT && urb->nInterval)
				g_psBus->release_bandwidth (urb->psDevice, urb, 0);
		}

		dbg("dequeued urb: %p\n", urb);
		dequeue_urb (s, urb);

		urb_priv = urb->pHCPrivate;

		uhci_urb_dma_unmap(s, urb, urb_priv);

#ifdef DEBUG_SLAB
		kmem_cache_free(urb_priv_kmem, urb_priv);
#else
		kfree (urb_priv);
#endif

		if ((usb_pipetype (urb->nPipe) != USB_PIPE_INTERRUPT)) {  // process_interrupt does completion on its own		
			USB_packet_s *next_urb = urb->psNext;
			int is_ring = 0;
			int contains_killed = 0;
			int loop_count=0;
			
			if (next_urb) {
				// Find out if the URBs are linked to a ring
				while  (next_urb != NULL && next_urb != urb && loop_count < MAX_NEXT_COUNT) {
					if (next_urb->nStatus == -ENOENT) {// killed URBs break ring structure & resubmission
						contains_killed = 1;
						break;
					}	
					next_urb = next_urb->psNext;
					loop_count++;
				}
				
				if (loop_count == MAX_NEXT_COUNT)
					printk("UHCI: process_urb: Too much linked URBs in ring detection!\n");

				if (next_urb == urb)
					is_ring=1;
			}			

			// Submit idle/non-killed URBs linked with urb->next
			// Stop before the current URB				
			
			next_urb = urb->psNext;	
			if (next_urb && !contains_killed) {
				int ret_submit;
				next_urb = urb->psNext;	
				
				loop_count=0;
				while (next_urb != NULL && next_urb != urb && loop_count < MAX_NEXT_COUNT) {
					if (next_urb->nStatus != -EINPROGRESS) {
					
						if (next_urb->nStatus == -ENOENT) 
							break;

						spinunlock(&s->urb_list_lock);

						ret_submit=uhci_submit_urb(next_urb);
						spinlock(&s->urb_list_lock);
						
						if (ret_submit)
							break;						
					}
					loop_count++;
					next_urb = next_urb->psNext;
				}
				if (loop_count == MAX_NEXT_COUNT)
					printk("UHCI: process_urb: Too much linked URBs in resubmission!\n");
			}

			// Completion
			if (urb->pComplete) {
				int was_unlinked = (urb->nStatus == -ENOENT);
				urb->psDevice = NULL;
				spinunlock(&s->urb_list_lock);

				urb->pComplete (urb);

				// Re-submit the URB if ring-linked
				if (is_ring && !was_unlinked && !contains_killed) {
					urb->psDevice=usb_dev;
					uhci_submit_urb (urb);
				}
				spinlock(&s->urb_list_lock);
			}
			atomic_dec( &usb_dev->nRefCount );
			//usb_dec_dev_use (usb_dev);
		}
	}

	return ret;
}

int uhci_interrupt(int irq, void *dev_id, SysCallRegs_s *regs )
{
	uhci_t *s = dev_id;
	unsigned int io_addr = s->io_addr;
	unsigned short status;
	struct list_head *p, *p2;
	int restarts, work_done;
	
	/*
	 * Read the interrupt status, and write it back to clear the
	 * interrupt cause
	 */

	status = inw (io_addr + USBSTS);

	if (!status)		/* shared interrupt, not mine */
		return( 0 );

	dbg("interrupt\n");

	if (status != 1) {
		// Avoid too much error messages at a time
		if ((get_system_time() - s->last_error_time > ERROR_SUPPRESSION_TIME*1000)) {
			dbg("interrupt, status %x, frame# %i\n", status, 
			     UHCI_GET_CURRENT_FRAME(s));
			s->last_error_time = get_system_time();
		}
		
		// remove host controller halted state
		if ((status&0x20) && (s->running)) {
			printk("UHCI: Host controller halted, trying to restart.\n");
			outw (USBCMD_RS | inw(io_addr + USBCMD), io_addr + USBCMD);
		}
		//uhci_show_status (s);
	}
	/*
	 * traverse the list in *reverse* direction, because new entries
	 * may be added at the end.
	 * also, because process_urb may unlink the current urb,
	 * we need to advance the list before
	 * New: check for max. workload and restart count
	 */

	spinlock (&s->urb_list_lock);

	restarts=0;
	work_done=0;

restart:
	s->unlink_urb_done=0;
	p = s->urb_list.prev;	

	while (p != &s->urb_list && (work_done < 1024)) {
		p2 = p;
		p = p->prev;

		process_urb (s, p2);
		
		work_done++;

		if (s->unlink_urb_done) {
			s->unlink_urb_done=0;
			restarts++;
			
			if (restarts<16)	// avoid endless restarts
				goto restart;
			else 
				break;
		}
	}
	if ((get_system_time() - s->timeout_check) > (1000/30)*1000) 
		uhci_check_timeouts(s);

	clean_descs(s, CLEAN_NOT_FORCED);
	uhci_cleanup_unlink(s, CLEAN_NOT_FORCED);
	uhci_switch_timer_int(s);
							
	spinunlock (&s->urb_list_lock);
	
	outw (status, io_addr + USBSTS);

	dbg("uhci_interrupt: done\n");
	
	return( 0 );
}

static void reset_hc (uhci_t *s)
{
	unsigned int io_addr = s->io_addr;

	s->apm_state = 0;
	/* Global reset for 50ms */
	outw (USBCMD_GRESET, io_addr + USBCMD);
	uhci_wait_ms (50);
	outw (0, io_addr + USBCMD);
	uhci_wait_ms (10);
}

static void start_hc (uhci_t *s)
{
	unsigned int io_addr = s->io_addr;
	int timeout = 1000;

	/*
	 * Reset the HC - this will force us to get a
	 * new notification of any already connected
	 * ports due to the virtual disconnect that it
	 * implies.
	 */
	outw (USBCMD_HCRESET, io_addr + USBCMD);

	while (inw (io_addr + USBCMD) & USBCMD_HCRESET) {
		if (!--timeout) {
			printk("UHCI: USBCMD_HCRESET timed out!\n");
			break;
		}
	}

	/* Turn on all interrupts */
	outw (USBINTR_TIMEOUT | USBINTR_RESUME | USBINTR_IOC | USBINTR_SP, io_addr + USBINTR);

	/* Start at frame 0 */
	outw (0, io_addr + USBFRNUM);
	outl ((uint32)s->framelist, io_addr + USBFLBASEADD);

	/* Run and mark it configured with a 64-byte max packet */
	outw (USBCMD_RS | USBCMD_CF | USBCMD_MAXP, io_addr + USBCMD);
	s->apm_state = 1;
	s->running = 1;
}

static int uhci_start_usb (uhci_t *s)
{				/* start it up */
	/* connect the virtual root hub */
	USB_device_s *usb_dev;

	usb_dev = g_psBus->alloc_device (NULL, s->bus);
	if (!usb_dev)
		return -1;

	s->bus->psRootHUB = usb_dev;
	g_psBus->connect (usb_dev);

	if (g_psBus->new_device (usb_dev) != 0) {
		g_psBus->free_device (usb_dev);
		return -1;
	}

	return 0;
}

static int alloc_uhci ( int nDeviceID, PCI_Info_s dev, int irq, unsigned int io_addr, unsigned int io_size)
{
	uhci_t *s;
	USB_bus_driver_s *bus;
	char buf[8], *bufp = buf;

#ifndef __sparc__
	sprintf(buf, "%d", irq);
#else
	bufp = __irq_itoa(irq);
#endif
	printk("USB UHCI at I/O 0x%x, IRQ %s\n",
		io_addr, bufp);

	s = kmalloc (sizeof (uhci_t), MEMF_KERNEL);
	if (!s)
		return -1;

	memset (s, 0, sizeof (uhci_t));
	INIT_LIST_HEAD (&s->free_desc);
	INIT_LIST_HEAD (&s->urb_list);
	INIT_LIST_HEAD (&s->urb_unlinked);
	spinlock_init (&s->urb_list_lock, "urb_list_lock");
	spinlock_init (&s->qh_lock, "qh_lock");
	spinlock_init (&s->td_lock, "td_lock");
	atomic_set(&s->avoid_bulk, 0);
	s->irq = -1;
	s->io_addr = io_addr;
	s->io_size = io_size;
	s->uhci_pci=dev;
	
	bus = g_psBus->alloc_bus();
	if (!bus) {
		kfree (s);
		return -1;
	}

	s->bus = bus;
	bus->AddDevice = uhci_alloc_dev;
	bus->RemoveDevice = uhci_free_dev;
	bus->GetFrameNumber = uhci_get_current_frame_number;
	bus->SubmitPacket = uhci_submit_urb;
	bus->CancelPacket = uhci_unlink_urb;
	bus->pHCPrivate = s;
	

	/* UHCI specs says devices must have 2 ports, but goes on to say */
	/* they may have more but give no way to determine how many they */
	/* have, so default to 2 */
	/* According to the UHCI spec, Bit 7 is always set to 1. So we try */
	/* to use this to our advantage */

	for (s->maxports = 0; s->maxports < (io_size - 0x10) / 2; s->maxports++) {
		unsigned int portstatus;

		portstatus = inw (io_addr + 0x10 + (s->maxports * 2));
		dbg("port %i, adr %x status %x\n", s->maxports,
			io_addr + 0x10 + (s->maxports * 2), portstatus);
		if (!(portstatus & 0x0080))
			break;
	}
	dbg("Detected %d ports\n", s->maxports);

	/* This is experimental so anything less than 2 or greater than 8 is */
	/*  something weird and we'll ignore it */
	if (s->maxports < 2 || s->maxports > 8) {
		printk("UHCI: Port count misdetected, forcing to 2 ports\n");
		s->maxports = 2;
	}
	
	
	s->rh.numports = s->maxports;
	s->loop_usage=0;
	if (init_skel (s)) {
		g_psBus->free_bus (bus);
		kfree(s);
		return -1;
	}

	reset_hc (s);
	g_psBus->add_bus( s->bus );

	start_hc (s);

	if (request_irq (dev.u.h0.nInterruptLine, uhci_interrupt, NULL, SA_SHIRQ, "usb_uhci", s) < 0) {
		printk("UHCI: request_irq %d failed!\n",irq);
		g_psBus->free_bus (bus);
		reset_hc (s);
		cleanup_skel(s);
		kfree(s);
		return -1;
	}

	/* Enable PIRQ */
	g_psPCIBus->write_pci_config( dev.nBus, dev.nDevice, dev.nFunction, USBLEGSUP, 2, USBLEGSUP_DEFAULT );

	s->irq = irq;

	if(uhci_start_usb (s) < 0) {
		return -1;
	}
	
	/* Claim device */
	if( claim_device( nDeviceID, dev.nHandle, "USB UHCI controller", DEVICE_CONTROLLER ) != 0 )
		return( -1 );

	//chain new uhci device into global list
	devs=s;

	return 0;
}


inline uint32 pci_size(uint32 base, uint32 mask)
{
	uint32 size = base & mask;
	return size & ~(size-1);
}

static uint32 get_pci_memory_size(const PCI_Info_s *pcPCIInfo, int nResource)
{
	uint32 nSize, nBase;
	int nBus = pcPCIInfo->nBus;
	int nDev = pcPCIInfo->nDevice;
	int nFnc = pcPCIInfo->nFunction;
	int nOffset = PCI_BASE_REGISTERS + nResource*4;
	nBase = g_psPCIBus->read_pci_config(nBus, nDev, nFnc, nOffset, 4);
	g_psPCIBus->write_pci_config(nBus, nDev, nFnc, nOffset, 4, ~0);
	nSize = g_psPCIBus->read_pci_config(nBus, nDev, nFnc, nOffset, 4);
	g_psPCIBus->write_pci_config(nBus, nDev, nFnc, nOffset, 4, nBase);
	if (!nSize || nSize == 0xffffffff) return 0;
	if (nBase == 0xffffffff) nBase = 0;
	if (!(nSize & PCI_ADDRESS_SPACE)) {
		return pci_size(nSize, PCI_ADDRESS_MEMORY_32_MASK);
	} else {
		return pci_size(nSize, PCI_ADDRESS_IO_MASK & 0xffff);
	}
}

static int uhci_pci_probe (int nDeviceID, PCI_Info_s dev )
{
	int i;

	if (!dev.u.h0.nInterruptLine) {
		printk("UHCI: found UHCI device with no IRQ assigned. check BIOS settings!\n");
		return -ENODEV;
	}
	
	/* Enable busmaster */
	g_psPCIBus->write_pci_config( dev.nBus, dev.nDevice, dev.nFunction, PCI_COMMAND, 2, g_psPCIBus->read_pci_config( dev.nBus, dev.nDevice, dev.nFunction, PCI_COMMAND, 2 )
					| PCI_COMMAND_IO | PCI_COMMAND_MASTER );

	/* Search for the IO base address.. */
	for (i = 0; i < 6; i++) {

		unsigned int io_addr = *( &dev.u.h0.nBase0 + i );
		unsigned int io_size = get_pci_memory_size( &dev, i );
		
		if( !( io_addr & PCI_ADDRESS_SPACE ) ) {
			continue;
		}

		/* disable legacy emulation */
		g_psPCIBus->write_pci_config( dev.nBus, dev.nDevice, dev.nFunction, USBLEGSUP, 2, 0 );

	
		return alloc_uhci( nDeviceID, dev, dev.u.h0.nInterruptLine, io_addr & PCI_ADDRESS_IO_MASK, io_size);
	}
	return -ENODEV;
}


status_t device_init( int nDeviceID )
{
	int i;
	PCI_Info_s pci;
	bool found = false;
	
	/* Get busmanagers */
	g_psPCIBus = get_busmanager( PCI_BUS_NAME, PCI_BUS_VERSION );
	if( g_psPCIBus == NULL )
	{
		return( -1 );
	}
	
	g_psBus = get_busmanager( USB_BUS_NAME, USB_BUS_VERSION );
	if( g_psBus == NULL )
	{
		return( -1 );
	}
	
	/* scan all PCI devices */
    for(i = 0 ; g_psPCIBus->get_pci_info( &pci, i ) == 0 ; ++i) {
    	if( pci.nClassApi == 0 && pci.nClassBase == PCI_SERIAL_BUS && pci.nClassSub == PCI_USB )
    	{
    		if( uhci_pci_probe( nDeviceID, pci ) == 0 )
    			found = true;
        }
        
    }
   	if( !found ) {
   		kerndbg( KERN_DEBUG, "No USB UHCI controller found\n" );
   		disable_device( nDeviceID );
   		return( -1 );
   	}
	return( 0 );
}


status_t device_uninit( int nDeviceID)
{
	return( 0 );
}




































































