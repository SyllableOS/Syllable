
/*
 *  The AtheOS kernel
 *  Copyright (C) 1999 - 2000 Kurt Skauen
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

#ifndef	__F_VFS_H__
#define	__F_VFS_H__

#include <atheos/types.h>
#include <atheos/atomic.h>
#include <atheos/semaphore.h>
#include <atheos/filesystem.h>

#include <posix/types.h>
#include <posix/stat.h>
#include <posix/dirent.h>
#include <posix/fcntl.h>
#include <posix/uio.h>

#include "../inc/typedefs.h"

#ifdef __cplusplus
extern "C"
{
#if 0
}				/*make emacs indention work */
#endif
#endif


typedef struct _Inode Inode_s;
typedef struct _File File_s;
typedef struct _FileLockRec FileLockRec_s;
typedef struct _NodeMonitor NodeMonitor_s;


extern sem_id g_hInodeHashSem;

enum
{
	FSID_ROOT = 1,
	FSID_DEV = 2,
	FSID_PIPE = 3,
	FSID_PTY = 4,
	FSID_SOCKET = 5,
	FSID_LAST = 6,
};

/* Some "well known" i-node numbers in the /dev/ filesystem */
enum
{
	DEVFS_ROOT = 1,
	DEVFS_DEVNULL,
	DEVFS_DEVZERO,
	DEVFS_DEVTTY,
};


typedef struct _FileSysDesc FileSysDesc_s;

struct _FileSysDesc
{
	FileSysDesc_s *fs_psNext;
	FSOperations_s *fs_psOperations;
	atomic_t fs_nRefCount;
	int fs_nImage;		/* Handle to the elf-image if loaded from disk. */
	int fs_nAPIVersion;	/* Version of filesystem driver API.  */
	char fs_zName[32];
};

typedef struct _Volume Volume_s;

struct _Volume
{
	Volume_s *v_psNext;
	Inode_s *v_psMountPoint;	/* The inode this volume is mounted on */
	ino_t v_nRootInode;	/* Root inode of the file system */
	kdev_t v_nDevNum;
	sem_id v_hMutex;
	void *v_pFSData;
	FSOperations_s *v_psOperations;
	FileSysDesc_s *v_psFSDesc;
	char v_zDevicePath[1024];
	atomic_t v_nOpenCount;
	bool v_bUnmounted;
};

typedef struct
{
	sem_id ic_hLock;
	Inode_s *ic_psRootDir;
	Inode_s *ic_psCurDir;
	Inode_s *ic_psBinDir;
	Inode_s *ic_psCtrlTTY;
	FileLockRec_s *ic_psFirstLock;
	int ic_nCount;
	uint32 *ic_panAlloc;
	uint32 *ic_panCloseOnExec;
	File_s **ic_apsFiles;
	NodeMonitor_s *ic_psFirstMonitor;
} IoContext_s;

struct _FileLockRec
{
	FileLockRec_s *fl_psNextWaiter;
	FileLockRec_s *fl_psPrevWaiter;
	sem_id fl_hLockQueue;
	thread_id fl_hOwner;
	int fl_nMode;
	off_t fl_nStart;
	off_t fl_nEnd;
	FileLockRec_s *fl_psNextInInode;
	FileLockRec_s *fl_psNextGlobal;
	FileLockRec_s *fl_psPrevGlobal;
	FileLockRec_s *fl_psNextInThread;
	FileLockRec_s *fl_psPrevInThread;
	Inode_s *fl_psInode;
};

struct _Inode
{
	ino_t i_nInode;		/* Inode number invented by the file-system */
	atomic_t i_nCount;		/* Reference counter */
	Volume_s *i_psVolume;	/* The volume we loaded this inode from */
	FSOperations_s *i_psOperations;	/* List of file-system operations */
	atomic_t i_nThreadsInFS;	/* Incremented before an decremented after a call
					   into the FS through i_psOperations */

	Inode_s *i_psNext;	/* Next node in eigher the free */
	Inode_s *i_psPrev;
	Inode_s *i_psHashNext;
	Inode_s *i_psHashPrev;
	Inode_s *i_psMount;
	Inode_s *i_psPipe;	/* Pointer to the pipe inode of fifo files located in a host FS */
	MemArea_s *i_psAreaList;
	File_s *i_psFirstFile;
	File_s *i_psLastFile;
	FileLockRec_s *i_psFirstLock;
	NodeMonitor_s *i_psFirstMonitor;
	void *i_pFSData;
	int i_nHashVal;
	uint16 i_nFlags;
	bool i_bBusy;		/* true during read_inode() and write_inode() */
	bool i_bDeleted;
};


enum
{
	FDT_FILE = 1,
	FDT_DIR,
	FDT_SYMLINK,
	FDT_ATTR_DIR,
	FDT_INDEX_DIR,
	FDT_SOCKET,
	FDT_FIFO
};

struct _File
{
	Inode_s *f_psInode;
	atomic_t f_nRefCount;
	int f_nType;
	int64 f_nPos;
	int f_nMode;
	int f_nFlags;
	void *f_pFSCookie;
	File_s *f_psNext;
	File_s *f_psPrev;
};

