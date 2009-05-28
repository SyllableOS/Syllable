/*
 *  The Syllable kernel
 *  Copyright (C) 2009 Kristian Van Der Vliet
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

#include <posix/errno.h>
#include <atheos/threads.h>

#include "vfs.h"
#include "inc/aio.h"
#include "inc/scheduler.h"

/**
 * Process all queued async I/O requests.
 * \internal
 * \param pData - pointer to a AIOContext_s structure.
 * \author Kristian Van Der Vliet (vanders@liqwyd.org)
 */
static int do_aio_worker( void *pData )
{
	AIOContext_s *psCtx = (AIOContext_s*)pData;

	while( psCtx->ai_nRun == AIO_RUN || psCtx->ai_psHead != NULL )
	{
		struct kaiocb *psOp;
		struct aiocb *psAiocb;
		struct sigevent *psEv;
		File_s *psFile;
		ssize_t nBytes;

		lock_semaphore( psCtx->ai_hWait, 1, INFINITE_TIMEOUT );
		if( psCtx->ai_psHead == NULL )
		{
			kerndbg( KERN_DEBUG, "%s: Stopping\n", __FUNCTION__ );
			break;
		}

		kerndbg( KERN_DEBUG_LOW, "%s: New AIOCB in buffer\n", __FUNCTION__ );

		/* Work has arrived */
		LOCK( psCtx->ai_hLock );

		psOp= psCtx->ai_psHead;
		psAiocb = &psOp->sRequest;

		if( psOp->bKernel == false )
		{
			if( verify_mem_area( (void*)psAiocb->aio_buf, psAiocb->aio_nbytes, true ) < 0 )
			{
				printk( "Invalid AIO buffer at %p\n", (void*)psAiocb );
				goto error;
			}

			psFile = get_fd( false, psAiocb->aio_fildes );
		}
		else
			psFile = psOp->psFile;

		if( NULL == psFile )
		{
			printk( "Invalid AIO file descriptor %d\n", psAiocb->aio_fildes );
			goto error;
		}

		kerndbg( KERN_DEBUG_LOW, "%s: OP code %d\n", __FUNCTION__, psAiocb->aio_lio_opcode );

		/* The AIO request appears to be valid (or at least sane) */
		switch( psAiocb->aio_lio_opcode )
		{
			case AIO_READ:
			{
				kerndbg( KERN_DEBUG, "AIO_READ( %d, %Ld, %p, %Ld)\n", psAiocb->aio_fildes, psAiocb->aio_offset, psAiocb->aio_buf, (uint64)psAiocb->aio_nbytes );
				nBytes = read_pos_p( psFile, psAiocb->aio_offset, (void *)psAiocb->aio_buf, psAiocb->aio_nbytes );
				break;
			}

			case AIO_WRITE:
			{
				kerndbg( KERN_DEBUG, "AIO_WRITE( %d, %Ld, %p, %Ld)\n", psAiocb->aio_fildes, psAiocb->aio_offset, psAiocb->aio_buf, (uint64)psAiocb->aio_nbytes );
				nBytes = write_pos_p( psFile, psAiocb->aio_offset, (const void *)psAiocb->aio_buf, (off_t)psAiocb->aio_nbytes );
				break;
			}

			default:
			{
				printk( "Unknown AIO request %d\n", psAiocb->aio_lio_opcode );
				goto error1;
			}
		}

		/* Store the return value */
		if( psOp->bKernel == false )
		{
			if( nBytes < 0 )
			{
				psAiocb->__error_code = nBytes;
				psAiocb->__return_value = -1;
			}
			else
			{
				psAiocb->__error_code = 0;
				psAiocb->__return_value = nBytes;
			}
			memcpy_to_user( psOp->psOrig, psAiocb, sizeof( struct aiocb ) );
		}

		/* Generate the notification */
		psEv = &psAiocb->aio_sigevent;
		switch( psEv->sigev_notify )
		{
			case SIGEV_SIGNAL:
			{
				sys_kill( psOp->hThread, psEv->sigev_signo );
				break;
			}

		    case SIGEV_THREAD:
		    {
				/* XXXKV: Implement me */
				printk( "AIO SIGEV_THREAD not implemented!\n" );
				break;
			}

			case SIGEV_NONE:
			default:
				break;
		}

		/* Update list */
		psCtx->ai_psHead = psOp->psNext;
		kfree( psOp );

		error1:
		put_fd( psFile );

		kerndbg( KERN_DEBUG_LOW, "%s: AIO cb complete\n", __FUNCTION__ );

		error:
		UNLOCK( psCtx->ai_hLock );
	}
	psCtx->ai_nRun = AIO_DONE;

	return 0;
}

