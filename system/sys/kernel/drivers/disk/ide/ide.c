
//-----------------------------------------------------------------------------

#include <atheos/time.h>
#include <atheos/isa_io.h>
#include <atheos/udelay.h>

#include "ide.h"

//-----------------------------------------------------------------------------

#if 0
uint32 CliOnBootCpu()
{
#if 0
    printk( "ide>CliOnBootCpu...\n" );
    while( 1 )
    {
	uint32 flags = cli();
	if( get_processor_id() == 0 ) // FIXME: 0 is not always boot cpu...
	{
	    printk( "ide>CliOnBootCpu...ok\n" );
	    return flags;
	}
	put_cpu_flags( flags );
	Schedule();
    }
#else
    return cli();
#endif
}
#endif

//-----------------------------------------------------------------------------


static void DumpHex( void *data, int length )
{
    void FormatHex( char *string, uint32 val, int nibbels )
	{
	    int i;
	    for( i=0; i<nibbels; i++ )
		string[i] = "0123456789abcdef"[(val>>((nibbels-(i+1))*4))&0xf];
	}
    
    int i,j;
    for( i=0; i<length; i+=16 )
    {
	// 00000000001111111111222222222233333333334444444444555555555566666666667777777777
	// 01234567890123456789012345678901234567890123456789012345678901234567890123456789
	// 00000000: 00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00 | xxxxxxxx xxxxxxxx
	char line[80+1];
	int sublen = length-i>16 ? 16 : length-i;

	memset( line, ' ', sizeof(line) );

	FormatHex( line+0, i, 8 );
	line[8] = ':';
	for( j=0; j<sublen; j++ )
	{
	    uint8 v = ((uint8*)data)[i+j];
	    FormatHex( line+(j<8?10:11)+j*3, v, 2 );
	    line[(j<8?61:62)+j] = v>=0x20 ? v : '.';
	}
	line[59] = '|';
	line[78] = '\n';
	line[79] = 0;
	printk( line );
    }
}

//-----------------------------------------------------------------------------

status_t ata_WaitForStatus( AtaController *ctrl, uint8 andmask, uint8 mask, bigtime_t timeout, bigtime_t delay )
{
    bigtime_t timeoutat = get_system_time() + timeout;
    uint8 status;

    do
    {
	status = inb( ctrl->ioport + ATA_REG_STATUS );
	if( (status&andmask) == mask ) return 0;
//	if( delay ) udelay( delay );
    }
    while( get_system_time() < timeoutat );

    printk( "ata>WaitForStatus: failed: wanted %02X:%02X got %02X\n", andmask, mask, status );
    if( status & 1 )
    {
	uint8 error = inb( ctrl->ioport + ATA_REG_ERROR );
	printk( "ata>WaitForStatus: ERROR: %02X\n", error );
    }
    return -1;
}

status_t ata_SoftReset( AtaController *ctrl )
{
//    int32 cpuflags = cli();

    outb( 0x0c, ctrl->ioport2 );
    udelay( 400 );
    outb( 0x08, ctrl->ioport2 );
    udelay( 400*10 );

    // wait for up to 2 secs for controller reset.
    if( ata_WaitForStatus(
	ctrl,
	ATA_STATUS_BUSY|ATA_STATUS_DRIVEREADY|ATA_STATUS_ERROR,
	ATA_STATUS_DRIVEREADY,
	2000000, 0) != 0 )
    {
	printk( "ata>SoftReset: timeout\n" );
	goto reseterror;
    }

    // is there realy a ata controller here?
    if( inb(ctrl->ioport+ATA_REG_SECTORNUMBER)!=1 || inb(ctrl->ioport+ATA_REG_SECTORCOUNT)!= 1 )
    {
	printk( "ata:SoftReset: could not find controller\n" );
	goto reseterror;
    }
    
//    put_cpu_flags( cpuflags );
    return 0;

reseterror:
//    put_cpu_flags( cpuflags );
    return -1;
}
  
status_t ata_SelectDrive( AtaDrive *drive )
{
    uint32 cpuflags;
    
    if( ((inb(drive->ctrl->ioport+ATA_REG_DRIVE_HEAD)>>4)&1) == drive->drive_id )
	return 0;

      // wait for busy flag to get 0
    if( ata_WaitForStatus(drive->ctrl,ATA_STATUS_BUSY,0,ATA_TIMEOUT_READY,0) != 0 )
    {
	printk( "ata:SelectDrive: drive is not ready\n" );
	return -1;
    }

    cpuflags = cli();

	// select drive
    outb( 0x40 | (drive->drive_id<<4), drive->ctrl->ioport + ATA_REG_DRIVE_HEAD );
    udelay( 400 );

    put_cpu_flags( cpuflags );
    
    if( ata_WaitForStatus(drive->ctrl,ATA_STATUS_BUSY,0,ATA_TIMEOUT_READY,0) != 0 )
    {
	printk( "ata:SelectDrive: drive is not ready (after select)\n" );
	return -1;
    }

    return 0;
}

