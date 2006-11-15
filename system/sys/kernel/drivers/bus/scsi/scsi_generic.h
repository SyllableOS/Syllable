/*
 *  The Syllable kernel
 *  Simple SCSI layer
 *  Contains some linux kernel code
 *  Copyright (C) 2003 Arno Klenke
 *  Copyright (C) 2006 Kristian Van Der Vliet
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of version 2 of the GNU
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
 *
 */

#ifndef SCSI_GENERIC_H_
#define SCSI_GENERIC_H_

#include <atheos/types.h>
#include <atheos/scsi.h>

int scsi_generic_open( void *pNode, uint32 nFlags, void **ppCookie );
int scsi_generic_close( void *pNode, void *pCookie );
int scsi_generic_read( void *pNode, void *pCookie, off_t nPos, void *pBuffer, size_t nLen );
int scsi_generic_write( void *pNode, void *pCookie, off_t nPos, const void *pBuffer, size_t nLen );
int scsi_generic_readv( void *pNode, void *pCookie, off_t nPos, const struct iovec *psVector, size_t nCount );
int scsi_generic_writev( void *pNode, void *pCookie, off_t nPos, const struct iovec *psVector, size_t nCount );

#endif	/* SCSI_COMMON_H_ */

