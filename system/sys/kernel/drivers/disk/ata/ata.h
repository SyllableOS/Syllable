/*
 *  ATA/ATAPI driver for Syllable
 *
 *  Copyright (C) 2003 Kristian Van Der Vliet
 *  Copyright (C) 2002 William Rose
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

#ifndef __ATA_H_
#define __ATA_H_

#include <posix/errno.h>
#include <posix/stat.h>
#include <posix/fcntl.h>
#include <posix/dirent.h>

#include <atheos/kernel.h>
#include <atheos/kdebug.h>
#include <atheos/device.h>
#include <atheos/timer.h>
#include <atheos/semaphore.h>
#include <atheos/irq.h>
#include <atheos/isa_io.h>
#include <atheos/bootmodules.h>
#include <atheos/udelay.h>
#include <atheos/pci.h>
#include <atheos/cdrom.h>

#include <macros.h>

/* Information flags */
#define DIF_DMA_BND_ERRORS_HANDLED	0x01 /* DMA boundary errors handled transparantly. */
#define DIF_CSH_INFO_VALID			0x02 /* CHS information is valid. */
#define DIF_REMOVABLE				0x04 /* Removable drive. */
#define DIF_WRITE_VERIFY_SUPPORTED	0x08 /* Write with verify supported. */
#define DIF_HAS_CHANGE_LINE			0x10 /* Drive has change-line support. (Removable only) */
#define DIF_CAN_LOCK				0x20 /* Drive can be locked. (Removable only) */
#define DIF_CHS_MAXED_OUT			0x40 /* CHS info set to maximum supported values, not current media. */

#if __ENABLE_CSH_
	#define DIF_USE_CSH				0x80 /* Should we use CSH or LBA addressing? */
#endif

#define ATA_DEFAULT_PRIMARY_BASE	0x1f0
#define ATA_DEFAULT_SECONDARY_BASE	0x170

#define ATA_DEFAULT_PRIMARY_IRQ	14
#define ATA_DEFAULT_SECONDARY_IRQ	15

#define MAX_CONTROLLERS	2
#define MAX_DRIVES	MAX_CONTROLLERS * 2

struct ata_controllers_s{
	int io_base;
	int dma_base;
	int irq;
	int state;
	sem_id buf_lock;
	sem_id irq_lock;
	int dma_active;
	uint8 *raw_data_buffer;
	uint8 *data_buffer;
	uint8 *raw_dma_buffer;
	uint8 *dma_buffer;
};

typedef struct ata_controllers_s ata_controllers_t;

/* States */
#define IDLE		0
#define DEV_TIMEOUT	1
#define BUSY		2
#define ERROR		3

#define DEVICE_NONE		0
#define DEVICE_UNKNOWN	1
#define DEVICE_ATAPI	2
#define DEVICE_ATA		3

/* ATA Read/Write Registers */
#define ATA_DATA			0	/* data register */
#define ATA_PRECOMP			1	/* start of write precompensation */
#define ATA_COUNT			2	/* sectors to transfer */
#define ATA_SECTOR			3	/* sector number */
#define ATA_CYL_LO			4	/* low byte of cylinder number */
#define ATA_CYL_HI			5	/* high byte of cylinder number */
#define ATA_LDH				6	/* lba, drive and head */
#define   LDH_DEFAULT	0xA0	/* ECC enable, 512 bytes per sector */
#define   LDH_LBA		0x40	/* Use LBA addressing */

/* ATA Read-Only Registers */
#define ATA_STATUS		7	/* status */
#define	  STATUS_BSY		0x80	/* controller busy */
#define	  STATUS_RDY		0x40	/* drive ready */
#define	  STATUS_WF			0x20	/* write fault */
#define	  STATUS_SC			0x10	/* seek complete (obsolete) */
#define	  STATUS_DRQ		0x08	/* data transfer request */
#define	  STATUS_CRD		0x04	/* corrected data */
#define	  STATUS_IDX		0x02	/* index pulse */
#define	  STATUS_ERR		0x01	/* error */

#define ATA_ERROR		1	/* error code */
#define	  ERROR_BB			0x80	/* bad block */
#define	  ERROR_ECC			0x40	/* bad ecc bytes */
#define	  ERROR_ID			0x10	/* id not found */
#define	  ERROR_AC			0x04	/* aborted command */
#define	  ERROR_TK			0x02	/* track zero error */
#define	  ERROR_DM			0x01	/* no data address mark */

