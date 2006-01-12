/*
 * URB OHCI HCD (Host Controller Driver) for USB.
 *
 * (C) Copyright 1999 Roman Weissgaerber <weissg@vienna.at>
 * (C) Copyright 2000-2001 David Brownell <dbrownell@users.sourceforge.net>
 * 
 * [ Initialisation is based on Linus'  ]
 * [ uhci code and gregs ohci fragments ]
 * [ (C) Copyright 1999 Linus Torvalds  ]
 * [ (C) Copyright 1999 Gregory P. Smith]
 * 
 * 
 * History:
 *
 * 2002/10/22 OHCI_USB_OPER for ALi lockup in IBM i1200 (ALEX <thchou@ali>)
 * 2002/03/08 interrupt unlink fix (Matt Hughes), better cleanup on
 *	load failure (Matthew Frederickson)
 * 2002/01/20 async unlink fixes:  return -EINPROGRESS (per spec) and
 *	make interrupt unlink-in-completion work (db)
 * 2001/09/19 USB_ZERO_PACKET support (Jean Tourrilhes)
 * 2001/07/17 power management and pmac cleanup (Benjamin Herrenschmidt)
 * 2001/03/24 td/ed hashing to remove bus_to_virt (Steve Longerbeam);
 	pci_map_single (db)
 * 2001/03/21 td and dev/ed allocation uses new pci_pool API (db)
 * 2001/03/07 hcca allocation uses pci_alloc_consistent (Steve Longerbeam)
 *
 * 2000/09/26 fixed races in removing the private portion of the urb
 * 2000/09/07 disable bulk and control lists when unlinking the last
 *	endpoint descriptor in order to avoid unrecoverable errors on
 *	the Lucent chips. (rwc@sgi)
 * 2000/08/29 use bandwidth claiming hooks (thanks Randy!), fix some
 *	urb unlink probs, indentation fixes
 * 2000/08/11 various oops fixes mostly affecting iso and cleanup from
 *	device unplugs.
 * 2000/06/28 use PCI hotplug framework, for better power management
 *	and for Cardbus support (David Brownell)
 * 2000/earlier:  fixes for NEC/Lucent chips; suspend/resume handling
 *	when the controller loses power; handle UE; cleanup; ...
 *
 * v5.2 1999/12/07 URB 3rd preview, 
 * v5.1 1999/11/30 URB 2nd preview, cpia, (usb-scsi)
 * v5.0 1999/11/22 URB Technical preview, Paul Mackerras powerbook susp/resume 
 * 	i386: HUB, Keyboard, Mouse, Printer 
 *
 * v4.3 1999/10/27 multiple HCs, bulk_request
 * v4.2 1999/09/05 ISO API alpha, new dev alloc, neg Error-codes
 * v4.1 1999/08/27 Randy Dunlap's - ISO API first impl.
 * v4.0 1999/08/18 
 * v3.0 1999/06/25 
 * v2.1 1999/05/09  code clean up
 * v2.0 1999/05/04 
 * v1.0 1999/04/27 initial release
 */
 
#include <atheos/udelay.h>
#include <atheos/isa_io.h>
#include <atheos/pci.h>
#include <atheos/usb.h>
#include <atheos/kernel.h>
#include <atheos/kdebug.h>
#include <atheos/time.h>
#include <atheos/bootmodules.h>
#include <atheos/device.h>
#include <atheos/bitops.h>
#include <posix/errno.h>
#include <posix/ioctls.h>
#include <posix/fcntl.h>
#include <posix/termios.h>
#include <posix/signal.h>
#include <macros.h>


//#define DEBUG
#undef DEBUG_LIMIT
#define DEBUG_LIMIT KERN_DEBUG

#include "hcd.h"

USB_bus_s* g_psBus;
PCI_bus_s* g_psPCIBus;

#define OHCI_USE_NPS		// force NoPowerSwitching mode
// #define OHCI_VERBOSE_DEBUG	/* not always helpful */

#include "usb-ohci.h"

#if 0
#define __io_virt(x) ((void *)(x))
#define readb(addr) (*(volatile unsigned char *) __io_virt(addr))
#define readw(addr) (*(volatile unsigned short *) __io_virt(addr))
#define readl(addr) (*(volatile unsigned int *) __io_virt(addr))


#define writeb(b,addr) (*(volatile unsigned char *) __io_virt(addr) = (b))
#define writew(b,addr) (*(volatile unsigned short *) __io_virt(addr) = (b))
#define writel(b,addr) (*(volatile unsigned int *) __io_virt(addr) = (b))
#endif


/* For initializing controller (mask in an HCFS mode too) */
#define	OHCI_CONTROL_INIT \
	(OHCI_CTRL_CBSR & 0x3) | OHCI_CTRL_IE | OHCI_CTRL_PLE

#define OHCI_UNLINK_TIMEOUT	(1000 / 10)

static LIST_HEAD (ohci_hcd_list);
static SpinLock_s usb_ed_lock = INIT_SPIN_LOCK( "usb_ed_lock" );


/*-------------------------------------------------------------------------*/

/* AMD-756 (D2 rev) reports corrupt register contents in some cases.
 * The erratum (#4) description is incorrect.  AMD's workaround waits
 * till some bits (mostly reserved) are clear; ok for all revs.
 */
#define read_roothub(hc, register, mask) ({ \
	u32 temp = readl (&hc->regs->roothub.register); \
	if (hc->flags & OHCI_QUIRK_AMD756) \
		while (temp & mask) \
			temp = readl (&hc->regs->roothub.register); \
	temp; })

static u32 roothub_a (struct ohci *hc)
	{ return read_roothub (hc, a, 0xfc0fe000); }
static inline u32 roothub_b (struct ohci *hc)
	{ return readl (&hc->regs->roothub.b); }
static inline u32 roothub_status (struct ohci *hc)
	{ return readl (&hc->regs->roothub.status); }
static u32 roothub_portstatus (struct ohci *hc, int i)
	{ return read_roothub (hc, portstatus [i], 0xffe0fce0); }


/*-------------------------------------------------------------------------*
 * URB support functions 
 *-------------------------------------------------------------------------*/ 
 
/* free HCD-private data associated with this URB */

static void urb_free_priv (struct ohci *hc, urb_priv_t * urb_priv)
{
	int		i;
	int		last = urb_priv->length - 1;
	int		len;
	//int		dir;
	struct td	*td;

	if (last >= 0) {

		/* ISOC, BULK, INTR data buffer starts at td 0 
		 * CTRL setup starts at td 0 */
		td = urb_priv->td [0];

		len = td->urb->nBufferLength;
		//dir = usb_pipeout (td->urb->nPipe)
			//		? PCI_DMA_TODEVICE
				//	: PCI_DMA_FROMDEVICE;

		/* unmap CTRL URB setup */
		if (usb_pipecontrol (td->urb->nPipe)) {
			//pci_unmap_single (hc->ohci_dev, 
					//td->data_dma, 8, PCI_DMA_TODEVICE);
			
			/* CTRL data buffer starts at td 1 if len > 0 */
			if (len && last > 0)
				td = urb_priv->td [1]; 		
		} 

		/* unmap data buffer */
		//if (len && td->data_dma)
			//pci_unmap_single (hc->ohci_dev, td->data_dma, len, dir);
		
		for (i = 0; i <= last; i++) {
			td = urb_priv->td [i];
			if (td)
				td_free (hc, td);
		}
	}

	kfree (urb_priv);
}
 
static void urb_rm_priv_locked (USB_packet_s * urb) 
{
	urb_priv_t * urb_priv = urb->pHCPrivate;
	
	if (urb_priv) {
		urb->pHCPrivate = NULL;

#ifdef	DO_TIMEOUTS
		if (urb->timeout) {
			list_del (&urb->urb_list);
			urb->timeout -= jiffies;
		}
#endif

		/* Release int/iso bandwidth */
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

		urb_free_priv ((struct ohci *)urb->psDevice->psBus->pHCPrivate, urb_priv);
		atomic_dec( &urb->psDevice->nRefCount );
		//usb_dec_dev_use (urb->psDevice);
		urb->psDevice = NULL;
	}
}

static void urb_rm_priv (USB_packet_s * urb)
{
	unsigned long flags;

	spin_lock_irqsave (&usb_ed_lock, flags);
	urb_rm_priv_locked (urb);
	spin_unlock_irqrestore (&usb_ed_lock, flags);
}

/*-------------------------------------------------------------------------*/
 
#ifdef DEBUG
static int sohci_get_current_frame_number (USB_device_s * dev);

/* debug| print the main components of an URB     
 * small: 0) header + data packets 1) just header */
 
static void urb_print (USB_packet_s * urb, char * str, int small)
{
	unsigned int pipe= urb->nPipe;
	
	if (!urb->psDevice || !urb->psDevice->psBus) {
		dbg("%s URB: no dev", str);
		return;
	}
	
#ifndef	OHCI_VERBOSE_DEBUG
	if (urb->status != 0)
#endif
	dbg("%s URB:[%4x] dev:%2d,ep:%2d-%c,type:%s,flags:%4x,len:%d/%d,stat:%d(%x)", 
			str,
		 	sohci_get_current_frame_number (urb->psDevice), 
		 	usb_pipedevice (pipe),
		 	usb_pipeendpoint (pipe), 
		 	usb_pipeout (pipe)? 'O': 'I',
		 	usb_pipetype (pipe) < 2? (usb_pipeint (pipe)? "INTR": "ISOC"):
		 		(usb_pipecontrol (pipe)? "CTRL": "BULK"),
		 	urb->transfer_flags, 
		 	urb->actual_length, 
		 	urb->transfer_buffer_length,
		 	urb->status, urb->status);
#ifdef	OHCI_VERBOSE_DEBUG
	if (!small) {
		int i, len;

		if (usb_pipecontrol (pipe)) {
			printk (KERN_DEBUG __FILE__ ": cmd(8):");
			for (i = 0; i < 8 ; i++) 
				printk (" %02x", ((__u8 *) urb->setup_packet) [i]);
			printk ("\n");
		}
		if (urb->transfer_buffer_length > 0 && urb->transfer_buffer) {
			printk (KERN_DEBUG __FILE__ ": data(%d/%d):", 
				urb->actual_length, 
				urb->transfer_buffer_length);
			len = usb_pipeout (pipe)? 
						urb->transfer_buffer_length: urb->actual_length;
			for (i = 0; i < 16 && i < len; i++) 
				printk (" %02x", ((__u8 *) urb->transfer_buffer) [i]);
			printk ("%s stat:%d\n", i < len? "...": "", urb->status);
		}
	} 
#endif
}

/* just for debugging; prints non-empty branches of the int ed tree inclusive iso eds*/
void ep_print_int_eds (ohci_t * ohci, char * str) {
	int i, j;
	 __u32 * ed_p;
	for (i= 0; i < 32; i++) {
		j = 5;
		ed_p = &(ohci->hcca->int_table [i]);
		if (*ed_p == 0)
		    continue;
		printk (KERN_DEBUG __FILE__ ": %s branch int %2d(%2x):", str, i, i);
		while (*ed_p != 0 && j--) {
			ed_t *ed = dma_to_ed (ohci, le32_to_cpup(ed_p));
			printk (" ed: %4x;", ed->hwINFO);
			ed_p = &ed->hwNextED;
		}
		printk ("\n");
	}
}


static void ohci_dump_intr_mask (char *label, __u32 mask)
{
	dbg ("%s: 0x%08x%s%s%s%s%s%s%s%s%s",
		label,
		mask,
		(mask & OHCI_INTR_MIE) ? " MIE" : "",
		(mask & OHCI_INTR_OC) ? " OC" : "",
		(mask & OHCI_INTR_RHSC) ? " RHSC" : "",
		(mask & OHCI_INTR_FNO) ? " FNO" : "",
		(mask & OHCI_INTR_UE) ? " UE" : "",
		(mask & OHCI_INTR_RD) ? " RD" : "",
		(mask & OHCI_INTR_SF) ? " SF" : "",
		(mask & OHCI_INTR_WDH) ? " WDH" : "",
		(mask & OHCI_INTR_SO) ? " SO" : ""
		);
}

static void maybe_print_eds (char *label, __u32 value)
{
	if (value)
		dbg ("%s %08x", label, value);
}

static char *hcfs2string (int state)
{
	switch (state) {
		case OHCI_USB_RESET:	return "reset";
		case OHCI_USB_RESUME:	return "resume";
		case OHCI_USB_OPER:	return "operational";
		case OHCI_USB_SUSPEND:	return "suspend";
	}
	return "?";
}

