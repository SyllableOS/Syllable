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

#ifndef __ATA_PROBE_H_
#define __ATA_PROBE_H_

void ata_detect_pci_controllers( void );
void ata_init_controllers( void );

int get_bios_parameters( int nDrive, DriveParams_s* psParams );
int get_drive_params( int nDrive, DriveParams_s* psParams );

size_t ata_read_partition_data( void* pCookie, off_t nOffset, void* pBuffer, size_t nSize );
int ata_decode_partitions( AtaInode_s* psInode );
int ata_create_node( int nDevID, const char* pzPath, const char* pzHDName, int nDriveNum, int nSec, int nCyl, int nHead, int nSecSize, off_t nStart, off_t nSize, bool bCSH );
int ata_scan_for_disks( int nDeviceID );

#endif