/**
 * Signal the async I/O worker thread to stop and wait for it to complete all requests.
 * \internal
 * \param psProc - pointer to a Process_s structure.
 * \author Kristian Van Der Vliet (vanders@liqwyd.org)
 */
status_t aio_stop( Process_s *psProc )
{
	AIOContext_s *psCtx = psProc->pr_psAioContext;

	psCtx->ai_nRun = AIO_STOP;

	/* Ensure all outstanding requests get flushed to disc as soon as possible */
	set_thread_priority( psCtx->ai_hWorker, REALTIME_PRIORITY );
	unlock_semaphore( psCtx->ai_hWait );
	do
	{
		kerndbg( KERN_DEBUG, "%s: worker not done, will schedule...\n", __FUNCTION__ );
		Schedule();
	}
	while( psCtx->ai_nRun != AIO_DONE );

	/* Clean up */
	delete_semaphore( psProc->pr_psAioContext->ai_hLock );
	delete_semaphore( psProc->pr_psAioContext->ai_hWait );
	kfree( psProc->pr_psAioContext );
	psProc->pr_psAioContext = NULL;

	return EOK;
}

/**
 * Start the async I/O worker.
 * \ingroup Syscalls
 * \author Kristian Van Der Vliet (vanders@liqwyd.org)
 */
int sys_aio_worker( void )
{
	Process_s *psProc = CURRENT_PROC;
	return do_aio_worker( psProc->pr_psAioContext );
}

/**
 * Create a new AIOContext for a process.
 * \internal
 * \par Note:
 * Must be called with the process semaphore locked
 * \param psProc - pointer to a Process_s structure.
 * \author Kristian Van Der Vliet (vanders@liqwyd.org)
 */
status_t aio_create_context( Process_s *psProc )
{
	AIOContext_s *psCtx;
	status_t nError;
	void *pCallbackAddr = NULL;
	int nLibrary = 0;

	kassertw( is_semaphore_locked( psProc->pr_hSem ) );

	psCtx = kmalloc( sizeof( AIOContext_s ), MEMF_KERNEL|MEMF_LOCKED|MEMF_OKTOFAIL|MEMF_CLEAR );
	if( NULL == psCtx )
	{
		nError = ENOMEM;
		goto error;
	}

	psCtx->ai_psProc = psProc;
	psCtx->ai_psHead = psCtx->ai_psTail = NULL;

	/* The worker thread needs a semaphore to indicate available work, and a mutex to control access to the buffer */
	psCtx->ai_hLock = create_semaphore( "aiocb_buf_lock", 1, SEMSTYLE_COUNTING );
	if( psCtx->ai_hLock < 0 )
	{
		nError = EINVAL;
		goto error1;
	}
	psCtx->ai_hWait = create_semaphore( "aiocb_buf_wait", 0, SEMSTYLE_COUNTING );
	if( psCtx->ai_hWait < 0 )
	{
		nError = EINVAL;
		goto error2;
	}

	/* The thread that will perform the I/O operations */
	while( do_get_symbol_address( psProc->pr_psImageContext, nLibrary++, "__aio_callback", -1, &pCallbackAddr ) == -ENOSYM )
		/* EMPTY */;

	if( pCallbackAddr == NULL )
	{
		printk( "%s: could not find __aio_callback\n", __FUNCTION__ );
		nError = EINVAL;
		goto error3;
	}

	psCtx->ai_hWorker = spawn_thread( "aio_worker", pCallbackAddr, IDLE_PRIORITY, 0, NULL );
	if( psCtx->ai_hWorker < 0 )
	{
		nError = EINVAL;
		goto error3;
	}
	psCtx->ai_nRun = AIO_RUN;
	wakeup_thread( psCtx->ai_hWorker, false );
		
	/* Stash the new AIO context */
	psProc->pr_psAioContext = psCtx;

	return 0;

error3:
	delete_semaphore( psCtx->ai_hWait );
error2:
	delete_semaphore( psCtx->ai_hLock );
error1:
	kfree( psCtx );
error:
	return nError;
}

