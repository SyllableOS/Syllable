/* Copyright (C) 2002 Free Software Foundation, Inc.
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
#include <sys/un.h>
#include <netinet/in.h>

int __libc_sa_len (sa_family_t __af)
{
  return(__libc_sa_len_internal(__af));
}

int __libc_sa_len_internal (sa_family_t __af)
{
  switch(__af)
  {
    case AF_INET:
      return sizeof(struct sockaddr_in);
    case AF_INET6:
      return sizeof(struct sockaddr_in6);
    case AF_LOCAL:
      return sizeof(struct sockaddr_un);
/* The following are not supported in Syllable
     AF_PACKET
     AF_ASH
     AF_AX25
     AF_ECONET
     AF_IPX
     AF_ROSE
     AF_APPLETALK */
    default:
      return 0;
  }
}
