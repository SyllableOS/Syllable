
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

#include <atheos/types.h>
#include <atheos/kernel.h>
#include <atheos/semaphore.h>
#include <atheos/atomic.h>
#include <atheos/filesystem.h>

#include <posix/errno.h>
#include <posix/limits.h>
#include <posix/fcntl.h>
#include <posix/ioctls.h>
#include <posix/signal.h>

#include <macros.h>

#include "vfs.h"

typedef struct
{
	Inode_s *pn_psHostInode;
	sem_id pn_hMutex;
	sem_id pn_hReadSema;
	sem_id pn_hWriteSema;
	sem_id pn_hReaderQueue;
	sem_id pn_hWriterQueue;
	atomic_t pn_nWriters;	// Number of O_WRONLY files attached
	atomic_t pn_nReaders;	// Number of O_RDONLY files attached
	int pn_nInPos;
	int pn_nOutPos;
	atomic_t pn_nReadOpenSeq;
	atomic_t pn_nWriteOpenSeq;
	int pn_nBytesInBuffer;
	SelectRequest_s *pn_psFirstReadSelReq;
	SelectRequest_s *pn_psFirstWriteSelReq;
	uint8 pn_anBuffer[PIPE_BUF];
} PipeNode_s;

typedef struct
{
	int pc_nMode;
} PipeCookie_s;

static FSOperations_s g_sOperations;
static sem_id g_hPipeMutex = -1;

Inode_s *create_fifo_inode( Inode_s *psHostInode )
{
	PipeNode_s *psNode;
	Inode_s *psInode;

	LOCK( g_hPipeMutex );

	if ( psHostInode != NULL && psHostInode->i_psPipe != NULL )
	{
		psInode = do_get_inode( FSID_PIPE, psHostInode->i_psPipe->i_nInode, false, false );
		UNLOCK( g_hPipeMutex );
		return ( psInode );
	}

	psNode = kmalloc( sizeof( PipeNode_s ), MEMF_KERNEL | MEMF_CLEAR );

	if ( psNode == NULL )
	{
		printk( "Error: create_fifo_inode() no memory for node\n" );
		UNLOCK( g_hPipeMutex );
		return ( NULL );
	}
	psNode->pn_psHostInode = psHostInode;

	psNode->pn_hMutex = create_semaphore( "fifo_mutex", 1, 0 );
	if ( psNode->pn_hMutex < 0 )
		goto error1;

	psNode->pn_hReadSema = create_semaphore( "fifo_read", 0, 0 );
	if ( psNode->pn_hReadSema < 0 )
		goto error2;

	psNode->pn_hWriteSema = create_semaphore( "fifo_write", 0, 0 );
	if ( psNode->pn_hWriteSema < 0 )
		goto error3;

	psNode->pn_hReaderQueue = create_semaphore( "fifo_ropen", 0, 0 );
	if ( psNode->pn_hReaderQueue < 0 )
		goto error4;

	psNode->pn_hWriterQueue = create_semaphore( "fifo_wopen", 0, 0 );
	if ( psNode->pn_hWriterQueue < 0 )
		goto error5;

	psInode = do_get_inode( FSID_PIPE, ( ino_t )( ( int )psNode ), false, false );
	if ( psInode == NULL )
	{
		printk( "Error: create_fifo_inode() failed to load inode\n" );
		goto error6;
	}
	if ( psHostInode != NULL )
	{
		atomic_inc( &psHostInode->i_nCount );
		psHostInode->i_psPipe = psInode;
	}
	UNLOCK( g_hPipeMutex );
	return ( psInode );
      error6:
	delete_semaphore( psNode->pn_hWriterQueue );
      error5:
	delete_semaphore( psNode->pn_hReaderQueue );
      error4:
	delete_semaphore( psNode->pn_hWriteSema );
      error3:
	delete_semaphore( psNode->pn_hReadSema );
      error2:
	delete_semaphore( psNode->pn_hMutex );
      error1:
	kfree( psNode );
	printk( "Error: create_fifo_inode() failed to create node\n" );
	UNLOCK( g_hPipeMutex );
	return ( NULL );
}


static int pi_read_inode( void *pVolume, ino_t nInodeNum, void **ppNode )
{
	*ppNode = ( void * )( ( int )nInodeNum );
	return ( 0 );
}

