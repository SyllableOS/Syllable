
/*
 * Generic parallel printer  driver
 *
 * Copyright (C) 1992 by Jim Weigand and Linus Torvalds
 * Copyright (C) 1992,1993 by Michael K. Johnson
 * - Thanks much to Gunter Windau for pointing out to me where the error
 *   checking ought to be.
 * Copyright (C) 1993 by Nigel Gamble (added interrupt code)
 * Copyright (C) 1994 by Alan Cox (Modularised it)
 * LPCAREFUL, LPABORT, LPGETSTATUS added by Chris Metcalf, metcalf@lcs.mit.edu
 * Statistics and support for slow printers by Rob Janssen, rob@knoware.nl
 * "lp=" command line parameters added by Grant Guenther, grant@torque.net
 * lp_read (Status readback) support added by Carsten Gross,
 *                                             carsten@sol.wohnheim.uni-ulm.de
 * Support for parport by Philip Blundell <philb@gnu.org>
 * Parport sharing hacking by Andrea Arcangeli
 * Fixed kernel_(to/from)_user memory copy to check for errors
 * 				by Riccardo Facchetti <fizban@tin.it>
 * 22-JAN-1998  Added support for devfs  Richard Gooch <rgooch@atnf.csiro.au>
 * Redesigned interrupt handling for handle printers with buggy handshake
 *				by Andrea Arcangeli, 11 May 1998
 * Full efficient handling of printer with buggy irq handshake (now I have
 * understood the meaning of the strange handshake). This is done sending new
 * characters if the interrupt is just happened, even if the printer say to
 * be still BUSY. This is needed at least with Epson Stylus Color. To enable
 * the new TRUST_IRQ mode read the `LP OPTIMIZATION' section below...
 * Fixed the irq on the rising edge of the strobe case.
 * Obsoleted the CAREFUL flag since a printer that doesn' t work with
 * CAREFUL will block a bit after in lp_check_status().
 *				Andrea Arcangeli, 15 Oct 1998
 * Obsoleted and removed all the lowlevel stuff implemented in the last
 * month to use the IEEE1284 functions (that handle the _new_ compatibilty
 * mode fine).
 */

/* This driver should, in theory, work with any parallel port that has an
 * appropriate low-level driver; all I/O is done through the parport
 * abstraction layer.
 *
 * If this driver is built into the kernel, you can configure it using the
 * kernel command-line.  For example:
 *
 *	lp=parport1,none,parport2	(bind lp0 to parport1, disable lp1 and
 *					 bind lp2 to parport2)
 *
 *	lp=auto				(assign lp devices to all ports that
 *				         have printers attached, as determined
 *					 by the IEEE-1284 autoprobe)
 * 
 *	lp=reset			(reset the printer during 
 *					 initialisation)
 *
 *	lp=off				(disable the printer driver entirely)
 *
 * If the driver is loaded as a module, similar functionality is available
 * using module parameters.  The equivalent of the above commands would be:
 *
 *	# insmod lp.o parport=1,none,2
 *
 *	# insmod lp.o parport=auto
 *
 *	# insmod lp.o reset=1
 */

/* COMPATIBILITY WITH OLD KERNELS
 *
 * Under Linux 2.0 and previous versions, lp devices were bound to ports at
 * particular I/O addresses, as follows:
 *
 *	lp0		0x3bc
 *	lp1		0x378
 *	lp2		0x278
 *
 * The new driver, by default, binds lp devices to parport devices as it
 * finds them.  This means that if you only have one port, it will be bound
 * to lp0 regardless of its I/O address.  If you need the old behaviour, you
 * can force it using the parameters described above.
 */