void ata_PrintIdentity( AtaIdentity *id )
{
    kassertw( sizeof(AtaIdentity) == 512/*, "The AtaIdentity struct MUST be 512 bytes long!"*/ );

    printk( "000 Flags:                    0x%04X\n", id->flags );
    printk( "\t 0, reserved:               %d\n", (id->flags>>0)&1 );
    printk( "\t 1, retired:                %d\n", (id->flags>>1)&1 );
    printk( "\t 2, response incomplete:    %d\n", (id->flags>>2)&1 );
    printk( "\t 3, retired:                %d\n", (id->flags>>3)&1 );
    printk( "\t 4, retired:                %d\n", (id->flags>>4)&1 );
    printk( "\t 5, retired:                %d\n", (id->flags>>5)&1 );
    printk( "\t 6, fixed controller/drive: %d\n", (id->flags>>6)&1 );
    printk( "\t 7, removable media device: %d\n", (id->flags>>7)&1 );
    printk( "\t 8, retired:                %d\n", (id->flags>>8)&1 );
    printk( "\t 9, retired:                %d\n", (id->flags>>9)&1 );
    printk( "\t10, retired:                %d\n", (id->flags>>10)&1 );
    printk( "\t11, retired:                %d\n", (id->flags>>11)&1 );
    printk( "\t12, retired:                %d\n", (id->flags>>12)&1 );
    printk( "\t13, retired:                %d\n", (id->flags>>13)&1 );
    printk( "\t14, retired:                %d\n", (id->flags>>14)&1 );
    printk( "\t15, ata device:             %d\n", (id->flags>>15)&1 );

    printk( "001 Logical cylinders:        %d\n", id->logical_cylinders );
    printk( "002 Reserved:                 0x%04X\n", id->__reserved_04 );
    printk( "003 Logical heads:            %d\n", id->logical_heads );
    printk( "004 Retired:                  0x%04X\n", id->__reserved_08 );
    printk( "005 Retired:                  0x%04X\n", id->__reserved_0a );
    printk( "006 Logical sectors/track:    %d\n", id->logical_sectors );
    printk( "007 Reserved:                 0x%04X\n", id->__reserved_0e );
    printk( "008 Reserved:                 0x%04X\n", id->__reserved_10 );
    printk( "009 Retired:                  0x%04X\n", id->__reserved_12 );
    printk( "010 Serial number:            \"%.20s\"\n", id->serial_number );
    printk( "020 Retired:                  0x%04X\n", id->__reserved_28 );
    printk( "021 Retired:                  0x%04X\n", id->__reserved_2a );
    printk( "022 Obsolete:                 0x%04X\n", id->__reserved_2c );
    printk( "023 Firmware rev.:            \"%.8s\"\n", id->firmware_revision );
    printk( "027 Model number:             \"%.40s\"\n", id->model_number );
    printk( "047 Max sects./interrupt:     0x%04X\n", id->max_sectors_per_interrupt );
    printk( "048 Reserved:                 0x%04X\n", id->__reserved_60 );

    printk( "049 Capabilities 1:           0x%04X\n", id->capabilities1 );
    printk( "\t 0, retired:                %d\n", (id->capabilities1>>0)&1 );
    printk( "\t 1, retired:                %d\n", (id->capabilities1>>1)&1 );
    printk( "\t 2, retired:                %d\n", (id->capabilities1>>2)&1 );
    printk( "\t 3, retired:                %d\n", (id->capabilities1>>3)&1 );
    printk( "\t 4, retired:                %d\n", (id->capabilities1>>4)&1 );
    printk( "\t 5, retired:                %d\n", (id->capabilities1>>5)&1 );
    printk( "\t 6, retired:                %d\n", (id->capabilities1>>6)&1 );
    printk( "\t 7, retired:                %d\n", (id->capabilities1>>7)&1 );
    printk( "\t 8, must be Set:            %d\n", (id->capabilities1>>8)&1 );
    printk( "\t 9, must be Set:            %d\n", (id->capabilities1>>9)&1 );
    printk( "\t10, IORDY disable:          %d\n", (id->capabilities1>>10)&1 );
    printk( "\t11, IORDY supported:        %d\n", (id->capabilities1>>11)&1 );
    printk( "\t12, reserved:               %d\n", (id->capabilities1>>12)&1 );
    printk( "\t13, standby timers sup.:    %d\n", (id->capabilities1>>13)&1 );
    printk( "\t14, reserved:               %d\n", (id->capabilities1>>14)&1 );
    printk( "\t15, reserved:               %d\n", (id->capabilities1>>15)&1 );
    
    printk( "050 Capabilities 2:           0x%04X\n", id->capabilities2 );
    printk( "\t 0, should be one:          %d\n", (id->capabilities2>>0)&1 );
    printk( "\t 1, reserved:               %d\n", (id->capabilities2>>1)&1 );
    printk( "\t 2, reserved:               %d\n", (id->capabilities2>>2)&1 );
    printk( "\t 3, reserved:               %d\n", (id->capabilities2>>3)&1 );
    printk( "\t 4, reserved:               %d\n", (id->capabilities2>>4)&1 );
    printk( "\t 5, reserved:               %d\n", (id->capabilities2>>5)&1 );
    printk( "\t 6, reserved:               %d\n", (id->capabilities2>>6)&1 );
    printk( "\t 7, reserved:               %d\n", (id->capabilities2>>7)&1 );
    printk( "\t 8, reserved:               %d\n", (id->capabilities2>>8)&1 );
    printk( "\t 9, reserved:               %d\n", (id->capabilities2>>9)&1 );
    printk( "\t10, reserved:               %d\n", (id->capabilities2>>10)&1 );
    printk( "\t11, reserved:               %d\n", (id->capabilities2>>11)&1 );
    printk( "\t12, reserved:               %d\n", (id->capabilities2>>12)&1 );
    printk( "\t13, reserved:               %d\n", (id->capabilities2>>13)&1 );
    printk( "\t14, must be set:            %d\n", (id->capabilities2>>14)&1 );
    printk( "\t15, must be zero:           %d\n", (id->capabilities2>>15)&1 );

    printk( "051 Obsolete: 	           0x%04X\n", id->__reserved_66 );
    printk( "052 Obsolete: 	           0x%04X\n", id->__reserved_68 );

    printk( "053 Valid flags:              0x%04X\n", id->valid_flag );
    printk( "\t 0, word 54-58 are valid:   %d\n", (id->valid_flag>>0)&1 );
    printk( "\t 1, word 64-70 are valid:   %d\n", (id->valid_flag>>1)&1 );
    printk( "\t 2, word 88 are valid:      %d\n", (id->valid_flag>>2)&1 );
    printk( "\t 3, reserved:               %d\n", (id->valid_flag>>3)&1 );
    printk( "\t 4, reserved:               %d\n", (id->valid_flag>>4)&1 );
    printk( "\t 5, reserved:               %d\n", (id->valid_flag>>5)&1 );
    printk( "\t 6, reserved:               %d\n", (id->valid_flag>>6)&1 );
    printk( "\t 7, reserved:               %d\n", (id->valid_flag>>7)&1 );
    printk( "\t 8, reserved:               %d\n", (id->valid_flag>>8)&1 );
    printk( "\t 9, reserved:               %d\n", (id->valid_flag>>9)&1 );
    printk( "\t10, reserved:               %d\n", (id->valid_flag>>10)&1 );
    printk( "\t11, reserved:               %d\n", (id->valid_flag>>11)&1 );
    printk( "\t12, reserved:               %d\n", (id->valid_flag>>12)&1 );
    printk( "\t13, reserved:               %d\n", (id->valid_flag>>13)&1 );
    printk( "\t14, reserved:               %d\n", (id->valid_flag>>14)&1 );
    printk( "\t15, reserved:               %d\n", (id->valid_flag>>15)&1 );

    printk( "054 Cur. logical cylinders:   %d\n", id->cur_logical_cylinders );
    printk( "055 Cur. logical headers:     %d\n", id->cur_logical_heads );
    printk( "056 Cur. logical sectors:     %d\n", id->cur_logical_sectors );
    printk( "057 Cur. size:                %d sectors\n", id->cur_size_in_sectors_lo +
	    id->cur_size_in_sectors_hi*65536 );
    printk( "059 Cur. max sec./interrupt:  %d\n", id->cur_max_sectors_per_interrupt );
    printk( "060 Cur. addrresseable secs:  %d sectors\n", id->cur_addresseable_sectors_lo +
	    id->cur_addresseable_sectors_hi*65536 );
    printk( "062 Obsolete: 	           0x%04X\n", id->__reserved_7c );

    printk( "063 DMA Mode:                 0x%04X\n", id->dma_mode );
    printk( "\t 0, MW DMA-0 supported:     %d\n", (id->dma_mode>>0)&1 );
    printk( "\t 1, MW DMA-1 supported:     %d\n", (id->dma_mode>>1)&1 );
    printk( "\t 2, MW DMA-2 supported:     %d\n", (id->dma_mode>>2)&1 );
    printk( "\t 3, reserved:               %d\n", (id->dma_mode>>3)&1 );
    printk( "\t 4, reserved:               %d\n", (id->dma_mode>>4)&1 );
    printk( "\t 5, reserved:               %d\n", (id->dma_mode>>5)&1 );
    printk( "\t 6, reserved:               %d\n", (id->dma_mode>>6)&1 );
    printk( "\t 7, reserved:               %d\n", (id->dma_mode>>7)&1 );
    printk( "\t 8, MW DMA-0 selected:      %d\n", (id->dma_mode>>8)&1 );
    printk( "\t 9, MW DMA-1 selected:      %d\n", (id->dma_mode>>9)&1 );
    printk( "\t10, MW DMA-2 selected:      %d\n", (id->dma_mode>>10)&1 );
    printk( "\t11, reserved:               %d\n", (id->dma_mode>>11)&1 );
    printk( "\t12, reserved:               %d\n", (id->dma_mode>>12)&1 );
    printk( "\t13, reserved:               %d\n", (id->dma_mode>>13)&1 );
    printk( "\t14, reserved:               %d\n", (id->dma_mode>>14)&1 );
    printk( "\t15, reserved:               %d\n", (id->dma_mode>>15)&1 );
    
    printk( "064 Advancesd PIO Mode:       0x%04X\n", id->advanced_pio_mode );
    printk( "\t 0, ?ADV PIO-0 supported:   %d\n", (id->advanced_pio_mode>>0)&1 );
    printk( "\t 1, ?ADV PIO-1 supported:   %d\n", (id->advanced_pio_mode>>1)&1 );
    printk( "\t 2, ?ADV PIO-2 supported:   %d\n", (id->advanced_pio_mode>>2)&1 );
    printk( "\t 3, ?ADV PIO-3 supported:   %d\n", (id->advanced_pio_mode>>3)&1 );
    printk( "\t 4, ?ADV PIO-4 supported:   %d\n", (id->advanced_pio_mode>>4)&1 );
    printk( "\t 5, ?ADV PIO-5 supported:   %d\n", (id->advanced_pio_mode>>5)&1 );
    printk( "\t 6, ?ADV PIO-6 supported:   %d\n", (id->advanced_pio_mode>>6)&1 );
    printk( "\t 7, ?ADV PIO-7 supported:   %d\n", (id->advanced_pio_mode>>7)&1 );
    printk( "\t 8, reserved:               %d\n", (id->advanced_pio_mode>>8)&1 );
    printk( "\t 9, reserved:               %d\n", (id->advanced_pio_mode>>9)&1 );
    printk( "\t10, reserved:               %d\n", (id->advanced_pio_mode>>10)&1 );
    printk( "\t11, reserved:               %d\n", (id->advanced_pio_mode>>11)&1 );
    printk( "\t12, reserved:               %d\n", (id->advanced_pio_mode>>12)&1 );
    printk( "\t13, reserved:               %d\n", (id->advanced_pio_mode>>13)&1 );
    printk( "\t14, reserved:               %d\n", (id->advanced_pio_mode>>14)&1 );
    printk( "\t15, reserved:               %d\n", (id->advanced_pio_mode>>15)&1 );

    printk( "065 Min MW DMA cycle time:    %d\n", id->min_dma_cycles );
    printk( "066 Rec MW DMA cycle time:    %d\n", id->recomended_dma_cycles );
    printk( "067 Min PIO cycle time(noflo):%d\n", id->min_noflow_pio_cycles );
    printk( "068 Min PIO cycle time(iordl):%d\n", id->min_iordy__pio_cycles );
    printk( "069 Reserved:                 0x%04X\n", id->__reserved_8a );
    printk( "070 Reserved:                 0x%04X\n", id->__reserved_8c );
    printk( "071 Reserved:                 0x%04X\n", id->__reserved_8e );
    printk( "072 Reserved:                 0x%04X\n", id->__reserved_90 );
    printk( "073 Reserved:                 0x%04X\n", id->__reserved_92 );
    printk( "074 Reserved:                 0x%04X\n", id->__reserved_94 );
    printk( "075 Queue depth:              %d\n", id->queue_depth );
    printk( "076 Reserved:                 0x%04X\n", id->__reserved_98 );
    printk( "077 Reserved:                 0x%04X\n", id->__reserved_9a );
    printk( "078 Reserved:                 0x%04X\n", id->__reserved_9c );
    printk( "079 Reserved:                 0x%04X\n", id->__reserved_9e );
    printk( "080 Major Version:            %d\n", id->major_version );
    printk( "081 Minor Version:            %d\n", id->minor_version );

    printk( "082 Supported commands 1:     0x%04X\n", id->supported_features_1 );
    printk( "\t 0, smart:                  %d\n", (id->supported_features_1>>0)&1 );
    printk( "\t 1, security features:      %d\n", (id->supported_features_1>>1)&1 );
    printk( "\t 2, removable media feat.:  %d\n", (id->supported_features_1>>2)&1 );
    printk( "\t 3, power manag. feat.:     %d\n", (id->supported_features_1>>3)&1 );
    printk( "\t 4, packet command:         %d\n", (id->supported_features_1>>4)&1 );
    printk( "\t 5, write cache:            %d\n", (id->supported_features_1>>5)&1 );
    printk( "\t 6, look-ahead:             %d\n", (id->supported_features_1>>6)&1 );
    printk( "\t 7, release interrupt:      %d\n", (id->supported_features_1>>7)&1 );
    printk( "\t 8, service interrupt:      %d\n", (id->supported_features_1>>8)&1 );
    printk( "\t 9, device reset interrupt: %d\n", (id->supported_features_1>>9)&1 );
    printk( "\t10, host protected area:    %d\n", (id->supported_features_1>>10)&1 );
    printk( "\t11, oboselete:              %d\n", (id->supported_features_1>>11)&1 );
    printk( "\t12, write buffer command:   %d\n", (id->supported_features_1>>12)&1 );
    printk( "\t13, read buffer command:    %d\n", (id->supported_features_1>>13)&1 );
    printk( "\t14, nop command:            %d\n", (id->supported_features_1>>14)&1 );
    printk( "\t15, oboselete:              %d\n", (id->supported_features_1>>15)&1 );

    printk( "083 Supported features 2:     0x%04X\n", id->supported_features_2 );
    printk( "\t 0, download microcode cmd.:%d\n", (id->supported_features_2>>0)&1 );
    printk( "\t 1, read/write dma queued:  %d\n", (id->supported_features_2>>1)&1 );
    printk( "\t 2, cfa features:           %d\n", (id->supported_features_2>>2)&1 );
    printk( "\t 3, apm features:           %d\n", (id->supported_features_2>>3)&1 );
    printk( "\t 4, rem.media stat. notify: %d\n", (id->supported_features_2>>4)&1 );
    printk( "\t 5, powerup in standby feat:%d\n", (id->supported_features_2>>5)&1 );
    printk( "\t 6, set features subcmd req:%d\n", (id->supported_features_2>>6)&1 );
    printk( "\t 7, reserved:               %d\n", (id->supported_features_2>>7)&1 );
    printk( "\t 8, set max security ext.:  %d\n", (id->supported_features_2>>8)&1 );
    printk( "\t 9, reserved:               %d\n", (id->supported_features_2>>9)&1 );
    printk( "\t10, reserved:               %d\n", (id->supported_features_2>>10)&1 );
    printk( "\t11, reserved:               %d\n", (id->supported_features_2>>11)&1 );
    printk( "\t12, reserved:               %d\n", (id->supported_features_2>>12)&1 );
    printk( "\t13, reserved:               %d\n", (id->supported_features_2>>13)&1 );
    printk( "\t14, must be set:            %d\n", (id->supported_features_2>>14)&1 );
    printk( "\t15, must be zero:           %d\n", (id->supported_features_2>>15)&1 );

    printk( "084 Supported features 3:     0x%04X\n", id->supported_features_3 );
    printk( "\t 0, reserved:               %d\n", (id->supported_features_3>>0)&1 );
    printk( "\t 1, reserved:               %d\n", (id->supported_features_3>>1)&1 );
    printk( "\t 2, reserved:               %d\n", (id->supported_features_3>>2)&1 );
    printk( "\t 3, reserved:               %d\n", (id->supported_features_3>>3)&1 );
    printk( "\t 4, reserved:               %d\n", (id->supported_features_3>>4)&1 );
    printk( "\t 5, reserved:               %d\n", (id->supported_features_3>>5)&1 );
    printk( "\t 6, reserved:               %d\n", (id->supported_features_3>>6)&1 );
    printk( "\t 7, reserved:               %d\n", (id->supported_features_3>>7)&1 );
    printk( "\t 8, reserved:               %d\n", (id->supported_features_3>>8)&1 );
    printk( "\t 9, reserved:               %d\n", (id->supported_features_3>>9)&1 );
    printk( "\t10, reserved:               %d\n", (id->supported_features_3>>10)&1 );
    printk( "\t11, reserved:               %d\n", (id->supported_features_3>>11)&1 );
    printk( "\t12, reserved:               %d\n", (id->supported_features_3>>12)&1 );
    printk( "\t13, reserved:               %d\n", (id->supported_features_3>>13)&1 );
    printk( "\t14, must be set:            %d\n", (id->supported_features_3>>14)&1 );
    printk( "\t15, must be zero:           %d\n", (id->supported_features_3>>15)&1 );
    
    printk( "085 Enabled features 1:     0x%04X\n", id->enabled_features_1 );
    printk( "\t 0, smart:                  %d\n", (id->enabled_features_1>>0)&1 );
    printk( "\t 1, security features:      %d\n", (id->enabled_features_1>>1)&1 );
    printk( "\t 2, removable media feat.:  %d\n", (id->enabled_features_1>>2)&1 );
    printk( "\t 3, power manag. feat.:     %d\n", (id->enabled_features_1>>3)&1 );
    printk( "\t 4, packet command:         %d\n", (id->enabled_features_1>>4)&1 );
    printk( "\t 5, write cache:            %d\n", (id->enabled_features_1>>5)&1 );
    printk( "\t 6, look-ahead:             %d\n", (id->enabled_features_1>>6)&1 );
    printk( "\t 7, release interrupt:      %d\n", (id->enabled_features_1>>7)&1 );
    printk( "\t 8, service interrupt:      %d\n", (id->enabled_features_1>>8)&1 );
    printk( "\t 9, device reset interrupt: %d\n", (id->enabled_features_1>>9)&1 );
    printk( "\t10, host protected area:    %d\n", (id->enabled_features_1>>10)&1 );
    printk( "\t11, oboselete:              %d\n", (id->enabled_features_1>>11)&1 );
    printk( "\t12, write buffer command:   %d\n", (id->enabled_features_1>>12)&1 );
    printk( "\t13, read buffer command:    %d\n", (id->enabled_features_1>>13)&1 );
    printk( "\t14, nop command:            %d\n", (id->enabled_features_1>>14)&1 );
    printk( "\t15, oboselete:              %d\n", (id->enabled_features_1>>15)&1 );

    printk( "086 Enabled features 2:     0x%04X\n", id->enabled_features_2 );
    printk( "\t 0, download microcode cmd.:%d\n", (id->enabled_features_2>>0)&1 );
    printk( "\t 1, read/write dma queued:  %d\n", (id->enabled_features_2>>1)&1 );
    printk( "\t 2, cfa features:           %d\n", (id->enabled_features_2>>2)&1 );
    printk( "\t 3, apm features:           %d\n", (id->enabled_features_2>>3)&1 );
    printk( "\t 4, rem.media stat. notify: %d\n", (id->enabled_features_2>>4)&1 );
    printk( "\t 5, powerup in standby feat:%d\n", (id->enabled_features_2>>5)&1 );
    printk( "\t 6, set features subcmd req:%d\n", (id->enabled_features_2>>6)&1 );
    printk( "\t 7, reserved:               %d\n", (id->enabled_features_2>>7)&1 );
    printk( "\t 8, set max security ext.:  %d\n", (id->enabled_features_2>>8)&1 );
    printk( "\t 9, reserved:               %d\n", (id->enabled_features_2>>9)&1 );
    printk( "\t10, reserved:               %d\n", (id->enabled_features_2>>10)&1 );
    printk( "\t11, reserved:               %d\n", (id->enabled_features_2>>11)&1 );
    printk( "\t12, reserved:               %d\n", (id->enabled_features_2>>12)&1 );
    printk( "\t13, reserved:               %d\n", (id->enabled_features_2>>13)&1 );
    printk( "\t14, reserved:               %d\n", (id->enabled_features_2>>14)&1 );
    printk( "\t15, reserved:               %d\n", (id->enabled_features_2>>15)&1 );

    printk( "087 Enabled features 3:      0x%04X\n", id->enabled_features_3 );
    printk( "\t 0, reserved:               %d\n", (id->enabled_features_3>>0)&1 );
    printk( "\t 1, reserved:               %d\n", (id->enabled_features_3>>1)&1 );
    printk( "\t 2, reserved:               %d\n", (id->enabled_features_3>>2)&1 );
    printk( "\t 3, reserved:               %d\n", (id->enabled_features_3>>3)&1 );
    printk( "\t 4, reserved:               %d\n", (id->enabled_features_3>>4)&1 );
    printk( "\t 5, reserved:               %d\n", (id->enabled_features_3>>5)&1 );
    printk( "\t 6, reserved:               %d\n", (id->enabled_features_3>>6)&1 );
    printk( "\t 7, reserved:               %d\n", (id->enabled_features_3>>7)&1 );
    printk( "\t 8, reserved:               %d\n", (id->enabled_features_3>>8)&1 );
    printk( "\t 9, reserved:               %d\n", (id->enabled_features_3>>9)&1 );
    printk( "\t10, reserved:               %d\n", (id->enabled_features_3>>10)&1 );
    printk( "\t11, reserved:               %d\n", (id->enabled_features_3>>11)&1 );
    printk( "\t12, reserved:               %d\n", (id->enabled_features_3>>12)&1 );
    printk( "\t13, reserved:               %d\n", (id->enabled_features_3>>13)&1 );
    printk( "\t14, must be set:            %d\n", (id->enabled_features_3>>14)&1 );
    printk( "\t15, must be zero:           %d\n", (id->enabled_features_3>>15)&1 );
#if 0
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
#endif
}

