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

#include <sys/socket.h>

ssize_t __sendto (int fd, const void *buf, size_t size, int flags,
                  __CONST_SOCKADDR_ARG from, socklen_t len)
{
  struct msghdr msg;
  struct iovec iov;

  iov.iov_base = (void*)buf;
  iov.iov_len = size;

  msg.msg_iov = &iov;
  msg.msg_iovlen = 1;
  /* <socket/sys/socket.h> defines __CONST_SOCKADDR_ARG as a bloody
     union.  Answers on a postcard on how to cast that to void*
     
     Anyway we'll just choose the sockaddr member of the union and
     cast that, on the basis that the POSIX definition for this
     function uses struct sockaddr and nobody should be using anything
     else.  Unless they're GNU-rific and actually take advantage of
     the fact that Glibc uses a union for that argument.  Bleurgh. */
  msg.msg_name = (void*)from.__sockaddr__;
  msg.msg_namelen = len;

  return sendmsg(fd, &msg, flags);
}
weak_alias(__sendto,sendto)