/*
 * The new interrupt handling code take care of the buggy handshake
 * of some HP and Epson printer:
 * ___
 * ACK    _______________    ___________
 *                       |__|
 * ____
 * BUSY   _________              _______
 *                 |____________|
 *
 * I discovered this using the printer scanner that you can find at:
 *
 *	ftp://e-mind.com/pub/linux/pscan/
 *
 *					11 May 98, Andrea Arcangeli
 *
 * My printer scanner run on an Epson Stylus Color show that such printer
 * generates the irq on the _rising_ edge of the STROBE. Now lp handle
 * this case fine too.
 *
 *					15 Oct 1998, Andrea Arcangeli
 *
 * The so called `buggy' handshake is really the well documented
 * compatibility mode IEEE1284 handshake. They changed the well known
 * Centronics handshake acking in the middle of busy expecting to not
 * break drivers or legacy application, while they broken linux lp
 * until I fixed it reverse engineering the protocol by hand some
 * month ago...
 *
 *                                     14 Dec 1998, Andrea Arcangeli
 *
 * Copyright (C) 2000 by Tim Waugh (added LPSETTIMEOUT ioctl)
 * Hacked from linux code for Syllable by Sandor Lengyel. 2006.
 */

#undef CONFIG_PARPORT_1284

#include <posix/errno.h>
#include <posix/ioctls.h>
#include <posix/fcntl.h>
#include <posix/termios.h>
#include <posix/wait.h>
#include <posix/signal.h>

#include <atheos/kernel.h>
#include <atheos/device.h>
#include <atheos/semaphore.h>
#include <atheos/schedule.h>
#include <atheos/smp.h>
#include <posix/fcntl.h>
#include <atheos/spinlock.h>
#include <atheos/pci.h>
#include <atheos/linux_compat.h>
#include <atheos/udelay.h>
#include <atheos/device.h>
#include <atheos/irq.h>
#include <atheos/bitops.h>
#include <macros.h>

#define LP_STATS
#include <atheos/lp.h>

#include "parport.h"

/* if you have more than 8 printers, remember to increase LP_NO */
#define LP_NO 8
#define LP_BUFFER_SIZE PAGE_SIZE

/* Magic numbers for defining port-device mappings */
#define LP_PARPORT_UNSPEC -4
#define LP_PARPORT_AUTO -3
#define LP_PARPORT_OFF -2
#define LP_PARPORT_NONE -1

#define LP_F(minor)	lp_table[(minor)].flags	/* flags for busy, etc. */
#define LP_CHAR(minor)	lp_table[(minor)].chars	/* busy timeout */
#define LP_TIME(minor)	lp_table[(minor)].time	/* wait time */
#define LP_WAIT(minor)	lp_table[(minor)].wait	/* strobe wait */
#define LP_IRQ(minor)	lp_table[(minor)].dev->irq	/* interrupt # */
													/* PARPORT_IRQ_NONE means polled */

#define LP_STAT(minor)	lp_table[(minor)].stats	/* statistics area */

struct lp_struct
{
	struct pardevice *dev;
	int pp_nDevNode;
	volatile unsigned long flags;
	unsigned int chars;
	unsigned int time;
	unsigned int wait;
	char *lp_buffer;
	unsigned int lastcall;
	unsigned int runchars;
	struct lp_stats stats;
	unsigned int last_error;
	sem_info port_mutex;
	long timeout;
	unsigned int best_mode;
	unsigned int current_mode;
	volatile unsigned long bits;
	int minor;
	uint32 pp_nFlags;
	sem_id hDeviceClaimed;
	sem_id hPreEmptRequest;
	bool pp_bOpen;
};

static struct lp_struct lp_table[LP_NO];

static unsigned int lp_count = 0;
static struct class *lp_class;

static int g_nIRQHandle;

/* Bits used to manage claiming the parport device */
#define LP_PREEMPT_REQUEST 1
#define LP_PARPORT_CLAIMED 2

#define time_before(a,b) ((long)(a) - (long)(b) < 0)

#ifndef __OPTIMIZE__
# error You must compile this file with the correct options!
# error You must compile this driver with "-O".
#endif

/* --- low-level port access ----------------------------------- */

static char *stringClaimed[3] = { "lpDevClaimed0", "lpDevClaimed0", "lpDevClaimed0" };

