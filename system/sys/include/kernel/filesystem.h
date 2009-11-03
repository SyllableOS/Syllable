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

#ifndef	__F_KERNEL_FILESYSTEM_H__
#define	__F_KERNEL_FILESYSTEM_H__

#include <kernel/types.h>
#include <kernel/fs_attribs.h>
#include <kernel/atomic.h>
#include <syllable/filesystem.h>

#include <posix/types.h>
#include <posix/stat.h>
#include <posix/dirent.h>
#include <posix/fcntl.h>
#include <posix/uio.h>

typedef struct _SelectRequest SelectRequest_s;

struct _SelectRequest
{
    SelectRequest_s* sr_psNext;	
    sem_id	     sr_hSema;
    int		     sr_nFileDesc;
    int		     sr_nMode;
    void*	     sr_pFile;
    bool	     sr_bReady;
    bool	     sr_bSendt;
};

enum
{
    SELECT_READ = 1,
    SELECT_WRITE,
    SELECT_EXCEPT
};

#define WSTAT_MODE   0x0001
#define WSTAT_UID    0x0002
#define WSTAT_GID    0x0004
#define WSTAT_SIZE   0x0008
#define WSTAT_ATIME  0x0010
#define WSTAT_MTIME  0x0020
#define WSTAT_CTIME  0x0040

#define WFSSTAT_NAME 0x0001

/* Filesystem hooks */
typedef int op_probe( const char* pzDevPath, fs_info* psInfo );
typedef int op_mount( kdev_t nDevNum, const char* pzDevPath,
		      uint32 nFlags, const void* pArgs, int nArgLen, void** ppVolData, ino_t* pnRootIno );

typedef int op_unmount( void* pVolume );

typedef	int op_read_inode( void* pVolume, ino_t nInodeNum, void** ppNode );
typedef	int op_write_inode( void* pVolume, void* pNode );

typedef	int op_locate_inode( void* pVolume, void* pParentNode, const char* pzName, int nNameLen, ino_t* pnInodeNum );


typedef int op_access( void* pVolume, void *pNode, int nMode );

typedef int op_create( void* pVolume, void* pParentNode, const char* pzName, int nNameLen,
		       int nMode, int nPerms, ino_t* pnInodeNum, void **cookie );

typedef int op_mkdir( void* pVolume, void* pParentNode, const char* pzName, int nNameLen, int nPerms );
typedef int op_mknod( void* pVolume, void* pParentNode, const char* pzName, int nNameLen, int nMode, dev_t nDev );

typedef int op_symlink( void* pVolume, void* pParentNode, const char* pzName, int nNameLen, const char* pzNewPath );

typedef int op_link( void* pVolume, void* pParentNode, const char* pzName, int nNameLen, void* pNode );

typedef int op_rename( void* pVolume, void* pOldDir, const char* pzOldName, int nOldNameLen,
			   void* pNewDir, const char* pzNewName, int nNewNameLen, bool bMustBeDir );

typedef int op_unlink( void* pVolume, void* pParentNode, const char* pzName, int nNameLen );
typedef int op_rmdir( void* pVolume, void* pParentNode, const char* pzName, int nNameLen );

typedef int op_readlink( void* pVolume, void* pNode, char* pzBuf, size_t nBufSize );

typedef int op_opendir( void* pVolume, void* pNode, void** ppCookie );
typedef int op_closedir( void* pVolume, void* pNode, void* ppCookie );
typedef int op_rewinddir( void* pVolume, void* pNode, void* pCookie );

typedef int op_readdir( void* pVolume, void* pNode, void* pCookie, int nPos, struct kernel_dirent* psBuf, size_t nBufSize );

typedef int op_open( void* pVolume, void* pNode, int nMode, void** ppCookie );
typedef int op_close( void* pVolume, void* pNode, void* pCookie );
typedef int op_free_cookie( void* pVolume, void* pNode, void* pCookie );

typedef int op_read( void* pVolume, void* pNode, void* pCookie, off_t nPos, void* pBuf, size_t nLen );
typedef int op_readv( void* pVolume, void* pNode, void* pCookie, off_t nPos, const struct iovec* psVector, size_t nCount );