// dump control and status registers
static void ohci_dump_status (ohci_t *controller)
{
	struct ohci_regs	*regs = controller->regs;
	__u32			temp;

	temp = readl (&regs->revision) & 0xff;
	if (temp != 0x10)
		dbg ("spec %d.%d", (temp >> 4), (temp & 0x0f));

	temp = readl (&regs->control);
	dbg ("control: 0x%08x%s%s%s HCFS=%s%s%s%s%s CBSR=%d", temp,
		(temp & OHCI_CTRL_RWE) ? " RWE" : "",
		(temp & OHCI_CTRL_RWC) ? " RWC" : "",
		(temp & OHCI_CTRL_IR) ? " IR" : "",
		hcfs2string (temp & OHCI_CTRL_HCFS),
		(temp & OHCI_CTRL_BLE) ? " BLE" : "",
		(temp & OHCI_CTRL_CLE) ? " CLE" : "",
		(temp & OHCI_CTRL_IE) ? " IE" : "",
		(temp & OHCI_CTRL_PLE) ? " PLE" : "",
		temp & OHCI_CTRL_CBSR
		);

	temp = readl (&regs->cmdstatus);
	dbg ("cmdstatus: 0x%08x SOC=%d%s%s%s%s", temp,
		(temp & OHCI_SOC) >> 16,
		(temp & OHCI_OCR) ? " OCR" : "",
		(temp & OHCI_BLF) ? " BLF" : "",
		(temp & OHCI_CLF) ? " CLF" : "",
		(temp & OHCI_HCR) ? " HCR" : ""
		);

	ohci_dump_intr_mask ("intrstatus", readl (&regs->intrstatus));
	ohci_dump_intr_mask ("intrenable", readl (&regs->intrenable));
	// intrdisable always same as intrenable
	// ohci_dump_intr_mask ("intrdisable", readl (&regs->intrdisable));

	maybe_print_eds ("ed_periodcurrent", readl (&regs->ed_periodcurrent));

	maybe_print_eds ("ed_controlhead", readl (&regs->ed_controlhead));
	maybe_print_eds ("ed_controlcurrent", readl (&regs->ed_controlcurrent));

	maybe_print_eds ("ed_bulkhead", readl (&regs->ed_bulkhead));
	maybe_print_eds ("ed_bulkcurrent", readl (&regs->ed_bulkcurrent));

	maybe_print_eds ("donehead", readl (&regs->donehead));
}

static void ohci_dump_roothub (ohci_t *controller, int verbose)
{
	__u32			temp, ndp, i;

	temp = roothub_a (controller);
	if (temp == ~(u32)0)
		return;
	ndp = (temp & RH_A_NDP);

	if (verbose) {
		dbg ("roothub.a: %08x POTPGT=%d%s%s%s%s%s NDP=%d", temp,
			((temp & RH_A_POTPGT) >> 24) & 0xff,
			(temp & RH_A_NOCP) ? " NOCP" : "",
			(temp & RH_A_OCPM) ? " OCPM" : "",
			(temp & RH_A_DT) ? " DT" : "",
			(temp & RH_A_NPS) ? " NPS" : "",
			(temp & RH_A_PSM) ? " PSM" : "",
			ndp
			);
		temp = roothub_b (controller);
		dbg ("roothub.b: %08x PPCM=%04x DR=%04x",
			temp,
			(temp & RH_B_PPCM) >> 16,
			(temp & RH_B_DR)
			);
		temp = roothub_status (controller);
		dbg ("roothub.status: %08x%s%s%s%s%s%s",
			temp,
			(temp & RH_HS_CRWE) ? " CRWE" : "",
			(temp & RH_HS_OCIC) ? " OCIC" : "",
			(temp & RH_HS_LPSC) ? " LPSC" : "",
			(temp & RH_HS_DRWE) ? " DRWE" : "",
			(temp & RH_HS_OCI) ? " OCI" : "",
			(temp & RH_HS_LPS) ? " LPS" : ""
			);
	}
	
	for (i = 0; i < ndp; i++) {
		temp = roothub_portstatus (controller, i);
		dbg ("roothub.portstatus [%d] = 0x%08x%s%s%s%s%s%s%s%s%s%s%s%s",
			i,
			temp,
			(temp & RH_PS_PRSC) ? " PRSC" : "",
			(temp & RH_PS_OCIC) ? " OCIC" : "",
			(temp & RH_PS_PSSC) ? " PSSC" : "",
			(temp & RH_PS_PESC) ? " PESC" : "",
			(temp & RH_PS_CSC) ? " CSC" : "",

			(temp & RH_PS_LSDA) ? " LSDA" : "",
			(temp & RH_PS_PPS) ? " PPS" : "",
			(temp & RH_PS_PRS) ? " PRS" : "",
			(temp & RH_PS_POCI) ? " POCI" : "",
			(temp & RH_PS_PSS) ? " PSS" : "",

			(temp & RH_PS_PES) ? " PES" : "",
			(temp & RH_PS_CCS) ? " CCS" : ""
			);
	}
}

static void ohci_dump (ohci_t *controller, int verbose)
{
	dbg ("OHCI controller usb-%s state", controller->ohci_dev->slot_name);

	// dumps some of the state we know about
	ohci_dump_status (controller);
	if (verbose)
		ep_print_int_eds (controller, "hcca");
	dbg ("hcca frame #%04x", controller->hcca->frame_no);
	ohci_dump_roothub (controller, 1);
}


#endif

/*-------------------------------------------------------------------------*
 * Interface functions (URB)
 *-------------------------------------------------------------------------*/

/* return a request to the completion handler */
 
static int sohci_return_urb (struct ohci *hc, USB_packet_s * urb)
{
	urb_priv_t * urb_priv = urb->pHCPrivate;
	USB_packet_s * urbt;
	unsigned long flags;
	int i;
	
	if (!urb_priv)
		return -1; /* urb already unlinked */

	/* just to be sure */
	if (!urb->pComplete) {
		urb_rm_priv (urb);
		return -1;
	}
	
#ifdef DEBUG
	urb_print (urb, "RET", usb_pipeout (urb->nPipe));
#endif

	switch (usb_pipetype (urb->nPipe)) {
  		case USB_PIPE_INTERRUPT:
			/*pci_unmap_single (hc->ohci_dev,
				urb_priv->td [0]->data_dma,
				urb->transfer_buffer_length,
				usb_pipeout (urb->nPipe)
					? PCI_DMA_TODEVICE
					: PCI_DMA_FROMDEVICE);*/

			if (urb->nInterval) {
				urb->pComplete (urb);
 
				/* implicitly requeued */
				urb->nActualLength = 0;
				urb->nStatus = -EINPROGRESS;
				td_submit_urb (urb);
			} else {
				urb_rm_priv(urb);
				urb->pComplete (urb);
			}
   			break;

  			
		case USB_PIPE_ISOCHRONOUS:
			for (urbt = urb->psNext; urbt && (urbt != urb); urbt = urbt->psNext);
			if (urbt) { /* send the reply and requeue URB */	
				#if 0
				pci_unmap_single (hc->ohci_dev,
					urb_priv->td [0]->data_dma,
					urb->nBufferLength,
					usb_pipeout (urb->nPipe)
						? PCI_DMA_TODEVICE
						: PCI_DMA_FROMDEVICE);
				#endif
				urb->pComplete (urb);
				spin_lock_irqsave (&usb_ed_lock, flags);
				urb->nActualLength = 0;
  				urb->nStatus = -EINPROGRESS;
  				urb->nStartFrame = urb_priv->ed->last_iso + 1;
  				if (urb_priv->state != URB_DEL) {
  					for (i = 0; i < urb->nPacketNum; i++) {
  						urb->sISOFrame[i].nActualLength = 0;
  						urb->sISOFrame[i].nStatus = -EXDEV;
  					}
  					td_submit_urb (urb);
  				}
  				spin_unlock_irqrestore (&usb_ed_lock, flags);
  				
  			} else { /* unlink URB, call complete */
				urb_rm_priv (urb);
				urb->pComplete (urb); 	
			}		
			break;
  				
		case USB_PIPE_BULK:
		case USB_PIPE_CONTROL: /* unlink URB, call complete */
			urb_rm_priv (urb);
			urb->pComplete (urb);	
			break;
	}
	return 0;
}

/*-------------------------------------------------------------------------*/

/* get a transfer request */
 
static int sohci_submit_urb (USB_packet_s * urb)
{
	ohci_t * ohci;
	ed_t * ed;
	urb_priv_t * urb_priv;
	unsigned int pipe = urb->nPipe;
	int maxps = usb_maxpacket (urb->psDevice, pipe, usb_pipeout (pipe));
	int i, size = 0;
	unsigned long flags;
	int bustime = 0;
	int mem_flags = MEMF_KERNEL;
	
	if (!urb->psDevice || !urb->psDevice->psBus)
		return -ENODEV;
	
	if (urb->pHCPrivate)			/* urb already in use */
		return -EINVAL;

//	if(usb_endpoint_halted (urb->psDevice, usb_pipeendpoint (pipe), usb_pipeout (pipe))) 
//		return -EPIPE;
	
	atomic_inc( &urb->psDevice->nRefCount );
	//usb_inc_dev_use (urb->psDevice);
	ohci = (ohci_t *) urb->psDevice->psBus->pHCPrivate;
	
#ifdef DEBUG
	urb_print (urb, "SUB", usb_pipein (pipe));
#endif
	
	/* handle a request to the virtual root hub */
	if (urb->psDevice == ohci->bus->psRootHUB) 
		return rh_submit_urb (urb);
	
	/* when controller's hung, permit only roothub cleanup attempts
	 * such as powering down ports */
	if (ohci->disabled) {
		atomic_dec( &urb->psDevice->nRefCount );
		//usb_dec_dev_use (urb->psDevice);	
		return -ESHUTDOWN;
	}

	/* every endpoint has a ed, locate and fill it */
	if (!(ed = ep_add_ed (urb->psDevice, pipe, urb->nInterval, 1, mem_flags))) {
		atomic_dec( &urb->psDevice->nRefCount );
		//usb_dec_dev_use (urb->psDevice);	
		return -ENOMEM;
	}

	/* for the private part of the URB we need the number of TDs (size) */
	switch (usb_pipetype (pipe)) {
		case USB_PIPE_BULK:	/* one TD for every 4096 Byte */
			size = (urb->nBufferLength - 1) / 4096 + 1;

			/* If the transfer size is multiple of the pipe mtu,
			 * we may need an extra TD to create a empty frame
			 * Jean II */
			if ((urb->nTransferFlags & USB_ZERO_PACKET) &&
			    usb_pipeout (pipe) &&
			    (urb->nBufferLength != 0) && 
			    ((urb->nBufferLength % maxps) == 0))
				size++;
			break;
		case USB_PIPE_ISOCHRONOUS: /* number of packets from URB */
			size = urb->nPacketNum;
			if (size <= 0) {
				atomic_dec( &urb->psDevice->nRefCount );
				//usb_dec_dev_use (urb->psDevice);	
				return -EINVAL;
			}
			for (i = 0; i < urb->nPacketNum; i++) {
  				urb->sISOFrame[i].nActualLength = 0;
  				urb->sISOFrame[i].nStatus = -EXDEV;
  			}
			break;
		case USB_PIPE_CONTROL: /* 1 TD for setup, 1 for ACK and 1 for every 4096 B */
			size = (urb->nBufferLength == 0)? 2: 
						(urb->nBufferLength - 1) / 4096 + 3;
			break;
		case USB_PIPE_INTERRUPT: /* one TD */
			size = 1;
			break;
	}

	/* allocate the private part of the URB */
	urb_priv = kmalloc (sizeof (urb_priv_t) + size * sizeof (td_t *), 
							MEMF_KERNEL);
	if (!urb_priv) {
		atomic_dec( &urb->psDevice->nRefCount );
		//usb_dec_dev_use (urb->psDevice);	
		return -ENOMEM;
	}
	memset (urb_priv, 0, sizeof (urb_priv_t) + size * sizeof (td_t *));
	
	/* fill the private part of the URB */
	urb_priv->length = size;
	urb_priv->ed = ed;	

	/* allocate the TDs (updating hash chains) */
	spin_lock_irqsave (&usb_ed_lock, flags);
	for (i = 0; i < size; i++) { 
		urb_priv->td[i] = td_alloc (ohci, MEMF_KERNEL);
		if (!urb_priv->td[i]) {
			urb_priv->length = i;
			urb_free_priv (ohci, urb_priv);
			spin_unlock_irqrestore (&usb_ed_lock, flags);
			atomic_dec( &urb->psDevice->nRefCount );
			
			//usb_dec_dev_use (urb->psDevice);	
			return -ENOMEM;
		}
	}	

	if (ed->state == ED_NEW || (ed->state & ED_DEL)) {
		urb_free_priv (ohci, urb_priv);
		spin_unlock_irqrestore (&usb_ed_lock, flags);
		//usb_dec_dev_use (urb->psDevice);	
		atomic_dec( &urb->psDevice->nRefCount );
		return -EINVAL;
	}
	
	/* allocate and claim bandwidth if needed; ISO
	 * needs start frame index if it was't provided.
	 */
	switch (usb_pipetype (pipe)) {
		case USB_PIPE_ISOCHRONOUS:
			if (urb->nTransferFlags & USB_ISO_ASAP) { 
				urb->nStartFrame = ((ed->state == ED_OPER)
					? (ed->last_iso + 1)
					: (le16_to_cpu (ohci->hcca->frame_no) + 10)) & 0xffff;
			}	
			/* FALLTHROUGH */
		case USB_PIPE_INTERRUPT:
			if (urb->nBandWidth == 0) {
				bustime = g_psBus->check_bandwidth (urb->psDevice, urb);
			}
			if (bustime < 0) {
				urb_free_priv (ohci, urb_priv);
				spin_unlock_irqrestore (&usb_ed_lock, flags);
				atomic_dec( &urb->psDevice->nRefCount );
				//usb_dec_dev_use (urb->psDevice);	
				return bustime;
			}
			g_psBus->claim_bandwidth (urb->psDevice, urb, bustime, usb_pipeisoc (urb->nPipe));
#ifdef	DO_TIMEOUTS
			urb->nTimeout = 0;
#endif
	}

	urb->nActualLength = 0;
	urb->pHCPrivate = urb_priv;
	urb->nStatus = -EINPROGRESS;

	/* link the ed into a chain if is not already */
	if (ed->state != ED_OPER)
		ep_link (ohci, ed);

	/* fill the TDs and link it to the ed */
	td_submit_urb (urb);

#ifdef	DO_TIMEOUTS
	/* maybe add to ordered list of timeouts */
	if (urb->timeout) {
		struct list_head	*entry;

		urb->timeout += jiffies;

		list_for_each (entry, &ohci->timeout_list) {
			USB_packet_s	*next_urb;

			next_urb = list_entry (entry, USB_packet_s, urb_list);
			if (time_after_eq (urb->timeout, next_urb->timeout))
				break;
		}
		list_add (&urb->urb_list, entry);

		/* drive timeouts by SF (messy, but works) */
		writel (OHCI_INTR_SF, &ohci->regs->intrenable);	
		(void)readl (&ohci->regs->intrdisable); /* PCI posting flush */
	}
#endif

	spin_unlock_irqrestore (&usb_ed_lock, flags);

	return 0;	
}

