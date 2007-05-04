/*
 * Universal Host Controller Interface driver for USB.
 *
 * Maintainer: Johannes Erdfelt <johannes@erdfelt.com>
 *
 * (C) Copyright 1999 Linus Torvalds
 * (C) Copyright 1999-2001 Johannes Erdfelt, johannes@erdfelt.com
 * (C) Copyright 1999 Randy Dunlap
 * (C) Copyright 1999 Georg Acher, acher@in.tum.de
 * (C) Copyright 1999 Deti Fliegl, deti@fliegl.de
 * (C) Copyright 1999 Thomas Sailer, sailer@ife.ee.ethz.ch
 * (C) Copyright 1999 Roman Weissgaerber, weissg@vienna.at
 * (C) Copyright 2000 Yggdrasil Computing, Inc. (port of new PCI interface
 *               support from usb-ohci.c by Adam Richter, adam@yggdrasil.com).
 * (C) Copyright 1999 Gregory P. Smith (from usb-ohci.c)
 *
 * Intel documents this fairly well, and as far as I know there
 * are no royalties or anything like that, but even so there are
 * people who decided that they want to do the same thing in a
 * completely different way.
 *
 * WARNING! The USB documentation is downright evil. Most of it
 * is just crap, written by a committee. You're better off ignoring
 * most of it, the important stuff is:
 *  - the low-level protocol (fairly simple but lots of small details)
 *  - working around the horridness of the rest
 */

#include <atheos/udelay.h>
#include <atheos/isa_io.h>
#include <posix/errno.h>
#include <posix/ioctls.h>
#include <posix/fcntl.h>
#include <posix/termios.h>
#include <posix/signal.h>
#include <macros.h>
#include "bitops.h"
#include "usb_uhci.h"

#define wait_ms(x) snooze(x*1000)


/* If a transfer is still active after this much time, turn off FSBR */
#define IDLE_TIMEOUT	(1000 / 20)	/* 50 ms */

#define MAX_URB_LOOP	2048		/* Maximum number of linked URB's */


int rh_submit_urb(USB_packet_s *urb);
int rh_unlink_urb(USB_packet_s *urb);
int uhci_get_current_frame_number(USB_device_s *dev);
int uhci_unlink_urb(USB_packet_s *urb);
void uhci_unlink_generic(struct uhci *uhci, USB_packet_s *urb);
void uhci_call_completion(USB_packet_s *urb);

int  ports_active(struct uhci *uhci);
void suspend_hc(struct uhci *uhci);
void wakeup_hc(struct uhci *uhci);

/* busmanagers */
PCI_bus_s* g_psPCIBus;
USB_bus_s* g_psUSBBus;

#define mb() 	__asm__ __volatile__ ("lock; addl $0,0(%%esp)": : :"memory")

/*
 * Only the USB core should call uhci_alloc_dev and uhci_free_dev
 */
int uhci_alloc_dev(USB_device_s *dev)
{
	return 0;
}

int uhci_free_dev(USB_device_s *dev)
{
	return 0;
}

inline void uhci_set_next_interrupt(struct uhci *uhci)
{
	unsigned long flags;

	spinlock_cli(&uhci->frame_list_lock, flags);
	set_bit(TD_CTRL_IOC_BIT, &uhci->skel_term_td->status);
	spinunlock_restore(&uhci->frame_list_lock, flags);
}

inline void uhci_clear_next_interrupt(struct uhci *uhci)
{
	unsigned long flags;

	spinlock_cli(&uhci->frame_list_lock, flags);
	clear_bit(TD_CTRL_IOC_BIT, &uhci->skel_term_td->status);
	spinunlock_restore(&uhci->frame_list_lock, flags);
}

inline void uhci_add_complete(USB_packet_s *urb)
{
	struct uhci *uhci = (struct uhci *)urb->psDevice->psBus->pHCPrivate;
	struct urb_priv *urbp = (struct urb_priv *)urb->pHCPrivate;
	unsigned long flags;

	spinlock_cli(&uhci->complete_list_lock, flags);
	list_add(&urbp->complete_list, &uhci->complete_list);
	spinunlock_restore(&uhci->complete_list_lock, flags);
}

struct uhci_td *uhci_alloc_td(struct uhci *uhci, USB_device_s *dev)
{
	void *buffer;
	struct uhci_td *td;
	
	buffer = kmalloc( sizeof( *td ) + 16, MEMF_KERNEL | MEMF_NOBLOCK );
	td = ( struct uhci_td* )( ( (uint32)buffer + 16 ) & ~15 );
	
	td->rbuffer = buffer;
	td->dma_handle = (uint32)td;

	td->link = UHCI_PTR_TERM;
	td->buffer = 0;

	td->frame = -1;
	td->dev = dev;

	INIT_LIST_HEAD(&td->list);
	INIT_LIST_HEAD(&td->fl_list);

	atomic_add( &dev->nRefCount, 1 );

	return td;
}


void inline uhci_fill_td(struct uhci_td *td, uint32 status,
		uint32 info, uint32 buffer)
{
	td->status = status;
	td->info = info;
	td->buffer = buffer;
}

void uhci_insert_td(struct uhci *uhci, struct uhci_td *skeltd, struct uhci_td *td)
{
	unsigned long flags;
	struct uhci_td *ltd;

	spinlock_cli(&uhci->frame_list_lock, flags);

	ltd = list_entry(skeltd->fl_list.prev, struct uhci_td, fl_list);

	td->link = ltd->link;
	mb();
	ltd->link = td->dma_handle;

	list_add_tail(&td->fl_list, &skeltd->fl_list);

	spinunlock_restore(&uhci->frame_list_lock, flags);
}

/*
 * We insert Isochronous transfers directly into the frame list at the
 * beginning
 * The layout looks as follows:
 * frame list pointer -> iso td's (if any) ->
 * periodic interrupt td (if frame 0) -> irq td's -> control qh -> bulk qh
 */
void uhci_insert_td_frame_list(struct uhci *uhci, struct uhci_td *td, unsigned framenum)
{
	unsigned long flags;

	framenum %= UHCI_NUMFRAMES;

	spinlock_cli(&uhci->frame_list_lock, flags);

	td->frame = framenum;

	/* Is there a TD already mapped there? */
	if (uhci->fl->frame_cpu[framenum]) {
		struct uhci_td *ftd, *ltd;

		ftd = uhci->fl->frame_cpu[framenum];
		ltd = list_entry(ftd->fl_list.prev, struct uhci_td, fl_list);

		list_add_tail(&td->fl_list, &ftd->fl_list);

		td->link = ltd->link;
		mb();
		ltd->link = td->dma_handle;
	} else {
		td->link = uhci->fl->frame[framenum];
		mb();
		uhci->fl->frame[framenum] = td->dma_handle;
		uhci->fl->frame_cpu[framenum] = td;
	}

	spinunlock_restore(&uhci->frame_list_lock, flags);
}

void uhci_remove_td(struct uhci *uhci, struct uhci_td *td)
{
	unsigned long flags;

	/* If it's not inserted, don't remove it */
	if (td->frame == -1 && list_empty(&td->fl_list))
		return;

	spinlock_cli(&uhci->frame_list_lock, flags);
	if (td->frame != -1 && uhci->fl->frame_cpu[td->frame] == td) {
		if (list_empty(&td->fl_list)) {
			uhci->fl->frame[td->frame] = td->link;
			uhci->fl->frame_cpu[td->frame] = NULL;
		} else {
			struct uhci_td *ntd;

			ntd = list_entry(td->fl_list.next, struct uhci_td, fl_list);
			uhci->fl->frame[td->frame] = ntd->dma_handle;
			uhci->fl->frame_cpu[td->frame] = ntd;
		}
	} else {
		struct uhci_td *ptd;

		ptd = list_entry(td->fl_list.prev, struct uhci_td, fl_list);
		ptd->link = td->link;
	}

	mb();
	td->link = UHCI_PTR_TERM;

	list_del_init(&td->fl_list);
	td->frame = -1;

	spinunlock_restore(&uhci->frame_list_lock, flags);
}


/*
 * Inserts a td into qh list at the top.
 */
void uhci_insert_tds_in_qh(struct uhci_qh *qh, USB_packet_s *urb, int breadth)
{
	struct list_head *tmp, *head;
	struct urb_priv *urbp = (struct urb_priv *)urb->pHCPrivate;
	struct uhci_td *td, *ptd;

	if (list_empty(&urbp->td_list))
		return;

	head = &urbp->td_list;
	tmp = head->next;

	/* Ordering isn't important here yet since the QH hasn't been */
	/*  inserted into the schedule yet */
	td = list_entry(tmp, struct uhci_td, list);

	/* Add the first TD to the QH element pointer */
	qh->element = td->dma_handle | (breadth ? 0 : UHCI_PTR_DEPTH);

	ptd = td;

	/* Then link the rest of the TD's */
	tmp = tmp->next;
	while (tmp != head) {
		td = list_entry(tmp, struct uhci_td, list);

		tmp = tmp->next;

		ptd->link = td->dma_handle | (breadth ? 0 : UHCI_PTR_DEPTH);

		ptd = td;
	}

	ptd->link = UHCI_PTR_TERM;
}

void uhci_free_td(struct uhci *uhci, struct uhci_td *td)
{
	if (!list_empty(&td->list) || !list_empty(&td->fl_list))
		printk("td is still in URB list!\n");

	if (td->dev) {
		atomic_add( &td->dev->nRefCount, -1 );
	}
	kfree( td->rbuffer );

}

struct uhci_qh *uhci_alloc_qh(struct uhci *uhci, USB_device_s *dev)
{
	struct uhci_qh *qh;
	void *buffer;

	buffer = kmalloc( sizeof( *qh ) + 16, MEMF_KERNEL | MEMF_NOBLOCK );
	qh = ( struct uhci_qh* )( ( (uint32)buffer + 16 ) & ~15 );
	qh->rbuffer = buffer;
	
	qh->dma_handle = (uint32)qh;

	qh->element = UHCI_PTR_TERM;
	qh->link = UHCI_PTR_TERM;

	qh->dev = dev;

	INIT_LIST_HEAD(&qh->list);
	INIT_LIST_HEAD(&qh->remove_list);

	atomic_add( &dev->nRefCount, 1 );

	return qh;
}

void uhci_free_qh(struct uhci *uhci, struct uhci_qh *qh)
{
	if (!list_empty(&qh->list))
		printk("qh list not empty!\n");
	if (!list_empty(&qh->remove_list))
		printk("qh still in remove_list!\n");

	if (qh->dev)
		atomic_add( &qh->dev->nRefCount, -1 );

	kfree( qh->rbuffer );
}