status_t ata_Identify( AtaDrive *drive )
{
    uint8 cyllo, cylhi, status;
    int sectors_in_chs_mode, sectors_in_lba_mode;

    if( ata_SelectDrive(drive) != 0 )
	return -1;

    cyllo = inb( drive->ctrl->ioport + ATA_REG_CYLINDERLOW );
    cylhi = inb( drive->ctrl->ioport + ATA_REG_CYLINDERHIGH );
    status = inb( drive->ctrl->ioport + ATA_REG_STATUS );

    if( cyllo==0x14 && cylhi==0xeb )
    {
	printk( "ata:Identify: found a ATAPI drive\n" );
	// note: not yet supported...
	return -1;
    }
    else if( cyllo==0x00 && cylhi==0x00 && status!=0 )
    {
	int i;
	AtaIdentity id;

	printk( "ata:Identify: found a ATA drive\n" );

	outb( ATA_COMMAND_IDENTIFY_DRIVE, drive->ctrl->ioport + ATA_REG_COMMAND );

	if( ata_WaitForStatus(drive->ctrl,ATA_STATUS_DATAREQUEST,ATA_STATUS_DATAREQUEST,ATA_TIMEOUT_READY,0) != 0 )
	{
	    printk( "ata:Identify: drive did not respond to ATA_COMMAND_IDENTIFY_DRIVE\n" );
	    return -1;
	}

	for( i=0; i<sizeof(AtaIdentity)/2; i++ )
	    ((uint16*)&id)[i] = inw( drive->ctrl->ioport + ATA_REG_DATA );

	ata_PrintIdentity( &id );

	drive->max_sectors = id.max_sectors_per_interrupt&0x8000 &&
	    (id.max_sectors_per_interrupt&0xff)>=1 ? id.max_sectors_per_interrupt&0xff : 1;

	sectors_in_chs_mode = id.logical_cylinders * id.logical_heads * id.logical_sectors;
	sectors_in_lba_mode = id.cur_addresseable_sectors_lo + id.cur_addresseable_sectors_hi*65536;

	if( sectors_in_lba_mode>sectors_in_chs_mode && (sectors_in_lba_mode*9/10)<sectors_in_chs_mode )
	    drive->lba_mode = true;
	else
	    drive->lba_mode = false;
	
	if( drive->lba_mode )
	{
	    drive->cylinders = 1;
	    drive->heads = 1;
	    drive->sectors = id.cur_addresseable_sectors_lo + id.cur_addresseable_sectors_hi*65536;
	}
	else
	{
	    drive->cylinders = id.logical_cylinders;
	    drive->heads = id.logical_heads;
	    drive->sectors = id.logical_sectors;
	}

	drive->size_sectors = drive->cylinders*drive->heads*drive->sectors;
	drive->size_bytes = drive->size_sectors * 512LL;
	
	
	printk( "ata:Identify: disk size: %d sectors, %Ld MB%s\n",
		drive->size_sectors, drive->size_bytes/(1024*1024),
		drive->lba_mode?" (lba)":"" );
	
	return 0;
    }
    else
    {
	printk( "ata:identify: found a unknown drive\n" );
	return -1;
    }
}