typedef int op_write( void* pVolume, void* pNode, void* pCookie, off_t nPos, const void* pBuf, size_t nLen );
typedef int op_writev( void* pVolume, void* pNode, void* pCookie, off_t nPos, const struct iovec* psVector, size_t nCount );

typedef int op_ioctl( void* pVolume, void* pNode, void* pCookie, int nCmd, void* pBuf, bool bFromKernel );
typedef	int op_setflags( void* pVolume, void* pNode, void* pCookie, uint32 nFlags );

typedef int op_rstat( void* pVolume, void* pNode, struct stat* psStat );
typedef int op_wstat( void* pVolume, void* pNode, const struct stat* psStat, uint32 nMask );
typedef int op_fsync( void* pVolume, void* pNode );

typedef int op_initialize( const char* pzDevPath, const char* pzVolName, void* pArgs, size_t nArgLen );
typedef int op_sync( void* pVolume );
typedef int op_rfsstat( void* pVolume, fs_info* );
typedef int op_wfsstat( void* pVolume, const fs_info*, uint32 mask );

typedef int op_isatty( void* pVolume, void* pNode );
typedef int op_add_select_req( void* pVolume, void* pNode, void *pCookie,
                               SelectRequest_s* psRequest );
typedef int op_rem_select_req( void* pVolume, void* pNode, void *pCookie,
                               SelectRequest_s* psRequest );


typedef int op_open_attrdir( void* pVolume, void* pNode, void** pCookie);
typedef int op_close_attrdir( void* pVolume, void* pNode, void* pCookie);
typedef int op_rewind_attrdir( void* pVolume, void* pNode, void* pCookie);
typedef int op_read_attrdir( void* pVolume, void* pNode, void* pCookie, struct kernel_dirent* psBuffer, size_t nBufSize );
typedef int op_remove_attr( void* pVolume, void* pNode, const char* pzName, int nNameLen );
typedef	int op_rename_attr( void* pVolume, void* pNode,
			    const char* pzOldName, int nOldNameLen, const char* pzNewName, int nNewNameLen );
typedef int op_stat_attr( void* pVolume, void* pNode, const char* pzName, int nNameLen, struct attr_info* psBuffer );

typedef int op_write_attr( void* pVolume, void* pNode, const char* pzName, int nNameLen, int nFlags, int nType,
			   const void* pBuffer, off_t nPos, size_t nLen );
typedef int op_read_attr( void* pVolume, void* pNode, const char* pzName, int nNameLen, int nType,
			  void* pBuffer, off_t nPos, size_t nLen );

typedef int op_open_indexdir( void* pVolume, void** ppCookie );
typedef int op_close_indexdir( void* pVolume, void* pCookie );
typedef int op_rewind_indexdir( void* pVolume, void* pCookie );
typedef int op_read_indexdir( void* pVolume, void* pCookie, struct kernel_dirent* psBuffer, size_t nBufferSize );
typedef int op_create_index( void* pVolume, const char* pzName, int nNameLen, int nType, int nFlags );
typedef int op_remove_index( void* pVolume, const char* pzName, int nNameLen );
typedef	int op_rename_index( void* pVolume, const char* pzOldName, int nOldNameLen, const char* pzNewName, int nNewNameLen );
typedef int op_stat_index( void* pVolume, const char* pzName, int nNameLen, struct index_info* psBuffer );
typedef int op_get_file_blocks( void* pVolume, void* pNode, off_t nPos, int nBlockCount,
				off_t* pnStart, int* pnActualCount );

typedef int op_truncate( void* pVolume, void* pNode, off_t nLen );

