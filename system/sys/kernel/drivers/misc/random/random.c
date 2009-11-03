// Rudimentary /dev/random
// by Jonas Sundstrom, Kirilla.com
// 
// A mutation of Marcus Lundblad's /dev/random for BeOS
// and the AtheOS keyboard driver.
//
// Modified, fixed and extended by Kristian Van Der Vliet
// (vanders@users.sourceforge.net) for Syllable

#include <kernel/types.h>
#include <kernel/device.h>
#include <kernel/random.h>

static int random_open( void* node, uint32 flags, void** cookie )
{
	return( 0 );
}

static int random_close( void* node, void* cookie )
{
    return( 0 );
}


static int random_read( void* node, void* cookie, off_t pos, void* buf, size_t length )
{
	int* buffer = (int*)buf;
	int count = length / sizeof(int);
	int n;

	for( n = 0;  n < count;  n++, buffer++ )
		*buffer = rand();

	return( count * sizeof(int) );
}


DeviceOperations_s random_ops = 
{
	random_open,
	random_close,
	NULL,		/* ioctl */
	random_read,
	NULL		/* write */
};

status_t device_init( int device_id )
{
	return( create_device_node( device_id, -1, "random", &random_ops, NULL ) );
}

status_t device_uninit( int device_id )
{
	return( delete_device_node( device_id ) );
}
