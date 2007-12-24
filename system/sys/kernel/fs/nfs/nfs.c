
/*
 *  anfs - AtheOS native network file-system
 *  Copyright (C) 1999  Kurt Skauen
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
#include <atheos/kernel.h>
#include <atheos/socket.h>
#include <atheos/semaphore.h>
#include <atheos/filesystem.h>
#include <net/in.h>
#include <net/net.h>
#include <macros.h>

#include "nfs.h"

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

NfsInode_s *nfs_find_inode( NfsVolume_s * psVolume, ino_t nInode )
{
	return ( ( NfsInode_s * ) ( ( uint32 )nInode ) );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static NfsInode_s *nfs_create_inode( const char *pzName, int nSrvHandle )
{
	NfsInode_s *psInode;

	psInode = kmalloc( sizeof( NfsInode_s ), MEMF_KERNEL | MEMF_CLEAR );

	if( NULL == psInode )
	{
		goto error1;
	}

	psInode->ni_hLock = create_semaphore( "nfs_inode", 1, SEM_RECURSIVE );

	if( psInode->ni_hLock < 0 )
	{
		goto error2;
	}
	strcpy( psInode->ni_zName, pzName );
	psInode->ni_bValid = true;
	psInode->ni_nServerHandle = nSrvHandle;
	return ( psInode );
      error2:
	kfree( psInode );
      error1:
	return ( NULL );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static ino_t nfs_add_inode( NfsVolume_s * psVolume, NfsInode_s * psParent, NfsInode_s * psInode )
{
	psInode->ni_psNext = psParent->ni_psFirstChild;
	psParent->ni_psFirstChild = psInode;
	psInode->ni_psParent = psParent;
	psInode->ni_nInode = ( uint32 )psInode;
	return ( psInode->ni_nInode );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static void nfs_remove_inode( NfsVolume_s * psVolume, NfsInode_s * psInode )
{
	NfsInode_s **ppsTmp;
	bool bFound = false;

	if( psInode->ni_psParent == NULL )
	{
		printk( "Error: nfs_remove_inode() Attempt to remove root inode!\n" );
		return;
	}

	for( ppsTmp = &psInode->ni_psParent->ni_psFirstChild; NULL != *ppsTmp; ppsTmp = &( *ppsTmp )->ni_psNext )
	{
		if( *ppsTmp == psInode )
		{
			*ppsTmp = psInode->ni_psNext;
			bFound = true;
			break;
		}
	}
	if( bFound == false )
	{
		printk( "Error: nfs_remove_inode() failed to find inode %s in parent\n", psInode->ni_zName );
	}
}


/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int nfs_rx_thread( void *pData )
{
	NfsVolume_s *psVolume = pData;

	for( ;; )
	{
		static uint8 anBuffer[2048];
		NfsPacketHdr_s *psHdr = ( NfsPacketHdr_s * ) anBuffer;
		int nSize;
		int nFromLen;

		memset( psHdr, 0, sizeof( NfsPacketHdr_s ) );

//    nSize = sys_socket_read( psVolume->nv_nRxPort, anBuffer, 1518 );

		nSize = recvfrom( psVolume->nv_nSocket, anBuffer, 1500, 0, NULL, &nFromLen );
		

		if( nSize > 0 )
		{
			NfsPendingReq_s *psNode;
			bool bFound = false;

			if( nSize < sizeof( NfsPacketHdr_s ) )
			{
				printk( "nfs_rx_thread() invalid packet size %d\n", nSize );
				continue;
			}
			LOCK( psVolume->nv_hMsgListSema );
			for( psNode = psVolume->nv_psFirstPending; psNode != NULL; psNode = psNode->psNext )
			{
				if( atomic_read( &psNode->nSeqNum ) == atomic_read( &psHdr->nSeqNum ) )
				{
					bFound = true;
					if( psNode->bFinnished )
					{
						printk( "Error: nfs_rx_thread() got reply to %d twice\n", psNode->nSeqNum );
						continue;	// Should we break?
					}

/*	  
	  psNode->pReply = kmalloc( nSize, MEMF_KERNEL );
	  if ( NULL == psNode->pReply ) {
	    printk( "Error: nfs_rx_thread() no memory for reply buffer\n" );
	    break;
	  }
	  */
					psNode->nReplySize = min( psNode->nReplySize, nSize );
        //printk( "Write %d bytes to reply %d\n", psNode->nReplySize, psNode->nSeqNum );
					memcpy( psNode->pReply, anBuffer, psNode->nReplySize );
					psNode->bFinnished = true;
					UNLOCK( psNode->hSema );
					break;
				}
			}
			UNLOCK( psVolume->nv_hMsgListSema );
			if( bFound == false )
			{
//      printk( "nfs_rx_thread() got invalid reply %d\n", psHdr->nSeqNum );
			}
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