void _uhci_insert_qh(struct uhci *uhci, struct uhci_qh *skelqh, USB_packet_s *urb)
{
	struct urb_priv *urbp = (struct urb_priv *)urb->pHCPrivate;
	struct list_head *head, *tmp;
	struct uhci_qh *lqh;

	/* Grab the last QH */
	lqh = list_entry(skelqh->list.prev, struct uhci_qh, list);

	if (lqh->urbp) {
		head = &lqh->urbp->queue_list;
		tmp = head->next;
		while (head != tmp) {
			struct urb_priv *turbp =
				list_entry(tmp, struct urb_priv, queue_list);

			tmp = tmp->next;

			turbp->qh->link = urbp->qh->dma_handle | UHCI_PTR_QH;
		}
	}

	head = &urbp->queue_list;
	tmp = head->next;
	while (head != tmp) {
		struct urb_priv *turbp =
			list_entry(tmp, struct urb_priv, queue_list);

		tmp = tmp->next;

		turbp->qh->link = lqh->link;
	}

	urbp->qh->link = lqh->link;
	mb();				/* Ordering is important */
	lqh->link = urbp->qh->dma_handle | UHCI_PTR_QH;

	list_add_tail(&urbp->qh->list, &skelqh->list);
}

void uhci_insert_qh(struct uhci *uhci, struct uhci_qh *skelqh, USB_packet_s *urb)
{
	unsigned long flags;

	spinlock_cli(&uhci->frame_list_lock, flags);
	_uhci_insert_qh(uhci, skelqh, urb);
	spinunlock_restore(&uhci->frame_list_lock, flags);
}

void uhci_remove_qh(struct uhci *uhci, USB_packet_s *urb)
{
	struct urb_priv *urbp = (struct urb_priv *)urb->pHCPrivate;
	unsigned long flags;
	struct uhci_qh *qh = urbp->qh, *pqh;

	if (!qh)
		return;

	/* Only go through the hoops if it's actually linked in */
	if (!list_empty(&qh->list)) {
		qh->urbp = NULL;

		spinlock_cli(&uhci->frame_list_lock, flags);

		pqh = list_entry(qh->list.prev, struct uhci_qh, list);

		if (pqh->urbp) {
			struct list_head *head, *tmp;

			head = &pqh->urbp->queue_list;
			tmp = head->next;
			while (head != tmp) {
				struct urb_priv *turbp =
					list_entry(tmp, struct urb_priv, queue_list);

				tmp = tmp->next;

				turbp->qh->link = qh->link;
			}
		}

		pqh->link = qh->link;
		mb();
		qh->element = qh->link = UHCI_PTR_TERM;

		list_del_init(&qh->list);

		spinunlock_restore(&uhci->frame_list_lock, flags);
	}

	spinlock_cli(&uhci->qh_remove_list_lock, flags);

	/* Check to see if the remove list is empty. Set the IOC bit */
	/* to force an interrupt so we can remove the QH */
	if (list_empty(&uhci->qh_remove_list))
		uhci_set_next_interrupt(uhci);

	list_add(&qh->remove_list, &uhci->qh_remove_list);

	spinunlock_restore(&uhci->qh_remove_list_lock, flags);
}


int uhci_fixup_toggle(USB_packet_s *urb, unsigned int toggle)
{
	struct urb_priv *urbp = (struct urb_priv *)urb->pHCPrivate;
	struct list_head *head, *tmp;

	head = &urbp->td_list;
	tmp = head->next;
	while (head != tmp) {
		struct uhci_td *td = list_entry(tmp, struct uhci_td, list);

		tmp = tmp->next;

		if (toggle)
			set_bit(TD_TOKEN_TOGGLE, &td->info);
		else
			clear_bit(TD_TOKEN_TOGGLE, &td->info);

		toggle ^= 1;
	}

	return toggle;
}

/* This function will append one URB's QH to another URB's QH. This is for */
/*  USB_QUEUE_BULK support for bulk transfers and soon implicitily for */
/*  control transfers */
void uhci_append_queued_urb(struct uhci *uhci, USB_packet_s *eurb, USB_packet_s *urb)
{
	struct urb_priv *eurbp, *urbp, *furbp, *lurbp;
	struct list_head *tmp;
	struct uhci_td *lltd;
	unsigned long flags;

	eurbp = eurb->pHCPrivate;
	urbp = urb->pHCPrivate;

	spinlock_cli(&uhci->frame_list_lock, flags);

	/* Find the first URB in the queue */
	if (eurbp->queued) {
		struct list_head *head = &eurbp->queue_list;

		tmp = head->next;
		while (tmp != head) {
			struct urb_priv *turbp =
				list_entry(tmp, struct urb_priv, queue_list);

			if (!turbp->queued)
				break;

			tmp = tmp->next;
		}
	} else
		tmp = &eurbp->queue_list;

	furbp = list_entry(tmp, struct urb_priv, queue_list);
	lurbp = list_entry(furbp->queue_list.prev, struct urb_priv, queue_list);

	lltd = list_entry(lurbp->td_list.prev, struct uhci_td, list);

	usb_settoggle(urb->psDevice, usb_pipeendpoint(urb->nPipe), usb_pipeout(urb->nPipe),
		uhci_fixup_toggle(urb, uhci_toggle(lltd->info) ^ 1));

	/* All qh's in the queue need to link to the next queue */
	urbp->qh->link = eurbp->qh->link;

	mb();			/* Make sure we flush everything */
	/* Only support bulk right now, so no depth */
	lltd->link = urbp->qh->dma_handle | UHCI_PTR_QH;

	list_add_tail(&urbp->queue_list, &furbp->queue_list);

	urbp->queued = 1;

	spinunlock_restore(&uhci->frame_list_lock, flags);
}

void uhci_delete_queued_urb(struct uhci *uhci, USB_packet_s *urb)
{
	struct urb_priv *urbp, *nurbp;
	struct list_head *head, *tmp;
	struct urb_priv *purbp;
	struct uhci_td *pltd;
	unsigned int toggle;
	unsigned long flags;

	urbp = urb->pHCPrivate;

	spinlock_cli(&uhci->frame_list_lock, flags);

	if (list_empty(&urbp->queue_list))
		goto out;

	nurbp = list_entry(urbp->queue_list.next, struct urb_priv, queue_list);

	/* Fix up the toggle for the next URB's */
	if (!urbp->queued)
		/* We set the toggle when we unlink */
		toggle = usb_gettoggle(urb->psDevice, usb_pipeendpoint(urb->nPipe), usb_pipeout(urb->nPipe));
	else {
		/* If we're in the middle of the queue, grab the toggle */
		/*  from the TD previous to us */
		purbp = list_entry(urbp->queue_list.prev, struct urb_priv,
				queue_list);

		pltd = list_entry(purbp->td_list.prev, struct uhci_td, list);

		toggle = uhci_toggle(pltd->info) ^ 1;
	}

	head = &urbp->queue_list;
	tmp = head->next;
	while (head != tmp) {
		struct urb_priv *turbp;

		turbp = list_entry(tmp, struct urb_priv, queue_list);

		tmp = tmp->next;

		if (!turbp->queued)
			break;

		toggle = uhci_fixup_toggle(turbp->urb, toggle);
	}

	usb_settoggle(urb->psDevice, usb_pipeendpoint(urb->nPipe),
		usb_pipeout(urb->nPipe), toggle);

	if (!urbp->queued) {
		nurbp->queued = 0;

		_uhci_insert_qh(uhci, uhci->skel_bulk_qh, nurbp->urb);
	} else {
		/* We're somewhere in the middle (or end). A bit trickier */
		/*  than the head scenario */
		purbp = list_entry(urbp->queue_list.prev, struct urb_priv,
				queue_list);

		pltd = list_entry(purbp->td_list.prev, struct uhci_td, list);
		if (nurbp->queued)
			pltd->link = nurbp->qh->dma_handle | UHCI_PTR_QH;
		else
			/* The next URB happens to be the beginning, so */
			/*  we're the last, end the chain */
			pltd->link = UHCI_PTR_TERM;
	}

	list_del_init(&urbp->queue_list);

out:
	spinunlock_restore(&uhci->frame_list_lock, flags);
}

struct urb_priv *uhci_alloc_urb_priv(struct uhci *uhci, USB_packet_s *urb)
{
	struct urb_priv *urbp;
	
	urbp = kmalloc( sizeof( *urbp ), MEMF_KERNEL | MEMF_NOBLOCK );

	if (!urbp) {
		printk("uhci_alloc_urb_priv: couldn't allocate memory for urb_priv\n");
		return NULL;
	}

	memset((void *)urbp, 0, sizeof(*urbp));

	urbp->inserttime = get_system_time();
	urbp->fsbrtime = get_system_time();
	urbp->urb = urb;
	urbp->dev = urb->psDevice;
	
	INIT_LIST_HEAD(&urbp->td_list);
	INIT_LIST_HEAD(&urbp->queue_list);
	INIT_LIST_HEAD(&urbp->complete_list);

	urb->pHCPrivate = urbp;

	if (urb->psDevice != uhci->rh.dev) {
		if (urb->nBufferLength) {
			urbp->transfer_buffer_dma_handle = (uint32)urb->pBuffer;
			if (!urbp->transfer_buffer_dma_handle)
				return NULL;
		}

		if (usb_pipetype(urb->nPipe) == USB_PIPE_CONTROL && urb->pSetupPacket) {
			urbp->setup_packet_dma_handle = (uint32)urb->pSetupPacket;
			if (!urbp->setup_packet_dma_handle)
				return NULL;
		}
	}

	return urbp;
}


void uhci_add_td_to_urb(USB_packet_s *urb, struct uhci_td *td)
{
	struct urb_priv *urbp = (struct urb_priv *)urb->pHCPrivate;

	td->urb = urb;

	list_add_tail(&td->list, &urbp->td_list);
}

void uhci_remove_td_from_urb(struct uhci_td *td)
{
	if (list_empty(&td->list))
		return;

	list_del_init(&td->list);

	td->urb = NULL;
}

void uhci_destroy_urb_priv(USB_packet_s *urb)
{
	struct list_head *head, *tmp;
	struct urb_priv *urbp;
	struct uhci *uhci;
	unsigned long flags;

	spinlock_cli(&urb->hLock, flags);

	urbp = (struct urb_priv *)urb->pHCPrivate;
	if (!urbp)
		goto out;

	if (!urbp->dev || !urbp->dev->psBus || !urbp->dev->psBus->pHCPrivate) {
		printk("uhci_destroy_urb_priv: urb %p belongs to disconnected device or bus?\n", urb);
		goto out;
	}

	if (!list_empty(&urb->lPacketList))
		printk("uhci_destroy_urb_priv: urb %p still on uhci->urb_list or uhci->remove_list\n", urb);

	if (!list_empty(&urbp->complete_list))
		printk("uhci_destroy_urb_priv: urb %p still on uhci->complete_list\n", urb);

	uhci = urbp->dev->psBus->pHCPrivate;

	head = &urbp->td_list;
	tmp = head->next;
	while (tmp != head) {
		struct uhci_td *td = list_entry(tmp, struct uhci_td, list);

		tmp = tmp->next;

		uhci_remove_td_from_urb(td);
		uhci_remove_td(uhci, td);
		uhci_free_td(uhci, td);
	}


	urb->pHCPrivate = NULL;
	kfree( urbp );
	
out:
	spinunlock_restore(&urb->hLock, flags);
}