static int pi_write_inode( void *pVolData, void *pNode )
{
	PipeNode_s *psNode = pNode;
	Inode_s *psHost;

	LOCK( g_hPipeMutex );
	psHost = psNode->pn_psHostInode;
	if ( psHost != NULL )
	{
		psHost->i_psPipe = NULL;
	}
	UNLOCK( g_hPipeMutex );

	if ( psHost != NULL )
	{
		put_inode( psHost );
	}
	delete_semaphore( psNode->pn_hWriterQueue );
	delete_semaphore( psNode->pn_hReaderQueue );
	delete_semaphore( psNode->pn_hWriteSema );
	delete_semaphore( psNode->pn_hReadSema );
	delete_semaphore( psNode->pn_hMutex );

	kfree( psNode );
	return ( 0 );
}

static int pi_rstat( void *pVolume, void *pNode, struct stat *psStat )
{
	PipeNode_s *psNode = pNode;
	int nError = 0;

	if ( psNode->pn_psHostInode == NULL )
	{
		psStat->st_dev = FSID_PIPE;
		psStat->st_ino = ( int )psNode;
		psStat->st_size = psNode->pn_nBytesInBuffer;
		psStat->st_mode = S_IFIFO | 0777;
		psStat->st_atime = 0;
		psStat->st_mtime = 0;
		psStat->st_ctime = 0;
		psStat->st_uid = 0;
		psStat->st_gid = 0;
	}
	else
	{
		Inode_s *psInode = psNode->pn_psHostInode;

		LOCK_INODE_RO( psInode );
		if ( NULL != psInode->i_psOperations->rstat )
		{
			memset( psStat, 0, sizeof( struct stat ) );
			nError = psInode->i_psOperations->rstat( psInode->i_psVolume->v_pFSData, psInode->i_pFSData, psStat );
		}
		else
		{
			nError = -EACCES;
		}
		UNLOCK_INODE_RO( psInode );
	}
	return ( nError );
}

static void pi_notify_read_select( PipeNode_s * psNode )
{
	SelectRequest_s *psReq;

	for ( psReq = psNode->pn_psFirstReadSelReq; NULL != psReq; psReq = psReq->sr_psNext )
	{
		psReq->sr_bReady = true;
		unlock_semaphore( psReq->sr_hSema );
	}
}

static void pi_notify_write_select( PipeNode_s * psNode )
{
	SelectRequest_s *psReq;

	for ( psReq = psNode->pn_psFirstWriteSelReq; NULL != psReq; psReq = psReq->sr_psNext )
	{
		psReq->sr_bReady = true;
		unlock_semaphore( psReq->sr_hSema );
	}
}

static int pi_open( void *pVolume, void *pNode, int nMode, void **ppCookie )
{
	PipeNode_s *psNode = pNode;
	PipeCookie_s *psCookie;
	int nSeq;
	int nError;

	if ( ( nMode & O_ACCMODE ) == O_RDWR )
	{
		printk( "Error: Attempt to open pipe in O_RDWR mode\n" );
		return ( -EACCES );
	}

	psCookie = kmalloc( sizeof( PipeCookie_s ), MEMF_KERNEL | MEMF_CLEAR );

	if ( psCookie == NULL )
	{
		printk( "Error: pi_open() failed to alloc cookie\n" );
		return ( -ENOMEM );
	}
	psCookie->pc_nMode = nMode;

	LOCK( psNode->pn_hMutex );
	switch ( nMode & O_ACCMODE )
	{
	case O_RDONLY:
		nSeq = atomic_read( &psNode->pn_nWriteOpenSeq );
		atomic_inc( &psNode->pn_nReaders );
		atomic_inc( &psNode->pn_nReadOpenSeq );
		wakeup_sem( psNode->pn_hWriterQueue, true );
		if ( ( nMode & O_NDELAY ) == 0 && psNode->pn_psHostInode != NULL )
		{
			while ( atomic_read( &psNode->pn_nWriters ) == 0 && nSeq == atomic_read( &psNode->pn_nWriteOpenSeq ) )
			{
				nError = unlock_and_suspend( psNode->pn_hReaderQueue, psNode->pn_hMutex );
				if ( nError < 0 )
				{
					atomic_dec( &psNode->pn_nReaders );
					goto error;
				}
				LOCK( psNode->pn_hMutex );
			}
		}
		break;
	case O_WRONLY:
		nSeq = atomic_read( &psNode->pn_nReadOpenSeq );
		atomic_inc( &psNode->pn_nWriters );
		atomic_inc( &psNode->pn_nWriteOpenSeq );
		wakeup_sem( psNode->pn_hReaderQueue, true );
		if ( ( nMode & O_NDELAY ) == 0 && psNode->pn_psHostInode != NULL )
		{
			while ( atomic_read( &psNode->pn_nReaders ) == 0 && nSeq == atomic_read( &psNode->pn_nReadOpenSeq ) )
			{
				nError = unlock_and_suspend( psNode->pn_hWriterQueue, psNode->pn_hMutex );
				if ( nError < 0 )
				{
					atomic_dec( &psNode->pn_nWriters );
					goto error;
				}
				LOCK( psNode->pn_hMutex );
			}
		}
		break;
	default:
		UNLOCK( psNode->pn_hMutex );
		printk( "Error: pi_open() invalid mode %08x\n", nMode & O_ACCMODE );
		return ( -EACCES );
	}
	UNLOCK( psNode->pn_hMutex );
	*ppCookie = psCookie;
	return ( 0 );
      error:
	kfree( psCookie );
	return ( nError );
}