static int nfs_rpc( NfsVolume_s * psVolume, void *pMsg, int nMsgSize, void *pReply, int nReplySize )
{
	NfsPacketHdr_s *psMsg = pMsg;
	NfsPendingReq_s sNode;
	NfsPendingReq_s **ppsTmp;
	int i;
	int nError;

	memset( &sNode, 0, sizeof( sNode ) );

	sNode.hSema = create_semaphore( "nfs_msg", 0, 0 );

	if( sNode.hSema < 0 )
	{
		nError = sNode.hSema;
		goto error1;
	}

	atomic_set( &psMsg->nSeqNum, atomic_inc_and_read( &psVolume->nv_nCurSequence ) );

	sNode.pReply = pReply;
	sNode.nReplySize = nReplySize;
	sNode.nSeqNum = psMsg->nSeqNum;

	LOCK( psVolume->nv_hMsgListSema );
	sNode.psNext = psVolume->nv_psFirstPending;
	psVolume->nv_psFirstPending = &sNode;
	UNLOCK( psVolume->nv_hMsgListSema );

	for( i = 0; i < 10; ++i )
	{
		int j;

   // printk( "Send msg %d (%d)\n", psMsg->nCmd, psMsg->nSeqNum );
//    sys_socket_write( psVolume->nv_nTxPort, psMsg, nMsgSize );

		sendto( psVolume->nv_nSocket, psMsg, nMsgSize, 0, ( const struct sockaddr * )&psVolume->nv_sSrvAddr, sizeof( psVolume->nv_sSrvAddr ) );

		for( j = 0; j < 10; ++j )
		{
			lock_semaphore( sNode.hSema, 0, 100000LL );	// Wait for a packet to arrive
			if( sNode.bFinnished )
			{
				nError = sNode.nReplySize;
				goto done;
			}
		}
	}
	nError = -ETIMEDOUT;
      done:

	LOCK( psVolume->nv_hMsgListSema );
	for( ppsTmp = &psVolume->nv_psFirstPending; *ppsTmp != NULL; ppsTmp = &( *ppsTmp )->psNext )
	{
		if( *ppsTmp == &sNode )
		{
			*ppsTmp = sNode.psNext;
			break;
		}
	}
	UNLOCK( psVolume->nv_hMsgListSema );
	delete_semaphore( sNode.hSema );
	return ( nError );
      error1:
	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int nfs_connect( NfsVolume_s * psVolume )
{
	struct sockaddr_in sOurAddr;
	int nPort;
	int nError;

	psVolume->nv_nSocket = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP );
	if( psVolume->nv_nSocket < 0 )
	{
		printk( "Error: nfs_connect() Failed to create socket: %d\n", psVolume->nv_nSocket );
		return ( psVolume->nv_nSocket );
	}

	for( nPort = 1024; nPort < 65536; nPort++ )
	{
		memset( &sOurAddr, 0, sizeof( sOurAddr ) );
		sOurAddr.sin_family = AF_INET;
		sOurAddr.sin_port = htons( nPort );
		nError = bind( psVolume->nv_nSocket, ( struct sockaddr * )&sOurAddr, sizeof( sOurAddr ) );
		if( nError >= 0 )
		{
			break;
		}
	}
	if( nError < 0 )
	{
		printk( "Error: nfs_connect() failed to allocate local port\n" );
		return ( nError );
	}
	printk( "nfs connect to local port %d\n", sOurAddr.sin_port );
	return ( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int nfs_mount( kdev_t nDevNum, const char *pzDevPath, uint32 nFlags, const void *pArgs, int nArgLen, void **ppVolData, ino_t *pnRootIno )
{
	NfsVolume_s *psVolume;
	thread_id hRxThread;
	struct sockaddr_in sSrvAddr;
	int nError;
	char zBuffer[64];

	if( pzDevPath == NULL )
	{
		printk( "Error: nfs_mount() URL ommitted\n" );
		return ( -EINVAL );
	}

	nError = parse_ipaddress( sSrvAddr.sin_addr, pzDevPath );

	if( nError < 0 )
	{
		printk( "Error: nfs_mount() Invalid IP address %s\n", pzDevPath );
		return ( -EINVAL );
	}
	sSrvAddr.sin_port = htons( 100 );

	if( nError < strlen( pzDevPath ) && pzDevPath[nError] == ':' )
	{
		sSrvAddr.sin_port = htons( atol( pzDevPath + nError + 1 ) );
	}
	format_ipaddress( zBuffer, sSrvAddr.sin_addr );
	printk( "nfs_mount() connect to %s:%d\n", zBuffer, sSrvAddr.sin_port );

	psVolume = kmalloc( sizeof( NfsVolume_s ), MEMF_KERNEL | MEMF_CLEAR );

	if( NULL == psVolume )
	{
		nError = -ENOMEM;
		goto error1;
	}
	psVolume->nv_psRoot = kmalloc( sizeof( NfsInode_s ), MEMF_KERNEL | MEMF_CLEAR );

	if( psVolume->nv_psRoot == NULL )
	{
		nError = -ENOMEM;
		goto error2;
	}
	memcpy( &psVolume->nv_sSrvAddr, &sSrvAddr, sizeof( sSrvAddr ) );

	psVolume->nv_psRoot->ni_hLock = create_semaphore( "nfs_root_inode", 1, SEM_RECURSIVE );

	if( psVolume->nv_psRoot->ni_hLock < 0 )
	{
		nError = psVolume->nv_psRoot->ni_hLock;
		goto error3;
	}

	psVolume->nv_psRoot->ni_nServerHandle = -1;
	psVolume->nv_psRoot->ni_nInode = ( uint32 )psVolume->nv_psRoot;
	psVolume->nv_psRoot->ni_zName[0] = '/';
	psVolume->nv_psRoot->ni_zName[1] = '\0';
	psVolume->nv_psRoot->ni_bValid = true;


	psVolume->nv_nFsID = nDevNum;
	psVolume->nv_hMsgListSema = create_semaphore( "nfs_vol", 1, SEM_RECURSIVE );

	if( psVolume->nv_hMsgListSema < 0 )
	{
		nError = psVolume->nv_hMsgListSema;
		goto error4;
	}

	nError = nfs_connect( psVolume );

	if( nError < 0 )
	{
		goto error5;
	}

	printk( "nfs filesystem mounted %p\n", psVolume->nv_psRoot );

	hRxThread = spawn_kernel_thread( "nfs_rx", nfs_rx_thread, 0, 0, psVolume );
	wakeup_thread( hRxThread, false );

	*ppVolData = psVolume;
	*pnRootIno = psVolume->nv_psRoot->ni_nInode;
	return ( 0 );
      error5:
	delete_semaphore( psVolume->nv_hMsgListSema );
      error4:
	delete_semaphore( psVolume->nv_psRoot->ni_hLock );
      error3:
	kfree( psVolume->nv_psRoot );
      error2:
	kfree( psVolume );
      error1:
	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int nfs_unmount( void *pVolData )
{
	return ( -ENOSYS );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int nfs_revalidate_inode( NfsVolume_s * psVolume, NfsInode_s * psInode, NfsOpen_s * psMsg )
{
	NfsOpenReply_s sReply;
	int nError;
	bool bFreeMsg = false;

//  printk( "Attempt to revalidate '%s'\n", psInode->ni_zName );
	if( psInode->ni_psParent == NULL )
	{
		psInode->ni_bValid = true;
		return ( 0 );
	}

	if( psMsg == NULL )
	{
		psMsg = kmalloc( sizeof( NfsOpen_s ), MEMF_KERNEL );
		if( NULL == psMsg )
		{
			return ( -ENOMEM );
		}
		bFreeMsg = true;
	}


//  if ( psInode->ni_psParent->ni_bValid == false ) {
	nfs_revalidate_inode( psVolume, psInode->ni_psParent, psMsg );
//  }

	psMsg->sHdr.nCmd = NFS_OPEN;
	psMsg->nParent = psInode->ni_psParent->ni_nServerHandle;
	strcpy( psMsg->zPath, psInode->ni_zName );

	nError = nfs_rpc( psVolume, psMsg, sizeof( *psMsg ), &sReply, sizeof( sReply ) );

	if( nError < 0 )
	{
		printk( "Failed to revalidate %s\n", psInode->ni_zName );
		goto error;;
	}
	if( sReply.nHandle < 0 )
	{
		printk( "Failed to revalidate %s\n", psInode->ni_zName );
		nError = sReply.nHandle;
		goto error;
	}
//  printk( "Succeded to revalidate %s (%d)\n", psInode->ni_zName, sReply.nHandle );
	psInode->ni_nServerHandle = sReply.nHandle;
	psInode->ni_bValid = true;

	return ( 0 );
      error:
	if( bFreeMsg )
	{
		kfree( psMsg );
	}
	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int nfs_mkdir( void *pVolume, void *pParent, const char *pzName, int nNameLen, int nMode )
{
	NfsVolume_s *psVolume = pVolume;
	NfsInode_s *psParent = pParent;
	NfsInode_s *psInode;
	NfsMkDir_s sMsg;
	NfsMkDirReply_s sReply;
	int nError;

	LOCK( psParent->ni_hLock );
	if( psParent->ni_bValid == false )
	{
		nError = nfs_revalidate_inode( psVolume, psParent, NULL );
		if( nError < 0 )
		{
			printk( "() failed to revalidate parent '%s'\n", psParent->ni_zName );
			goto error;
		}
	}

	for( psInode = psParent->ni_psFirstChild; psInode != NULL; psInode = psInode->ni_psNext )
	{
		if( strncmp( psInode->ni_zName, pzName, nNameLen ) == 0 && strlen( psInode->ni_zName ) == nNameLen )
		{
			nError = -EEXIST;
			goto error;
		}
	}
      again:
	sMsg.sHdr.nCmd = NFS_MKDIR;
	sMsg.nPerms = nMode;
	sMsg.nParent = psParent->ni_nServerHandle;

	memcpy( sMsg.zName, pzName, nNameLen );
	sMsg.zName[nNameLen] = 0;

	nError = nfs_rpc( psVolume, &sMsg, sizeof( sMsg ), &sReply, sizeof( sReply ) );
	if( nError < 0 )
	{
		goto error;
	}
	if( sReply.nError == -ESTALE )
	{
		psParent->ni_bValid = false;
		if( nfs_revalidate_inode( psVolume, psInode, NULL ) >= 0 )
		{
			goto again;
		}
	}
	UNLOCK( psParent->ni_hLock );

	return ( sReply.nError );
      error:
	UNLOCK( psParent->ni_hLock );
	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int nfs_rmdir( void *pVolume, void *pParent, const char *pzName, int nNameLen )
{
	NfsVolume_s *psVolume = pVolume;
	NfsInode_s *psParent = pParent;
	NfsInode_s *psInode;
	NfsRmDir_s sMsg;
	NfsRmDirReply_s sReply;
	int nError;

	LOCK( psParent->ni_hLock );

	if( psParent->ni_bValid == false )
	{
		nError = nfs_revalidate_inode( psVolume, psParent, NULL );
		if( nError < 0 )
		{
			printk( "() failed to revalidate parent '%s'\n", psParent->ni_zName );
			goto error;
		}
	}

	for( psInode = psParent->ni_psFirstChild; psInode != NULL; psInode = psInode->ni_psNext )
	{
		if( strncmp( psInode->ni_zName, pzName, nNameLen ) == 0 && strlen( psInode->ni_zName ) == nNameLen )
		{
			psInode->ni_bValid = false;
			break;
		}
	}
      again:
	sMsg.sHdr.nCmd = NFS_RMDIR;
	sMsg.nParent = psParent->ni_nServerHandle;

	memcpy( sMsg.zName, pzName, nNameLen );
	sMsg.zName[nNameLen] = 0;

	nError = nfs_rpc( psVolume, &sMsg, sizeof( sMsg ), &sReply, sizeof( sReply ) );
	if( nError < 0 )
	{
		goto error;
	}
	if( sReply.nError == -ESTALE )
	{
		psParent->ni_bValid = false;
		if( nfs_revalidate_inode( psVolume, psParent, NULL ) >= 0 )
		{
			goto again;;
		}
	}

	UNLOCK( psParent->ni_hLock );

	return ( sReply.nError );
      error:
	UNLOCK( psParent->ni_hLock );
	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int nfs_rename( void *pVolume, void *pOldDir, const char *pzOldName, int nOldNameLen, void *pNewDir, const char *pzNewName, int nNewNameLen, bool bMustBeDir )
{
	NfsVolume_s *psVolume = pVolume;
	NfsInode_s *psOldDir = pOldDir;
	NfsInode_s *psNewDir = pNewDir;
	NfsRename_s *psMsg;
	NfsRenameReply_s sReply;
	int nError;

	psMsg = kmalloc( sizeof( *psMsg ), MEMF_KERNEL );

	if( psMsg == NULL )
	{
		nError = -ENOMEM;
		goto error1;
	}

	psMsg->sHdr.nCmd = NFS_RENAME;


	memcpy( psMsg->zOldName, pzOldName, nOldNameLen );
	psMsg->zOldName[nOldNameLen] = 0;
	memcpy( psMsg->zNewName, pzNewName, nNewNameLen );
	psMsg->zNewName[nNewNameLen] = 0;

      again:
	if( psOldDir->ni_bValid == false )
	{
		nError = nfs_revalidate_inode( psVolume, psOldDir, NULL );
		if( nError < 0 )
		{
			printk( "() failed to revalidate old-dir '%s'\n", psOldDir->ni_zName );
			goto error2;
		}
	}
	if( psNewDir->ni_bValid == false )
	{
		nError = nfs_revalidate_inode( psVolume, psNewDir, NULL );
		if( nError < 0 )
		{
			printk( "() failed to revalidate new-dir '%s'\n", psNewDir->ni_zName );
			goto error2;
		}
	}

	psMsg->nOldDir = psOldDir->ni_nServerHandle;
	psMsg->nNewDir = psNewDir->ni_nServerHandle;

	nError = nfs_rpc( psVolume, psMsg, sizeof( *psMsg ), &sReply, sizeof( sReply ) );
	if( nError < 0 )
	{
		goto error2;
	}
	nError = sReply.nError;

	if( nError == -ESTALE )
	{
		psOldDir->ni_bValid = false;
		psNewDir->ni_bValid = false;
		goto again;
	}

	kfree( psMsg );
	return ( nError );
      error2:
	kfree( psMsg );
      error1:
	return ( nError );

}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int nfs_unlink( void *pVolume, void *pParent, const char *pzName, int nNameLen )
{
	NfsVolume_s *psVolume = pVolume;
	NfsInode_s *psParent = pParent;
	NfsInode_s *psInode;
	NfsUnlink_s sMsg;
	NfsUnlinkReply_s sReply;
	int nError;

	LOCK( psParent->ni_hLock );

	if( psParent->ni_bValid == false )
	{
		nError = nfs_revalidate_inode( psVolume, psParent, NULL );
		if( nError < 0 )
		{
			printk( "() failed to revalidate parent '%s'\n", psParent->ni_zName );
			goto error;
		}
	}

	for( psInode = psParent->ni_psFirstChild; psInode != NULL; psInode = psInode->ni_psNext )
	{
		if( strncmp( psInode->ni_zName, pzName, nNameLen ) == 0 && strlen( psInode->ni_zName ) == nNameLen )
		{
			psInode->ni_bValid = false;
			break;
		}
	}
      again:
	sMsg.sHdr.nCmd = NFS_UNLINK;
	sMsg.nParent = psParent->ni_nServerHandle;

	memcpy( sMsg.zName, pzName, nNameLen );
	sMsg.zName[nNameLen] = 0;

	nError = nfs_rpc( psVolume, &sMsg, sizeof( sMsg ), &sReply, sizeof( sReply ) );
	if( nError < 0 )
	{
		goto error;
	}
	if( sReply.nError == -ESTALE )
	{
		psParent->ni_bValid = false;
		if( nfs_revalidate_inode( psVolume, psParent, NULL ) >= 0 )
		{
			goto again;
		}
	}
	UNLOCK( psParent->ni_hLock );
	return ( sReply.nError );
      error:
	UNLOCK( psParent->ni_hLock );
	return ( nError );
}


/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int nfs_lookup( void *pVolume, void *pParent, const char *pzName, int nNameLen, ino_t *pnResInode )
{
	NfsVolume_s *psVolume = pVolume;
	NfsInode_s *psParent = pParent;
	NfsInode_s *psInode;
	NfsOpen_s sMsg;
	NfsOpenReply_s sReply;
	int nError;

//  printk( "nfs_lookup\n" );

	LOCK( psParent->ni_hLock );

	if( psParent->ni_bValid == false )
	{
		nError = nfs_revalidate_inode( psVolume, psParent, NULL );
		if( nError < 0 )
		{
			printk( "() failed to revalidate parent '%s'\n", psParent->ni_zName );
			goto error1;
		}
	}


	if( nNameLen == 1 && pzName[0] == '.' )
	{
		*pnResInode = psParent->ni_nInode;
		goto done;
	}
	if( nNameLen == 2 && pzName[0] == '.' && pzName[1] == '.' )
	{
		if( psParent->ni_psParent != NULL )
		{
			*pnResInode = psParent->ni_psParent->ni_nInode;
			goto done;
		}
		else
		{
			nError = -ENOENT;
			goto error1;
		}
	}

	for( psInode = psParent->ni_psFirstChild; psInode != NULL; psInode = psInode->ni_psNext )
	{
		if( strncmp( psInode->ni_zName, pzName, nNameLen ) == 0 && strlen( psInode->ni_zName ) == nNameLen )
		{
			if( psInode->ni_bValid == false )
			{
				nError = nfs_revalidate_inode( psVolume, psInode, NULL );
				if( nError < 0 )
				{
					printk( "() failed to revalidate '%s'\n", psInode->ni_zName );
					nError = -ENOENT;
					goto error1;
				}
			}
			*pnResInode = psInode->ni_nInode;
			goto done;
		}
	}

	sMsg.sHdr.nCmd = NFS_OPEN;
	sMsg.nParent = psParent->ni_nServerHandle;
	memcpy( sMsg.zPath, pzName, nNameLen );
	sMsg.zPath[nNameLen] = 0;

//  printk( "nfs_lookup rpc\n" );

	nError = nfs_rpc( psVolume, &sMsg, sizeof( sMsg ), &sReply, sizeof( sReply ) );
	if( nError < 0 )
	{
		goto error1;
	}
	if( sReply.nHandle < 0 )
	{
		nError = sReply.nHandle;
		goto error1;
	}
	psInode = nfs_create_inode( sMsg.zPath, sReply.nHandle );
//  psInode = kmalloc( sizeof( NfsInode_s ), MEMF_KERNEL | MEMF_CLEAR );
	if( NULL == psInode )
	{
		nError = -ENOMEM;
		goto error2;
	}
//  psInode->ni_bValid = true;
//  strcpy( psInode->ni_zName, sMsg.zPath );
//  psInode->ni_nServerHandle = sReply.nHandle;

//  printk( "Lookup '%s' (%08x)\n", psInode->ni_zName, psInode );
	*pnResInode = nfs_add_inode( psVolume, psParent, psInode );
      done:
	UNLOCK( psParent->ni_hLock );
	return ( 0 );
      error2:

      /*** FIXME: close the handle ***/
      error1:
	UNLOCK( psParent->ni_hLock );
	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int nfs_read_inode( void *pVolume, ino_t nInodeNum, void **ppNode )
{
	NfsVolume_s *psVolume = pVolume;
	NfsInode_s *psInode;

	psInode = nfs_find_inode( psVolume, nInodeNum );
	*ppNode = psInode;

	if( psInode->ni_bValid == false )
	{
		nfs_revalidate_inode( psVolume, psInode, NULL );
	}
	return ( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int nfs_create( void *pVolume, void *pParent, const char *pzName, int nNameLen, int nMode, int nPerms, ino_t *pnResInode, void **ppCookie )
{
	NfsVolume_s *psVolume = pVolume;
	NfsInode_s *psParent = pParent;
	NfsInode_s *psInode;
	NfsCreate_s sMsg;
	NfsCreateReply_s sReply;
	int nError;

//  printk( "nfs_lookup\n" );

	LOCK( psParent->ni_hLock );

	if( psParent->ni_bValid == false )
	{
		nError = nfs_revalidate_inode( psVolume, psParent, NULL );
		if( nError < 0 )
		{
			printk( "() failed to revalidate parent '%s'\n", psParent->ni_zName );
			goto error1;
		}
	}

	for( psInode = psParent->ni_psFirstChild; psInode != NULL; psInode = psInode->ni_psNext )
	{
		if( strncmp( psInode->ni_zName, pzName, nNameLen ) == 0 && strlen( psInode->ni_zName ) == nNameLen )
		{
//      printk( "create foun inode %s (%d)\n", psInode->ni_zName, psInode->ni_bValid );
			if( psInode->ni_bValid == false )
			{
				nError = nfs_revalidate_inode( psVolume, psInode, NULL );
//      printk( "create reval = %d\n", nError );
				if( nError < 0 )
				{
					printk( "() failed to revalidate '%s'\n", psInode->ni_zName );
					break;
				}
			}
			*pnResInode = psInode->ni_nInode;
			goto done;
		}
	}

	sMsg.sHdr.nCmd = NFS_CREATE;
	sMsg.nPerms = nPerms;
	sMsg.nParent = psParent->ni_nServerHandle;

	memcpy( sMsg.zName, pzName, nNameLen );
	sMsg.zName[nNameLen] = 0;

//  printk( "nfs_lookup rpc\n" );

	nError = nfs_rpc( psVolume, &sMsg, sizeof( sMsg ), &sReply, sizeof( sReply ) );
	if( nError < 0 )
	{
		goto error1;
	}
	if( sReply.nHandle < 0 )
	{
		nError = sReply.nHandle;
		goto error1;
	}
	if( psInode == NULL )
	{
		psInode = nfs_create_inode( sMsg.zName, sReply.nHandle );
//    psInode = kmalloc( sizeof( NfsInode_s ), MEMF_KERNEL | MEMF_CLEAR );
		if( NULL == psInode )
		{
			nError = -ENOMEM;
			goto error2;
		}
//    strcpy( psInode->ni_zName, sMsg.zName );
//    psInode->ni_bValid = true;
//    psInode->ni_nServerHandle = sReply.nHandle;
		*pnResInode = nfs_add_inode( psVolume, psParent, psInode );
	}
	else
	{
		psInode->ni_bValid = true;
		psInode->ni_nServerHandle = sReply.nHandle;
		*pnResInode = psInode->ni_nInode;
	}
//  printk( "New server handle = %d (%08x)\n", psInode->ni_nServerHandle, psInode );
      done:
	UNLOCK( psParent->ni_hLock );
	return ( 0 );
      error2:

    /*** FIXME: close the handle ***/
      error1:
	UNLOCK( psParent->ni_hLock );
	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int nfs_open( void *pVolData, void *pNode, int nMode, void **ppCookie )
{
	NfsCookie_s *psCookie;

	psCookie = kmalloc( sizeof( NfsCookie_s ), MEMF_KERNEL | MEMF_CLEAR );

	if( psCookie == NULL )
	{
		return ( -ENOMEM );
	}
	*ppCookie = psCookie;
	return ( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int nfs_close( void *pVolume, void *pNode, void *pCookie )
{
	kfree( pCookie );
	return ( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int nfs_read( void *pVolume, void *pNode, void *pCookie, off_t nPos, void *pBuffer, size_t nLen )
{
	NfsVolume_s *psVolume = pVolume;
	NfsInode_s *psInode = pNode;
	NfsRead_s sMsg;
	NfsReadReply_s *psReply;
	int nActualSize = 0;
	int nError = 0;

	LOCK( psInode->ni_hLock );

	if( psInode->ni_bValid == false )
	{
		nError = nfs_revalidate_inode( psVolume, psInode, NULL );
		if( nError < 0 )
		{
			printk( "() failed to revalidate '%s'\n", psInode->ni_zName );
			goto error1;
		}
	}

	psReply = kmalloc( sizeof( *psReply ), MEMF_KERNEL );

	if( psReply == NULL )
	{
		nError = -ENOMEM;
		goto error1;
	}

	sMsg.sHdr.nCmd = NFS_READ;

	while( nLen > 0 )
	{
		sMsg.nHandle = psInode->ni_nServerHandle;
		sMsg.nPos = nPos;
		sMsg.nSize = min( NFS_MAX_READ_BUF, nLen );
		nError = nfs_rpc( psVolume, &sMsg, sizeof( sMsg ), psReply, sizeof( *psReply ) );
		if( nError < 0 )
		{
			printk( "nfs_read() RPC error %d\n", nError );
			goto error2;
		}
		if( psReply->nError < 0 )
		{
			nError = psReply->nError;

			if( psReply->nError == -ESTALE )
			{
				psInode->ni_bValid = false;
				if( nfs_revalidate_inode( psVolume, psInode, NULL ) >= 0 )
				{
					continue;
				}
			}
			goto error2;
		}
		if( psReply->nError > sMsg.nSize )
		{
			printk( "Error: nfs_read() Requested %d bytes got %d\n", sMsg.nSize, psReply->nError );
			nError = -EIO;
			goto error2;
		}
		memcpy( pBuffer, psReply->anBuffer, psReply->nError );

		nActualSize += psReply->nError;
		nPos += psReply->nError;
		pBuffer = ( ( char * )pBuffer ) + psReply->nError;
		nLen -= psReply->nError;

		if( psReply->nError < sMsg.nSize )
		{
			break;
		}
	}
	UNLOCK( psInode->ni_hLock );
	kfree( psReply );
	return ( nActualSize );
      error2:
	kfree( psReply );
      error1:
	UNLOCK( psInode->ni_hLock );
	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int nfs_write( void *pVolume, void *pNode, void *pCookie, off_t nPos, const void *pBuffer, size_t nLen )
{
	NfsVolume_s *psVolume = pVolume;
	NfsInode_s *psInode = pNode;
	NfsWrite_s *psMsg;
	NfsWriteReply_s sReply;
	int nActualSize = 0;
	int nError = 0;

	LOCK( psInode->ni_hLock );

	if( psInode->ni_bValid == false )
	{
		nError = nfs_revalidate_inode( psVolume, psInode, NULL );
		if( nError < 0 )
		{
			printk( "() failed to revalidate '%s'\n", psInode->ni_zName );
			goto error1;
		}
	}

	psMsg = kmalloc( sizeof( *psMsg ), MEMF_KERNEL );

	if( psMsg == NULL )
	{
		nError = -ENOMEM;
		goto error1;
	}

	psMsg->sHdr.nCmd = NFS_WRITE;

	while( nLen > 0 )
	{
		psMsg->nHandle = ( volatile int )psInode->ni_nServerHandle;
		psMsg->nPos = nPos;
		psMsg->nSize = min( NFS_MAX_READ_BUF, nLen );

		memcpy( psMsg->anBuffer, pBuffer, psMsg->nSize );

		nError = nfs_rpc( psVolume, psMsg, sizeof( *psMsg ), &sReply, sizeof( sReply ) );
		if( nError < 0 )
		{
			printk( "nfs_write() RPC error %d\n", nError );
			goto error2;
		}
		if( sReply.nError < 0 )
		{
			nError = sReply.nError;
			if( nError == -ESTALE )
			{
				psInode->ni_bValid = false;
				if( nfs_revalidate_inode( psVolume, psInode, NULL ) >= 0 )
				{
					continue;
				}
			}
			goto error2;
		}
		if( sReply.nError > psMsg->nSize )
		{
			printk( "Error: nfs_write() Requested %d bytes got %d\n", psMsg->nSize, sReply.nError );
			nError = -EIO;
			goto error2;
		}
		nActualSize += sReply.nError;
		nPos += sReply.nError;
		pBuffer = ( ( char * )pBuffer ) + sReply.nError;
		nLen -= sReply.nError;


		if( sReply.nError < psMsg->nSize )
		{
			break;
		}
	}
	UNLOCK( psInode->ni_hLock );
	kfree( psMsg );
	return ( nActualSize );
      error2:
	kfree( psMsg );
      error1:
	UNLOCK( psInode->ni_hLock );
	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int nfs_write_inode( void *pVolume, void *pNode )
{
	NfsVolume_s *psVolume = pVolume;
	NfsInode_s *psInode = pNode;
	NfsClose_s sMsg;
	NfsCloseReply_s sReply;
	int nError;

	return ( 0 );
	if( psInode->ni_psFirstChild == NULL )
	{
		nfs_remove_inode( psVolume, psInode );
	}

	if( psInode->ni_bValid == false )
	{
		return ( 0 );
	}

	sMsg.sHdr.nCmd = NFS_CLOSE;
	sMsg.nHandle = psInode->ni_nServerHandle;


	nError = nfs_rpc( psVolume, &sMsg, sizeof( sMsg ), &sReply, sizeof( sReply ) );

	if( nError < 0 )
	{
		return ( nError );
	}
	return ( sReply.nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int nfs_rstat( void *pVolume, void *pNode, struct stat *psStat )
{
	NfsVolume_s *psVolume = pVolume;
	NfsInode_s *psInode = pNode;
	NfsStat_s sMsg;
	NfsStatReply_s sReply;
	int nError;

	LOCK( psInode->ni_hLock );
	if( psInode->ni_bValid == false )
	{
		nError = nfs_revalidate_inode( psVolume, psInode, NULL );
		if( nError < 0 )
		{
			printk( "() failed to revalidate '%s'\n", psInode->ni_zName );
			goto error;
		}
	}
      again:
	sMsg.sHdr.nCmd = NFS_STAT;
	sMsg.nHandle = psInode->ni_nServerHandle;

	nError = nfs_rpc( psVolume, &sMsg, sizeof( sMsg ), &sReply, sizeof( sReply ) );
	if( nError < 0 )
	{
		printk( "nfs_stat() RPC error %d\n", nError );
		goto error;
	}
	nError = sReply.nError;

	if( nError < 0 )
	{
		if( nError == -ESTALE )
		{
			psInode->ni_bValid = false;
			if( nfs_revalidate_inode( psVolume, psInode, NULL ) >= 0 )
			{
				goto again;
			}
		}
		goto error;
	}

	*psStat = sReply.sStat;
	UNLOCK( psInode->ni_hLock );

	return ( 0 );
      error:
	UNLOCK( psInode->ni_hLock );
	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int nfs_wstat( void *pVolume, void *pNode, const struct stat *psStat, uint32 nMask )
{
	NfsVolume_s *psVolume = pVolume;
	NfsInode_s *psInode = pNode;
	NfsWStat_s sMsg;
	NfsWStatReply_s sReply;
	int nError;

	LOCK( psInode->ni_hLock );

	if( psInode->ni_bValid == false )
	{
		nError = nfs_revalidate_inode( psVolume, psInode, NULL );
		if( nError < 0 )
		{
			printk( "() failed to revalidate '%s'\n", psInode->ni_zName );
			goto error;
		}
	}

      again:
	sMsg.sHdr.nCmd = NFS_WSTAT;
	sMsg.nHandle = psInode->ni_nServerHandle;
	sMsg.sStat = *psStat;

	nError = nfs_rpc( psVolume, &sMsg, sizeof( sMsg ), &sReply, sizeof( sReply ) );
	if( nError < 0 )
	{
		printk( "nfs_wstat() RPC error %d\n", nError );
		goto error;
	}
	nError = sReply.nError;

	if( nError < 0 )
	{
		if( nError == -ESTALE )
		{
			psInode->ni_bValid = false;
			if( nfs_revalidate_inode( psVolume, psInode, NULL ) >= 0 )
			{
				goto again;
			}
		}
		goto error;
	}
	UNLOCK( psInode->ni_hLock );

	return ( 0 );
      error:
	UNLOCK( psInode->ni_hLock );
	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int nfs_readdir( void *pVolume, void *pNode, void *pCookie, int nPos, struct kernel_dirent *psFileInfo, size_t nBufSize )
{
	NfsVolume_s *psVolume = pVolume;
	NfsInode_s *psInode = pNode;

//  NfsCookie_s*         psCookie = pCookie;
	NfsReadDir_s sMsg;
	NfsReadDirReply_s sReply;

//  NfsDirEntry_s*    psEntry;
	int nError;

	LOCK( psInode->ni_hLock );

	if( psInode->ni_bValid == false )
	{
		nError = nfs_revalidate_inode( psVolume, psInode, NULL );
		if( nError < 0 )
		{
			printk( "() failed to revalidate '%s'\n", psInode->ni_zName );
			goto error;
		}
	}

      again:
	sMsg.sHdr.nCmd = NFS_READ_DIR;
	sMsg.nHandle = psInode->ni_nServerHandle;
	sMsg.nPos = nPos;

//  printk( "nfs_readdir rpc\n" );
	nError = nfs_rpc( psVolume, &sMsg, sizeof( sMsg ), &sReply, sizeof( sReply ) );
	if( nError < 0 )
	{
		printk( "nfs_readdir() RPC error %d\n", nError );
		goto error;
	}
	if( sReply.nError != 1 )
	{
		nError = sReply.nError;
		if( sReply.nError == -ESTALE )
		{
			psInode->ni_bValid = false;
			if( nfs_revalidate_inode( psVolume, psInode, NULL ) >= 0 )
			{
				goto again;
			}
		}
		goto error;
	}
	strcpy( psFileInfo->d_name, sReply.zName );
	psFileInfo->d_namlen = strlen( psFileInfo->d_name );
	
	//printk( "Entry %s\n", psFileInfo->d_name );
	
	if( ( nError = nfs_lookup( pVolume, pNode, psFileInfo->d_name, psFileInfo->d_namlen, &psFileInfo->d_ino ) ) != 0 )
	{
		printk( "nfs_lookup() failed\n" );
		goto error;
	}

	UNLOCK( psInode->ni_hLock );

	return ( sReply.nError );
      error:
	UNLOCK( psInode->ni_hLock );
	return ( nError );


}


static int nfs_readlink( void *pVolume, void *pNode, char *pzBuf, size_t nBufSize )
{
	NfsVolume_s *psVolume = pVolume;
	NfsInode_s *psInode = pNode;
	NfsReadLink_s sMsg;
	NfsReadLinkReply_s sReply;
	int nError;

	LOCK( psInode->ni_hLock );

	if( psInode->ni_bValid == false )
	{
		nError = nfs_revalidate_inode( psVolume, psInode, NULL );
		if( nError < 0 )
		{
			printk( "() failed to revalidate '%s'\n", psInode->ni_zName );
			goto error;
		}
	}

      again:
	sMsg.sHdr.nCmd = NFS_READLINK;
	sMsg.nHandle = psInode->ni_nServerHandle;

	nError = nfs_rpc( psVolume, &sMsg, sizeof( sMsg ), &sReply, sizeof( sReply ) );
	if( nError < 0 )
	{
		printk( "nfs_readlink() RPC error %d\n", nError );
		goto error;
	}
	nError = sReply.nError;

	if( nError < 0 )
	{
		if( nError == -ESTALE )
		{
			psInode->ni_bValid = false;
			if( nfs_revalidate_inode( psVolume, psInode, NULL ) >= 0 )
			{
				goto again;
			}
		}
		goto error;
	}

	strncpy( pzBuf, sReply.zBuffer, nBufSize );
	pzBuf[nBufSize - 1] = '\0';

	UNLOCK( psInode->ni_hLock );

	return ( strlen( pzBuf ) );
      error:
	UNLOCK( psInode->ni_hLock );
	return ( nError );
}

static int nfs_read_fs_stat(void * pVolume, fs_info * fsinfo) {
	NfsVolume_s *psVolume = pVolume;


	fsinfo->fi_dev = psVolume->nv_nFsID;
	fsinfo->fi_root = psVolume->nv_psRoot->ni_nInode;
	fsinfo->fi_flags = FS_IS_PERSISTENT | FS_IS_SHARED | FS_IS_BLOCKBASED | FS_CAN_MOUNT;
	fsinfo->fi_block_size = 512;
	fsinfo->fi_io_size = 65536;
	fsinfo->fi_total_blocks =1;
	fsinfo->fi_free_blocks = 0;
	fsinfo->fi_free_user_blocks = 0;
	fsinfo->fi_total_inodes = 1;
	fsinfo->fi_free_inodes = 0;
	
	char zBuffer[64];
	format_ipaddress( zBuffer, psVolume->nv_sSrvAddr.sin_addr );
	
	
	strcpy( fsinfo->fi_volume_name, "Network - " );
	strcat( fsinfo->fi_volume_name, zBuffer );
	strcpy( fsinfo->fi_driver_name, "nfs" );
	
	return -EOK;
}


static FSOperations_s g_sOperations = {
	NULL,			// op_probe
	nfs_mount,
	nfs_unmount,
	nfs_read_inode,
	nfs_write_inode,
	nfs_lookup,
	NULL,			/* op_access */
	nfs_create,
	nfs_mkdir,
	NULL,			/* mknod        */
	NULL,			/* op_symlink */
	NULL,			/* op_link */
	nfs_rename,
	nfs_unlink,
	nfs_rmdir,
	nfs_readlink,		/* op_readlink */
	NULL,			/* op_ppendir */
	NULL,			/* op_closedir */
	NULL,			/* op_rewinddir */
	nfs_readdir,
	nfs_open,
	nfs_close,
	NULL,			/* op_free_cookie */
	nfs_read,		// op_read
	nfs_write,		// op_write
	NULL,			// op_readv
	NULL,			// op_writev
	NULL,			/* op_ioctl */
	NULL,			/* op_setflags */
	nfs_rstat,		/* op_rstat */
	nfs_wstat,		/* op_wstat */
	NULL,			/* op_fsync */
	NULL,			/* op_initialize */
	NULL,			/* op_sync */
	nfs_read_fs_stat,			/* op_rfsstat */
	NULL,			/* op_wfsstat */
	NULL			/* op_isatty    */
};



int fs_init( const char *pzName, FSOperations_s ** ppsOps )
{
	printk( "initialize_fs called in nfs\n" );
	*ppsOps = &g_sOperations;
	return ( FSDRIVER_API_VERSION );
}













