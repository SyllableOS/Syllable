/*
 *  The Syllable kernel
 *  Keyboard and PS2 mouse driver
 *  Copyright (C) 2006 Arno Klenke
 *  Parts of this code are from the linux kernel
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of version 2 of the GNU
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
 */
 
#include <posix/errno.h>
#include <atheos/kernel.h>
#include <atheos/device.h>
#include <atheos/semaphore.h>
#include <atheos/spinlock.h>
#include <atheos/irq.h>
#include <atheos/udelay.h>
#include <atheos/isa_io.h>

#include "ps2.h"

#define PS2_BUF_SIZE 2048
 
typedef struct
{
	bool		bPresent;
	bool		bIsAux;
	sem_id		hWait;
	int			nAckReceived;
	int			nIrq;
    char	    pBuffer[ PS2_BUF_SIZE ];
    atomic_t	nOutPos;
    atomic_t	nInPos;
    atomic_t	nBytesReceived;
    atomic_t	nOpenCount;
    int			nDevHandle;
    int			nIrqHandle;
} PS2_Port_s;

static PS2_Port_s g_sKbdPort;
static PS2_Port_s g_sAuxPort;
static uint8 g_nKbdLedStatus;
static int g_nDevNum = 0;

SpinLock_s g_sLock = INIT_SPIN_LOCK( "ps2_lock" );

#define PS2_TIMEOUT 10000

/* Wait until data is available in the output (from device) buffer */
/* Needs to be called with g_sLock held */
static status_t ps2_wait_read()
{
	
	int i = 0;
	while( ( ~inb( PS2_STATUS_REG ) & PS2_STS_OBF ) && ( i < PS2_TIMEOUT ) )
	{
		udelay( 50 );
		i++;
	}
	if( i == PS2_TIMEOUT )
	{
		printk( "ps2_wait_read() timed out!\n" );
		return( -ETIMEDOUT );
	}
	return( 0 );
}

/* Wait until input buffer (to device) is empty */
/* Needs to be called with g_sLock held */
static status_t ps2_wait_write()
{
	int i = 0;
	while( ( inb( PS2_STATUS_REG ) & PS2_STS_IBF ) && ( i < PS2_TIMEOUT ) )
	{
		udelay( 50 );
		i++;
	}
	if( i == PS2_TIMEOUT )
	{
		printk( "ps2_wait_write() timed out!\n" );
		return( -ETIMEDOUT );
	}
	return( 0 );
}

/* Flushes the output buffer */
static status_t ps2_flush()
{
	int i = 0;
	uint32 nFlags;
	spinlock_cli( &g_sLock, nFlags );
	while( ( inb( PS2_STATUS_REG ) & PS2_STS_OBF ) && ( i < PS2_BUF_SIZE ) )
	{
		udelay( 50 );
		inb( PS2_DATA_REG );
		i++;
	}
	spinunlock_restore( &g_sLock, nFlags );
	return( 0 );
}

/* Write command */
static status_t ps2_command( uint8 nCmd )
{
	uint32 nFlags;
	int nError;
	spinlock_cli( &g_sLock, nFlags );
	nError = ps2_wait_write();
	if( nError < 0 ) {
		spinunlock_restore( &g_sLock, nFlags );
		return( -EIO );
	}
	outb( nCmd, PS2_COMMAND_REG );
	spinunlock_restore( &g_sLock, nFlags );
	return( 0 );
}

/* Read data */
static status_t ps2_read_command( uint8 nCmd, uint8* pnData )
{
	uint32 nFlags;
	int nError;
	spinlock_cli( &g_sLock, nFlags );
	nError = ps2_wait_write();
	if( nError < 0 ) {
		spinunlock_restore( &g_sLock, nFlags );
		return( -EIO );
	}
	outb( nCmd, PS2_COMMAND_REG );
	nError = ps2_wait_read();
	if( nError < 0 ) {	
		spinunlock_restore( &g_sLock, nFlags );
		return( -EIO );
	}	
	*pnData = inb( PS2_DATA_REG );
	
	spinunlock_restore( &g_sLock, nFlags );
	return( 0 );
}

/* Write data */
static status_t ps2_write_command( uint8 nCmd, uint8 nData )
{
	uint32 nFlags;
	int nError;
	spinlock_cli( &g_sLock, nFlags );
	nError = ps2_wait_write();
	if( nError < 0 ) {
		spinunlock_restore( &g_sLock, nFlags );
		return( -EIO );
	}
	outb( nCmd, PS2_COMMAND_REG );
	nError = ps2_wait_write();
	if( nError < 0 ) {
		spinunlock_restore( &g_sLock, nFlags );
		return( -EIO );
	}
	outb( nData, PS2_DATA_REG );
	spinunlock_restore( &g_sLock, nFlags );
	return( 0 );
}

