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

#ifndef __F_ATHEOS_SYSINFO_H__
#define __F_ATHEOS_SYSINFO_H__

#include <syllable/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_CPU_COUNT	16

enum 
{
	CPU_FEATURE_NONE 	= 0x00,
	CPU_FEATURE_MMX 	= 0x01,
	CPU_FEATURE_MMX2	= 0x02,
	CPU_FEATURE_3DNOW	= 0x04,
	CPU_FEATURE_3DNOWEX	= 0x08,
	CPU_FEATURE_SSE		= 0x10,
	CPU_FEATURE_SSE2	= 0x20,
	CPU_FEATURE_APIC	= 0x40,
	CPU_FEATURE_FXSAVE	= 0x80
};

typedef struct
{
  uint64	nCoreSpeed;
  uint64	nBusSpeed;
  bigtime_t	nActiveTime;
} CPUInfo_s;

enum { SYS_INFO_VERSION = 4 };

typedef struct
{
    int64     nKernelVersion;
    char      zKernelName[ OS_NAME_LENGTH ];	 	/* Name of kernel image				*/
    char      zKernelBuildDate[ OS_NAME_LENGTH ];	/* Date of kernel built				*/
    char      zKernelBuildTime[ OS_NAME_LENGTH ];	/* Time of kernel built				*/
    char      zKernelCpuArch[ OS_NAME_LENGTH ];		/* CPU this kernel is running on	*/
    char      zKernelSystem[ OS_NAME_LENGTH ];		/* OS name (E.g. "Syllable")		*/
    bigtime_t nBootTime;				/* time of boot (# usec since 1/1/70) */
    int	      nCPUCount;
    int	      nCPUType;
    CPUInfo_s asCPUInfo[MAX_CPU_COUNT];
    int       nMaxPages;				/* total # physical pages		*/
    int       nFreePages;				/* Number of free physical pages	*/
    int	      nCommitedPages;				/* Total number of allocated pages	*/
    int	      nKernelMemSize;
    int       nPageFaults;				/* Number of page faults		*/
    int       nUsedSemaphores;				/* Number of semaphores in use		*/
    int       nUsedPorts;				/* Number of message ports in use	*/
    int       nUsedThreads;			 	/* Number of living threads		*/
    int       nUsedProcesses;			 	/* Number of living processes		*/

    int	      nLoadedImageCount;
    int	      nImageInstanceCount;

    int	      nOpenFileCount;
    int       nAllocatedInodes;
    int       nLoadedInodes;
    int       nUsedInodes;
    int	      nBlockCacheSize;
    int	      nDirtyCacheSize;
    int	      nLockedCacheBlocks;
} system_info_v3;

/* version 4 - added kernel boot parameters */
typedef struct
{
    int64     nKernelVersion;
    char      zKernelName[ OS_NAME_LENGTH ];	 	/* Name of kernel image				*/
    char      zKernelBuildDate[ OS_NAME_LENGTH ];	/* Date of kernel built				*/
    char      zKernelBuildTime[ OS_NAME_LENGTH ];	/* Time of kernel built				*/
    char      zKernelCpuArch[ OS_NAME_LENGTH ];		/* CPU this kernel is running on	*/
    char      zKernelSystem[ OS_NAME_LENGTH ];		/* OS name (E.g. "Syllable")		*/
    char      zKernelBootParams[ 4096 ];			/* Boot parameters provided by bootloader */
    bigtime_t nBootTime;				/* time of boot (# usec since 1/1/70) */
    int	      nCPUCount;
    int	      nCPUType;
    CPUInfo_s asCPUInfo[MAX_CPU_COUNT];
    int       nMaxPages;				/* total # physical pages		*/
    int       nFreePages;				/* Number of free physical pages	*/
    int	      nCommitedPages;				/* Total number of allocated pages	*/
    int	      nKernelMemSize;
    int       nPageFaults;				/* Number of page faults		*/
    int       nUsedSemaphores;				/* Number of semaphores in use		*/
    int       nUsedPorts;				/* Number of message ports in use	*/
    int       nUsedThreads;			 	/* Number of living threads		*/
    int       nUsedProcesses;			 	/* Number of living processes		*/

    int	      nLoadedImageCount;
    int	      nImageInstanceCount;

    int	      nOpenFileCount;
    int       nAllocatedInodes;
    int       nLoadedInodes;
    int       nUsedInodes;
    int	      nBlockCacheSize;
    int	      nDirtyCacheSize;
    int	      nLockedCacheBlocks;
} system_info;

/* Userspace get_system_info() is deprecated because it doesn't properly handle versioning.
   get_system_info() is likely to cause segfaults on binaries compiled on recent Syllable versions.
   Use get_system_info_v() instead, or the get_system_info() macro.
*/
#ifndef __KERNEL__
status_t get_system_info( system_info* psInfo );
#endif

status_t get_system_info_v( system_info* psInfo, int nVersion );

/* This macro provides easy use of get_system_info_v with the latest version */
#define get_system_info( psInfo ) get_system_info_v( psInfo, SYS_INFO_VERSION )

#ifdef __cplusplus
}
#endif

#endif	/* __F_ATHEOS_SYSINFO_H__ */
