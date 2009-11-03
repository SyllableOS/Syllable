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

#ifndef __F_KERNEL_SMP_H__
#define __F_KERNEL_SMP_H__

#include <kernel/types.h>

#ifndef __BUILD_KERNEL__
int get_processor_id(void);
#endif

enum { CPU_EXTENDED_INFO_VERSION = 1 };

typedef struct
{
	int nAcpiId;
	char anVendorID[16];
	char zName[255];
	uint64 nCoreSpeed;
	uint64 nBusSpeed;
	uint32 nDelayCount;
	int nFamily;
	int nModel;
	int nAPICVersion;
	bool bIsPresent;
	bool bIsRunning;
	bool bHaveFXSR; 		/* CPU has fast FPU save and restore */
	bool bHaveXMM;			/* CPU has SSE extensions */
	bool bHaveMTRR;			/* CPU has MTRRs */
	uint32 nFeatures;
} CPU_Extended_Info_v1_s;

typedef CPU_Extended_Info_v1_s CPU_Extended_Info_s;	/*default to latest version */

int get_active_cpu_count( void );
status_t get_cpu_extended_info( int nPhysicalCPUId, CPU_Extended_Info_s * _psInfo, int nVersion );
void update_cpu_speed(int nPhysicalCPUId, uint64 nCoreSpeed, uint32 nDelayCount );

void set_idle_loop_handler( void ( *pHandler )( int ) );
void set_cpu_time_handler( bigtime_t ( *pHandler )( int ) );

#endif /* __F_KERNEL_SMP_H__ */