/* Write data and read response */
static status_t ps2_write_read_command( uint8 nCmd, uint8* pnData )
{
	uint32 nFlags;
	int nError;
	spinlock_cli( &g_sLock, nFlags );
	nError = ps2_wait_write();
	if( nError < 0 ) {
		spinunlock_restore( &g_sLock, nFlags );
		return( -EIO );
	}
	outb( nCmd, PS2_COMMAND_REG );
	nError = ps2_wait_write();
	if( nError < 0 ) {
		spinunlock_restore( &g_sLock, nFlags );
		return( -EIO );
	}
	outb( *pnData, PS2_DATA_REG );
	nError = ps2_wait_read();
	if( nError < 0 ) {
		spinunlock_restore( &g_sLock, nFlags );
		return( -EIO );
	}
	*pnData = inb( PS2_DATA_REG );
	spinunlock_restore( &g_sLock, nFlags );
	return( 0 );
}

static uint ConvertKeyCode( uint8 nCode )
{
    uint	     nKey;
    int	     nFlg;
    static int nLastCode      = 0;
    static int nLastKey       = 0;
    static int nPauseKeyCount = 0;

	
    if ( nPauseKeyCount > 0 )
    {
	nPauseKeyCount++;

	if ( nPauseKeyCount == 6 ) {
	    nPauseKeyCount	=	0;
	    return( 0x10 );		/* PAUSE	*/
	} else {
	    return( 0 );
	}
    }

    if ( 0xe1 == nCode ) {
#if 0    	
	    sti();
		reboot();
		for(;;) {}
#endif		
	nPauseKeyCount = 1;
	return( 0 );
    }

    nFlg = nCode & 0x80;

    if ( 0xe0 == nLastCode ) {
	nKey = g_anExtRawKeyTab[ nCode & ~0x80 ];
    } else {
	if ( 0xe0 != nCode ) {
	    nKey = g_anRawKeyTab[ nCode & ~0x80 ];
	} else {
	    nKey = 0;
	}
    }

    nLastCode = nCode;

    if ( (nKey | nFlg) == nLastKey || 0 == nKey ) {
	return( 0 );
    }
    nLastKey = nKey | nFlg;

    return( nKey | nFlg );
}

static int ps2_interrupt( int nIrqNum, void* pData, SysCallRegs_s* psRegs )
{
    uint32 nFlg;
    int	   nData;
    
    PS2_Port_s* psPort = (PS2_Port_s*)pData;
    
    nFlg = spinlock_disable( &g_sLock );

    if( ( inb( PS2_STATUS_REG ) & PS2_STS_OBF ) != PS2_STS_OBF ) {
		spinunlock_enable( &g_sLock, nFlg );
		return( 0 );
    }
	nData = inb( PS2_DATA_REG );
	

	if( !psPort->bIsAux )
	{
		if( nData == 0xfa ) {
			psPort->nAckReceived = 1; // Received ack
		}
		else if( nData == 0xfe ) {
			psPort->nAckReceived = -1; // Received noack
		}
		nData = ConvertKeyCode( nData );
	}
    
    if( atomic_read( &psPort->nBytesReceived ) < PS2_BUF_SIZE ) {
		psPort->pBuffer[ atomic_inc_and_read( &psPort->nInPos ) % PS2_BUF_SIZE ] = nData;
		atomic_inc( &psPort->nBytesReceived );
		wakeup_sem( psPort->hWait, false );
    }
    spinunlock_enable( &g_sLock, nFlg );
    return( 0 );
}

