/*
 *  The AtheOS kernel
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

#include <posix/errno.h>
#include <posix/stat.h>
#include <posix/fcntl.h>
#include <posix/dirent.h>

#include <atheos/kernel.h>
#include <atheos/device.h>
#include <atheos/timer.h>
#include <atheos/semaphore.h>
#include <atheos/isa_io.h>
#include <atheos/bootmodules.h>


#include <macros.h>


#define BCF_EXTENDED_DISK_ACCESS	0x01 // (0x42-0x44,0x47,0x48)
#define BCF_REMOVABLE_DRIVE_FUNCTIONS	0x02 // (0x45,0x46,0x48,0x49, INT 15/0x52)
#define BCF_ENHANCED_DDRIVE_FUNCTIONS	0x04 // (0x48,0x4e)

// Information flags returned by GET DRIVE PARAMETERS (0x48)

#define DIF_DMA_BND_ERRORS_HANDLED	0x01 // DMA boundary errors handled transparantly.
#define DIF_CSH_INFO_VALID		0x02 // CHS information is valid.
#define DIF_REMOVABLE			0x04 // Removable drive.
#define DIF_WRITE_VERIFY_SUPPORTED	0x08 // Write with verify supported.
#define DIF_HAS_CHANGE_LINE		0x10 // Drive has change-line support. (Removable only)
#define DIF_CAN_LOCK			0x20 // Drive can be locked. (Removable only)
#define DIF_CHS_MAXED_OUT		0x40 // CHS info set to maximum supported values, not current media.




static int bdd_open( void* pNode, uint32 nFlags, void **ppCookie );
static int bdd_close( void* pNode, void* pCookie );
static int bdd_read( void* pNode, void* pCookie, off_t nPos, void* pBuf, size_t nLen );
static int bdd_write( void* pNode, void* pCookie, off_t nPos, const void* pBuf, size_t nLen );
static int bdd_readv( void* pNode, void* pCookie, off_t nPos, const struct iovec* psVector, size_t nCount );
static int bdd_writev( void* pNode, void* pCookie, off_t nPos, const struct iovec* psVector, size_t nCount );
static status_t bdd_ioctl( void* pNode, void* pCookie, uint32 nCommand, void* pArgs, bool bFromKernel );

/*
Bitfields for IBM/MS INT 13 Extensions information flags:

0      DMA boundary errors handled transparently
1      cylinder/head/sectors-per-track information is valid
2      removable drive
3      write with verify supported
4      drive has change-line support (required if drive >= 80h is removable)
5      drive can be locked (required if drive >= 80h is removable)
6      CHS information set to maximum supported values, not current media
15-7   reserved (0)
*/

#define BDD_BUFFER_SIZE 65536

static char*	g_pCmdBuffer;
static char*	g_pDataBuffer;
static char*	g_pRawDataBuffer; // Same as g_pDataBuffer but is not aligned to 16 byte boundary
static sem_id	g_hRelBufLock;
static ktimer_t g_hMotorTimer;
static int	g_nDevID = -1;

typedef struct
{
    uint16 nStructSize;
    uint16 nFlags;
    uint32 nCylinders;
    uint32 nHeads;
    uint32 nSectors;
    uint64 nTotSectors;
    uint16 nBytesPerSector;
} DriveParams_s;


#define	BDD_ROOT_INODE	0x124131

typedef struct _BddInode BddInode_s;
struct _BddInode
{
    BddInode_s*	bi_psFirstPartition;
    BddInode_s*	bi_psNext;
    int		bi_nDeviceHandle;
    char	bi_zName[16];
    atomic_t	bi_nOpenCount;
    int		bi_nDriveNum;	/* The bios drive number (0x80-0xff) */
    int		bi_nNodeHandle;
    int		bi_nPartitionType;
    int		bi_nSectorSize;
    int		bi_nSectors;
    int		bi_nCylinders;
    int		bi_nHeads;
    off_t	bi_nStart;
    off_t	bi_nSize;
    bool	bi_bRemovable;
    bool	bi_bLockable;
    bool	bi_bHasChangeLine;
    bool	bi_bCSHAddressing;
    bool	bi_bTruncateToCyl;
};


DeviceOperations_s g_sOperations = {
    bdd_open,
    bdd_close,
    bdd_ioctl,
    bdd_read,
    bdd_write,
    bdd_readv,
    bdd_writev,
    NULL,	// dop_add_select_req
    NULL	// dop_rem_select_req
};