void ata_Sector2CHS( AtaDrive *drive, uint32 offset, uint16 *cylinder, uint8 *head, uint8 *sector )
{
    if( drive->lba_mode )
    {
	*sector = offset & 0xff;
	offset >>= 8;
	*cylinder = offset & 0xffff;
	offset >>= 16;
	*head = (offset & 0x0f) | 0x40;
    }
    else
    {
	*sector = (offset % drive->sectors) + 1;
	offset /= drive->sectors;

	*head = offset % drive->heads;
	offset /= drive->heads;

	*cylinder = offset;
    }
}

status_t ata_ReadSectors( AtaDrive *drive, void *buffer, int offset, int length )
{
    int8 idestatus;
    uint16 cylinder;
    uint8 head, sector;
    int isector;
    uint32 cpuflags;
    int retrycnt = 0;
    
//    printk( "ata:ReadSectors: %d sector(s) @ %08X\n", length, offset );

    if( offset<0 || offset+length>drive->size_sectors || length<0 )
    {
	printk( "ata:ReadSectors: ILLEGAL READ: %d sectors @ %d\n", length, offset );
	goto readerror;
    }

retry:
    retrycnt++;
    if( retrycnt > 3 )
	goto readerror;

    if( retrycnt != 1 )
	printk( "ata:ReadSectors: RETRY%d: %d sector(s) @ %08X\n", retrycnt, length, offset );


    if( ata_SelectDrive(drive) != 0 )
    {
	printk( "ata:ReadSectors: could not select drive\n" );
	goto retry;
    }

    for( isector=0; isector<length; isector+=drive->max_sectors )
    {
	int subsectorcnt = length-isector;
	if( subsectorcnt > drive->max_sectors )
	    subsectorcnt = drive->max_sectors;

	ata_Sector2CHS( drive, offset+isector, &cylinder, &head, &sector );
//	printk( "ata:ReadSectors: %d sector(s) @ %08X (CHS:%d,%d,%d)\n",
//		subsectorcnt, offset+isector, cylinder, head, sector );
	
	// Wait for drive to get ready to accept commands...
	if( ata_WaitForStatus(drive->ctrl,
			      ATA_STATUS_BUSY|ATA_STATUS_DATAREQUEST|ATA_STATUS_DRIVEREADY,
			      ATA_STATUS_DRIVEREADY,ATA_TIMEOUT_READY,0) != 0 )
	{
	    printk( "ata:ReadSectors: drive is not ready\n" );
	    goto retry;
	}
	
	cpuflags = cli();

	outb( subsectorcnt&0xff, drive->ctrl->ioport + ATA_REG_SECTORCOUNT );
	outb( sector, drive->ctrl->ioport + ATA_REG_SECTORNUMBER );
	outb( cylinder&255, drive->ctrl->ioport + ATA_REG_CYLINDERLOW );
	outb( cylinder/256, drive->ctrl->ioport + ATA_REG_CYLINDERHIGH );
	outb( (drive->drive_id<<4) | head, drive->ctrl->ioport + ATA_REG_DRIVE_HEAD );
	udelay( 400 );
	outb( subsectorcnt>1 ? ATA_COMMAND_READ_MULTI : ATA_COMMAND_READ,
	      drive->ctrl->ioport + ATA_REG_COMMAND );
	udelay( 400 );

	put_cpu_flags( cpuflags );
    
	// Wait for drive to get ready...
	if( ata_WaitForStatus(drive->ctrl,
			      ATA_STATUS_BUSY|ATA_STATUS_DRIVEREADY,
			      ATA_STATUS_DRIVEREADY,1000000,0) != 0 )
	{
	    printk( "ata:ReadSectors: read command failed\n" );
	    goto retry;
	}

//	for( iword=0; iword<subsectorcnt*512/2; iword++ )
//	    ((uint16*)buffer)[isector*512/2+iword] = inw( drive->ctrl->ioport + ATA_REG_DATA );
//	insw( drive->ctrl->ioport + ATA_REG_DATA, ((uint16*)buffer)+isector*512/2, subsectorcnt*512/2 );
	insl( drive->ctrl->ioport + ATA_REG_DATA, ((uint16*)buffer)+isector*512/2, subsectorcnt*512/4 );


//	DumpHex( ((uint16*)buffer)+isector*512/2, 512 );
	
	idestatus = inb( drive->ctrl->ioport + ATA_REG_STATUS );
	if( idestatus & ATA_STATUS_CORRECTEDDATA )
	    printk( "ata:ReadSectors: THERE WAS A READ ERROR (but the controller fixed it)\n" );
	if( idestatus & ATA_STATUS_ERROR )
	{
	    printk( "ata:ReadSectors: THERE WAS A FATAL READ ERROR!\n" );
	    goto retry;
	}
    }    

    return 0;

readerror:
    return -1;
}

