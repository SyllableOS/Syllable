#include <stdio.h>
#include <atheos/types.h>
#include <atheos/time.h>
#include <atheos/kernel.h>

int main( int argc, char** argv )
{
    system_info	sSysInfo;
    bigtime_t	nUpTime = get_system_time();
    bigtime_t     nIdleTime;
    char* pzTotPhysPF;
    char* pzPostfix;
    char* pzCachePF;
    char* pzDirtyPF;
    float	vTotPhys;
    float	vFreeMem;
    float	vCache;
    float	vDirty;
    int		nDays;
    int		nHour;
    int		nMin;
    int		nSec;
    bool	bDoLoop = false;
    int		i;

    if ( argc > 1 && strcmp( argv[1], "-l" ) == 0  ) {
	bDoLoop = true;
    }
loop:
    if ( bDoLoop ) {
	printf( "\x1b[H" );
    }
    nUpTime = get_system_time();
    get_system_info( &sSysInfo );

    vTotPhys = (float)(sSysInfo.nMaxPages * PAGE_SIZE);
    vFreeMem = (float)(sSysInfo.nFreePages * PAGE_SIZE);
    vCache   = (float)sSysInfo.nBlockCacheSize;
    vDirty   = (float)sSysInfo.nDirtyCacheSize;
    
    if ( vTotPhys > 1024.0f * 1024.0f ) {
	vTotPhys /= 1024.0f * 1024.0f;
	pzTotPhysPF = "Mb";
    } else if ( vTotPhys > 1024.0f ) {
	vTotPhys /= 1024.0f;
	pzTotPhysPF = "KB";
    } else {
	pzTotPhysPF = "B";
    }
    if ( vFreeMem > 1024.0f * 1024.0f ) {
	vFreeMem /= 1024.0f * 1024.0f;
	pzPostfix = "Mb";
    } else if ( vFreeMem > 1024.0f ) {
	vFreeMem /= 1024.0f;
	pzPostfix = "KB";
    } else {
	pzPostfix = "B";
    }
    if ( vCache > 1024.0f * 1024.0f ) {
	vCache /= 1024.0f * 1024.0f;
	pzCachePF = "MB";
    } else if ( vCache > 1024.0f ) {
	vCache /= 1024.0f;
	pzCachePF = "KB";
    } else {
	pzCachePF = "B";
    }
    if ( vDirty > 1024.0f * 1024.0f ) {
	vDirty /= 1024.0f * 1024.0f;
	pzDirtyPF = "MB";
    } else if ( vDirty > 1024.0f ) {
	vDirty /= 1024.0f;
	pzDirtyPF = "KB";
    } else {
	pzDirtyPF = "B";
    }

    
    printf( "Kernel info: NAME: %s VER: %d.%d.%d BUILT: %s %s\x1b[K\n",
	    sSysInfo.zKernelName,
	    (int)((sSysInfo.nKernelVersion >> 32) & 0xffff),
	    (int)((sSysInfo.nKernelVersion >> 16) & 0xffff),
	    (int)(sSysInfo.nKernelVersion & 0xffff),
	    sSysInfo.zKernelBuildDate, sSysInfo.zKernelBuildTime );
	printf( "Boot parameters: %s\n", sSysInfo.zKernelBootParams );
    printf( "Physical memory: TOT: %.2f%s AVAIL: %.2f%s KERN: %d\x1b[K\n",
	    vTotPhys, pzTotPhysPF, vFreeMem, pzPostfix, sSysInfo.nKernelMemSize );
    printf( "Disk cache     : TOT: %.2f%s DIRTY: %.2f%s\x1b[K\n", vCache, pzCachePF, vDirty, pzDirtyPF );
    printf( "Pagefaults     : %d\x1b[K\n", sSysInfo.nPageFaults );
    printf( "Used semaphores: %d\x1b[K\n", sSysInfo.nUsedSemaphores );
    printf( "Used ports     : %d\x1b[K\n", sSysInfo.nUsedPorts );
    printf( "Used threads   : %d\x1b[K\n", sSysInfo.nUsedThreads );
    printf( "Used processes : %d\x1b[K\n", sSysInfo.nUsedProcesses );

    printf( "Open files     : %d\x1b[K\n", sSysInfo.nOpenFileCount );
    printf( "Alloc Inodes   : %d\x1b[K\n", sSysInfo.nAllocatedInodes );
    printf( "Loaded Inodes  : %d\x1b[K\n", sSysInfo.nLoadedInodes );
    printf( "Used Inodes    : %d\x1b[K\n", sSysInfo.nUsedInodes );

 
    nDays	   = nUpTime / (1000000LL * 60LL * 60LL * 24LL);
    nUpTime -= nDays * (1000000LL * 60LL * 60LL * 24LL);

    nHour	=	nUpTime / (1000000LL * 60LL * 60LL);
    nUpTime -= nHour * (1000000LL * 60LL * 60LL);

    nMin	=	nUpTime / (1000000LL * 60LL);
    nUpTime -= nMin * (1000000LL * 60LL);
    nSec	=	nUpTime / 1000000LL;
	
    printf( "Up time        : %d days %d hours % d mins %d secs\x1b[K\n", nDays, nHour, nMin, nSec );
    printf( "CPU count      : %d\x1b[K\n", sSysInfo.nCPUCount );

    for ( i = 0 ; i < sSysInfo.nCPUCount ; ++i ) {
  
	nIdleTime = get_idle_time( i );
/*	
	nDays	= nUpTime / (1000000LL * 60LL * 60LL * 24LL);
	nUpTime -= nDays * (1000000LL * 60LL * 60LL * 24LL);

	nHour	=	nUpTime / (1000000LL * 60LL * 60LL);
	nUpTime -= nHour * (1000000LL * 60LL * 60LL);

	nMin	=	nUpTime / (1000000LL * 60LL);
	nUpTime -= nMin * (1000000LL * 60LL);
	nSec	=	nUpTime / 1000000LL;
	*/
	nUpTime = get_system_time();
	printf( "  CPU #%d: CORE: %.1fMHz BUS: %.1fMHz IDLE: %.2f%%\x1b[K\n", i,
		sSysInfo.asCPUInfo[i].nCoreSpeed / 1000000.0f, sSysInfo.asCPUInfo[i].nBusSpeed / 1000000.0f,
		(double)nIdleTime / (double)nUpTime * 100.0,  nDays, nHour, nMin, nSec );
    
//    printf( "Idle: %d days %d hours % d mins %d secs\n", nDays, nHour, nMin, nSec );
    }
    if ( bDoLoop ) {
	printf( "\x1b[J" );
	fflush( stdout );
	snooze( 1000000LL );
	goto loop;
    }
}
