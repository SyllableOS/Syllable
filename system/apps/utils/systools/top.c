#include <string.h>
#include <stdio.h>
#include <malloc.h>
#include <termios.h>
#include <assert.h>

#include <atheos/types.h>
#include <atheos/threads.h>
#include <atheos/semaphore.h>
#include <atheos/kernel.h>

#include <macros.h>

typedef struct
{
    thread_id	nThreadID;
    proc_id	nProcID;
    bigtime_t	nThisTime;
    bigtime_t	nLastTime;
    char		zThreadName[32];
    char		zProcName[32];
    char		zState[32];
    int		nPri;
    int		nRunNumber;
} TopThreadInfo_s;


static int g_nThreadCount = 0;
static int g_nMaxThreadCount = 0;
static int g_nRunNumber = 1;
static TopThreadInfo_s** g_apsList = NULL;

int SortCmp( const void* p1, const void* p2 )
{
    TopThreadInfo_s* psI1 = *((TopThreadInfo_s**)p1);
    TopThreadInfo_s* psI2 = *((TopThreadInfo_s**)p2);

    if ( psI1->nRunNumber != psI2->nRunNumber ) {
	return( psI2->nRunNumber - psI1->nRunNumber );
    }
    return( (psI2->nThisTime - psI2->nLastTime) - (psI1->nThisTime - psI1->nLastTime) );
}



void insert_thread( thread_info* psInfo )
{
    int	i;
  
    for ( i = 0 ; i < g_nThreadCount ; ++i )
    {
	if ( psInfo->ti_thread_id == g_apsList[i]->nThreadID )
	{
	    g_apsList[i]->nLastTime = g_apsList[i]->nThisTime;
	    g_apsList[i]->nThisTime = psInfo->ti_user_time;
	    g_apsList[i]->nRunNumber = g_nRunNumber;

	    g_apsList[i]->nPri	 = psInfo->ti_priority;
	    strcpy( g_apsList[i]->zThreadName, psInfo->ti_thread_name );
	    strcpy( g_apsList[i]->zProcName, psInfo->ti_process_name );
	    return;
	}
    }
    if ( g_nThreadCount >= g_nMaxThreadCount )
    {
	int nNewCount = max( g_nMaxThreadCount << 1, 8 );
	TopThreadInfo_s* apsNewList;
	if ( g_apsList == NULL ) {
	    g_apsList = malloc( nNewCount * sizeof( TopThreadInfo_s* ) );
	} else {
	    g_apsList = realloc( g_apsList, nNewCount * sizeof( TopThreadInfo_s* ) );
	}
	assert( nNewCount == 0 || g_apsList != NULL );
	g_nMaxThreadCount = nNewCount;
    }
    g_apsList[g_nThreadCount] = malloc( sizeof( TopThreadInfo_s ) );
    g_apsList[g_nThreadCount]->nThreadID  = psInfo->ti_thread_id;
    g_apsList[g_nThreadCount]->nProcID    = psInfo->ti_process_id;
    g_apsList[g_nThreadCount]->nThisTime  = psInfo->ti_user_time;
    g_apsList[g_nThreadCount]->nLastTime  = psInfo->ti_user_time;
    g_apsList[g_nThreadCount]->nPri	  = psInfo->ti_priority;
    g_apsList[g_nThreadCount]->nRunNumber = g_nRunNumber;

    strcpy( g_apsList[g_nThreadCount]->zThreadName, psInfo->ti_thread_name );
    strcpy( g_apsList[g_nThreadCount]->zProcName, psInfo->ti_process_name );
    g_nThreadCount++;
  
}

void print_list()
{
    struct winsize sWinSize;
    int	 nCount;
    int	 i;
    bigtime_t nTotTime = 0;
  
    ioctl( 0, TIOCGWINSZ, &sWinSize );

    nCount = min( g_nThreadCount, sWinSize.ws_row - 1 );

    for ( i = 0 ; i < nCount ; ++i )
    {
	TopThreadInfo_s* psInfo = g_apsList[i];
	nTotTime += psInfo->nThisTime - psInfo->nLastTime;
    }
  
    printf( "\x1b[H" );
//  printf( "\x1b[H\x1b[2J" );
    for ( i = 0 ; i < nCount ; ++i )
    {
	TopThreadInfo_s* psInfo = g_apsList[i];
	bigtime_t nTime = psInfo->nThisTime - psInfo->nLastTime;
    
	printf( "%-13.13s %-16.16s (%04d:%04d) %4d, %.3fs %5.1f%%\x1b[K\n",
		psInfo->zProcName, psInfo->zThreadName,
		psInfo->nProcID, psInfo->nThreadID,
		psInfo->nPri,
		(float)nTime / 1000000.0f, (float)nTime * 100.0f / (float)nTotTime );
    
    }
    printf( "\x1b[J" );
}

void update_list()
{
    thread_info sInfo;
//    thread_id hThread;
    status_t  nError;
    int	      i;
    
//    for ( hThread = 0 ; -1 != hThread ; hThread = get_next_thread( hThread ) )
    for ( nError = get_thread_info( -1, &sInfo ) ; nError >= 0 ; nError = get_next_thread_info( &sInfo ) )
    {
/*
	if ( get_thread_info( hThread, &sInfo ) != 0 ) {
	    continue;
	}*/
	insert_thread(  &sInfo );
    }
    qsort( g_apsList, g_nThreadCount, sizeof(TopThreadInfo_s*), SortCmp );
    for ( i = g_nThreadCount - 1 ; i >= 0  ; --i )
    {
	if ( g_apsList[i]->nRunNumber != g_nRunNumber ) {
	    free( g_apsList[i] );
	} else {
	    g_nThreadCount = i + 1;
	    break;
	}
    }
    g_nRunNumber++;
}
    
int main( int argc, char** argv )
{
    uint32	nDelay = 1000000 * 5;

    if ( argc == 2 ) {
	nDelay = atol( argv[1] ) * 1000;
    }
  
    update_list();
    printf( "Wait for initial update\n" );
    for (;;)
    {
	snooze( nDelay );
	update_list();
	print_list();
    }
}