static void motor_off_timer( void* pData )
{
    outb( 0x0c, 0x3f2 ); // Stop the floppy motor.
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int get_drive_capabilities( int nDrive )
{
    struct RMREGS rm;

    memset( &rm, 0, sizeof( rm ) );

    rm.EAX	= 0x4100;
    rm.EBX	= 0x55AA;
    rm.EDX	= nDrive;

    realint( 0x13, &rm );

    if ( rm.flags & 0x01 ) {
	return( 0 );
    }
    if ( (rm.EBX & 0xffff) != 0xAA55 ) {
	return( 0 );
    }
    return( rm.ECX & 0xffff );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int get_drive_params_csh( int nDrive, DriveParams_s* psParams )
{
    struct RMREGS rm;

    memset( &rm, 0, sizeof( rm ) );

    rm.EAX	= 0x0800;
    rm.EDX	= nDrive;

    realint( 0x13, &rm );
    
    psParams->nStructSize     = sizeof(DriveParams_s);
    psParams->nFlags	      = 0;
    psParams->nCylinders      = (((rm.ECX >> 8) & 0xff) | ((rm.ECX & 0xc0) << 2)) + 1;
    psParams->nHeads	      = ((rm.EDX >> 8) & 0xff) + 1;
    psParams->nSectors	      = rm.ECX & 0x3f;
    psParams->nTotSectors     = psParams->nCylinders * psParams->nHeads * psParams->nSectors;
    psParams->nBytesPerSector = 512;
/*    if ( nDrive < 2 ) {
	switch( rm.EBX & 0xff )
	{
	    case 3:
		psParams->nCylinders = 40;
		psParams->nHeads     = 2;
		psParams->nSectors   = 18;
		break;
	    case 4:
		psParams->nCylinders = 80;
		psParams->nHeads     = 2;
		psParams->nSectors   = 18;
		break;
	}
	psParams->nTotSectors     = psParams->nCylinders * psParams->nHeads * psParams->nSectors;
	psParams->nBytesPerSector = 512;
    }*/

//  printk( "Number of drives = %d\n", (rm.EDX & 0xff) );
  
    return( ( rm.flags & 0x01 ) ? -1 : 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int get_drive_params_lba( int nDrive, DriveParams_s* psParams )
{
    struct RMREGS rm;

//    if ( get_drive_params_csh( nDrive, psParams ) < 0 ) {
//	return( -1 );
//    }
    
    memset( &rm, 0, sizeof( rm ) );

    rm.EAX	= 0x4800;
    rm.EDX	= nDrive;

    rm.DS	= ((uint) (g_pCmdBuffer) ) >> 4;
    rm.ESI	= ((uint) (g_pCmdBuffer) ) & 0x0f;

    ((uint16*)(g_pCmdBuffer))[0] = 0x2c;	/* Size */

    realint( 0x13, &rm );
  
    psParams->nStructSize = ((uint16*)(g_pCmdBuffer + 0))[0];
    psParams->nFlags      = ((uint16*)(g_pCmdBuffer + 2))[0];
    psParams->nCylinders  = ((uint32*)(g_pCmdBuffer + 4))[0];
    psParams->nHeads      = ((uint32*)(g_pCmdBuffer + 8))[0];
    psParams->nSectors    = ((uint32*)(g_pCmdBuffer + 12))[0];
    psParams->nTotSectors = ((uint64*)(g_pCmdBuffer + 16))[0];
    psParams->nBytesPerSector  = ((uint16*)(g_pCmdBuffer + 24))[0];

    return( ( rm.flags & 0x01 ) ? -1 : 0 );
}


/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int read_sectors_lba( BddInode_s* psInode, void* pBuffer, int64 nSector, int nSectorCount )
{
    struct RMREGS rm;
    int	 seg = ((uint) (pBuffer) ) >> 4;
    int	 off = ((uint) (pBuffer) ) & 0x0f;
    int  nRetryCount = 0;
    
    
    if ( psInode->bi_bTruncateToCyl && psInode->bi_nSectors > 1 ) {
	if ( nSector / psInode->bi_nSectors != (nSector+nSectorCount-1) / psInode->bi_nSectors ) {
	    nSectorCount = psInode->bi_nSectors - nSector % psInode->bi_nSectors;
	}
    }
    if ( nSectorCount > 128 ) {
	nSectorCount = 128;
    }
    
retry:
    memset( &rm, 0, sizeof( rm ) );

    rm.EAX	= 0x4200;
    rm.EDX	= psInode->bi_nDriveNum;
	
    ((uint8*)(g_pCmdBuffer  + 0))[0] = 0x10;	/* Size */
    ((uint8*)(g_pCmdBuffer  + 1))[0] = 0x00;	/* reserved */
    ((uint16*)(g_pCmdBuffer + 2))[0] = nSectorCount;
    ((uint32*)(g_pCmdBuffer + 4))[0] = (seg << 16) | off;
    ((uint64*)(g_pCmdBuffer + 8))[0] = nSector;

	
    rm.DS  = ((uint) (g_pCmdBuffer) ) >> 4;
    rm.ESI = ((uint) (g_pCmdBuffer) ) & 0x0f;

    realint( 0x13, &rm );

    Schedule();
//  printk( "Status = %lx (%lx)\n", ((rm.EAX >> 8) & 0xff), g_pDataBuffer );

    if ( ( rm.flags & 0x01 ) ) {
    	if ( nRetryCount++ < 3 ) {
	    if ( nRetryCount == 2 ) {
		nSectorCount = 1;
		printk( "Error: read_sectors_lba() failed to read %d sectors (%lx), try with 1\n", nSectorCount, ((rm.EAX >> 8) & 0xff) );
	    }
	    goto retry;
	}
    } else {
	if ( nRetryCount > 0 ) {
	    psInode->bi_bTruncateToCyl = true;
	    printk( "*** LBA BIOS SEEMS TO HAVE PROBLEMS WITH IO CROSSING CYLINDER BOUNDARIES ***" );
	    printk( "*** WILL TRUNCATE FURTHER REQUESTS TO FIT WITHIN CYLINDER BOUNDARIES     ***" );
	}
    }
    return( ( rm.flags & 0x01 ) ? -EIO : nSectorCount );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int write_sectors_lba( BddInode_s* psInode, const void* pBuffer, off_t nSector, int nSectorCount )
{
    struct RMREGS rm;
    int	 seg = ((uint) (pBuffer) ) >> 4;
    int	 off = ((uint) (pBuffer) ) & 0x0f;
    int  nRetryCount = 0;
    
    if ( psInode->bi_bTruncateToCyl && psInode->bi_nSectors > 1 ) {
	if ( nSector / psInode->bi_nSectors != (nSector+nSectorCount-1) / psInode->bi_nSectors ) {
	    nSectorCount = psInode->bi_nSectors - nSector % psInode->bi_nSectors;
	}
    }
    if ( nSectorCount > 128 ) {
	nSectorCount = 128;
    }
retry:
    memset( &rm, 0, sizeof( rm ) );

    rm.EAX	= 0x4301; // Write sector without verify
    rm.EDX	= psInode->bi_nDriveNum;
	
    ((uint8*)(g_pCmdBuffer  + 0))[0] = 0x10;	/* Size */
    ((uint8*)(g_pCmdBuffer  + 1))[0] = 0x00;	/* reserved */
    ((uint16*)(g_pCmdBuffer + 2))[0] = nSectorCount;
    ((uint32*)(g_pCmdBuffer + 4))[0] = (seg << 16) | off;
    ((uint64*)(g_pCmdBuffer + 8))[0] = nSector;

    rm.DS  = ((uint) (g_pCmdBuffer) ) >> 4;
    rm.ESI = ((uint) (g_pCmdBuffer) ) & 0x0f;

    realint( 0x13, &rm );
    Schedule();
    if ( rm.flags & 0x01 ) {
	if ( nRetryCount++ < 3 ) {
	    if ( nRetryCount == 2 ) {
		nSectorCount = 1;
		printk( "Error: write_sectors_lba() failed to write %d sectors (%lx), try with 1\n", nSectorCount, ((rm.EAX >> 8) & 0xff) );
	    }
	    goto retry;
	}
	printk( "Error: write_sectors_lba() failed to write %d sectors at pos %Ld (Status = %lx)\n", nSectorCount, nSector, ((rm.EAX >> 8) & 0xff) );
	return( -EIO );
    } else {
	if ( nRetryCount > 0 ) {
	    psInode->bi_bTruncateToCyl = true;
	    printk( "*** LBA BIOS SEEMS TO HAVE PROBLEMS WITH IO CROSSING CYLINDER BOUNDARIES ***" );
	    printk( "*** WILL TRUNCATE FURTHER REQUESTS TO FIT WITHIN CYLINDER BOUNDARIES     ***" );
	}
	return( nSectorCount );
    }
}

static int read_sectors_csh( BddInode_s* psInode, const void* pBuffer, int64 nLBASector, int nSectorCount )
{
    struct RMREGS rm;
    int64 nSector;
    int64 nCyl;
    int64 nHead;
    int64 nTmp;
    int   nRetryCount = 0;

    nCyl    = nLBASector / ((int64)psInode->bi_nSectors * psInode->bi_nHeads);
    nTmp    = nLBASector % ((int64)psInode->bi_nSectors * psInode->bi_nHeads);
    nHead   = nTmp / (int64)psInode->bi_nSectors;
    nSector = (nTmp % (int64)psInode->bi_nSectors) + 1;

    if ( nSectorCount > ((int64)psInode->bi_nSectors) - nSector + 1LL  ) {
	nSectorCount = ((int64)psInode->bi_nSectors) - nSector + 1LL;
    }

    if ( (nLBASector + nSectorCount - 1) / (psInode->bi_nSectors * psInode->bi_nHeads) != nCyl ) {
	printk( "read_sectors_csh() Failed to reduse sectors at %Ld:%Ld:%Ld (%d)\n",
		nCyl, nSector, nHead, nSectorCount );
	nSectorCount = 1;
    }

    kassertw( nSectorCount > 0 );
    kassertw( nSector > 0 && nSector < 64 );
retry:  
    memset( &rm, 0, sizeof( rm ) );

    if ( nSectorCount > 255 ) {
	printk( "read_sectors_csh() attempt to read more than 255 sectors (%d)\n", nSectorCount );
	return( -EINVAL );
    }
  
    rm.EAX = 0x0200 | nSectorCount;
    rm.ECX = ((nCyl & 0xff) << 8) | (nSector & 0x3f) | ((nCyl & 0x300) >> 2);
    rm.EDX = (nHead << 8) | psInode->bi_nDriveNum;
  
    rm.ES  = ((uint32)pBuffer) >> 4;
    rm.EBX = ((uint32)pBuffer) & 0x0f;

    if ( nCyl < 0 || nCyl >= 1024 ) {
	printk( "read_sectors_csh() Invalid cylinder number %Ld\n", nCyl );
	printk( "LBA = %Ld\n", nLBASector );
    }
  
    if ( nCyl    != (((rm.ECX >> 8) & 0xff) | ((rm.ECX & 0xc0) << 2)) ) {
	printk( "read_sectors_csh() Failed to split cylinder # %Ld, got %ld\n",
		nCyl, (((rm.ECX >> 8) & 0xff) | ((rm.ECX & 0xc0) << 2)) );
    }
  
    kassertw( nHead   == ((rm.EDX >> 8) & 0xff) );
    kassertw( nSector == (rm.ECX & 0x3f) );

    if ( psInode->bi_nDriveNum < 2 ) {
	start_timer( g_hMotorTimer, NULL, NULL, 0LL, true );
    }
    
    realint( 0x13, &rm );
    Schedule();
    if ( psInode->bi_nDriveNum < 2 ) {
	start_timer( g_hMotorTimer, motor_off_timer, NULL, 4000000LL, true );
    }
    if ( rm.flags & 0x01 ) {
    	if ( nRetryCount++ < 3 ) {
	    if ( psInode->bi_nDriveNum < 2 ) {
		memset( &rm, 0, sizeof( rm ) );
		rm.EAX = 0x0000; // Reset the floppy drive
		rm.EDX = psInode->bi_nDriveNum;
		realint( 0x13, &rm );
	    }
	    if ( nRetryCount == 2 ) {
		nSectorCount = 1;
		printk( "Error: read_sectors_csh() failed to read %d sectors (%lx), try with 1\n", nSectorCount, ((rm.EAX >> 8) & 0xff) );
	    }
	    goto retry;
	}
	
	printk( "Read sector %Ld (%Ld:%Ld:%Ld) (%d:%d)\n",
		nLBASector, nCyl, nHead, nSector, psInode->bi_nHeads, psInode->bi_nSectors );
	printk( "Status = %lx (%p)\n", ((rm.EAX >> 8) & 0xff), pBuffer );
    }
    return( ( rm.flags & 0x01 ) ? -EIO : nSectorCount );
}

static int write_sectors_csh( BddInode_s* psInode, const void* pBuffer, int64 nLBASector, int nSectorCount )
{
    struct RMREGS rm;
    int64	 nSector;
    int64    nCyl;
    int64	 nHead;
    int64	 nTmp;
    int          nRetryCount = 0;

    nCyl    = nLBASector / ((int64)psInode->bi_nSectors * psInode->bi_nHeads);
    nTmp    = nLBASector % ((int64)psInode->bi_nSectors * psInode->bi_nHeads);
    nHead   = nTmp / (int64)psInode->bi_nSectors;
    nSector = (nTmp % (int64)psInode->bi_nSectors) + 1;

    if ( nSectorCount > ((int64)psInode->bi_nSectors) - nSector + 1LL ) {
	nSectorCount = ((int64)psInode->bi_nSectors) - nSector + 1LL;
    }

    if ( (nLBASector + nSectorCount - 1) / (psInode->bi_nSectors * psInode->bi_nHeads) != nCyl ) {
	printk( "write_sectors_csh() Failed to reduse sectors at %Ld:%Ld:%Ld (%d)\n",
		nCyl, nSector, nHead, nSectorCount );
	nSectorCount = 1;
    }
  
    kassertw( nSectorCount > 0 );
retry:    
    memset( &rm, 0, sizeof( rm ) );

    if ( nSectorCount > 255 ) {
	printk( "write_sectors_csh() attempt to write more than 255 sectors (%d)\n", nSectorCount );
	return( -EINVAL );
    }
  
    rm.EAX = 0x0300 | nSectorCount;
    rm.ECX = ((nCyl & 0xff) << 8) | (nSector & 0x3f) | ((nCyl & 0x300) >> 2);
    rm.EDX = (nHead << 8) | psInode->bi_nDriveNum;

    if ( nCyl < 0 || nCyl >= 1024 ) {
	printk( "write_sectors_csh() Invalid cylinder number %Ld\n", nCyl );
//      return( -EINVAL );
    }
  
    if ( nCyl != (((rm.ECX >> 8) & 0xff) | ((rm.ECX & 0xc0) << 2)) ) {
	printk( "write_sectors_csh() Failed to split cylinder # %Ld, got %ld\n", nCyl, (((rm.ECX >> 8) & 0xff) | ((rm.ECX & 0xc0) << 2)) );
    }
  
    rm.ES  = ((uint32)pBuffer) >> 4;
    rm.EBX = ((uint32)pBuffer) & 0x0f;

    if ( psInode->bi_nDriveNum < 2 ) {
	start_timer( g_hMotorTimer, NULL, NULL, 0LL, true );
    }
    
    realint( 0x13, &rm );
    Schedule();
    
    if ( psInode->bi_nDriveNum < 2 ) {
	start_timer( g_hMotorTimer, motor_off_timer, NULL, 2000000LL, true );
    }
    
    if ( rm.flags & 0x01 ) {
    	if ( nRetryCount++ < 3 ) {
	    if ( psInode->bi_nDriveNum < 2 ) {
		memset( &rm, 0, sizeof( rm ) );
		rm.EAX = 0x0000; // Reset the floppy drive
		rm.EDX = psInode->bi_nDriveNum;
		realint( 0x13, &rm );
	    }
	    if ( nRetryCount == 2 ) {
		nSectorCount = 1;
		printk( "Error: write_sectors_csh() failed to write %d sectors (%lx), try with 1\n", nSectorCount, ((rm.EAX >> 8) & 0xff) );
	    }
	    goto retry;
	}
	printk( "Write sector %Ld (%Ld:%Ld:%Ld) (%d:%d)\n",
		nLBASector, nCyl, nHead, nSector, psInode->bi_nHeads, psInode->bi_nSectors );
	printk( "Status = %lx (%p)\n", ((rm.EAX >> 8) & 0xff), pBuffer );
    }
  
    return( ( rm.flags & 0x01 ) ? -EIO : nSectorCount );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int bdd_open( void* pNode, uint32 nFlags, void **ppCookie )
{
    BddInode_s* psInode = pNode;

    LOCK( g_hRelBufLock );
    atomic_inc( &psInode->bi_nOpenCount );
    UNLOCK( g_hRelBufLock );
    return( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int bdd_close( void* pNode, void* pCookie )
{
    BddInode_s* psInode = pNode;

    LOCK( g_hRelBufLock );
    atomic_dec( &psInode->bi_nOpenCount );
    UNLOCK( g_hRelBufLock );
    return( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int bdd_read( void* pNode, void* pCookie, off_t nPos, void* pBuf, size_t nLen )
{
    BddInode_s*  psInode  = pNode;
    int nError;
    int nBytesLeft;
  
    if ( nPos & (512 - 1) ) {
	printk( "Error: bdd_read() position has bad alignment %Ld\n", nPos );
	return( -EINVAL );
    }
    if ( nLen & (512 - 1) ) {
	printk( "Error: bdd_read() length has bad alignment %d\n", nLen );
	return( -EINVAL );
    }
    if ( nPos >= psInode->bi_nSize ) {
	printk( "Warning: bdd_read() Request outside partiton %Ld\n", nPos );
	return( 0 );
    }
    if ( nPos + nLen > psInode->bi_nSize ) {
	printk( "Warning: bdd_read() Request truncated from %d to %Ld\n", nLen, (psInode->bi_nSize - nPos) );
	nLen = psInode->bi_nSize - nPos;
    }
    nBytesLeft = nLen;
    nPos += psInode->bi_nStart;
  
    while ( nBytesLeft > 0 ) {
	int nCurSize = min( BDD_BUFFER_SIZE, nBytesLeft );
    
	if ( nPos < 0 ) {
	    printk( "bdd_read() vierd pos = %Ld\n", nPos );
	    kassertw(0);
	}
	LOCK( g_hRelBufLock );
	if ( psInode->bi_bCSHAddressing ) {
	    nError = read_sectors_csh( psInode, g_pDataBuffer, (nPos / 512), nCurSize / 512 );
	} else {
	    nError = read_sectors_lba( psInode, g_pDataBuffer, (nPos / 512), nCurSize / 512 );
	}
	memcpy( pBuf, g_pDataBuffer, nCurSize );
	UNLOCK( g_hRelBufLock );
	if ( nError < 0 ) {
	    printk( "bdd_read() failed to read %d sectors from drive %x\n",
		    (nCurSize / 512), psInode->bi_nDriveNum );
	    return( nError );
	}
	nCurSize = nError * 512;
	pBuf = ((uint8*)pBuf) + nCurSize;
	nPos       += nCurSize;
	nBytesLeft -= nCurSize;
    }
    return( nLen );
}



static size_t bdd_read_partition_data( void* pCookie, off_t nOffset, void* pBuffer, size_t nSize )
{
    return( bdd_read( pCookie, NULL, nOffset, pBuffer, nSize ) );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int bdd_write( void* pNode, void* pCookie, off_t nPos, const void* pBuf, size_t nLen )
{
    BddInode_s*  psInode  = pNode;
    int nBytesLeft;
    int nError;

    if ( nPos & (512 - 1) ) {
	printk( "Error: bdd_write() position has bad alignment %Ld\n", nPos );
	return( -EINVAL );
    }

    if ( nLen & (512 - 1) ) {
	printk( "Error: bdd_write() length has bad alignment %d\n", nLen );
	return( -EINVAL );
    }

    if ( nPos >= psInode->bi_nSize ) {
	printk( "Warning: bdd_write() Request outside partiton : %Ld\n", nPos );
	return( 0 );
    }
    if ( nPos + nLen > psInode->bi_nSize ) {
	printk( "Warning: bdd_write() Request truncated from %d to %Ld\n", nLen, (psInode->bi_nSize - nPos) );
	nLen = psInode->bi_nSize - nPos;
    }
    nBytesLeft = nLen;
    nPos += psInode->bi_nStart;
  
    while ( nBytesLeft > 0 ) {
	int nCurSize = min( BDD_BUFFER_SIZE, nBytesLeft );
	LOCK( g_hRelBufLock );
	memcpy( g_pDataBuffer, pBuf, nCurSize );
	if ( psInode->bi_bCSHAddressing ) {
	    nError = write_sectors_csh( psInode, g_pDataBuffer, nPos / 512, nCurSize / 512 );
	} else {
	    nError = write_sectors_lba( psInode, g_pDataBuffer, nPos / 512, nCurSize / 512 );
	}
	UNLOCK( g_hRelBufLock );
	if ( nError < 0 ) {
	    printk( "bdd_write() failed to write %d sectors to drive %x\n",
		    (nCurSize / 512), psInode->bi_nDriveNum );
	    return( nError );
	}
	nCurSize = nError * 512;
	pBuf = ((uint8*)pBuf) + nCurSize;
	nPos       += nCurSize;
	nBytesLeft -= nCurSize;
    }
    return( nLen );
}



static int bdd_readv( void* pNode, void* pCookie, off_t nPos, const struct iovec* psVector, size_t nCount )
{
    BddInode_s*  psInode  = pNode;
    int nError;
    int nBytesLeft;
    int nLen = 0;
    int i;
    int nCurVec;
    char* pCurVecPtr;
    int   nCurVecLen;
    
    for ( i = 0 ; i < nCount ; ++i ) {
	nLen += psVector[i].iov_len;
    }
    if ( nLen <= 0 ) {
	return( 0 );
    }
    if ( nPos & (512 - 1) ) {
	printk( "Error: bdd_readv() position has bad alignment %Ld\n", nPos );
	return( -EINVAL );
    }
    if ( nLen & (512 - 1) ) {
	printk( "Error: bdd_readv() length has bad alignment %d\n", nLen );
	return( -EINVAL );
    }
    
    if ( nPos >= psInode->bi_nSize ) {
	printk( "Warning: bdd_readv() Request outside partiton : %Ld\n", nPos );
	return( 0 );
    }

    if ( nPos + nLen > psInode->bi_nSize ) {
	printk( "Warning: bdd_readv() Request truncated from %d to %Ld\n", nLen, (psInode->bi_nSize - nPos) );
	nLen = psInode->bi_nSize - nPos;
    }
    nBytesLeft = nLen;
    nPos += psInode->bi_nStart;

    pCurVecPtr = psVector[0].iov_base;
    nCurVecLen = psVector[0].iov_len;
    nCurVec = 1;

//    printk( "Read %d bytes from %d (%d segs)\n", nLen, nPos, nCount );
    LOCK( g_hRelBufLock );
    while ( nBytesLeft > 0 ) {
	int nCurSize = min( BDD_BUFFER_SIZE, nBytesLeft );
	char* pSrcPtr = g_pDataBuffer;
	
	if ( nPos < 0 ) {
	    printk( "bdd_readv() vierd pos = %Ld\n", nPos );
	    kassertw(0);
	}
	if ( psInode->bi_bCSHAddressing ) {
	    nError = read_sectors_csh( psInode, g_pDataBuffer, (nPos / 512), nCurSize / 512 );
	} else {
	    nError = read_sectors_lba( psInode, g_pDataBuffer, (nPos / 512), nCurSize / 512 );
	}
	
	nCurSize = nError * 512;

//	printk( "%d bytes read from disk\n", nCurSize );

	if ( nError < 0 ) {
	    printk( "bdd_readv() failed to read %d sectors from drive %x\n",
		    (nCurSize / 512), psInode->bi_nDriveNum );
	    nLen = nError;
	    goto error;
	}

	nPos       += nCurSize;
	nBytesLeft -= nCurSize;
	
	while ( nCurSize > 0 ) {
	    int nSegSize = min( nCurSize, nCurVecLen );
	    
//	    printk( "%d copyed to buffer\n", nSegSize );
	    memcpy( pCurVecPtr, pSrcPtr, nSegSize );
	    pSrcPtr += nSegSize;
	    pCurVecPtr += nSegSize;
	    nCurVecLen -= nSegSize;
	    nCurSize   -= nSegSize;
	    
	    if ( nCurVecLen == 0 && nCurVec < nCount  ) {
//		printk( "Switch to seg %d\n", nCurVec );
		pCurVecPtr = psVector[nCurVec].iov_base;
		nCurVecLen = psVector[nCurVec].iov_len;
		nCurVec++;
	    }
	}
    }
error:
    UNLOCK( g_hRelBufLock );
    return( nLen );
}

static int bdd_do_write( BddInode_s* psInode, off_t nPos, void* pBuffer, int nLen )
{
    int nBytesLeft;
    int nError;

    nBytesLeft = nLen;
  
    while ( nBytesLeft > 0 ) {
	int nCurSize = min( BDD_BUFFER_SIZE, nBytesLeft );

	if ( psInode->bi_bCSHAddressing ) {
	    nError = write_sectors_csh( psInode, pBuffer, nPos / 512, nCurSize / 512 );
	} else {
	    nError = write_sectors_lba( psInode, pBuffer, nPos / 512, nCurSize / 512 );
	}
	if ( nError < 0 ) {
	    printk( "bdd_do_write() failed to write %d sectors to drive %x\n",
		    nCurSize / 512, psInode->bi_nDriveNum );
	    return( nError );
	}
	nCurSize = nError * 512;
	pBuffer = ((uint8*)pBuffer) + nCurSize;
	nPos       += nCurSize;
	nBytesLeft -= nCurSize;
    }
    return( (nLen < 0) ? nLen : (nLen * 512) );
    
}

static int bdd_writev( void* pNode, void* pCookie, off_t nPos, const struct iovec* psVector, size_t nCount )
{
    BddInode_s*  psInode  = pNode;
    int nBytesLeft;
    int nError;
    int nLen = 0;
    int i;
    int nCurVec;
    char* pCurVecPtr;
    int   nCurVecLen;
    
    for ( i = 0 ; i < nCount ; ++i ) {
	if ( psVector[i].iov_len < 0 ) {
	    printk( "Error: bdd_writev() negative size (%d) in vector %d\n", psVector[i].iov_len, i );
	    return( -EINVAL );
	}
	nLen += psVector[i].iov_len;
    }
    
    if ( nLen <= 0 ) {
	printk( "Warning: bdd_writev() length to small: %d\n", nLen );
	return( 0 );
    }
    if ( nPos < 0 ) {
	printk( "Error: bdd_writev() negative position %Ld\n", nPos );
	return( -EINVAL );
    }
    if ( nPos & (512 - 1) ) {
	printk( "Error: bdd_writev() position has bad alignment %Ld\n", nPos );
	return( -EINVAL );
    }
    if ( nLen & (512 - 1) ) {
	printk( "Error: bdd_writev() length has bad alignment %d\n", nLen );
	return( -EINVAL );
    }

    if ( nPos >= psInode->bi_nSize ) {
	printk( "Warning: bdd_writev() Request outside partiton : %Ld\n", nPos );
	return( 0 );
    }
    if ( nPos + nLen > psInode->bi_nSize ) {
	printk( "Warning: bdd_writev() Request truncated from %d to %Ld\n", nLen, (psInode->bi_nSize - nPos) );
	nLen = psInode->bi_nSize - nPos;
    }
    nBytesLeft = nLen;
    nPos += psInode->bi_nStart;

//    printk( "Write %d bytes to %d (%d segs)\n", nLen, nPos, nCount );

    pCurVecPtr = psVector[0].iov_base;
    nCurVecLen = psVector[0].iov_len;
    nCurVec = 1;
    
    LOCK( g_hRelBufLock );
    while ( nBytesLeft > 0 ) {
	int nCurSize = min( BDD_BUFFER_SIZE, nBytesLeft );
	char* pDstPtr = g_pDataBuffer;
	int   nBytesInBuf = 0;

	while ( nCurSize > 0 ) {
	    int nSegSize = min( nCurSize, nCurVecLen );
	    
	    memcpy( pDstPtr, pCurVecPtr, nSegSize );
	    pDstPtr += nSegSize;
	    pCurVecPtr += nSegSize;
	    nCurVecLen -= nSegSize;
	    nCurSize   -= nSegSize;
	    nBytesInBuf += nSegSize;
	    
	    if ( nCurVecLen == 0 && nCurVec < nCount  ) {
		pCurVecPtr = psVector[nCurVec].iov_base;
		nCurVecLen = psVector[nCurVec].iov_len;
		nCurVec++;
	    }
	}
	nError = bdd_do_write( psInode, nPos, g_pDataBuffer, nBytesInBuf );
	if ( nError < 0 ) {
	    nLen = nError;
	    goto error;
	}
	nCurSize    = nError;
	nPos       += nCurSize;
	nBytesLeft -= nCurSize;
    }
error:
    UNLOCK( g_hRelBufLock );
    return( nLen );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int bdd_decode_partitions( BddInode_s* psInode )
{
    int		    nNumPartitions;
    device_geometry sDiskGeom;
    Partition_s     asPartitions[16];
    BddInode_s*     psPartition;
    BddInode_s**    ppsTmp;
    int		    nError;
    int		    i;

    if ( psInode->bi_nDriveNum < 0x80 || psInode->bi_nPartitionType != 0 ) {
	return( -EINVAL );
    }

    sDiskGeom.sector_count      = psInode->bi_nSize / 512;
    sDiskGeom.cylinder_count    = psInode->bi_nCylinders;
    sDiskGeom.sectors_per_track = psInode->bi_nSectors;
    sDiskGeom.head_count	= psInode->bi_nHeads;
    sDiskGeom.bytes_per_sector  = 512;
    sDiskGeom.read_only 	= false;
    sDiskGeom.removable 	= psInode->bi_bRemovable;

    printk( "Decode partition table for %s\n", psInode->bi_zName );
    
    nNumPartitions = decode_disk_partitions( &sDiskGeom, asPartitions, 16, psInode, bdd_read_partition_data );

    if ( nNumPartitions < 0 ) {
	printk( "   Invalid partition table\n" );
	return( nNumPartitions );
    }
    for ( i = 0 ; i < nNumPartitions ; ++i ) {
	if ( asPartitions[i].p_nType != 0 && asPartitions[i].p_nSize != 0 ) {
	    printk( "   Partition %d : %10Ld -> %10Ld %02x (%Ld)\n", i, asPartitions[i].p_nStart,
		    asPartitions[i].p_nStart + asPartitions[i].p_nSize - 1LL, asPartitions[i].p_nType,
		    asPartitions[i].p_nSize );
	}
    }
    LOCK( g_hRelBufLock );
    nError = 0;

    for ( psPartition = psInode->bi_psFirstPartition ; psPartition != NULL ; psPartition = psPartition->bi_psNext ) {
	bool bFound = false;
	for ( i = 0 ; i < nNumPartitions ; ++i ) {
	    if ( asPartitions[i].p_nStart == psPartition->bi_nStart && asPartitions[i].p_nSize == psPartition->bi_nSize ) {
		bFound = true;
		break;
	    }
	}
	if ( bFound == false && atomic_read( &psPartition->bi_nOpenCount ) > 0 ) {
	    printk( "bdd_decode_partitions() Error: Open partition %s on %s has changed\n", psPartition->bi_zName, psInode->bi_zName );
	    nError = -EBUSY;
	    goto error;
	}
    }

      // Remove deleted partitions from /dev/disk/bios/*/*
    for ( ppsTmp = &psInode->bi_psFirstPartition ; *ppsTmp != NULL ; ) {
	bool bFound = false;
	psPartition = *ppsTmp;
	for ( i = 0 ; i < nNumPartitions ; ++i ) {
	    if ( asPartitions[i].p_nStart == psPartition->bi_nStart && asPartitions[i].p_nSize == psPartition->bi_nSize ) {
		asPartitions[i].p_nSize = 0;
		psPartition->bi_nPartitionType = asPartitions[i].p_nType;
		sprintf( psPartition->bi_zName, "%d", i );
		bFound = true;
		break;
	    }
	}
	if ( bFound == false ) {
	    *ppsTmp = psPartition->bi_psNext;
	    delete_device_node( psPartition->bi_nNodeHandle );
	    kfree( psPartition );
	} else {
	    ppsTmp = &(*ppsTmp)->bi_psNext;
	}
    }

      // Create nodes for any new partitions.
    for ( i = 0 ; i < nNumPartitions ; ++i ) {
	char zNodePath[64];

	if ( asPartitions[i].p_nType == 0 || asPartitions[i].p_nSize == 0 ) {
	    continue;
	}

	psPartition = kmalloc( sizeof( BddInode_s ), MEMF_KERNEL | MEMF_CLEAR | MEMF_OKTOFAILHACK );

	if ( psPartition == NULL ) {
	    printk( "Error: bdd_decode_partitions() no memory for partition inode\n" );
	    break;
	}

	sprintf( psPartition->bi_zName, "%d", i );
	psPartition->bi_nDeviceHandle  = psInode->bi_nDeviceHandle;
	psPartition->bi_nDriveNum      = psInode->bi_nDriveNum;
	psPartition->bi_nSectors       = psInode->bi_nSectors;
	psPartition->bi_nCylinders     = psInode->bi_nCylinders;
	psPartition->bi_nHeads	       = psInode->bi_nHeads;
	psPartition->bi_nSectorSize    = psInode->bi_nSectorSize;
	psPartition->bi_bCSHAddressing = psInode->bi_bCSHAddressing;
	psPartition->bi_bRemovable     = psInode->bi_bRemovable;
	psPartition->bi_bLockable      = psInode->bi_bLockable;
	psPartition->bi_bHasChangeLine = psInode->bi_bHasChangeLine;

	psPartition->bi_nStart = asPartitions[i].p_nStart;
	psPartition->bi_nSize  = asPartitions[i].p_nSize;

	psPartition->bi_psNext = psInode->bi_psFirstPartition;
	psInode->bi_psFirstPartition = psPartition;
	
	strcpy( zNodePath, "disk/bios/" );
	strcat( zNodePath, psInode->bi_zName );
	strcat( zNodePath, "/" );
	strcat( zNodePath, psPartition->bi_zName );
	strcat( zNodePath, "_new" );

	psPartition->bi_nNodeHandle = create_device_node( g_nDevID, psPartition->bi_nDeviceHandle, zNodePath, &g_sOperations, psPartition );
    }

      /* We now have to rename nodes that might have moved around in the table and
       * got new names. To avoid name-clashes while renaming we first give all
       * nodes a unique temporary name before looping over again giving them their
       * final names
       */
    
    for ( psPartition = psInode->bi_psFirstPartition ; psPartition != NULL ; psPartition = psPartition->bi_psNext ) {
	char zNodePath[64];
	strcpy( zNodePath, "disk/bios/" );
	strcat( zNodePath, psInode->bi_zName );
	strcat( zNodePath, "/" );
	strcat( zNodePath, psPartition->bi_zName );
	strcat( zNodePath, "_tmp" );
	rename_device_node( psPartition->bi_nNodeHandle, zNodePath );
    }
    for ( psPartition = psInode->bi_psFirstPartition ; psPartition != NULL ; psPartition = psPartition->bi_psNext ) {
	char zNodePath[64];
	strcpy( zNodePath, "disk/bios/" );
	strcat( zNodePath, psInode->bi_zName );
	strcat( zNodePath, "/" );
	strcat( zNodePath, psPartition->bi_zName );
	rename_device_node( psPartition->bi_nNodeHandle, zNodePath );
    }
    
error:
    UNLOCK( g_hRelBufLock );
    return( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static status_t bdd_ioctl( void* pNode, void* pCookie, uint32 nCommand, void* pArgs, bool bFromKernel )
{
    BddInode_s*  psInode  = pNode;
    int nError = 0;
    
    switch( nCommand )
    {
	case IOCTL_GET_DEVICE_GEOMETRY:
	{
	    device_geometry sGeo;
      
	    sGeo.sector_count      = psInode->bi_nSize / 512;
	    sGeo.cylinder_count    = psInode->bi_nCylinders;
	    sGeo.sectors_per_track = psInode->bi_nSectors;
	    sGeo.head_count	   = psInode->bi_nHeads;
	    sGeo.bytes_per_sector  = 512;
	    sGeo.read_only 	   = false;
	    sGeo.removable 	   = psInode->bi_bRemovable;
	    if ( bFromKernel ) {
		memcpy( pArgs, &sGeo, sizeof(sGeo) );
	    } else {
		nError = memcpy_to_user( pArgs, &sGeo, sizeof(sGeo) );
	    }
	    break;
	}
	case IOCTL_REREAD_PTABLE:
	    nError = bdd_decode_partitions( psInode );
	    break;
	default:
	    printk( "Error: bdd_ioctl() unknown command %ld\n", nCommand );
	    nError = -ENOSYS;
	    break;
    }
    return( nError );
}

static int bdd_create_node( int nDevID, int nDeviceHandle, const char* pzPath, const char* pzHDName, int nDriveNum,
			    int nSec, int nCyl, int nHead, int nSecSize,
			    off_t nStart, off_t nSize, bool bCSH )
{
    BddInode_s* psInode = kmalloc( sizeof( BddInode_s ), MEMF_KERNEL | MEMF_CLEAR | MEMF_OKTOFAILHACK );
    int		nError;
    
    if ( psInode == NULL ) {
	return( -ENOMEM );
    }

    strcpy( psInode->bi_zName, pzHDName );
    psInode->bi_nDriveNum   = nDriveNum;
    psInode->bi_nSectors    = nSec;
    psInode->bi_nCylinders  = nCyl;
    psInode->bi_nHeads	    = nHead;
    psInode->bi_nSectorSize = nSecSize;
    psInode->bi_nStart	    = nStart;
    psInode->bi_nSize	    = nSize;
    psInode->bi_bCSHAddressing = bCSH;
    
//    printk( "  /dev/bdd/%s (%02x) %d sectors of %d bytes (%d:%d:%d)\n", psInode->bi_zName,
//	    psInode->bi_nDriveNum,  sDriveParams.nTotSectors, sDriveParams.nBytesPerSector,
//	    psInode->bi_nHeads, psInode->bi_nCylinders, psInode->bi_nSectors );

    nError = create_device_node( nDevID, nDeviceHandle, pzPath, &g_sOperations, psInode );
    psInode->bi_nNodeHandle = nError;
    if ( nError < 0 ) {
	kfree( psInode );
    }
    return( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int bdd_scan_for_disks( int nDeviceID )
{
    BddInode_s* psInode;
    int	      nError;
    int	      i;
    char	      zNodePath[128];
    int       nHandle;

    const char* const * argv;
    int			argc;
    bool		bCHS;
    
    get_kernel_arguments( &argc, &argv );

    
    printk( "Scan for floppy-drives exported by bios:\n" );
    for ( i = 0x00 ; i < 0x02 ; ++i )
    {
	DriveParams_s sDriveParams;
	char	      zName[16];
	if ( get_drive_params_csh( i, &sDriveParams ) < 0 || sDriveParams.nTotSectors == 0 )  {
	    continue;
	}
	sprintf( zName, "fd%c", 'a' + i );
	strcpy( zNodePath, "disk/bios/" );
	strcat( zNodePath, zName );
	strcat( zNodePath, "/raw" );
	
	nHandle = register_device( "", "bios" );
	claim_device( nDeviceID, nHandle, "Floppy drive", DEVICE_DRIVE );

	nError = bdd_create_node( nDeviceID, nHandle, zNodePath, zName, i, sDriveParams.nSectors, sDriveParams.nCylinders, sDriveParams.nHeads,
				  sDriveParams.nBytesPerSector, 0, sDriveParams.nTotSectors * sDriveParams.nBytesPerSector, true );
    }

    printk( "Scan for hard-drives exported by bios:\n" );
    for ( i = 0x80 ; i < 0x90 ; ++i ) {
	DriveParams_s sDriveParams;
	char	      zName[16];
	char	      zArgName[32];
	char	      zMode[64];
	bool	      bForceCSH = false;
	bool	      bForceLBA = false;
	uint32	      nFlags;
	int	      j;
	bool		  bForceBios = false;
	
	sprintf( zName, "hd%c", 'a' + i - 0x80 );

	strcpy( zArgName, zName );
	strcat( zArgName, "=" );

	for( j = 0; j < argc; ++j )
	{
		if( get_str_arg( zMode, zArgName, argv[j], strlen( argv[j] ) ) )
		{
			if( strcmp( zMode, "bios" ) == 0 )
			{
				bForceBios = true;
				break;
			}
		}
	}

	if( bForceBios == false )
		continue;
	else {
		printk( "BIOS controller enabled for drive %s\n", zName );
		nHandle = register_device( "", "bios" );
		claim_device( nDeviceID, nHandle, "Harddisk", DEVICE_DRIVE );
	}

	strcpy( zArgName, "bdd_" );
	strcat( zArgName, zName );
	strcat( zArgName, "_mode=" );

	for ( j = 0 ; j < argc ; ++j ) {
	    if ( get_str_arg( zMode, zArgName, argv[j], strlen(argv[j]) ) ) {
		if ( strcmp( zMode, "lba" ) == 0 ) {
		    bForceLBA = true;
		    printk( "Force LBA on drive %s\n", zName );
		} else if ( strcmp( zMode, "csh" ) == 0 ) {
		    bForceCSH = true;
		    printk( "Force CSH on drive %s\n", zName );
		} else {
		    printk( "Error: Invalide mode argument '%s' for drive %s\n", zMode, zName );
		}
		break;
	    }
	}
	
	nFlags = get_drive_capabilities( i );
    
	if ( bForceCSH == false && ((nFlags & BCF_EXTENDED_DISK_ACCESS) || bForceLBA) &&
	     get_drive_params_lba( i, &sDriveParams ) >= 0 && sDriveParams.nTotSectors > 0 ) {
	    bCHS = false;
	} else if ( get_drive_params_csh( i, &sDriveParams ) >= 0 && sDriveParams.nTotSectors > 0 ) {
	    bCHS = true;
	} else {
	    continue;
	}
	psInode = kmalloc( sizeof( BddInode_s ), MEMF_KERNEL | MEMF_CLEAR );

	if ( psInode == NULL ) {
	    nError = -ENOMEM;
	    goto error;
	}
	strcpy( psInode->bi_zName, zName );
	psInode->bi_nDeviceHandle  = nHandle;
	psInode->bi_nDriveNum      = i;
	psInode->bi_nSectors       = sDriveParams.nSectors;
	psInode->bi_nCylinders     = sDriveParams.nCylinders;
	psInode->bi_nHeads	   = sDriveParams.nHeads;
	psInode->bi_nSectorSize    = sDriveParams.nBytesPerSector;
	psInode->bi_nStart	   = 0;
	psInode->bi_nSize	   = sDriveParams.nTotSectors * sDriveParams.nBytesPerSector;
	psInode->bi_bRemovable     = (sDriveParams.nFlags & DIF_REMOVABLE) != 0;
	psInode->bi_bLockable      = (sDriveParams.nFlags & DIF_CAN_LOCK) != 0;
	psInode->bi_bHasChangeLine = (sDriveParams.nFlags & DIF_HAS_CHANGE_LINE) != 0;
	psInode->bi_bCSHAddressing = bCHS;
    
	printk( "/dev/disk/bios/%s : (%02x) %Ld sectors of %d bytes (%s) (%d:%d:%d)\n", zName,
		psInode->bi_nDriveNum, sDriveParams.nTotSectors, sDriveParams.nBytesPerSector,
		((bCHS) ? "CSH" : "LBA"),
		psInode->bi_nHeads, psInode->bi_nCylinders, psInode->bi_nSectors );

	strcpy( zNodePath, "disk/bios/" );
	strcat( zNodePath, zName );
	strcat( zNodePath, "/raw" );

	if ( psInode->bi_bRemovable ) {
	    printk( "Drive %s is removable\n", zNodePath );
	}
	nError = create_device_node( nDeviceID, nHandle, zNodePath, &g_sOperations, psInode );
	psInode->bi_nNodeHandle = nError;
	if ( nError >= 0 ) {
	    bdd_decode_partitions( psInode );
	}
    }

    return( 0 );
error:
    return( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

#ifdef __BDD_EXTERNAL
status_t device_init( int nDeviceID )
#else
status_t init_bdd( int nDeviceID )
#endif
{
    g_nDevID = nDeviceID;
    g_pCmdBuffer = alloc_real( 512, 0 );
  
    if ( g_pCmdBuffer == NULL ) {
	printk( "Error: init_bdd() failed to alloc command buffer\n" );
	return( -ENOMEM );
    }
    g_pRawDataBuffer = (uint8*) alloc_real( BDD_BUFFER_SIZE + 65535, 0 );
    if ( g_pRawDataBuffer == NULL ) {
	printk( "Error: init_bdd() failed to alloc io buffer\n" );
	free_real( g_pCmdBuffer );
	return( -ENOMEM );
    }
    g_pDataBuffer = (uint8*) (((uint32)g_pRawDataBuffer + 65535) & ~65535);
    g_hRelBufLock	= create_semaphore( "bdd_buffer_lock", 1, 0 );

    if ( g_hRelBufLock < 0 ) {
	printk( "Error: init_bdd() failed to create g_hRelBufLock semaphore\n" );
	return( g_hRelBufLock );
    }
    g_hMotorTimer = create_timer();
    bdd_scan_for_disks( nDeviceID );
    return( 0 );
}

#ifdef __BDD_EXTERNAL
status_t device_uninit( int nDeviceID )
{
    free_real( g_pCmdBuffer );
    free_real( g_pRawDataBuffer );
    delete_semaphore( g_hRelBufLock );
    return( 0 );
}
#endif // __BDD_EXTERNAL
