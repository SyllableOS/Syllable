/*
 *  lfndos - AtheOS filesystem that use MS-DOS to store/retrive files
 *  Copyright (C) 1999  Kurt Skauen
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
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

#include <posix/dirent.h>
#include <posix/stat.h>
#include <posix/fcntl.h>

#include <atheos/types.h>
#include <atheos/time.h>
#include <atheos/kernel.h>

#include "dos.h"
#include "inodeext.h"

#include <macros.h>

/*
^DOS Error Codes

	Of the following error codes, only error codes 1-12 are
	returned in AX upon exit from interrupt 21 or 24;  The rest
	are obtained by issuing the "get extended error" function
	call;  see ~INT 21,59~

	01  Invalid function number
	02  File not found
	03  Path not found
	04  Too many open files (no handles left)
	05  Access denied
	06  Invalid handle
	07  Memory control blocks destroyed
	08  Insufficient memory
	09  Invalid memory block address
	0A  Invalid environment
	0B  Invalid format
	0C  Invalid access mode (open mode is invalid)
	0D  Invalid data
	0E  Reserved
	0F  Invalid drive specified
	10  Attempt to remove current directory
	11  Not same device
	12  No more files
	13  Attempt to write on a write-protected diskette
	14  Unknown unit
	15  Drive not ready
	16  Unknown command
	17  CRC error
	18  Bad request structure length
	19  Seek error
	1A  Unknown media type
	1B  Sector not found
	1C  Printer out of paper
	1D  Write fault
	1E  Read fault
	1F  General failure
	20  Sharing violation
	21  Lock violation
	22  Invalid disk change
	23  FCB unavailable
	24  Sharing buffer overflow
	25  Reserved
	26  Unable to complete file operation (DOS 4.x)
	27-31 Reserved
	32  Network request not supported
	33  Remote computer not listening
	34  Duplicate name on network
	35  Network name not found
	36  Network busy
	37  Network device no longer exists
	38  NetBIOS command limit exceeded
	39  Network adapter error
	3A  Incorrect network response
	3B  Unexpected network error
	3C  Incompatible remote adapter
	3D  Print queue full
	3E  No space for print file
	3F  Print file deleted
	40  Network name deleted
	41  Access denied
	42  Network device type incorrect
	43  Network name not found
	44  Network name limit exceeded
	45  NetBIOS session limit exceeded
	46  Temporarily paused
	47  Network request not accepted
	48  Print or disk redirection is paused
	49-4F Reserved
	50  File already exists
	51  Reserved
	52  Cannot make directory entry
	53  Fail on INT 24
	54  Too many redirections
	55  Duplicate redirection
	56  Invalid password
	57  Invalid parameter
	58  Network device fault
	59  Function not supported by network (DOS 4.x)
	5A  Required system component not installed (DOS 4.x)


^DOS Error Code/Classes

%	Error Classes

	01  Out of resource, out of space, channel, etc
	02  Temporary situation, not an error, ex: file lock
	03  Authorization, permission denied
	04  Internal, system detected internal error
	05  Hardware failure, serious problem related to hardware
	06  System failure, ex: invalid configuration
	07  Application error, inconsistent request
	08  Not found, file/item not found
	09  Bad format, file/item in invalid format
	0A  Locked, file/item interlocked
	0B  Media failure, ECC/CRC error, wrong or bad disk
	0C  Already exists, collision with existing item
	0D  Unknown, classification doesn't exist or is inappropriate
	
*/
extern struct	llfuncs	*llfuncs;
static char* pReadWriteBuf = NULL;

int	g_hDbSem = -1;

static int g_nOpenFiles = 0;

typedef struct {
    char	d_dta[ 21 ];		/* disk transfer area */
    char	d_first;		/* flag for 1st time */
    struct	kernel_dirent	sDirent;
} __DIR;

/******************************************************************************
 ******************************************************************************/


static time_t	StupidDateToSmartDate( int16 wData, int16 wTime )
{
    ClockTime_s sTime;

    sTime.tm_sec	= (wTime & 0x001F) * 2;
    sTime.tm_min	= wTime >> 5 & 0x003F;
    sTime.tm_hour	= wTime >> 11 & 0x001F;
    sTime.tm_mday	= (wData & 0x001F) - 1;
    sTime.tm_mon	= (wData >> 5 & 0x000F) - 1;
    sTime.tm_year	= (wData >> 9 & 0x007F) + 80;	/* DOS time is releative to 1980, while UNIX time is relative to 1900	*/

    return( ClockToSec( &sTime ) );
}

