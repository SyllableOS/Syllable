/*
 *  The AtheOS kernel
 *  Copyright (C) 1999 - 2001 Kurt Skauen
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of version 2 of the GNU Library
 *  General Public License as published by the Free Software
 *  Foundation.
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
 *  Based on the psaux.c file from the Linux driver suite.
 *  2001-08-11 -	Intellimouse mouse-wheel support added by a patch
 *			from Catalin Climov <xxl@climov.com>
 */




#include <posix/errno.h>

#include <atheos/kernel.h>
#include <atheos/device.h>
#include <atheos/semaphore.h>
#include <atheos/spinlock.h>
#include <atheos/smp.h>
#include <atheos/irq.h>

#include <atheos/isa_io.h>

static SpinLock_s g_sSPinLock = INIT_SPIN_LOCK( "ps2aux_slock" );

/* aux controller ports */
#define AUX_INPUT_PORT	0x60		/* Aux device output buffer */
#define AUX_OUTPUT_PORT	0x60		/* Aux device input buffer */
#define AUX_COMMAND	0x64		/* Aux device command buffer */
#define AUX_STATUS	0x64		/* Aux device status reg */

/* aux controller status bits */
#define AUX_OBUF_FULL	0x21		/* output buffer (from device) full */
#define AUX_IBUF_FULL	0x02		/* input buffer (to device) full */

/* aux controller commands */
#define AUX_CMD_WRITE	0x60		/* value to write to controller */
#define AUX_MAGIC_WRITE	0xd4		/* value to send aux device data */

#define AUX_INTS_ON	0x47		/* enable controller interrupts */
#define AUX_INTS_OFF	0x65		/* disable controller interrupts */

#define AUX_DISABLE	0xa7		/* disable aux */
#define AUX_ENABLE	0xa8		/* enable aux */

/* aux device commands */
#define AUX_SET_RES	0xe8		/* set resolution */
#define AUX_SET_SCALE11	0xe6		/* set 1:1 scaling */
#define AUX_SET_SCALE21	0xe7		/* set 2:1 scaling */
#define AUX_GET_SCALE	0xe9		/* get scaling factor */
#define AUX_SET_STREAM	0xea		/* set stream mode */
#define AUX_SET_SAMPLE	0xf3		/* set sample rate */
#define AUX_ENABLE_DEV	0xf4		/* enable aux device */
#define AUX_DISABLE_DEV	0xf5		/* disable aux device */
#define AUX_RESET	0xff		/* reset aux device */

#define MAX_RETRIES	60		/* some aux operations take long time*/
#define AUX_IRQ	12
#define AUX_BUF_SIZE	2048

static int aux_count = 0;

static char   g_anReceiveBuffer[AUX_BUF_SIZE];
static int    g_nRecvSize = 0;
static int    g_nRecvInPos  = 0;
static int    g_nRecvOutPos = 0;
static sem_id g_hRecvWaitQueue = -1;
static sem_id g_hRecvMutex     = -1;
static int    g_nIRQHandle     = -1;

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int poll_aux_status(void)
{
    int retries=0;

    while ((inb(AUX_STATUS)&0x03) && retries < MAX_RETRIES) {
	if ((inb_p(AUX_STATUS) & AUX_OBUF_FULL) == AUX_OBUF_FULL)
	    inb_p(AUX_INPUT_PORT);
	snooze( 50000 );
	retries++;
    }
    return !(retries==MAX_RETRIES);
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int poll_aux_status_nosleep(void)
{
    int retries = 0;

    while ((inb(AUX_STATUS)&0x03) && retries < 1000000) {
	if ((inb_p(AUX_STATUS) & AUX_OBUF_FULL) == AUX_OBUF_FULL)
	    inb_p(AUX_INPUT_PORT);
	retries++;
    }
    return !(retries == 1000000);
}

/*****************************************************************************
 * NAME:
 * DESC:
 *	Write to aux device
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static void aux_write_dev(int val)
{
    poll_aux_status_nosleep();
    outb_p(AUX_MAGIC_WRITE,AUX_COMMAND);	/* write magic cookie */
    poll_aux_status_nosleep();
    outb_p(val,AUX_OUTPUT_PORT);		/* write data */
}

/*****************************************************************************
 * NAME:
 * DESC:
 *	Write aux device command
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static void aux_write_cmd(int val)
{
    poll_aux_status_nosleep();
    outb_p(AUX_CMD_WRITE,AUX_COMMAND);
    poll_aux_status_nosleep();
    outb_p(val,AUX_OUTPUT_PORT);
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int aux_interrupt( int nIrqNum, void* pData, SysCallRegs_s* psRegs )
{
    uint32 nFlg;
    int	   nData;
    nFlg = spinlock_disable( &g_sSPinLock );

    if ((inb(AUX_STATUS) & AUX_OBUF_FULL) != AUX_OBUF_FULL) {
	spinunlock_enable( &g_sSPinLock, nFlg );
	return(0);
    }
    nData = inb(AUX_INPUT_PORT);
    if ( g_nRecvSize < AUX_BUF_SIZE ) {
	g_anReceiveBuffer[ atomic_add( &g_nRecvInPos, 1 ) % AUX_BUF_SIZE ] = nData;
	atomic_add( &g_nRecvSize, 1 );;
	wakeup_sem( g_hRecvWaitQueue, false );
    }
    spinunlock_enable( &g_sSPinLock, nFlg );
    return(1);
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

status_t ps2_open( void* pNode, uint32 nFlags, void **pCookie )
{
    uint32 nFlg;
  
    if ( atomic_add( &aux_count, 1 ) > 0 ) {
	return( 0 );
    }
    if (!poll_aux_status()) {
	atomic_add( &aux_count, -1 );
	return -EBUSY;
    }
    g_nIRQHandle = request_irq( AUX_IRQ, aux_interrupt, NULL, 0, "auxps2_device", NULL );
    if ( g_nIRQHandle < 0 ) {
	atomic_add( &aux_count, -1 );
	return( -EBUSY );
    }
    nFlg = spinlock_disable( &g_sSPinLock );
      /* disable kbd bh to avoid mixing of cmd bytes */
