/*
 *  AFS - The native AtheOS file-system
 *  Copyright (C) 1999 - 2000 Kurt Skauen
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

#include <posix/fcntl.h>
#include <posix/errno.h>
#include <posix/stat.h>
#include <posix/ioctl.h>
#include <macros.h>

#include <atheos/string.h>
#include <atheos/types.h>
#include <atheos/time.h>
#include <atheos/filesystem.h>
#include <atheos/kernel.h>
#include <atheos/bcache.h>
#include <atheos/device.h>
#include <atheos/semaphore.h>

#include "afs.h"
#include "btree.h"


int afs_open_indexdir( void* pVolume, void** ppCookie )
{
    AfsFileCookie_s* psCookie;
  
    psCookie = kmalloc( sizeof( AfsFileCookie_s ), MEMF_KERNEL | MEMF_CLEAR | MEMF_OKTOFAILHACK );
  
    if ( NULL == psCookie ) {
	return( -ENOMEM );
    }
    psCookie->fc_nOMode = O_RDONLY;
    psCookie->fc_sBIterator.bi_nCurNode = -1;
  
    *ppCookie = psCookie;
    
    return( 0 );
}

int afs_close_indexdir( void* pVolume, void* pCookie )
{
    kfree( pCookie );
    return( 0 );
}

int afs_rewind_indexdir( void* pVolume, void* pCookie )
{
    return( 0 );
}

int afs_read_indexdir( void* pVolume, void* pCookie, struct kernel_dirent* psBuffer, size_t nBufferSize )
{
    AfsVolume_s*     psVolume     = pVolume;
    AfsSuperBlock_s* psSuperBlock = psVolume->av_psSuperBlock;
    AfsFileCookie_s* psCookie     = pCookie;
    AfsInode_s*      psIndexDir;
    int		     nError;

    LOCK( psVolume->av_hIndexDirMutex );
    nError = afs_do_read_inode( psVolume, &psSuperBlock->as_sIndexDir, &psIndexDir );

    if ( nError < 0 ) {
	printk( "Error: afs_create_index() failed to read index directory inode %d\n", nError );
	goto error;
    }
    nError = afs_do_read_dir( psVolume, psIndexDir, &psCookie->fc_sBIterator, -1, psBuffer, nBufferSize );
    
    kfree( psIndexDir );
error:
    UNLOCK( psVolume->av_hIndexDirMutex );
    return( nError );
}

int afs_create_index( void* pVolume, const char* pzName, int nNameLen, int nType, int nFlags )
{
    AfsVolume_s*     psVolume     = pVolume;
    AfsSuperBlock_s* psSuperBlock = psVolume->av_psSuperBlock;
    AfsInode_s*      psIndexDir;
    int		     nError;

	if (psVolume->av_nFlags & FS_IS_READONLY)
		return( -EROFS );

    LOCK( psVolume->av_hIndexDirMutex );
    
    nError = afs_begin_transaction( psVolume );
    if ( nError < 0 ) {
	printk( "PANIC : afs_create_index() failed to start transaction %d\n", nError );
	goto error1;
    }
    
    nError = afs_do_read_inode( psVolume, &psSuperBlock->as_sIndexDir, &psIndexDir );

    if ( nError < 0 ) {
	printk( "Error: afs_create_index() failed to read index directory inode %d\n", nError );
	goto error2;
    }
    nError = afs_create_inode( psVolume, psIndexDir, 0777 | S_IFDIR, 0, 0, pzName, nNameLen, NULL );
    kfree( psIndexDir );
error2:
    afs_end_transaction( psVolume, true );
error1:
    UNLOCK( psVolume->av_hIndexDirMutex );
    
    return( nError );
}

int afs_remove_index( void* pVolume, const char* pzName, int nNameLen )
{
    return( 0 );
}

int afs_stat_index( void* pVolume, const char* pzName, int nNameLen, struct index_info* psBuffer )
{
    return( 0 );
}