static uint32 SmartDateToStupidDate( time_t nTime )
{
    uint32      nDosTime;
    uint32      nDosDate;
    ClockTime_s sTime;

    SecToClock( nTime, &sTime );

    nDosTime = (sTime.tm_sec / 2) | (sTime.tm_min << 5) | (sTime.tm_hour << 11);
    nDosDate = (sTime.tm_mday + 1) | ((sTime.tm_mon + 1) << 5) | ((sTime.tm_year - 80) << 9);

    return( nDosTime | (nDosDate << 16) );
}

/******************************************************************************
											Set Disk Transfer Address (DTA)
 ******************************************************************************/

static void SetDTA( void* addr )
{
    struct RMREGS rm;

    memset( &rm, 0, sizeof(struct RMREGS) );

    rm.EAX = 0x1A00;
    rm.DS  = ((uint32)addr) >> 4;
    rm.EDX = ((uint32)addr) & 0x0f;

    realint( 0x21, &rm );
}

int dos_rename( const char *pzOldName, const char* pzNewName )
{
    struct RMREGS rm;
    char*         pzRmOldName;
    char*	  pzRmNewName;

    if ( (pzRmOldName = alloc_real( strlen( pzOldName ) + strlen( pzNewName ) + 2, MEMF_REAL )) )
    {
	pzRmNewName = pzRmOldName + strlen( pzOldName ) + 1;

	strcpy( pzRmOldName, pzOldName );
	strcpy( pzRmNewName, pzNewName );

	memset( &rm, 0, sizeof(struct RMREGS) );

	rm.EAX  = 0x5600;
	rm.DS   = ((uint32)pzRmOldName ) >> 4;
	rm.EDX  = ((uint32)pzRmOldName ) & 0x0f;

	rm.ES  = ((uint32)pzRmNewName ) >> 4;
	rm.EDI = ((uint32)pzRmNewName ) & 0x0f;

	realint( 0x21, &rm );

	free_real( pzRmOldName );

	if ( rm.flags & 0x01 ) {
	    printk( "dos_rename( <%s>, <%s> ) failed with return code %ld\n", pzOldName, pzNewName, rm.EAX & 0xffff );
	}
	return( ( rm.flags & 0x01 ) ? -1 : 0 );
    }
    else
    {
	printk( "ERROR : dos_rename() failed to alloc mem for path\n" );
    }
    return( -1 );
}

/******************************************************************************
 ******************************************************************************/

int dos_create( const char *path )
{
    struct RMREGS rm;
    char*  rmpath;

    if ( (rmpath = alloc_real( strlen( path ) + 1, MEMF_REAL )) )
    {
	strcpy( rmpath, path );
	memset( &rm, 0, sizeof(struct RMREGS) );

	rm.EAX	= 0x3c00;
	rm.ECX	= 0;			/* no attribs set	*/
	rm.DS	= ((uint32)rmpath) >> 4;
	rm.EDX	= ((uint32)rmpath) & 0x0f;

	realint( 0x21, &rm );

	free_real( rmpath );


	if ( ( rm.flags & 0x01 ) == 0 ) {
	    g_nOpenFiles++;
	}

/*		printk( "create( '%s' ) -> %d (%d)\n", path, ( rm.flags & 0x01 ) ? -1 : (rm.EAX & 0xffff), g_nOpenFiles ); */

	return( ( rm.flags & 0x01 ) ? -1 : (rm.EAX & 0xffff) );
    }
    return( -1 );
}

/******************************************************************************
 ******************************************************************************/

int dos_open( const char *path, int access, ... )
{
    int 		cacc = 0;
    struct	RMREGS rm;
    char*		rmpath;

    if ( (access & O_CREAT) && ( !(access & O_RDONLY) ) )	{
	return( dos_create( path ) );
    }
  
    if ( (rmpath = alloc_real( strlen( path ) + 1, MEMF_REAL )) )
    {
	strcpy( rmpath, path );

	if ( access & O_RDONLY ) cacc |= 0x0000;
	if ( access & O_WRONLY ) cacc |= 0x0001;
	if ( access & O_RDWR )	 cacc |= 0x0002;

    
	memset( &rm, 0, sizeof(struct RMREGS) );

	rm.EAX	= 0x3D00 | cacc;
	rm.DS	= ((uint32)rmpath ) >> 4;
	rm.EDX	= ((uint32)rmpath ) & 0x0f;

	realint( 0x21, &rm );

	free_real( rmpath );


	if ( ( rm.flags & 0x01 ) == 0 ) {
	    g_nOpenFiles++;
	} else {
	    printk( "dos_open() failed to open '%s' : %ld\n", path, (rm.EAX & 0xffff) );
	}

/*		printk( "open( '%s' ) -> %d (%d)\n", path, ( rm.flags & 0x01 ) ? -1 : (rm.EAX & 0xffff), g_nOpenFiles ); */

	return( ( rm.flags & 0x01 ) ? -1 : (rm.EAX & 0xffff) );
    }
    else
    {
	printk( "Error : dos_open() failed to alloc memory for path\n" );
    }
    return( -1 );
}