status_t ata_WriteSectors( AtaDrive *drive, const void *buffer, int offset, int length )
{
    int8 idestatus;
    uint16 cylinder;
    uint8 head, sector;
    int isector;
//    int iword;
    uint32 cpuflags;
    int retrycnt = 0;

//    printk( "ata:WriteSectors: %d sector(s) @ %08X\n", length, offset );

    if( offset<0 || offset+length>drive->size_sectors || length<0 )
    {
	printk( "ata:WriteSectors: ILLEGAL WRITE: %d sectors @ %d\n", length, offset );
	goto writeerror;
    }

retry:
    retrycnt++;
    if( retrycnt > 3 )
	goto writeerror;
    
    if( retrycnt != 1 )
	printk( "ata:WriteSectors: RETRY%d: %d sector(s) @ %08X\n", retrycnt, length, offset );

    if( ata_SelectDrive(drive) != 0 )
    {
	printk( "ata:WriteSectors: could not select drive\n" );
	goto retry;
    }

    for( isector=0; isector<length; isector+=drive->max_sectors )
    {
	int subsectorcnt = length-isector;
	if( subsectorcnt > drive->max_sectors )
	    subsectorcnt = drive->max_sectors;

	ata_Sector2CHS( drive, offset+isector, &cylinder, &head, &sector );
//	printk( "ata:WriteSectors: %d sector(s) @ %08X (CHS:%d,%d,%d)\n",
//		subsectorcnt, offset+isector, cylinder, head, sector );
	
	// Wait for drive to get ready to accept commands...
	if( ata_WaitForStatus(drive->ctrl,
			      ATA_STATUS_BUSY|ATA_STATUS_DATAREQUEST|ATA_STATUS_DRIVEREADY,
			      ATA_STATUS_DRIVEREADY,ATA_TIMEOUT_READY,0) != 0 )
	{
	    printk( "ata:WriteSectors: drive is not ready\n" );
	    goto retry;
	}

	cpuflags = cli();

	outb( subsectorcnt&0xff, drive->ctrl->ioport + ATA_REG_SECTORCOUNT );
	outb( sector, drive->ctrl->ioport + ATA_REG_SECTORNUMBER );
	outb( cylinder&255, drive->ctrl->ioport + ATA_REG_CYLINDERLOW );
	outb( cylinder/256, drive->ctrl->ioport + ATA_REG_CYLINDERHIGH );
	outb( (drive->drive_id<<4) | head, drive->ctrl->ioport + ATA_REG_DRIVE_HEAD );
	udelay( 400 );
	outb( subsectorcnt>1 ? ATA_COMMAND_WRITE_MULTI : ATA_COMMAND_WRITE,
	      drive->ctrl->ioport + ATA_REG_COMMAND );
	udelay( 400 );

	put_cpu_flags( cpuflags );
	
	// Wait for drive to get ready...
	if( ata_WaitForStatus(drive->ctrl,
			      ATA_STATUS_BUSY|ATA_STATUS_DRIVEREADY,
			      ATA_STATUS_DRIVEREADY,1000000,0) != 0 )
	{
	    printk( "ata:WriteSectors: write command failed\n" );
	    goto retry;
	}

//	for( iword=0; iword<subsectorcnt*512/2; iword++ )
//	    outw( ((uint16*)buffer)[isector*512/2+iword], drive->ctrl->ioport + ATA_REG_DATA );
//	outw( drive->ctrl->ioport + ATA_REG_DATA, ((uint16*)buffer)+isector*512/2, subsectorcnt*512/2 );
	outsl( drive->ctrl->ioport + ATA_REG_DATA, ((uint16*)buffer)+isector*512/2, subsectorcnt*512/4 );

	idestatus = inb( drive->ctrl->ioport + ATA_REG_STATUS );
	if( idestatus & ATA_STATUS_CORRECTEDDATA )
	    printk( "ata:WriteSectors: THERE WAS A WRITE ERROR (but the controller fixed it)\n" );
	if( idestatus & ATA_STATUS_ERROR )
	{
	    printk( "ata:WriteSectors: THERE WAS A FATAL WRITE ERROR!\n" );
	    goto retry;
	}
    }    

    return 0;

writeerror:
    return -1;
}
