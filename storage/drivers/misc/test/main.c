#include <atheos/kernel.h>
#include <atheos/device.h>

#include <macros.h>

status_t tst_open( void* pNode, uint32 nFlags, void **pCookie )
{
    printk( "open called\n" );
    return( 0 );
}

status_t tst_close( void* pNode, void* pCookie )
{
    printk( "close called\n" );
    return( 0 );
}

status_t tst_ioctl( void* pNode, void* pCookie, uint32 nCommand, void* pArgs, bool bFromKernel )
{
    printk( "ioctl called\n" );
    return( 0 );
}

int  tst_read( void* pNode, void* pCookie, off_t nPosition, void* pBuffer, size_t nSize )
{
    char* zBuf = "Test device test string\n";

    printk( "read %d, %d called\n", (int) nPosition, nSize );
  
    if ( nPosition + nSize > strlen( zBuf ) ) {
	nSize = min( strlen( zBuf ) - nPosition, nSize );
    }
    if ( nSize > 0 ) {
	memcpy( pBuffer, zBuf + nPosition, nSize );
    } else {
	nSize = 0;
    }
    return( nSize );
}

int  tst_write( void* pNode, void* pCookie, off_t nPosition, const void* pBuffer, size_t nSize )
{
    printk( "write %d, %d called\n", (int) nPosition, nSize );
    return( nSize );
}

DeviceOperations_s g_sOperations = {
    tst_open,	// dop_open
    tst_close,	// dop_close
    tst_ioctl,	// dop_ioctl
    tst_read,	// dop_read
    tst_write,	// dop_write
    NULL,	// dop_readv
    NULL,	// dop_writev
    NULL,	// dop_add_select_req
    NULL	// dop_rem_select_req
};

status_t device_init( int nDeviceID )
{
    int nError;
    printk( "Test device: device_init() called\n" );
  
    nError = create_device_node( nDeviceID, "misc/test", &g_sOperations, NULL );

    if ( nError < 0 ) {
	printk( "Failed to create 1 node %d\n", nError );
    }
  
    nError = create_device_node( nDeviceID, "misc/test", &g_sOperations, NULL );

    if ( nError < 0 ) {
	printk( "Failed to create 2 node %d\n", nError );
    }
  
  
    return( nError );
}

status_t device_uninit( int nDeviceID )
{
    printk( "Test device: device_uninit() called\n" );
    return( 0 );
}