unsigned char writeControl( struct pardevice *p, unsigned char val )
{
	unsigned char mask = ( PARPORT_CONTROL_STROBE | PARPORT_CONTROL_AUTOFD | PARPORT_CONTROL_INIT | PARPORT_CONTROL_SELECT );

	unsigned char ctr = p->ctr;
	unsigned long base = p->base;

	ctr = 0x04 ^ val;	// invert init
	ctr &= 0x0f;		/* only write writable bits. */
	isa_writeb( base + 2, ctr );
	p->ctr = ctr;		/* Update soft copy */

	return ctr;
}

unsigned char parportReadStatus( struct pardevice *p )
{
	unsigned long base = p->base;

	return isa_readb( base + 1 );
}

void parportWriteData( struct pardevice *p, unsigned char d )
{
	unsigned long base = p->base;

	isa_writeb( base, d );
}

unsigned char parportReadData( struct pardevice *p )
{
	unsigned long base = p->base;
	unsigned char val = isa_readb( base );

	return val;
}

#define r_dtr(x)	(parportReadData(lp_table[(x)].dev))
#define r_str(x)	(parportReadStatus(lp_table[(x)].dev))
#define w_ctr(x,y)	 (writeControl(lp_table[(x)].dev, (y)))
#define w_dtr(x,y)   (parportWriteData(lp_table[(x)].dev, (y)));

/* Claim the parport or block trying unless we've already claimed it */
static void lp_claim_parport_or_block( struct lp_struct *this_lp )
{
	LOCK( this_lp->hDeviceClaimed );
}

/* Claim the parport or block trying unless we've already claimed it */
static void lp_release_parport( struct lp_struct *this_lp )
{
	UNLOCK( this_lp->hDeviceClaimed );
}

void parport_set_timeout( int minor, long theTimeout )
{
	lp_table[minor].dev->timeout = theTimeout;
}

/* 
 * Try to negotiate to a new mode; if unsuccessful negotiate to
 * compatibility mode.  Return the mode we ended up in.
 */

static int lp_reset( int minor )
{
	int retval;

	lp_claim_parport_or_block( &lp_table[minor] );
	w_ctr( minor, LP_PSELECP );
	udelay( LP_DELAY );
	w_ctr( minor, LP_PSELECP | LP_PINITP );
	retval = r_str( minor );
	lp_release_parport( &lp_table[minor] );
	return retval;
}

static void lp_error( int minor )
{
	int polling;

	if( LP_F( minor ) & LP_ABORT )
		return;

	polling = lp_table[minor].dev->irq == PARPORT_IRQ_NONE;
	if( polling )
		lp_release_parport( &lp_table[minor] );
	udelay( LP_TIMEOUT_POLLED );
	lp_claim_parport_or_block( &lp_table[minor] );
}

static int lp_check_status( int minor )
{
	int error = 0;
	unsigned int last = lp_table[minor].last_error;
	unsigned char status = r_str( minor );

	if( ( status & LP_POUTPA ) )
	{
		if( last != LP_POUTPA )
		{
			last = LP_POUTPA;
			printk( KERN_INFO "lp%d out of paper\n", minor );
		}
		error = -ENOSPC;
	}
	else if( !( status & LP_PSELECD ) )
	{
		if( last != LP_PSELECD )
		{
			last = LP_PSELECD;
			printk( KERN_INFO "lp%d off-line\n", minor );
		}
		error = -EIO;
	}
	else if( !( status & LP_PERRORP ) )
	{
		if( last != LP_PERRORP )
		{
			last = LP_PERRORP;
			printk( KERN_INFO "lp%d on fire\n", minor );
		}
		error = -EIO;
	}
	else
	{
		last = 0;	/* Come here if LP_CAREFUL is set and no
				   errors are reported. */
	}

	lp_table[minor].last_error = last;

	if( last != 0 )
		lp_error( minor );

	return error;
}

