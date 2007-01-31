/*
 *  ATA/ATAPI driver for Syllable
 *
 *  Copyright (C) 2004 Arno Klenke
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

#ifndef DEBUG_LIMIT
#define DEBUG_LIMIT KERN_INFO
#endif

#include <posix/errno.h>
#include <posix/stat.h>
#include <posix/fcntl.h>
#include <posix/dirent.h>

#include <atheos/types.h>
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

#include "ata_regs.h"
#include "atapi_regs.h"

#define ATA_BUS_NAME "ata"
#define ATA_BUS_VERSION 1
#define ATA_MAX_PORTS 8

/* Types */
enum ATA_type
{
	ATA_PATA,	/* parallel ata */
	ATA_SATA	/* serial ata */
};

/* Cables */
enum ATA_cable
{
	ATA_CABLE_NONE,		/* no cable connected */
	ATA_CABLE_UNKNOWN,	/* unknown cable */
	ATA_CABLE_PATA40,	/* 40pin cable */
	ATA_CABLE_PATA80,	/* 80pin cable */
	ATA_CABLE_SATA		/* sata cable */
};

/* Devices */
enum ATA_device
{
	ATA_DEV_NONE,
	ATA_DEV_UNKNOWN,
	ATA_DEV_ATA,
	ATA_DEV_ATAPI
};

/* Controller speed */
enum ATA_speed
{
	ATA_SPEED_PIO,
	ATA_SPEED_PIO_3,
	ATA_SPEED_PIO_4,
	ATA_SPEED_PIO_5,
	ATA_SPEED_DMA,
	ATA_SPEED_MWDMA_0,
	ATA_SPEED_MWDMA_1,
	ATA_SPEED_MWDMA_2,
	ATA_SPEED_UDMA_0,
	ATA_SPEED_UDMA_1,
	ATA_SPEED_UDMA_2,
	ATA_SPEED_UDMA_3,
	ATA_SPEED_UDMA_4,
	ATA_SPEED_UDMA_5,
	ATA_SPEED_UDMA_6,
	ATA_SPEED_UDMA_7,
	ATA_SPEED_HIGHEST = ATA_SPEED_UDMA_7
};

/* Drive ID */
typedef struct
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

	uint8 capabilities; /* 0x1 SWDMA, 0x2 LBA */
	uint16 _reserved08;
	uint8 _reserved09;
	uint8 pio_cycle_time;
	uint8 _reserved10;

	uint8 dma;
	uint16 valid; /* 0x2 EIDE, 0x4 ULTRA */
	uint16 current_cylinders;
	uint16 current_heads;
	uint16 current_sectors;

	uint16 current_capacity0;			/* Multiply by 512 to get size in bytes */
	uint16 current_capacity1;
	uint8 sectors_per_rw_irq;
	uint8 sectors_per_rw_irq_valid;
	uint32 lba_sectors;				/* Maximum number of sectors that can be addressed with LBA */

	uint16 single_word_dma_info;
	uint16 multi_word_dma_info; /* 0x1 MWDMA0 0x2 MWDMA1 0x4 MWDMA 2 */
	uint16 eide_pio_modes; /* 0x1 PIO3 0x2 PIO4 0x4 PIO5 */
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
	uint16 command_set_2; /* 0x400 48 Bit addressing */

	uint16 command_set_features_extensions;
	uint16 command_set_features_enable_1;
	uint16 command_set_features_enable_2; /* 0x400 48 Bit addressing */
	uint16 command_set_features_default;	/* Command set/features default */
	uint16 ultra_dma_modes;			/* Ultra DMA Mode support */

	uint16 _reserved16[2];
	uint16 advanced_power_management;	/* Advanced power management value */
	uint16 _reserved17;
	uint16 hardware_config;			/* Results of hardware reset */
	
	uint16 acoustic;
	uint16 _reserved25[5];
	
	uint64 lba_capacity_48;
	
	uint16 _reserved18[22];
	uint16 last_lun;
	uint16 _reserved19;
	uint16 device_lock_functions;
	uint16 current_set_features_options;

	uint16 _reserved20[26];
	uint16 _reserved21;
	uint16 _reserved22[3];
	uint16 _reserved23[95];
} ATA_identify_info_s;


struct ATA_port_t;
struct ATA_cmd_t;

