/*
 *  ATA/ATAPI driver for Syllable
 *
 *  Copyright (C) 2004 Kristian Van Der Vliet
 *  Copyright (C) 2004 Arno Klenke
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

#ifndef __ATAPI_H_
#define __ATAPI_H_

#include "ata.h"

/* ATAPI commands */
#define ATAPI_PAUSE_RESUME			0x4b
#define ATAPI_PLAY_AUDIO_MSF	    0x47
#define ATAPI_READ_10			    0x28
#define ATAPI_READ_CDVD_CAPACITY	0x25
#define ATAPI_READ_SUB_CHANNEL		0x42
#define ATAPI_READ_TOC_PMA_ATIP		0x43
#define ATAPI_STOP_AUDIO			0x4e
#define ATAPI_REQUEST_SENSE		    0x03
#define ATAPI_START_STOP_UNIT		0x1b
#define ATAPI_TEST_UNIT_READY	    0x00
#define ATAPI_READ_CD				0xbe
#define ATAPI_WRITE_10				0x2a
#define ATAPI_XPWRITE_10			0x51
#define ATAPI_READ_CD_12			0xd8

/* Sense keys */
#define ATAPI_NO_SENSE                0x00
#define ATAPI_RECOVERED_ERROR         0x01
#define ATAPI_NOT_READY               0x02
#define ATAPI_MEDIUM_ERROR            0x03
#define ATAPI_HARDWARE_ERROR          0x04
#define ATAPI_ILLEGAL_REQUEST         0x05
#define ATAPI_UNIT_ATTENTION          0x06
#define ATAPI_DATA_PROTECT            0x07
#define ATAPI_BLANK_CHECK             0x08
#define ATAPI_VENDOR_SPECIFIC         0x09
#define ATAPI_COPY_ABORTED            0x0a
#define ATAPI_ABORTED_COMMAND         0x0b
#define ATAPI_VOLUME_OVERFLOW         0x0d
#define ATAPI_MISCOMPARE              0x0e

/* Additional sense codes */
#define ATAPI_NO_ASC_DATA				0x00
#define ATAPI_LOGICAL_UNIT_NOT_READY	0x04
	#define ATAPI_NOT_REPORTABLE			0x00
	#define ATAPI_BECOMING_READY			0x01
	#define ATAPI_MUST_INITIALIZE			0x02
	#define ATAPI_MANUAL_INTERVENTION		0x03
	#define ATAPI_FORMAT_IN_PROGRESS		0x04
	#define ATAPI_REBUILD_IN_PROGRESS		0x05
	#define ATAPI_RECALC_IN_PROGRESS		0x06
	#define ATAPI_OP_IN_PROGRESS			0x07
	#define ATAPI_LONG_WRITE_IN_PROGRESS	0x08
	#define ATAPI_SELF_TEST_IN_PROGRESS		0x09
	#define ATAPI_ASSYM_ACCESS_STATE_TRANS	0x0a
	#define ATAPI_TARGET_PORT_STANDBY		0x0b
	#define ATAPI_TARGET_PORT_UNAVAILABLE	0x0c
	#define ATAPI_AUX_MEM_UNAVAILABLE		0x10
	#define ATAPI_NOTIFY_REQUIRED			0x11
#define ATAPI_NOT_RESPONDING			0x05
#define ATAPI_MEDIUM					0x3a
	#define ATAPI_MEDIUM_NOT_PRESENT		0x00
	#define ATAPI_MEDIUM_TRAY_CLOSED		0x01
	#define	ATAPI_MEDIUM_TRAY_OPEN			0x02
	#define	ATAPI_MEDIUM_LOADABLE			0x03

/* ATAPI (SCSI) device classes */
#define ATAPI_CLASS_DAD				0x00	/* Direct Access Device (Disk) */
#define ATAPI_CLASS_SEQUENTIAL		0x01	/* Sequential-access device (Tape) */
#define ATAPI_CLASS_PRINTER			0x02	/* Printer */
#define ATAPI_CLASS_PROC			0x03	/* Processor */
#define ATAPI_CLASS_WORM			0x04	/* Write-Once device */
#define ATAPI_CLASS_CDVD			0x05	/* CD-ROM/DVD-ROM */
#define ATAPI_CLASS_SCANNER			0x06	/* Optical scanner */
#define ATAPI_CLASS_OPTI_MEM		0x07	/* Optical memory device */
#define ATAPI_CLASS_CHANGER			0x08	/* Medium changer (E.g. a tape robot) */
#define ATAPI_CLASS_COMMS			0x09	/* Communications device */
#define ATAPI_CLASS_ARRAY			0x0c	/* Array Controller E.g. RAID */
#define ATAPI_CLASS_ENCLOSURE		0x0d	/* Enclosure services device E.g. fan */
#define ATAPI_CLASS_REDUCED_BLK		0x0e	/* Reduced block command device */
#define ATAPI_CLASS_OPTI_CARD		0x0f	/* Optical card reader/writer */
#define ATAPI_CLASS_RESERVED		0x1f	/* Reserved/Unknown device */

/* Functions used by higher ATAPI layers */
status_t atapi_check_sense( ATAPI_device_s* psDev, ATAPI_sense_s *psSense, bool bQuiet );
status_t atapi_unit_not_ready( ATAPI_device_s* psDev, ATAPI_sense_s *psSense );
int atapi_drive_request_sense( ATAPI_device_s* psDev, ATAPI_sense_s *psSense );
status_t atapi_read_capacity( ATAPI_device_s* psDev, unsigned long* pnCapacity );
status_t atapi_test_ready( ATAPI_device_s* psDev );
status_t atapi_start( ATAPI_device_s* psDev );
status_t atapi_eject( ATAPI_device_s* psDev );
status_t atapi_read_toc_entry( ATAPI_device_s* psDev, int trackno, int msf_flag, int format, char *buf, int buflen);
status_t atapi_read_toc( ATAPI_device_s* psDev, cdrom_toc_s *toc );
status_t atapi_play_audio( ATAPI_device_s* psDev, int lba_start, int lba_end);
status_t atapi_pause_resume_audio( ATAPI_device_s* psDev, bool bPause );
status_t atapi_stop_audio( ATAPI_device_s *psDev );
status_t atapi_get_playback_time( ATAPI_device_s *psDev, cdrom_msf_s *pnTime );
status_t atapi_do_read( ATAPI_device_s *psDev, uint64 nBlock, int nSize );
status_t atapi_do_cdda_read( ATAPI_device_s *psDev, uint64 nBlock, int nSize );
status_t atapi_do_packet_command( ATAPI_device_s *psDev, cdrom_packet_cmd_s *psCmd );

/* Inline functions */
static inline uint32 be32_to_cpu(uint32 be_val)
{
	return( ( be_val & 0xff000000 ) >> 24 |
		    ( be_val & 0x00ff0000 ) >> 8 |
		    ( be_val & 0x0000ff00 ) << 8 |
		    ( be_val & 0x000000ff ) << 24 );
}

#endif