static int pi_close( void *pVolume, void *pNode, void *pCookie )
{
	PipeNode_s *psNode = pNode;
	PipeCookie_s *psCookie = pCookie;

	LOCK( psNode->pn_hMutex );
	switch ( psCookie->pc_nMode & O_ACCMODE )
	{
	case O_RDONLY:
		atomic_dec( &psNode->pn_nReaders );
		if ( atomic_read( &psNode->pn_nReaders ) < 0 )
		{
			printk( "Error: pi_close() node got reader's count of %d\n", atomic_read( &psNode->pn_nReaders ) );
			atomic_set( &psNode->pn_nReaders, 0 );
		}
		// Not sure if this is correct. It should might be cleared
		// every time someone closed it, or it might should only be
		// cleared when both all reader and all writers are gone.
		if ( atomic_read( &psNode->pn_nReaders ) == 0 )
		{
			psNode->pn_nOutPos = psNode->pn_nInPos = psNode->pn_nBytesInBuffer = 0;
		}
		wakeup_sem( psNode->pn_hWriteSema, true );
		pi_notify_write_select( psNode );
		break;
	case O_WRONLY:
		atomic_dec( &psNode->pn_nWriters );
		if ( atomic_read( &psNode->pn_nWriters ) < 0 )
		{
			printk( "Error: pi_close() node got writer's count of %d\n", atomic_read( &psNode->pn_nWriters ) );
			atomic_set( &psNode->pn_nWriters, 0 );
		}
		wakeup_sem( psNode->pn_hReadSema, true );
		pi_notify_read_select( psNode );
		break;
	}
	UNLOCK( psNode->pn_hMutex );
	kfree( psCookie );
	return ( 0 );
}

static int pi_read( void *pVolume, void *pNode, void *pCookie, off_t nPos, void *pBuf, size_t nLen )
{
	PipeNode_s *psNode = pNode;
	PipeCookie_s *psCookie = pCookie;
	char *pBuffer = pBuf;
	int nError;
	int nCurLen;

	LOCK( psNode->pn_hMutex );

	while ( psNode->pn_nBytesInBuffer <= 0 )
	{
		if ( atomic_read( &psNode->pn_nWriters ) == 0 )
		{
			nError = 0;
			goto error;
		}
		if ( psCookie->pc_nMode & O_NONBLOCK )
		{
			nError = -EWOULDBLOCK;
			goto error;
		}
		nError = unlock_and_suspend( psNode->pn_hReadSema, psNode->pn_hMutex );
		if ( nError < 0 )
		{
			return ( nError );
		}
		LOCK( psNode->pn_hMutex );
	}
	if ( nLen > psNode->pn_nBytesInBuffer )
	{
		nLen = psNode->pn_nBytesInBuffer;
	}
	psNode->pn_nBytesInBuffer -= nLen;
	nError = nLen;
	while ( nLen > 0 )
	{
		nCurLen = PIPE_BUF - psNode->pn_nOutPos;
		if ( nCurLen > nLen )
		{
			nCurLen = nLen;
		}
		memcpy( pBuffer, psNode->pn_anBuffer + psNode->pn_nOutPos, nCurLen );
		psNode->pn_nOutPos = ( psNode->pn_nOutPos + nCurLen ) & ( PIPE_BUF - 1 );
		nLen -= nCurLen;
		pBuffer += nCurLen;
	}
	wakeup_sem( psNode->pn_hWriteSema, false );
	pi_notify_write_select( psNode );
      error:
	UNLOCK( psNode->pn_hMutex );
	return ( nError );
}