void uhci_inc_fsbr(struct uhci *uhci, USB_packet_s *urb)
{
	unsigned long flags;
	struct urb_priv *urbp = (struct urb_priv *)urb->pHCPrivate;

	spinlock_cli(&uhci->frame_list_lock, flags);

	if ((!(urb->nTransferFlags & USB_NO_FSBR)) && !urbp->fsbr) {
		urbp->fsbr = 1;
		if (!uhci->fsbr++)
			uhci->skel_term_qh->link = uhci->skel_hs_control_qh->dma_handle | UHCI_PTR_QH;
	}

	spinunlock_restore(&uhci->frame_list_lock, flags);
}

void uhci_dec_fsbr(struct uhci *uhci, USB_packet_s *urb)
{
	unsigned long flags;
	struct urb_priv *urbp = (struct urb_priv *)urb->pHCPrivate;

	spinlock_cli(&uhci->frame_list_lock, flags);

	if ((!(urb->nTransferFlags & USB_NO_FSBR)) && urbp->fsbr) {
		urbp->fsbr = 0;
		if (!--uhci->fsbr)
			uhci->skel_term_qh->link = UHCI_PTR_TERM;
	}

	spinunlock_restore(&uhci->frame_list_lock, flags);
}


/*
 * Control transfers
 */
int uhci_submit_control(USB_packet_s *urb)
{
	struct urb_priv *urbp = (struct urb_priv *)urb->pHCPrivate;
	struct uhci *uhci = (struct uhci *)urb->psDevice->psBus->pHCPrivate;
	struct uhci_td *td;
	struct uhci_qh *qh;
	unsigned long destination, status;
	int maxsze = usb_maxpacket(urb->psDevice, urb->nPipe, usb_pipeout(urb->nPipe));
	int len = urb->nBufferLength;
	uint32 data = urbp->transfer_buffer_dma_handle;

	/* The "pipe" thing contains the destination in bits 8--18 */
	destination = (urb->nPipe & USB_PIPE_DEVEP_MASK) | USB_PID_SETUP;

	/* 3 errors */
	status = (urb->nPipe & TD_CTRL_LS) | TD_CTRL_ACTIVE | (3 << 27);

	/*
	 * Build the TD for the control request
	 */
	td = uhci_alloc_td(uhci, urb->psDevice);
	if (!td)
		return -ENOMEM;

	uhci_add_td_to_urb(urb, td);
	uhci_fill_td(td, status, destination | (7 << 21),
		urbp->setup_packet_dma_handle);

	/*
	 * If direction is "send", change the frame from SETUP (0x2D)
	 * to OUT (0xE1). Else change it from SETUP to IN (0x69).
	 */
	destination ^= (USB_PID_SETUP ^ usb_packetid(urb->nPipe));

	if (!(urb->nTransferFlags & USB_DISABLE_SPD))
		status |= TD_CTRL_SPD;

	/*
	 * Build the DATA TD's
	 */
	while (len > 0) {
		int pktsze = len;

		if (pktsze > maxsze)
			pktsze = maxsze;

		td = uhci_alloc_td(uhci, urb->psDevice);
		if (!td)
			return -ENOMEM;

		/* Alternate Data0/1 (start with Data1) */
		destination ^= 1 << TD_TOKEN_TOGGLE;
	
		uhci_add_td_to_urb(urb, td);
		uhci_fill_td(td, status, destination | ((pktsze - 1) << 21),
			data);

		data += pktsze;
		len -= pktsze;
	}

	/*
	 * Build the final TD for control status 
	 */
	td = uhci_alloc_td(uhci, urb->psDevice);
	if (!td)
		return -ENOMEM;

	/*
	 * It's IN if the pipe is an output pipe or we're not expecting
	 * data back.
	 */
	destination &= ~TD_PID;
	if (usb_pipeout(urb->nPipe) || !urb->nBufferLength)
		destination |= USB_PID_IN;
	else
		destination |= USB_PID_OUT;

	destination |= 1 << TD_TOKEN_TOGGLE;		/* End in Data1 */

	status &= ~TD_CTRL_SPD;

	uhci_add_td_to_urb(urb, td);
	uhci_fill_td(td, status | TD_CTRL_IOC,
		destination | (UHCI_NULL_DATA_SIZE << 21), 0);

	qh = uhci_alloc_qh(uhci, urb->psDevice);
	if (!qh)
		return -ENOMEM;

	urbp->qh = qh;
	qh->urbp = urbp;

	/* Low speed or small transfers gets a different queue and treatment */
	if (urb->nPipe & TD_CTRL_LS) {
		uhci_insert_tds_in_qh(qh, urb, 0);
		uhci_insert_qh(uhci, uhci->skel_ls_control_qh, urb);
	} else {
		uhci_insert_tds_in_qh(qh, urb, 1);
		uhci_insert_qh(uhci, uhci->skel_hs_control_qh, urb);
		uhci_inc_fsbr(uhci, urb);
	}

	return -EINPROGRESS;
}


/*
 * Map status to standard result codes
 *
 * <status> is (td->status & 0xFE0000) [a.k.a. uhci_status_bits(td->status)]
 * <dir_out> is True for output TDs and False for input TDs.
 */
int uhci_map_status(int status, int dir_out)
{
	if (!status)
		return 0;
	if (status & TD_CTRL_BITSTUFF)			/* Bitstuff error */
		return -EPROTO;
	if (status & TD_CTRL_CRCTIMEO) {		/* CRC/Timeout */
		if (dir_out)
			return -ETIMEDOUT;
		else
			return -EILSEQ;
	}
	if (status & TD_CTRL_NAK)			/* NAK */
		return -ETIMEDOUT;
	if (status & TD_CTRL_BABBLE)			/* Babble */
		return -EOVERFLOW;
	if (status & TD_CTRL_DBUFERR)			/* Buffer error */
		return -ENOSR;
	if (status & TD_CTRL_STALLED)			/* Stalled */
		return -EPIPE;
	if (status & TD_CTRL_ACTIVE)			/* Active */
		return 0;

	return -EINVAL;
}

static int debug = 5;
static int errbuf = 0;
static int usb_control_retrigger_status(USB_packet_s *urb);

int uhci_result_control(USB_packet_s *urb)
{
	struct list_head *tmp, *head;
	struct urb_priv *urbp = urb->pHCPrivate;
	struct uhci_td *td;
	unsigned int status;
	int ret = 0;

	if (list_empty(&urbp->td_list))
		return -EINVAL;

	head = &urbp->td_list;

	if (urbp->short_control_packet) {
		tmp = head->prev;
		goto status_phase;
	}

	tmp = head->next;
	td = list_entry(tmp, struct uhci_td, list);

	/* The first TD is the SETUP phase, check the status, but skip */
	/*  the count */
	status = uhci_status_bits(td->status);
	if (status & TD_CTRL_ACTIVE)
		return -EINPROGRESS;

	if (status)
		goto td_error;

	urb->nActualLength = 0;

	/* The rest of the TD's (but the last) are data */
	tmp = tmp->next;
	while (tmp != head && tmp->next != head) {
		td = list_entry(tmp, struct uhci_td, list);

		tmp = tmp->next;

		if (urbp->fsbr_timeout && (td->status & TD_CTRL_IOC) &&
		    !(td->status & TD_CTRL_ACTIVE)) {
			uhci_inc_fsbr(urb->psDevice->psBus->pHCPrivate, urb);
			urbp->fsbr_timeout = 0;
			urbp->fsbrtime = get_system_time();
			clear_bit(TD_CTRL_IOC_BIT, &td->status);
		}

		status = uhci_status_bits(td->status);
		if (status & TD_CTRL_ACTIVE)
			return -EINPROGRESS;

		urb->nActualLength += uhci_actual_length(td->status);

		if (status)
			goto td_error;

		/* Check to see if we received a short packet */
		if (uhci_actual_length(td->status) < uhci_expected_length(td->info)) {
			if (urb->nTransferFlags & USB_DISABLE_SPD) {
				ret = -EREMOTEIO;
				goto err;
			}

			if (uhci_packetid(td->info) == USB_PID_IN)
				return usb_control_retrigger_status(urb);
			else
				return 0;
		}
	}

status_phase:
	td = list_entry(tmp, struct uhci_td, list);

	/* Control status phase */
	status = uhci_status_bits(td->status);

	if (status & TD_CTRL_ACTIVE)
		return -EINPROGRESS;

	if (status)
		goto td_error;

	return 0;

td_error:
	ret = uhci_map_status(status, uhci_packetout(td->info));
	if (ret == -EPIPE)
		/* endpoint has stalled - mark it halted */
		usb_endpoint_halt(urb->psDevice, uhci_endpoint(td->info),
	    			uhci_packetout(td->info));

err:
	if ((debug == 1 && ret != -EPIPE) || debug > 1) {
		/* Some debugging code */
		printk("uhci_result_control() failed with status %x\n", status);

		if (errbuf) {
			/* Print the chain for debugging purposes */
//			uhci_show_qh(urbp->qh, errbuf, ERRBUF_LEN, 0);

			//lprintk(errbuf);
		}
	}

	return ret;
}


int usb_control_retrigger_status(USB_packet_s *urb)
{
	struct list_head *tmp, *head;
	struct urb_priv *urbp = (struct urb_priv *)urb->pHCPrivate;
	struct uhci *uhci = urb->psDevice->psBus->pHCPrivate;

	urbp->short_control_packet = 1;

	/* Create a new QH to avoid pointer overwriting problems */
	uhci_remove_qh(uhci, urb);

	/* Delete all of the TD's except for the status TD at the end */
	head = &urbp->td_list;
	tmp = head->next;
	while (tmp != head && tmp->next != head) {
		struct uhci_td *td = list_entry(tmp, struct uhci_td, list);

		tmp = tmp->next;

		uhci_remove_td_from_urb(td);
		uhci_remove_td(uhci, td);
		uhci_free_td(uhci, td);
	}

	urbp->qh = uhci_alloc_qh(uhci, urb->psDevice);
	if (!urbp->qh) {
		printk("unable to allocate new QH for control retrigger\n");
		return -ENOMEM;
	}

	urbp->qh->urbp = urbp;

	/* One TD, who cares about Breadth first? */
	uhci_insert_tds_in_qh(urbp->qh, urb, 0);

	/* Low speed or small transfers gets a different queue and treatment */
	if (urb->nPipe & TD_CTRL_LS)
		uhci_insert_qh(uhci, uhci->skel_ls_control_qh, urb);
	else
		uhci_insert_qh(uhci, uhci->skel_hs_control_qh, urb);

	return -EINPROGRESS;
}


/*
 * Interrupt transfers
 */
int uhci_submit_interrupt(USB_packet_s *urb)
{
	struct uhci_td *td;
	unsigned long destination, status;
	struct uhci *uhci = (struct uhci *)urb->psDevice->psBus->pHCPrivate;
	struct urb_priv *urbp = (struct urb_priv *)urb->pHCPrivate;

	if (urb->nBufferLength > usb_maxpacket(urb->psDevice, urb->nPipe, usb_pipeout(urb->nPipe)))
		return -EINVAL;

	/* The "pipe" thing contains the destination in bits 8--18 */
	destination = (urb->nPipe & USB_PIPE_DEVEP_MASK) | usb_packetid(urb->nPipe);

	status = (urb->nPipe & TD_CTRL_LS) | TD_CTRL_ACTIVE | TD_CTRL_IOC;

	td = uhci_alloc_td(uhci, urb->psDevice);
	if (!td)
		return -ENOMEM;

	destination |= (usb_gettoggle(urb->psDevice, usb_pipeendpoint(urb->nPipe), usb_pipeout(urb->nPipe)) << TD_TOKEN_TOGGLE);
	destination |= ((urb->nBufferLength - 1) << 21);

	usb_dotoggle(urb->psDevice, usb_pipeendpoint(urb->nPipe), usb_pipeout(urb->nPipe));

	uhci_add_td_to_urb(urb, td);
	uhci_fill_td(td, status, destination, urbp->transfer_buffer_dma_handle);

	uhci_insert_td(uhci, uhci->skeltd[__interval_to_skel(urb->nInterval)], td);

	return -EINPROGRESS;
}

