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

#ifndef __ATAPI_DRIVE_H_
#define __ATAPI_DRIVE_H_

#include "ata.h"
#include <atheos/cdrom.h>

/* ATAPI devices store more data than ATA devices, so they have their own inode struct */
typedef struct _AtapiInode AtapiInode_s;
struct _AtapiInode
{
	AtapiInode_s*	bi_psFirstPartition;
	AtapiInode_s*	bi_psNext;
	int				bi_nDeviceHandle;
	char			bi_zName[16];
	bool			bi_bDMA;
	int				bi_nOpenCount;
	int				bi_nDriveNum;	/* The drive number */
	int				bi_nNodeHandle;
	int				bi_nController;
	off_t			bi_nStart;
	off_t			bi_nSize;
	bool			bi_bRemovable;
	bool			bi_bLockable;
	bool			bi_bMediaChanged;
	bool			bi_bTocValid;
	cdrom_toc_s		toc;
};

/* ATAPI packet data */
struct request_sense
{
	uint8 error_code	: 7;
	uint8 valid			: 1;
	uint8 segment_number;
	uint8 sense_key		: 4;
	uint8 reserved2		: 1;
	uint8 ili			: 1;
	uint8 reserved1		: 2;
	uint8 information[4];
	uint8 add_sense_len;
	uint8 command_info[4];
	uint8 asc;
	uint8 ascq;
	uint8 fruc;
	uint8 sks[3];
	uint8 asb[46];
};

typedef struct request_sense request_sense_s;

struct atapi_packet
{
	uint8 packet[12];
	int status;
	int error;
	int count;
	int sense_key;
	request_sense_s sense;
};

typedef struct atapi_packet atapi_packet_s;

/* ATAPI CD/DVD-ROM specific */
status_t cdrom_test_unit_ready( AtapiInode_s *psInode );
status_t cdrom_wait_for_unit_ready( AtapiInode_s *psInode );
status_t cdrom_eject_media( AtapiInode_s *psInode );
status_t cdrom_read_toc_entry( AtapiInode_s *psInode, int trackno, int msf_flag, int format, char *buf, int buflen );
status_t cdrom_read_toc( AtapiInode_s *psInode, cdrom_toc_s *toc );
status_t cdrom_read_capacity(AtapiInode_s *psInode, unsigned long *capacity );
status_t cdrom_play_audio(AtapiInode_s *psInode, int lba_start, int lba_end );
status_t cdrom_pause_resume_audio(AtapiInode_s *psInode, bool bPause );
status_t cdrom_stop_audio( AtapiInode_s *psInode );
status_t cdrom_get_playback_time( AtapiInode_s *psInode, cdrom_msf_s *pnTime );
status_t cdrom_saw_media_change(AtapiInode_s *psInode );

/* Device function prototypes */
int atapi_open( void* pNode, uint32 nFlags, void **ppCookie );
int atapi_close( void* pNode, void* pCookie );
int atapi_read( void* pNode, void* pCookie, off_t nPos, void* pBuf, size_t nLen );
int atapi_readv( void* pNode, void* pCookie, off_t nPos, const struct iovec* psVector, size_t nCount );
status_t atapi_ioctl( void* pNode, void* pCookie, uint32 nCommand, void* pArgs, bool bFromKernel );

/* Internal general ATAPI command */
int atapi_request_sense( AtapiInode_s *psInode, request_sense_s *psSense );

/* Some data is in Big Endian format (Although luckily its not as bad as
   an ISO9660 disc, which is double-encoded!) */
static inline uint32 be32_to_cpu(uint32 be_val)
{
	return( ( be_val & 0xff000000 ) >> 24 |
		    ( be_val & 0x00ff0000 ) >> 8 |
		    ( be_val & 0x0000ff00 ) << 8 |
		    ( be_val & 0x000000ff ) << 24 );
}

/* ATAPI Packet Commands */
#define GPCMD_PAUSE_RESUME			0x4b
#define GPCMD_PLAY_AUDIO_MSF	    0x47
#define GPCMD_READ_10			    0x28
#define GPCMD_READ_CDVD_CAPACITY	0x25
#define GPCMD_READ_SUB_CHANNEL		0x42
#define GPCMD_READ_TOC_PMA_ATIP		0x43
#define GPCMD_STOP_AUDIO			0x4e
#define GPCMD_REQUEST_SENSE		    0x03
#define GPCMD_START_STOP_UNIT		0x1b
#define GPCMD_TEST_UNIT_READY	    0x00

/* Sense key data */
#define NO_SENSE                0x00
#define RECOVERED_ERROR         0x01
#define NOT_READY               0x02
#define MEDIUM_ERROR            0x03
#define HARDWARE_ERROR          0x04
#define ILLEGAL_REQUEST         0x05
#define UNIT_ATTENTION          0x06
#define DATA_PROTECT            0x07
#define ABORTED_COMMAND         0x0b
#define MISCOMPARE              0x0e

/* Additional sense codes */
#define NO_ASC_DATA				0x00
#define UNIT_NOT_SUPPORTED		0x01
#define NOT_RESPONDING			0x02
#define MEDIUM_NOT_PRESENT		0x03
#define NOT_REPORTABLE			0x04
#define BECOMING_READY			0x05
#define MUST_INITIALIZE			0x06
#define MANUAL_INTERVENTION		0x07
#define FORMAT_IN_PROGRESS		0x08

#define ATAPI_CMD_TIMEOUT	TIMEOUT*2
#define ATAPI_SPIN_TIMEOUT	31000000		/* An ATAPI device gets ~10 seconds to spin up */
#endif