/*-------------------------------------------------------------------------*/

/* deactivate all TDs and remove the private part of the URB */
/* interrupt callers must use async unlink mode */

static int sohci_unlink_urb (USB_packet_s * urb)
{
	unsigned long flags;
	ohci_t * ohci;
	
	if (!urb) /* just to be sure */ 
		return -EINVAL;
		
	if (!urb->psDevice || !urb->psDevice->psBus)
		return -ENODEV;

	ohci = (ohci_t *) urb->psDevice->psBus->pHCPrivate; 

#ifdef DEBUG
	urb_print (urb, "UNLINK", 1);
#endif		  

	/* handle a request to the virtual root hub */
	if (usb_pipedevice (urb->nPipe) == ohci->rh.devnum)
		return rh_unlink_urb (urb);

	if (urb->pHCPrivate && (urb->nStatus == -EINPROGRESS)) { 
		if (!ohci->disabled) {
			urb_priv_t  * urb_priv;

			/* interrupt code may not sleep; it must use
			 * async status return to unlink pending urbs.
			 */
			#if 0
			if (!(urb->nTransferFlags & USB_ASYNC_UNLINK)
					&& in_interrupt ()) {
				err ("bug in call from %p; use async!",
					__builtin_return_address(0));
				return -EWOULDBLOCK;
			}
			#endif

			/* flag the urb and its TDs for deletion in some
			 * upcoming SF interrupt delete list processing
			 */
			spin_lock_irqsave (&usb_ed_lock, flags);
			urb_priv = urb->pHCPrivate;

			if (!urb_priv || (urb_priv->state == URB_DEL)) {
				spin_unlock_irqrestore (&usb_ed_lock, flags);
				return 0;
			}
				
			urb_priv->state = URB_DEL; 
			ep_rm_ed (urb->psDevice, urb_priv->ed);
			urb_priv->ed->state |= ED_URB_DEL;

			if (!(urb->nTransferFlags & USB_ASYNC_UNLINK)) {
				//DECLARE_WAIT_QUEUE_HEAD (unlink_wakeup); 
				//DECLARE_WAITQUEUE (wait, current);
				int timeout = OHCI_UNLINK_TIMEOUT;

				//add_wait_queue (&unlink_wakeup, &wait);
				urb_priv->wait = create_semaphore( "urb_delete_wait", 0, 0 );
				spin_unlock_irqrestore (&usb_ed_lock, flags);

				/* wait until all TDs are deleted */
				sleep_on_sem( urb_priv->wait, timeout * 1000 );
				#if 0
				set_current_state(TASK_UNINTERRUPTIBLE);
				while (timeout && (urb->status == USB_ST_URB_PENDING)) {
					timeout = schedule_timeout (timeout);
					set_current_state(TASK_UNINTERRUPTIBLE);
				}
				set_current_state(TASK_RUNNING);
				remove_wait_queue (&unlink_wakeup, &wait); 
				#endif
				delete_semaphore( urb_priv->wait );
				urb_priv->wait = -1;
				if (urb->nStatus == -EINPROGRESS) {
					printk("OHCI: unlink URB timeout\n");
					return -ETIMEDOUT;
				}
			} else {
				/* usb_dec_dev_use done in dl_del_list() */
				urb->nStatus = -EINPROGRESS;
				spin_unlock_irqrestore (&usb_ed_lock, flags);
				return -EINPROGRESS;
			}
		} else {
			urb_rm_priv (urb);
			if (urb->nTransferFlags & USB_ASYNC_UNLINK) {
				urb->nStatus = -ECONNRESET;
				if (urb->pComplete)
					urb->pComplete (urb); 
			} else 
				urb->nStatus = -ENOENT;
		}	
	}	
	return 0;
}

/*-------------------------------------------------------------------------*/

/* allocate private data space for a usb device */

static int sohci_alloc_dev (USB_device_s *usb_dev)
{
	struct ohci_device * dev;

	dev = dev_alloc ((struct ohci *) usb_dev->psBus->pHCPrivate, ALLOC_FLAGS);
	if (!dev)
		return -ENOMEM;

	usb_dev->pHCPrivate = dev;
	return 0;
}

/*-------------------------------------------------------------------------*/

/* may be called from interrupt context */
/* frees private data space of usb device */
  
static int sohci_free_dev (USB_device_s * usb_dev)
{
	unsigned long flags;
	int i, cnt = 0;
	ed_t * ed;
	struct ohci_device * dev = usb_to_ohci (usb_dev);
	ohci_t * ohci = usb_dev->psBus->pHCPrivate;
	
	if (!dev)
		return 0;
	
	if (usb_dev->nDeviceNum >= 0) {
	
		/* driver disconnects should have unlinked all urbs
		 * (freeing all the TDs, unlinking EDs) but we need
		 * to defend against bugs that prevent that.
		 */
		spin_lock_irqsave (&usb_ed_lock, flags);	
		for(i = 0; i < NUM_EDS; i++) {
  			ed = &(dev->ed[i]);
  			if (ed->state != ED_NEW) {
  				if (ed->state == ED_OPER) {
					/* driver on that interface didn't unlink an urb */
					kerndbg( KERN_DEBUG, "driver usb-%i dev %d ed 0x%x unfreed URB\n",
						ohci->ohci_dev.nDevice, usb_dev->nDeviceNum, i);
					ep_unlink (ohci, ed);
				}
  				ep_rm_ed (usb_dev, ed);
  				ed->state = ED_DEL;
  				cnt++;
  			}
  		}
  		spin_unlock_irqrestore (&usb_ed_lock, flags);
  		
		/* if the controller is running, tds for those unlinked
		 * urbs get freed by dl_del_list at the next SF interrupt
		 */
		if (cnt > 0) {

			if (ohci->disabled) {
				/* FIXME: Something like this should kick in,
				 * though it's currently an exotic case ...
				 * the controller won't ever be touching
				 * these lists again!!
				dl_del_list (ohci,
					le16_to_cpu (ohci->hcca->frame_no) & 1);
				 */
				kerndbg( KERN_WARNING, "TD leak, %d\n", cnt);

			} else if (1/*!in_interrupt ()*/) {
				//DECLARE_WAIT_QUEUE_HEAD (freedev_wakeup); 
				//DECLARE_WAITQUEUE (wait, current);
				int timeout = OHCI_UNLINK_TIMEOUT;

				/* SF interrupt handler calls dl_del_list */
				//add_wait_queue (&freedev_wakeup, &wait);
				//dev->wait = &freedev_wakeup;
				dev->wait = create_semaphore( "dev_free_wait", 0, 0 );
				sleep_on_sem( dev->wait, timeout * 1000 );
				#if 0
				set_current_state(TASK_UNINTERRUPTIBLE);
				while (timeout && dev->ed_cnt)
					timeout = schedule_timeout (timeout);
				set_current_state(TASK_RUNNING);
				remove_wait_queue (&freedev_wakeup, &wait);
				#endif
				delete_semaphore( dev->wait );
				dev->wait = -1;
				
				if (dev->ed_cnt) {
					printk("OHCI: free device %d timeout\n", usb_dev->nDeviceNum);
					return -ETIMEDOUT;
				}
			} 
			#if 0
			else {
				/* likely some interface's driver has a refcount bug */
				err ("bus %s devnum %d deletion in interrupt",
					ohci->ohci_dev->slot_name, usb_dev->devnum);
				BUG ();
			}
			#endif
		}
	}

	/* free device, and associated EDs */
	dev_free (ohci, dev);

	return 0;
}

/*-------------------------------------------------------------------------*/

/* tell us the current USB frame number */

static int sohci_get_current_frame_number (USB_device_s *usb_dev) 
{
	ohci_t * ohci = usb_dev->psBus->pHCPrivate;
	
	return le16_to_cpu (ohci->hcca->frame_no);
}

/*-------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------*
 * ED handling functions
 *-------------------------------------------------------------------------*/  
		
/* search for the right branch to insert an interrupt ed into the int tree 
 * do some load ballancing;
 * returns the branch and 
 * sets the interval to interval = 2^integer (ld (interval)) */

static int ep_int_ballance (ohci_t * ohci, int interval, int load)
{
	int i, branch = 0;
   
	/* search for the least loaded interrupt endpoint branch of all 32 branches */
	for (i = 0; i < 32; i++) 
		if (ohci->ohci_int_load [branch] > ohci->ohci_int_load [i]) branch = i; 
  
	branch = branch % interval;
	for (i = branch; i < 32; i += interval) ohci->ohci_int_load [i] += load;

	return branch;
}

/*-------------------------------------------------------------------------*/

/*  2^int( ld (inter)) */

static int ep_2_n_interval (int inter)
{	
	int i;
	for (i = 0; ((inter >> i) > 1 ) && (i < 5); i++); 
	return 1 << i;
}

/*-------------------------------------------------------------------------*/

/* the int tree is a binary tree 
 * in order to process it sequentially the indexes of the branches have to be mapped 
 * the mapping reverses the bits of a word of num_bits length */
 
static int ep_rev (int num_bits, int word)
{
	int i, wout = 0;

	for (i = 0; i < num_bits; i++) wout |= (((word >> i) & 1) << (num_bits - i - 1));
	return wout;
}

/*-------------------------------------------------------------------------*/

/* link an ed into one of the HC chains */

