/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef _ALLOCS_H
#define _ALLOCS_H

extern _IMPEXP_KERNEL void *smalloc(unsigned int nbytes);
extern _IMPEXP_KERNEL void sfree(void *ptr);
extern _IMPEXP_KERNEL void *scalloc(unsigned int nobj, unsigned int size);
extern _IMPEXP_KERNEL void *srealloc(void *p, unsigned int newsize);

#endif