//  disable_bh(KEYBOARD_BH);
    poll_aux_status_nosleep();
    outb_p(AUX_ENABLE,AUX_COMMAND);		/* Enable Aux */
    aux_write_dev(AUX_ENABLE_DEV);		/* enable aux device */
    aux_write_cmd(AUX_INTS_ON);		/* enable controller ints */
    poll_aux_status_nosleep();
      /* reenable kbd bh */
//  enable_bh(KEYBOARD_BH);
  
//  aux_ready = 0;
    g_nRecvSize = 0;
    g_nRecvInPos  = 0;
    g_nRecvOutPos = 0;
    
    spinunlock_enable( &g_sSPinLock, nFlg );
  
    return( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

status_t ps2_close( void* pNode, void* pCookie )
{
    uint32 nFlg;
  
    if ( atomic_add( &aux_count, -1 ) == 1 ) {
	release_irq( AUX_IRQ, g_nIRQHandle );
	nFlg = spinlock_disable( &g_sSPinLock );
	spinunlock_enable( &g_sSPinLock, nFlg );
    }
    return( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

status_t ps2_ioctl( void* pNode, void* pCookie, uint32 nCommand, void* pArgs, bool bFromKernel )
{
    return( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int  ps2_read( void* pNode, void* pCookie, off_t nPosition, void* pBuffer, size_t nSize )
{
    uint32 nFlg;
    int	 nError;
    int	 nBytesReceived = 0;
  
    LOCK( g_hRecvMutex );
again:
    nFlg = spinlock_disable( &g_sSPinLock );
  
    if ( g_nRecvSize == 0 ) {
//    if ( g_nOpenFlags & O_NONBLOCK ) {
//      nError = -EWOULDBLOCK;
//    } else {
	nError = spinunlock_and_suspend( g_hRecvWaitQueue, &g_sSPinLock, nFlg, INFINITE_TIMEOUT );
//    }
	if ( nError < 0 ) {
//	    if ( nError != -EINTR ) {
//		spinunlock_enable( &g_sSPinLock, nFlg );
//	    }
	    goto error;
	}
	goto again;
    }
    while( g_nRecvSize > 0 && nBytesReceived < nSize ) {
	((char*)pBuffer)[nBytesReceived++] = g_anReceiveBuffer[g_nRecvOutPos++ % AUX_BUF_SIZE ];
	atomic_add( &g_nRecvSize, -1 );
    }
    spinunlock_enable( &g_sSPinLock, nFlg );
    UNLOCK( g_hRecvMutex );
    return( nBytesReceived );
error:
    UNLOCK( g_hRecvMutex );
    return( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int  ps2_write( void* pNode, void* pCookie, off_t nPosition, const void* pBuffer, size_t nSize )
{
    uint32     nFlg;
    int	     i;
  
    nFlg = spinlock_disable( &g_sSPinLock );
    for ( i = 0 ; i < nSize ; ++i ) {
	aux_write_dev( ((char*)pBuffer)[i] );
    }
    spinunlock_enable( &g_sSPinLock, nFlg );

    return( nSize );
}

DeviceOperations_s g_sOperations = {
    ps2_open,
    ps2_close,
    ps2_ioctl,
    ps2_read,
    ps2_write
};

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

status_t device_init( int nDeviceID )
{
    struct	RMREGS	rm;
    int nError;
    int nHandle;

    memset( &rm, 0, sizeof( struct RMREGS ) );
    realint( 0x11, &rm );

    if ( (rm.EAX & 0x04) == 0 ) {
	printk( "No PS2 pointing device found\n" );
	return( -EIO );
    }

    g_hRecvWaitQueue = create_semaphore( "ps2aux_recv_queue", 0, 0 );
    if ( g_hRecvWaitQueue < 0 ) {
	return( g_hRecvWaitQueue );
    }
    g_hRecvMutex = create_semaphore( "ps2aux_recv_mutex", 1, 0 );

    if ( g_hRecvMutex < 0 ) {
	delete_semaphore( g_hRecvWaitQueue );
	return( g_hRecvMutex );
    }
    nHandle = register_device( "", "system" );
    claim_device( nDeviceID, nHandle , "PS/2 port", DEVICE_PORT );
    nError = create_device_node( nDeviceID, nHandle, "misc/ps2aux", &g_sOperations, NULL );
    if ( nError < 0 ) {
	delete_semaphore( g_hRecvWaitQueue );
	delete_semaphore( g_hRecvMutex );
	return( nError );
    }
    outb_p(AUX_DISABLE,AUX_COMMAND);   /* Disable Aux device */
    poll_aux_status_nosleep();
    outb_p(AUX_CMD_WRITE,AUX_COMMAND);
    poll_aux_status_nosleep();             /* Disable interrupts */
    outb_p(AUX_INTS_OFF, AUX_OUTPUT_PORT); /*  on the controller */
    
    
  
    return( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

status_t device_uninit( int nDeviceID )
{
    return( 0 );
}