static int ep_link (ohci_t * ohci, ed_t * edi)
{	 
	int int_branch;
	int i;
	int inter;
	int interval;
	int load;
	__u32 * ed_p;
	volatile ed_t * ed = edi;
	
	ed->state = ED_OPER;
	
	switch (ed->type) {
	case USB_PIPE_CONTROL:
		ed->hwNextED = 0;
		if (ohci->ed_controltail == NULL) {
			writel (ed->dma, &ohci->regs->ed_controlhead);
		} else {
			ohci->ed_controltail->hwNextED = cpu_to_le32 (ed->dma);
		}
		ed->ed_prev = ohci->ed_controltail;
		if (!ohci->ed_controltail && !ohci->ed_rm_list[0] &&
			!ohci->ed_rm_list[1] && !ohci->sleeping) {
			ohci->hc_control |= OHCI_CTRL_CLE;
			writel (ohci->hc_control, &ohci->regs->control);
		}
		ohci->ed_controltail = edi;	  
		break;
		
	case USB_PIPE_BULK:
		ed->hwNextED = 0;
		if (ohci->ed_bulktail == NULL) {
			writel (ed->dma, &ohci->regs->ed_bulkhead);
		} else {
			ohci->ed_bulktail->hwNextED = cpu_to_le32 (ed->dma);
		}
		ed->ed_prev = ohci->ed_bulktail;
		if (!ohci->ed_bulktail && !ohci->ed_rm_list[0] &&
			!ohci->ed_rm_list[1] && !ohci->sleeping) {
			ohci->hc_control |= OHCI_CTRL_BLE;
			writel (ohci->hc_control, &ohci->regs->control);
		}
		ohci->ed_bulktail = edi;	  
		break;
		
	case USB_PIPE_INTERRUPT:
		load = ed->int_load;
		interval = ep_2_n_interval (ed->int_period);
		ed->int_interval = interval;
		int_branch = ep_int_ballance (ohci, interval, load);
		ed->int_branch = int_branch;
		
		for (i = 0; i < ep_rev (6, interval); i += inter) {
			inter = 1;
			for (ed_p = &(ohci->hcca->int_table[ep_rev (5, i) + int_branch]); 
				(*ed_p != 0) && ((dma_to_ed (ohci, le32_to_cpup (ed_p)))->int_interval >= interval); 
				ed_p = &((dma_to_ed (ohci, le32_to_cpup (ed_p)))->hwNextED)) 
					inter = ep_rev (6, (dma_to_ed (ohci, le32_to_cpup (ed_p)))->int_interval);
			ed->hwNextED = *ed_p; 
			*ed_p = cpu_to_le32 (ed->dma);
		}
#ifdef DEBUG
		ep_print_int_eds (ohci, "LINK_INT");
#endif
		break;
		
	case USB_PIPE_ISOCHRONOUS:
		ed->hwNextED = 0;
		ed->int_interval = 1;
		if (ohci->ed_isotail != NULL) {
			ohci->ed_isotail->hwNextED = cpu_to_le32 (ed->dma);
			ed->ed_prev = ohci->ed_isotail;
		} else {
			for ( i = 0; i < 32; i += inter) {
				inter = 1;
				for (ed_p = &(ohci->hcca->int_table[ep_rev (5, i)]); 
					*ed_p != 0; 
					ed_p = &((dma_to_ed (ohci, le32_to_cpup (ed_p)))->hwNextED)) 
						inter = ep_rev (6, (dma_to_ed (ohci, le32_to_cpup (ed_p)))->int_interval);
				*ed_p = cpu_to_le32 (ed->dma);	
			}	
			ed->ed_prev = NULL;
		}	
		ohci->ed_isotail = edi;  
#ifdef DEBUG
		ep_print_int_eds (ohci, "LINK_ISO");
#endif
		break;
	}	 	
	return 0;
}

/*-------------------------------------------------------------------------*/

/* scan the periodic table to find and unlink this ED */
static void periodic_unlink (
	struct ohci	*ohci,
	struct ed	*ed,
	unsigned	index,
	unsigned	period
) {
	for (; index < NUM_INTS; index += period) {
		__u32	*ed_p = &ohci->hcca->int_table [index];

		/* ED might have been unlinked through another path */
		while (*ed_p != 0) {
			if ((dma_to_ed (ohci, le32_to_cpup (ed_p))) == ed) {
				*ed_p = ed->hwNextED;		
				break;
			}
			ed_p = & ((dma_to_ed (ohci,
				le32_to_cpup (ed_p)))->hwNextED);
		}
	}	
}

/* unlink an ed from one of the HC chains. 
 * just the link to the ed is unlinked.
 * the link from the ed still points to another operational ed or 0
 * so the HC can eventually finish the processing of the unlinked ed */

static int ep_unlink (ohci_t * ohci, ed_t * ed) 
{
	int i;

	ed->hwINFO |= cpu_to_le32 (OHCI_ED_SKIP);

	switch (ed->type) {
	case USB_PIPE_CONTROL:
		if (ed->ed_prev == NULL) {
			if (!ed->hwNextED) {
				ohci->hc_control &= ~OHCI_CTRL_CLE;
				writel (ohci->hc_control, &ohci->regs->control);
			}
			writel (le32_to_cpup (&ed->hwNextED), &ohci->regs->ed_controlhead);
		} else {
			ed->ed_prev->hwNextED = ed->hwNextED;
		}
		if (ohci->ed_controltail == ed) {
			ohci->ed_controltail = ed->ed_prev;
		} else {
			(dma_to_ed (ohci, le32_to_cpup (&ed->hwNextED)))->ed_prev = ed->ed_prev;
		}
		break;
      
	case USB_PIPE_BULK:
		if (ed->ed_prev == NULL) {
			if (!ed->hwNextED) {
				ohci->hc_control &= ~OHCI_CTRL_BLE;
				writel (ohci->hc_control, &ohci->regs->control);
			}
			writel (le32_to_cpup (&ed->hwNextED), &ohci->regs->ed_bulkhead);
		} else {
			ed->ed_prev->hwNextED = ed->hwNextED;
		}
		if (ohci->ed_bulktail == ed) {
			ohci->ed_bulktail = ed->ed_prev;
		} else {
			(dma_to_ed (ohci, le32_to_cpup (&ed->hwNextED)))->ed_prev = ed->ed_prev;
		}
		break;
      
	case USB_PIPE_INTERRUPT:
		periodic_unlink (ohci, ed, 0, 1);
		for (i = ed->int_branch; i < 32; i += ed->int_interval)
		    ohci->ohci_int_load[i] -= ed->int_load;
#ifdef DEBUG
		ep_print_int_eds (ohci, "UNLINK_INT");
#endif
		break;
		
	case USB_PIPE_ISOCHRONOUS:
		if (ohci->ed_isotail == ed)
			ohci->ed_isotail = ed->ed_prev;
		if (ed->hwNextED != 0) 
		    (dma_to_ed (ohci, le32_to_cpup (&ed->hwNextED)))
		    	->ed_prev = ed->ed_prev;
				    
		if (ed->ed_prev != NULL)
			ed->ed_prev->hwNextED = ed->hwNextED;
		else
			periodic_unlink (ohci, ed, 0, 1);
#ifdef DEBUG
		ep_print_int_eds (ohci, "UNLINK_ISO");
#endif
		break;
	}
	ed->state = ED_UNLINK;
	return 0;
}


/*-------------------------------------------------------------------------*/

/* add/reinit an endpoint; this should be done once at the usb_set_configuration command,
 * but the USB stack is a little bit stateless  so we do it at every transaction
 * if the state of the ed is ED_NEW then a dummy td is added and the state is changed to ED_UNLINK
 * in all other cases the state is left unchanged
 * the ed info fields are setted anyway even though most of them should not change */
 
static ed_t * ep_add_ed (
	USB_device_s * usb_dev,
	unsigned int pipe,
	int interval,
	int load,
	int mem_flags
)
{
   	ohci_t * ohci = usb_dev->psBus->pHCPrivate;
	td_t * td;
	ed_t * ed_ret;
	volatile ed_t * ed; 
	unsigned long flags;
 	
 	
	spin_lock_irqsave (&usb_ed_lock, flags);

	ed = ed_ret = &(usb_to_ohci (usb_dev)->ed[(usb_pipeendpoint (pipe) << 1) | 
			(usb_pipecontrol (pipe)? 0: usb_pipeout (pipe))]);

	if ((ed->state & ED_DEL) || (ed->state & ED_URB_DEL)) {
		/* pending delete request */
		spin_unlock_irqrestore (&usb_ed_lock, flags);
		return NULL;
	}
	
	if (ed->state == ED_NEW) {
		ed->hwINFO = cpu_to_le32 (OHCI_ED_SKIP); /* skip ed */
  		/* dummy td; end of td list for ed */
		td = td_alloc (ohci, MEMF_KERNEL);
		/* hash the ed for later reverse mapping */
 		if (!td || !hash_add_ed (ohci, (ed_t *)ed)) {
			/* out of memory */
		        if (td)
		            td_free(ohci, td);
			spin_unlock_irqrestore (&usb_ed_lock, flags);
			return NULL;
		}
		ed->hwTailP = cpu_to_le32 (td->td_dma);
		ed->hwHeadP = ed->hwTailP;	
		ed->state = ED_UNLINK;
		ed->type = usb_pipetype (pipe);
		usb_to_ohci (usb_dev)->ed_cnt++;
	}

	ohci->dev[usb_pipedevice (pipe)] = usb_dev;
	
	ed->hwINFO = cpu_to_le32 (usb_pipedevice (pipe)
			| usb_pipeendpoint (pipe) << 7
			| (usb_pipeisoc (pipe)? 0x8000: 0)
			| (usb_pipecontrol (pipe)? 0: (usb_pipeout (pipe)? 0x800: 0x1000)) 
			| usb_pipeslow (pipe) << 13
			| usb_maxpacket (usb_dev, pipe, usb_pipeout (pipe)) << 16);
  
  	if (ed->type == USB_PIPE_INTERRUPT && ed->state == ED_UNLINK) {
  		ed->int_period = interval;
  		ed->int_load = load;
  	}
  	
	spin_unlock_irqrestore (&usb_ed_lock, flags);
	return ed_ret; 
}

/*-------------------------------------------------------------------------*/
 
/* request the removal of an endpoint
 * put the ep on the rm_list and request a stop of the bulk or ctrl list 
 * real removal is done at the next start frame (SF) hardware interrupt */
 
static void ep_rm_ed (USB_device_s * usb_dev, ed_t * ed)
{    
	unsigned int frame;
	ohci_t * ohci = usb_dev->psBus->pHCPrivate;

	if ((ed->state & ED_DEL) || (ed->state & ED_URB_DEL))
		return;
	
	ed->hwINFO |= cpu_to_le32 (OHCI_ED_SKIP);

	if (!ohci->disabled) {
		switch (ed->type) {
			case USB_PIPE_CONTROL: /* stop control list */
				ohci->hc_control &= ~OHCI_CTRL_CLE;
				writel (ohci->hc_control, &ohci->regs->control); 
  				break;
			case USB_PIPE_BULK: /* stop bulk list */
				ohci->hc_control &= ~OHCI_CTRL_BLE;
				writel (ohci->hc_control, &ohci->regs->control); 
				break;
		}
	}

	frame = le16_to_cpu (ohci->hcca->frame_no) & 0x1;
	ed->ed_rm_list = ohci->ed_rm_list[frame];
	ohci->ed_rm_list[frame] = ed;

	if (!ohci->disabled && !ohci->sleeping) {
		/* enable SOF interrupt */
		writel (OHCI_INTR_SF, &ohci->regs->intrstatus);
		writel (OHCI_INTR_SF, &ohci->regs->intrenable);
		(void)readl (&ohci->regs->intrdisable); /* PCI posting flush */
	}
}

/*-------------------------------------------------------------------------*
 * TD handling functions
 *-------------------------------------------------------------------------*/

/* enqueue next TD for this URB (OHCI spec 5.2.8.2) */

static void
td_fill (ohci_t * ohci, unsigned int info,
	dma_addr_t data, int len,
	USB_packet_s * urb, int index)
{
	volatile td_t  * td, * td_pt;
	urb_priv_t * urb_priv = urb->pHCPrivate;
	
	if (index >= urb_priv->length) {
		printk("internal OHCI error: TD index > length\n");
		return;
	}
	
	/* use this td as the next dummy */
	td_pt = urb_priv->td [index];
	td_pt->hwNextTD = 0;

	/* fill the old dummy TD */
	td = urb_priv->td [index] = dma_to_td (ohci,
			le32_to_cpup (&urb_priv->ed->hwTailP) & ~0xf);

	td->ed = urb_priv->ed;
	td->next_dl_td = NULL;
	td->index = index;
	td->urb = urb; 
	td->data_dma = data;
	if (!len)
		data = 0;

	td->hwINFO = cpu_to_le32 (info);
	if ((td->ed->type) == USB_PIPE_ISOCHRONOUS) {
		td->hwCBP = cpu_to_le32 (data & 0xFFFFF000);
		td->ed->last_iso = info & 0xffff;
	} else {
		td->hwCBP = cpu_to_le32 (data); 
	}			
	if (data)
		td->hwBE = cpu_to_le32 (data + len - 1);
	else
		td->hwBE = 0;
	td->hwNextTD = cpu_to_le32 (td_pt->td_dma);
	td->hwPSW [0] = cpu_to_le16 ((data & 0x0FFF) | 0xE000);

	/* append to queue */
	wmb();
	td->ed->hwTailP = td->hwNextTD;
}

