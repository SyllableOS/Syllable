
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

#ifndef	__F_INC_SEMAPHORES_H__
#define	__F_INC_SEMAPHORES_H__

#include <atheos/types.h>
#include "typedefs.h"

#ifdef __cplusplus
extern "C"
{
#if 0
}				/*make emacs indention work */
#endif
#endif

typedef struct _Semaphore Semaphore_s;
struct _Semaphore
{
	char ss_zName[OS_NAME_LENGTH];
	sem_id ss_hSemaID;
	int32 ss_lNestCount;
	uint32 ss_nFlags;
	WaitQueue_s *ss_psWaitQueue;
	thread_id ss_hHolder;	/* Last thread holding the semaphore            */
	proc_id ss_hOwner;	/* The process that created the semaphore       */
	int ss_nDeadLockDetectRun;
	Semaphore_s *ss_psNext;	/* Next in process */
	Semaphore_s *ss_psPrev;	/* Previous in process */
//  int32               ss_anLockers[16];
//  int         ss_nLockBufPos;
};

typedef struct _RWHolder RWHolder_s;
struct _RWHolder
{
	RWHolder_s *rwh_psNext;
	thread_id rwh_hThreadId;
	int32 rwh_nReadCount;
	int32 rwh_nWriteCount;
};

typedef struct _RWWaiter RWWaiter_s;
struct _RWWaiter
{
	RWWaiter_s *rww_psNext;
	WaitQueue_s *rww_psWaitQueue;
	int32 rww_nReading;
};

typedef struct _RWLock RWLock_s;
struct _RWLock
{
	Semaphore_s rw_sSemaphore;
	RWHolder_s *rw_psHolders;
	RWWaiter_s *rw_psWaiters;
};

typedef struct
{
	Semaphore_s **sc_apsSemaphores;
	uint32 *sc_pnAllocMap;
	int sc_nAllocCount;
	int sc_nMaxCount;
	int sc_nLastID;
} SemContext_s;

void init_semaphores( void );
SemContext_s *create_semaphore_context( void );
SemContext_s *clone_semaphore_context( SemContext_s * psOrig, proc_id hNewOwner );
void release_thread_semaphores( thread_id hThread );
void update_semaphore_context( SemContext_s * psCtx, proc_id hOwner );
void exit_free_semaphores( Process_s *psProc );
void exec_free_semaphores( Process_s *psProc );

Semaphore_s *get_semaphore_by_handle( proc_id hProcess, sem_id hSema );

static const uint32_t SEM_TYPE_MASK	= 0xff000000;
static const uint32_t SEM_ID_MASK	= 0x00ffffff;
static const uint32_t SEMTYPE_USER	= 0x00000000;
static const uint32_t SEMTYPE_GLOBAL	= 0x01000000;
static const uint32_t SEMTYPE_KERNEL	= 0x02000000;

#ifdef __cplusplus
}
#endif


#endif /* __F_INC_SEMAPHORES_H__ */