static int lp_wait_ready( int minor, int nonblock )
{
	int error = 0;

	/* If we're not in compatibility mode, we're ready now! */
	if( lp_table[minor].current_mode != IEEE1284_MODE_COMPAT )
	{
		return ( 0 );
	}

	do
	{
		error = lp_check_status( minor );
		if( error && ( nonblock || ( LP_F( minor ) & LP_ABORT ) ) )
			break;
		if( is_signal_pending() )
		{
			error = -EINTR;
			break;
		}
	}
	while( error );
	return error;
}

size_t parport_write( int minor, const void *buffer, size_t len )
{
	int no_irq = 1;
	ssize_t count = 0;
	const unsigned char *addr = buffer;
	unsigned char byte;
	unsigned char ctl = ( PARPORT_CONTROL_SELECT | PARPORT_CONTROL_INIT );

	w_ctr( minor, ctl );	// Disable IRQ
	no_irq = 1;
	lp_table[minor].dev->ieee1284phase = IEEE1284_PH_FWD_DATA;
	w_ctr( minor, ctl );
	udelay( 10 );
	w_ctr( minor, PARPORT_CONTROL_SELECT );

	while( count < len )
	{
		unsigned long expire = jiffies + lp_table[minor].dev->timeout;
		long wait = ( HZ + 99 ) / 100;
		unsigned char mask = ( PARPORT_STATUS_ERROR | PARPORT_STATUS_BUSY );
		unsigned char val = ( PARPORT_STATUS_ERROR | PARPORT_STATUS_BUSY );

		/* Wait until the peripheral's ready */
		do
		{
			/* Is the peripheral ready yet? */
			if( ( r_str( minor ) & mask ) == mask )	// No error and no busy
				/* Skip the loop */
				goto ready;

			/* Is the peripheral upset? */
			if( ( r_str( minor ) & ( PARPORT_STATUS_PAPEROUT | PARPORT_STATUS_SELECT | PARPORT_STATUS_ERROR ) ) != ( PARPORT_STATUS_SELECT | PARPORT_STATUS_ERROR ) )
				/* If nFault is asserted (i.e. no
				 * error) and PAPEROUT and SELECT are
				 * just red herrings, give the driver
				 * a chance to check it's happy with
				 * that before continuing. */
				goto stop;

			/* Have we run out of time? */
			if( !time_before( jiffies, expire ) )
				break;

			/* Yield the port for a while.  If this is the
			   first time around the loop, don't let go of
			   the port.  This way, we find out if we have
			   our interrupt handler called. */
			if( count && no_irq )
			{
				lp_release_parport( &lp_table[minor] );
				udelay( wait );
				lp_claim_parport_or_block( &lp_table[minor] );
			}
			else
				/* We must have the device claimed here */
				udelay( wait );

			/* Is there a signal pending? */
			if( is_signal_pending() )
				break;

			/* Wait longer next time. */
			wait *= 2;
		}
		while( time_before( jiffies, expire ) );

		if( is_signal_pending() )
			break;
		printk( KERN_DEBUG "%s: Timed out\n", lp_table[minor].dev->name );
		break;

	      ready:
		/* Write the character to the data lines. */
		byte = *addr++;
		w_dtr( minor, byte );

		udelay( 1 );

		/* Pulse strobe. */
		w_ctr( minor, ctl | PARPORT_CONTROL_STROBE );
		udelay( 1 );	/* strobe */

		w_ctr( minor, ctl );
		udelay( 1 );	/* hold */

		/* Assume the peripheral received it. */
		count++;

		/* Let another process run if it needs to. */
		if( time_before( jiffies, expire ) )
			Schedule();
	}

stop:
	lp_table[minor].dev->ieee1284phase = IEEE1284_PH_FWD_IDLE;
	return count;
}