/*-------------------------------------------------------------------------*/
 
/* prepare all TDs of a transfer */

static void td_submit_urb (USB_packet_s * urb)
{ 
	urb_priv_t * urb_priv = urb->pHCPrivate;
	ohci_t * ohci = (ohci_t *) urb->psDevice->psBus->pHCPrivate;
	dma_addr_t data;
	int data_len = urb->nBufferLength;
	int maxps = usb_maxpacket (urb->psDevice, urb->nPipe, usb_pipeout (urb->nPipe));
	int cnt = 0; 
	__u32 info = 0;
  	unsigned int toggle = 0;

	/* OHCI handles the DATA-toggles itself, we just use the USB-toggle bits for reseting */
  	if(usb_gettoggle(urb->psDevice, usb_pipeendpoint(urb->nPipe), usb_pipeout(urb->nPipe))) {
  		toggle = TD_T_TOGGLE;
	} else {
  		toggle = TD_T_DATA0;
		usb_settoggle(urb->psDevice, usb_pipeendpoint(urb->nPipe), usb_pipeout(urb->nPipe), 1);
	}
	
	urb_priv->td_cnt = 0;

	if (data_len) {
		data = (dma_addr_t)urb->pBuffer;
		#if 0
		data = pci_map_single (ohci->ohci_dev,
			urb->transfer_buffer, data_len,
			usb_pipeout (urb->nPipe)
				? PCI_DMA_TODEVICE
				: PCI_DMA_FROMDEVICE
			);
		#endif
	} else
		data = 0;
	
	switch (usb_pipetype (urb->nPipe)) {
		case USB_PIPE_BULK:
			info = usb_pipeout (urb->nPipe)? 
				TD_CC | TD_DP_OUT : TD_CC | TD_DP_IN ;
			while(data_len > 4096) {		
				td_fill (ohci, info | (cnt? TD_T_TOGGLE:toggle), data, 4096, urb, cnt);
				data += 4096; data_len -= 4096; cnt++;
			}
			info = usb_pipeout (urb->nPipe)?
				TD_CC | TD_DP_OUT : TD_CC | TD_R | TD_DP_IN ;
			td_fill (ohci, info | (cnt? TD_T_TOGGLE:toggle), data, data_len, urb, cnt);
			cnt++;

			/* If the transfer size is multiple of the pipe mtu,
			 * we may need an extra TD to create a empty frame
			 * Note : another way to check this condition is
			 * to test if(urb_priv->length > cnt) - Jean II */
			if ((urb->nTransferFlags & USB_ZERO_PACKET) &&
			    usb_pipeout (urb->nPipe) &&
			    (urb->nBufferLength != 0) && 
			    ((urb->nBufferLength % maxps) == 0)) {
				td_fill (ohci, info | (cnt? TD_T_TOGGLE:toggle), 0, 0, urb, cnt);
				cnt++;
			}

			if (!ohci->sleeping) {
				wmb();
				writel (OHCI_BLF, &ohci->regs->cmdstatus); /* start bulk list */
				(void)readl (&ohci->regs->intrdisable); /* PCI posting flush */
			}
			break;

		case USB_PIPE_INTERRUPT:
			info = usb_pipeout (urb->nPipe)? 
				TD_CC | TD_DP_OUT | toggle: TD_CC | TD_R | TD_DP_IN | toggle;
			td_fill (ohci, info, data, data_len, urb, cnt++);
			break;

		case USB_PIPE_CONTROL:
			info = TD_CC | TD_DP_SETUP | TD_T_DATA0;
			td_fill (ohci, info, (dma_addr_t)urb->pSetupPacket/*
				pci_map_single (ohci->ohci_dev,
					urb->setup_packet, 8,
					PCI_DMA_TODEVICE)*/,
				8, urb, cnt++); 
			if (data_len > 0) {  
				info = usb_pipeout (urb->nPipe)? 
					TD_CC | TD_R | TD_DP_OUT | TD_T_DATA1 : TD_CC | TD_R | TD_DP_IN | TD_T_DATA1;
				/* NOTE:  mishandles transfers >8K, some >4K */
				td_fill (ohci, info, data, data_len, urb, cnt++);  
			} 
			info = usb_pipeout (urb->nPipe)? 
 				TD_CC | TD_DP_IN | TD_T_DATA1: TD_CC | TD_DP_OUT | TD_T_DATA1;
			td_fill (ohci, info, data, 0, urb, cnt++);
			if (!ohci->sleeping) {
				wmb();
				writel (OHCI_CLF, &ohci->regs->cmdstatus); /* start Control list */
				(void)readl (&ohci->regs->intrdisable); /* PCI posting flush */
			}
			break;

		case USB_PIPE_ISOCHRONOUS:
			for (cnt = 0; cnt < urb->nPacketNum; cnt++) {
				td_fill (ohci, TD_CC|TD_ISO | ((urb->nStartFrame + cnt) & 0xffff), 
					data + urb->sISOFrame[cnt].nOffset, 
					urb->sISOFrame[cnt].nLength, urb, cnt); 
			}
			break;
	} 
	if (urb_priv->length != cnt) 
		printk("TD LENGTH %d != CNT %d\n", urb_priv->length, cnt);
}

/*-------------------------------------------------------------------------*
 * Done List handling functions
 *-------------------------------------------------------------------------*/


/* calculate the transfer length and update the urb */

static void dl_transfer_length(td_t * td)
{
	__u32 tdINFO, tdBE, tdCBP;
 	__u16 tdPSW;
 	USB_packet_s * urb = td->urb;
 	urb_priv_t * urb_priv = urb->pHCPrivate;
	int dlen = 0;
	int cc = 0;
	
	tdINFO = le32_to_cpup (&td->hwINFO);
  	tdBE   = le32_to_cpup (&td->hwBE);
  	tdCBP  = le32_to_cpup (&td->hwCBP);


  	if (tdINFO & TD_ISO) {
 		tdPSW = le16_to_cpu (td->hwPSW[0]);
 		cc = (tdPSW >> 12) & 0xF;
		if (cc < 0xE)  {
			if (usb_pipeout(urb->nPipe)) {
				dlen = urb->sISOFrame[td->index].nLength;
			} else {
				dlen = tdPSW & 0x3ff;
			}
			urb->nActualLength += dlen;
			urb->sISOFrame[td->index].nActualLength = dlen;
			if (!(urb->nTransferFlags & USB_DISABLE_SPD) && (cc == TD_DATAUNDERRUN))
				cc = TD_CC_NOERROR;
					 
			urb->sISOFrame[td->index].nStatus = cc_to_error[cc];
		}
	} else { /* BULK, INT, CONTROL DATA */
		if (!(usb_pipetype (urb->nPipe) == USB_PIPE_CONTROL && 
				((td->index == 0) || (td->index == urb_priv->length - 1)))) {
 			if (tdBE != 0) {
 				if (td->hwCBP == 0)
					urb->nActualLength += tdBE - td->data_dma + 1;
  				else
					urb->nActualLength += tdCBP - td->data_dma;
			}
  		}
  	}
}

/* handle an urb that is being unlinked */

static void dl_del_urb (USB_packet_s * urb)
{
	sem_id wait = ((urb_priv_t *)(urb->pHCPrivate))->wait;
	//wait_queue_head_t * wait_head = ((urb_priv_t *)(urb->pHCPrivate))->wait;

	urb_rm_priv_locked (urb);

	if (urb->nTransferFlags & USB_ASYNC_UNLINK) {
		urb->nStatus = -ECONNRESET;
		if (urb->pComplete)
			urb->pComplete (urb);
	} else {
		urb->nStatus = -ENOENT;
		if (urb->pComplete)
			urb->pComplete (urb);

		/* unblock sohci_unlink_urb */
		if( wait >= 0 )
			wakeup_sem( wait, false );
		#if 0
		if (wait_head)
			wake_up (wait_head);
		#endif
	}
}

/*-------------------------------------------------------------------------*/

/* replies to the request have to be on a FIFO basis so
 * we reverse the reversed done-list */
 
static td_t * dl_reverse_done_list (ohci_t * ohci)
{
	__u32 td_list_hc;
	td_t * td_rev = NULL;
	td_t * td_list = NULL;
  	urb_priv_t * urb_priv = NULL;
  	unsigned long flags;
  	
  	spin_lock_irqsave (&usb_ed_lock, flags);
  	
	td_list_hc = le32_to_cpup (&ohci->hcca->done_head) & 0xfffffff0;
	ohci->hcca->done_head = 0;
	
	while (td_list_hc) {		
		td_list = dma_to_td (ohci, td_list_hc);

		if (TD_CC_GET (le32_to_cpup (&td_list->hwINFO))) {
			urb_priv = (urb_priv_t *) td_list->urb->pHCPrivate;
			//printk(" USB-error/status: %x : %p\n", 
			//		TD_CC_GET (le32_to_cpup (&td_list->hwINFO)), td_list);
			if (td_list->ed->hwHeadP & cpu_to_le32 (0x1)) {
				if (urb_priv && ((td_list->index + 1) < urb_priv->length)) {
					td_list->ed->hwHeadP = 
						(urb_priv->td[urb_priv->length - 1]->hwNextTD & cpu_to_le32 (0xfffffff0)) |
									(td_list->ed->hwHeadP & cpu_to_le32 (0x2));
					urb_priv->td_cnt += urb_priv->length - td_list->index - 1;
				} else 
					td_list->ed->hwHeadP &= cpu_to_le32 (0xfffffff2);
			}
		}

		td_list->next_dl_td = td_rev;	
		td_rev = td_list;
		td_list_hc = le32_to_cpup (&td_list->hwNextTD) & 0xfffffff0;	
	}	
	spin_unlock_irqrestore (&usb_ed_lock, flags);
	return td_list;
}

/*-------------------------------------------------------------------------*/

/* there are some pending requests to remove 
 * - some of the eds (if ed->state & ED_DEL (set by sohci_free_dev)
 * - some URBs/TDs if urb_priv->state == URB_DEL */
 
static void dl_del_list (ohci_t  * ohci, unsigned int frame)
{
	unsigned long flags;
	ed_t * ed;
	__u32 edINFO;
	__u32 tdINFO;
	td_t * td = NULL, * td_next = NULL, * tdHeadP = NULL, * tdTailP;
	__u32 * td_p;
	int ctrl = 0, bulk = 0;

	spin_lock_irqsave (&usb_ed_lock, flags);

	for (ed = ohci->ed_rm_list[frame]; ed != NULL; ed = ed->ed_rm_list) {

		tdTailP = dma_to_td (ohci, le32_to_cpup (&ed->hwTailP) & 0xfffffff0);
		tdHeadP = dma_to_td (ohci, le32_to_cpup (&ed->hwHeadP) & 0xfffffff0);
		edINFO = le32_to_cpup (&ed->hwINFO);
		td_p = &ed->hwHeadP;

		for (td = tdHeadP; td != tdTailP; td = td_next) { 
			USB_packet_s * urb = td->urb;
			urb_priv_t * urb_priv = td->urb->pHCPrivate;
			
			td_next = dma_to_td (ohci, le32_to_cpup (&td->hwNextTD) & 0xfffffff0);
			if ((urb_priv->state == URB_DEL) || (ed->state & ED_DEL)) {
				tdINFO = le32_to_cpup (&td->hwINFO);
				if (TD_CC_GET (tdINFO) < 0xE)
					dl_transfer_length (td);
				*td_p = td->hwNextTD | (*td_p & cpu_to_le32 (0x3));

				/* URB is done; clean up */
				if (++(urb_priv->td_cnt) == urb_priv->length)
					dl_del_urb (urb);
			} else {
				td_p = &td->hwNextTD;
			}
		}

		if (ed->state & ED_DEL) { /* set by sohci_free_dev */
			struct ohci_device * dev = usb_to_ohci (ohci->dev[edINFO & 0x7F]);
			td_free (ohci, tdTailP); /* free dummy td */
   	 		ed->hwINFO = cpu_to_le32 (OHCI_ED_SKIP); 
			ed->state = ED_NEW;
			hash_free_ed(ohci, ed);
   	 		/* if all eds are removed wake up sohci_free_dev */
   	 		if (!--dev->ed_cnt) {
   	 			sem_id wait = dev->wait;
				//wait_queue_head_t *wait_head = dev->wait;

				dev->wait = -1;
				if (wait >= 0)
					wakeup_sem (wait, false);
			}
   	 	} else {
   	 		ed->state &= ~ED_URB_DEL;
			tdHeadP = dma_to_td (ohci, le32_to_cpup (&ed->hwHeadP) & 0xfffffff0);

			if (tdHeadP == tdTailP) {
				if (ed->state == ED_OPER)
					ep_unlink(ohci, ed);
			} else
   	 			ed->hwINFO &= ~cpu_to_le32 (OHCI_ED_SKIP);
   	 	}

		switch (ed->type) {
			case USB_PIPE_CONTROL:
				ctrl = 1;
				break;
			case USB_PIPE_BULK:
				bulk = 1;
				break;
		}
   	}
   	
	/* maybe reenable control and bulk lists */ 
	if (!ohci->disabled) {
		if (ctrl) 	/* reset control list */
			writel (0, &ohci->regs->ed_controlcurrent);
		if (bulk)	/* reset bulk list */
			writel (0, &ohci->regs->ed_bulkcurrent);
		if (!ohci->ed_rm_list[!frame] && !ohci->sleeping) {
			if (ohci->ed_controltail)
				ohci->hc_control |= OHCI_CTRL_CLE;
			if (ohci->ed_bulktail)
				ohci->hc_control |= OHCI_CTRL_BLE;
			writel (ohci->hc_control, &ohci->regs->control);   
		}
	}

   	ohci->ed_rm_list[frame] = NULL;
   	spin_unlock_irqrestore (&usb_ed_lock, flags);
}


  		
/*-------------------------------------------------------------------------*/