int uhci_result_interrupt(USB_packet_s *urb)
{
	struct list_head *tmp, *head;
	struct urb_priv *urbp = urb->pHCPrivate;
	struct uhci_td *td;
	unsigned int status;
	int ret = 0;

	urb->nActualLength = 0;

	head = &urbp->td_list;
	tmp = head->next;
	while (tmp != head) {
		td = list_entry(tmp, struct uhci_td, list);

		tmp = tmp->next;

		if (urbp->fsbr_timeout && (td->status & TD_CTRL_IOC) &&
		    !(td->status & TD_CTRL_ACTIVE)) {
			uhci_inc_fsbr(urb->psDevice->psBus->pHCPrivate, urb);
			urbp->fsbr_timeout = 0;
			urbp->fsbrtime = get_system_time();
			clear_bit(TD_CTRL_IOC_BIT, &td->status);
		}

		status = uhci_status_bits(td->status);
		if (status & TD_CTRL_ACTIVE)
			return -EINPROGRESS;

		urb->nActualLength += uhci_actual_length(td->status);

		if (status)
			goto td_error;

		if (uhci_actual_length(td->status) < uhci_expected_length(td->info)) {
			if (urb->nTransferFlags & USB_DISABLE_SPD) {
				ret = -EREMOTEIO;
				goto err;
			} else
				return 0;
		}
	}

	return 0;

td_error:
	ret = uhci_map_status(status, uhci_packetout(td->info));
	if (ret == -EPIPE)
		/* endpoint has stalled - mark it halted */
		usb_endpoint_halt(urb->psDevice, uhci_endpoint(td->info),
	    			uhci_packetout(td->info));

err:
	if ((debug == 1 && ret != -EPIPE) || debug > 1) {
		/* Some debugging code */
		printk("uhci_result_interrupt/bulk() failed with status %x\n",
			status);
	}

	return ret;
}


void uhci_reset_interrupt(USB_packet_s *urb)
{
	struct uhci *uhci = (struct uhci *)urb->psDevice->psBus->pHCPrivate;
	struct urb_priv *urbp = (struct urb_priv *)urb->pHCPrivate;
	struct uhci_td *td;
	unsigned long flags;

	spinlock_cli(&urb->hLock, flags);

	/* Root hub is special */
	if (urb->psDevice == uhci->rh.dev)
		goto out;

	td = list_entry(urbp->td_list.next, struct uhci_td, list);

	td->status = (td->status & 0x2F000000) | TD_CTRL_ACTIVE | TD_CTRL_IOC;
	td->info &= ~(1 << TD_TOKEN_TOGGLE);
	td->info |= (usb_gettoggle(urb->psDevice, usb_pipeendpoint(urb->nPipe), usb_pipeout(urb->nPipe)) << TD_TOKEN_TOGGLE);
	usb_dotoggle(urb->psDevice, usb_pipeendpoint(urb->nPipe), usb_pipeout(urb->nPipe));

out:
	urb->nStatus = -EINPROGRESS;

	spinunlock_restore(&urb->hLock, flags);
}

/*
 * Bulk transfers
 */
int uhci_submit_bulk(USB_packet_s *urb, USB_packet_s *eurb)
{
	struct uhci_td *td;
	struct uhci_qh *qh;
	unsigned long destination, status;
	struct uhci *uhci = (struct uhci *)urb->psDevice->psBus->pHCPrivate;
	int maxsze = usb_maxpacket(urb->psDevice, urb->nPipe, usb_pipeout(urb->nPipe));
	int len = urb->nBufferLength;
	struct urb_priv *urbp = (struct urb_priv *)urb->pHCPrivate;
	uint32 data = urbp->transfer_buffer_dma_handle;

	if (len < 0)
		return -EINVAL;

	/* Can't have low speed bulk transfers */
	if (urb->nPipe & TD_CTRL_LS)
		return -EINVAL;

	/* The "pipe" thing contains the destination in bits 8--18 */
	destination = (urb->nPipe & USB_PIPE_DEVEP_MASK) | usb_packetid(urb->nPipe);

	/* 3 errors */
	status = TD_CTRL_ACTIVE | (3 << TD_CTRL_C_ERR_SHIFT);

	if (!(urb->nTransferFlags & USB_DISABLE_SPD))
		status |= TD_CTRL_SPD;

	/*
	 * Build the DATA TD's
	 */
	do {	/* Allow zero length packets */
		int pktsze = len;

		if (pktsze > maxsze)
			pktsze = maxsze;

		td = uhci_alloc_td(uhci, urb->psDevice);
		if (!td)
			return -ENOMEM;

		uhci_add_td_to_urb(urb, td);
		uhci_fill_td(td, status, destination |
			(((pktsze - 1) & UHCI_NULL_DATA_SIZE) << 21) |
			(usb_gettoggle(urb->psDevice, usb_pipeendpoint(urb->nPipe),
			 usb_pipeout(urb->nPipe)) << TD_TOKEN_TOGGLE),
			data);

		data += pktsze;
		len -= pktsze;

		usb_dotoggle(urb->psDevice, usb_pipeendpoint(urb->nPipe),
			usb_pipeout(urb->nPipe));
	} while (len > 0);

	if (usb_pipeout(urb->nPipe) && (urb->nTransferFlags & USB_ZERO_PACKET) &&
	   urb->nBufferLength) {
		td = uhci_alloc_td(uhci, urb->psDevice);
		if (!td)
			return -ENOMEM;

		uhci_add_td_to_urb(urb, td);
		uhci_fill_td(td, status, destination | UHCI_NULL_DATA_SIZE |
			(usb_gettoggle(urb->psDevice, usb_pipeendpoint(urb->nPipe),
			 usb_pipeout(urb->nPipe)) << TD_TOKEN_TOGGLE),
			data);

		usb_dotoggle(urb->psDevice, usb_pipeendpoint(urb->nPipe),
			usb_pipeout(urb->nPipe));
	}

	/* Set the flag on the last packet */
	td->status |= TD_CTRL_IOC;

	qh = uhci_alloc_qh(uhci, urb->psDevice);
	if (!qh)
		return -ENOMEM;

	urbp->qh = qh;
	qh->urbp = urbp;

	/* Always assume breadth first */
	uhci_insert_tds_in_qh(qh, urb, 1);

	if (urb->nTransferFlags & USB_QUEUE_BULK && eurb)
		uhci_append_queued_urb(uhci, eurb, urb);
	else
		uhci_insert_qh(uhci, uhci->skel_bulk_qh, urb);

	uhci_inc_fsbr(uhci, urb);

	return -EINPROGRESS;
}

/* We can use the result interrupt since they're identical */
#define uhci_result_bulk uhci_result_interrupt

/*
 * Isochronous transfers
 */
int isochronous_find_limits(USB_packet_s *urb, unsigned int *start, unsigned int *end)
{
	USB_packet_s *last_urb = NULL;
	struct uhci *uhci = (struct uhci *)urb->psDevice->psBus->pHCPrivate;
	struct list_head *tmp, *head;
	int ret = 0;
	unsigned long flags;

	spinlock_cli(&uhci->urb_list_lock, flags);
	head = &uhci->urb_list;
	tmp = head->next;
	while (tmp != head) {
		USB_packet_s *u = list_entry(tmp, USB_packet_s, lPacketList);

		tmp = tmp->next;

		/* look for pending URB's with identical pipe handle */
		if ((urb->nPipe == u->nPipe) && (urb->psDevice == u->psDevice) &&
		    (u->nStatus == -EINPROGRESS) && (u != urb)) {
			if (!last_urb)
				*start = u->nStartFrame;
			last_urb = u;
		}
	}

	if (last_urb) {
		*end = (last_urb->nStartFrame + last_urb->nPacketNum) & 1023;
		ret = 0;
	} else
		ret = -1;	/* no previous urb found */

	spinunlock_restore(&uhci->urb_list_lock, flags);

	return ret;
}

int isochronous_find_start(USB_packet_s *urb)
{
	int limits;
	unsigned int start = 0, end = 0;

	if (urb->nPacketNum > 900)	/* 900? Why? */
		return -EFBIG;

	limits = isochronous_find_limits(urb, &start, &end);

	if (urb->nTransferFlags & USB_ISO_ASAP) {
		if (limits) {
			int curframe;

			curframe = uhci_get_current_frame_number(urb->psDevice) % UHCI_NUMFRAMES;
			urb->nStartFrame = (curframe + 10) % UHCI_NUMFRAMES;
		} else
			urb->nStartFrame = end;
	} else {
		urb->nStartFrame %= UHCI_NUMFRAMES;
		/* FIXME: Sanity check */
	}

	return 0;
}

/*
 * Isochronous transfers
 */
int uhci_submit_isochronous(USB_packet_s *urb)
{
	struct uhci_td *td;
	struct uhci *uhci = (struct uhci *)urb->psDevice->psBus->pHCPrivate;
	int i, ret, framenum;
	int status, destination;
	struct urb_priv *urbp = (struct urb_priv *)urb->pHCPrivate;

	status = TD_CTRL_ACTIVE | TD_CTRL_IOS;
	destination = (urb->nPipe & USB_PIPE_DEVEP_MASK) | usb_packetid(urb->nPipe);

	ret = isochronous_find_start(urb);
	if (ret)
		return ret;

	framenum = urb->nStartFrame;
	for (i = 0; i < urb->nPacketNum; i++, framenum++) {
		
		if (!urb->sISOFrame[i].nLength)
			continue;

		td = uhci_alloc_td(uhci, urb->psDevice);
		if (!td)
			return -ENOMEM;

		uhci_add_td_to_urb(urb, td);
		uhci_fill_td(td, status, destination | ((urb->sISOFrame[i].nLength - 1) << 21),
			urbp->transfer_buffer_dma_handle + urb->sISOFrame[i].nOffset);

		if (i + 1 >= urb->nPacketNum)
			td->status |= TD_CTRL_IOC;

		uhci_insert_td_frame_list(uhci, td, framenum);
		
	}

	return -EINPROGRESS;
}

