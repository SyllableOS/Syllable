/*
 *  ATA/ATAPI driver for Syllable
 *
 *  Copyright (C) 2003 Kristian Van Der Vliet
 *  Copyright (C) 2002 William Rose
 *  Copyright (C) 1999 - 2001 Kurt Skauen
 *
 *  Code from the Linux ide driver is
 *  Copyright (C) 1994, 1995, 1996  scott snyder  <snyder@fnald0.fnal.gov>
 *  Copyright (C) 1996-1998  Erik Andersen <andersee@debian.org>
 *  Copyright (C) 1998-2000  Jens Axboe <axboe@suse.de>
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

#ifndef __ATA_DRIVE_H_
#define __ATA_DRIVE_H_

/* Device function prototypes */
int ata_open( void* pNode, uint32 nFlags, void **ppCookie );
int ata_close( void* pNode, void* pCookie );
int ata_read( void* pNode, void* pCookie, off_t nPos, void* pBuf, size_t nLen );
int ata_write( void* pNode, void* pCookie, off_t nPos, const void* pBuf, size_t nLen );
int ata_readv( void* pNode, void* pCookie, off_t nPos, const struct iovec* psVector, size_t nCount );
int ata_writev( void* pNode, void* pCookie, off_t nPos, const struct iovec* psVector, size_t nCount );
status_t ata_ioctl( void* pNode, void* pCookie, uint32 nCommand, void* pArgs, bool bFromKernel );

#endif

