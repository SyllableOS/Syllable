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
 
#ifndef __ATA_PRIVATE_H_
#define __ATA_PRIVATE_H_

#define ATA_CMD_DELAY 400
#define ATA_CMD_TIMEOUT 315000

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

/* ata_port.c */
void ata_port_identify( ATA_port_s* psPort );
status_t ata_port_configure( ATA_port_s* psPort );
void     ata_port_select( ATA_port_s* psPort, uint8 nAdd );
uint8    ata_port_status( ATA_port_s* psPort );
uint8    ata_port_altstatus( ATA_port_s* psPort );
status_t ata_port_prepare_dma_read( ATA_port_s* psPort, uint8* pBuffer, uint32 nLen );
status_t ata_port_prepare_dma_write( ATA_port_s* psPort, uint8* pBuffer, uint32 nLen );
status_t ata_port_start_dma( ATA_port_s* psPort );

/* ata_probe.c */
status_t ata_probe_configure_drive( ATA_port_s* psPort );
void ata_probe_port( ATA_port_s* psPort );

/* ata_io.c */
status_t ata_io_wait( ATA_port_s* psPort, int nMask, int nValue );
status_t ata_io_wait_alt( ATA_port_s* psPort, int nMask, int nValue );
int ata_io_read( ATA_port_s* psPort, void *pBuffer, int nBytes );
int ata_io_write( ATA_port_s* psPort, void *pBuffer, int nBytes );

/* ata_command.c */
void ata_cmd_init_buffer( ATA_controller_s* psPort, int nChannel );
void ata_cmd_init( ATA_cmd_s* psCmd, ATA_port_s* psPort );
void ata_cmd_free( ATA_cmd_s* psCmd );
void ata_cmd_queue( ATA_port_s* psPort, ATA_cmd_s* psCmd );
int32 ata_cmd_thread( void* pData );
void ata_cmd_atapi( ATA_cmd_s* psCmd );
void ata_cmd_ata( ATA_cmd_s* psCmd );

/* ata_drive.c */
void ata_drive_add( ATA_port_s* psPort );

/* atapi_drive.c */
void atapi_drive_add( ATA_port_s* psPort );

#endif



















