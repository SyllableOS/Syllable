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

#ifndef __F_ATHEOS_STDLIB_H__
#define __F_ATHEOS_STDLIB_H__

#ifdef __KERNEL__
#include <stdarg.h>
#include <atheos/types.h>
#include <atheos/string.h>
/* From string.c */
#ifdef __cplusplus
extern "C" {
#endif

int stricmp(const char * cs,const char * ct);
int strnicmp(const char * cs,const char * ct,size_t count);
size_t strspn(const char *s, const char *accept);
char * strpbrk(const char * cs,const char * ct);


char * strcpy(char * dest,const char *src);
char * strncpy(char * dest,const char *src,size_t count);
char * strcat(char * dest, const char * src);
char * strncat(char *dest, const char *src, size_t count);
int strcmp(const char * cs,const char * ct);
int strncmp(const char * cs,const char * ct,size_t count);
char * strchr(const char * s, int c);
char * strrchr(const char * s, int c);
size_t strlen(const char * s);
size_t strnlen(const char * s, size_t count);
char * strtok(char * s,const char * ct);
/*void * memset( void* s, int c, size_t count);*/
void * bcopy(const void * src, void * dest, size_t count);
/*void * memcpy(void * dest,const void *src,size_t count);*/
void * memmove(void * dest,const void *src,size_t count);
int memcmp(const void * cs,const void * ct,size_t count);
void * memscan(void * addr, int c, size_t size);
char * strstr(const char * s1,const char * s2);

long atol(const char *str);
long strtol(const char *nptr, char **endptr, int base);

/* From vsprintf.c */

int vsprintf(char *buf, const char *fmt, va_list args);
int sprintf(char * buf, const char *fmt, ...);

/* From qsort.c */

void qsort(void *base0, size_t n, size_t size, int (*compar)(const void *, const void *));

#ifdef __cplusplus
}
#endif

#endif /* __KERNEL__ */

#endif /* __F_ATHEOS_STDLIB_H__ */
