/* BSD `setjmp' entry point to `sigsetjmp (..., 0)'.  Stub version.
   Copyright (C) 1994, 1997, 1999 Free Software Foundation, Inc.
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

#include <sysdep.h>
#include <setjmp.h>

#undef setjmp

/* This implementation in C will not usually work, because the call
   really needs to be a tail-call so __sigsetjmp saves the state of
   the caller, not the state of this `setjmp' frame which then
   immediate unwinds.  */

int
setjmp (jmp_buf env)
{
  return __sigsetjmp (env, 0);
}
