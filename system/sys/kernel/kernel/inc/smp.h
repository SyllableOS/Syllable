/*
 *  The Syllable kernel
 *  Copyright (C) 2006 Kristian Van Der Vliet
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

#ifndef __F_SMP_H__
#define __F_SMP_H__

#include <atheos/kernel.h>
#include <atheos/types.h>

#include "typedefs.h"
#include "intel.h"
#include "apic.h"

typedef struct
{
	int pi_nAcpiId;
	char pi_anVendorID[16];
	char pi_zName[255];
	uint64 pi_nCoreSpeed;
	uint64 pi_nBusSpeed;
	uint32 pi_nDelayCount;
	uint32 pi_nGS;			/* GS segment, used for thread specific data */
	int64 pi_nCPUTime; /* Time of last schedule */	
	int pi_nFamily;
	int pi_nModel;
	int pi_nAPICVersion;
	Thread_s *pi_psCurrentThread;
	Thread_s *pi_psIdleThread;
	bigtime_t pi_nIdleTime;
	TaskStateSeg_s pi_sTSS;	/* Intel 386 specific task state buffer */
	bool pi_bIsPresent;
	bool pi_bIsRunning;
	bool pi_bHaveFXSR; 		/* CPU has fast FPU save and restore */
	bool pi_bHaveXMM;		/* CPU has SSE extensions */
	bool pi_bHaveMTRR;		/* CPU has MTRRs */
	uint32 pi_nFeatures;
} ProcessorInfo_s;

/* Kernel-global variables */
extern ProcessorInfo_s g_asProcessorDescs[MAX_CPU_COUNT];
extern int g_nBootCPU;
extern int g_nActiveCPUCount;

extern bool g_bAPICPresent;

typedef struct
{
	uint32 tc_nJmpInstr;
	uint32 tc_nKernelEntry;
	uint32 tc_nKernelStack;
	uint32 tc_nKernelDS;
	uint32 tc_nKernelCS;
	uint32 tc_nGdt;		/* Pointer to global descriptor table */
} SmpTrampolineCfg_s;

/* This is the number of bits of precision for pi_nDelayCount.  Each bit takes on average 1.5 / INT_FREQ
   seconds.  This is a little better than 1%. */
#define LPS_PREC 8;

static inline volatile ProcessorInfo_s* get_processor( void )
{
	return( &g_asProcessorDescs[get_processor_id()] );
}

static inline int get_processor_acpi_id( int nProc )
{
	return( g_asProcessorDescs[nProc].pi_nAcpiId );
}

/* XXXKV */
#define barrier()	__asm__ __volatile( "": : :"memory")

#define mb()		__asm__ __volatile__( "lock; addl $0, 0(%%esp)": : :"memory")
#define rmb()		barrier()
#define wmb()		mb()

int logical_to_physical_cpu_id( int nLogicalID );

void init_smp( bool bInitSMP, bool bScanACPI );
void boot_ap_processors( void );
void shutdown_ap_processors( void );
void shutdown_processor( void );
inline bigtime_t get_cpu_time( int );

#endif	/* __F_SMP_H__ */