status_t ps2_open( void* pNode, uint32 nFlags, void **pCookie )
{
    uint8 nControl;
    uint32 nFlg;
    PS2_Port_s* psPort = (PS2_Port_s*)pNode;
    
    printk( "ps2_open()\n" );
  
    if ( atomic_inc_and_read( &psPort->nOpenCount ) > 0 ) {
    	atomic_dec( &psPort->nOpenCount );
		return( -EBUSY );
    }
	ps2_flush();
	
    psPort->nIrqHandle = request_irq( psPort->nIrq, ps2_interrupt, NULL, 0, "ps2", psPort );
    if ( psPort->nIrqHandle < 0 ) {
    	printk( "PS2: Could not get irq %i\n", psPort->nIrq );
		atomic_dec( &psPort->nOpenCount );
		return( -EBUSY );
    }
    
    /* Enable device */
    if( psPort->bIsAux )
		ps2_command( PS2_CMD_AUX_ENABLE );
	nFlg = spinlock_disable( &g_sLock );
	ps2_read_command( PS2_CMD_RCTR, &nControl );
    
	if( psPort->bIsAux ) {
    	nControl &= ~PS2_CTR_AUXDIS;
    	nControl |= PS2_CTR_AUXINT;
    } else {
    	nControl &= ~PS2_CTR_KBDDIS;
    	nControl |= PS2_CTR_KBDINT;
    }
	ps2_write_command( PS2_CMD_WCTR, nControl );
    spinunlock_enable( &g_sLock, nFlg );
  
    atomic_set( &psPort->nBytesReceived, 0 );
    atomic_set( &psPort->nInPos, 0 );
    atomic_set( &psPort->nOutPos, 0 );
    
 
    return( 0 );
}
status_t ps2_close( void* pNode, void* pCookie )
{
    uint32 nFlg;
    PS2_Port_s* psPort = (PS2_Port_s*)pNode;
    
    if ( atomic_dec_and_test( &psPort->nOpenCount ) ) {
		release_irq( psPort->nIrq, psPort->nIrqHandle );
    }
    return( 0 );
}

status_t ps2_ioctl( void* pNode, void* pCookie, uint32 nCommand, void* pArgs, bool bFromKernel )
{
	PS2_Port_s* psPort = (PS2_Port_s*)pNode;
	uint32 nFlg;
	
	if( psPort->bIsAux )
		return( 0 );
		
	
		
	switch( nCommand )
	{
		case IOCTL_KBD_LEDRST:
			g_nKbdLedStatus=0;
			break;

		case IOCTL_KBD_SCRLOC:
			g_nKbdLedStatus ^= 0x01;
			break;

		case IOCTL_KBD_NUMLOC:
			g_nKbdLedStatus ^= 0x02;
			break;

		case IOCTL_KBD_CAPLOC:
			g_nKbdLedStatus ^= 0x04;
			break;

		default:
			printk( "PS2: Unknown IOCTL %x\n",(int)nCommand );
			break;
	}
	
	/* Write command */
	psPort->nAckReceived = 0;
	nFlg = spinlock_disable( &g_sLock );
	ps2_wait_write();
	outb( PS2_CMD_KBD_SETLEDS, PS2_DATA_REG );
	spinunlock_enable( &g_sLock, nFlg );
	int i = 0;
	while( psPort->nAckReceived == 0 && i < 200 )
	{
		snooze( 1000 );
		i++;
	}
	if( psPort->nAckReceived == -1 ) {
		printk( "Could not set LED status: Hardware reported an error for the command!\n" );
		return( 0 );
	}
	if( i == 200 ) {
		printk( "Could not set LED status: Timeout!\n" );
		return( 0 );
	}
	psPort->nAckReceived = 0;
	nFlg = spinlock_disable( &g_sLock );
	ps2_wait_write();
	outb( g_nKbdLedStatus, PS2_DATA_REG );
	spinunlock_enable( &g_sLock, nFlg );
	i = 0;
	while( psPort->nAckReceived == 0 && i < 200 )
	{
		snooze( 1000 );
		i++;
	}
	if( psPort->nAckReceived == -1 ) {
		printk( "Could not set LED status: Hardware reported an error for the data!\n" );
		return( 0 );
	}
	if( i == 200 ) {
		printk( "Could not set LED status: Timeout!\n" );
		return( 0 );
	}
    return( 0 );
}

int ps2_read( void* pNode, void* pCookie, off_t nPosition, void* pBuffer, size_t nSize )
{
	PS2_Port_s* psPort = (PS2_Port_s*)pNode;
    uint32 nFlg;
    int	 nError;
    int	 nBytesReceived = 0;
  
again:
    nFlg = spinlock_disable( &g_sLock );
  
    if ( atomic_read( &psPort->nBytesReceived ) == 0 ) {
	nError = spinunlock_and_suspend( psPort->hWait, &g_sLock, nFlg, INFINITE_TIMEOUT );
	if ( nError < 0 ) {
	    goto error;
	}
	goto again;
    }
    while( atomic_read( &psPort->nBytesReceived ) > 0 && nBytesReceived < nSize ) {
	((char*)pBuffer)[nBytesReceived++] = psPort->pBuffer[ atomic_inc_and_read( &psPort->nOutPos ) % PS2_BUF_SIZE ];
	atomic_dec( &psPort->nBytesReceived );
    }
    spinunlock_enable( &g_sLock, nFlg );
    return( nBytesReceived );
error:
    return( nError );
}

