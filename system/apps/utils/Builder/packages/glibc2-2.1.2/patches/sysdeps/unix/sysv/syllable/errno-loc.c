/* MT support function to get address of `errno' variable on Syllable
   
   Copyright (C) 2001 Kurt Skauen
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the GNU C Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

#include <errno.h>
#include <atheos/tld.h>
#undef errno

int *
weak_const_function
__errno_location (void)
{
    int* errno_addr;
    __asm__("movl %%gs:(%1),%0" : "=r" (errno_addr) : "r"  (TLD_ERRNO_ADDR) );
    return( errno_addr );
}
