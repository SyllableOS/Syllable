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

#ifndef __ATA_IO_H_
#define __ATA_IO_H_

#include "ata.h"
#include "atapi-drive.h"

int get_data( int controller, int bytes, void *buffer );
int put_data( int controller, int bytes, void *buffer );

int read_sectors( AtaInode_s* psInode, void* pBuffer, int64 nSector, int nSectorCount );
int write_sectors( AtaInode_s* psInode, const void* pBuffer, off_t nSector, int nSectorCount );

void timeout( void* data );
int wait_for_status( int controller, int mask, int value );

void select_drive( int nDrive );

void ata_drive_reset( int controller );

int atapi_reset( int drive );
int atapi_packet_command( AtapiInode_s *psInode, atapi_packet_s *command );

#endif