static int lp_write( void *pNode, void *pCookie, off_t nPosition, const void *pBuffer, size_t nSize )
{
	uint32 f_flags;
	struct lp_struct *lpStruct = pNode;
	unsigned int minor = lpStruct->minor;

	f_flags = lpStruct->pp_nFlags;
	char *kbuf = lp_table[minor].lp_buffer;
	ssize_t retv = 0;
	ssize_t written;
	size_t copy_size = nSize;
	int nonblock = ( ( f_flags & O_NONBLOCK ) || ( LP_F( minor ) & LP_ABORT ) );

	if( jiffies - lp_table[minor].lastcall > LP_TIME( minor ) )
		lp_table[minor].runchars = 0;

	lp_table[minor].lastcall = jiffies;

	/* Need to copy the data from user-space. */
	if( copy_size > LP_BUFFER_SIZE )
		copy_size = LP_BUFFER_SIZE;

	if( copy_from_user( kbuf, ( char * )pBuffer, copy_size ) )
	{
		printk( "No buffer avail.\n" );
		retv = -EFAULT;
		goto out_unlock;
	}

	/* Claim Parport or sleep until it becomes available
	 */

	lp_claim_parport_or_block( &lp_table[minor] );

	/* Go to the proper mode. */

	parport_set_timeout( minor, ( nonblock ? PARPORT_INACTIVITY_O_NONBLOCK : lp_table[minor].timeout ) );

	if( ( retv = lp_wait_ready( minor, nonblock ) ) == 0 )
		do
		{
			/* Write the data. */
			written = parport_write( minor, kbuf, copy_size );	// return size

			if( written > 0 )
			{
				copy_size -= written;
				nSize -= written;
				pBuffer += written;
				retv += written;
			}

			if( is_signal_pending() )
			{
				if( retv == 0 )
					retv = -EINTR;

				break;
			}

			if( copy_size > 0 )
			{
				/* incomplete write -> check error ! */
				int error;


				lp_table[minor].current_mode = IEEE1284_MODE_COMPAT;

				error = lp_wait_ready( minor, nonblock );

				if( error )
				{
					if( retv == 0 )
						retv = error;
					break;
				}
				else if( nonblock )
				{
					if( retv == 0 )
						retv = -EAGAIN;
					break;
				}

				lp_table[minor].current_mode = IEEE1284_MODE_COMPAT;

			}
			else
				Schedule();

			if( nSize )
			{
				copy_size = nSize;
				if( copy_size > LP_BUFFER_SIZE )
					copy_size = LP_BUFFER_SIZE;

				if( copy_from_user( kbuf, ( char * )pBuffer, copy_size ) )
				{
					if( retv == 0 )
						printk( "Efault\n" );
					retv = -EFAULT;
					break;
				}
			}
		}
		while( nSize > 0 );


	lp_release_parport( &lp_table[minor] );

out_unlock:
	return retv;
}

static int lp_read( void *pNode, void *pCookie, off_t nPosition, void *pBuffer, size_t nSize )
{
	return 0;
}

status_t lp_open( void *pNode, uint32 nFlags, void **pCookie )
{
	uint32 f_flags;
	struct lp_struct *lpStruct = pNode;
	unsigned int minor = lpStruct->minor;

	f_flags = lpStruct->pp_nFlags;
	lp_table[minor].hDeviceClaimed = create_semaphore( stringClaimed[minor], 1, 0 );

	if( minor >= LP_NO )
		return -ENXIO;
	if( ( LP_F( minor ) & LP_EXIST ) == 0 )
		return -ENXIO;
	if( test_and_set_bit( LP_BUSY_BIT_POS, &LP_F( minor ) ) )
		return -EBUSY;
	/* If ABORTOPEN is set and the printer is offline or out of paper,
	   we may still want to open it to perform ioctl()s.  Therefore we
	   have commandeered O_NONBLOCK, even though it is being used in
	   a non-standard manner.  This is strictly a Linux hack, and
	   should most likely only ever be used by the tunelp application. */
	if( ( LP_F( minor ) & LP_ABORTOPEN ) && !( f_flags & O_NONBLOCK ) )
	{
		int status;

		lp_claim_parport_or_block( &lp_table[minor] );
		status = r_str( minor );
		lp_release_parport( &lp_table[minor] );
		if( status & LP_POUTPA )
		{
			printk( KERN_INFO "lp%d out of paper\n", minor );
			LP_F( minor ) &= ~LP_BUSY;
			return -ENOSPC;
		}
		else if( !( status & LP_PSELECD ) )
		{
			printk( KERN_INFO "lp%d off-line\n", minor );
			LP_F( minor ) &= ~LP_BUSY;
			return -EIO;
		}
		else if( !( status & LP_PERRORP ) )
		{
			printk( KERN_ERR "lp%d printer error\n", minor );
			LP_F( minor ) &= ~LP_BUSY;
			return -EIO;
		}
	}
	lp_table[minor].lp_buffer = ( char * )kmalloc( LP_BUFFER_SIZE, MEMF_KERNEL );
	if( !lp_table[minor].lp_buffer )
	{
		LP_F( minor ) &= ~LP_BUSY;
		return -ENOMEM;
	}

	/* Determine if the peripheral supports ECP mode */

	lp_claim_parport_or_block( &( lp_table[minor] ) );
	lp_table[minor].best_mode = IEEE1284_MODE_COMPAT;
	/* Leave peripheral in compatibility mode */
	lp_release_parport( &( lp_table[minor] ) );
	lp_table[minor].current_mode = IEEE1284_MODE_COMPAT;
	return 0;
}