/* td done list */

static void dl_done_list (ohci_t * ohci, td_t * td_list)
{
  	td_t * td_list_next = NULL;
	ed_t * ed;
	int cc = 0;
	USB_packet_s * urb;
	urb_priv_t * urb_priv;
 	__u32 tdINFO, edHeadP, edTailP;
 	
 	unsigned long flags;
 	
  	while (td_list) {
   		td_list_next = td_list->next_dl_td;
   		
  		urb = td_list->urb;
  		urb_priv = urb->pHCPrivate;
  		tdINFO = le32_to_cpup (&td_list->hwINFO);
  		
   		ed = td_list->ed;
   		
   		dl_transfer_length(td_list);
 			
  		/* error code of transfer */
  		cc = TD_CC_GET (tdINFO);
  		if (cc == TD_CC_STALL)
			usb_endpoint_halt(urb->psDevice,
				usb_pipeendpoint(urb->nPipe),
				usb_pipeout(urb->nPipe));
  		
  		if (!(urb->nTransferFlags & USB_DISABLE_SPD)
				&& (cc == TD_DATAUNDERRUN))
			cc = TD_CC_NOERROR;

  		if (++(urb_priv->td_cnt) == urb_priv->length) {
			if ((ed->state & (ED_OPER | ED_UNLINK))
					&& (urb_priv->state != URB_DEL)) {
  				urb->nStatus = cc_to_error[cc];
  				sohci_return_urb (ohci, urb);
  			} else {
				spin_lock_irqsave (&usb_ed_lock, flags);
  				dl_del_urb (urb);
				spin_unlock_irqrestore (&usb_ed_lock, flags);
			}
  		}
  		
  		spin_lock_irqsave (&usb_ed_lock, flags);
  		if (ed->state != ED_NEW) { 
  			edHeadP = le32_to_cpup (&ed->hwHeadP) & 0xfffffff0;
  			edTailP = le32_to_cpup (&ed->hwTailP);

			/* unlink eds if they are not busy */
     			if ((edHeadP == edTailP) && (ed->state == ED_OPER)) 
     				ep_unlink (ohci, ed);
     		}	
     		spin_unlock_irqrestore (&usb_ed_lock, flags);
     	
    		td_list = td_list_next;
  	}  
}




/*-------------------------------------------------------------------------*
 * Virtual Root Hub 
 *-------------------------------------------------------------------------*/
 
/* Device descriptor */
static __u8 root_hub_dev_des[] =
{
	0x12,       /*  __u8  bLength; */
	0x01,       /*  __u8  bDescriptorType; Device */
	0x10,	    /*  __u16 bcdUSB; v1.1 */
	0x01,
	0x09,	    /*  __u8  bDeviceClass; HUB_CLASSCODE */
	0x00,	    /*  __u8  bDeviceSubClass; */
	0x00,       /*  __u8  bDeviceProtocol; */
	0x08,       /*  __u8  bMaxPacketSize0; 8 Bytes */
	0x00,       /*  __u16 idVendor; */
	0x00,
	0x00,       /*  __u16 idProduct; */
 	0x00,
	0x00,       /*  __u16 bcdDevice; */
 	0x00,
	0x00,       /*  __u8  iManufacturer; */
	0x02,       /*  __u8  iProduct; */
	0x01,       /*  __u8  iSerialNumber; */
	0x01        /*  __u8  bNumConfigurations; */
};


/* Configuration descriptor */
static __u8 root_hub_config_des[] =
{
	0x09,       /*  __u8  bLength; */
	0x02,       /*  __u8  bDescriptorType; Configuration */
	0x19,       /*  __u16 wTotalLength; */
	0x00,
	0x01,       /*  __u8  bNumInterfaces; */
	0x01,       /*  __u8  bConfigurationValue; */
	0x00,       /*  __u8  iConfiguration; */
	0x40,       /*  __u8  bmAttributes; 
                 Bit 7: Bus-powered, 6: Self-powered, 5 Remote-wakwup, 4..0: resvd */
	0x00,       /*  __u8  MaxPower; */
      
	/* interface */	  
	0x09,       /*  __u8  if_bLength; */
	0x04,       /*  __u8  if_bDescriptorType; Interface */
	0x00,       /*  __u8  if_bInterfaceNumber; */
	0x00,       /*  __u8  if_bAlternateSetting; */
	0x01,       /*  __u8  if_bNumEndpoints; */
	0x09,       /*  __u8  if_bInterfaceClass; HUB_CLASSCODE */
	0x00,       /*  __u8  if_bInterfaceSubClass; */
	0x00,       /*  __u8  if_bInterfaceProtocol; */
	0x00,       /*  __u8  if_iInterface; */
     
	/* endpoint */
	0x07,       /*  __u8  ep_bLength; */
	0x05,       /*  __u8  ep_bDescriptorType; Endpoint */
	0x81,       /*  __u8  ep_bEndpointAddress; IN Endpoint 1 */
 	0x03,       /*  __u8  ep_bmAttributes; Interrupt */
 	0x02,       /*  __u16 ep_wMaxPacketSize; ((MAX_ROOT_PORTS + 1) / 8 */
 	0x00,
	0xff        /*  __u8  ep_bInterval; 255 ms */
};

/* Hub class-specific descriptor is constructed dynamically */


/*-------------------------------------------------------------------------*/

/* prepare Interrupt pipe data; HUB INTERRUPT ENDPOINT */ 
 
static int rh_send_irq (ohci_t * ohci, void * rh_data, int rh_len)
{
	int num_ports;
	int i;
	int ret;
	int len;

	__u8 data[8];

	num_ports = roothub_a (ohci) & RH_A_NDP; 
	if (num_ports > MAX_ROOT_PORTS) {
		printk("bogus NDP=%d for OHCI usb-%i\n", num_ports,
			ohci->ohci_dev.nDevice);
		printk("rereads as NDP=%d\n",
			readl (&ohci->regs->roothub.a) & RH_A_NDP);
		/* retry later; "should not happen" */
		return 0;
	}
	*(__u8 *) data = (roothub_status (ohci) & (RH_HS_LPSC | RH_HS_OCIC))
		? 1: 0;
	ret = *(__u8 *) data;

	for ( i = 0; i < num_ports; i++) {
		*(__u8 *) (data + (i + 1) / 8) |= 
			((roothub_portstatus (ohci, i) &
				(RH_PS_CSC | RH_PS_PESC | RH_PS_PSSC | RH_PS_OCIC | RH_PS_PRSC))
			    ? 1: 0) << ((i + 1) % 8);
		ret += *(__u8 *) (data + (i + 1) / 8);
	}
	len = i/8 + 1;
  
	if (ret > 0) { 
		memcpy(rh_data, data,
		       min(len,
			   min(rh_len, sizeof(data))));
		return len;
	}
	return 0;
}

/*-------------------------------------------------------------------------*/

/* Virtual Root Hub INTs are polled by this timer every "interval" ms */
 
static void rh_int_timer_do (void* ptr)
{
	int len; 

	USB_packet_s * urb = (USB_packet_s *) ptr;
	ohci_t * ohci = urb->psDevice->psBus->pHCPrivate;

	if (ohci->disabled)
		return;

	/* ignore timers firing during PM suspend, etc */
	if ((ohci->hc_control & OHCI_CTRL_HCFS) != OHCI_USB_OPER)
		goto out;

	if(ohci->rh.send) { 
		len = rh_send_irq (ohci, urb->pBuffer, urb->nBufferLength);
		if (len > 0) {
			urb->nActualLength = len;
#ifdef DEBUG
			urb_print (urb, "RET-t(rh)", usb_pipeout (urb->nPipe));
#endif
			if (urb->pComplete)
				urb->pComplete (urb);
		}
	}
 out:
 	delete_timer( ohci->rh.rh_int_timer );
 	
	rh_init_int_timer (urb);
}

/*-------------------------------------------------------------------------*/

/* Root Hub INTs are polled by this timer */

static int rh_init_int_timer (USB_packet_s * urb) 
{
	ohci_t * ohci = urb->psDevice->psBus->pHCPrivate;

	ohci->rh.interval = urb->nInterval;
	
	ohci->rh.rh_int_timer = create_timer();
	start_timer( ohci->rh.rh_int_timer, rh_int_timer_do, (void*)urb, (1000 * (urb->nInterval < 30? 30: urb->nInterval)),
				true );
				
	#if 0
	
	init_timer (&ohci->rh.rh_int_timer);
	ohci->rh.rh_int_timer.function = rh_int_timer_do;
	ohci->rh.rh_int_timer.data = (unsigned long) urb;
	ohci->rh.rh_int_timer.expires = 
			jiffies + (HZ * (urb->interval < 30? 30: urb->interval)) / 1000;
	add_timer (&ohci->rh.rh_int_timer);
	#endif
	
	return 0;
}

/*-------------------------------------------------------------------------*/

#define OK(x) 			len = (x); break
#define WR_RH_STAT(x) 		writel((x), &ohci->regs->roothub.status)
#define WR_RH_PORTSTAT(x) 	writel((x), &ohci->regs->roothub.portstatus[wIndex-1])
#define RD_RH_STAT		roothub_status(ohci)
#define RD_RH_PORTSTAT		roothub_portstatus(ohci,wIndex-1)

/* request to virtual root hub */

