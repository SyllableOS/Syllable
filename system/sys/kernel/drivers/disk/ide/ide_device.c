/*
 *  The AltOS kernel
 *  Copyright (C) 1999  Kurt Skauen
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
 */

//-----------------------------------------------------------------------------

//extern "C" {
#include <posix/errno.h>

#include <atheos/kernel.h>
#include <atheos/types.h>
#include <atheos/device.h>
#include <atheos/semaphore.h>
#include <atheos/isa_io.h>
//}

#include "ide.h"

//-----------------------------------------------------------------------------

static int ide_open( void* pNode, uint32 nFlags, void **ppCookie );
static int ide_close( void* pNode, void* pCookie );
static int ide_read( void* pNode, void* pCookie, off_t nPos, void* pBuf, size_t nLen );
static int ide_write( void* pNode, void* pCookie, off_t nPos, const void* pBuf, size_t nLen );
static status_t ide_ioctl( void* pNode, void* pCookie, uint32 nCommand, void* pArgs, size_t nLen );

static DeviceOperations_s ide_functions =
{
  ide_open,
  ide_close,
  ide_ioctl,
  ide_read,
  ide_write
};

//-----------------------------------------------------------------------------

#define DEVICENAME "ide"

AtaController *ControllerList = NULL;

//-----------------------------------------------------------------------------

#ifdef __IDE_EXTERNAL
status_t device_init( int nDeviceID )
#else
status_t init_ide( int nDeviceID )
#endif
{
    int i, j;
    AtaController **nextnode;
    status_t status;
    // FIXME: get these from isa p'n'p or pci.
    static int controllers[][3] =
    {
	//used through bios  {0x1f0,0x3f6,14},
	{0x170,0x376,15}
    };
    
    printk( "ide>device_init\n" );

    nextnode = &ControllerList;
    for( i=0; i<sizeof(controllers)/sizeof(controllers[0]); i++ )
    {
	int ioport = controllers[i][0];
	int ioport2 = controllers[i][1];
	int irq = controllers[i][2];
	char nodename[256];
	AtaController *ctrl;
//	uint32 cpuflags;

	printk( "ide>testing for controller @ 0x%04X\n", ioport );

//	cpuflags = CliOnBootCpu();

	outb( 0x55, ioport + ATA_REG_SECTORCOUNT );
	outb( 0xaa, ioport + ATA_REG_SECTORNUMBER );
	if( inb(ioport+ATA_REG_SECTORCOUNT)!=0x55 || inb(ioport+ATA_REG_SECTORNUMBER)!=0xaa )
	{
	    printk( "ide>no controller @ 0x%04X\n", ioport );
	    goto nocontroller;
	}

	ctrl = kmalloc( sizeof(AtaController), MEMF_KERNEL );
	if( ctrl == NULL )
	    goto nocontroller;
	ctrl->next = NULL;
	ctrl->index = *nextnode ? (*nextnode)->index+1 : 0;
	ctrl->ioport = ioport;
	ctrl->ioport2 = ioport2;
	ctrl->irq = irq;
	ctrl->iolock = create_semaphore( "ide controller ioport lock", 1, 0 );
	*nextnode = ctrl;
	nextnode = &(ctrl->next);

	if( ata_SoftReset(ctrl) != 0 )
	    goto nocontroller;

	for( j=0; j<2; j++ )
	{
	    AtaDrive *drive = kmalloc( sizeof(AtaDrive), MEMF_KERNEL );

	    if( drive == NULL )
		continue;

	    drive->ctrl = ctrl;
	    drive->drive_id = j;

	    if( ata_Identify(drive) != 0 )
	    {
		kfree( drive );
		continue;
	    }

	    sprintf( nodename, "disk/" DEVICENAME "/%d/%d", i, j );
	    status = create_device_node( nDeviceID, nodename, &ide_functions, drive );
	}

    nocontroller:
//	put_cpu_flags( cpuflags );
    }
    return 0;
}

#ifdef __IDE_EXTERNAL
status_t device_uninit( int nDeviceID )
{
    AtaController *node;

    printk( "ide>device_uninit\n" );

    node=ControllerList;
    while( node )
    {
	AtaController *nextnode = node->next;

	delete_semaphore( node->iolock );
	kfree( node );

	node = nextnode;
    }

    return 0;
}
#endif

//-----------------------------------------------------------------------------

static int ide_open( void* pNode, uint32 nFlags, void **ppCookie )
{
    return 0;
}

static int ide_close( void* pNode, void* pCookie )
{
    return 0;
}

//-----------------------------------------------------------------------------

static status_t ide_ioctl( void* pNode, void* pCookie, uint32 nCommand, void* pArgs, size_t nLen )
{
    AtaDrive *drive = (AtaDrive*)pNode;

    switch( nCommand )
    {
      case IOCTL_GET_DEVICE_GEOMETRY:
      {
	  device_geometry *psGeo = pArgs;
      
	  psGeo->sector_count      = drive->size_sectors;
	  psGeo->cylinder_count    = drive->cylinders;
	  psGeo->sectors_per_track = drive->sectors;
	  psGeo->head_count	   = drive->heads;
	  psGeo->bytes_per_sector  = 512;
	  psGeo->read_only 	   = false;
	  psGeo->removable 	   = false;
	  printk( "ata:IOCTL_GET_DEVICE_GEOMETRY:\n" );
	  printk( "\tsector_count:      %d\n", psGeo->sector_count );
	  printk( "\tcylinder_count:    %d\n", psGeo->cylinder_count );
	  printk( "\tsectors_per_track: %d\n", psGeo->sectors_per_track );
	  printk( "\thead_count:        %d\n", psGeo->head_count );
	  printk( "\tbytes_per_sector:  %d\n", psGeo->bytes_per_sector );
	  printk( "\tread_only:         %d\n", psGeo->read_only );
	  printk( "\tremovable:         %d\n", psGeo->removable );
	  return 0;
      }
      default:
	  printk( "ata:>ioctl: Unknown ioctl %d\n", nCommand );
	  return -EINVAL;
    }
}

//-----------------------------------------------------------------------------

static int ide_read( void* pNode, void* pCookie, off_t nPos, void* pBuf, size_t nLen )
{
    AtaDrive *drive = (AtaDrive*)pNode;

    if( nPos&(512-1) || nLen&(512-1) )
    {
	printk( "ide>Read: position/length has bad alignment, pos:0x%LX length:0x%lX\n", nPos, nLen );
	return -EINVAL;
    }

    LOCK( drive->ctrl->iolock );
    
    if( ata_ReadSectors(drive,pBuf,nPos/512,nLen/512) != 0 )
    {
	UNLOCK( drive->ctrl->iolock );
	return -EIO;
    }

    UNLOCK( drive->ctrl->iolock );
    return nLen;
}

static int ide_write( void* pNode, void* pCookie, off_t nPos, const void* pBuf, size_t nLen )
{
    AtaDrive *drive = (AtaDrive*)pNode;

    if( nPos&(512-1) || nLen&(512-1) )
    {
	printk( "ide>Write: position/length has bad alignment, pos:0x%LX length:0x%lX\n", nPos, nLen );
	return -EINVAL;
    }

    LOCK( drive->ctrl->iolock );
    
    if( ata_WriteSectors(drive,pBuf,nPos/512,nLen/512) != 0 )
    {
	UNLOCK( drive->ctrl->iolock );
	return -EIO;
    }

    UNLOCK( drive->ctrl->iolock );
    return nLen;
}

//-----------------------------------------------------------------------------

