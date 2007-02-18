/*
 *  The Syllable kernel
 *  Configuration file management
 *  Copyright (C) 2003 The Syllable Team
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

#include <atheos/types.h>
#include <atheos/filesystem.h>
#include <atheos/ctype.h>
#include <atheos/config.h>
#include <atheos/device.h>
#include <posix/errno.h>
#include <posix/unistd.h>
#include <posix/fcntl.h>
#include <macros.h>
#include "inc/sysbase.h"

static uint8 *g_pConfigAddr = NULL;	/* pointer to the configfile bootmodule */
static uint32 g_nConfigSize = 0;	/* Size of the configfile bootmodule */
static int g_nConfigFd = -1;	/* filedescriptor of the configfile when writing */

extern bool g_bDisableKernelConfig;

extern void write_dev_fs_config( void );

/** Write one entryheader into the configfile.
 * \par Description:
 * Writes one entryheader into the kernel configfile. This is only possible if the 
 * kernel has requested this write.
 * \param pzName - Name of the entry.
 * \param nSize - Total size of the data.
 * \return
 * 0 if successful.
 * \sa read_kernel_config_entry()
 * \author Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
status_t write_kernel_config_entry_header( char *pzName, size_t nSize )
{
	const char *pzBreak = "\n";
	char zSize[255];

	if ( g_bDisableKernelConfig || g_nConfigFd < 0 )
		return ( -EIO );
	

	/* Check size */
	if ( nSize > 4 * PAGE_SIZE )
		return ( -ENOMEM );

	write( g_nConfigFd, pzName, strlen( pzName ) );
	write( g_nConfigFd, pzBreak, strlen( pzBreak ) );
	sprintf( zSize, "%i\n", nSize );
	write( g_nConfigFd, zSize, strlen( zSize ) );


	return ( 0 );
}


/** Write data of one entry into the configfile.
 * \par Description:
 * Writes data of one entry into the kernel configfile. This is only possible if the 
 * kernel has requested this write.
 * \param pBuffer - Buffer size.
 * \param nSize - Size of the data.
 * \return
 * 0 if successful.
 * \sa read_kernel_config_entry()
 * \author Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
status_t write_kernel_config_entry_data( uint8 *pBuffer, size_t nSize )
{
	if ( g_bDisableKernelConfig || g_nConfigFd < 0 )
		return ( -EIO );

	write( g_nConfigFd, pBuffer, nSize );
	return ( 0 );
}


/** Read one entry from the configfile.
 * \par Description:
 * Reads one entry from the kernel configfile.
 * \par Note:
 * You have to delete the buffer after usage.
 * \param pzName - Name of the entry in "<>" signs.
 * \param pBuffer - Pointer to the data belonging to this entry.
 * \param pnSize - Pointer to the size of the entry.
 * \return
 * 0 if successful.
 * \sa write_kernel_config_entry()
 * \author Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
status_t read_kernel_config_entry( char *pzName, uint8 **pBuffer, size_t *pnSize )
{
	char *pConfig = (char*)g_pConfigAddr;
	uint32 nRead = 0;
	char zTempLine[255];
	char zName[255];
	bool bReadName = true;
	int nSize = 0;
	
	if( g_bDisableKernelConfig )
		return( -EIO );

	strcpy( zTempLine, "" );

	while ( nRead < g_nConfigSize )
	{
		/* Read one byte */
		if ( *pConfig == '\n' )
		{
			if ( bReadName )
			{
				/* We have the entryname */
				strcpy( zName, zTempLine );
				//printk( "Entry: %s\n", zName );
				bReadName = false;
			}
			else
			{
				/* We have the entrysize */
				nSize = atol( zTempLine );
				//printk( "Size: %i\n", nSize );
				bReadName = true;
				if ( !strcmp( zName, pzName ) )
				{
					/* Copy data into the buffer */
					*pnSize = nSize;
					if ( *pnSize > 0 )
					{
						*pBuffer = kmalloc( *pnSize, MEMF_KERNEL | MEMF_CLEAR );
						memcpy( *pBuffer, pConfig + 1, *pnSize );
					}
					return ( 0 );
				}
				pConfig += nSize;
			}
			strcpy( zTempLine, "" );
		}
		else
		{
			char zTemp[2];

			zTemp[0] = *pConfig;
			zTemp[1] = 0;
			strcat( zTempLine, zTemp );
		}
		pConfig++;
		nRead++;
	}
	return ( -1 );
}

/** Write configfile to disk.
 * \author Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
void write_kernel_config( void )
{

	char zPath[255];	/* /system/config/kernel.cfg */
	
	if( g_bDisableKernelConfig )
		return;

	/* Build path */
	sys_get_system_path( zPath, 256 );
	strcat( zPath, "system/config/kernel.cfg" );

	/* Open file */
	g_nConfigFd = open( zPath, O_WRONLY | O_CREAT | O_TRUNC );

	if ( g_nConfigFd < 0 )
	{
		printk( "Failed to open kernel configfile\n" );
		return;
	}

	/* Write header */
	write_kernel_config_entry_header( "<KERNEL_CONFIG>", 0 );

	/* Write devfs configuration */
	write_dev_fs_config();

	/* Write end */
	write_kernel_config_entry_header( "<END_CONFIG>", 0 );

	/* Close file */
	close( g_nConfigFd );

	g_nConfigFd = -1;
}


/** Load configfile from a bootmodule
 * \author Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
void init_kernel_config( void )
{
	int i;
	
	if( g_bDisableKernelConfig )
		return;

	/* Search bootmodule ( code taken from elf.c ) */
	for ( i = 0; i < get_boot_module_count(); ++i )
	{
		BootModule_s *psModule = get_boot_module( i );
		char zFullPath[1024];
		const char *pzPath;
		int j;

		pzPath = strstr( psModule->bm_pzModuleArgs, "/config/" );
		if ( pzPath == NULL )
		{
			continue;
		}
		strcpy( zFullPath, "/boot/system" );
		j = strlen( zFullPath );
		while ( *pzPath != '\0' && isspace( *pzPath ) == false )
		{
			zFullPath[j++] = *pzPath++;
		}
		zFullPath[j] = '\0';
		while ( isspace( *pzPath ) )
			pzPath++;

		if ( *pzPath != '\0' )
		{
			const char *pzArg = pzPath;

			for ( j = 0;; ++j )
			{
				int nLen;

				if ( pzPath[j] != '\0' && isspace( pzPath[j] ) == false )
				{
					continue;
				}
				nLen = pzPath + j - pzArg;
				get_str_arg( zFullPath, "path=", pzArg, nLen );

				if ( pzPath[j] == '\0' )
				{
					break;
				}
				pzArg = pzPath + j + 1;
			}
		}


		if ( strstr( zFullPath, "/config/kernel.cfg" ) )
		{
			size_t nSize;
			uint8 *pBuffer;

			/* Got it! */
			printk( "Kernel configuration: %s\n", zFullPath );

			g_pConfigAddr = psModule->bm_pAddress;
			g_nConfigSize = psModule->bm_nSize;

			/* Validate config file */
			if ( read_kernel_config_entry( "<KERNEL_CONFIG>", &pBuffer, &nSize ) != 0 || read_kernel_config_entry( "<END_CONFIG>", &pBuffer, &nSize ) != 0 )
			{
				printk( "Kernel configuration corrupted!\n" );
				g_nConfigSize = 0;
			}
		}
	}
}