/* ATA Write-Only Registers */
#define ATA_COMMAND				7		/* command */
#define   CMD_IDLE				0x00	/* for w_command: drive idle */
#define   CMD_RECALIBRATE		0x10	/* recalibrate drive */
#define   CMD_READ				0x20	/* read data */
#define   CMD_WRITE				0x30	/* write data */
#define   CMD_READVERIFY		0x40	/* read verify */
#define   CMD_FORMAT			0x50	/* format track */
#define   CMD_SEEK				0x70	/* seek cylinder */
#define   CMD_DIAG				0x90	/* execute device diagnostics */
#define   CMD_SPECIFY			0x91	/* specify parameters */
#define   CMD_IDENTIFY			0xEC	/* identify drive */
#define   CMD_READ_MULTIPLE		0xc4	/* Read multiple sectors */
#define   CMD_WRITE_MULTIPLE	0xc5	/* Write multiple sectors */
#define   CMD_READ_DMA			0xC8
#define   CMD_WRITE_DMA			0xCA
 
#define   CMD_ATAPI_RESET		0x08	/* Reset an ATAPI device */
#define   CMD_ATAPI_PACKET		0xA0	/* ATAPI packet command */
#define   CMD_ATAPI_IDENTIFY	0xA1	/* ATAPI identify drive */

#define ATA_CTL					0x206	/* control register (Offset from base1)*/
#define	  CTL_NORETRY			0x80	/* disable access retry */
#define	  CTL_NOECC				0x40	/* disable ecc retry */
#define	  CTL_EIGHTHEADS		0x08	/* more than eight heads */
#define	  CTL_RESET				0x04	/* reset controller */
#define	  CTL_INTDISABLE		0x02	/* disable interrupts */

#define REG_ALT_STATUS			0x206	/* alternative status register */

/* ATAPI commands & register definitions */
#define	ATAPI_FEAT			1   /* features */
#define		FEAT_OVERLAP	0x02    /* overlap */
#define		FEAT_DMA		0x01    /* dma */

#define	ATAPI_IRR			2   /* interrupt reason register */
#define		IRR_REL			0x04    /* release */
#define		IRR_IO			0x02    /* direction for xfer */
#define		IRR_COD			0x01    /* command or data */

#define	ATAPI_SAMTAG		3
#define	ATAPI_CNT_LO		4   /* low byte of cylinder number */
#define	ATAPI_CNT_HI		5   /* high byte of cylinder number */
#define	ATAPI_DRIVE			6   /* drive select */

#define	ATAPI_STATUS		7   /* status */
#define		STATUS_BSY		0x80    /* controller busy */
#define		STATUS_DRDY		0x40    /* drive ready */
#define		STATUS_DMADF	0x20    /* dma ready/drive fault */
#define		STATUS_SRVCDSC 	0x10    /* service or dsc */
#define		STATUS_DRQ		0x08    /* data transfer request */
#define		STATUS_CORR		0x04    /* correctable error occurred */
#define		STATUS_CHECK	0x01    /* check error */

#define TIMEOUT					315000	/* The controller has ~31msecs to respond */
#define DELAY					500		/* Amount of time to wait for a command to complete */
#define MiB						1000000	/* 1 MiB (Metric MiB, Not 1 Megabyte!) */

#define SECTOR_BITS 			9
#define SECTORS_PER_FRAME		(CD_FRAMESIZE >> SECTOR_BITS)

#define ATA_BUFFER_SIZE			65536

typedef struct
{
	int    nDeviceHandle;
    uint16 nStructSize;
    uint16 nFlags;
    uint32 nCylinders;
    uint32 nHeads;
    uint32 nSectors;
    uint64 nTotSectors;
    uint16 nBytesPerSector;
} DriveParams_s;

