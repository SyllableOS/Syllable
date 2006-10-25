/*
 *  SoundBlaster Pro Driver for the AtheOS kernel
 *  Copyright (C) 2000  Joel Smith
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
 * Revision History :
 *		19/08/00      Initial version           Joel Smith
 *							<joels@mobyfoo.org>
 *
 *		29/12/00      Fixed for AtheOS 0.3.0    Kristian Van Der Vliet
 *		08/07/02      Initalise correctly	Kristian Van Der Vliet
 */
                 
#include <posix/errno.h>
#include <posix/ioctls.h>
#include <posix/fcntl.h>
#include <posix/termios.h>

#include <atheos/kernel.h>
#include <atheos/device.h>
#include <atheos/semaphore.h>
#include <atheos/irq.h>
#include <atheos/dma.h>
#include <atheos/soundcard.h>
#include <atheos/spinlock.h>
#include <macros.h>

#include "sbpro.h"

static SpinLock_s spin_lock = INIT_SPIN_LOCK( "dsp_slock" );

/* Macros for port I/O. */
#define dsp_out( reg, val )	isa_writeb( (reg), (val) )
#define dsp_in( reg ) 		isa_readb( (reg) )

#define DEBUG( str, ... )	printk( "%s: " str, __FUNCTION__, ## __VA_ARGS__ )

#define DEV_READ	0
#define DEV_WRITE	1

static int dsp_stereo;
static int dsp_speed;
static int version[2];

static int dsp_found = 0;
static int dsp_in_use = 0;
static int dsp_highspeed = 0;
static uchar dsp_tconst = 0;

static char *dma_buffer;

static int open_mode = 0;

static int irq_handle;
static int irq_lock;

static int nDeviceNode = 0;

static int
sb_dsp_command( int com )
{
	register int i;
	
	for( i = 0; i < SB_TIMEOUT; i++ ) {
		if ( (dsp_in( SB_DSP_STATUS ) & 0x80) == 0 ) {
			dsp_out( SB_DSP_COMMAND, com );
			return 1;
		}
	}
	
	DEBUG( "DSP command timeout.\n" );
	return(0);
}
                                                                                                        
static int
sb_dsp_reset( void )
{
	int i;
	
	DEBUG( "Resetting device.\n" );
	
	dsp_out( SB_DSP_RESET, 1 );
	for( i = 0; i < SB_TIMEOUT; i++ ) ;
	/*udelay( 10 );*/
	dsp_out( SB_DSP_RESET, 0 );
	for( i = 0; i < SB_TIMEOUT; i++ ) ;
	/*udelay( 30 );*/
	
	for (i = 0; i < SB_TIMEOUT && !(dsp_in( SB_DSP_DATA_AVL )&0x80); i++);

	if( dsp_in( SB_DSP_READ ) != 0xAA ) {
		DEBUG( "Reset failed.\n" );
		return 0;
	}
	
	DEBUG( "Reset OK.\n" );
	return(1);
}

static void
sb_dsp_speaker( int state )
{
	if( state )
		sb_dsp_command( SB_DSP_CMD_SPKON );
	else
		sb_dsp_command( SB_DSP_CMD_SPKOFF );

	return;
}

static int
sb_dsp_init( void )
{
	int i;
	
	DEBUG( "Initializing SBPro dsp.\n" );
	
	if( !sb_dsp_reset() ) {
		DEBUG( "SoundBlaster card not found.\n" );
		return(0);
	}
	
	version[0] = version[1] = 0;
	sb_dsp_command( SB_DSP_CMD_GETVER );
	
	for( i = 0; i < SB_TIMEOUT; i++ ) {
		if( dsp_in( SB_DSP_DATA_AVL ) & 0x80 ) {
			if( version[0] == 0 )
				version[0] = dsp_in( SB_DSP_READ );
			else {
				version[1] = dsp_in( SB_DSP_READ );
				break;
			}
		}
	}
	
	DEBUG( "DSP version %d.%d.\n", version[0], version[1] );
	
	return(1);
}

static int
sb_irq( int irq, void *data, SysCallRegs_s *regs )
{
	dsp_in( SB_DSP_DATA_AVL );

	free_dma( SB_DMA );
	sb_dsp_speaker( 0 );
		
	irq_lock = 0;
		
	return(0);
}

static status_t
sb_set_irq( void )
{
	DEBUG( "Setting up irq %d.\n", SB_IRQ );
	
	irq_handle = request_irq( SB_IRQ, sb_irq, NULL, 0, "sb_device", NULL );

	return (irq_handle < 0) ? 0 : 1;
}

static status_t
sb_dsp_set_stereo( int stereo )
{
	if( version[0] < 3 )
		return(0);
		
	dsp_stereo = (stereo <= 0) ? 0 : 1;
	return(dsp_stereo);
}

