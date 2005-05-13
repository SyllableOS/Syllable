/*
 *  The AtheOS kernel
 *  Copyright (C) 1999 - 2001 Kurt Skauen
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

#ifndef __F_ATHEOS_POSIX_UIO_H__
#define __F_ATHEOS_POSIX_UIO_H__

#ifdef __KERNEL__
#include <atheos/types.h>
#include <posix/types.h>
#else
#include <sys/types.h>
#endif

struct iovec
{
  void*	 iov_base;
  size_t iov_len;
};

#define UIO_FASTIOV	8
#define UIO_MAXIOV	1024

#ifdef __KERNEL__
ssize_t	readv( int nFile, const struct iovec* psVector, int nCount );
ssize_t	writev( int nFile, const struct iovec* psVector, int nCount );
#endif

#endif /* __F_ATHEOS_POSIX_UIO_H__ */