static int pi_write( void *pVolume, void *pNode, void *pCookie, off_t nPos, const void *pBuf, size_t nLen )
{
	PipeNode_s *psNode = pNode;
	PipeCookie_s *psCookie = pCookie;
	int nError;
	int nCurLen;
	const char *pBuffer = pBuf;
	int nBytesWritten = 0;
	int nBytesToWrite = nLen;

	while ( nBytesToWrite > 0 )
	{
		LOCK( psNode->pn_hMutex );

		while ( psNode->pn_nBytesInBuffer == PIPE_BUF )
		{
			// check for broken pipe before blocking
			if ( atomic_read( &psNode->pn_nReaders ) == 0 )
			{
				nError = -EPIPE;
				switch ( get_signal_mode( SIGPIPE ) )
				{
				case SIG_DEFAULT:
				case SIG_HANDLED:
					sys_kill( sys_get_thread_id( NULL ), SIGPIPE );
				}
				goto error;
			}
			if ( psCookie->pc_nMode & O_NONBLOCK )
			{
				nError = -EWOULDBLOCK;
				goto error;
			}
			nError = unlock_and_suspend( psNode->pn_hWriteSema, psNode->pn_hMutex );
			if ( nError < 0 )
			{
				return ( nError );
			}
			LOCK( psNode->pn_hMutex );
		}
		// check for broken pipe before writing
		if ( atomic_read( &psNode->pn_nReaders ) == 0 )
		{
			nError = -EPIPE;
			switch ( get_signal_mode( SIGPIPE ) )
			{
			case SIG_DEFAULT:
			case SIG_HANDLED:
				sys_kill( sys_get_thread_id( NULL ), SIGPIPE );
			}
			goto error;
		}
		nLen = nBytesToWrite;
		if ( nLen > PIPE_BUF - psNode->pn_nBytesInBuffer )
		{
			nLen = PIPE_BUF - psNode->pn_nBytesInBuffer;
		}
		nBytesToWrite -= nLen;
		nBytesWritten += nLen;
		psNode->pn_nBytesInBuffer += nLen;

		while ( nLen > 0 )
		{
			nCurLen = PIPE_BUF - psNode->pn_nInPos;
			if ( nCurLen > nLen )
			{
				nCurLen = nLen;
			}
			memcpy( psNode->pn_anBuffer + psNode->pn_nInPos, pBuffer, nCurLen );
			psNode->pn_nInPos = ( psNode->pn_nInPos + nCurLen ) & ( PIPE_BUF - 1 );
			nLen -= nCurLen;
			pBuffer += nCurLen;
		}
		wakeup_sem( psNode->pn_hReadSema, false );
		pi_notify_read_select( psNode );
		if ( atomic_read( &psNode->pn_nReaders ) == 0 || ( psCookie->pc_nMode & O_NONBLOCK ) )
		{
			UNLOCK( psNode->pn_hMutex );
			break;
		}
		UNLOCK( psNode->pn_hMutex );
	}
	return ( nBytesWritten );
      error:
	UNLOCK( psNode->pn_hMutex );
	return ( nError );
}

static int pi_ioctl( void *pVolume, void *pNode, void *pCookie, int nCmd, void *pBuf, bool bFromKernel )
{
	PipeNode_s *psNode = pNode;
	PipeCookie_s *psCookie = pCookie;
	int nError = 0;

	switch ( nCmd )
	{
	case FIONBIO:
		{
			int nArg;

			if ( bFromKernel )
			{
				nArg = *( ( int * )pBuf );
			}
			else
			{
				nError = memcpy_from_user( &nArg, pBuf, sizeof( nArg ) );
			}
			if ( nError >= 0 )
			{
				if ( nArg == false )
				{
					psCookie->pc_nMode &= ~O_NONBLOCK;
				}
				else
				{
					psCookie->pc_nMode |= O_NONBLOCK;
				}
			}
			break;
		}
	case FIONREAD:
		if ( bFromKernel )
		{
			*( ( int * )pBuf ) = psNode->pn_nBytesInBuffer;
		}
		else
		{
			nError = memcpy_to_user( pBuf, &psNode->pn_nBytesInBuffer, sizeof( int ) );
		}
		break;
	default:
		nError = -ENOSYS;
		break;
	}
	return ( nError );
}

static int pi_setflags( void *pVolume, void *pNode, void *pCookie, uint32 nFlags )
{
	PipeCookie_s *psCookie = pCookie;

	psCookie->pc_nMode = nFlags;
	return ( 0 );
}