typedef struct _AtaInode AtaInode_s;
struct _AtaInode
{
    AtaInode_s*	bi_psFirstPartition;
    AtaInode_s*	bi_psNext;
    int		bi_nDeviceHandle;
    char	bi_zName[16];
    int		bi_nOpenCount;
    int		bi_nDriveNum;	/* The drive number */
    bool		bi_bDMA;
    int		bi_nNodeHandle;
    int		bi_nPartitionType;
    int		bi_nSectorSize;
    int		bi_nSectors;
    int		bi_nCylinders;
    int		bi_nHeads;
	int		bi_nController;
    off_t	bi_nStart;
    off_t	bi_nSize;
    bool	bi_bRemovable;
    bool	bi_bRemoved;
    bool	bi_bLockable;
    bool	bi_bLocked;
    bool	bi_bHasChangeLine;
    bool	bi_bTruncateToCyl;
#if __ENABLE_CSH_
	bool	bi_bCSHAddressing;
#endif
};

/* Data returned by an Identify Drive command */
struct ata_identify_info_s
{
	uint16 configuration;
	uint16 cylinders;
	uint16 _reserved01;
	uint16 heads;
	uint16 track_bytes;

	uint16 sector_bytes;
	uint16 sectors;
	uint16 _reserved03[3];

	uint8 serial_number[20];
	uint16 buf_type;
	uint16 buf_size;
	uint16 ecc_bytes;
	uint8 revision[8];

	uint8 model_id[40];
	uint8 sectors_per_rw_long;
	uint8 _reserved05;
	uint16 _reserved06;
	uint8 _reserved07;

	uint8 capabilities;
	uint16 _reserved08;
	uint8 _reserved09;
	uint8 pio_cycle_time;
	uint8 _reserved10;

	uint8 dma;
	uint16 valid;
	uint16 current_cylinders;
	uint16 current_heads;
	uint16 current_sectors;

	uint16 current_capacity0;			/* Multiply by 512 to get size in bytes */
	uint16 current_capacity1;
	uint8 sectors_per_rw_irq;
	uint8 sectors_per_rw_irq_valid;
	uint32 lba_sectors;				/* Maximum number of sectors that can be addressed with LBA */

	uint16 single_word_dma_info;
	uint16 multi_word_dma_info;
	uint16 eide_pio_modes;
	uint16 eide_dma_min;
	uint16 eide_dma_time;

	uint16 eide_pio;
	uint16 eide_pio_iordy;
	uint16 _reserved13[2];
	uint16 _reserved14[4];

	uint16 command_queue_depth;		/* Command queue depth */
	uint16 _reserved15[4];
	uint16 major;
	uint16 minor;
	uint16 command_set_1;			/* Bit field of command set support */
	uint16 command_set_2;

	uint16 command_set_features_extensions;
	uint16 command_set_features_enable_1;
	uint16 command_set_features_enable_2;
	uint16 command_set_features_default;	/* Command set/features default */
	uint16 ultra_dma_modes;			/* Ultra DMA Mode support */

	uint16 _reserved16[2];
	uint16 advanced_power_management;	/* Advanced power management value */
	uint16 _reserved17;
	uint16 hardware_config;			/* Results of hardware reset */

	uint16 _reserved18[32];
	uint16 last_lun;
	uint16 _reserved19;
	uint16 device_lock_functions;
	uint16 current_set_features_options;

	uint16 _reserved20[26];
	uint16 _reserved21;
	uint16 _reserved22[3];
	uint16 _reserved23[95];
};

typedef struct ata_identify_info_s ata_identify_info_t;

/* Strings from the drive are byte swapped */
static inline void byte_swap( char* string, int len )
{
	char swap;
	int i;

	for( i=0; i<len;i+=2)
	{
		swap=string[i];
		string[i]=string[i+1];
		string[i+1]=swap;
	}
}

/* Cleanup the model I.D string */
static inline void extract_model_id( char *buffer, char *string )
{
	int i;

	byte_swap( string, 40 );

	for( i = 39; i >= 0; i-- )
		if( string[i] != ' ' )
			break;

	strncpy( buffer, string, i+1 );
	buffer[i+1] = 0;
}

#define get_controller( drive )	(drive / 2)	/* Each controller can have 2 drives */
#define get_drive( drive )		(drive & 0x1)	/* Odd numbered drives are always slaves */

/* I/O */
#define ata_inb( reg )		inb( g_nControllers[controller].io_base + reg )
#define ata_inw( reg )		inw( g_nControllers[controller].io_base + reg )

#define ata_outb( reg, value )	outb( value, g_nControllers[controller].io_base + reg )
#define ata_outw( reg, value )	outw( value, g_nControllers[controller].io_base + reg )

#endif