/**
 * Append a new request into the async I/O queue.
 * \internal
 * \param psOp - pointer to a struct kaiocb structure.
 * \author Kristian Van Der Vliet (vanders@liqwyd.org)
 */
status_t aio_insert_op( struct kaiocb *psOp )
{
	Process_s *psProc = CURRENT_PROC;
	AIOContext_s *psCtx;
	status_t nError = EOK;

	LOCK( psProc->pr_hSem );

	/* Setup new AIO buffer & worker thread if required */
	if( psProc->pr_psAioContext == NULL )
	{
		nError = aio_create_context( psProc );
		if( nError != EOK )
		{
			printk( "%s: could not create new AIO context\n", __FUNCTION__ );
			UNLOCK( psProc->pr_hSem );
			goto error;
		}
	}
	psCtx = psProc->pr_psAioContext;

	UNLOCK( psProc->pr_hSem );

	/* Put the pointer to the aiocb into the ring buffer */
	LOCK( psCtx->ai_hLock );

	/* Add to list */
	psKcb->psNext = NULL;
	if( psCtx->ai_psHead == NULL )
		psCtx->ai_psHead = psOp;
	else
		psCtx->ai_psTail->psNext = psOp;
	psCtx->ai_psTail = psOp;

	UNLOCK( psCtx->ai_hLock );

	/* Notify worker */
	unlock_semaphore( psCtx->ai_hWait );

error:
	return nError;
}

/**
 * Queue an async I/O request.
 * \ingroup Syscalls
 * \param psAiocb - pointer to a struct aiocb structure.
 * \author Kristian Van Der Vliet (vanders@liqwyd.org)
 */
int sys_aio_request( struct aiocb *psAiocb )
{
	int nError = -EINVAL;

	if ( psAiocb && ( ( psAiocb->aio_lio_opcode == AIO_READ ) || ( psAiocb->aio_lio_opcode == AIO_WRITE ) ) )
	{
		struct kaiocb *psOp;

		psOp = kmalloc( sizeof( struct kaiocb ), MEMF_KERNEL|MEMF_LOCKED|MEMF_OKTOFAIL|MEMF_CLEAR ); 
		if( psOp != NULL )
		{
			memcpy_from_user( &psOp->sRequest, psAiocb, sizeof( struct aiocb ) );
			psOp->psOrig = psAiocb;
			psOp->hThread = get_thread_id( NULL );
			psOp->bKernel = false;

			nError = aio_insert_op( psOp );
		}
		else
			nError = -ENOMEM;
	}

	return nError;
}

/**
 * Queue an async write request on behalf of the kernel.
 * \internal
 * \param psFile - File to perform the operation for.
 * \param nPos - Offset.
 * \param pBuffer - Pointer to the data to write.
 * \param nLength - Length of the data to write.
 * \author Kristian Van Der Vliet (vanders@liqwyd.org)
 */
status_t aio_write_pos_p( File_s *psFile, off_t nPos, const void *pBuffer, size_t nLength )
{
	struct kaiocb *psOp;
	status_t nError;

	psOp = kmalloc( sizeof( struct kaiocb ), MEMF_KERNEL|MEMF_LOCKED|MEMF_OKTOFAIL|MEMF_CLEAR ); 
	if( psOp != NULL )
	{
		struct aiocb *psRequest = &psOp->sRequest;

		psOp->bKernel = true;
		psOp->psFile = psFile;

		psRequest->aio_fildes = 0;
		psRequest->aio_lio_opcode = AIO_WRITE;
		psRequest->aio_buf = (volatile void*)pBuffer;
		psRequest->aio_nbytes = nLength;
		psRequest->aio_offset = nPos;
		psRequest->aio_sigevent.sigev_notify = SIGEV_NONE;

		atomic_inc( &psFile->f_nRefCount );

		nError = aio_insert_op( psOp );
	}
	else
		nError = ENOMEM;

	return nError;
}