static int pi_add_select_req( void *pVolume, void *pNode, void *pCookie, SelectRequest_s *psReq )
{
	PipeNode_s *psNode = pNode;
	int nError = 0;

	LOCK( psNode->pn_hMutex );

	switch ( psReq->sr_nMode )
	{
	case SELECT_READ:
		psReq->sr_psNext = psNode->pn_psFirstReadSelReq;
		psNode->pn_psFirstReadSelReq = psReq;
		if ( psNode->pn_nBytesInBuffer > 0 || atomic_read( &psNode->pn_nWriters ) == 0 )
		{
			pi_notify_read_select( psNode );
		}
		break;
	case SELECT_WRITE:
		psReq->sr_psNext = psNode->pn_psFirstWriteSelReq;
		psNode->pn_psFirstWriteSelReq = psReq;
		if ( psNode->pn_nBytesInBuffer != PIPE_BUF || atomic_read( &psNode->pn_nReaders ) == 0 )
		{
			pi_notify_write_select( psNode );
		}
		break;

	case SELECT_EXCEPT:
		break;

	default:
		kerndbg( KERN_WARNING, "pi_add_select_req(): Unknown mode %d\n", psReq->sr_nMode );

		nError = -EINVAL;
		break;
	}
	UNLOCK( psNode->pn_hMutex );
	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int pi_rem_select_req( void *pVolume, void *pNode, void *pCookie, SelectRequest_s *psReq )
{
	PipeNode_s *psNode = pNode;
	SelectRequest_s **ppsTmp = NULL;
	int nError = 0;

	LOCK( psNode->pn_hMutex );

	switch ( psReq->sr_nMode )
	{
	case SELECT_READ:
		ppsTmp = &psNode->pn_psFirstReadSelReq;
		break;

	case SELECT_WRITE:
		ppsTmp = &psNode->pn_psFirstWriteSelReq;
		break;

	case SELECT_EXCEPT:
		break;

	default:
		kerndbg( KERN_WARNING, "pi_rem_select_req(): Unknown mode %d\n", psReq->sr_nMode );

		nError = -EINVAL;
		break;
	}

	if ( NULL != ppsTmp )
	{
		for ( ; NULL != *ppsTmp; ppsTmp = &( ( *ppsTmp )->sr_psNext ) )
		{
			if ( *ppsTmp == psReq )
			{
				*ppsTmp = psReq->sr_psNext;
				break;
			}
		}
	}
	UNLOCK( psNode->pn_hMutex );
	return ( nError );
}

static FSOperations_s g_sOperations = {
	NULL,			// op_probe
	NULL,			/* mount        */
	NULL,			/* unmount      */
	pi_read_inode,
	pi_write_inode,
	NULL,			/* lookup       */
	NULL,			/* access       */
	NULL,			/* create,      */
	NULL,			/* mkdir        */
	NULL,			/* mknod        */
	NULL,			/* symlink      */
	NULL,			/* link         */
	NULL,			/* rename       */
	NULL,			/* unlink       */
	NULL,			/* rmdir        */
	NULL,			/* readlink     */
	NULL,			/* ppendir      */
	NULL,			/* closedir     */
	NULL,			/* rewinddir    */
	NULL,			/* readdir      */
	pi_open,		/* open         */
	pi_close,		/* close        */
	NULL,			/* freecookie   */
	pi_read,		// op_read
	pi_write,		// op_write
	NULL,			// op_readv
	NULL,			// op_writev
	pi_ioctl,		/* ioctl        */
	pi_setflags,		/* setflags     */
	pi_rstat,		/* rstat        */
	NULL,			/* wstat        */
	NULL,			/* fsync        */
	NULL,			/* initialize   */
	NULL,			/* sync         */
	NULL,			/* rfsstat      */
	NULL,			/* wfsstat      */
	NULL,			/* isatty       */
	pi_add_select_req,
	pi_rem_select_req
};


int init_fifo( void )
{
	Volume_s *psKVolume;

	psKVolume = kmalloc( sizeof( Volume_s ), MEMF_KERNEL | MEMF_CLEAR );

	if ( psKVolume == NULL )
	{
		panic( "init_fifo() failed to alloc memory for kernel volume struct\n" );
		return ( -ENOMEM );
	}

	g_hPipeMutex = create_semaphore( "pipe_mutex", 1, 0 );
	psKVolume->v_psNext = NULL;
	psKVolume->v_nDevNum = FSID_PIPE;
	psKVolume->v_pFSData = NULL;
	psKVolume->v_psOperations = &g_sOperations;
	psKVolume->v_nRootInode = 0xdeadbeef;

	add_mount_point( psKVolume );
	return ( 0 );
}