int uhci_result_isochronous(USB_packet_s *urb)
{
	struct list_head *tmp, *head;
	struct urb_priv *urbp = (struct urb_priv *)urb->pHCPrivate;
	int status;
	int i, ret = 0;

	urb->nActualLength = 0;

	i = 0;
	head = &urbp->td_list;
	tmp = head->next;
	while (tmp != head) {
		struct uhci_td *td = list_entry(tmp, struct uhci_td, list);
		int actlength;

		tmp = tmp->next;

		if (td->status & TD_CTRL_ACTIVE)
			return -EINPROGRESS;

		actlength = uhci_actual_length(td->status);
		urb->sISOFrame[i].nActualLength = actlength;
		urb->nActualLength += actlength;

		status = uhci_map_status(uhci_status_bits(td->status), usb_pipeout(urb->nPipe));
		urb->sISOFrame[i].nStatus = status;
		if (status) {
			urb->nErrorCount++;
			ret = status;
		}

		i++;
	}
	

	return ret;
}


USB_packet_s *uhci_find_urb_ep(struct uhci *uhci, USB_packet_s *urb)
{
	struct list_head *tmp, *head;
	unsigned long flags;
	USB_packet_s *u = NULL;

	/* We don't match Isoc transfers since they are special */
	if (usb_pipeisoc(urb->nPipe))
		return NULL;

	spinlock_cli(&uhci->urb_list_lock, flags);
	head = &uhci->urb_list;
	tmp = head->next;
	while (tmp != head) {
		u = list_entry(tmp, USB_packet_s, lPacketList);

		tmp = tmp->next;

		if (u->psDevice == urb->psDevice && u->nPipe == urb->nPipe &&
		    u->nStatus == -EINPROGRESS)
			goto out;
	}
	u = NULL;

out:
	spinunlock_restore(&uhci->urb_list_lock, flags);

	return u;
}

int uhci_submit_urb(USB_packet_s *urb)
{
	int ret = -EINVAL;
	struct uhci *uhci;
	unsigned long flags;
	USB_packet_s *eurb;
	int bustime;

	if (!urb)
		return -EINVAL;

	if (!urb->psDevice || !urb->psDevice->psBus || !urb->psDevice->psBus->pHCPrivate) {
		printk("uhci_submit_urb: urb %p belongs to disconnected device or bus?\n", urb);
		return -ENODEV;
	}

	uhci = (struct uhci *)urb->psDevice->psBus->pHCPrivate;

	INIT_LIST_HEAD(&urb->lPacketList);
	atomic_add( &urb->psDevice->nRefCount, 1 );

	spinlock_cli(&urb->hLock, flags);

	if (urb->nStatus == -EINPROGRESS || urb->nStatus == -ECONNRESET ||
	    urb->nStatus == -ECONNABORTED) {
		printk("uhci_submit_urb: urb not available to submit (status = %d)\n", urb->nStatus);
		/* Since we can have problems on the out path */
		spinunlock_restore(&urb->hLock, flags);
		atomic_add( &urb->psDevice->nRefCount, -1 );

		return ret;
	}

	if (!uhci_alloc_urb_priv(uhci, urb)) {
		ret = -ENOMEM;

		goto out;
	}

	eurb = uhci_find_urb_ep(uhci, urb);
	if (eurb && !(urb->nTransferFlags & USB_QUEUE_BULK)) {
		ret = -ENXIO;

		goto out;
	}

	/* Short circuit the virtual root hub */
	if (urb->psDevice == uhci->rh.dev) {
		ret = rh_submit_urb(urb);

		goto out;
	}

	switch (usb_pipetype(urb->nPipe)) {
	case USB_PIPE_CONTROL:
		ret = uhci_submit_control(urb);
		break;
	case USB_PIPE_INTERRUPT:
		if (urb->nBandWidth == 0) {	/* not yet checked/allocated */
			bustime = g_psUSBBus->check_bandwidth(urb->psDevice, urb);
			if (bustime < 0)
				ret = bustime;
			else {
				ret = uhci_submit_interrupt(urb);
				if (ret == -EINPROGRESS)
					g_psUSBBus->claim_bandwidth(urb->psDevice, urb, bustime, 0);
			}
		} else		/* bandwidth is already set */
			ret = uhci_submit_interrupt(urb);
		break;
	case USB_PIPE_BULK:
		ret = uhci_submit_bulk(urb, eurb);
		break;
	case USB_PIPE_ISOCHRONOUS:
		if (urb->nBandWidth == 0) {	/* not yet checked/allocated */
			if (urb->nPacketNum <= 0) {
				ret = -EINVAL;
				break;
			}
			bustime = g_psUSBBus->check_bandwidth(urb->psDevice, urb);
			if (bustime < 0) {
				ret = bustime;
				break;
			}

			ret = uhci_submit_isochronous(urb);
			if (ret == -EINPROGRESS)
				g_psUSBBus->claim_bandwidth(urb->psDevice, urb, bustime, 1);
		} else		/* bandwidth is already set */
			ret = uhci_submit_isochronous(urb);
		break;
	}

out:
	urb->nStatus = ret;

	spinunlock_restore(&urb->hLock, flags);

	if (ret == -EINPROGRESS) {
		spinlock_cli(&uhci->urb_list_lock, flags);
		/* We use _tail to make find_urb_ep more efficient */
		list_add_tail(&urb->lPacketList, &uhci->urb_list);
		spinunlock_restore(&uhci->urb_list_lock, flags);

		return 0;
	}

	uhci_unlink_generic(uhci, urb);
	uhci_call_completion(urb);

	return ret;
}

/*
 * Return the result of a transfer
 *
 * Must be called with urb_list_lock acquired
 */
void uhci_transfer_result(struct uhci *uhci, USB_packet_s *urb)
{
	int ret = -EINVAL;
	unsigned long flags;
	struct urb_priv *urbp;

	/* The root hub is special */
	if (urb->psDevice == uhci->rh.dev)
		return;

	spinlock_cli(&urb->hLock, flags);

	urbp = (struct urb_priv *)urb->pHCPrivate;

	if (urb->nStatus != -EINPROGRESS) {
		printk("uhci_transfer_result: called for URB %p not in flight?\n", urb);
		spinunlock_restore(&urb->hLock, flags);
		return;
	}

	switch (usb_pipetype(urb->nPipe)) {
	case USB_PIPE_CONTROL:
		ret = uhci_result_control(urb);
		break;
	case USB_PIPE_INTERRUPT:
		ret = uhci_result_interrupt(urb);
		break;
	case USB_PIPE_BULK:
		ret = uhci_result_bulk(urb);
		break;
	case USB_PIPE_ISOCHRONOUS:
		ret = uhci_result_isochronous(urb);
		break;
	}

	urbp->status = ret;

	spinunlock_restore(&urb->hLock, flags);

	if (ret == -EINPROGRESS)
		return;

	switch (usb_pipetype(urb->nPipe)) {
	case USB_PIPE_CONTROL:
	case USB_PIPE_BULK:
	case USB_PIPE_ISOCHRONOUS:
		/* Release bandwidth for Interrupt or Isoc. transfers */
		/* Spinlock needed ? */
		if (urb->nBandWidth)
			g_psUSBBus->release_bandwidth(urb->psDevice, urb, 1);
		uhci_unlink_generic(uhci, urb);
		break;
	case USB_PIPE_INTERRUPT:
		/* Interrupts are an exception */
		if (urb->nInterval) {
			uhci_add_complete(urb);
			return;		/* <-- note return */
		}

		/* Release bandwidth for Interrupt or Isoc. transfers */
		/* Spinlock needed ? */
		if (urb->nBandWidth)
			g_psUSBBus->release_bandwidth(urb->psDevice, urb, 0);
		uhci_unlink_generic(uhci, urb);
		break;
	default:
		printk("uhci_transfer_result: unknown pipe type %d for urb %p\n",
			usb_pipetype(urb->nPipe), urb);
	}

	list_del_init(&urb->lPacketList);

	uhci_add_complete(urb);
}

void uhci_unlink_generic(struct uhci *uhci, USB_packet_s *urb)
{
	struct list_head *head, *tmp;
	struct urb_priv *urbp = urb->pHCPrivate;

	/* We can get called when urbp allocation fails, so check */
	if (!urbp)
		return;

	uhci_dec_fsbr(uhci, urb);	/* Safe since it checks */

	head = &urbp->td_list;
	tmp = head->next;
	while (tmp != head) {
		struct uhci_td *td = list_entry(tmp, struct uhci_td, list);

		tmp = tmp->next;

		/* Control and Isochronous ignore the toggle, so this */
		/* is safe for all types */
		if ((!(td->status & TD_CTRL_ACTIVE) && 
		    ((uhci_actual_length(td->status) < uhci_expected_length(td->info)) ||
		    tmp == head))) {
			usb_settoggle(urb->psDevice, uhci_endpoint(td->info),
				uhci_packetout(td->info),
				uhci_toggle(td->info) ^ 1);
		}
	}

	uhci_delete_queued_urb(uhci, urb);

	/* The interrupt loop will reclaim the QH's */
	uhci_remove_qh(uhci, urb);
}

int uhci_unlink_urb(USB_packet_s* urb)
{
	struct uhci *uhci;
	unsigned long flags;
	struct urb_priv *urbp = urb->pHCPrivate;

	if (!urb)
		return -EINVAL;

	if (!urb->psDevice || !urb->psDevice->psBus || !urb->psDevice->psBus->pHCPrivate)
		return -ENODEV;

	uhci = (struct uhci *)urb->psDevice->psBus->pHCPrivate;

	/* Release bandwidth for Interrupt or Isoc. transfers */
	/* Spinlock needed ? */
	if (urb->nBandWidth) {
		switch (usb_pipetype(urb->nPipe)) {
		case USB_PIPE_INTERRUPT:
			g_psUSBBus->release_bandwidth(urb->psDevice, urb, 0);
			break;
		case USB_PIPE_ISOCHRONOUS:
			g_psUSBBus->release_bandwidth(urb->psDevice, urb, 1);
			break;
		default:
			break;
		}
	}

	if (urb->nStatus != -EINPROGRESS)
		return 0;

	spinlock_cli(&uhci->urb_list_lock, flags);
	list_del_init(&urb->lPacketList);
	spinunlock_restore(&uhci->urb_list_lock, flags);

	uhci_unlink_generic(uhci, urb);

	/* Short circuit the virtual root hub */
	if (urb->psDevice == uhci->rh.dev) {
		rh_unlink_urb(urb);
		uhci_call_completion(urb);
	} else {
		if (urb->nTransferFlags & USB_ASYNC_UNLINK) {
			/* urb_list is available now since we called */
			/*  uhci_unlink_generic already */

			urbp->status = urb->nStatus = -ECONNABORTED;

			spinlock_cli(&uhci->urb_remove_list_lock, flags);

			/* Check to see if the remove list is empty */
			if (list_empty(&uhci->urb_remove_list))
				uhci_set_next_interrupt(uhci);
			
			list_add(&urb->lPacketList, &uhci->urb_remove_list);

			spinunlock_restore(&uhci->urb_remove_list_lock, flags);
		} else {
			urb->nStatus = -ENOENT;
#if 0
			if (in_interrupt()) {	/* wait at least 1 frame */
				static int errorcount = 10;

				if (errorcount--)
					dbg("uhci_unlink_urb called from interrupt for urb %p", urb);
				udelay(1000);
			} else
				schedule_timeout(1+1*HZ/1000); 
#endif
			snooze( 1000 );

			uhci_call_completion(urb);
		}
	}

	return 0;
}

