/*
 *  The Syllable kernel
 *  Copyright (C) 2003 Kristian Van Der Vliet
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

#ifndef __F_SYLLABLE_CDROM_H__
#define __F_SYLLABLE_CDROM_H__

#include <atheos/types.h>
#include <atheos/device.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MKCDIOCTL(x)	IOCTL_USER+x

/* Audio control */
#define CD_PLAY_MSF			MKCDIOCTL(0x1001)	/* Play Audio MSF */
#define CD_PLAY_LBA			MKCDIOCTL(0x1002)	/* Play Audio LBA */
#define CD_PLAY_TRACK		MKCDIOCTL(0x1003)	/* Play Audio Track */
#define CD_PAUSE			MKCDIOCTL(0x1004)	/* Pause Audio */ 
#define CD_RESUME			MKCDIOCTL(0x1005)	/* Resume paused Audio */
#define CD_STOP				MKCDIOCTL(0x1006)	/* Stop Audio playback */
#define CD_GET_TIME			MKCDIOCTL(0x1007)	/* Get current playback time & position */

/* Drive controls */
#define CD_EJECT			MKCDIOCTL(0x2001)	/* Open / Close the CD-ROM tray */

/* TOC/Track management */
#define CD_READ_TOC			MKCDIOCTL(0x3001)	/* Read the Table of Contents */
#define CD_READ_TOC_ENTRY	MKCDIOCTL(0x3002)	/* Read an entry of the Table of Contents */

/* CD-DA */
#define CD_READ_CDDA		MKCDIOCTL(0x4001)	/* Read a single CD-DA block */

/* Direct packet interface (E.g. cdrecord) */
#define CD_PACKET_COMMAND	MKCDIOCTL(0x5001)	/* Send a raw packet command to the drive */

/* Disc specific */

/* The leadout track is always 0xAA, regardless of # of tracks on disc */
#define	CDROM_LEADOUT		0xAA

/* Audio discs */
#define CD_FRAMESIZE	2048	/* Sector size for Mode 1 & 2 discs */
#define CD_SECS              60	/* seconds per minute */
#define CD_FRAMES            75	/* frames per second */
#define CD_MSF_OFFSET       150 /* MSF numbering offset of first frame */

#define CD_CDDA_FRAMESIZE	2352

static inline void lba_to_msf(int lba, uint8 *m, uint8 *s, uint8 *f)
{
	lba += CD_MSF_OFFSET;
	lba &= 0xffffff;  /* negative lbas use only 24 bits */
	*m = lba / (CD_SECS * CD_FRAMES);
	lba %= (CD_SECS * CD_FRAMES);
	*s = lba / CD_FRAMES;
	*f = lba % CD_FRAMES;
}

static inline int msf_to_lba(uint8 m, uint8 s, uint8 f)
{
	return (((m * CD_SECS) + s) * CD_FRAMES + f) - CD_MSF_OFFSET;
}

/* CD-ROM Table Of Contents and Audio Track structures */
struct cdrom_toc_header
{
	unsigned short toc_length;
	uint8 first_track;
	uint8 last_track;
};

typedef struct cdrom_toc_header cdrom_toc_header_s;

/* Structure of a MSF cdrom address. */
struct cdrom_msf
{
	uint8 reserved;
	uint8 minute;
	uint8 second;
	uint8 frame;
};

typedef struct cdrom_msf cdrom_msf_s;

struct cdrom_toc_entry
{
	uint8 reserved1;
	uint8 control : 4;
	uint8 adr     : 4;
	uint8 track;
	uint8 reserved2;
	union
	{
		unsigned lba;
		cdrom_msf_s msf;
	} addr;
};

typedef struct cdrom_toc_entry cdrom_toc_entry_s;

#define MAX_TRACKS 99
struct cdrom_toc
{
	int    last_session_lba;
	int    xa_flag;
	unsigned long capacity;
	cdrom_toc_header_s hdr;
	cdrom_toc_entry_s  ent[MAX_TRACKS+1];		/* One extra track for the leadout. */
};

typedef struct cdrom_toc cdrom_toc_s;

struct cdrom_audio_track
{
	uint8 track;
	cdrom_msf_s msf_start;
	cdrom_msf_s msf_stop;
	uint32 lba_start;
	uint32 lba_stop;
};

typedef struct cdrom_audio_track cdrom_audio_track_s;

struct cdda_block
{
	uint32 nBlock;
	uint8 *pBuf;
	uint32 nSize;
};

struct cdrom_packet_cmd
{
	uint8 nCommand[12];
	int nCommandLength;
	uint8 *pnData;
	int nDataLength;
	uint8 *pnSense;
	int nSenseLength;
	unsigned int nDirection;
	unsigned int nFlags;
	uint8 nSense;
	uint8 nError;
};

typedef struct cdrom_packet_cmd cdrom_packet_cmd_s;

enum direction
{
	NO_DATA = 0,
	WRITE,	/* TO device */
	READ	/* FROM device */
};

/* Return values from atapi_check_sense() */
enum check_sense
{
	SENSE_OK,
	SENSE_RETRY,
	SENSE_FATAL
};

#ifdef __cplusplus
}
#endif

#endif	/* __F_SYLLABLE_CDROM_H__ */