/* ATA port operations */
typedef struct
{
	status_t ( *reset )( struct ATA_port_t* psPort ); /* Reset port (required) */
	void ( *identify )( struct ATA_port_t* psPort ); /* Identify device (optional) */
	status_t ( *configure )( struct ATA_port_t* psPort ); /* Configure port (optional) */
	void ( *select )( struct ATA_port_t* psPort, uint8 nAdd ); /* Select port (optional) */
	uint8 ( *status )( struct ATA_port_t* psPort ); /* Read status (optional) */
	uint8 ( *altstatus )( struct ATA_port_t* psPort ); /* Read alternate status (optional) */
	
	status_t ( *prepare_dma_read )( struct ATA_port_t* psPort, uint8* pBuffer, uint32 nLen ); /* (optional) */
	status_t ( *prepare_dma_write )( struct ATA_port_t* psPort, uint8* pBuffer, uint32 nLen ); /* (optional) */
	status_t ( *start_dma )( struct ATA_port_t* psPort ); /* (optional) */
	
	void ( *ata_cmd_ata )( struct ATA_cmd_t* psCmd ); /* Process ata command (optional) */
	void ( *ata_cmd_atapi )( struct ATA_cmd_t* psCmd ); /* Process atapi command (optional) */
	
} ATA_port_ops_s;

/* ATA command */
struct ATA_cmd_t
{
	struct ATA_cmd_t* psNext;
	struct ATA_cmd_t* psPrev;
	
	struct ATA_port_t* psPort;
	uint32 nFlags;
	uint8 nCmd[14]; /* 12 entries for atapi, 8 entries + 5 48bit entries for ata */
	int nStatus;
	int nError;
	ATAPI_sense_s sSense; /* For atapi devices only */
	bool bCanDMA;	/* Indicate if the command is DMA'able */
	int nDirection;	/* Currently ATAPI only; indicate direction of data transfer */

	uint8* pTransferBuffer;
	uint32 nTransferLength;
	
	sem_id hWait;
};

typedef struct ATA_cmd_t ATA_cmd_s;

/* ATA command buffer */
typedef struct
{
	int nChannel; /* Controller channel */
	thread_id hThread; /* Command thread */
	sem_id hCount; /* Number of command in the buffer */
	sem_id hLock; /* buffer lock */
	ATA_cmd_s* psHead;
	ATA_cmd_s* psTail;
} ATA_cmd_buf_s;

struct ATA_controller_t;

/* ATA port ( controller/busmanager means which driver fills in the information */
struct ATA_port_t
{
	struct ATA_controller_t* psCtrl; /* Controller (busmanager) */
	
	int nID; /* ID (busmanager) */
	int nChannel; /* Channel (busmanager) */
	int nPort; /* Port (busmanager) */
	
	int nDeviceID; /* device id of the controller driver (busmanager) */
	ATA_port_ops_s sOps; /* operations (busmanager/controller) */
	
	bool bMMIO; /* Use MMIO instead of PIO (controller) */
	uint32 nRegs[ATA_TOTAL_REGS]; /* ata registers (controller) */
	uint32 nDmaRegs[ATA_DMA_TOTAL_REGS]; /* ata dma registers (controller) */
	uint32 nSATARegisters[SATA_TOTAL_REGS]; /* sata registers (controller) */
	
	int nType; /* ATA_type value (controller) */
	int nCable; /* ATA_cable value (controller) */
	int nDevice; /* ATA_device value (busmanager) */
	char zDeviceName[41]; /* ATA device name (busmanager) */
	int nClass;	/* ATAPI device class (busmanager) */
	
	bool bConfigured; /* device is configured (busmanager/controller) */
	
	int nSupportedPortSpeed; /* ATA_speed bitfield (controller) */
	int nSupportedDriveSpeed; /* ATA_speed bitfield (busmanager) */
	int nCurrentSpeed; /* ATA_speed value (busmanager/controller) */
	
	ATA_identify_info_s sID; /* identification (busmanager) */
	
	bool bPIO32bit; /* 32 bit PIO transfers (busmanager) */
	bool bLBA48bit; /* 48 bit addressing (busmanager) */
	
	uint8* pDataBuf; /* Data buffer (busmanager) */
	sem_id hPortLock; /* Port lock (controller) */
	sem_id hIRQWait; /* Interrupt wait (controller) */
	bool bIRQError; /* Interrupt error (controller) */
	uint8* pDMATable; /* DMA table buffer (controller) */
	
	ATA_cmd_buf_s* psCmdBuf; /* Command buffer */

	void* pPrivate; /* Additional private data (controller) */
};

typedef struct ATA_port_t ATA_port_s;

/* ATA controller */
struct ATA_controller_t
{
	struct ATA_controller_t* psNext;
	
	int nDeviceID;
	char zName[30];
	int nChannels;
	int nPortsPerChannel;
	
	ATA_port_s* psPort[ATA_MAX_PORTS];
};

typedef struct ATA_controller_t ATA_controller_s;