int ps2_write( void* pNode, void* pCookie, off_t nPosition, const void* pBuffer, size_t nSize )
{
	PS2_Port_s* psPort = (PS2_Port_s*)pNode;
    uint32     nFlg;
    int	     i;
  
    for ( i = 0 ; i < nSize ; ++i ) {
    	if( psPort->bIsAux )
	    	ps2_write_command( PS2_CMD_AUX_SEND, ((uint8*)pBuffer)[i] );
    }

    return( nSize );
}


DeviceOperations_s g_sOperations = {
    ps2_open,
    ps2_close,
    ps2_ioctl,
    ps2_read,
    ps2_write
};

/* Initializes the controller and the keyboard part */
static status_t ps2_keyboard_init()
{
	int nError = 0;
	g_nKbdLedStatus = 0;
	
	memset( &g_sKbdPort, 0, sizeof( g_sKbdPort ) );
	
	/* Flush buffer */
	ps2_flush();
	
	/* Read control register */
	uint8 nControl = 0;
	nError = ps2_read_command( PS2_CMD_RCTR, &nControl );
	
	if( nError < 0 ) {
		printk( "PS2 I/O error\n" );
		return( -EIO );
	}
	
	/* TODO: Disable keyboard */
	nControl |= PS2_CTR_KBDDIS;
	nControl &= ~PS2_CTR_KBDINT;
	
	/* Check if translated mode is enabled */
	if( !( nControl & PS2_CTR_XLATE ) )
	{
		printk( "Keyboard is in non-translated mode. This is not supported\n" );
		return( -ENOENT );
	}
	
	/* Write control register */
	nError = ps2_write_command( PS2_CMD_WCTR, nControl );
	if( nError < 0 ) {
		printk( "PS2 I/O error\n" );
		return( -EIO );
	}
	
	g_sKbdPort.bPresent = true;
	g_sKbdPort.hWait = create_semaphore( "ps2_wait", 0, 0 );
	g_sKbdPort.nIrq = 1;
	g_sKbdPort.nDevHandle = register_device( "", "isa" );
	claim_device( g_nDevNum, g_sKbdPort.nDevHandle, "PS/2 or AT keyboard", DEVICE_INPUT );
	set_device_data( g_sKbdPort.nDevHandle, &g_sKbdPort );
	nError = create_device_node( g_nDevNum, g_sKbdPort.nDevHandle, "keybd", &g_sOperations, &g_sKbdPort );
	
	printk( "PS2 or AT Keyboard detected\n" );
	return( 0 );
}

static status_t ps2_aux_init()
{
	int nError = 0;
	struct RMREGS rm;

	/* TODO: Do this without calling the bios */
	memset( &rm, 0, sizeof( struct RMREGS ) );
	realint( 0x11, &rm );

	if( ( rm.EAX & 0x04 ) == 0 ) {
		printk( "No PS2 mouse present\n" );
		return( -EIO );
    }
	
	memset( &g_sAuxPort, 0, sizeof( g_sAuxPort ) );
	g_sAuxPort.bIsAux = true;
	
	/* Flush buffer */
	ps2_flush();
	
	/* Test loop command */
	uint8 nData = 0x5a;
	
	nError = ps2_write_read_command( PS2_CMD_AUX_LOOP, &nData );
	
	if( nError < 0 || nData != 0x5a )
	{
		/* According to linux driver the loop test fails on some chipsets */
		printk( "PS2 Aux loop test failed (error = %i, data = %x)! Trying test command...\n", nError, (uint)nData );
		if( ps2_read_command( PS2_CMD_AUX_TEST, &nData ) < 0 )
		{
			printk( "Failed -> Aux port not present!\n" );
			return( -ENOENT );
		}
		printk( "Test command returned %x\n", (uint)nData );
		if( nData && nData != 0xfa && nData != 0xff )
		{
			printk( "Invalid return code!\n" );
			return( -ENOENT );
		}
	}
	
	/* Disable and then enable the auxport */
	if( ps2_command( PS2_CMD_AUX_DISABLE ) < 0 )
		return( -ENOENT );
	if( ps2_command( PS2_CMD_AUX_ENABLE ) < 0 )
		return( -ENOENT );
	if( ps2_read_command( PS2_CMD_RCTR, &nData ) < 0 || ( nData & PS2_CTR_AUXDIS ) )
		return( -EIO );
		
	/* Disable aux port */
	nData |= PS2_CTR_AUXDIS;
	nData &= ~PS2_CTR_AUXINT;
	
	/* Write control register */
	nError = ps2_write_command( PS2_CMD_WCTR, nData );
	if( nError < 0 ) {
		printk( "PS2 I/O error\n" );
		return( -EIO );
	}

	printk( "PS2 AUX port detected\n" );
	
	/* Register device */
	
	g_sAuxPort.bPresent = true;
	g_sAuxPort.hWait = create_semaphore( "ps2_wait", 0, 0 );
	g_sAuxPort.nDevHandle = register_device( "", "isa" );
	g_sAuxPort.nIrq = 12;
	claim_device( g_nDevNum, g_sAuxPort.nDevHandle, "PS/2 Aux port", DEVICE_PORT );
	set_device_data( g_sAuxPort.nDevHandle, &g_sAuxPort );
	nError = create_device_node( g_nDevNum, g_sAuxPort.nDevHandle, "misc/ps2aux", &g_sOperations, &g_sAuxPort );
	
	if( nError < 0 )
		return( -EIO );
	
	return( 0 );
}