typedef	struct
{
    op_probe*		probe;
    op_mount*		mount;
    op_unmount*		unmount;
    op_read_inode*	read_inode;
    op_write_inode*	write_inode;
    op_locate_inode*	locate_inode;
    op_access*		access;
    op_create*		create;
    op_mkdir*		mkdir;
    op_mknod*		mknod;
    op_symlink*		symlink;
    op_link*		link;
    op_rename*		rename;
    op_unlink*		unlink;
    op_rmdir*		rmdir;
    op_readlink*	readlink;
    op_opendir*		___opendir;
    op_closedir*	___closedir;
    op_rewinddir*	rewinddir;
    op_readdir*		readdir;
    op_open*		open;
    op_close*		close;
    op_free_cookie*	__free_cookie;
    op_read*		read;
    op_write*		write;
    op_readv*		readv;
    op_writev*		writev;
    op_ioctl*		ioctl;
    op_setflags*	setflags;
    op_rstat*		rstat;
    op_wstat*		wstat;
    op_fsync*		fsync;
    op_initialize*	initialize;
    op_sync*		sync;
    op_rfsstat*		rfsstat;
    op_wfsstat*		wfsstat;
    op_isatty*		isatty;
    op_add_select_req*	add_select_req;
    op_rem_select_req*	rem_select_req;

    op_open_attrdir*	open_attrdir;
    op_close_attrdir*	close_attrdir;
    op_rewind_attrdir*	rewind_attrdir;
    op_read_attrdir*	read_attrdir;
    op_remove_attr*	remove_attr;
    op_rename_attr*	rename_attr;
    op_stat_attr*	stat_attr;
    op_write_attr*	write_attr;
    op_read_attr*	read_attr;
    op_open_indexdir*	open_indexdir;
    op_close_indexdir*	close_indexdir;
    op_rewind_indexdir*	rewind_indexdir;
    op_read_indexdir*	read_indexdir;
    op_create_index*	create_index;
    op_remove_index*	remove_index;
    op_rename_index*	rename_index;
    op_stat_index*	stat_index;
    op_get_file_blocks*	get_file_blocks;
    op_truncate*	truncate;		// API version 2
} FSOperations_s;

/* Current filesystem driver API version */
#define FSDRIVER_API_VERSION	2

/* Prototype for the fs-driver init function.
 * The init function should be named fs_init() and should
 * return FSDRIVER_API_VERSION if successfully initialized
 * or a negative error code otherwhice
 */
int fs_init( const char* pzName, FSOperations_s** ppsOps );
typedef int init_fs_func( const char* pzName, FSOperations_s** ppsOps );

/* Syscalls */
int sys_initialize_fs( const char* pzDevPath, const char* pzFsType, const char* pzVolName, void* pArgs, int nArgLen );
int sys_mount( const char* pzDevName, const char* pzDirName, const char* pzFSName, int nFlags, const void* pData );
int sys_unmount( const char* pzPath, bool bForce );
int	sys_get_system_path( char* pzBuffer, int nBufLen );

/* Kernel file functions */
int open( const char *pzName, int nFlags, ... );
int close( int nFileDesc );
int pipe( int* pnFiles );
ssize_t read( int nFileDesc, void* pBuffer, size_t	nLength );
int dup( int nSrc );
int dup2( int nSrc, int nDst );
int symlink( const char* pzSrc, const char* pzDst );
int chdir( const char* pzPath );
int fchdir( int nFile );

typedef int iterate_dir_callback( const char* pzPath, struct stat* psStat, void* pArg );
int iterate_directory( const char* pzDirectory, bool bIncludeDirs, iterate_dir_callback* pfCallback, void* pArg );
int normalize_path( const char* pzPath, char** ppzResult );

/* XXXKV: Stuff that probably shouldn't be here */
int	set_inode_deleted_flag( int nDev, ino_t nInode, bool bIsDeleted );
int	get_inode_deleted_flag( int nDev, ino_t nInode );
int	flush_inode( int nDev, ino_t nInode );

status_t notify_node_monitors( int nEvent, dev_t nDevice, ino_t nOldDir, ino_t nNewDir, ino_t nNode, const char* pzName, int nNameLen );

int	get_vnode( int nDev, ino_t nInode, void** pNode );
int	put_vnode( int nDev, ino_t nInode );

#endif	/*	__F_KERNEL_FILESYSTEM_H__	*/