/******************************************************************************
 ******************************************************************************/

int dos_close( int fn )
{
    struct	RMREGS rm;

    memset( &rm, 0, sizeof(struct RMREGS) );

    rm.EAX = 0x3E00;
    rm.EBX = fn;

    if ( -1 != fn ) {
	g_nOpenFiles--;
    }

/*	printk( "close( %d ) (%d)\n", fn, g_nOpenFiles ); */


    realint( 0x21, &rm );

    return( 0 );
}

int dos_setftime( int nFile, time_t nTime )
{
    struct RMREGS rm;
    int    nDosTime = SmartDateToStupidDate( nTime );

    memset( &rm, 0, sizeof(struct RMREGS) );

    rm.EAX	= 0x5701;
    rm.EBX	= nFile;
    rm.ECX	= nDosTime & 0xffff;
    rm.EDX	= nDosTime >> 16;


    rm.ES		= ((uint32)pReadWriteBuf ) >> 4;
    rm.EDI	= ((uint32)pReadWriteBuf ) & 0x0f;	/* Dont know why, but the docs tell that this shold point to a buffer	*/


    realint( 0x21, &rm );

    return( ( rm.flags & 0x01 ) ? -1 : 0 );
}


/******************************************************************************
 ******************************************************************************/

static int realread( int handle, char *buffer, int size  )
{
    struct RMREGS rm;

    memset( &rm, 0, sizeof(struct RMREGS) );

    rm.EAX	= 0x3F00;
    rm.EBX	= handle;
    rm.ECX	= size;

    rm.DS		= ((uint32)buffer ) >> 4;
    rm.EDX	= ((uint32)buffer ) & 0x0f;

    realint( 0x21, &rm );

    return( ( rm.flags & 0x01 ) ? -1 : (rm.EAX & 0xffff) );
}

/******************************************************************************
 ******************************************************************************/

int dos_read( int fn, void *buffer, unsigned size )
{
    int	Bytes;
    int	blksize;

    Bytes=0;

    if ( size == 0 )	return( 0 );

    blksize = size & 0x7fff;

    if ( NULL == pReadWriteBuf ) {
	pReadWriteBuf = alloc_real( 65536, MEMF_REAL );
    }

    Bytes = realread( fn, pReadWriteBuf, min( size, 65536 ) );
    if ( Bytes != -1 ) {
	memcpy( buffer, pReadWriteBuf, Bytes );
    }
    return( Bytes );
}

/******************************************************************************
 ******************************************************************************/

static int realwrite( int handle, char *buffer, int size  )
{
    struct	RMREGS	rm;

    memset( &rm, 0, sizeof(struct RMREGS) );

    rm.EAX = 0x4000;
    rm.EBX = handle;
    rm.ECX = size;

    rm.DS		= ((uint32)buffer) >> 4;
    rm.EDX	= ((uint32)buffer) & 0x0f;

    realint( 0x21, &rm );

    return( ( rm.flags & 0x01 ) ? -1 : (rm.EAX & 0xffff) );
}

/******************************************************************************
 ******************************************************************************/

