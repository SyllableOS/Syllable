#include <posix/errno.h>
#include <posix/ioctls.h>
#include <posix/fcntl.h>
#include <posix/termios.h>

#include <atheos/kernel.h>
#include <atheos/device.h>
#include <atheos/semaphore.h>
#include <atheos/spinlock.h>
#include <atheos/irq.h>

#include "serial_reg.h"

#include <macros.h>

static SpinLock_s g_sSPinLock = INIT_SPIN_LOCK( "ser_slock" );

#define RECV_BUF_SIZE 4096

static int g_nIRQHandle;

typedef struct
{
    struct termios sp_sTermios;
    int	 	 sp_nPortBase;
    int	 	 sp_nBaudRate;
    sem_id 	 sp_hRecvMutex;
    sem_id 	 sp_hRecvWaitQueue;
    int	 	 sp_nRecvInPos;
    int	 	 sp_nRecvOutPos;
    int	 	 sp_nRecvSize;	// Number of bytes in sp_anReceiveBuffer
    char	 sp_anReceiveBuffer[RECV_BUF_SIZE];
    int	 	 sp_nDevNode;
    char	 sp_zName[128];
    uint32	 sp_nFlags;
    uint32 	 sp_nMCR;
    bool	 sp_bOpen;
} SerPort_s;


static SerPort_s g_asPorts[8];

#define ser_out( port, reg, val ) isa_writeb( (port)->sp_nPortBase + (reg), (val) )
#define ser_in( port, reg )       isa_readb( (port)->sp_nPortBase + (reg) )

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