static int rh_submit_urb (USB_packet_s * urb)
{
	USB_device_s * usb_dev = urb->psDevice;
	ohci_t * ohci = usb_dev->psBus->pHCPrivate;
	unsigned int pipe = urb->nPipe;
	USB_request_s * cmd = (USB_request_s *) urb->pSetupPacket;
	void * data = urb->pBuffer;
	int leni = urb->nBufferLength;
	int len = 0;
	int status = TD_CC_NOERROR;
	
	__u32 datab[4];
	__u8  * data_buf = (__u8 *) datab;
	
 	__u16 bmRType_bReq;
	__u16 wValue; 
	__u16 wIndex;
	__u16 wLength;
	
	if (usb_pipeint(pipe)) {
		ohci->rh.urb =  urb;
		ohci->rh.send = 1;
		ohci->rh.interval = urb->nInterval;
		rh_init_int_timer(urb);
		urb->nStatus = cc_to_error [TD_CC_NOERROR];
		
		return 0;
	}

	bmRType_bReq  = cmd->nRequestType | (cmd->nRequest << 8);
	wValue        = le16_to_cpu (cmd->nValue);
	wIndex        = le16_to_cpu (cmd->nIndex);
	wLength       = le16_to_cpu (cmd->nLength);

	switch (bmRType_bReq) {
	/* Request Destination:
	   without flags: Device, 
	   RH_INTERFACE: interface, 
	   RH_ENDPOINT: endpoint,
	   RH_CLASS means HUB here, 
	   RH_OTHER | RH_CLASS  almost ever means HUB_PORT here 
	*/
  
		case RH_GET_STATUS: 				 		
				*(__u16 *) data_buf = cpu_to_le16 (1); OK (2);
		case RH_GET_STATUS | RH_INTERFACE: 	 		
				*(__u16 *) data_buf = cpu_to_le16 (0); OK (2);
		case RH_GET_STATUS | RH_ENDPOINT:	 		
				*(__u16 *) data_buf = cpu_to_le16 (0); OK (2);   
		case RH_GET_STATUS | RH_CLASS: 				
				*(__u32 *) data_buf = cpu_to_le32 (
					RD_RH_STAT & ~(RH_HS_CRWE | RH_HS_DRWE));
				OK (4);
		case RH_GET_STATUS | RH_OTHER | RH_CLASS: 	
				*(__u32 *) data_buf = cpu_to_le32 (RD_RH_PORTSTAT); OK (4);

		case RH_CLEAR_FEATURE | RH_ENDPOINT:  
			switch (wValue) {
				case (RH_ENDPOINT_STALL): OK (0);
			}
			break;

		case RH_CLEAR_FEATURE | RH_CLASS:
			switch (wValue) {
				case RH_C_HUB_LOCAL_POWER:
					OK(0);
				case (RH_C_HUB_OVER_CURRENT): 
						WR_RH_STAT(RH_HS_OCIC); OK (0);
			}
			break;
		
		case RH_CLEAR_FEATURE | RH_OTHER | RH_CLASS:
			switch (wValue) {
				case (RH_PORT_ENABLE): 			
						WR_RH_PORTSTAT (RH_PS_CCS ); OK (0);
				case (RH_PORT_SUSPEND):			
						WR_RH_PORTSTAT (RH_PS_POCI); OK (0);
				case (RH_PORT_POWER):			
						WR_RH_PORTSTAT (RH_PS_LSDA); OK (0);
				case (RH_C_PORT_CONNECTION):	
						WR_RH_PORTSTAT (RH_PS_CSC ); OK (0);
				case (RH_C_PORT_ENABLE):		
						WR_RH_PORTSTAT (RH_PS_PESC); OK (0);
				case (RH_C_PORT_SUSPEND):		
						WR_RH_PORTSTAT (RH_PS_PSSC); OK (0);
				case (RH_C_PORT_OVER_CURRENT):	
						WR_RH_PORTSTAT (RH_PS_OCIC); OK (0);
				case (RH_C_PORT_RESET):			
						WR_RH_PORTSTAT (RH_PS_PRSC); OK (0); 
			}
			break;
 
		case RH_SET_FEATURE | RH_OTHER | RH_CLASS:
			switch (wValue) {
				case (RH_PORT_SUSPEND):			
						WR_RH_PORTSTAT (RH_PS_PSS ); OK (0); 
				case (RH_PORT_RESET): /* BUG IN HUP CODE *********/
						if (RD_RH_PORTSTAT & RH_PS_CCS)
						    WR_RH_PORTSTAT (RH_PS_PRS);
						OK (0);
				case (RH_PORT_POWER):			
						WR_RH_PORTSTAT (RH_PS_PPS ); OK (0); 
				case (RH_PORT_ENABLE): /* BUG IN HUP CODE *********/
						if (RD_RH_PORTSTAT & RH_PS_CCS)
						    WR_RH_PORTSTAT (RH_PS_PES );
						OK (0);
			}
			break;

		case RH_SET_ADDRESS: ohci->rh.devnum = wValue; OK(0);

		case RH_GET_DESCRIPTOR:
			switch ((wValue & 0xff00) >> 8) {
				case (0x01): /* device descriptor */
					len = min(
						  leni,
						  min(
						      sizeof (root_hub_dev_des),
						      wLength));
					data_buf = root_hub_dev_des; OK(len);
				case (0x02): /* configuration descriptor */
					len = min(
						  leni,
						  min(
						      sizeof (root_hub_config_des),
						      wLength));
					data_buf = root_hub_config_des; OK(len);
				case (0x03): /* string descriptors */
					len = g_psBus->root_hub_string (wValue & 0xff,
						(int)(long) ohci->regs, "OHCI",
						data, wLength);
					if (len > 0) {
						data_buf = data;
						OK(min(leni, len));
					}
					// else fallthrough
				default: 
					status = TD_CC_STALL;
			}
			break;
		
		case RH_GET_DESCRIPTOR | RH_CLASS:
		    {
			    __u32 temp = roothub_a (ohci);

			    data_buf [0] = 9;		// min length;
			    data_buf [1] = 0x29;
			    data_buf [2] = temp & RH_A_NDP;
			    data_buf [3] = 0;
			    if (temp & RH_A_PSM) 	/* per-port power switching? */
				data_buf [3] |= 0x1;
			    if (temp & RH_A_NOCP)	/* no overcurrent reporting? */
				data_buf [3] |= 0x10;
			    else if (temp & RH_A_OCPM)	/* per-port overcurrent reporting? */
				data_buf [3] |= 0x8;

			    datab [1] = 0;
			    data_buf [5] = (temp & RH_A_POTPGT) >> 24;
			    temp = roothub_b (ohci);
			    data_buf [7] = temp & RH_B_DR;
			    if (data_buf [2] < 7) {
				data_buf [8] = 0xff;
			    } else {
				data_buf [0] += 2;
				data_buf [8] = (temp & RH_B_DR) >> 8;
				data_buf [10] = data_buf [9] = 0xff;
			    }
				
			    len = min(leni,
				      min(data_buf [0], wLength));
			    OK (len);
			}
 
		case RH_GET_CONFIGURATION: 	*(__u8 *) data_buf = 0x01; OK (1);

		case RH_SET_CONFIGURATION: 	WR_RH_STAT (0x10000); OK (0);

		default: 
			printk("unsupported root hub command\n");
			status = TD_CC_STALL;
	}
	
#ifdef	DEBUG
	// ohci_dump_roothub (ohci, 0);
#endif

	len = min(len, leni);
	if (data != data_buf)
	    memcpy (data, data_buf, len);
  	urb->nActualLength = len;
	urb->nStatus = cc_to_error [status];
	
#ifdef DEBUG
	urb_print (urb, "RET(rh)", usb_pipeout (urb->nPipe));
#endif

	urb->pHCPrivate = NULL;
	atomic_dec( &usb_dev->nRefCount );
//	usb_dec_dev_use (usb_dev);
	urb->psDevice = NULL;
	if (urb->pComplete)
	    	urb->pComplete (urb);
	return 0;
}

/*-------------------------------------------------------------------------*/

static int rh_unlink_urb (USB_packet_s * urb)
{
	ohci_t * ohci = urb->psDevice->psBus->pHCPrivate;
 
	if (ohci->rh.urb == urb) {
		ohci->rh.send = 0;
		delete_timer (ohci->rh.rh_int_timer);
		ohci->rh.urb = NULL;

		urb->pHCPrivate = NULL;
		atomic_dec( &urb->psDevice->nRefCount );
		//usb_dec_dev_use(urb->psDevice);
		urb->psDevice = NULL;
		if (urb->nTransferFlags & USB_ASYNC_UNLINK) {
			urb->nStatus = -ECONNRESET;
			if (urb->pComplete)
				urb->pComplete (urb);
		} else
			urb->nStatus = -ENOENT;
	}
	return 0;
}
 
/*-------------------------------------------------------------------------*
 * HC functions
 *-------------------------------------------------------------------------*/

/* reset the HC and BUS */

static int hc_reset (ohci_t * ohci)
{
	int timeout = 30;
	int smm_timeout = 50; /* 0,5 sec */
	 	
#ifndef __hppa__
	/* PA-RISC doesn't have SMM, but PDC might leave IR set */
	if (readl (&ohci->regs->control) & OHCI_CTRL_IR) { /* SMM owns the HC */
		writel (OHCI_OCR, &ohci->regs->cmdstatus); /* request ownership */
		printk("USB HC TakeOver from SMM\n");
		while (readl (&ohci->regs->control) & OHCI_CTRL_IR) {
			udelay(10*1000);
			if (--smm_timeout == 0) {
				printk("USB HC TakeOver failed!\n");
				return -1;
			}
		}
	}
#endif	
		
	/* Disable HC interrupts */
	writel (OHCI_INTR_MIE, &ohci->regs->intrdisable);

	kerndbg(KERN_DEBUG, "USB HC reset_hc usb-%i: ctrl = 0x%x ;\n",
		ohci->ohci_dev.nDevice,
		readl (&ohci->regs->control));

  	/* Reset USB (needed by some controllers) */
	writel (0, &ohci->regs->control);

	/* Force a state change from USBRESET to USBOPERATIONAL for ALi */
	(void) readl (&ohci->regs->control);	/* PCI posting */
	writel (ohci->hc_control = OHCI_USB_OPER, &ohci->regs->control);

	/* HC Reset requires max 10 ms delay */
	writel (OHCI_HCR,  &ohci->regs->cmdstatus);
	while ((readl (&ohci->regs->cmdstatus) & OHCI_HCR) != 0) {
		if (--timeout == 0) {
			printk("USB HC reset timed out!\n");
			return -1;
		}	
		udelay (1);
	}	 
	return 0;
}

/*-------------------------------------------------------------------------*/

/* Start an OHCI controller, set the BUS operational
 * enable interrupts 
 * connect the virtual root hub */

static int hc_start (ohci_t * ohci)
{
  	__u32 mask;
  	unsigned int fminterval;
  	USB_device_s  * usb_dev;
	struct ohci_device * dev;
	
	ohci->disabled = 1;

	/* Tell the controller where the control and bulk lists are
	 * The lists are empty now. */
	 
	writel (0, &ohci->regs->ed_controlhead);
	writel (0, &ohci->regs->ed_bulkhead);
	
	writel (ohci->hcca_dma, &ohci->regs->hcca); /* a reset clears this */
   
  	fminterval = 0x2edf;
	writel ((fminterval * 9) / 10, &ohci->regs->periodicstart);
	fminterval |= ((((fminterval - 210) * 6) / 7) << 16); 
	writel (fminterval, &ohci->regs->fminterval);	
	writel (0x628, &ohci->regs->lsthresh);

 	/* start controller operations */
 	ohci->hc_control = OHCI_CONTROL_INIT | OHCI_USB_OPER;
	ohci->disabled = 0;
 	writel (ohci->hc_control, &ohci->regs->control);
 
	/* Choose the interrupts we care about now, others later on demand */
	mask = OHCI_INTR_MIE | OHCI_INTR_UE | OHCI_INTR_WDH | OHCI_INTR_SO;
	writel (mask, &ohci->regs->intrenable);
	writel (mask, &ohci->regs->intrstatus);

#ifdef	OHCI_USE_NPS
	if(ohci->flags & OHCI_QUIRK_SUCKYIO)
	{
		/* NSC 87560 at least requires different setup .. */
		writel ((roothub_a (ohci) | RH_A_NOCP) &
			~(RH_A_OCPM | RH_A_POTPGT | RH_A_PSM | RH_A_NPS),
			&ohci->regs->roothub.a);
	}
	else
	{
		/* required for AMD-756 and some Mac platforms */
		writel ((roothub_a (ohci) | RH_A_NPS) & ~RH_A_PSM,
			&ohci->regs->roothub.a);
	}
	writel (RH_HS_LPSC, &ohci->regs->roothub.status);
#endif	/* OHCI_USE_NPS */

	(void)readl (&ohci->regs->intrdisable); /* PCI posting flush */

	// POTPGT delay is bits 24-31, in 2 ms units.
	udelay(((roothub_a (ohci) >> 23) & 0x1fe)*1000);
 
	/* connect the virtual root hub */
	ohci->rh.devnum = 0;
	usb_dev = g_psBus->alloc_device (NULL, ohci->bus);
	if (!usb_dev) {
	    ohci->disabled = 1;
	    return -ENOMEM;
	}

	dev = usb_to_ohci (usb_dev);
	ohci->bus->psRootHUB = usb_dev;
	g_psBus->connect (usb_dev);
	if (g_psBus->new_device (usb_dev) != 0) {
		g_psBus->free_device (usb_dev); 
		ohci->disabled = 1;
		return -ENODEV;
	}
	
	return 0;
}

/*-------------------------------------------------------------------------*/

/* called only from interrupt handler */

static void check_timeouts (struct ohci *ohci)
{
	spinlock (&usb_ed_lock);
	while (!list_empty (&ohci->timeout_list)) {
		USB_packet_s	*urb;

		urb = list_entry (ohci->timeout_list.next, USB_packet_s, lPacketList);
		
		#if 0
		if (time_after (jiffies, urb->nTimeout))
			break;
		#endif

		list_del_init (&urb->lPacketList);
		if (urb->nStatus != -EINPROGRESS)
			continue;

		urb->nTransferFlags |= USB_TIMEOUT_KILLED | USB_ASYNC_UNLINK;
		spinunlock (&usb_ed_lock);

		// outside the interrupt handler (in a timer...)
		// this reference would race interrupts
		sohci_unlink_urb (urb);

		spinlock (&usb_ed_lock);
	}
	spinunlock (&usb_ed_lock);
}


/*-------------------------------------------------------------------------*/

/* an interrupt happens */

