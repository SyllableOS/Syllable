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

#ifndef __F_AIO_H__
#define __F_AIO_H__

#include "inc/typedefs.h"
#include "inc/scheduler.h"

#include <posix/aio.h>
#include <macros.h>

typedef enum _AIOCtxState
{
	AIO_RUN,
	AIO_STOP,
	AIO_DONE	
} AIOCtxState_e;

struct _AIOContext
{
	Process_s *ai_psProc;		/* Owning process */
	thread_id ai_hWorker;		/* I/O worker */

	AIOCtxState_e ai_nRun;

	sem_id ai_hLock;			/* Mutex for ai_psFirst & ai_psLast */
	sem_id ai_hWait;			/* Semaphore to indicate work in buffer */

	struct kaiocb *ai_psHead;	/* First aiocb in the list */
	struct kaiocb *ai_psTail;	/* Last aiocb in the list */
};

struct kaiocb
{
	struct aiocb sRequest;
	struct aiocb *psOrig;		/* Original request */
	thread_id hThread;			/* Requesting thread */

	bool bKernel;				/* True if the request came from the kernel */
	File_s *psFile;				/* File to use instead of aio_fildes for kernel requests */

	struct kaiocb *psNext;
};

status_t aio_create_context( Process_s *psProc );
status_t aio_insert_op( struct kaiocb *psKcb );

status_t aio_stop( Process_s *psProc );
status_t aio_write_pos_p( File_s *psFile, off_t nPos, const void *pBuffer, size_t nLength );

int sys_aio_worker( void );
int sys_aio_request( struct aiocb *psAiocb );

#endif	/* __F_AIO_H__ */
