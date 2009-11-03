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

#include <posix/ioctl.h>
#include <posix/limits.h>
#include <syllable/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Standard IOCTLs */
#define MK_IOCT(a,b) (a+b)
enum
{
	IOCTL_GET_DEVICE_GEOMETRY = 1,
	IOCTL_REREAD_PTABLE,
	IOCTL_GET_DEVICE_PATH = MK_IOCTLW( IOCTL_MOD_KERNEL, 0x01, PATH_MAX ),
	IOCTL_GET_DEVICE_HANDLE = MK_IOCTLR( IOCTL_MOD_KERNEL, 0x02, 4 ),
	
	IOCTL_GET_APPSERVER_DRIVER = MK_IOCTLR( IOCTL_MOD_KERNEL, 0x100, PATH_MAX ),
	IOCTL_GET_USERSPACE_DRIVER = MK_IOCTLR( IOCTL_MOD_KERNEL, 0x100, PATH_MAX ),
	
	IOCTL_USER = 100000000
};

/* Block device information */
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

#ifdef __cplusplus
}
#endif

#endif /* __F_ATHEOS_DEVICE_H__ */