int uhci_fsbr_timeout(struct uhci *uhci, USB_packet_s *urb)
{
	struct urb_priv *urbp = (struct urb_priv *)urb->pHCPrivate;
	struct list_head *head, *tmp;

	uhci_dec_fsbr(uhci, urb);

	/* There is a race with updating IOC in here, but it's not worth */
	/*  trying to fix since this is merely an optimization. The only */
	/*  time we'd lose is if the status of the packet got updated */
	/*  and we'd be turning on FSBR next frame anyway, so it's a wash */
	urbp->fsbr_timeout = 1;

	head = &urbp->td_list;
	tmp = head->next;
	while (tmp != head) {
		struct uhci_td *td = list_entry(tmp, struct uhci_td, list);

		tmp = tmp->next;

		if (td->status & TD_CTRL_ACTIVE) {
			set_bit(TD_CTRL_IOC_BIT, &td->status);
			break;
		}
	}

	return 0;
}

/*
 * uhci_get_current_frame_number()
 *
 * returns the current frame number for a USB bus/controller.
 */
int uhci_get_current_frame_number(USB_device_s *dev)
{
	struct uhci *uhci = (struct uhci *)dev->psBus->pHCPrivate;

	return inw(uhci->io_addr + USBFRNUM);
}

/* Virtual Root Hub */

