#include <atheos/kernel.h>
#include <atheos/filesystem.h>
#include <atheos/types.h>
#include <atheos/device.h>

#define ATA_REG_DATA 			0x00 // Read/Write
#define ATA_REG_ERROR			0x01 // Read
#define ATA_REG_PRECOMP			0x01 // Write
#define ATA_REG_SECTORCOUNT		0x02 // Read/Write
#define ATA_REG_SECTORNUMBER		0x03 // Read/Write
#define ATA_REG_CYLINDERLOW		0x04 // Read/Write
#define ATA_REG_CYLINDERHIGH		0x05 // Read/Write
#define ATA_REG_DRIVE_HEAD		0x06 // Read/Write
#define ATA_REG_STATUS			0x07 // Read
#define ATA_REG_COMMAND			0x07 // Write

// ATA_REG_STATUS:
#define ATA_STATUS_ERROR		0x01 // the last command failed
#define ATA_STATUS_INDEX		0x02 // set when the index mask on a track is detected.
#define ATA_STATUS_CORRECTEDDATA	0x04 // there was a readerror, but the drive was able to fix the data.
#define ATA_STATUS_DATAREQUEST		0x08 // the drive is ready to transfer a byte/word
#define ATA_STATUS_SEEKCOMPLETE		0x10 // seek is completed, and the head is settled
#define ATA_STATUS_WRITEFAULT		0x20 // set when write fails (reset when the status reg is read)
#define ATA_STATUS_DRIVEREADY		0x40 // set when the drive is ready for a command
#define ATA_STATUS_BUSY			0x80 // the controller is busy

#define ATA_TIMEOUT_READY		1000*1000 // (hmmm, find real value)

#define ATA_COMMAND_READ		0x20
#define ATA_COMMAND_READ_NORETRY	0x21
#define ATA_COMMAND_READ_MULTI		0xC4
#define ATA_COMMAND_WRITE		0x30
#define ATA_COMMAND_WRITE_MULTI		0xC5
#define ATA_COMMAND_IDENTIFY_DRIVE	0xec


// TODO: get real names for struct:
typedef struct AtaIdentity
{
    uint16	flags;			  	// 00
    uint16	logical_cylinders;	  	// 02
    uint16	__reserved_04;		  	// 04
    uint16	logical_heads;		  	// 06
    uint16	__reserved_08;			// 08
    uint16	__reserved_0a;			// 0a
    uint16	logical_sectors;		// 0c
    uint16	__reserved_0e; 			// 0e
    uint16	__reserved_10;			// 10
    uint16	__reserved_12;			// 12
    char	serial_number[20];	     	// 14
    uint16	__reserved_28;			// 28
    uint16	__reserved_2a;			// 2a
    uint16	__reserved_2c;			// 2c
    char	firmware_revision[8];		// 2e
    char	model_number[40];		// 36
    uint16	max_sectors_per_interrupt;	// 5e
    uint16	__reserved_60;			// 60
    uint16	capabilities1;			// 62
    uint16	capabilities2;		       	// 64
    uint16	__reserved_66;	 		// 66
    uint16	__reserved_68;	 		// 68
    uint16	valid_flag;			// 6a
    uint16	cur_logical_cylinders;		// 6c
    uint16	cur_logical_heads;		// 6e
    uint16	cur_logical_sectors;		// 70
    uint16	cur_size_in_sectors_lo;		// 72
    uint16	cur_size_in_sectors_hi;		// 74
    uint16	cur_max_sectors_per_interrupt;	// 76
    uint16	cur_addresseable_sectors_lo;	// 78
    uint16	cur_addresseable_sectors_hi;	// 7a
    uint16	__reserved_7c;	 		// 7c
    uint16	dma_mode;			// 7e
    uint16	advanced_pio_mode;		// 80
    uint16	min_dma_cycles;			// 82
    uint16	recomended_dma_cycles;		// 84
    uint16	min_noflow_pio_cycles;		// 86
    uint16	min_iordy__pio_cycles;		// 88
    uint16	__reserved_8a;	 		// 8a
    uint16	__reserved_8c;	 		// 8c
    uint16	__reserved_8e;	 		// 8e
    uint16	__reserved_90;	 		// 90
    uint16	__reserved_92;	 		// 92
    uint16	__reserved_94;	 		// 94
    uint16	queue_depth;	 		// 96
    uint16	__reserved_98;	 		// 98
    uint16	__reserved_9a;	 		// 9a
    uint16	__reserved_9c;	 		// 9c
    uint16	__reserved_9e;	 		// 9e
    uint16	major_version;	 		// a0
    uint16	minor_version;	 		// a2
    uint16	supported_features_1; 		// a4
    uint16	supported_features_2; 		// a6
    uint16	supported_features_3;		// a8
    uint16	enabled_features_1;    		// aa
    uint16	enabled_features_2;    		// ac
    uint16	enabled_features_3;    		// ae
    uint16	dma_mode_2; 	       		// b0
    uint16	security_erase_time;   		// b2
    uint16	enhanced_security_erase_time;   // b4
    uint16	cur_adv_power_value;		// b6
    uint16	master_password_revision_code;	// b8
    uint16	hardware_reset_result;		// ba
    char	__reserved_bc_fc[0xfe -0xbc];	// bc
    uint16	removable_media_status_features;// fe
    uint16	security_status;		// 100
    char	__reserved_102_13e[0x140-0x102];// 102
    uint16	cfa_power_mode_1;		// 140
    char	__reserved_142_14e[0x150-0x142];// 142	// compact flash
    char	__reserved_150_1fc[0x1fe -0x150];// 150
    uint16	integrity;			// 1fe
} AtaIdentity;

typedef struct AtaController
{
    struct AtaController *next;
    int		index;
    
    int		ioport;
    int		ioport2;
    int		irq;

    sem_id	iolock;
} AtaController;

typedef struct
{
    AtaController *ctrl;
    int           drive_id; // 0=master, 1=slave

    bool	  lba_mode;

    int		  cylinders;
    int		  heads;
    int		  sectors;

    int		  max_sectors; // how many sectors can be transferd in one command

    int		  size_sectors;
    off_t	  size_bytes;
} AtaDrive;

uint32 CliOnBootCpu();

status_t ata_WaitForStatus( AtaController *ctrl, uint8 andmask, uint8 mask, bigtime_t timeout, bigtime_t delay );
status_t ata_SoftReset( AtaController *ctrl );
status_t ata_Identify( AtaDrive *drive );
status_t ata_ReadSectors( AtaDrive *drive, void *buffer, int offset, int length );
status_t ata_WriteSectors( AtaDrive *drive, const void *buffer, int offset, int length );

