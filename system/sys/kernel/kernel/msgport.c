
/*
 *  The AtheOS kernel
 *  Copyright (C) 1999 - 2001 Kurt Skauen
 *  Copyright (C) 2002 Kristian Van Der Vliet
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

#include <atheos/types.h>
#include <atheos/msgport.h>
#include <atheos/kernel.h>
#include <atheos/semaphore.h>

#include <macros.h>

#include "inc/scheduler.h"
#include "inc/sysbase.h"
#include "inc/msgport.h"
#include "inc/array.h"

static MultiArray_s g_sMsgPorts;
static sem_id g_hPortListSema;

static PublicPort_s *g_psPubPortListHead;
static sem_id g_hPubPortListSema;

#define lock_mutex( a, b ) LOCK(a)
#define unlock_mutex(a) UNLOCK(a)

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

MsgPort_s *get_port_from_handle( port_id hPort )
{
	return ( MArray_GetObj( &g_sMsgPorts, hPort ) );
}

static void link_port( Process_s *psProc, MsgPort_s *psPort )
{
	psPort->mp_hOwner = psProc->tc_hProcID;

	if ( psProc->pr_psLastPort != NULL )
	{
		psProc->pr_psLastPort->mp_psNext = psPort;
	}
	if ( psProc->pr_psFirstPort == NULL )
	{
		psProc->pr_psFirstPort = psPort;
	}
	psPort->mp_psNext = NULL;
	psPort->mp_psPrev = psProc->pr_psLastPort;
	psProc->pr_psLastPort = psPort;
	psProc->pr_nPortCount++;
}

static void unlink_port( Process_s *psProc, MsgPort_s *psPort )
{
	kassertw( psProc->tc_hProcID == psPort->mp_hOwner );
	psPort->mp_hOwner = -1;

	if ( NULL != psPort->mp_psNext )
	{
		psPort->mp_psNext->mp_psPrev = psPort->mp_psPrev;
	}
	if ( NULL != psPort->mp_psPrev )
	{
		psPort->mp_psPrev->mp_psNext = psPort->mp_psNext;
	}

	if ( psProc->pr_psFirstPort == psPort )
	{
		psProc->pr_psFirstPort = psPort->mp_psNext;
	}
	if ( psProc->pr_psLastPort == psPort )
	{
		psProc->pr_psLastPort = psPort->mp_psPrev;
	}

	psPort->mp_psNext = NULL;
	psPort->mp_psPrev = NULL;
	psProc->pr_nPortCount--;

	if ( psProc->pr_nPortCount < 0 )
	{
		panic( "unlink_port() proc %s got port-count of %d\n", psProc->tc_zName, psProc->pr_nPortCount );
	}
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

port_id sys_create_port( const char *const pzName, int nMaxCount )
{
	int nError;
	MsgPort_s *psPort;

	lock_mutex( g_hPortListSema, true );

	psPort = kmalloc( sizeof( MsgPort_s ), MEMF_CLEAR | MEMF_KERNEL | MEMF_LOCKED | MEMF_OKTOFAIL );

	if ( psPort == NULL )
	{
		nError = -ENOMEM;
		goto error1;
	}
	psPort->mp_hSyncSema = create_semaphore( "port_sync", 0, 0 );

	if ( psPort->mp_hSyncSema < 0 )
	{
		printk( "Error: sys_create_port() failed to allocate semaphore\n" );
		nError = psPort->mp_hSyncSema;
		goto error2;
	}
	psPort->mp_hPortID = MArray_Insert( &g_sMsgPorts, psPort, false );

	if ( psPort->mp_hPortID < 0 )
	{
		printk( "Failed to alloc descriptor for msg port %s\n", pzName );
		nError = psPort->mp_hPortID;
		goto error3;
	}
	psPort->mp_nMaxCount = nMaxCount;
	psPort->mp_nFlags = 0;
	strncpy_from_user( psPort->mp_zName, pzName, OS_NAME_LENGTH );
	psPort->mp_zName[OS_NAME_LENGTH - 1] = '\0';

	link_port( CURRENT_PROC, psPort );

	unlock_mutex( g_hPortListSema );

	atomic_inc( &g_sSysBase.ex_nMessagePortCount );

	return ( psPort->mp_hPortID );

      error3:
	delete_semaphore( psPort->mp_hSyncSema );
      error2:
	kfree( psPort );
      error1:
	unlock_mutex( g_hPortListSema );

	return ( nError );
}


static void do_delete_port( Process_s *psProc, MsgPort_s *psPort )
{
	if ( psPort->mp_nFlags & MSG_PORT_PUBLIC )
	{
		unlock_mutex( g_hPortListSema );
		make_port_private( psPort->mp_hPortID );
		lock_mutex( g_hPortListSema, true );
	}

	MArray_Remove( &g_sMsgPorts, psPort->mp_hPortID );

	delete_semaphore( psPort->mp_hSyncSema );

	while ( psPort->mp_psFirstMsg != NULL )
	{
		MessageNode_s *psMsg = psPort->mp_psFirstMsg;

		psPort->mp_psFirstMsg = psMsg->mn_psNext;
		kfree( psMsg );
	}

	if ( psProc != NULL )
	{
		unlink_port( psProc, psPort );
	}

	kfree( psPort );
	atomic_dec( &g_sSysBase.ex_nMessagePortCount );

}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

status_t sys_delete_port( port_id hPort )
{
	Process_s *psProc = CURRENT_PROC;
	MsgPort_s *psPort;
	int nError;

	lock_mutex( g_hPortListSema, true );

	psPort = get_port_from_handle( hPort );
	if ( psPort == NULL )
	{
		printk( "Warning : Attempt to delete invalid message port %d\n", hPort );
		nError = -EINVAL;
		goto error;
	}

	if ( psPort->mp_hOwner != psProc->tc_hProcID )
	{
		printk( "Warning: Attempt to delete message port not owned by us(%d) but by %d\n", psProc->tc_hProcID, psPort->mp_hOwner );
		nError = -EACCES;
		goto error;
	}
	do_delete_port( psProc, psPort );
	unlock_mutex( g_hPortListSema );
	return ( 0 );
      error:
	unlock_mutex( g_hPortListSema );
	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

status_t send_msg_x( port_id hPort, uint32 nCode, const void *pBuffer, int nSize, bigtime_t nTimeOut )
{
	MsgPort_s *psPort;
	MessageNode_s *psNode;
	int nError;

	psNode = kmalloc( sizeof( MessageNode_s ) + nSize, MEMF_KERNEL | MEMF_LOCKED | MEMF_OKTOFAIL );

	if ( psNode == NULL )
	{
		printk( "Error: send_msg_x() failed to alloc message buffer\n" );
		return ( -ENOMEM );
	}

	psNode->mn_psNext = NULL;
	psNode->mn_nCode = nCode;

	if ( NULL != pBuffer )
	{
		memcpy( psNode + 1, pBuffer, nSize );
		psNode->mn_nSize = nSize;
	}
	else
	{
		psNode->mn_nSize = 0;
	}

	lock_mutex( g_hPortListSema, true );

	psPort = get_port_from_handle( hPort );

	if ( psPort == NULL )
	{
		printk( "Error: send_msg_x() attempt to send to invalid port %d\n", hPort );
		nError = -EINVAL;
		goto error;
	}

	if ( NULL == psPort->mp_psFirstMsg )
	{
		psPort->mp_psFirstMsg = psNode;
		psPort->mp_psLastMsg = psNode;
	}
	else
	{
		kassertw( NULL != psPort->mp_psLastMsg );

		psPort->mp_psLastMsg->mn_psNext = psNode;
		psPort->mp_psLastMsg = psNode;
	}
	psPort->mp_nCount++;
	UNLOCK( psPort->mp_hSyncSema );
	unlock_mutex( g_hPortListSema );

	return ( 0 );
      error:
	kfree( psNode );
	unlock_mutex( g_hPortListSema );
	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static status_t do_get_msg_x( bool bFromKernel, port_id hPort, uint32 *pnCode, void *pBuffer, int nSize, bigtime_t nTimeOut )
{
	MsgPort_s *psPort = NULL;
	MessageNode_s *psNode;
	int nRealSize;
	int nError;

	lock_mutex( g_hPortListSema, true );

	psPort = get_port_from_handle( hPort );

	if ( NULL == psPort )
	{
		printk( "get_msg_x() attempt to receive message from invalid port %d\n", hPort );
		nError = -EINVAL;
		goto error1;
	}
      again:
	if ( psPort->mp_psFirstMsg == NULL )
	{
		sem_id hSyncSema;

		// We take a backup here since the port might get deleted
		// when we release the g_hPortListSema semaphore.
		// If the port is deleted so is the semaphore and lock_semaphore()
		// will return an error. It's then very important not to
		// touch the port again until we has revalidated the port-handle.

		hSyncSema = psPort->mp_hSyncSema;

		unlock_mutex( g_hPortListSema );

		nError = lock_semaphore( hSyncSema, 0, nTimeOut );

		lock_mutex( g_hPortListSema, true );

		if ( nError < 0 )
		{
			goto error1;
		}

		// We must revalidate the handle after unlocking g_hPortListSema
		psPort = get_port_from_handle( hPort );

		if ( NULL == psPort )
		{
			nError = -EINVAL;
			printk( "Message port %d deleted while we were wating for a message\n", hPort );
			goto error1;
		}
	}
	else
	{
		if ( lock_semaphore( psPort->mp_hSyncSema, SEM_NOSIG, 0LL ) < 0 )
		{
			printk( "get_msg_x() failed to lock mp_hSyncSema even though there were pending messages\n" );
		}
	}
	psNode = psPort->mp_psFirstMsg;

	if ( psNode == NULL )
	{
		printk( "get_msg_x() locked mp_hSyncSema even though the message list was empty on port %d\n", hPort );
		goto again;
	}

	nRealSize = min( nSize, psNode->mn_nSize );

	psPort->mp_psFirstMsg = psNode->mn_psNext;
	if ( NULL == psPort->mp_psFirstMsg )
	{
		psPort->mp_psLastMsg = NULL;
	}
	psPort->mp_nCount--;

	if ( NULL != pBuffer )
	{
		if( bFromKernel )
			memcpy( pBuffer, psNode + 1, nRealSize );
		else
			memcpy_to_user( pBuffer, psNode + 1, nRealSize );
	}
	if ( NULL != pnCode )
	{
		if( bFromKernel )
			*pnCode = psNode->mn_nCode;
		else
			memcpy_to_user( pnCode, &psNode->mn_nCode, sizeof( psNode->mn_nCode ) );
	}
	nError = ( nSize >= psNode->mn_nSize ) ? 0 : -E2BIG;

	kfree( psNode );
      error1:
	unlock_mutex( g_hPortListSema );
	return ( nError );
}


/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

status_t get_msg_x( port_id hPort, uint32 *pnCode, void *pBuffer, int nSize, bigtime_t nTimeOut )
{
	int nError;

	nError = do_get_msg_x( true, hPort, pnCode, pBuffer, nSize, nTimeOut );
	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

status_t sys_send_msg( port_id hPort, uint32 nCode, const void *pBuffer, int nSize )
{
	int nError;

	nError = send_msg_x( hPort, nCode, pBuffer, nSize, INFINITE_TIMEOUT );
	Schedule();
	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

status_t sys_raw_send_msg_x( port_id hPort, uint32 nCode, const void *pBuffer, int nSize, const bigtime_t *pnTimeOut )
{
	int nError;

	nError = send_msg_x( hPort, nCode, pBuffer, nSize, *pnTimeOut );
	Schedule();
	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

status_t sys_get_msg( port_id hPort, uint32 *pnCode, void *pBuffer, int nSize )
{
	int nError;

	nError = do_get_msg_x( false, hPort, pnCode, pBuffer, nSize, INFINITE_TIMEOUT );
	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

status_t sys_raw_get_msg_x( port_id hPort, uint32 *pnCode, void *pBuffer, int nSize, const bigtime_t *pnTimeOut )
{
	int nError;

	nError = do_get_msg_x( false, hPort, pnCode, pBuffer, nSize, *pnTimeOut );
	return ( nError );
}

static void do_get_port_info( MsgPort_s *psPort, port_info * psInfo )
{
	psInfo->pi_port_id = psPort->mp_hPortID;
	psInfo->pi_flags = psPort->mp_nFlags;
	psInfo->pi_max_count = psPort->mp_nMaxCount;
	psInfo->pi_cur_count = psPort->mp_nCount;
	psInfo->pi_owner = psPort->mp_hOwner;
	strcpy( psInfo->pi_name, psPort->mp_zName );
}

status_t sys_get_port_info( port_id hPort, port_info * psInfo )
{
	port_info sInfo;
	MsgPort_s *psPort;
	int nError;

	lock_mutex( g_hPortListSema, true );
	if ( hPort == -1 )
	{
		hPort = MArray_GetNextIndex( &g_sMsgPorts, -1 );
	}
	psPort = get_port_from_handle( hPort );
	if ( psPort == NULL )
	{
		nError = -EINVAL;
		goto error;
	}
	do_get_port_info( psPort, &sInfo );
	nError = 0;
	if ( memcpy_to_user( psInfo, &sInfo, sizeof( sInfo ) ) < 0 )
	{
		nError = -EFAULT;
	}
      error:
	unlock_mutex( g_hPortListSema );
	return ( nError );
}

status_t sys_get_next_port_info( port_info * psInfo )
{
	port_info sInfo;
	MsgPort_s *psPort;
	port_id hPort;
	int nError;

	if ( memcpy_from_user( &hPort, &psInfo->pi_port_id, sizeof( hPort ) ) < 0 )
	{
		return ( -EFAULT );
	}

	lock_mutex( g_hPortListSema, true );
	hPort = MArray_GetNextIndex( &g_sMsgPorts, hPort );

	psPort = get_port_from_handle( hPort );
	if ( psPort == NULL )
	{
		nError = 0;
		goto error;
	}
	do_get_port_info( psPort, &sInfo );
	nError = 1;
	if ( memcpy_to_user( psInfo, &sInfo, sizeof( sInfo ) ) < 0 )
	{
		nError = -EFAULT;
	}
      error:
	unlock_mutex( g_hPortListSema );
	return ( nError );
}

void exit_free_ports( Process_s *psProc )
{
	lock_mutex( g_hPortListSema, true );
	while ( psProc->pr_psFirstPort != NULL )
	{
		do_delete_port( psProc, psProc->pr_psFirstPort );
	}
	unlock_mutex( g_hPortListSema );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/
status_t make_port_public( port_id hPort )
{
	MsgPort_s *psPort;
	PublicPort_s *psPubPort;
	int nError = EOK;
	
	psPort = get_port_from_handle( hPort );

	if ( psPort == NULL )
	{
		printk( "Error: make_port_public() called with invalid message port %d\n", hPort );
		nError = -EINVAL;
		goto error;
	}

	/* Ensure that the port name does not already exist in the public ports list */
	if ( do_find_port( psPort->mp_zName ) >= 0 )
	{
		printk( "make_port_public() : Attempt to make a duplicate port \"%s\" public\n", psPort->mp_zName );
		nError = -EEXIST;
		goto error;
	}

	psPubPort = kmalloc( sizeof( PublicPort_s ), MEMF_KERNEL | MEMF_LOCKED | MEMF_OKTOFAIL );

	if ( psPubPort == NULL )
	{
		printk( "Error: make_port_public() failed to alloc port struct\n" );
		nError = -ENOMEM;
		goto error;
	}

	psPubPort->pp_hPortID = psPort->mp_hPortID;
	psPubPort->pp_pzName = psPort->mp_zName;

	psPubPort->pp_psNext = NULL;
	psPubPort->pp_psPrev = g_psPubPortListHead;

	lock_mutex( g_hPubPortListSema, true );

	if ( NULL != g_psPubPortListHead )
		g_psPubPortListHead->pp_psNext = psPubPort;
	
	g_psPubPortListHead = psPubPort;

	unlock_mutex( g_hPubPortListSema );

	lock_mutex( g_hPortListSema, true );

	psPort->mp_nFlags |= MSG_PORT_PUBLIC;

	unlock_mutex( g_hPortListSema );

      error:
	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/
