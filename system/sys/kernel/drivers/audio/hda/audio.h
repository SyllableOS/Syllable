#ifndef _AUDIO_H_
#define _AUDIO_H_

#define AUDIO_NAME_LENGTH 255

typedef struct
{
	uint32 ( *pfGetFragSize )( void* pDriverData );
	uint32 ( *pfGetFragNumber )( void* pDriverData );
	uint8* ( *pfGetBuffer )( void* pDriverData );
	uint32 ( *pfGetCurrentPosition )( void* pDriverData );
	status_t ( *pfStart )( void* pDriverData );
	void ( *pfStop )( void* pDriverData );
	void* pDriverData;
	
	bool bRecord;
	int nBufferSize;
	unsigned int nSwPtr;
	atomic_t nHwPtr;
	atomic_t nAvailableFrags;
	unsigned int nPartialBufSize;
	bool bRunning;
	sem_id hWakeup;
} GenericStream_s;


void generic_stream_init( GenericStream_s* psStream, bool bRecord );
void generic_stream_free( GenericStream_s* psStream );
size_t generic_stream_read( GenericStream_s* psStream, void* pBuffer, size_t nSize );
size_t generic_stream_write( GenericStream_s* psStream, void* pBuffer, size_t nLen );
void generic_stream_fragment_processed( GenericStream_s* psStream );
uint32 generic_stream_get_delay( GenericStream_s* psStream );
void generic_stream_clear( GenericStream_s* psStream );

#endif













