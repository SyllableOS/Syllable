/* misc.c -- misc utility routines for xspringies
 * Copyright (C) 1991,1992  Douglas M. DeCarlo
 *
 * This file is part of XSpringies, a mass and spring simulation system for X
 *
 * XSpringies is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 1, or (at your option)
 * any later version.
 *
 * XSpringies is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with XSpringies; see the file COPYING.  If not, write to
 * the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include "defs.h"

#if defined(__STRICT_BSD__) || defined(__STDC__) || defined(_ANSI_C_SOURCE)
extern void *malloc(), *realloc();
#else
extern char *malloc(), *realloc();
#endif

/* malloc space, and call fatal if allocation fails */
char *xmalloc (size)
int size;
{
    register char *tmp = (char *)malloc(size);

    if (!tmp)
      fatal ("Out of memory");

    return tmp;
}

/* realloc space, and call fatal if re-allocation fails 
   (also, call malloc if ptr is NULL) */
char *xrealloc (ptr, size)
char *ptr;
int size;
{
    register char *tmp;

    if (ptr == NULL)
      return (char *)xmalloc(size);

    tmp = (char *)realloc(ptr, size);

    if (!tmp)
      fatal ("Out of memory");

    return tmp;
}
