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
#include <unistd.h>
#include <signal.h>

#ifdef BE_AIO64
#define aiocb		aiocb64
#define aio_read	aio_read64
#endif

int
aio_read (struct aiocb *aiocbp)
{
  int ret = 0;
  ssize_t err;
  struct sigevent *ev;

  aiocbp->__error_code = 0;

  err = pread( aiocbp->aio_fildes, aiocbp->aio_buf, aiocbp->aio_nbytes, aiocbp->aio_offset );
  if( err >= 0 )
    aiocbp->__return_value = err;
  else
  {
    aiocbp->__error_code = errno;
    ret = -1;
  }

  ev = &aiocbp->aio_sigevent;
  switch( ev->sigev_notify )
  {
    case SIGEV_SIGNAL:
    {
      raise( ev->sigev_signo );
      break;
    }
    case SIGEV_THREAD:
    {
      ev->sigev_notify_function( ev->sigev_value );
      break;
    }
    case SIGEV_NONE:
    default:
      break;
  }

  return ret;
}