static int lp_close( void *pNode, void *pCookie )
{
	struct lp_struct *lpStruct = pNode;
	unsigned int minor = lpStruct->minor;

	lp_claim_parport_or_block( &lp_table[minor] );
	lp_table[minor].current_mode = IEEE1284_MODE_COMPAT;
	lp_release_parport( &lp_table[minor] );
	kfree( lp_table[minor].lp_buffer );
	LP_F( minor ) &= ~LP_BUSY;
	delete_semaphore( lp_table[minor].hDeviceClaimed );
	return 0;
}

static int lp_ioctl( void *pNode, void *pCookie, uint32 nCommand, void *pArgs, bool bFromKernel )
{
	struct lp_struct *lpStruct = pNode;
	unsigned int minor = lpStruct->minor;
	int status;
	int retval = 0;

	if( minor >= LP_NO )
		return -ENODEV;
	if( ( LP_F( minor ) & LP_EXIST ) == 0 )
		return -ENODEV;
	switch ( nCommand )
	{
		struct kernel_timeval par_timeout;
		long to_jiffies;

		case LPTIME:
			LP_TIME( minor ) = ( long int )pArgs *HZ / 100;
			break;
		case LPCHAR:
			LP_CHAR( minor ) = pArgs;
			break;
		case LPABORT:
			if( pArgs )
				LP_F( minor ) |= LP_ABORT;
			else
				LP_F( minor ) &= ~LP_ABORT;
			break;
		case LPABORTOPEN:
			if( pArgs )
				LP_F( minor ) |= LP_ABORTOPEN;
			else
				LP_F( minor ) &= ~LP_ABORTOPEN;
			break;
		case LPCAREFUL:
			if( pArgs )
				LP_F( minor ) |= LP_CAREFUL;
			else
				LP_F( minor ) &= ~LP_CAREFUL;
			break;
		case LPWAIT:
			LP_WAIT( minor ) = pArgs;
			break;
		case LPSETIRQ:
			return -EINVAL;
			break;
		case LPGETIRQ:
			if( copy_to_user( pArgs, &LP_IRQ( minor ), sizeof( int ) ) )
				return -EFAULT;
			break;
		case LPGETSTATUS:
			lp_claim_parport_or_block( &lp_table[minor] );
			status = r_str( minor );
			lp_release_parport( &lp_table[minor] );

			if( copy_to_user( pArgs, &status, sizeof( int ) ) )
				return -EFAULT;
			break;
		case LPRESET:
			lp_reset( minor );
			break;
		case LPGETSTATS:
			if( copy_to_user( pArgs, &LP_STAT( minor ), sizeof( struct lp_stats ) ) )
				return -EFAULT;
			break;
		case LPGETFLAGS:
			status = LP_F( minor );
			if( copy_to_user( pArgs, &status, sizeof( int ) ) )
				return -EFAULT;
			break;

		case LPSETTIMEOUT:
			if( copy_from_user( &par_timeout, pArgs, sizeof( struct kernel_timeval ) ) )
			{
				return -EFAULT;
			}
			/* Convert to jiffies, place in lp_table */
			if( ( par_timeout.tv_sec < 0 ) || ( par_timeout.tv_usec < 0 ) )
			{
				return -EINVAL;
			}
			to_jiffies = ROUND_UP( par_timeout.tv_usec, 1000000 / HZ );
			to_jiffies += par_timeout.tv_sec * ( long )HZ;
			if( to_jiffies <= 0 )
			{
				return -EINVAL;
			}
			lp_table[minor].timeout = to_jiffies;
			break;

		default:
			retval = -EINVAL;
	}
	return retval;
}

