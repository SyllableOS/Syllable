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

#ifndef	__F_ATHEOS_FILESYSTEM_H__
#define	__F_ATHEOS_FILESYSTEM_H__

#include <atheos/types.h>
#include <atheos/fs_attribs.h>

#include <posix/types.h>
#include <posix/stat.h>
#include <posix/dirent.h>
#include <posix/fcntl.h>
#include <posix/uio.h>

#ifdef __cplusplus
extern "C" {
#endif

/* NOTE: One less than max int64 to avoid arithmetic overflow in later expressions */
#define MAX_FILE_OFFSET ((uint64)(1LL<<63) - 2)

typedef unsigned int kdev_t;

/* Flags passed to mount() */
#define MNTF_READONLY	0x01	/* Disable write operations */
#define MNTF_DIRECT		0x02	/* Do not use caching device I/O operations */

/* Flags returned in the fi_flags field of fs_info */
#define FS_IS_READONLY	 0x00000001	/* Set if mounted readonly or resides on a RO meadia */
#define FS_IS_REMOVABLE	 0x00000002	/* Set if mounted on a removable media */
#define FS_IS_PERSISTENT 0x00000004	/* Set if data written to the FS is preserved across reboots */
#define FS_IS_SHARED	 0x00000008	/* Set if the FS is shared across multiple machines (Network FS) */
#define FS_IS_BLOCKBASED 0x00000010	/* Set if the FS use a regular blockdevice
					 * (or loopback from a single file) to store its data.
					 */
#define FS_CAN_MOUNT	 0x00000020	/* Set by probe() if the FS can mount the given device */
#define FS_HAS_MIME	 0x00010000
#define FS_HAS_ATTR	 0x00020000
#define FS_HAS_QUERY	 0x00040000

#define FSINFO_VERSION	1

typedef struct
{
    dev_t  fi_dev;							/* Device ID */
    ino_t  fi_root;							/* Root inode number */
    uint32 fi_flags;						/* Filesystem flags (defined above) */
    int    fi_block_size;					/* Fundamental block size */
    int    fi_io_size;						/* Optimal IO size */
    off_t  fi_total_blocks;					/* Total number of blocks in FS */
    off_t  fi_free_blocks;					/* Number of free blocks in FS */
    off_t  fi_free_user_blocks;				/* Number of blocks available for non-root users */
    off_t  fi_total_inodes;					/* Total number of inodes (-1 if inodes are allocated dynamically) */
    off_t  fi_free_inodes;					/* Number of free inodes (-1 if inodes are allocated dynamically) */
    char   fi_device_path[1024];			/* Device backing the FS (might not be a real
											 * device unless the FS_IS_BLOCKBASED is set) */
    char   fi_mount_args[1024];				/* Arguments given to FS when mounted */
    char   fi_volume_name[256];				/* Volume name */
    char   fi_driver_name[OS_NAME_LENGTH];	/* Name of filesystem driver */
} fs_info;

status_t get_fs_info( int nFile, fs_info* psInfo );
status_t probe_fs( const char* pzDevicePath, fs_info* psInfo );

/* Node monitors */
enum
{
    NWATCH_NAME	 	 = 0x0001,
    NWATCH_STAT  	 = 0x0002,
    NWATCH_ATTR  	 = 0x0004,
    NWATCH_DIR   	 = 0x0008,
    NWATCH_ALL   	 = 0x000f,
    NWATCH_FULL_DST_PATH = 0x0010,
    NWATCH_MOUNT 	 = 0x0100,
    NWATCH_WORLD 	 = 0x0200
};

enum
{
    NWEVENT_CREATED = 1,
    NWEVENT_DELETED,
    NWEVENT_MOVED,
    NWEVENT_STAT_CHANGED,
    NWEVENT_ATTR_WRITTEN,
    NWEVENT_ATTR_DELETED,
    NWEVENT_FS_MOUNTED,
    NWEVENT_FS_UNMOUNTED
};

enum
{
    NWPATH_NONE,
    NWPATH_OLD,
    NWPATH_NEW
};

typedef struct
{
    int		nw_nTotalSize;
    int		nw_nEvent;
    int		nw_nWhichPath;
    int		nw_nMonitor;
    void*	nw_pUserData;
    dev_t	nw_nDevice;
    ino_t	nw_nNode;
    ino_t	nw_nOldDir;
    ino_t	nw_nNewDir;
    int		nw_nNameLen;
    int		nw_nPathLen;
} NodeWatchEvent_s;

#define NWE_NAME( event ) ((char*)((event)+1))
#define NWE_PATH( event ) (NWE_NAME( (event ) ) + (event)->nw_nNameLen)

int 	 watch_node( int nNode, int nPort, uint32 nFlags, void* pDate );
status_t delete_node_monitor( int nMonitor );

/* Misc. FS functions */
status_t get_directory_path( int nFD, char* pzBuffer, int nBufLen );

int get_mount_point_count(void);
int get_mount_point( int nIndex, char* pzBuffer, int nBufLen );

int	get_system_path( char* pzBuffer, int nBufLen );

enum { IMGFILE_BIN_DIR = -2 };
int open_image_file( int nImageID );

int initialize_fs( const char* pzDevPath, const char* pzFsType, const char* pzVolName, void* pArgs, int nArgLen );
int mount( const char* pzDevName, const char* pzDirName, const char* pzFSName, int nFlags, const void* pData );
int unmount( const char* pzPath, bool bForce );

int freadlink( int nFile, char* pzBuffer, size_t nBufSize );

/* Directory relative OPs */
int based_open( int nRootFD, const char *pzPath, int nFlags, ... );
int based_symlink( int nDir, const char* pzSrc, const char* pzDst );
int based_mkdir( int nDir, const char* pzPath, mode_t nMode );
int based_rmdir( int nDir, const char* pzPath );
int based_unlink( int nDir, const char* pzPath );

/* Extended OPs */
ssize_t	read_pos( int nFile, off_t nPos, void* pBuffer, size_t nLength );
ssize_t	write_pos( int nFile, off_t nPos, const void* pBuffer, size_t nLength );

ssize_t	readv_pos( int nFile, off_t nPos, const struct iovec* psVector, int nCount );
ssize_t	writev_pos( int nFile, off_t nPos, const struct iovec* psVector, int nCount );

#ifdef __cplusplus
}
#endif

#endif	/*	__F_ATHEOS_FILESYSTEM_H__	*/