status_t device_suspend( int nDeviceID, int nDeviceHandle, void* pData )
{
	PS2_Port_s* psPort = (PS2_Port_s*)pData;
	uint32 nFlg;
	uint8 nControl;	
	
	
	/* Disable interrupt */
	nFlg = spinlock_disable( &g_sLock );
	ps2_read_command( PS2_CMD_RCTR, &nControl );
    
	if( psPort->bIsAux ) {
		nControl &= ~PS2_CTR_AUXINT;
		nControl |= PS2_CTR_AUXDIS;
		printk( "Suspend AUX port\n" );
	} else {
		nControl &= ~PS2_CTR_KBDINT;
		nControl |= PS2_CTR_KBDDIS;
		printk( "Suspend keyboard port\n" );
	}
	ps2_write_command( PS2_CMD_WCTR, nControl );
	spinunlock_enable( &g_sLock, nFlg );
    return( 0 );
}

status_t device_resume( int nDeviceID, int nDeviceHandle, void* pData )
{
	PS2_Port_s* psPort = (PS2_Port_s*)pData;
	uint32 nFlg;
	uint8 nControl;
	
	/* Enable interrupt */
	nFlg = spinlock_disable( &g_sLock );
	ps2_read_command( PS2_CMD_RCTR, &nControl );
    
    if( atomic_read( &psPort->nOpenCount ) > 0 )
    {
		if( psPort->bIsAux ) {
			nControl &= ~PS2_CTR_AUXDIS;
			nControl |= PS2_CTR_AUXINT;
			printk( "Resume AUX port\n" );
		} else {
			nControl &= ~PS2_CTR_KBDDIS;
			nControl |= PS2_CTR_KBDINT;
			printk( "Resume keyboard port\n" );
    	}
    }
	ps2_write_command( PS2_CMD_WCTR, nControl );
    spinunlock_enable( &g_sLock, nFlg );
    return( 0 );
}

status_t device_init( int nDeviceID )
{
	int i;
	int argc;
	const char *const *argv;
	bool bDisableKeyboard = false;
	bool bDisableAux = false;
	uint32 nFlags;
	
	g_nDevNum = nDeviceID;

	get_kernel_arguments( &argc, &argv );
	
	printk( "PS2 keyboard and mouse driver\n" );

	for ( i = 0; i < argc; ++i )
	{
		if ( get_bool_arg( &bDisableKeyboard, "disable_keyboard=", argv[i], strlen( argv[i] ) ) )
			printk( "PS2 Keyboard support disabled\n" );
		if ( get_bool_arg( &bDisableAux, "disable_aux=", argv[i], strlen( argv[i] ) ) )
			printk( "PS2 AUX support disabled\n" );
	}
	
	if( !bDisableKeyboard )
		if( ps2_keyboard_init() == -EIO )
			return( -1 );
	if( !bDisableAux )
		ps2_aux_init();
	
	return( 0 );
}
	
status_t device_uninit( int nDeviceID )
{
    return( 0 );
}
































