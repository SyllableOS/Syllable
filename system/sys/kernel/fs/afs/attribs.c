
/*
 *  AFS - The native AtheOS file-system
 *  Copyright(C) 1999 - 2001 Kurt Skauen
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
#include <macros.h>

#include <atheos/types.h>
#include <atheos/filesystem.h>
#include <atheos/kernel.h>
#include <atheos/bcache.h>
#include <atheos/semaphore.h>

#include "afs.h"





/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int afs_put_inode( AfsVolume_s * psVolume, AfsInode_s * psInode, bool bWriteBack )
{
	int nError = 0;

	if( psInode == NULL )
	{
		printk( "Panic: afs_put_inode() called with inode == NULL!\n" );
		return( -EINVAL );
	}

	if( afs_validate_inode( psVolume, psInode ) != 0 )
	{
		panic( "afs_put_inode() attempt to release an invalide inode\n" );
	}
	if( bWriteBack )
	{
		if( psInode->ai_nFlags & INF_WAS_WRITTEN )
		{
			nError = afs_logged_write( psVolume, psInode, afs_run_to_num( psVolume->av_psSuperBlock, &psInode->ai_sInodeNum ) );

			if( nError < 0 )
			{
				printk( "Panic: afs_put_inode() failed to write inode to log\n" );
			}
			else
			{
				psInode->ai_nFlags &= ~INF_WAS_WRITTEN;
			}
		}
	}
	kfree( psInode );
	return( nError );
}

static int afs_load_attr_inode( AfsVolume_s * psVolume, AfsInode_s * psAttrDirInode, const char *pzName, int nNameLen, AfsInode_s ** ppsAttrInode )
{
	int nError;
	ino_t nAttrInode;
	BlockRun_s sAttrInode;

	nError = afs_lookup_inode( psVolume, psAttrDirInode, pzName, nNameLen, &nAttrInode );

	if( nError < 0 )
	{
		return( nError );
	}
	afs_num_to_run( psVolume->av_psSuperBlock, &sAttrInode, nAttrInode );
	nError = afs_do_read_inode( psVolume, &sAttrInode, ppsAttrInode );
	return( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int afs_sd_create_attribute( char *pInode, int nBlockSize, int nType, const char *pzName, int nNameLen, const void *pData, int nPos, int nSize )
{
	AfsSmallData_s *psEntry;
	int nTotSize = 0;

//  printk( "afs_sd_add_create_attribute() %s\n",(char*)pData );

	for( psEntry = ( AfsSmallData_s * ) ( pInode ); ( ( char * )psEntry ) < ( pInode + nBlockSize ) && 0 != psEntry->sd_nNameSize; psEntry = NEXT_SD( psEntry ) )
	{
		nTotSize += SD_SIZE( psEntry );

		if( psEntry->sd_nNameSize == nNameLen && strncmp( ( char * )( psEntry + 1 ), pzName, nNameLen ) == 0 )
		{
			if( nSize + nPos <= psEntry->sd_nDataSize )
			{
//      printk( "Will fit\n" );
				memcpy( SD_DATA( psEntry ) + nPos, pData, nSize );
				psEntry->sd_nType = nType;
				return( 0 );
			}
			else
			{
				AfsSmallData_s *psTmp;
				const int nDeltaSize = nPos + nSize - psEntry->sd_nDataSize;
				int nMoveSize = 0;

				for( psTmp = NEXT_SD( psEntry ); ( ( char * )psTmp ) < ( pInode + nBlockSize ) && 0 != psTmp->sd_nNameSize; psTmp = NEXT_SD( psTmp ) )
				{
					nMoveSize += SD_SIZE( psTmp );
				}

				if( nTotSize + nMoveSize + nDeltaSize > nBlockSize )
				{
//        printk( "Cant insert %d extra bytes. Block is full %d(%d)\n", nDeltaSize, nTotSize + nMoveSize, nBlockSize );
					return( -ENOSPC );
				}
				memmove(( ( char * )NEXT_SD( psEntry ) ) + nDeltaSize, NEXT_SD( psEntry ), nMoveSize );

//      printk( "Move %d byte by %d\n", nMoveSize, nDeltaSize );

				psEntry->sd_nDataSize = nPos + nSize;
				memcpy( SD_DATA( psEntry ) + nPos, pData, nSize );
				psEntry->sd_nType = nType;
				return( 0 );
			}
		}
//    printk( "afs_sd_add_create_attribute() scan %s\n", SD_DATA( psEntry ) );
	}
//  printk( "TotSize = %d\n", nTotSize );

	if( nTotSize + sizeof( AfsSmallData_s ) + nNameLen + nSize <= nBlockSize )
	{
		int i;

		psEntry->sd_nType = nType;
		psEntry->sd_nNameSize = nNameLen;
		psEntry->sd_nDataSize = nSize;

		kassertw(( ( uint32 )SD_DATA( psEntry ) + nSize ) <= ( ( ( uint32 )pInode ) + nBlockSize ) );
		memcpy( psEntry + 1, pzName, nNameLen );
		memcpy( SD_DATA( psEntry ), pData, nSize );


		for( i = ( ( ( uint32 )NEXT_SD( psEntry ) ) - ( ( uint32 )pInode ) ); i < nBlockSize; ++i )
		{
			if( pInode[i] != 0 )
			{
				printk( "PANIC : create : pos %d == %d\n", i, pInode[i] );
				pInode[i] = 0;
			}
		}

	}
	else
	{
//    printk( "afs_sd_add_create_attribute() inode is full %d\n", nTotSize );
		return( -ENOSPC );
	}
	return( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static AfsSmallData_s *afs_sd_find_entry( char *pInode, int nBlockSize, const char *pzName, int nNameLen )
{
	AfsSmallData_s *psEntry;

	for( psEntry = ( AfsSmallData_s * ) pInode; ( ( char * )psEntry ) < ( pInode + nBlockSize ) && 0 != psEntry->sd_nNameSize; psEntry = NEXT_SD( psEntry ) )
	{
//    printk( "afs_sd_find_entry() scan %s\n", SD_DATA( psEntry ) );
		if( psEntry->sd_nNameSize == nNameLen && strncmp( ( char * )( psEntry + 1 ), pzName, nNameLen ) == 0 )
		{
//      printk( "afs_sd_find_entry() found %s\n", SD_DATA( psEntry ) );
			return( psEntry );
		}
	}
	return( NULL );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int afs_sd_delete_entry( char *pInode, int nBlockSize, const char *pzName, int nNameLen )
{
	AfsSmallData_s *psEntry;
	AfsSmallData_s *psTmp;
	int nTotSize = 0;
	int nSize;
	int i;

//  printk( "afs_sd_delete_entry()  %s\n", pzName );
	psEntry = afs_sd_find_entry( pInode, nBlockSize, pzName, nNameLen );

	if( NULL == psEntry )
	{
//    printk( "afs_sd_delete_entry() failed to delete %s\n", pzName );
		return( -ENOENT );
	}

	nSize = SD_SIZE( psEntry );

	for( psTmp = NEXT_SD( psEntry ); ( ( char * )psTmp ) < ( pInode + nBlockSize ) && 0 != psTmp->sd_nNameSize; psTmp = NEXT_SD( psTmp ) )
	{
		nTotSize += SD_SIZE( psTmp );
//    printk( "afs_sd_delete_entry() scan %s(ts=%d s=%d ns=%d)\n",
//          SD_DATA( psTmp ), nTotSize, SD_SIZE( psTmp ), psTmp->sd_nNameSize );
	}

//  printk( "Move %d bytes , clear %d - %d\n", nTotSize,(((char*)psEntry) - pInode) + nTotSize, nSize );
	memmove( psEntry, NEXT_SD( psEntry ), nTotSize );
	memset(( ( char * )psEntry ) + nTotSize, 0, nSize );
	kassertw(( ( char * )psEntry ) + nTotSize + nSize <= pInode + nBlockSize );

	for( i = ( ( ( uint32 )psEntry ) - ( ( uint32 )pInode ) ) + nTotSize; i < nBlockSize; ++i )
	{
		if( pInode[i] != 0 )
		{
			printk( "PANIC : delete : pos %d == %d\n", i, pInode[i] );
			pInode[i] = 0;
		}
	}
	return( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int afs_open_attrdir( void *pVolume, void *pNode, void **pCookie )
{
	AfsAttrIterator_s *psIterator;

	psIterator = kmalloc( sizeof( AfsAttrIterator_s ), MEMF_KERNEL | MEMF_CLEAR | MEMF_OKTOFAILHACK );

	if( NULL == psIterator )
	{
		printk( "PANIC : afs_open_attrdir() no memory for cookie\n" );
		return( -ENOMEM );
	}
	psIterator->aa_sBIterator.bi_nCurNode = -1;

	*pCookie = psIterator;
	return( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int afs_close_attrdir( void *pVolume, void *pNode, void *pCookie )
{
	kfree( pCookie );
	return( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int afs_rewind_attrdir( void *pVolume, void *pNode, void *pCookie )
{
	AfsAttrIterator_s *psIterator = pCookie;

	if( NULL == psIterator )
	{
		panic( "afs_rewind_attrdir() pCookie == NULL\n" );
		return( -EINVAL );
	}

	memset( psIterator, 0, sizeof( AfsAttrIterator_s ) );
	psIterator->aa_sBIterator.bi_nCurNode = -1;
	return( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int afs_read_attrdir( void *pVolume, void *pNode, void *pCookie, struct kernel_dirent *psBuffer, size_t bufsize )
{
	AfsVolume_s *psVolume = pVolume;
	const int nBlockSize = psVolume->av_psSuperBlock->as_nBlockSize;
	AfsVNode_s *psVnode = pNode;
	AfsInode_s *psInode = psVnode->vn_psInode;
	AfsAttrIterator_s *psIterator = pCookie;
	int nError = 0;

	if( NULL == psIterator )
	{
		panic( "afs_read_attrdir() pCookie == NULL\n" );
		return( -EINVAL );
	}

	AFS_LOCK( psVnode );

	if( -1 != psIterator->aa_nPos )
	{
		AfsSmallData_s *psEntry =( AfsSmallData_s * ) ( psInode + 1 );
		int i;

		for( i = 0; i < psIterator->aa_nPos; ++i )
		{
			if( ( ( uint32 )psEntry ) >= ( ( ( uint32 )psInode ) + nBlockSize ) || 0 == psEntry->sd_nNameSize )
			{
				break;
			}
			psEntry = NEXT_SD( psEntry );
		}
		if( ( ( uint32 )psEntry ) < ( ( ( uint32 )psInode ) + nBlockSize ) && 0 != psEntry->sd_nNameSize )
		{
			memcpy( psBuffer->d_name, psEntry + 1, psEntry->sd_nNameSize );
			psBuffer->d_name[psEntry->sd_nNameSize] = '\0';
			psBuffer->d_namlen = psEntry->sd_nNameSize;
			psIterator->aa_nPos++;
			nError = 1;
		}
	}

	if( 0 == nError )
	{
		AfsVNode_s *psAttrVnode;
		AfsInode_s *psAttrInode;
		ino_t nAttrInode;

		if( psInode->ai_sAttribDir.len == 0 )
		{
			goto done;
		}

		nAttrInode = afs_run_to_num( psVolume->av_psSuperBlock, &psInode->ai_sAttribDir );
		nError = get_vnode( psVolume->av_nFsID, nAttrInode,( void ** )&psAttrVnode );

		if( nError < 0 )
		{
			printk( "PANIC : afs_read_attrdir() failed to load attribdir inode\n" );
			goto done;
		}
		psAttrInode = psAttrVnode->vn_psInode;

		if( psIterator->aa_nPos > 0 )
		{
			nError = bt_find_first_key( psVolume, psAttrInode, &psIterator->aa_sBIterator );
			psIterator->aa_nPos = -1;
		}
		else
		{
			nError = bt_get_next_key( psVolume, psAttrInode, &psIterator->aa_sBIterator );
		}
		if( nError >= 0 )
		{
			memcpy( psBuffer->d_name, psIterator->aa_sBIterator.bi_anKey, psIterator->aa_sBIterator.bi_nKeySize );
			psBuffer->d_name[psIterator->aa_sBIterator.bi_nKeySize] = '\0';
			psBuffer->d_namlen = psIterator->aa_sBIterator.bi_nKeySize;
			nError = 1;
		}
		else
		{
			if( nError == -ENOENT )
			{
				nError = 0;	/* End of directory */
			}
		}
		put_vnode( psVolume->av_nFsID, nAttrInode );
	}
      done:
	AFS_UNLOCK( psVnode );

	return( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int afs_do_remove_attr( AfsVolume_s * psVolume, AfsInode_s * psInode, const char *pzName, int nNameLen )
{
	const int nBlockSize = psVolume->av_psSuperBlock->as_nBlockSize;
	AfsInode_s *psAttrDirInode = NULL;
	int nError;

	nError = afs_sd_delete_entry(( char * )( psInode + 1 ), nBlockSize - sizeof( AfsInode_s ), pzName, nNameLen );

	if( -ENOENT == nError )
	{
		AfsInode_s *psAttrInode = NULL;

		if( psInode->ai_sAttribDir.len == 0 )
		{
			nError = -ENOENT;
			goto done;
		}

		nError = afs_do_read_inode( psVolume, &psInode->ai_sAttribDir, &psAttrDirInode );
		if( nError < 0 )
		{
			printk( "PANIC : afs_do_remove_attr() failed to load attribdir inode\n" );
			psAttrDirInode = NULL;
			goto done;
		}
		nError = afs_load_attr_inode( psVolume, psAttrDirInode, pzName, nNameLen, &psAttrInode );

		if( nError < 0 )
		{
			printk( "PANIC : afs_do_remove_attr() failed to load attrib inode\n" );
			goto done;
		}

		// Decrease count and make sure it doesn't go negative.
		if( !atomic_add_negative( &psAttrInode->ai_nLinkCount, -1 ) )
		{
			nError = bt_delete_key( psVolume, psAttrDirInode, pzName, nNameLen );

			if( nError < 0 )
			{
				panic( "afs_do_remove_attr() failed to remove attribute from attribute directory %d\n", nError );
			}
			else
			{
				psAttrInode->ai_sData.ds_nSize = 0;
				nError = afs_truncate_stream( psVolume, psAttrInode );

				if( nError < 0 )
				{
					printk( "PANIC : afs_do_remove_attr() Something whent wrong during file truncation! Err = %d\n", nError );
				}
				psAttrInode->ai_nFlags &= ~INF_WAS_WRITTEN;	// Make sure afs_put_inode don't write to the released block

				afs_free_blocks( psVolume, &psAttrInode->ai_sInodeNum );
			}
		}
		else
		{
			atomic_inc( &psAttrInode->ai_nLinkCount );
			printk( "afs_do_remove_attr() Seems like someone else got the same idea!\n" );
			nError = -ENOENT;
		}
		afs_put_inode( psVolume, psAttrInode, true );
	}
      done:
	if( psAttrDirInode != NULL )
	{
		afs_put_inode( psVolume, psAttrDirInode, true );
	}

	return( nError );
}

int afs_remove_attr( void *pVolume, void *pNode, const char *pzName, int nNameLen )
{
	AfsVolume_s *psVolume = pVolume;
	AfsVNode_s *psVnode = pNode;
	AfsInode_s *psInode = psVnode->vn_psInode;
	int nError;

	if( psVolume->av_nFlags & FS_IS_READONLY )
		return( -EROFS );

	nError = afs_begin_transaction( psVolume );

	if( nError < 0 )
	{
		printk( "PANIC : afs_remove_attr() failed to start transaction. Err = %d\n", nError );
		return( nError );
	}

	AFS_LOCK( psVnode );
	nError = afs_do_remove_attr( psVolume, psInode, pzName, nNameLen );

	afs_end_transaction( psVolume, true );

	if( nError > 0 )
	{
		notify_node_monitors( NWEVENT_ATTR_DELETED, psVolume->av_nFsID, 0, 0, afs_run_to_num( psVolume->av_psSuperBlock, &psInode->ai_sInodeNum ), pzName, nNameLen );
	}
	AFS_UNLOCK( psVnode );

	return( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int afs_delete_file_attribs( AfsVolume_s * psVolume, AfsInode_s * psInode )
{
	AfsVNode_s *psVnode = psInode->ai_psVNode;
	AfsInode_s *psAttrDirInode;
	BIterator_s sIterator;
	int i;
	int nError;

	if( psInode->ai_sAttribDir.len == 0 )
	{
		return( 0 );
	}

	AFS_LOCK( psVnode );

	nError = afs_do_read_inode( psVolume, &psInode->ai_sAttribDir, &psAttrDirInode );

	if( nError < 0 )
	{
		printk( "Error: afs_delete_file_attribs() failed to load attribdir inode\n" );
		goto error1;
	}

	i = 0;
	while( bt_find_first_key( psVolume, psAttrDirInode, &sIterator ) >= 0 )
	{
		char zBuffer[512];

		strncpy( zBuffer, sIterator.bi_anKey, sIterator.bi_nKeySize );
		zBuffer[sIterator.bi_nKeySize] = '\0';

//    printk( "afs_delete_file_attribs() delete %s\n", zBuffer );
		nError = afs_do_remove_attr( psVolume, psInode, sIterator.bi_anKey, sIterator.bi_nKeySize );

		if( nError < 0 )
		{
			printk( "Error: afs_delete_file_attribs() failed to delete attribute %s\n", zBuffer );
			goto error2;
		}

		if( ( i++ % 5 ) == 0 )
		{
			nError = afs_checkpoint_journal( psVolume );
			if( nError < 0 )
			{
				printk( "Error: afs_delete_file_attribs() failed to checkpoint journal\n" );
				goto error2;
			}
		}

	}

	psAttrDirInode->ai_sData.ds_nSize = 0;
	nError = afs_truncate_stream( psVolume, psAttrDirInode );

	if( nError < 0 )
	{
		printk( "PANIC : afs_delete_file_attribs() Something whent wrong during file truncation! Err = %d\n", nError );
	}
	afs_free_blocks( psVolume, &psAttrDirInode->ai_sInodeNum );
	psAttrDirInode->ai_nFlags &= ~INF_WAS_WRITTEN;	// Make sure afs_put_inode don't write to the released block
      error2:
	afs_put_inode( psVolume, psAttrDirInode, false );
      error1:
	AFS_UNLOCK( psVnode );

	return( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int afs_stat_attr( void *pVolume, void *pNode, const char *pzName, int nNameLen, struct attr_info *psBuffer )
{
	AfsVolume_s *psVolume = pVolume;
	const int nBlockSize = psVolume->av_psSuperBlock->as_nBlockSize;
	AfsVNode_s *psVnode = pNode;
	AfsInode_s *psInode = psVnode->vn_psInode;
	AfsInode_s *psAttrDirInode;
	AfsInode_s *psAttrInode;
	AfsSmallData_s *psEntry;
	int nError;

	AFS_LOCK( psVnode );

	psEntry = afs_sd_find_entry(( char * )( psInode + 1 ), nBlockSize - sizeof( AfsInode_s ), pzName, nNameLen );

	if( NULL != psEntry )
	{
		psBuffer->ai_size = psEntry->sd_nDataSize;
		psBuffer->ai_type = psEntry->sd_nType;
		nError = 0;
		goto done;
	}


	if( psInode->ai_sAttribDir.len == 0 )
	{
		nError = -ENOENT;
		goto done;
	}

	nError = afs_do_read_inode( psVolume, &psInode->ai_sAttribDir, &psAttrDirInode );

	if( nError < 0 )
	{
		printk( "PANIC : afs_stat_attr() failed to load attribdir inode\n" );
		goto done;
	}
	nError = afs_load_attr_inode( psVolume, psAttrDirInode, pzName, nNameLen, &psAttrInode );
	afs_put_inode( psVolume, psAttrDirInode, false );

	if( nError < 0 )
	{
		goto done;
	}

	psBuffer->ai_size = psAttrInode->ai_sData.ds_nSize;
	psBuffer->ai_type = psAttrInode->ai_nIndexType;

	afs_put_inode( psVolume, psAttrInode, false );

	nError = 0;
      done:
	AFS_UNLOCK( psVnode );
	return( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int afs_write_big_attr( AfsVolume_s * psVolume, AfsInode_s * psAttrInode, int nFlags, const void *pBuffer, off_t nPos, size_t nLen )
{
	const int nBlockSize = psVolume->av_psSuperBlock->as_nBlockSize;
	off_t nOldBlockCount;
	int nError = 0;

//  printk( "write big attrib\n" );

	if( nFlags & O_TRUNC && psAttrInode->ai_sData.ds_nSize > 0 )
	{
		psAttrInode->ai_sData.ds_nSize = 0;
		afs_truncate_stream( psVolume, psAttrInode );
	}

	nOldBlockCount = afs_get_inode_block_count( psAttrInode );

	if( ( nOldBlockCount * nBlockSize ) < ( nPos + nLen ) )
	{
		off_t nNewBlockCount =( nPos + nLen + nBlockSize - 1 ) / nBlockSize;

		kassertw( nNewBlockCount > nOldBlockCount );

		afs_expand_stream( psVolume, psAttrInode, nNewBlockCount - nOldBlockCount );

		nNewBlockCount = afs_get_inode_block_count( psAttrInode );

		if( ( nNewBlockCount * nBlockSize ) < ( nPos + nLen ) )
		{
			printk( "Error: afs_write_big_attr() failed to expand file to fit %d bytes. Scrinked to %Ld\n", nLen, nNewBlockCount * nBlockSize - nPos );

			nLen = nNewBlockCount * nBlockSize - nPos;

			if( nLen == 0 )
			{
				nError = -ENOSPC;
			}
		}
		kassertw( nLen >= 0 );
	}

	if( nError >= 0 )
	{
		nError = afs_do_write( psVolume, psAttrInode, pBuffer, nPos, nLen );
		if( nError >= 0 )
		{
			nError = nLen;
		}
		else
		{
			printk( "PANIC : afs_write_big_attr() failed to write buffer to file %d\n", nError );
		}
	}
	return( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int afs_write_small_attr( AfsVolume_s * psVolume, AfsInode_s * psInode, const char *pzName, int nNameLen, int nType, const char *a_pBuffer, int nPos, size_t nLen )
{
	const int nSDSize = psVolume->av_psSuperBlock->as_nBlockSize - sizeof( AfsInode_s );
	char *pBuffer =( char * )a_pBuffer;

	return( afs_sd_create_attribute( ( char * )( psInode + 1 ), nSDSize, nType, pzName, nNameLen, pBuffer, nPos, nLen ) );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int afs_write_attr( void *pVolume, void *pNode, const char *pzName, int nNameLen, int nFlags, int nType, const void *pBuffer, off_t nPos, size_t nLen )
{
	AfsVolume_s *psVolume = pVolume;
	const int nSDSize = psVolume->av_psSuperBlock->as_nBlockSize - sizeof( AfsInode_s );
	AfsVNode_s *psVnode = pNode;
	AfsInode_s *psInode = psVnode->vn_psInode;
	AfsInode_s *psAttrDirInode = NULL;
	bool bInodeDirty = false;
	int nError;
	int nSmallDataSize = nPos + nLen + nNameLen + sizeof( AfsSmallData_s ) + sizeof( AfsInode_s );

	if( nType >= ATTR_TYPE_COUNT )
	{
		return( -EINVAL );
	}
	if( psVolume->av_nFlags & FS_IS_READONLY )
		return( -EROFS );

	if( nPos > nSDSize )
	{
		nSmallDataSize = nSDSize + 1;
	}

//  printk( "afs_write_attr(%s)\n", pzName );

	nError = afs_begin_transaction( psVolume );

	if( nError < 0 )
	{
		printk( "PANIC : afs_write_attr() failed to start transaction. Err = %d\n", nError );
		return( nError );
	}

	AFS_LOCK( psVnode );

	if( psInode->ai_sAttribDir.len != 0 )
	{
		AfsInode_s *psAttrInode;

		nError = afs_do_read_inode( psVolume, &psInode->ai_sAttribDir, &psAttrDirInode );

		if( nError < 0 )
		{
			printk( "PANIC: afs_write_attr() failed to load attribute directory.\n" );
			psAttrDirInode = NULL;	// Just in case...
			goto done;
		}
		nError = afs_load_attr_inode( psVolume, psAttrDirInode, pzName, nNameLen, &psAttrInode );

		if( nError < 0 && nError != -ENOENT )
		{
			printk( "Error : afs_write_attr() failed to lookup attrib inode. Err = %d\n", nError );
			goto done;
		}
		if( nError >= 0 )
		{
			nError = afs_write_big_attr( psVolume, psAttrInode, nFlags, pBuffer, nPos, nLen );
			afs_put_inode( psVolume, psAttrInode, true );
			goto done;
		}
	}

	// If we got this far the node does not exist in the attrib dir
	// We first attempt to store it in the small data area...

	if( nFlags & O_TRUNC )
	{
		nError = afs_sd_delete_entry(( char * )( psInode + 1 ), nSDSize, pzName, nNameLen );
		if( nError >= 0 )
		{
			bInodeDirty = true;
		}
	}

	nError = afs_write_small_attr( psVolume, psInode, pzName, nNameLen, nType, pBuffer, nPos, nLen );

	// ... if that failed we writes it to a file.
	if( nError < 0 )
	{
		AfsInode_s *psAttrInode = NULL;

		if( psInode->ai_sAttribDir.len == 0 )
		{
			kassertw( NULL == psAttrDirInode );
			nError = afs_create_inode( psVolume, NULL, S_IFDIR | 0777, INF_LOGGED | INF_ATTRIBUTES, e_KeyTypeString, "", 0, &psAttrDirInode );
			if( nError < 0 )
			{
				printk( "Error : afs_write_attr() failed to create attribute directory. Err = %d\n", nError );
				psAttrDirInode = NULL;
				goto done;
			}
			psInode->ai_sAttribDir = psAttrDirInode->ai_sInodeNum;
			bInodeDirty = true;
		}
		nError = afs_create_inode( psVolume, psAttrDirInode, S_IFREG | 0777, INF_LOGGED, nType, pzName, nNameLen, &psAttrInode );
		if( nError < 0 )
		{
			printk( "Error : afs_write_attr() failed to create attribute i-node\n" );
			goto done;
		}

		if( ( nFlags & O_TRUNC ) == 0 )
		{
			AfsSmallData_s *psEntry = afs_sd_find_entry(( char * )( psInode + 1 ), nSDSize, pzName, nNameLen );

			if( NULL != psEntry )
			{
//      printk( "Move attrib from small to big area\n" );
				nError = afs_write_big_attr( psVolume, psAttrInode, nFlags, SD_DATA( psEntry ), 0, psEntry->sd_nDataSize );
				if( nError < 0 )
				{
					afs_put_inode( psVolume, psAttrInode, true );
					goto done;
				}
			}
			afs_sd_delete_entry(( char * )( psInode + 1 ), nSDSize, pzName, nNameLen );
			bInodeDirty = true;
		}
		nError = afs_write_big_attr( psVolume, psAttrInode, nFlags, pBuffer, nPos, nLen );
		afs_put_inode( psVolume, psAttrInode, true );
	}
	else
	{
		bInodeDirty = true;
	}
      done:
	if( NULL != psAttrDirInode )
	{
		afs_put_inode( psVolume, psAttrDirInode, true );
	}
	if( bInodeDirty )
	{
		afs_do_write_inode( psVolume, psInode );
	}
	afs_end_transaction( psVolume, true );

	if( nError > 0 )
	{
		notify_node_monitors( NWEVENT_ATTR_WRITTEN, psVolume->av_nFsID, 0, 0, afs_run_to_num( psVolume->av_psSuperBlock, &psInode->ai_sInodeNum ), pzName, nNameLen );
	}
	AFS_UNLOCK( psVnode );

	return( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int afs_read_attr( void *pVolume, void *pNode, const char *pzName, int nNameLen, int nType, void *pBuffer, off_t nPos, size_t nLen )
{
	AfsVolume_s *psVolume = pVolume;
	const int nSDSize = psVolume->av_psSuperBlock->as_nBlockSize - sizeof( AfsInode_s );
	AfsVNode_s *psVnode = pNode;
	AfsInode_s *psInode = psVnode->vn_psInode;
	AfsSmallData_s *psEntry;
	AfsInode_s *psAttrDirInode = NULL;
	AfsInode_s *psAttrInode = NULL;
	int nError;

	AFS_LOCK( psVnode );

	psEntry = afs_sd_find_entry(( char * )( psInode + 1 ), nSDSize, pzName, nNameLen );

	if( NULL != psEntry )
	{
		if( nType != psEntry->sd_nType )
		{
			nError = -EINVAL;
			goto done;
		}
		if( nPos + nLen > psEntry->sd_nDataSize )
		{
			nLen =( psEntry->sd_nDataSize > nPos ) ? ( psEntry->sd_nDataSize - nPos ) : 0;
		}
		if( nLen > 0 )
		{
			memcpy( pBuffer, SD_DATA( psEntry ) + nPos, nLen );
		}
		nError = nLen;
		goto done;
	}

	if( psInode->ai_sAttribDir.len == 0 )
	{
		nError = -ENOENT;
		goto done;
	}
	nError = afs_do_read_inode( psVolume, &psInode->ai_sAttribDir, &psAttrDirInode );

	if( nError < 0 )
	{
		printk( "PANIC : afs_read_attr() failed to load attribdir i-node\n" );
		psAttrDirInode = NULL;	// Just in case...
		goto done;
	}

	nError = afs_load_attr_inode( psVolume, psAttrDirInode, pzName, nNameLen, &psAttrInode );

	if( nError < 0 )
	{
		printk( "Error : afs_read_attr() failed to lookup attrib i-node. Err = %d\n", nError );
		psAttrInode = NULL;	// Just in case...
		goto done;
	}
	if( psAttrInode->ai_nIndexType == nType )
	{
		nError = afs_read_pos( pVolume, psAttrInode, pBuffer, nPos, nLen );
	}
	else
	{
		nError = -EINVAL;
	}
      done:
	if( NULL != psAttrInode )
	{
		afs_put_inode( psVolume, psAttrInode, false );
	}
	if( NULL != psAttrDirInode )
	{
		afs_put_inode( psVolume, psAttrDirInode, false );
	}
	AFS_UNLOCK( psVnode );
	return( nError );
}