static int
sb_dsp_speed( int speed )
{
	int error = 0;
	uint8 tconst;
		
	if( speed < SB_MIN_SPEED ) {
		speed = SB_MIN_SPEED;
		error = 1;
	}
	else if( speed > SB_MAX_SPEED ) {
		speed = SB_MAX_SPEED;
		error = 1;
	}
		
	if( dsp_stereo && speed > 22050 ) {
		speed = 22050;
		error = 1;
	}
	
	if( dsp_stereo )
		speed *= 2;
	
	if( speed > 22050 ) {
		tconst = (uint8) ((65536 - ((256000000 + speed / 2) / 
			speed)) >> 8);
		dsp_highspeed = 1;
		
		dsp_tconst = tconst;
		
		speed = 65536 - (tconst << 8);
		speed = (256000000 + speed / 2) / speed;
	}
	else {
		tconst = (256 - ((1000000 + speed / 2) / speed)) & 0xff;
		dsp_highspeed = 0;
		
		dsp_tconst = tconst;
		
		speed = 256 - tconst;
		speed = (1000000 + speed / 2) / speed;
	}
	
	if( dsp_stereo )
		speed /= 2;
		
	dsp_speed = speed;

	return (error == 1) ? -EINVAL : speed;
}

static status_t
sb_dma_setup( void *address, int count )
{
	uint32 flg;
	
	/* should lock here. */
	flg = spinlock_disable( &spin_lock );
		
	if( request_dma( SB_DMA ) < 0 ) {
		DEBUG( "Request of DMA channel Failed.\n" );
		return -EBUSY;
	}
		
	disable_dma_channel( SB_DMA );
	clear_dma_ff( SB_DMA );

	set_dma_mode( SB_DMA, (open_mode == DEV_WRITE) ?
		SB_DMA_PLAY : SB_DMA_REC );
			
	set_dma_address( SB_DMA, address );
	set_dma_page( SB_DMA, (int)address >> 16 );
	set_dma_count( SB_DMA, count );
		
	enable_dma_channel( SB_DMA );
		
	/* unlock. */
	spinunlock_enable( &spin_lock, flg );
		
	return(1);
}

static status_t
sb_dsp_setup( int count )
{
	uint32 flg;
			
	count--;
	flg = spinlock_disable( &spin_lock );
	
	if( open_mode == DEV_WRITE ) {
		if( dsp_highspeed ) {
			sb_dsp_command(SB_DSP_CMD_HSSIZE);
			sb_dsp_command((uint8) (count & 0xff));
			sb_dsp_command((uint8) ((count >> 8) & 0xff));
			sb_dsp_command(SB_DSP_CMD_HSDAC);
		}
		else {
			sb_dsp_command(SB_DSP_CMD_DAC8);
			sb_dsp_command((uint8) (count & 0xff) );	
			sb_dsp_command((uint8) ((count >> 8) & 0xff));
		}
	}
	else {
		if( dsp_highspeed ) {
			sb_dsp_command(SB_DSP_CMD_HSSIZE);
			sb_dsp_command((uint8) (count & 0xff));
			sb_dsp_command((uint8) ((count >> 8) & 0xff));
			sb_dsp_command(SB_DSP_CMD_HSADC);
		}
		else {
			sb_dsp_command(SB_DSP_CMD_ADC8);
			sb_dsp_command((uint8) (count & 0xff));
			sb_dsp_command((uint8) ((count >> 8) & 0xff));
		}
	}
	spinunlock_enable( &spin_lock, flg );
		
	return(1);
}

static status_t sb_write( void *node, void *cookie, off_t ps, const void *buffer, size_t size )
{
	int count;
	uchar i;

	if( size > SB_DMA_SIZE )
		count = SB_DMA_SIZE;
	else
		count = size;

	open_mode = DEV_WRITE;

	/* Wait for last transfer to complete. */
	while( atomic_swap( (int *)&irq_lock, 1 ) != 0 )
		/* do nothing. */ ;

	memcpy( dma_buffer, buffer, count );

	if( dsp_stereo ) {
		dsp_out( SB_MIXER_ADDR, 0x0E );
		i = dsp_in( SB_MIXER_DATA );
		/*dsp_out( SB_MIXER_ADDR, 0x0E );*/
		dsp_out( SB_MIXER_DATA, i | 0x02 );
	}

        sb_dsp_command( SB_DSP_CMD_TCONST );
        sb_dsp_command( dsp_tconst );
        sb_dsp_speaker( 1 );
	
	sb_dma_setup( dma_buffer, count );
	sb_dsp_setup( count );
	
	return(count);
}