status_t ser_open( void* pNode, uint32 nFlags, void **pCookie )
{
    SerPort_s* psPort = pNode;
    uint	     nDivisor = 115200 / psPort->sp_nBaudRate;
    uint32     nFlg;

    if ( psPort->sp_bOpen == true ) {
	printk( "ser_open(): port is already open\n" );
	return( -EBUSY );
    }
    psPort->sp_bOpen = true;
    psPort->sp_nFlags         = nFlags;
    psPort->sp_hRecvMutex	    = create_semaphore( "ser_recv_mutex", 1, 0 );
    psPort->sp_hRecvWaitQueue = create_semaphore( "ser_recv_queue", 0, 0 );
    psPort->sp_nRecvInPos	    = 0;
    psPort->sp_nRecvOutPos    = 0;
    psPort->sp_nRecvSize	    = 0;
    psPort->sp_nMCR	    = 0x0f;
  
    nFlg = spinlock_disable( &g_sSPinLock );
  
    ser_out( psPort, UART_LCR, UART_LCR_DLAB | UART_LCR_WLEN7 | UART_LCR_STOP ); // Set UART_LCR_DLAB to enable baud rate divisors
  
    ser_out( psPort, UART_DLL, nDivisor & 0xff ); // Baud rate divisor LSB
    ser_out( psPort, UART_DLM, nDivisor >> 8  ); // Baud rate divisor MSB
 
    ser_out( psPort, UART_LCR, UART_LCR_WLEN7 | UART_LCR_STOP ); // Clr UART_LCR_DLAB to disable baud rate divisors


      // Enable FIFO, IRQ when 8 bytes received
    ser_out( psPort, UART_FCR, /*UART_FCR_ENABLE_FIFO | UART_FCR6_R_TRIGGER_24*/ 0 ); 
    ser_out( psPort, UART_IER, UART_IER_RDI ); // receive irq enabled

    ser_out( psPort, UART_MCR, psPort->sp_nMCR );
  


      // Clear interrupt registers
    ser_in( psPort, UART_LSR );  // Line status (LSR)
    ser_in( psPort, UART_RX );
    ser_in( psPort, UART_IIR );  // Check interrupt type (IIR)
    ser_in( psPort, UART_MSR );  // Check modem status (MSR)

    spinunlock_enable( &g_sSPinLock, nFlg );
  
    return( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static status_t ser_close( void* pNode, void* pCookie )
{
    SerPort_s* psPort = pNode;
  
    delete_semaphore( psPort->sp_hRecvMutex );
    delete_semaphore( psPort->sp_hRecvWaitQueue );

    psPort->sp_bOpen = false;
    return( 0 );
}

/*****************************************************************************
 * NAME:
 * 	get_lsr_info - get line status register info
 * DESC:
 *	Let user call ioctl() to get info when the UART physically
 * 	is emptied.  On bus types like RS485, the transmitter must
 * 	release the bus after transmitting. This must be done when
 * 	the transmit shift register is empty, not be done when the
 * 	transmit holding register is empty.  This functionality
 * 	allows an RS485 driver to be written in user space. 
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int get_lsr_info( SerPort_s* psPort, uint32* value, bool bFromKernel )
{
    uint8  status;
    uint32 result;
    uint32 flags;

    flags = spinlock_disable( &g_sSPinLock );
    status = ser_in( psPort, UART_LSR );
    spinunlock_enable( &g_sSPinLock, flags );
    result = ((status & UART_LSR_TEMT) ? TIOCSER_TEMT : 0);

    if ( bFromKernel ) {
	*value = result;
	return( 0 );
    } else {
	return( memcpy_to_user( value, &result, sizeof(uint32) ) );
    }
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int set_modem_info( SerPort_s* psPort, int cmd, uint32* value, bool bFromKernel )
{
    int error;
    unsigned int arg;
    unsigned long flags;

    if ( bFromKernel ) {
	arg = *value;
	error = 0;
    } else {
	error = memcpy_from_user( &arg, value, sizeof(uint32) );
    }
    if (error)
	return error;
  
    switch (cmd) {
	case TIOCMBIS: 
	    if (arg & TIOCM_RTS)
		psPort->sp_nMCR |= UART_MCR_RTS;
	    if (arg & TIOCM_DTR)
		psPort->sp_nMCR |= UART_MCR_DTR;
#ifdef TIOCM_OUT1
	    if (arg & TIOCM_OUT1)
		psPort->sp_nMCR |= UART_MCR_OUT1;
	    if (arg & TIOCM_OUT2)
		psPort->sp_nMCR |= UART_MCR_OUT2;
#endif
	    break;
	case TIOCMBIC:
	    if (arg & TIOCM_RTS)
		psPort->sp_nMCR &= ~UART_MCR_RTS;
	    if (arg & TIOCM_DTR)
		psPort->sp_nMCR &= ~UART_MCR_DTR;
#ifdef TIOCM_OUT1
	    if (arg & TIOCM_OUT1)
		psPort->sp_nMCR &= ~UART_MCR_OUT1;
	    if (arg & TIOCM_OUT2)
		psPort->sp_nMCR &= ~UART_MCR_OUT2;
#endif
	    break;
	case TIOCMSET:
	    psPort->sp_nMCR = ((psPort->sp_nMCR & ~(UART_MCR_RTS |
#ifdef TIOCM_OUT1
						    UART_MCR_OUT1 |UART_MCR_OUT2 |
#endif
						    UART_MCR_DTR)) | ((arg & TIOCM_RTS) ? UART_MCR_RTS : 0)
#ifdef TIOCM_OUT1
			       | ((arg & TIOCM_OUT1) ? UART_MCR_OUT1 : 0)
			       | ((arg & TIOCM_OUT2) ? UART_MCR_OUT2 : 0)
#endif
			       | ((arg & TIOCM_DTR) ? UART_MCR_DTR : 0));
	    break;
	default:
	    return -EINVAL;
    }
    flags = spinlock_disable( &g_sSPinLock );
    ser_out( psPort, UART_MCR, psPort->sp_nMCR );
    spinunlock_enable( &g_sSPinLock, flags );
    return 0;
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int get_modem_info( SerPort_s* psPort, uint32* value, bool bFromKernel )
{
    uint8 control, status;
    uint32 result;
    uint32 flags;

    control = psPort->sp_nMCR;

    flags = spinlock_disable( &g_sSPinLock );
    status = ser_in( psPort, UART_MSR );
    spinunlock_enable( &g_sSPinLock, flags );
  
    result =  ((control & UART_MCR_RTS) ? TIOCM_RTS : 0)
	| ((control & UART_MCR_DTR) ? TIOCM_DTR : 0)
#ifdef TIOCM_OUT1
	| ((control & UART_MCR_OUT1) ? TIOCM_OUT1 : 0)
	| ((control & UART_MCR_OUT2) ? TIOCM_OUT2 : 0)
#endif
	| ((status  & UART_MSR_DCD) ? TIOCM_CAR : 0)
	| ((status  & UART_MSR_RI) ? TIOCM_RNG : 0)
	| ((status  & UART_MSR_DSR) ? TIOCM_DSR : 0)
	| ((status  & UART_MSR_CTS) ? TIOCM_CTS : 0);

    if ( bFromKernel ) {
	*value = result;
	return( 0 );
    } else {
	return( memcpy_to_user( value, &result, sizeof(uint32) ) );
    }
}

static int set_termios( SerPort_s* psPort, struct termios* psInfo )
{
    struct termios sTermios;
    int	 nError;

    nError = memcpy_from_user( &sTermios, psInfo, sizeof( sTermios ) );

    if ( nError < 0 ) {
	return( nError );
    }
  
    if ( (psPort->sp_sTermios.c_cflag & CBAUD) != (sTermios.c_cflag & CBAUD) ) {
	uint32 nFlg;
	switch( sTermios.c_cflag & CBAUD )
	{
	    case B0:		psPort->sp_nBaudRate = 0;
	    case B50:		psPort->sp_nBaudRate = 50;
	    case B75:		psPort->sp_nBaudRate = 75;
	    case B110:	psPort->sp_nBaudRate = 110;
	    case B134:	psPort->sp_nBaudRate = 134;
	    case B150:	psPort->sp_nBaudRate = 150;
	    case B200:	psPort->sp_nBaudRate = 200;
	    case B300:	psPort->sp_nBaudRate = 300;
	    case B600:	psPort->sp_nBaudRate = 600;
	    case B1200:	psPort->sp_nBaudRate = 1200;
	    case B1800:	psPort->sp_nBaudRate = 1800;
	    case B2400:	psPort->sp_nBaudRate = 2400;
	    case B4800:	psPort->sp_nBaudRate = 4800;
	    case B9600:	psPort->sp_nBaudRate = 9600;
	    case B19200:	psPort->sp_nBaudRate = 19200;
	    case B38400:	psPort->sp_nBaudRate = 38400;
	    case B57600:	psPort->sp_nBaudRate = 57600;
	    case B115200:	psPort->sp_nBaudRate = 115200;
//      case B230400:	psPort->sp_nBaudRate = 230400;
//      case B460800:	psPort->sp_nBaudRate = 460800;
	    default:
		printk( "serial: set_termios() invalid baudrate %08x\n", sTermios.c_cflag & CBAUD );
		return( -EINVAL );
	}
	nFlg = spinlock_disable( &g_sSPinLock );
	if ( psPort->sp_nBaudRate > 0 ) {
	    uint nDivisor = 115200 / psPort->sp_nBaudRate;
	    ser_out( psPort, UART_LCR, 0x83 ); // Set bit 7 to enable baud rate divisors
	    ser_out( psPort, UART_DLL, nDivisor & 0xff ); // Baud rate divisor LSB
	    ser_out( psPort, UART_DLM, nDivisor >> 8  ); // Baud rate divisor MSB
	    ser_out( psPort, UART_LCR, 0x03 ); // Clr bit 7 to disable baud rate divisors
	}
	spinunlock_enable( &g_sSPinLock, nFlg );
    }
    psPort->sp_sTermios = sTermios;
    return( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static status_t ser_ioctl( void* pNode, void* pCookie, uint32 nCommand, void* pArgs, bool bFromKernel )
{
    SerPort_s* psPort = pNode;
  
    switch( nCommand )
    {
	case TIOCMGET:
	    return( get_modem_info( psPort, pArgs, bFromKernel ) );
	case TIOCMBIS:
	case TIOCMBIC:
	case TIOCMSET:
	    return( set_modem_info( psPort, nCommand, pArgs, bFromKernel ) );
//    case TIOCGSERIAL:
//      return( get_serial_info( psPort, (struct serial_struct *) pArgs ) );
//    case TIOCSSERIAL:
//      return( set_serial_info( psPort, (struct serial_struct *) pArgs ) );
	case TIOCSERGETLSR: /* Get line status register */
	    return( get_lsr_info( psPort, (uint32*)pArgs, bFromKernel ) );

	case TCSETA:
	case TCSETAW:
	case TCSETAF:
	    if ( bFromKernel ) {
		return( set_termios( psPort, (struct termios*) pArgs ) );
	    } else {
		struct termios sTerm;
		int nError;
		nError = memcpy_from_user( &sTerm, pArgs, sizeof(sTerm) );
		if ( nError >= 0 ) {
		    nError = set_termios( psPort, &sTerm );
		}
		return( nError );
	    }
	case TCGETA:
	    if ( bFromKernel ) {
		memcpy( pArgs, &psPort->sp_sTermios, sizeof( struct termios ) );
		return( 0 );
	    } else {
		return( memcpy_to_user( pArgs, &psPort->sp_sTermios, sizeof( struct termios ) ) );
	    }
	default:
	    printk( "ser_ioctl() invalid command %ld\n", nCommand );
	    return( -EINVAL );
    }
    return( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int  ser_read( void* pNode, void* pCookie, off_t nPosition, void* pBuffer, size_t nSize )
{
    SerPort_s* psPort = pNode;
    uint32 nFlg;
    int	 nError;
    int	 nBytesReceived = 0;
  
    LOCK( psPort->sp_hRecvMutex );
again:
    nFlg = spinlock_disable( &g_sSPinLock );
  
    if ( psPort->sp_nRecvSize == 0 ) {
	if ( psPort->sp_nFlags & O_NONBLOCK ) {
	    nError = -EWOULDBLOCK;
	} else {
	    nError = spinunlock_and_suspend( psPort->sp_hRecvWaitQueue, &g_sSPinLock, nFlg, INFINITE_TIMEOUT );
	}
	if ( nError < 0 ) {
	    if ( nError != -EINTR ) {
		spinunlock_enable( &g_sSPinLock, nFlg );
	    }
	    goto error;
	}
	goto again;
    }
    spinunlock_enable( &g_sSPinLock, nFlg );
    while( psPort->sp_nRecvSize > 0 && nBytesReceived < nSize ) {
	((char*)pBuffer)[nBytesReceived++] = psPort->sp_anReceiveBuffer[psPort->sp_nRecvOutPos++ % RECV_BUF_SIZE ];
	psPort->sp_nRecvSize--;
    }
    UNLOCK( psPort->sp_hRecvMutex );
    return( nBytesReceived );
error:
    UNLOCK( psPort->sp_hRecvMutex );
    return( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int  ser_write( void* pNode, void* pCookie, off_t nPosition, const void* pBuffer, size_t nSize )
{
    SerPort_s* psPort = pNode;
    uint32     nFlg;
    int	     i;
  
    nFlg = spinlock_disable( &g_sSPinLock );
    for ( i = 0 ; i < nSize ; ++i ) {
	while( (ser_in( psPort, UART_LSR ) & 0x40) == 0 )
	      /*** EMPTY ***/;
	ser_out( psPort, UART_TX, ((char*)pBuffer)[i] );
    }
    spinunlock_enable( &g_sSPinLock, nFlg );
    return( nSize );
}

DeviceOperations_s g_sOperations = {
    ser_open,
    ser_close,
    ser_ioctl,
    ser_read,
    ser_write
};

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static void ser_receive_data( SerPort_s* psPort )
{
    bool bWakeup = false;
    while ( psPort->sp_nRecvSize < RECV_BUF_SIZE && (ser_in( psPort, UART_LSR ) & 0x01) ) {
	psPort->sp_anReceiveBuffer[psPort->sp_nRecvInPos++ % RECV_BUF_SIZE ] = ser_in( psPort, UART_RX );
	psPort->sp_nRecvSize++;
	bWakeup = true;
    }
    if ( bWakeup ) {
	wakeup_sem( psPort->sp_hRecvWaitQueue, false );
    }
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int ser_irq( int nIrqNum, void* pData, SysCallRegs_s* psRegs )
{
    SerPort_s* psPort = pData;

    if ( psPort != &g_asPorts[0] ) {
	printk( "ser_irq(): Bad port\n" );
    }
    ser_in( psPort, UART_LSR );  // Line status (LSR)
    ser_in( psPort, UART_IIR );  // Check interrupt type (IIR)
    ser_in( psPort, UART_MSR );  // Check modem status (MSR)
  
    ser_receive_data( psPort );
  
    return( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

status_t device_init( int nDeviceID )
{
    int nError;
    int nHandle;
    printk( "Serial device: device_init() called\n" );

    g_nIRQHandle = request_irq( 4, ser_irq, NULL, 0, "ser_device", &g_asPorts[0] );

    if ( g_nIRQHandle < 0 ) {
	printk( "Serial device failed to install IRQ handler\n" );
	nError = g_nIRQHandle;
	goto error1;
    }

    strcpy( g_asPorts[0].sp_zName, "com0" );
    g_asPorts[0].sp_nPortBase = 0x3f8;
    g_asPorts[0].sp_nBaudRate = 1200;
    g_asPorts[0].sp_bOpen     = false;

    strcpy( g_asPorts[1].sp_zName, "com1" );
    g_asPorts[1].sp_nPortBase = 0x2f8;
    g_asPorts[1].sp_nBaudRate = 1200;
    g_asPorts[1].sp_bOpen     = false;
  
  	nHandle = register_device( "", "system" );
  	claim_device( nDeviceID, nHandle, "Serial port 1", DEVICE_PORT );
    g_asPorts[0].sp_nDevNode = create_device_node( nDeviceID, nHandle, "misc/com/1", &g_sOperations, &g_asPorts[0] );
    if ( g_asPorts[0].sp_nDevNode < 0 ) {
	nError = g_asPorts[0].sp_nDevNode;
	printk( "Error: Serial device failet to create device node misc/com/1\n" );
	goto error2;
    }
    nHandle = register_device( "", "system" );
  	claim_device( nDeviceID, nHandle, "Serial port 2", DEVICE_PORT );
    g_asPorts[1].sp_nDevNode = create_device_node( nDeviceID, nHandle, "misc/com/2", &g_sOperations, &g_asPorts[1] );
    if ( g_asPorts[1].sp_nDevNode < 0 ) {
	nError = g_asPorts[1].sp_nDevNode;
	printk( "Error: Serial device failet to create device node misc/com/1\n" );
	goto error3;
    }
    
    
  
    return( 0 );
error3:
    delete_device_node( g_asPorts[0].sp_nDevNode );
error2:
    release_irq( 4, g_nIRQHandle );
error1:
    return( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

status_t device_uninit( int nDeviceID )
{
    printk( "Serial device: device_uninit() called\n" );
    release_irq( 4, g_nIRQHandle );
    return( 0 );
}
