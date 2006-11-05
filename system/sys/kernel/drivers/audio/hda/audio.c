#include <posix/errno.h>
#include <atheos/kernel.h>
#include <atheos/device.h>
#include <atheos/string.h>
#include <atheos/semaphore.h>
#include "audio.h"
#include <macros.h>

void generic_stream_init( GenericStream_s* psStream, bool bRecord )
{
	psStream->bRecord = bRecord;
	psStream->nSwPtr = 0;
	atomic_set( &psStream->nHwPtr, 0 );
	if( bRecord )
		atomic_set( &psStream->nAvailableFrags, 0 );
	else
		atomic_set( &psStream->nAvailableFrags, psStream->pfGetFragNumber( psStream->pDriverData ) );
	psStream->nPartialBufSize = 0;
	psStream->bRunning = false;
	psStream->hWakeup = create_semaphore( "generic_stream_wakeup", 0, 0 );
}

void generic_stream_free( GenericStream_s* psStream )
{
	delete_semaphore( psStream->hWakeup );
}

size_t generic_stream_write( GenericStream_s* psStream, void* pBuffer, size_t nSize )
{
	uint8* pSource = pBuffer;
	size_t nBytesCopied = 0;
	uint32 nAvailableFrags;
	
	
again:	
	
	nAvailableFrags = atomic_read( &psStream->nAvailableFrags );
	kassertw( nAvailableFrags >= 0 && nAvailableFrags <= psStream->pfGetFragNumber( psStream->pDriverData ) );
	while( nAvailableFrags == 0 )
	{
		//printk("Sleep...\n");
		sleep_on_sem( psStream->hWakeup, INFINITE_TIMEOUT );
		nAvailableFrags = atomic_read( &psStream->nAvailableFrags );
	}
	
	/* Copy data into the buffer */
	while( ( nSize > 0 ) && ( psStream->nPartialBufSize < psStream->pfGetFragSize( psStream->pDriverData ) ) )
	{
		size_t nLeft = psStream->pfGetFragSize( psStream->pDriverData ) - psStream->nPartialBufSize;
		if( nLeft > nSize )
			nLeft = nSize;
		//printk( "Put %i bytes at %i:%i\n", nLeft, (int)psStream->nSwPtr, (int)psStream->nPartialBufSize );
		uint8* pTarget = psStream->pfGetBuffer( psStream->pDriverData ) + psStream->nSwPtr * psStream->pfGetFragSize( psStream->pDriverData ) + psStream->nPartialBufSize;
		memcpy_from_user( pTarget, pSource, nLeft );
		nSize -= nLeft;
		psStream->nPartialBufSize += nLeft;
		pSource += nLeft;
		nBytesCopied += nLeft;
	}
	
	/* We cannot start unless we have a full fragment */
	if( psStream->nPartialBufSize < psStream->pfGetFragSize( psStream->pDriverData ) )
		return( nBytesCopied );
		
	/* Update software pointer */
	if( psStream->nSwPtr == ( psStream->pfGetFragNumber( psStream->pDriverData ) - 1 ) )
		psStream->nSwPtr = 0;
	else
		psStream->nSwPtr++;
	
	
	/* We have filled a complete buffer */
	atomic_dec( &psStream->nAvailableFrags );
	psStream->nPartialBufSize = 0;
	
//	printk( "Software pointer now %i %i\n", (int)psStream->nSwPtr, (int)atomic_read( &psStream->nAvailableFrags ) );
	
	
	/* Start */
	psStream->pfStart( psStream->pDriverData );
	psStream->bRunning = true;
	
	if( nSize > 0 )
		goto again;
		
	//printk( "Returning %i\n", nBytesCopied );
	return( nBytesCopied );
}