static int sb_read( void *node, void *cookie, off_t ps, void *buffer, size_t size )
{
	int count;

	while( atomic_swap( (int *)&irq_lock, 1 ) != 0 )
		/* do nothing. */ ;
		
	if( size > SB_DMA_SIZE )
		count = SB_DMA_SIZE;
	else
		count = size;

	open_mode = DEV_READ;
	
	if( dsp_stereo )
		sb_dsp_command( 0xa8 );
	else
		sb_dsp_command( 0xa0 );
		
	sb_dsp_command( SB_DSP_CMD_TCONST );
        sb_dsp_command( dsp_tconst );
        
        sb_dma_setup( dma_buffer, count );
	sb_dsp_setup( count );

        /* Wait until DMA is complete. */
        while( atomic_swap( (int *)&irq_lock, 1 ) != 0 )
                        /* do nothing. */ ;
        
        memcpy( (char *)buffer, dma_buffer, count );
        	
	return(count);
}

static status_t 
sb_ioctl( void *node, void *cookie, uint32 com, void *args, bool bFromKernel )
{
	switch( com ) {
		case SOUND_PCM_WRITE_RATE :
			return sb_dsp_speed( *((int *)args) );
		
		case SOUND_PCM_READ_RATE :
			return dsp_speed;
			
		case SOUND_PCM_WRITE_CHANNELS :
			return sb_dsp_set_stereo( *((int *)args) - 1 ) + 1;
		
		case SOUND_PCM_READ_CHANNELS :
			return dsp_stereo + 1;
			
		case SNDCTL_DSP_STEREO :
			return sb_dsp_set_stereo( *((int *)args) );
		
		/*case SOUND_PCM_WRITE_BITS :
		case SOUND_PCM_READ_BITS :
			return 8;*/
		
		case SNDCTL_DSP_GETBLKSIZE :
			return SB_DMA_SIZE;
		
		case SNDCTL_DSP_RESET :
			return sb_dsp_reset();
		
		case SNDCTL_DSP_GETFMTS :
			return 8;
			
		case SNDCTL_DSP_SETFMT :
			if( *((int *)args) != 8 )
				return -EINVAL;
			return 1;

		case IOCTL_GET_USERSPACE_DRIVER:
		{
			memcpy_to_user( args, "oss.so", strlen( "oss.so" ) );
			return( 0 );
		}

		default :
			return -EINVAL;
	}
	
	return -EINVAL;
}

status_t
sb_open( void* pNode, uint32 nFlags, void **pCookie )
{
	DEBUG( "Opening device.\n" );
	
	if( dsp_in_use == 1 ) {
		DEBUG( "Device already in use.\n" );
		return -EBUSY;
	}
	
	if( !dsp_found && !sb_dsp_init() )
		return -EIO;
	
	if( !sb_dsp_reset() )
		return -EIO;
	
	dsp_stereo = 0;
	dsp_speed = SB_DEFAULT_SPEED;
	sb_dsp_speed( dsp_speed );
	irq_lock = 0;
	
	if( !sb_set_irq() ) {
		DEBUG( "Requesting of IRQ Failed.\n" );
		return -EIO;
	}

	/* If soundcard as not been used before then setup irq and allocate dma
	 * memory.  */
	if( !dsp_found ) {
		dma_buffer = alloc_real( SB_DMA_SIZE, 0 );
		if( dma_buffer == NULL ) {
			DEBUG( "Out of memory.\n" );
			return -ENOMEM;
		}
	}
			
	dsp_found = 1;
	dsp_in_use = 1;
	
	return 0;		/* Shouldn't this be 1? */
}

status_t
sb_close( void* pNode, void* pCookie )
{
	DEBUG( "Closing device.\n" );
	
	release_irq( SB_IRQ, irq_handle );
	free_dma( SB_DMA );
	
	dsp_in_use = 0;
	
	return 0;
}

DeviceOperations_s dsp_ops = {
	sb_open,
	sb_close,
	sb_ioctl,
	sb_read,
	sb_write
};

status_t 
device_init( int nDeviceID )
{
	int nError = -1;
	int nHandle;
	
	DEBUG( "Initializing device\n" );
	/* See if we can find an SB Pro device */
	if( sb_dsp_init() ) {
		nHandle = register_device( "", "isa" );
		claim_device( nDeviceID, nHandle, "Soundblaster Pro", DEVICE_AUDIO );
		nError = nDeviceNode = create_device_node( nDeviceID, nHandle, "audio/sbpro", &dsp_ops, NULL );
	}
	if ( nError < 0 )
		DEBUG( "Failed to create 1 node %d\n", nError );
		
	
	return nError;
}

status_t
device_uninit( int nDeviceID )
{
	DEBUG( "Unitializing device\n" );
	delete_device_node( nDeviceNode );

        return( 0 );
}




