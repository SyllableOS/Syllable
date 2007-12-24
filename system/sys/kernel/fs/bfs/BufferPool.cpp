/* BufferPool - a buffer pool for uncached file access
**
** Initial version by Axel Dörfler, axeld@pinc-software.de
** This file may be used under the terms of the OpenBeOS License.
*/

#include "Utility.h"
#include "BufferPool.h"
#include "Debug.h"

#include "kernel_cpp.h"
#include <atheos/semaphore.h>


const uint32 kNumBuffers = 8;


BufferPool::BufferPool()
	:
	fFirstFree(NULL)
{
	fLock = create_semaphore("buffer lock", 1, 0);
	fFreeBuffers = create_semaphore("free buffers", 0, 0);

#ifndef USER
	//set_sem_owner(fLock, B_SYSTEM_TEAM);
	//set_sem_owner(fFreeBuffers, B_SYSTEM_TEAM);
#endif
}


BufferPool::~BufferPool()
{
	delete_semaphore(fFreeBuffers);

	LOCK(fLock);
		// the return value doesn't interest us anymore

	void **buffer = fFirstFree;
	while (buffer != NULL) {
		void **nextBuffer = (void **)*buffer;
		free(buffer);
		buffer = nextBuffer;
	}

	delete_semaphore(fLock);
}


status_t 
BufferPool::InitCheck()
{
	if (fLock < B_OK
		|| fFreeBuffers < B_OK)
		return B_ERROR;

	return B_OK;
}


status_t 
BufferPool::RequestBuffers(uint32 blockSize)
{
	void **buffers[kNumBuffers];

	// allocate and connect buffers

	for (uint32 i = 0; i < kNumBuffers; i++) {
		buffers[i] = (void **)malloc(blockSize);
		if (buffers[i] == NULL) {
			// free already allocated buffers
			for (;i-- > 0; i++)
				free(buffers[i]);
			RETURN_ERROR(B_NO_MEMORY);
		}
		if (i > 0)
			*(buffers[i]) = buffers[i - 1];
	}

	// add the buffers to the free buffers queue

	status_t status = LOCK(fLock);
	if (status == B_OK) {
		*(buffers[0]) = fFirstFree;
		fFirstFree = buffers[kNumBuffers - 1];
		UNLOCK(fLock);
		unlock_semaphore_ex(fFreeBuffers, kNumBuffers);
	} else {
		for (uint32 i = 0; i < kNumBuffers; i++)
			free(buffers[i]);
	}

	RETURN_ERROR(status);
}


status_t 
BufferPool::ReleaseBuffers()
{
	status_t status = lock_semaphore_ex(fFreeBuffers, kNumBuffers, 0, 0);
	if (status < B_OK)
		return status;

	status = LOCK(fLock);
	if (status < B_OK)
		return status;

	void **buffer = fFirstFree;
	for (uint32 i = 0; i < kNumBuffers && buffer; i++) {
		void **nextBuffer = (void **)*buffer;

		free(buffer);
		buffer = nextBuffer;
	}
	fFirstFree = buffer;

	UNLOCK(fLock);
	return B_OK;
}


status_t
BufferPool::GetBuffer(void **_buffer)
{
	status_t status = LOCK(fFreeBuffers);
	if (status < B_OK)
		return status;

	if ((status = LOCK(fLock)) < B_OK) {
		UNLOCK(fFreeBuffers);
		return status;
	}

	void **buffer = fFirstFree;
	fFirstFree = (void **)*buffer;

	UNLOCK(fLock);

	*_buffer = (void *)buffer;
	return B_OK;
}


status_t 
BufferPool::PutBuffer(void *_buffer)
{
	void **buffer = (void **)_buffer;
	if (buffer == NULL)
		return B_BAD_VALUE;

	status_t status = LOCK(fLock);
	if (status < B_OK)
		return status;

	*buffer = fFirstFree;
	fFirstFree = buffer;

	UNLOCK(fLock);
	UNLOCK(fFreeBuffers);

	return B_OK;
}