static uint8 root_hub_dev_des[] =
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
static uint8 root_hub_config_des[] =
{
	0x09,			/*  __u8  bLength; */
	0x02,			/*  __u8  bDescriptorType; Configuration */
	0x19,			/*  __u16 wTotalLength; */
	0x00,
	0x01,			/*  __u8  bNumInterfaces; */
	0x01,			/*  __u8  bConfigurationValue; */
	0x00,			/*  __u8  iConfiguration; */
	0x40,			/*  __u8  bmAttributes;
					Bit 7: Bus-powered, 6: Self-powered,
					Bit 5 Remote-wakeup, 4..0: resvd */
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

static uint8 root_hub_hub_des[] =
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

/* prepare Interrupt pipe transaction data; HUB INTERRUPT ENDPOINT */
int rh_send_irq(USB_packet_s *urb)
{
	int i, len = 1;
	struct uhci *uhci = (struct uhci *)urb->psDevice->psBus->pHCPrivate;
	unsigned int io_addr = uhci->io_addr;
	uint16 data = 0;
	struct urb_priv *urbp = (struct urb_priv *)urb->pHCPrivate;

	for (i = 0; i < uhci->rh.numports; i++) {
		data |= ((inw(io_addr + USBPORTSC1 + i * 2) & 0xa) > 0 ? (1 << (i + 1)) : 0);
		len = (i + 1) / 8 + 1;
	}

	*(uint16 *) urb->pBuffer = data;
	urb->nActualLength = len;
	urbp->status = 0;

	if ((data > 0) && (uhci->rh.send != 0)) {
		//printk("root-hub INT complete: port1: %x port2: %x data: %x\n",
			//inw(io_addr + USBPORTSC1), inw(io_addr + USBPORTSC2), data);
		uhci_call_completion(urb);
	}

	return 0;
}

/* Virtual Root Hub INTs are polled by this timer every "interval" ms */
int rh_init_int_timer(USB_packet_s *urb);

void rh_int_timer_do(void* ptr)
{
	USB_packet_s *urb = (USB_packet_s *)ptr;
	struct uhci *uhci = (struct uhci *)urb->psDevice->psBus->pHCPrivate;
	struct list_head list, *tmp, *head;
	unsigned long flags;

	if (uhci->rh.send)
		rh_send_irq(urb);

	INIT_LIST_HEAD(&list);

	spinlock_cli(&uhci->urb_list_lock, flags);
	head = &uhci->urb_list;

	tmp = head->next;
	while (tmp != head) {
		USB_packet_s *u = list_entry(tmp, USB_packet_s, lPacketList);
		struct urb_priv *urbp = (struct urb_priv *)u->pHCPrivate;

		tmp = tmp->next;

		/* Check if the FSBR timed out */
		if (urbp->fsbr && !urbp->fsbr_timeout && ( get_system_time() > urbp->fsbrtime + IDLE_TIMEOUT*1000))
			uhci_fsbr_timeout(uhci, u);

		/* Check if the URB timed out */
		if (u->nTimeOut && (get_system_time() > urbp->inserttime + u->nTimeOut * 1000)) {
			list_del(&u->lPacketList);
			list_add_tail(&u->lPacketList, &list);
		}
	}
	spinunlock_restore(&uhci->urb_list_lock, flags);

	head = &list;
	tmp = head->next;
	while (tmp != head) {
		USB_packet_s *u = list_entry(tmp, USB_packet_s, lPacketList);

		tmp = tmp->next;

		u->nTransferFlags |= USB_ASYNC_UNLINK | USB_TIMEOUT_KILLED;
		uhci_unlink_urb(u);
	}

	/* enter global suspend if nothing connected */
	if (!uhci->is_suspended && !ports_active(uhci))
		suspend_hc(uhci);

	rh_init_int_timer(urb);
}

/* Root Hub INTs are polled by this timer */
int rh_init_int_timer(USB_packet_s *urb)
{
	struct uhci *uhci = (struct uhci *)urb->psDevice->psBus->pHCPrivate;

	uhci->rh.interval = urb->nInterval;
	/*init_timer(&uhci->rh.rh_int_timer);
	uhci->rh.rh_int_timer.function = rh_int_timer_do;
	uhci->rh.rh_int_timer.data = (unsigned long)urb;
	uhci->rh.rh_int_timer.expires = jiffies + (HZ * (urb->interval < 30 ? 30 : urb->interval)) / 1000;
	add_timer(&uhci->rh.rh_int_timer);*/
	uhci->rh.rh_int_timer = create_timer();
	start_timer( uhci->rh.rh_int_timer, rh_int_timer_do, (void*)urb, (1000 * (urb->nInterval < 30 ? 30 : urb->nInterval)), true );


	return 0;
}

#define OK(x)			len = (x); break

#define CLR_RH_PORTSTAT(x) \
	status = inw(io_addr + USBPORTSC1 + 2 * (wIndex-1)); \
	status = (status & 0xfff5) & ~(x); \
	outw(status, io_addr + USBPORTSC1 + 2 * (wIndex-1))

#define SET_RH_PORTSTAT(x) \
	status = inw(io_addr + USBPORTSC1 + 2 * (wIndex-1)); \
	status = (status & 0xfff5) | (x); \
	outw(status, io_addr + USBPORTSC1 + 2 * (wIndex-1))

#define wait_ms(x) snooze(x*1000)


#define min_t(type,x,y) \
	({ type __x = (x); type __y = (y); __x < __y ? __x: __y; })

/* Root Hub Control Pipe */
int rh_submit_urb(USB_packet_s *urb)
{
	struct uhci *uhci = (struct uhci *)urb->psDevice->psBus->pHCPrivate;
	unsigned int pipe = urb->nPipe;
	USB_request_s *cmd = (USB_request_s *)urb->pSetupPacket;
	void *data = urb->pBuffer;
	int leni = urb->nBufferLength;
	int len = 0;
	int status = 0;
	int stat = 0;
	int i;
	unsigned int io_addr = uhci->io_addr;
	uint16 cstatus;
	uint16 bmRType_bReq;
	uint16 wValue;
	uint16 wIndex;
	uint16 wLength;
	
	if (usb_pipetype(pipe) == USB_PIPE_INTERRUPT) {
		uhci->rh.urb = urb;
		uhci->rh.send = 1;
		uhci->rh.interval = urb->nInterval;
		rh_init_int_timer(urb);

		return -EINPROGRESS;
	}

	bmRType_bReq = cmd->nRequestType | cmd->nRequest << 8;
	wValue = cmd->nValue;
	wIndex = cmd->nIndex;
	wLength = cmd->nLength;

	for (i = 0; i < 8; i++)
		uhci->rh.c_p_r[i] = 0;

	switch (bmRType_bReq) {
		/* Request Destination:
		   without flags: Device,
		   RH_INTERFACE: interface,
		   RH_ENDPOINT: endpoint,
		   RH_CLASS means HUB here,
		   RH_OTHER | RH_CLASS  almost ever means HUB_PORT here
		*/

	case RH_GET_STATUS:
		*(uint16 *)data = 1;
		OK(2);
	case RH_GET_STATUS | RH_INTERFACE:
		*(uint16 *)data = 0;
		OK(2);
	case RH_GET_STATUS | RH_ENDPOINT:
		*(uint16 *)data = 0;
		OK(2);
	case RH_GET_STATUS | RH_CLASS:
		*(uint32 *)data = 0;
		OK(4);		/* hub power */
	case RH_GET_STATUS | RH_OTHER | RH_CLASS:
		status = inw(io_addr + USBPORTSC1 + 2 * (wIndex - 1));
		cstatus = ((status & USBPORTSC_CSC) >> (1 - 0)) |
			((status & USBPORTSC_PEC) >> (3 - 1)) |
			(uhci->rh.c_p_r[wIndex - 1] << (0 + 4));
			status = (status & USBPORTSC_CCS) |
			((status & USBPORTSC_PE) >> (2 - 1)) |
			((status & USBPORTSC_SUSP) >> (12 - 2)) |
			((status & USBPORTSC_PR) >> (9 - 4)) |
			(1 << 8) |      /* power on */
			((status & USBPORTSC_LSDA) << (-8 + 9));

		*(uint16 *)data = status;
		*((uint16 *)(data + 2)) = cstatus;
		OK(4);
	case RH_CLEAR_FEATURE | RH_ENDPOINT:
		switch (wValue) {
		case RH_ENDPOINT_STALL:
			OK(0);
		}
		break;
	case RH_CLEAR_FEATURE | RH_CLASS:
		switch (wValue) {
		case RH_C_HUB_OVER_CURRENT:
			OK(0);	/* hub power over current */
		}
		break;
	case RH_CLEAR_FEATURE | RH_OTHER | RH_CLASS:
		switch (wValue) {
		case RH_PORT_ENABLE:
			CLR_RH_PORTSTAT(USBPORTSC_PE);
			OK(0);
		case RH_PORT_SUSPEND:
			CLR_RH_PORTSTAT(USBPORTSC_SUSP);
			OK(0);
		case RH_PORT_POWER:
			OK(0);	/* port power */
		case RH_C_PORT_CONNECTION:
			SET_RH_PORTSTAT(USBPORTSC_CSC);
			OK(0);
		case RH_C_PORT_ENABLE:
			SET_RH_PORTSTAT(USBPORTSC_PEC);
			OK(0);
		case RH_C_PORT_SUSPEND:
			/*** WR_RH_PORTSTAT(RH_PS_PSSC); */
			OK(0);
		case RH_C_PORT_OVER_CURRENT:
			OK(0);	/* port power over current */
		case RH_C_PORT_RESET:
			uhci->rh.c_p_r[wIndex - 1] = 0;
			OK(0);
		}
		break;
	case RH_SET_FEATURE | RH_OTHER | RH_CLASS:
		switch (wValue) {
		case RH_PORT_SUSPEND:
			SET_RH_PORTSTAT(USBPORTSC_SUSP);
			OK(0);
		case RH_PORT_RESET:
			SET_RH_PORTSTAT(USBPORTSC_PR);
			wait_ms(50);	/* USB v1.1 7.1.7.3 */
			uhci->rh.c_p_r[wIndex - 1] = 1;
			CLR_RH_PORTSTAT(USBPORTSC_PR);
			udelay(10);
			SET_RH_PORTSTAT(USBPORTSC_PE);
			wait_ms(10);
			SET_RH_PORTSTAT(0xa);
			OK(0);
		case RH_PORT_POWER:
			OK(0); /* port power ** */
		case RH_PORT_ENABLE:
			SET_RH_PORTSTAT(USBPORTSC_PE);
			OK(0);
		}
		break;
	case RH_SET_ADDRESS:
		uhci->rh.devnum = wValue;
		OK(0);
	case RH_GET_DESCRIPTOR:
		switch ((wValue & 0xff00) >> 8) {
		case 0x01:	/* device descriptor */
			len = min_t(unsigned int, leni,
				  min_t(unsigned int,
				      sizeof(root_hub_dev_des), wLength));
			memcpy(data, root_hub_dev_des, len);
			OK(len);
		case 0x02:	/* configuration descriptor */
			len = min_t(unsigned int, leni,
				  min_t(unsigned int,
				      sizeof(root_hub_config_des), wLength));
			memcpy (data, root_hub_config_des, len);
			OK(len);
		case 0x03:	/* string descriptors */
			len = g_psUSBBus->root_hub_string (wValue & 0xff,
				uhci->io_addr, "UHCI-alt",
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
			  min_t(unsigned int, sizeof(root_hub_hub_des), wLength));
		memcpy(data, root_hub_hub_des, len);
		OK(len);
	case RH_GET_CONFIGURATION:
		*(uint8 *)data = 0x01;
		OK(1);
	case RH_SET_CONFIGURATION:
		OK(0);
	case RH_GET_INTERFACE | RH_INTERFACE:
		*(uint8 *)data = 0x00;
		OK(1);
	case RH_SET_INTERFACE | RH_INTERFACE:
		OK(0);
	default:
		stat = -EPIPE;
	}

	urb->nActualLength = len;

	return stat;
}

int rh_unlink_urb(USB_packet_s *urb)
{
	struct uhci *uhci = (struct uhci *)urb->psDevice->psBus->pHCPrivate;

	if (uhci->rh.urb == urb) {
		urb->nStatus = -ENOENT;
		uhci->rh.send = 0;
		uhci->rh.urb = NULL;
		delete_timer(&uhci->rh.rh_int_timer);
	}
	return 0;
}

static void uhci_free_pending_qhs(struct uhci *uhci)
{
	struct list_head *tmp, *head;
	unsigned long flags;

	spinlock_cli(&uhci->qh_remove_list_lock, flags);
	head = &uhci->qh_remove_list;
	tmp = head->next;
	while (tmp != head) {
		struct uhci_qh *qh = list_entry(tmp, struct uhci_qh, remove_list);

		tmp = tmp->next;

		list_del_init(&qh->remove_list);

		uhci_free_qh(uhci, qh);
	}
	spinunlock_restore(&uhci->qh_remove_list_lock, flags);
}

void uhci_call_completion(USB_packet_s *urb)
{
	struct urb_priv *urbp = (struct urb_priv *)urb->pHCPrivate;
	USB_device_s *dev = urb->psDevice;
//	struct uhci *uhci = (struct uhci *)dev->psBus->pHCPrivate;
	int is_ring = 0, killed, resubmit_interrupt, status;
	USB_packet_s *nurb;

	killed = (urb->nStatus == -ENOENT || urb->nStatus == -ECONNABORTED ||
			urb->nStatus == -ECONNRESET);
	resubmit_interrupt = (usb_pipetype(urb->nPipe) == USB_PIPE_INTERRUPT &&
			urb->nInterval && !killed);

	nurb = urb->psNext;
	if (nurb && !killed) {
		int count = 0;

		while (nurb && nurb != urb && count < MAX_URB_LOOP) {
			if (nurb->nStatus == -ENOENT ||
			    nurb->nStatus == -ECONNABORTED ||
			    nurb->nStatus == -ECONNRESET) {
				killed = 1;
				break;
			}

			nurb = nurb->psNext;
			count++;
		}

		if (count == MAX_URB_LOOP)
			printk("uhci_call_completion: too many linked URB's, loop? (first loop)\n");

		/* Check to see if chain is a ring */
		is_ring = (nurb == urb);
	}

	status = urbp->status;
	if (!resubmit_interrupt)
		/* We don't need urb_priv anymore */
		uhci_destroy_urb_priv(urb);

	if (!killed)
		urb->nStatus = status;

	urb->psDevice = NULL;
	if (urb->pComplete)
		urb->pComplete(urb);

	if (resubmit_interrupt) {
		urb->psDevice = dev;
		uhci_reset_interrupt(urb);
	} else {
		if (is_ring && !killed) {
			urb->psDevice = dev;
			uhci_submit_urb(urb);
		} else {
			/* We decrement the usage count after we're done */
			/*  with everything */
			atomic_add( &dev->nRefCount, -1 );
			//usb_dec_dev_use(dev);
		}
	}
}

void uhci_finish_completion(struct uhci *uhci)
{
	struct list_head *tmp, *head;
	unsigned long flags;

	spinlock_cli(&uhci->complete_list_lock, flags);
	head = &uhci->complete_list;
	tmp = head->next;
	while (tmp != head) {
		struct urb_priv *urbp = list_entry(tmp, struct urb_priv, complete_list);
		USB_packet_s *urb = urbp->urb;

		tmp = tmp->next;

		list_del_init(&urbp->complete_list);

		uhci_call_completion(urb);
	}
	spinunlock_restore(&uhci->complete_list_lock, flags);
}

void uhci_remove_pending_qhs(struct uhci *uhci)
{
	struct list_head *tmp, *head;
	unsigned long flags;

	spinlock_cli(&uhci->urb_remove_list_lock, flags);
	head = &uhci->urb_remove_list;
	tmp = head->next;
	while (tmp != head) {
		USB_packet_s *urb = list_entry(tmp, USB_packet_s, lPacketList);
		struct urb_priv *urbp = (struct urb_priv *)urb->pHCPrivate;

		tmp = tmp->next;

		list_del_init(&urb->lPacketList);

		urbp->status = urb->nStatus = -ECONNRESET;
		uhci_call_completion(urb);
	}
	spinunlock_restore(&uhci->urb_remove_list_lock, flags);
}

int uhci_interrupt(int irq, void *dev_id, SysCallRegs_s *regs )
{
	struct uhci *uhci = dev_id;
	unsigned int io_addr = uhci->io_addr;
	unsigned short status;
	struct list_head *tmp, *head;

	/*
	 * Read the interrupt status, and write it back to clear the
	 * interrupt cause
	 */
	status = inw(io_addr + USBSTS);
	if (!status)	/* shared interrupt, not mine */
		return( -1 );
	outw(status, io_addr + USBSTS);		/* Clear it */

	if (status & ~(USBSTS_USBINT | USBSTS_ERROR | USBSTS_RD)) {
		if (status & USBSTS_HSE)
			printk("%x: host system error, PCI problems?\n", io_addr);
		if (status & USBSTS_HCPE)
			printk("%x: host controller process error. something bad happened\n", io_addr);
		if ((status & USBSTS_HCH) && !uhci->is_suspended) {
			printk("%x: host controller halted. very bad\n", io_addr);
			/* FIXME: Reset the controller, fix the offending TD */
		}
	}

	if (status & USBSTS_RD)
		wakeup_hc(uhci);

	uhci_free_pending_qhs(uhci);

	uhci_remove_pending_qhs(uhci);

	uhci_clear_next_interrupt(uhci);

	/* Walk the list of pending URB's to see which ones completed */
	spinlock(&uhci->urb_list_lock);
	head = &uhci->urb_list;
	tmp = head->next;
	while (tmp != head) {
		USB_packet_s*urb = list_entry(tmp, USB_packet_s, lPacketList);

		tmp = tmp->next;

		/* Checks the status and does all of the magic necessary */
		uhci_transfer_result(uhci, urb);
	}
	spinunlock(&uhci->urb_list_lock);

	uhci_finish_completion(uhci);
	
	return( 0 );
}

void reset_hc(struct uhci *uhci)
{
	unsigned int io_addr = uhci->io_addr;

	/* Global reset for 50ms */
	outw(USBCMD_GRESET, io_addr + USBCMD);
	snooze( 50 * 1000 );
	outw(0, io_addr + USBCMD);
	snooze( 10 * 1000 );
}

void suspend_hc(struct uhci *uhci)
{
	unsigned int io_addr = uhci->io_addr;

	//printk("%x: suspend_hc\n", io_addr);

	outw(USBCMD_EGSM, io_addr + USBCMD);

	uhci->is_suspended = 1;
}

void wakeup_hc(struct uhci *uhci)
{
	unsigned int io_addr = uhci->io_addr;
	unsigned int status;

	//printk("%x: wakeup_hc\n", io_addr);

	outw(0, io_addr + USBCMD);
	
	/* wait for EOP to be sent */
	status = inw(io_addr + USBCMD);
	while (status & USBCMD_FGR)
		status = inw(io_addr + USBCMD);

	uhci->is_suspended = 0;

	/* Run and mark it configured with a 64-byte max packet */
	outw(USBCMD_RS | USBCMD_CF | USBCMD_MAXP, io_addr + USBCMD);
}

int ports_active(struct uhci *uhci)
{
	unsigned int io_addr = uhci->io_addr;
	int connection = 0;
	int i;

	for (i = 0; i < uhci->rh.numports; i++)
		connection |= (inw(io_addr + USBPORTSC1 + i * 2) & 0x1);

	return connection;
}

void start_hc(struct uhci *uhci)
{
	unsigned int io_addr = uhci->io_addr;
	int timeout = 1000;

	/*
	 * Reset the HC - this will force us to get a
	 * new notification of any already connected
	 * ports due to the virtual disconnect that it
	 * implies.
	 */
	outw(USBCMD_HCRESET, io_addr + USBCMD);
	while (inw(io_addr + USBCMD) & USBCMD_HCRESET) {
		if (!--timeout) {
			printk("uhci: USBCMD_HCRESET timed out!\n");
			break;
		}
	}

	/* Turn on all interrupts */
	outw(USBINTR_TIMEOUT | USBINTR_RESUME | USBINTR_IOC | USBINTR_SP,
		io_addr + USBINTR);

	/* Start at frame 0 */
	outw(0, io_addr + USBFRNUM);
	outl(uhci->fl->dma_handle, io_addr + USBFLBASEADD);

	/* Run and mark it configured with a 64-byte max packet */
	outw(USBCMD_RS | USBCMD_CF | USBCMD_MAXP, io_addr + USBCMD);
}

/*
 * Allocate a frame list, and then setup the skeleton
 *
 * The hardware doesn't really know any difference
 * in the queues, but the order does matter for the
 * protocols higher up. The order is:
 *
 *  - any isochronous events handled before any
 *    of the queues. We don't do that here, because
 *    we'll create the actual TD entries on demand.
 *  - The first queue is the interrupt queue.
 *  - The second queue is the control queue, split into low and high speed
 *  - The third queue is bulk queue.
 *  - The fourth queue is the bandwidth reclamation queue, which loops back
 *    to the high speed control queue.
 */
static int alloc_uhci( int nDeviceID, PCI_Info_s dev, unsigned int io_addr, unsigned int io_size)
{
	struct uhci *uhci;
	int retval;
//	char buf[8]/*, *bufp = buf*/;
	int i, port;
	USB_bus_driver_s* bus;
//	uint32 dma_handle;

	retval = -ENODEV;
	
	/* Claim device */
	if( claim_device( nDeviceID, dev.nHandle, "USB UHCI controller", DEVICE_CONTROLLER ) != 0 )
		return( -1 );
	
	/* Enable busmaster */
	g_psPCIBus->write_pci_config( dev.nBus, dev.nDevice, dev.nFunction, PCI_COMMAND, 2, g_psPCIBus->read_pci_config( dev.nBus, dev.nDevice, dev.nFunction, PCI_COMMAND, 2 )
					| PCI_COMMAND_MASTER );
					
	printk( "USB UHCI controller @ 0x%x, IRQ %i\n", io_addr, dev.u.h0.nInterruptLine );
	
	uhci = kmalloc(sizeof(*uhci), MEMF_KERNEL | MEMF_NOBLOCK );
	if (!uhci) {
		printk("couldn't allocate uhci structure\n");
		retval = -ENOMEM;
		goto err_alloc_uhci;
	}

	uhci->dev = dev;
	uhci->io_addr = io_addr;
	uhci->io_size = io_size;

	
	/* Reset here so we don't get any interrupts from an old setup */
	/*  or broken setup */
	reset_hc(uhci);
	
	spinlock_init( &uhci->qh_remove_list_lock, "qh_remove_list_lock" );
	INIT_LIST_HEAD(&uhci->qh_remove_list);

	spinlock_init( &uhci->urb_remove_list_lock, "urb_remove_list_lock" );
	INIT_LIST_HEAD(&uhci->urb_remove_list);

	spinlock_init( &uhci->urb_list_lock, "urb_list_lock" );
	INIT_LIST_HEAD(&uhci->urb_list);

	spinlock_init( &uhci->complete_list_lock, "complete_list_lock" );
	INIT_LIST_HEAD(&uhci->complete_list);

	spinlock_init( &uhci->frame_list_lock, "frame_list_lock" );
	
	uhci->fl_real = kmalloc( sizeof(*uhci->fl) + PAGE_SIZE, MEMF_KERNEL | MEMF_NOBLOCK );
	uhci->fl = ( struct uhci_frame_list* )( ( (uint32)uhci->fl_real + PAGE_SIZE ) & PAGE_MASK );
	memset((void *)uhci->fl, 0, sizeof(*uhci->fl));
	
	uhci->fl->dma_handle = (uint32)uhci->fl;
	
	
	bus = g_psUSBBus->alloc_bus();
	if (!bus) {
		printk("unable to allocate bus\n");
		goto err_alloc_bus;
	}
	uhci->bus = bus;
	bus->AddDevice = uhci_alloc_dev;
	bus->RemoveDevice = uhci_free_dev;
	bus->GetFrameNumber = uhci_get_current_frame_number;
	bus->SubmitPacket = uhci_submit_urb;
	bus->CancelPacket = uhci_unlink_urb;
	bus->pHCPrivate = uhci;
	g_psUSBBus->add_bus( uhci->bus );
	
	
	/* Initialize the root hub */
	
	/* UHCI specs says devices must have 2 ports, but goes on to say */
	/*  they may have more but give no way to determine how many they */
	/*  have. However, according to the UHCI spec, Bit 7 is always set */
	/*  to 1. So we try to use this to our advantage */
	for (port = 0; port < (uhci->io_size - 0x10) / 2; port++) {
		unsigned int portstatus;

		portstatus = inw(uhci->io_addr + 0x10 + (port * 2));
		if (!(portstatus & 0x0080))
			break;
		//printk("Port %i detected\n", port + 1 );
	}
	
	/* This is experimental so anything less than 2 or greater than 8 is */
	/*  something weird and we'll ignore it */
	if (port < 2 || port > 8) {
		printk("port count misdetected? forcing to 2 ports\n");
		port = 2;
	}

	uhci->rh.numports = port;
	
	uhci->bus->psRootHUB = uhci->rh.dev = g_psUSBBus->alloc_device(NULL, uhci->bus);
	if (!uhci->rh.dev) {
		printk("unable to allocate root hub\n");
		goto err_alloc_root_hub;
	}
	
	uhci->skeltd[0] = uhci_alloc_td(uhci, uhci->rh.dev);
	if (!uhci->skeltd[0]) {
		printk("unable to allocate TD 0\n");
		goto err_alloc_skeltd;
	}
	
	/*
	 * 9 Interrupt queues; link int2 to int1, int4 to int2, etc
	 * then link int1 to control and control to bulk
	 */
	for (i = 1; i < 9; i++) {
		struct uhci_td *td;

		td = uhci->skeltd[i] = uhci_alloc_td(uhci, uhci->rh.dev);
		if (!td) {
			printk("unable to allocate TD %d\n", i);
			goto err_alloc_skeltd;
		}

		uhci_fill_td(td, 0, (UHCI_NULL_DATA_SIZE << 21) | (0x7f << 8) | USB_PID_IN, 0);
		td->link = uhci->skeltd[i - 1]->dma_handle;
	}
	
	uhci->skel_term_td = uhci_alloc_td(uhci, uhci->rh.dev);
	if (!uhci->skel_term_td) {
		printk("unable to allocate skel TD term\n");
		goto err_alloc_skeltd;
	}
	
	for (i = 0; i < UHCI_NUM_SKELQH; i++) {
		uhci->skelqh[i] = uhci_alloc_qh(uhci, uhci->rh.dev);
		if (!uhci->skelqh[i]) {
			printk("unable to allocate QH %d\n", i);
			goto err_alloc_skelqh;
		}
	}
	
	uhci_fill_td(uhci->skel_int1_td, 0, (UHCI_NULL_DATA_SIZE << 21) | (0x7f << 8) | USB_PID_IN, 0);
	uhci->skel_int1_td->link = uhci->skel_ls_control_qh->dma_handle | UHCI_PTR_QH;

	uhci->skel_ls_control_qh->link = uhci->skel_hs_control_qh->dma_handle | UHCI_PTR_QH;
	uhci->skel_ls_control_qh->element = UHCI_PTR_TERM;

	uhci->skel_hs_control_qh->link = uhci->skel_bulk_qh->dma_handle | UHCI_PTR_QH;
	uhci->skel_hs_control_qh->element = UHCI_PTR_TERM;

	uhci->skel_bulk_qh->link = uhci->skel_term_qh->dma_handle | UHCI_PTR_QH;
	uhci->skel_bulk_qh->element = UHCI_PTR_TERM;

	/* This dummy TD is to work around a bug in Intel PIIX controllers */
	uhci_fill_td(uhci->skel_term_td, 0, (UHCI_NULL_DATA_SIZE << 21) | (0x7f << 8) | USB_PID_IN, 0);
	uhci->skel_term_td->link = uhci->skel_term_td->dma_handle;

	uhci->skel_term_qh->link = UHCI_PTR_TERM;
	uhci->skel_term_qh->element = uhci->skel_term_td->dma_handle;
	
	/*
	 * Fill the frame list: make all entries point to
	 * the proper interrupt queue.
	 *
	 * This is probably silly, but it's a simple way to
	 * scatter the interrupt queues in a way that gives
	 * us a reasonable dynamic range for irq latencies.
	 */
	for (i = 0; i < UHCI_NUMFRAMES; i++) {
		int irq = 0;

		if (i & 1) {
			irq++;
			if (i & 2) {
				irq++;
				if (i & 4) { 
					irq++;
					if (i & 8) { 
						irq++;
						if (i & 16) {
							irq++;
							if (i & 32) {
								irq++;
								if (i & 64)
									irq++;
							}
						}
					}
				}
			}
		}

		/* Only place we don't use the frame list routines */
		uhci->fl->frame[i] =  uhci->skeltd[irq]->dma_handle;
	}
	
	start_hc(uhci);
	
	if (request_irq(dev.u.h0.nInterruptLine, uhci_interrupt, NULL, SA_SHIRQ, "usb-uhci", uhci)<0)
		goto err_request_irq;
		
	uhci->irq = dev.u.h0.nInterruptLine;
	
	/* disable legacy emulation */
	g_psPCIBus->write_pci_config( dev.nBus, dev.nDevice, dev.nFunction, USBLEGSUP, 2, USBLEGSUP_DEFAULT );
	
	g_psUSBBus->connect(uhci->rh.dev);
	
	if (g_psUSBBus->new_device(uhci->rh.dev) != 0) {
		printk("unable to start root hub\n");
		retval = -ENOMEM;
		goto err_start_root_hub;
	}
	printk( "UHCI controller initialized\n" );
	
	
	return 0;

/*
 * error exits:
 */
err_start_root_hub:
	uhci->irq = -1;

err_request_irq:
	for (i = 0; i < UHCI_NUM_SKELQH; i++)
		if (uhci->skelqh[i]) {
			uhci_free_qh(uhci, uhci->skelqh[i]);
			uhci->skelqh[i] = NULL;
		}

err_alloc_skelqh:
	for (i = 0; i < UHCI_NUM_SKELTD; i++)
		if (uhci->skeltd[i]) {
			uhci_free_td(uhci, uhci->skeltd[i]);
			uhci->skeltd[i] = NULL;
		}

err_alloc_skeltd:
	g_psUSBBus->free_device(uhci->rh.dev);
	uhci->rh.dev = NULL;

err_alloc_root_hub:
	g_psUSBBus->free_bus(uhci->bus);
	uhci->bus = NULL;


//err_create_qh_pool:
	

//err_create_td_pool:
	kfree( uhci->fl_real );
	uhci->fl = NULL;

//err_alloc_fl:
	
err_alloc_bus:

err_alloc_uhci:

	return retval;
	
	
	return( 0 );
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

int uhci_pci_probe( int nDeviceID, PCI_Info_s dev )
{
	int i;

	/* Search for the IO base address.. */
	for (i = 0; i < 6; i++) {
		
		unsigned int io_addr = *( &dev.u.h0.nBase0 + i );
		
		if( !( io_addr & PCI_ADDRESS_SPACE ) ) {
			continue;
		}
		return alloc_uhci( nDeviceID, dev, io_addr & PCI_ADDRESS_IO_MASK, get_pci_memory_size( &dev, i ) );
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
	
	g_psUSBBus = get_busmanager( USB_BUS_NAME, USB_BUS_VERSION );
	if( g_psUSBBus == NULL )
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
   		printk( "No USB UHCI controller found\n" );
   		disable_device( nDeviceID );
   		return( -1 );
   	}
	return( 0 );
}


status_t device_uninit( int nDeviceID)
{
	return( 0 );
}
