size_t generic_stream_read( GenericStream_s* psStream, void* pBuffer, size_t nSize )
{
	uint8* pTarget = pBuffer;
	size_t nBytesCopied = 0;
	uint32 nAvailableFrags;

	
	/* Start stream */
	psStream->pfStart( psStream->pDriverData );
	psStream->bRunning = true;
	
	
again:	
	
	nAvailableFrags = atomic_read( &psStream->nAvailableFrags );
	kassertw( nAvailableFrags >= 0 && nAvailableFrags <= psStream->pfGetFragNumber( psStream->pDriverData ) );
	while( nAvailableFrags == 0 )
	{
		//printk("Sleep...\n");
		sleep_on_sem( psStream->hWakeup, INFINITE_TIMEOUT );
		nAvailableFrags = atomic_read( &psStream->nAvailableFrags );
	}
	
	/* Copy data into the buffer */
	while( ( nSize > 0 ) && ( psStream->nPartialBufSize < psStream->pfGetFragSize( psStream->pDriverData ) ) )
	{
		size_t nLeft = psStream->pfGetFragSize( psStream->pDriverData ) - psStream->nPartialBufSize;
		if( nLeft > nSize )
			nLeft = nSize;
		//printk( "Read %i bytes at %i:%i\n", nLeft, (int)psStream->nSwPtr, (int)psStream->nPartialBufSize );
		uint8* pSource = psStream->pfGetBuffer( psStream->pDriverData ) + psStream->nSwPtr * psStream->pfGetFragSize( psStream->pDriverData ) + psStream->nPartialBufSize;
		memcpy_to_user( pTarget, pSource, nLeft );
		nSize -= nLeft;
		psStream->nPartialBufSize += nLeft;
		pTarget += nLeft;
		nBytesCopied += nLeft;
	}
	
	/* Check if we have only partially copied the buffer */
	if( psStream->nPartialBufSize < psStream->pfGetFragSize( psStream->pDriverData ) )
		return( nBytesCopied );
		
	/* Update software pointer */
	if( psStream->nSwPtr == ( psStream->pfGetFragNumber( psStream->pDriverData ) - 1 ) )
		psStream->nSwPtr = 0;
	else
		psStream->nSwPtr++;
	
	
	/* We have filled a complete buffer */
	atomic_dec( &psStream->nAvailableFrags );
	psStream->nPartialBufSize = 0;
	
	//printk( "Software pointer now %i %i\n", (int)psStream->nSwPtr, (int)atomic_read( &psStream->nAvailableFrags ) );
	
	
	if( nSize > 0 )
		goto again;
		
	//printk( "Returning %i\n", nBytesCopied );
	return( nBytesCopied );
}

void generic_stream_fragment_processed( GenericStream_s* psStream )
{
	/* Read hardware pointer */
	uint32 nHwPtr = atomic_read( &psStream->nHwPtr );
	
	//if( psStream->bRecord )
		//printk( "Fragment processed HW PTR:%i %i!\n", (int)atomic_read( &psStream->nHwPtr ), (int)atomic_read( &psStream->nAvailableFrags ) );
	
	/* Increase hardware pointer */
	if( nHwPtr == ( psStream->pfGetFragNumber( psStream->pDriverData ) - 1 ) )
		atomic_set( &psStream->nHwPtr, 0 );
	else
		atomic_inc( &psStream->nHwPtr );

	if( (uint32)atomic_read( &psStream->nAvailableFrags ) < psStream->pfGetFragNumber( psStream->pDriverData ) )
		atomic_inc( &psStream->nAvailableFrags );
	if( (uint32)atomic_read( &psStream->nAvailableFrags ) == psStream->pfGetFragNumber( psStream->pDriverData ) )
	{
		
		psStream->pfStop( psStream->pDriverData );
		printk("STOP!\n");
		psStream->bRunning = false;
	}
	
	wakeup_sem( psStream->hWakeup, false );
}


uint32 generic_stream_get_delay( GenericStream_s* psStream )
{
	if( !psStream->bRunning )
		return( 0 );
		
	uint32 nValue = psStream->pfGetFragNumber( psStream->pDriverData ) - atomic_read( &psStream->nAvailableFrags );
	if( nValue > 0 )
	{
		nValue *= psStream->pfGetFragSize(psStream->pDriverData );
		nValue -= psStream->pfGetCurrentPosition( psStream->pDriverData );
	}
	nValue += psStream->nPartialBufSize;
	
	return( nValue );
}

void generic_stream_clear( GenericStream_s* psStream )
{
	psStream->nSwPtr = 0;
	atomic_set( &psStream->nHwPtr, 0 );
	if( psStream->bRecord )
		atomic_set( &psStream->nAvailableFrags, 0 );
	else
		atomic_set( &psStream->nAvailableFrags, psStream->pfGetFragNumber( psStream->pDriverData ) );
	psStream->nPartialBufSize = 0;
	psStream->bRunning = false;
}






