static int hc_interrupt (int irq, void *dev_id, SysCallRegs_s *sys_regs )
{
	ohci_t * ohci = dev_id;
	struct ohci_regs * regs = ohci->regs;
 	int ints; 

	/* avoid (slow) readl if only WDH happened */
	if ((ohci->hcca->done_head != 0)
			&& !(le32_to_cpup (&ohci->hcca->done_head) & 0x01)) {
		ints =  OHCI_INTR_WDH;

	/* cardbus/... hardware gone before remove() */
	} else if ((ints = readl (&regs->intrstatus)) == ~(u32)0) {
		ohci->disabled++;
		printk("%i device removed!\n", ohci->ohci_dev.nDevice);
		return( 0 );

	/* interrupt for some other device? */
	} else if ((ints &= readl (&regs->intrenable)) == 0) {
		return( 0 );
	} 

	// dbg("Interrupt: %x frame: %x", ints, le16_to_cpu (ohci->hcca->frame_no));

	if (ints & OHCI_INTR_UE) {
		ohci->disabled++;
		printk("OHCI Unrecoverable Error, controller usb-%i disabled\n",
			ohci->ohci_dev.nDevice);
		// e.g. due to PCI Master/Target Abort

#ifdef	DEBUG
		ohci_dump (ohci, 1);
#else
		// FIXME: be optimistic, hope that bug won't repeat often.
		// Make some non-interrupt context restart the controller.
		// Count and limit the retries though; either hardware or
		// software errors can go forever...
#endif
		hc_reset (ohci);
	}
  
	if (ints & OHCI_INTR_WDH) {
		writel (OHCI_INTR_WDH, &regs->intrdisable);	
		(void)readl (&regs->intrdisable); /* PCI posting flush */
		dl_done_list (ohci, dl_reverse_done_list (ohci));
		writel (OHCI_INTR_WDH, &regs->intrenable); 
		(void)readl (&regs->intrdisable); /* PCI posting flush */
	}
  
	if (ints & OHCI_INTR_SO) {
		kerndbg(KERN_DEBUG, "USB Schedule overrun\n");
		writel (OHCI_INTR_SO, &regs->intrenable); 	 
		(void)readl (&regs->intrdisable); /* PCI posting flush */
	}

	// FIXME:  this assumes SOF (1/ms) interrupts don't get lost...
	if (ints & OHCI_INTR_SF) { 
		unsigned int frame = le16_to_cpu (ohci->hcca->frame_no) & 1;
		writel (OHCI_INTR_SF, &regs->intrdisable);	
		(void)readl (&regs->intrdisable); /* PCI posting flush */
		if (ohci->ed_rm_list[!frame] != NULL) {
			dl_del_list (ohci, !frame);
		}
		if (ohci->ed_rm_list[frame] != NULL) {
			writel (OHCI_INTR_SF, &regs->intrenable);	
			(void)readl (&regs->intrdisable); /* PCI posting flush */
		}
	}

	if (!list_empty (&ohci->timeout_list)) {
		check_timeouts (ohci);
// FIXME:  enable SF as needed in a timer;
// don't make lots of 1ms interrupts
// On unloaded USB, think 4k ~= 4-5msec
		if (!list_empty (&ohci->timeout_list))
			writel (OHCI_INTR_SF, &regs->intrenable);	
	}

	writel (ints, &regs->intrstatus);
	writel (OHCI_INTR_MIE, &regs->intrenable);	
	(void)readl (&regs->intrdisable); /* PCI posting flush */
	
	return( 0 );
}

/*-------------------------------------------------------------------------*/

/* allocate OHCI */

static ohci_t * hc_alloc_ohci (PCI_Info_s dev, void * mem_base)
{
	ohci_t * ohci;
	void *real;

	ohci = (ohci_t *) kmalloc (sizeof *ohci, MEMF_KERNEL);
	if (!ohci)
		return NULL;
		
	memset (ohci, 0, sizeof (ohci_t));

	real = kmalloc( sizeof( struct ohci_hcca ) + PAGE_SIZE, MEMF_KERNEL );
	ohci->hcca = ( struct ohci_hcca* )( ((uint32)real + PAGE_SIZE ) & ~( PAGE_SIZE - 1 ) );
	ohci->hcca_real = real;
	ohci->hcca_dma = (uint32)ohci->hcca;

	#if 0
	ohci->hcca = pci_alloc_consistent (dev, sizeof *ohci->hcca,
			&ohci->hcca_dma);
        if (!ohci->hcca) {
                kfree (ohci);
                return NULL;
        }
    #endif
        memset (ohci->hcca, 0, sizeof (struct ohci_hcca));

	ohci->disabled = 1;
	ohci->sleeping = 0;
	ohci->irq = -1;
	ohci->regs = mem_base;   

	ohci->ohci_dev = dev;
	//pci_set_drvdata(dev, ohci);
 
	INIT_LIST_HEAD (&ohci->ohci_hcd_list);
	list_add (&ohci->ohci_hcd_list, &ohci_hcd_list);

	INIT_LIST_HEAD (&ohci->timeout_list);

	ohci->bus = g_psBus->alloc_bus ();
	if (!ohci->bus) {
		//pci_set_drvdata (dev, NULL);
		//pci_free_consistent (ohci->ohci_dev, sizeof *ohci->hcca,
			//	ohci->hcca, ohci->hcca_dma);
		kfree( ohci->hcca );
		kfree (ohci);
		return NULL;
	}
	ohci->bus->AddDevice = sohci_alloc_dev;
	ohci->bus->RemoveDevice = sohci_free_dev;
	ohci->bus->GetFrameNumber = sohci_get_current_frame_number;
	ohci->bus->SubmitPacket = sohci_submit_urb;
	ohci->bus->CancelPacket = sohci_unlink_urb;
	
	ohci->bus->pHCPrivate = (void *) ohci;

	return ohci;
} 


/*-------------------------------------------------------------------------*/

/* De-allocate all resources.. */

static void hc_release_ohci (ohci_t * ohci)
{	
	kerndbg(KERN_DEBUG, "USB HC release ohci usb-%i\n", ohci->ohci_dev.nDevice);

	/* disconnect all devices */    
	if (ohci->bus->psRootHUB)
		g_psBus->disconnect (&ohci->bus->psRootHUB);

	if (!ohci->disabled)
		hc_reset (ohci);
	
	if (ohci->irq >= 0) {
		//release_irq(ohci->irq, ohci);
		ohci->irq = -1;
	}
	//pci_set_drvdata(ohci->ohci_dev, NULL);
	if (ohci->bus) {
		if (ohci->bus->nBusNum != -1)
			g_psBus->remove_bus (ohci->bus);

		g_psBus->free_bus (ohci->bus);
	}

	list_del (&ohci->ohci_hcd_list);
	INIT_LIST_HEAD (&ohci->ohci_hcd_list);

	ohci_mem_cleanup (ohci);
    
	/* unmap the IO address space */
	//iounmap (ohci->regs);

	kfree( ohci->hcca_real );
//	pci_free_consistent (ohci->ohci_dev, sizeof *ohci->hcca,
	//	ohci->hcca, ohci->hcca_dma);
	kfree (ohci);
}

/*-------------------------------------------------------------------------*/

/* Increment the module usage count, start the control thread and
 * return success. */

static int
hc_found_ohci(PCI_Info_s dev , int irq,
	void *mem_base)
{
	ohci_t * ohci;
	char buf[8], *bufp = buf;
	int ret;

#ifndef __sparc__
	sprintf(buf, "%d", irq);
#else
	bufp = __irq_itoa(irq);
#endif
	kerndbg(KERN_INFO, "OHCI: USB OHCI at membase 0x%lx, IRQ %s\n",
		(unsigned long)	mem_base, bufp);
//	kerndbg(KERN_INFO, "OHCI: usb-%s, %s\n", dev->slot_name, dev->name);
    
	ohci = hc_alloc_ohci (dev, mem_base);
	if (!ohci) {
		return -ENOMEM;
	}
	if ((ret = ohci_mem_init (ohci)) < 0) {
		hc_release_ohci (ohci);
		return ret;
	}
	ohci->flags = 0/*id->driver_data*/;
	
	#if 0
	
	/* Check for NSC87560. We have to look at the bridge (fn1) to identify
	   the USB (fn2). This quirk might apply to more or even all NSC stuff
	   I don't know.. */
	   
	if(dev->vendor == PCI_VENDOR_ID_NS)
	{
		struct pci_dev *fn1  = pci_find_slot(dev->bus->number, PCI_DEVFN(PCI_SLOT(dev->devfn), 1));
		if(fn1 && fn1->vendor == PCI_VENDOR_ID_NS && fn1->device == PCI_DEVICE_ID_NS_87560_LIO)
			ohci->flags |= OHCI_QUIRK_SUCKYIO;
		
	}
	
	if (ohci->flags & OHCI_QUIRK_SUCKYIO)
		printk (KERN_INFO __FILE__ ": Using NSC SuperIO setup\n");
	if (ohci->flags & OHCI_QUIRK_AMD756)
		printk (KERN_INFO __FILE__ ": AMD756 erratum 4 workaround\n");

	#endif

	if (hc_reset (ohci) < 0) {
		hc_release_ohci (ohci);
		return -ENODEV;
	}

	/* FIXME this is a second HC reset; why?? */
	writel (ohci->hc_control = OHCI_USB_RESET, &ohci->regs->control);
	(void)readl (&ohci->regs->intrdisable); /* PCI posting flush */
	udelay(10*100);

	g_psBus->add_bus (ohci->bus);
	
	if (request_irq (irq, hc_interrupt, NULL, SA_SHIRQ,
			"usb_ohci", ohci) < 0 ) {
		printk("request interrupt %s failed\n", bufp);
		hc_release_ohci (ohci);
		return -EBUSY;
	}
	ohci->irq = irq;     

	if (hc_start (ohci) < 0) {
		printk("can't start usb-%i\n", dev.nDevice);
		hc_release_ohci (ohci);
		return -EBUSY;
	}

#ifdef	DEBUG
	ohci_dump (ohci, 1);
#endif
	return 0;
}


/*-------------------------------------------------------------------------*/


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

/* configured so that an OHCI device is always provided */
/* always called with process context; sleeping is OK */

static int
ohci_pci_probe (int nDeviceID, PCI_Info_s dev)
{
	int status;
	void *base;
	uint32 temp;
	int	region;
	unsigned long		resource, len;
	
	g_psPCIBus->write_pci_config( dev.nBus, dev.nDevice, dev.nFunction, PCI_COMMAND, 2, g_psPCIBus->read_pci_config( dev.nBus, dev.nDevice, dev.nFunction, PCI_COMMAND, 2 )
					| PCI_COMMAND_IO | PCI_COMMAND_MASTER );

	if (!dev.u.h0.nInterruptLine) {
      	kerndbg( KERN_FATAL, "Found HC with no IRQ.  Check BIOS/PCI %i setup!\n",
		dev.nDevice);
   	       return -ENODEV;
    }
    
    
	region = 0;
	base = NULL;
	resource = dev.u.h0.nBase0 & PCI_ADDRESS_MEMORY_32_MASK;
	len = get_pci_memory_size( &dev, 0 );
	len = PAGE_ALIGN( len );
		
	remap_area( create_area( "usb_ohci", &base, len + PAGE_SIZE, len + PAGE_SIZE, AREA_ANY_ADDRESS|AREA_KERNEL,
								AREA_FULL_LOCK ), (void*)( resource & PAGE_MASK ) ); 
	temp = (uint32)base + ( resource - ( resource & PAGE_MASK ) );
	base = (void*)temp;
	
	

	status = hc_found_ohci (dev, dev.u.h0.nInterruptLine, base);
	if (status < 0) {
		printk( "Failed to initialize OHCI\n" );
		//iounmap (mem_base);
		//release_mem_region (mem_resource, mem_len);
		//pci_disable_device (dev);
	}
	
	/* Claim device */
	if( claim_device( nDeviceID, dev.nHandle, "USB OHCI controller", DEVICE_CONTROLLER ) != 0 )
		return( -1 );
	
	return status;
} 

/*-------------------------------------------------------------------------*/





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
    	
    	if( pci.nClassApi == 16 && pci.nClassBase == PCI_SERIAL_BUS && pci.nClassSub == PCI_USB )
    	{
    		if( ohci_pci_probe( nDeviceID, pci ) == 0 )
    			found = true;
        }
        
    }
   	if( !found ) {
   		kerndbg( KERN_DEBUG, "No USB OHCI controller found\n" );
   		disable_device( nDeviceID );
   		return( -1 );
   	}
	return( 0 );
}


status_t device_uninit( int nDeviceID)
{
	return( 0 );
}







