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

#ifndef SCSI_COMMON_H_
#define SCSI_COMMON_H_

#include <atheos/types.h>
#include <atheos/scsi.h>

extern SCSI_device_s *g_psFirstDevice;

static inline uint32 be32_to_cpu(uint32 be_val)
{
	return( ( be_val & 0xff000000 ) >> 24 |
		    ( be_val & 0x00ff0000 ) >> 8 |
		    ( be_val & 0x0000ff00 ) << 8 |
		    ( be_val & 0x000000ff ) << 24 );
}

static inline void scsi_init_cmd( SCSI_cmd_s *psCmd, SCSI_device_s *psDevice )
{
	if( NULL == psCmd || NULL == psDevice )
		return;

	/* Clear everything, including the command and sense data */
	memset( psCmd, 0, sizeof( *psCmd ) );

	psCmd->psHost = psDevice->psHost;
	psCmd->nChannel = psDevice->nChannel;
	psCmd->nDevice = psDevice->nDevice;
	psCmd->nLun = psDevice->nLun;

	/* Supply default values.  The caller can override these as they require */
	psCmd->nDirection = SCSI_DATA_NONE;
	psCmd->pRequestBuffer = psDevice->pDataBuffer;
	psCmd->nRequestSize = sizeof( psCmd->pRequestBuffer );
}

SCSI_device_s * scsi_add_disk( SCSI_host_s * psHost, int nChannel, int nDevice, int nLun, unsigned char nSCSIResult[] );
SCSI_device_s * scsi_add_cdrom( SCSI_host_s * psHost, int nChannel, int nDevice, int nLun, unsigned char nSCSIResult[] );

unsigned char scsi_get_command_size( int nOpcode );

status_t scsi_request_sense( SCSI_device_s* psDevice, SCSI_sense_s *psSense );
status_t scsi_check_sense( SCSI_device_s* psDevice, SCSI_sense_s *psSense, bool bQuiet );
status_t scsi_test_ready( SCSI_device_s* psDevice );
status_t scsi_read_capacity( SCSI_device_s* psDevice, unsigned long* pnCapacity );

int scsi_generic_read( void *pNode, void *pCookie, off_t nPos, void *pBuf, size_t nLen );
int scsi_generic_write( void *pNode, void *pCookie, off_t nPos, const void *pBuf, size_t nLen );
int scsi_generic_readv( void *pNode, void *pCookie, off_t nPos, const struct iovec *psVector, size_t nCount );
int scsi_generic_writev( void *pNode, void *pCookie, off_t nPos, const struct iovec *psVector, size_t nCount );


#endif	/* SCSI_COMMON_H_ */