int dos_write( int fn, const void *buffer, unsigned size )
{
    int	Bytes = 0;
    int	BytesToWrite,BytesWritten;
    int	blksize;
    char*	rmbuff;

    if ( fn == 2 ) {
	printk( "ERROR : Attempt to write to 'STDERR'\n" );
	return( -1 );
    }

    if ( fn > 20 ) {
	printk( "Error: Attempt to write to invalid dos handle %d\n", fn );
	return( -1 );
    }


    if ( fn < 2 ) {
	return( size );
    }

    if ( fn != -1 ) {
	memcpy( pReadWriteBuf, buffer, size );
	Bytes = realwrite( fn, pReadWriteBuf, size );
    }
    return( Bytes );

    if ( fn != -1 )
    {
	if ( size == 0 )	return( 0 );
	blksize = size & 0x7fff;

	for (;;)
	{
	    if ( (rmbuff = alloc_real( blksize, MEMF_REAL )) )	break;
	    if ( blksize < 128 )	return( -1 );
	    blksize -= 128;
	}

	while( Bytes < size )
	{
	    if ( size - Bytes > blksize )
		BytesToWrite = blksize;
	    else
		BytesToWrite = size - Bytes;

	    memcpy( rmbuff, &((char*)buffer)[Bytes], BytesToWrite );
	    BytesWritten = realwrite( fn, rmbuff, BytesToWrite );

	    if ( BytesWritten == -1 )
	    {
		free_real( rmbuff );
		return( -1 );
	    }

	    Bytes += BytesWritten;

	    if ( BytesWritten < BytesToWrite )	break;
	}	/* write loop	*/

	free_real( rmbuff );
	return( Bytes );
    }
    return( -1 );
}

/******************************************************************************
 ******************************************************************************/

long dos_lseek( int fn, long disp, int mode )
{
    struct RMREGS rm;

    memset( &rm, 0, sizeof(struct RMREGS) );

    rm.EAX = 0x4200 | mode;
    rm.EBX = fn;
    rm.ECX = disp>>16;
    rm.EDX = disp & 0xffff;

    realint( 0x21, &rm );

    return( ( rm.flags & 0x01 ) ? -1 : ((rm.EDX << 16 ) | (rm.EAX & 0xffff)) );
}

/******************************************************************************
 ******************************************************************************/


int dos_stat( const char *pth, struct stat *buf )
{
    int		     i;
    char*	     path;
    struct RMREGS    rm;
    struct fileinfo* fi;

    if ( (path = alloc_real( strlen( pth ) + 200, MEMF_REAL )) ) {
	fi = SUMADR( path, strlen( pth ) + 1 );

	for ( i = 0 ; pth[i] ; i++ ) {
	    if ( pth[i] == '/' )
		path[i] = '\\';
	    else
		path[i] = pth[i];
	}
	path[i] = 0;


	strcpy( path, pth );

	if ( path[strlen( pth ) - 1] == '\\' ) {
	    path[ strlen( pth ) - 1 ] = 0;
	}
	memset( &rm, 0, sizeof( struct RMREGS ) );

	rm.EAX = 0x4E00;
	rm.ECX = 0x37;

	rm.DS  = ((uint32)path ) >> 4;
	rm.EDX = ((uint32)path) & 0x0f;

	SetDTA( fi );
	realint( 0x21, &rm );


	buf->st_atime = StupidDateToSmartDate( fi->date, fi->time );
	buf->st_mtime = buf->st_atime;
	buf->st_ctime = buf->st_atime;

	buf->st_mode = (fi->fileattribs & 0x10 ) ? DOS_S_IFDIR : DOS_S_IFREG;
	buf->st_size = fi->size;

	free_real( path );

	if ( !( rm.flags & 0x01 ) ) {
	    return( 0 );
	} else {
	    printk( "dos_stat() failed to stat '%s' : %ld\n", path, rm.EAX & 0xffff );
	}
    }
    return( -1 );
}


DOS_DIR *dos_opendir( const char *pth )
{
    int		   i;
    char*		   path;
    __DIR*	   dirent;
    struct RMREGS	   rm;
    struct fileinfo* fi;


    if ( (path = alloc_real( strlen( pth ) + 210, MEMF_REAL )) )
    {
	fi = SUMADR( path, strlen( pth ) + 10 );


	for ( i = 0 ; pth[i] ; i++ )
	{
	    if ( pth[i] == ',' )
		path[i] = '\\';
	    else
		path[i] = pth[i];
	}

	path[i] = 0;

	if ( (dirent = kmalloc( sizeof( __DIR ) + strlen( path ) + 5, MEMF_KERNEL )) )
	{
	    memset( &rm, 0, sizeof( struct RMREGS ) );

	    rm.EAX = 0x4E00;
	    rm.ECX = 0x37;

	    rm.DS	= ((uint32)path) >> 4;
	    rm.EDX	= ((uint32)path) & 0x0f;


	    if ( path[strlen( path ) -1 ] == '\\' )
		strcat( path, "*.*" );
	    else
		strcat( path, "\\*.*" );

	    strcpy( (char*) (dirent + 1), path );	/* save path for matchnext	*/

	    SetDTA( fi );
	    realint( 0x21, &rm );

	    memcpy( dirent->d_dta, fi, 21 );
	    strncpy( dirent->sDirent.d_name, fi->name, 13 );
	    dirent->sDirent.d_name[12] = 0;
	    dirent->sDirent.d_namlen = strlen( dirent->sDirent.d_name );

	    dirent->d_first = TRUE;

	    free_real( path );

	    if ( !( rm.flags & 0x01 ) ) {
		return( (DOS_DIR*) dirent );
	    } else {
		printk( "dos_opendir() failed to stat '%s' : %ld\n", pth, rm.EAX & 0xffff );
	    }
	    kfree( dirent );
	}
    }
    return( NULL );
}

