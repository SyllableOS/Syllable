
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
 *  Foundation, Inc., 58 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __ATHEOS_SYSTEMBASE_H__
#define __ATHEOS_SYSTEMBASE_H__

#include <atheos/kernel.h>
#include <atheos/filesystem.h>
#include <atheos/bootmodules.h>

#include "intel.h"
#include "mman.h"
#include "array.h"
#include "scheduler.h"

#ifdef __cplusplus
extern "C"
{
#if 0
}				/*make emacs indention work */
#endif
#endif

#define MAX_BOOTMODULE_COUNT		64
#define MAX_BOOTMODULE_ARGUMENT_SIZE	32768

struct SystemBase
{

      /*** Timer Variables ***/

	bigtime_t ex_nRealTime;
	bigtime_t ex_nBootTime;

      /*** linked list of ready threads that haven't consumed all their timeslice ***/

	ThreadList_s ex_sFirstReady;

	  /*** linked list of ready threads that have consumed all their timeslice (expired) ***/

	ThreadList_s ex_sFirstExpired;

      /*** Other Globals ***/

	struct i3IntrGate ex_IDT[256];	/* Interrupt Descriptor Table   */
	struct i3Desc ex_GDT[8192];	/* Global Descriptor Table      */
	uint8 ex_DTAllocList[8192];	/* array describing whitch descriptors allocated in the different tables */
	struct i3MTRRDescrTable ex_sMTRR; /* The MTRR descriptor table */

	char *ex_pNullPage;

	thread_id ex_hInitProc;

	bool ex_bSingleUserMode;

	/* Start and end of the user address-space. This is normally
	 * 0x80000000 -> 0xffffffff but can be configured by kernel-parameters
	 * to workaround shortcomings in some virtual machines (VMWare) where
	 * some addresses are used by the VM and can't be used by the OS.
	 */
	uint32 sb_nFirstUserAddress;
	uint32 sb_nLastUserAddress;

      /*** Various resource/statistics counters ***/
	atomic_t ex_nPageFaultCount;

	int ex_nTotalPageCount;
	atomic_t ex_nFreePageCount;
	int ex_nCommitPageCount;
	atomic_t ex_nKernelMemSize;	/* kmalloced() memory */
	atomic_t ex_nKernelMemPages;	/* Pages allocated by kmalloc() */
	atomic_t ex_nBlockCacheSize;
	atomic_t ex_nDirtyCacheSize;
	int ex_nLockedCacheBlocks;
	atomic_t ex_nSemaphoreCount;
	atomic_t ex_nMessagePortCount;
	atomic_t ex_nProcessCount;
	atomic_t ex_nThreadCount;

	atomic_t ex_nLoadedImageCount;
	atomic_t ex_nImageInstanceCount;

	atomic_t ex_nAllocatedInodeCount;
	atomic_t ex_nLoadedInodeCount;
	atomic_t ex_nUsedInodeCount;
	atomic_t ex_nOpenFileCount;

	int ex_nBootModuleCount;
	char ex_anBootModuleArgs[MAX_BOOTMODULE_ARGUMENT_SIZE];
	BootModule_s ex_asBootModules[MAX_BOOTMODULE_COUNT];
};

extern struct SystemBase *SysBase;
extern struct SystemBase g_sSysBase;

static const uint8_t DTAL_IDT = 0x01;		/* allocated in IDT     */
static const uint8_t DTAL_GDT = 0x02;		/* allocated in GDT     */
static const uint8_t DTAL_LDT = 0x04;		/* allocated in LDT     */

#ifdef __cplusplus
}
#endif

#endif /* __ATHEOS_SYSTEMBASE_H__ */
