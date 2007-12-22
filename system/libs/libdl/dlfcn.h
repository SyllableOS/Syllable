/* libdl for Syllable - Dynamic Library handling                             */
/*                                                                           */
/* Copyright (C) 2003 Kristian Van Der Vliet (vanders@liqwyd.com)            */
/*                                                                           */
/* This program is free software; you can redistribute it and/or             */
/* modify it under the terms of the GNU Library General Public License       */
/* as published by the Free Software Foundation; either version 2            */
/* of the License, or (at your option) any later version.                    */
/*                                                                           */
/* This program is distributed in the hope that it will be useful,           */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of            */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             */
/* GNU Library General Public License for more details.                      */

#ifndef __F_SYLLABLE_DLFCN_H_
#define __F_SYLLABLE_DLFCN_H_

#ifdef __cplusplus
extern "C"{
#endif

#define RTLD_LAZY	0x01
#define RTLD_NOW	0x02
#define RTLD_GLOBAL	0x04
#define RTLD_LOCAL	0x08

#define RTLD_NEXT	0x00

enum __dl_errors {
	_DL_ENONE,
	_DL_ENOGLOBAL,
	_DL_EBADHANDLE,
	_DL_EBADSYMBOL,
	_DL_ENOSYM,
	_DL_EBADLIBRARY
};

void *dlopen(const char *file, int mode);
void *dlsym(void *handle, const char *name);
int   dlclose(void *handle);
char *dlerror(void);

/* Type for namespace indeces. Required by Glibc link.h */
typedef long int Lmid_t;

#ifdef __cplusplus
}
#endif

#endif	/* __SYLLABLE_DLFCN_H_ */