status_t make_port_private( port_id hPort )
{
	MsgPort_s *psPort;
	PublicPort_s *psPubPort;
	int nError = -EINVAL;

	psPort = get_port_from_handle( hPort );
	
	if ( psPort == NULL )
	{
		printk( "Error: make_port_private() called for invalid message port %d\n", hPort );
		nError = -EINVAL;
		goto error;
	}

	lock_mutex( g_hPubPortListSema, true );

	psPubPort = g_psPubPortListHead;

	
	while ( psPubPort != NULL )
	{

		if ( psPubPort->pp_hPortID == hPort )
		{
			if( psPubPort->pp_psPrev )
				psPubPort->pp_psPrev->pp_psNext = psPubPort->pp_psNext;
			if( psPubPort->pp_psNext )
				psPubPort->pp_psNext->pp_psPrev = psPubPort->pp_psPrev;
			else
				g_psPubPortListHead = psPubPort->pp_psPrev;

			kfree( psPubPort );

			lock_mutex( g_hPortListSema, true );

			psPort->mp_nFlags &= ~MSG_PORT_PUBLIC;

			unlock_mutex( g_hPortListSema );

			nError = EOK;
			break;
		}

		psPubPort = psPubPort->pp_psPrev;

	}
	

	unlock_mutex( g_hPubPortListSema );

	if ( nError == -EINVAL )
		printk( "Error: make_port_private() called with a non public port id %d\n", hPort );

      error:
	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/
port_id do_find_port( const char *pzPortname )
{
	PublicPort_s *psPubPort;
	port_id hPort = -1;

	if ( NULL == g_psPubPortListHead )
		goto error;

	lock_mutex( g_hPubPortListSema, true );

	psPubPort = g_psPubPortListHead;

	while ( psPubPort != NULL )
	{

		if ( !strcmp( psPubPort->pp_pzName, pzPortname ) )
		{
			hPort = psPubPort->pp_hPortID;
			break;
		}

		psPubPort = psPubPort->pp_psPrev;

	}

	unlock_mutex( g_hPubPortListSema );

      error:
	return ( hPort );
}

port_id find_port( const char *pzPortname )
{
	port_id hPort = -1;
	char *pzName;

	pzName = kmalloc( OS_NAME_LENGTH, MEMF_KERNEL | MEMF_LOCKED | MEMF_OKTOFAIL );

	if ( pzName == NULL )
	{
		printk( "Error: find_port() failed to alloc memory\n" );
		goto error;
	}

	strncpy_from_user( pzName, pzPortname, OS_NAME_LENGTH );
	pzName[OS_NAME_LENGTH - 1] = '\0';

	hPort = do_find_port( pzName );

	kfree( pzName );

      error:
	return ( hPort );
}

size_t sys_get_msg_size( port_id hPort )
{
	MsgPort_s *psPort = NULL;
	int nError = 0;

	lock_mutex( g_hPortListSema, true );

	psPort = get_port_from_handle( hPort );

	if ( NULL == psPort )
	{
		printk( "sys_get_msg_size() attempt to receive message from invalid port %d\n", hPort );
		nError = -EINVAL;
		goto error;
	}

again:
	if ( psPort->mp_psFirstMsg == NULL )
	{
		sem_id hSyncSema;

		// We take a backup here since the port might get deleted
		// when we release the g_hPortListSema semaphore.
		// If the port is deleted so is the semaphore and lock_semaphore()
		// will return an error. It's then very important not to
		// touch the port again until we has revalidated the port-handle.

		hSyncSema = psPort->mp_hSyncSema;

		unlock_mutex( g_hPortListSema );
		nError = lock_semaphore( hSyncSema, 0, INFINITE_TIMEOUT );
		lock_mutex( g_hPortListSema, true );

		if ( nError < 0 )
			goto error;

		// We must revalidate the handle after unlocking g_hPortListSema
		psPort = get_port_from_handle( hPort );

		if ( NULL == psPort )
		{
			nError = -EINVAL;
			printk( "Message port %d deleted while we were wating for a message\n", hPort );
			goto error;
		}
	}
	else
		if( lock_semaphore( psPort->mp_hSyncSema, SEM_NOSIG, 0LL ) < 0 )
			printk( "sys_get_msg_size() failed to lock mp_hSyncSema even though there were pending messages\n" );

	if ( psPort->mp_psFirstMsg == NULL )
	{
		printk( "sys_get_msg_size() locked mp_hSyncSema even though the message list was empty on port %d\n", hPort );
		goto again;
	}

	nError = psPort->mp_psFirstMsg->mn_nSize;

	unlock_semaphore( psPort->mp_hSyncSema );
error:
	unlock_mutex( g_hPortListSema );
	return nError;
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/
status_t sys_make_port_public( port_id hPort )
{
	int nError;

	nError = make_port_public( hPort );
	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/
status_t sys_make_port_private( port_id hPort )
{
	int nError;

	nError = make_port_private( hPort );
	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/
port_id sys_find_port( const char *pzPortname )
{
	port_id hPort;

	hPort = find_port( pzPortname );
	return ( hPort );
}


/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/
static void db_list_public_ports( int argc, char **argv )
{
	/* Dump all public message ports */
	PublicPort_s *psPubPort;
	
	psPubPort = g_psPubPortListHead;

	while ( psPubPort != NULL )
	{
		port_id hPrevious = 0;
		port_id hNext = 0;
		if( psPubPort->pp_psPrev )
			hPrevious = psPubPort->pp_psPrev->pp_hPortID;
		if( psPubPort->pp_psNext )
			hNext = psPubPort->pp_psNext->pp_hPortID;
		
		dbprintf( DBP_DEBUGGER, "%04i ( Previous %04i Next %04i )\n", psPubPort->pp_hPortID,
					hPrevious, hNext );
		
		psPubPort = psPubPort->pp_psPrev;
	}
}


/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

void init_msg_ports( void )
{
	register_debug_cmd( "ls_pubports", "list all public message ports.", db_list_public_ports );
	g_hPortListSema = create_semaphore( "port_list__bad1", 1, 0 );
	g_hPortListSema = create_semaphore( "port_list__bad2", 1, 0 );
	g_hPortListSema = create_semaphore( "port_list__bad3", 1, 0 );
	g_hPortListSema = create_semaphore( "port_list", 1, 0 );
	MArray_Init( &g_sMsgPorts );

	g_hPubPortListSema = create_semaphore( "public_port_list", 1, 0 );
	g_psPubPortListHead = NULL;

}