/******************************************************************************
 ******************************************************************************/

int dos_closedir( DOS_DIR *dir )
{
    kfree( dir );
    return( 0 );
}

/******************************************************************************
 ******************************************************************************/


struct kernel_dirent *dos_readdir( DOS_DIR *pDir )
{
    struct RMREGS	   rm;
    struct fileinfo* fi;
    char*		   path;

    __DIR*	dirent = (__DIR*) pDir;

    if ( FALSE != dirent->d_first )
    {
	dirent->d_first = false;

	return( &dirent->sDirent );
    }

    if ( (path = alloc_real( strlen( (char*) (dirent + 1) ) + 210, MEMF_REAL )) )
    {
	fi = SUMADR( path, strlen( (char*) (dirent + 1) ) + 10 );

	strcpy( path, (char*) (dirent + 1) );

	memset( &rm, 0, sizeof( struct RMREGS ) );

	rm.EAX = 0x4F00;
	rm.ECX = 0x3F;

	rm.DS  = ((uint32)path) >> 4;
	rm.EDX = ((uint32)path) & 0x0f;

	memcpy( fi, dirent->d_dta, 21 );

	SetDTA( fi );
	realint( 0x21, &rm );

	memcpy( dirent->d_dta, fi, 21 );

	strncpy( dirent->sDirent.d_name, fi->name, 13 );
	dirent->sDirent.d_name[12] = 0;
	dirent->sDirent.d_namlen = strlen( dirent->sDirent.d_name );

	free_real( path );

	return( ( rm.flags & 0x01 ) ? 0 : &dirent->sDirent );
    }
    return( NULL );
}

/******************************************************************************
 ******************************************************************************/

int dos_mkdir( const char *path, int ehhh )
{
    struct RMREGS rm;
    char*	rmpath;

    if ( (rmpath = alloc_real( strlen( path ) + 1, MEMF_REAL )) )
    {
	strcpy( rmpath, path );
	memset( &rm, 0, sizeof(struct RMREGS) );

	rm.EAX	= 0x3900;
	rm.DS	= ((uint32)rmpath) >> 4;
	rm.EDX	= ((uint32)rmpath) & 0x0f;

	realint( 0x21, &rm );

	free_real( rmpath );

	return( ( rm.flags & 0x01 ) ? -1 : 0 );
    }
    return( -1 );
}

/******************************************************************************
 ******************************************************************************/

int dos_rmdir( const char *path )
{
    struct RMREGS rm;
    char*	rmpath;

    if ( (rmpath = alloc_real( strlen( path ) + 1, MEMF_REAL )) )
    {
	strcpy( rmpath, path );
	memset( &rm, 0, sizeof(struct RMREGS) );

	rm.EAX	= 0x3A00;
	rm.DS	= ((uint32)rmpath) >> 4;
	rm.EDX	= ((uint32)rmpath) & 0x0f;

	realint( 0x21, &rm );

	free_real( rmpath );

	return( ( rm.flags & 0x01 ) ? -1 : 0 );
    }
    printk( "dos_rmdir() failed to delete %s\n", path );
    return( -1 );
}

/******************************************************************************
 ******************************************************************************/

int dos_unlink( const char *path )
{
    struct RMREGS rm;
    char*	rmpath;

    if ( (rmpath = alloc_real( strlen( path ) + 1, MEMF_REAL )) )
    {
	strcpy( rmpath, path );
	memset( &rm, 0, sizeof(struct RMREGS) );

	rm.EAX	= 0x4100;
	rm.DS	= ((uint32)rmpath) >> 4;
	rm.EDX	= ((uint32)rmpath) & 0x0f;

	realint( 0x21, &rm );

	free_real( rmpath );

	return( ( rm.flags & 0x01 ) ? -1 : 0 );
    }
    printk( "dos_unlink() failed to delete %s\n", path );
    return( -1 );
}
