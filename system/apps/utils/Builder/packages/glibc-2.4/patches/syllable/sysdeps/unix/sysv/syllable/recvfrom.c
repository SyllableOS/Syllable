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

int __recvfrom(int fd, void *buf, size_t size, int flags,
               struct sockaddr *from, socklen_t *len)
{
  struct msghdr msg;
  struct iovec iov;
  int error;

  iov.iov_base = buf;
  iov.iov_len = size;

  msg.msg_iov = &iov;
  msg.msg_iovlen = 1;
  msg.msg_name = from;
  msg.msg_namelen = *len;

  error = recvmsg(fd, &msg, flags);
  *len = msg.msg_namelen;

  return error;
}
weak_alias(__recvfrom,recvfrom)