DeviceOperations_s g_pOperations = {
	lp_open,
	lp_close,
	lp_ioctl,
	lp_read,
	lp_write
};

/* --- initialisation code ------------------------------------- */

static int parport_nr[LP_NO] = {[0 ... LP_NO - 1] = LP_PARPORT_UNSPEC };
static char *parport[LP_NO] = { NULL, };
static int reset = 0;

static int lp_initFirst( void )
{
	int i, err = 0;

	for( i = 0; i < LP_NO; i++ )
	{
		lp_table[i].dev = NULL;
		lp_table[i].flags = 0;
		lp_table[i].chars = LP_INIT_CHAR;
		lp_table[i].time = LP_INIT_TIME;
		lp_table[i].wait = LP_INIT_WAIT;
		lp_table[i].lp_buffer = NULL;

		lp_table[i].minor = i;
		lp_table[i].lastcall = 0;
		lp_table[i].runchars = 0;
		memset( &lp_table[i].stats, 0, sizeof( struct lp_stats ) );
		lp_table[i].last_error = 0;
		lp_table[i].timeout = 10 * HZ;
		lp_table[i].pp_bOpen = false;
	}

	return 0;
}

status_t device_init( int nDeviceID )
{
	int nError;
	int nHandle;
	int i, j;
	char *stringPr[3] = { "printer/lp/0", "printer/lp/1", "printer/lp/2" };
	char *stringPr1[3] = { "LP port 0", "LP port 1", "LP port 2" };
	int rdData[3] = { 0x55, 0xaa, 0xcc };

	lp_initFirst();

	static struct pardevice dev1 = { "lp0", 0x378, 0, 0, PARPORT_IRQ_NONE, 0, 0, 0, 0, 0, 0 };
	static struct pardevice dev2 = { "lp1", 0x3bc, 0, 0, PARPORT_IRQ_NONE, 0, 0, 0, 0, 0, 0 };
	static struct pardevice dev3 = { "lp2", 0x278, 0, 0, PARPORT_IRQ_NONE, 0, 0, 0, 0, 0, 0 };

	isa_writeb( 0x378, 0x55 );
	isa_writeb( 0x3bc, 0xaa );
	isa_writeb( 0x278, 0xcc );

	lp_table[0].dev = &dev1;
	lp_table[1].dev = &dev2;
	lp_table[2].dev = &dev3;

	j = 0;
	for( i = 0; i < 2; i++ )
	{

		if( rdData[i] == isa_readb( lp_table[i].dev->base ) )
		{
			nHandle = register_device( "", "system" );
			claim_device( nDeviceID, nHandle, stringPr1[j], DEVICE_PORT );
			lp_table[0].pp_nDevNode = create_device_node( nDeviceID, nHandle, stringPr[j], &g_pOperations, &lp_table[i] );
			if( lp_table[i].pp_nDevNode < 0 )
			{
				nError = lp_table[i].pp_nDevNode;
				printk( "Error: Line Printer device failet to create device node %s \n", stringPr[j] );
				goto error1;
			}
			else
			{
				lp_table[i].flags |= LP_EXIST;
			}
			j++;
		}
	}

error1:
	return ( nError );
}
