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
 */

#ifndef __F_ATHEOS_DEVICE_H__
#define __F_ATHEOS_DEVICE_H__

#include <atheos/types.h>
#include <posix/ioctl.h>
#include <posix/limits.h>

#ifdef __cplusplus
extern "C" {
#if 0  
} /*make emacs indention work */
#endif
#endif


status_t device_init( int nDeviceID );
status_t device_uninit( int nDeviceID );

#define MK_IOCT(a,b) (a+b)
enum
{
    IOCTL_GET_DEVICE_GEOMETRY = 1,
    IOCTL_REREAD_PTABLE,
    IOCTL_GET_DEVICE_PATH = MK_IOCTLW( IOCTL_MOD_KERNEL, 0x01, PATH_MAX ),
    IOCTL_USER = 100000000
};


typedef struct device_geometry
{
  uint64 sector_count;
  uint64 cylinder_count;
  uint32 sectors_per_track;
  uint32 head_count;
  uint32 bytes_per_sector;
  bool	read_only;
  bool	removable;
} device_geometry;



typedef size_t disk_read_op( void* pCookie, off_t nOffset, void* pBuffer, size_t nSize );

typedef struct
{
    off_t	p_nStart;	/* Offset in bytes */
    off_t	p_nSize;	/* Size in bytes   */
    int		p_nType;	/* Type as found in the partition table	*/
    int		p_nStatus;	/* Status as found in partition table (bit 7=active) */
} Partition_s;

int decode_disk_partitions( device_geometry* psDiskGeom, Partition_s* pasPartitions, int nMaxPartitions, void* pCookie, disk_read_op* pfReadCallback );


#ifdef __KERNEL__

#include <atheos/filesystem.h>

typedef status_t dop_open( void* pNode, uint32 nFlags, void **pCookie );
typedef status_t dop_close( void* pNode, void* pCookie );
typedef status_t dop_ioctl( void* pNode, void* pCookie, uint32 nCommand, void* pArgs, bool bFromKernel );
typedef int	 dop_read( void* pNode, void* pCookie, off_t nPosition, void* pBuffer, size_t nSize );
typedef int	 dop_write( void* pNode, void* pCookie, off_t nPosition, const void* pBuffer, size_t nSize );
typedef int	 dop_readv( void* pNode, void* pCookie, off_t nPosition, const struct iovec* psVector, size_t nCount );
typedef int	 dop_writev( void* pNode, void* pCookie, off_t nPosition, const struct iovec* psVector, size_t nCount );
typedef int	 dop_add_select_req( void* pNode, void* pCookie, SelectRequest_s* psRequest );
typedef int	 dop_rem_select_req( void* pNode, void* pCookie, SelectRequest_s* psRequest );


/** \struct DeviceOperations_s
 * \ingroup DriverAPI
 * \brief Functions exported from device drivers
 * \par Description:
 *	A device driver export most of it functionality through this
 *	set of functions. The device driver must fill out an instance
 *	of this structure and pass a pointer to it to the
 *	create_device_node() function to export it's functionality. If
 *	the driver have more than one node inside /dev/ it can eighter
 *	use a different set of functions for each node, or it can use
 *	the same functions and differ between the nodes through the
 *	associated cookie.
 * \sa create_device_node()
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

typedef struct
{
    dop_open*		open;
    dop_close*		close;
    dop_ioctl*		ioctl;
    dop_read*		read;
    dop_write*		write;
    dop_readv*		readv;
    dop_writev*		writev;
    dop_add_select_req*	add_select_req;
    dop_rem_select_req*	rem_select_req;
} DeviceOperations_s;

#define BUILTIN_DEVICE_ID 1	// Device ID for devices built in to the kernel.

void init_boot_device( const char* pzPath );

int create_device_node( int nDeviceID, const char* pzPath, const DeviceOperations_s* psOps, void* pCookie );
int delete_device_node( int nHandle );
int rename_device_node( int nHandle, const char* pzNewPath );

#endif /* __KERNEL__ */

#ifdef __cplusplus
}
#endif

#endif /* __F_ATHEOS_DEVICE_H__ */
