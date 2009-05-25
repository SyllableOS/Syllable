/*
 *  The Syllable kernel
 *  Copyright (C) 2009 Kristian Van Der Vliet
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

#ifndef	_POSIX_AIO_H_
#define	_POSIX_AIO_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __KERNEL__
# include <atheos/types.h>
# include <posix/siginfo.h>
#endif

/* Asynchronous I/O control block.  */
struct aiocb
{
  int aio_fildes;				/* File desriptor.  */
  int aio_lio_opcode;			/* Operation to be performed.  */
  int aio_reqprio;				/* Request priority offset.  */
  volatile void *aio_buf;		/* Location of buffer.  */
  size_t aio_nbytes;			/* Length of transfer.  */
  off_t aio_offset;				/* File offset.  */
  struct sigevent aio_sigevent;	/* Signal number and value.  */

  /* Internal members.  */
  int __error_code;
  ssize_t __return_value;
};

/* Operation codes for `aio_lio_opcode'.  The opcodes are also used to
   indicate standard AIO functions (I.e. aio_read(), aio_write() */
enum
{
  LIO_READ,
#define LIO_READ LIO_READ
#define AIO_READ LIO_READ
  LIO_WRITE,
#define LIO_WRITE LIO_WRITE
#define AIO_WRITE LIO_WRITE
  LIO_NOP
#define LIO_NOP LIO_NOP
};

#ifdef __cplusplus
}
#endif

#endif	/*	_POSIX_AIO_H_ */
