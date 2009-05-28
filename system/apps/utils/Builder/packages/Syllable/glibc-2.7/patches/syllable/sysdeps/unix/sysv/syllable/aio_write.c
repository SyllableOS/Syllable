/* Copyright (C) 1996, 1997, 1998, 1999, 2002 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.  */

#include <aio.h>
#include <errno.h>
#include <sysdep.h>
#include <sys/types.h>
#include <sys/syscall.h>

#ifdef BE_AIO64
#define aiocb		aiocb64
#define aio_write	aio_write64
#endif

int
aio_write (struct aiocb *aiocbp)
{
  int err;

  aiocbp->aio_lio_opcode = AIO_WRITE;
  aiocbp->__error_code = 0;
  aiocbp->__return_value = 0;

  err = INLINE_SYSCALL(aio_request,1,aiocbp);
  return err;
}