/* ATA device */
struct ATA_device_t
{
	struct ATA_device_t* psNext;
	struct ATA_device_t* psFirstPartition;
	
	ATA_port_s* 	psPort;
	sem_id			hLock;
	int 			nDeviceHandle;
	char 			zName[16];
	atomic_t			nOpenCount;
	int 			nNodeHandle;
	int				nPartitionType;
	int				nSectorSize;
	uint64			nSectors;
	off_t			nStart;
	off_t			nSize;
	bool			bRemovable;
};

typedef struct ATA_device_t ATA_device_s;

/* ATAPI device */
typedef struct
{
	ATA_port_s*		psPort;
	sem_id			hLock;
	int				nDeviceHandle;
	char			zName[16];
	atomic_t			nOpenCount;
	int				nNodeHandle;
	off_t			nStart;
	off_t			nSize;
	int				nSectorSize;
	bool			bRemovable;
	bool			bMediaChanged;
	bool			bTocValid;
	cdrom_toc_s		sToc;
} ATAPI_device_s;

/* ATA busmanager */
typedef struct
{
	ATA_controller_s* ( *alloc_controller )( int nDeviceID, int nChannels, int nPortsPerChannel );
	void( *free_controller )( ATA_controller_s* psCtrl );
	
	status_t ( *add_controller )( ATA_controller_s* psCtrl );
	void ( *remove_controller )( ATA_controller_s* psCtrl );
		
	ATA_port_s* ( *alloc_port )( ATA_controller_s* psCtrl );
	void ( *free_port )( ATA_port_s* psPort );
	
	uint32 ( *get_controller_count )();
	ATA_controller_s* ( *get_controller )( uint32 nCtrl );
} ATA_bus_s;

/* Macros */
#define ATA_READ_REG( psPort, nReg, nValue ) { if( psPort->bMMIO ) nValue = *((uint8*)psPort->nRegs[nReg]); \
												else nValue = inb( psPort->nRegs[nReg] ); }
#define ATA_WRITE_REG( psPort, nReg, nValue ) { if( psPort->bMMIO ) *((uint8*)psPort->nRegs[nReg]) = nValue; \
												else outb( nValue, psPort->nRegs[nReg] ); }
#define ATA_READ_REG16( psPort, nReg, nValue ) { if( psPort->bMMIO ) nValue = *((uint16*)psPort->nRegs[nReg]); \
												else nValue = inw( psPort->nRegs[nReg] ); }
#define ATA_WRITE_REG16( psPort, nReg, nValue ) { if( psPort->bMMIO ) *((uint16*)psPort->nRegs[nReg]) = nValue; \
												else outw( nValue, psPort->nRegs[nReg] ); }
#define ATA_READ_REG32( psPort, nReg, nValue ) { if( psPort->bMMIO ) nValue = *((uint32*)psPort->nRegs[nReg]); \
												else nValue = inl( psPort->nRegs[nReg] ); }
#define ATA_WRITE_REG32( psPort, nReg, nValue ) { if( psPort->bMMIO ) *((uint32*)psPort->nRegs[nReg]) = nValue; \
												else outl( nValue, psPort->nRegs[nReg] ); }
#define ATA_READ_DMA_REG( psPort, nReg, nValue ) { if( psPort->bMMIO ) nValue = *((uint8*)psPort->nDmaRegs[nReg]); \
												else nValue = inb( psPort->nDmaRegs[nReg] ); }
#define ATA_WRITE_DMA_REG( psPort, nReg, nValue ) { if( psPort->bMMIO ) *((uint8*)psPort->nDmaRegs[nReg]) = nValue; \
												else outb( nValue, psPort->nDmaRegs[nReg] ); }
#define ATA_READ_DMA_REG32( psPort, nReg, nValue ) { if( psPort->bMMIO ) nValue = *((uint32*)psPort->nDmaRegs[nReg]); \
												else nValue = inl( psPort->nDmaRegs[nReg] ); }
#define ATA_WRITE_DMA_REG32( psPort, nReg, nValue ) { if( psPort->bMMIO ) *((uint32*)psPort->nDmaRegs[nReg]) = nValue; \
												else outl( nValue, psPort->nDmaRegs[nReg] ); }
#define SATA_READ_REG32( sPort, nReg, nValue ) { if( psPort->bMMIO ) nValue = *((uint32*)psPort->nSATARegs[nReg]); \
												else nValue = inl( psPort->nSATARegs[nReg] ); }										
#define SATA_WRITE_REG32( psPort, nReg, nValue ) { if( psPort->bMMIO ) *((uint32*)psPort->nSATARegs[nReg]) = nValue; \
												else outl( nValue, psPort->nSATARegs[nReg] ); }
#endif


























