struct _NodeMonitor
{
	NodeMonitor_s *nm_psNextInInode;
	NodeMonitor_s *nm_psNextInCtx;
	int nm_nID;
	void *nm_pUserData;
	IoContext_s *nm_psOwner;
	Inode_s *nm_psInode;
	uint32 nm_nWatchFlags;
	port_id nm_hPortID;
};

static const size_t VOLUME_MUTEX_COUNT = 1000000;


static inline status_t LOCK_INODE_RO( Inode_s *psInode )
{
	return ( lock_semaphore_ex( psInode->i_psVolume->v_hMutex, 1, SEM_NOSIG, INFINITE_TIMEOUT ) );
}

static inline status_t UNLOCK_INODE_RO( Inode_s *psInode )
{
	return ( unlock_semaphore_ex( psInode->i_psVolume->v_hMutex, 1 ) );
}

static inline status_t LOCK_INODE_RW( Inode_s *psInode )
{
	return ( lock_semaphore_ex( psInode->i_psVolume->v_hMutex, VOLUME_MUTEX_COUNT, 0, INFINITE_TIMEOUT ) );
}

static inline status_t UNLOCK_INODE_RW( Inode_s *psInode )
{
	return ( unlock_semaphore_ex( psInode->i_psVolume->v_hMutex, VOLUME_MUTEX_COUNT ) );
}


FileSysDesc_s *register_file_system( const char *pzName, FSOperations_s * psOps, int nAPIVersion );

/* Only called when mounting the virtual root file system and register the FIFO filesystem */
void add_mount_point( Volume_s *psVol );

void shutdown_vfs( void );

void add_file_to_inode( Inode_s *psInode, File_s *psFile );
void remove_file_from_inode( Inode_s *psInode, File_s *psFile );

File_s *alloc_fd( void );
void free_fd( File_s *psFile );

int new_fd( bool bKernel, int nNewFile, int nBase, File_s *psFile, bool bCloseOnExec );
File_s *get_fd( bool bKernel, int nFile );
int put_fd( File_s *psFile );

int sys_open( const char *pzName, int nFlags, ... );
int sys_close( int nFileDesc );
int sys_pipe( int *pnFiles );
int sys_dup2( int nSrc, int nDst );

int extended_open( bool bKernel, Inode_s *psRoot, const char *pzPath, Inode_s **ppsParent, int nFlags, int nPerms );

ssize_t read_pos_p( File_s *psFile, off_t nPos, void *pBuffer, size_t nLength );
ssize_t read_p( File_s *psFile, void *pBuffer, size_t nLength );
ssize_t readv_pos_p( File_s *psFile, off_t nPos, const struct iovec *psVector, int nCount );

ssize_t write_pos_p( File_s *psFile, off_t nPos, const void *pBuffer, size_t nLength );
ssize_t write_p( File_s *psFile, const void *pBuffer, size_t nLength );
ssize_t writev_pos_p( File_s *psFile, off_t nPos, const struct iovec *psVector, int nCount );

int get_file_blocks_p( File_s *psFile, off_t nPos, int nBlockCount, off_t *pnStart, int *pnActualCount );

Inode_s *do_get_inode( kdev_t nDevNum, ino_t nInoNum, bool bCrossMount, bool bOkToFlush );
Inode_s *get_inode( kdev_t nDevNum, ino_t nInoNum, bool bCrossMount );
void put_inode( Inode_s *psInode );


int get_named_inode( Inode_s *psRoot, const char *pzName, Inode_s **ppsResInode, bool bFollowMount, bool bFollowSymlink );

status_t get_dirname( Inode_s *psInode, char *pzPath, size_t nBufSize );

int lock_inode_record( Inode_s *psInode, off_t nStart, off_t nEnd, int nMode, bool bWait );
int unlock_inode_record( Inode_s *psInode, off_t nStart, off_t nEnd );
int get_inode_lock_record( Inode_s *psInode, off_t nStart, off_t nEnd, int nMode, struct flock *psFLock );
void flock_thread_died( Thread_s *psThread );

void init_vfs_module( void );
void init_vfs( void );
bool mount_root_fs( void );

IoContext_s *fs_alloc_ioctx( int nCount );
IoContext_s *fs_clone_io_context( IoContext_s * psSrc );
void fs_free_ioctx( IoContext_s * psCtx );
IoContext_s *get_current_iocxt( bool bKernel );

status_t nwatch_init( void );
void nwatch_exit( IoContext_s * psCtx );

Inode_s *create_fifo_inode( Inode_s *psHostInode );
int open_inode( bool bKernel, Inode_s *psInode, int nType, int nMode );

void disassociate_ctty( bool bOnExit );
void clear_ctty( void );
void clone_ctty( IoContext_s * psDst, IoContext_s * psSrc );
Inode_s *get_ctty( void );

#ifdef __cplusplus
}
#endif

#endif /* __F_VFS_H__ */
