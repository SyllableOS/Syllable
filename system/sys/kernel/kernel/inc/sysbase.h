
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

#ifndef __ATHEOS_SYSTEMBASE_H__
#define __ATHEOS_SYSTEMBASE_H__

#include <atheos/kernel.h>
#include <atheos/filesystem.h>
#include <atheos/bootmodules.h>

#include "intel.h"
#include "mman.h"
#include "array.h"

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

      /*** Single linked list of ready threads ***/

	Thread_s *ex_psFirstReady;

      /*** Other Globals ***/

	struct i3IntrGate ex_IDT[256];	/* Interrupt Descriptor Table   */
	struct i3Desc ex_GDT[8192];	/* Global Descriptor Table      */
	uint8 ex_DTAllocList[8192];	/* array describing whitch descriptors allocated in the different tables */

	MemHeader_s ex_sRealMemHdr;	/* Real memory  (mem below 1M)          */

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
	int ex_nPageFaultCount;

	int ex_nTotalPageCount;
	int ex_nFreePageCount;
	int ex_nCommitPageCount;
	int ex_nKernelMemSize;	/* kmalloced() memory */
	int ex_nKernelMemPages;	/* Pages allocated by kmalloc() */
	int ex_nBlockCacheSize;
	int ex_nDirtyCacheSize;
	int ex_nLockedCacheBlocks;
	int ex_nSemaphoreCount;
	int ex_nMessagePortCount;
	int ex_nProcessCount;
	int ex_nThreadCount;

	int ex_nLoadedImageCount;
	int ex_nImageInstanceCount;

	int ex_nAllocatedInodeCount;
	int ex_nLoadedInodeCount;
	int ex_nUsedInodeCount;
	int ex_nOpenFileCount;

	int ex_nBootModuleCount;
	char ex_anBootModuleArgs[MAX_BOOTMODULE_ARGUMENT_SIZE];
	BootModule_s ex_asBootModules[MAX_BOOTMODULE_COUNT];
};

extern struct SystemBase *SysBase;
extern struct SystemBase g_sSysBase;

#define	DTAL_IDT 0x01		/* allocated in IDT     */
#define	DTAL_GDT 0x02		/* allocated in GDT     */
#define	DTAL_LDT 0x04		/* allocated in LDT     */

#ifdef __cplusplus
}
#endif

#endif /* __ATHEOS_SYSTEMBASE_H__ */
