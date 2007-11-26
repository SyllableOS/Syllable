
/*
 *  The AtheOS kernel
 *  Copyright (C) 1999 - 2001 Kurt Skauen
 *  Copyright (C) 2002 Ville Kallioniemi
 *
 *  Changes:
 *	 2002	Ville Kallioniemi		Added raw socket handling and dump_socket_info
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
#include <posix/uio.h>
#include <posix/fcntl.h>

#include <atheos/kernel.h>
#include <atheos/kdebug.h>
#include <atheos/filesystem.h>
#include <atheos/socket.h>
#include <atheos/semaphore.h>

#include <macros.h>

#include <net/net.h>
#include <net/ip.h>
#include <net/if.h>
#include <net/if_ether.h>
#include <net/if_arp.h>
#include <net/icmp.h>
#include <net/udp.h>
#include <net/tcp.h>
#include <net/raw.h>
#include <net/in.h>
#include <net/route.h>
#include <net/sockios.h>

#include "vfs/vfs.h"

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/
void dump_socket_info( Socket_s *psSocket )
{
	kerndbg( KERN_DEBUG, "DUMP_SOCKET_INFO(): \n" );
	kerndbg( KERN_DEBUG, "socket->\n" );
	kerndbg( KERN_DEBUG, "sk_nInodeNum:\t%d\n", ( int )psSocket->sk_nInodeNum );
	kerndbg( KERN_DEBUG, "sk_nFamily:\t%d\n", psSocket->sk_nFamily );
	kerndbg( KERN_DEBUG, "sk_nType:\t%d\n", psSocket->sk_nType );
	kerndbg( KERN_DEBUG, "sk_nProto:\t%d\n", psSocket->sk_nProto );
}


int create_socket( bool bKernel, int nFamily, int nType, int nProtocol, bool bInitProtocol, Socket_s **ppsRes )
{
	Socket_s *psSocket;
	File_s *psFile;
	int nError = 0;

	kerndbg( KERN_DEBUG_LOW, "CREATE_SOCKET(): called with nType: %d, nProtocol: %d\n", nType, nProtocol );
	// If the protocol isn't specified we'll set the default for the socket type
	if ( nProtocol == 0 )
	{
		switch ( nType )
		{
		case SOCK_DGRAM:
			nProtocol = IPPROTO_UDP;
			break;
		case SOCK_STREAM:
			nProtocol = IPPROTO_TCP;
			break;
		case SOCK_RAW:
			nProtocol = IPPROTO_RAW;
			break;
		default:
			kerndbg( KERN_WARNING, "create_socket() unknown type %d\n", nType );
			return ( -EINVAL );
		}
	}


	psSocket = kmalloc( sizeof( Socket_s ), MEMF_KERNEL | MEMF_CLEAR );

	if ( psSocket == NULL )
	{
		kerndbg( KERN_FATAL, "Error: sys_socket() ran out of memory\n" );
		return ( -ENOMEM );
	}

	psSocket->sk_nInodeNum = ( ino_t )( ( uint32 )psSocket );
	atomic_set( &psSocket->sk_nOpenCount, 1 );
	psSocket->sk_nFamily = nFamily;
	psSocket->sk_nType = nType;
	psSocket->sk_nProto = nProtocol;

	if ( bInitProtocol )
	{

		if ( nType == SOCK_RAW )
		{
			kerndbg( KERN_DEBUG_LOW, "CREATE_SOCKET(): socket type RAW.\n" );
			nError = raw_open( psSocket );
		}
		else
		{
			switch ( nProtocol )
			{
			case IPPROTO_TCP:
				nError = tcp_open( psSocket );
				break;
			case IPPROTO_UDP:
				nError = udp_open( psSocket );
				break;
			case IPPROTO_RAW:
				nError = raw_open( psSocket );
				break;
			default:
				kerndbg( KERN_WARNING, "create_socket() unknown protocol %d\n", nProtocol );
				nError = -EINVAL;
				break;
			}
		}
	}

	if ( nError < 0 )
	{
		kfree( psSocket );
		return ( nError );
	}

	psFile = alloc_fd();

	if ( psFile == NULL )
	{
		kerndbg( KERN_FATAL, "Error: sys_socket() no memory for file struct\n" );
		kfree( psSocket );
		return ( -ENOMEM );
	}
	psFile->f_nType = FDT_SOCKET;
	psFile->f_psInode = get_inode( FSID_SOCKET, psSocket->sk_nInodeNum, false );
	if ( psFile->f_psInode == NULL )
	{
		kerndbg( KERN_FATAL, "sys_socket() failed to get socket inode\n" );
		free_fd( psFile );
		kfree( psSocket );
		return ( -ENOMEM );
	}
	psFile->f_nMode = O_RDWR;
	nError = new_fd( bKernel, -1, 0, psFile, false );
	if ( nError < 0 )
	{
		put_inode( psFile->f_psInode );	// Will delete the socket object
		free_fd( psFile );
		return ( nError );
	}
	if ( ppsRes != NULL )
	{
		*ppsRes = psSocket;
	}
	add_file_to_inode( psFile->f_psInode, psFile );
	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int sys_socket( int nFamily, int nType, int nProtocol )
{
	return ( create_socket( false, nFamily, nType, nProtocol, true, NULL ) );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int socket( int nFamily, int nType, int nProtocol )
{
	return ( create_socket( true, nFamily, nType, nProtocol, true, NULL ) );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int sys_closesocket( int nFile )
{

	sys_close( nFile );
	return ( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int closesocket( int nFile )
{
	return ( close( nFile ) );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

Socket_s *get_socket( File_s *psFile )
{
	if ( psFile->f_psInode->i_psVolume->v_nDevNum != FSID_SOCKET )
	{
		return ( NULL );
	}
	return ( psFile->f_psInode->i_pFSData );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int do_shutdown( bool bKernel, int nFile, int nHow )
{
	File_s *psFile;
	Socket_s *psSocket;
	int nError;
	int nMask;

	nMask = nHow + 1;	/* maps 0->1 has the advantage of making bit 1 rcvs and
				 * 1->2 bit 2 snds.
				 * 2->3
				 */

	psFile = get_fd( bKernel, nFile );

	if ( psFile == NULL )
	{
		nError = -EBADF;
		goto error1;
	}

	psSocket = get_socket( psFile );

	if ( psSocket == NULL )
	{
		nError = -ENOTSOCK;
		goto error2;
	}

	if ( psSocket->sk_psOps->shutdown != NULL )
	{
		nError = psSocket->sk_psOps->shutdown( psSocket, nMask );
	}
	else
	{
		nError = -EINVAL;
	}

      error2:
	put_fd( psFile );
      error1:
	return ( nError );
}

int shutdown( int nFile, int nHow )
{
	return ( do_shutdown( true, nFile, nHow ) );
}


int sys_shutdown( int nFile, int nHow )
{
	return ( do_shutdown( false, nFile, nHow ) );
}

static int sockopt_getintval( bool bKernel, const void *pOptVal )
{
	/* Get the option value */
	int nValue = 0;

	if( bKernel )
		memcpy( &nValue, pOptVal, sizeof( int ) );
	else
	{
		if( memcpy_from_user( &nValue, pOptVal, sizeof( int ) ) < 0 )
			return -EFAULT;
	}
	return !!nValue;
}

int do_setsockopt( bool bKernel, int nFile, int nLevel, int nOptName, const void *pOptVal, int nOptLen )
{
	File_s *psFile;
	Socket_s *psSocket;
	int nError = -EINVAL;

	psFile = get_fd( bKernel, nFile );

	if( psFile == NULL )
	{
		nError = -EBADF;
		goto error1;
	}

	psSocket = get_socket( psFile );

	if( psSocket == NULL )
	{
		nError = -ENOTSOCK;
		goto error2;
	}

	switch( nLevel )
	{
		case SOL_SOCKET:
		{
			switch( nOptName )
			{
				case SO_DEBUG:
				{
					int nValue = sockopt_getintval( bKernel, pOptVal );
					psSocket->sk_bDebug = (nValue ? true : false );
					nError = 0;
					break;
				}

				case SO_BROADCAST:
				{
					int nValue = sockopt_getintval( bKernel, pOptVal );
					psSocket->sk_bBroadcast = (nValue ? true : false );
					nError = 0;
					break;
				}

				case SO_REUSEADDR:
				{
					int nValue = sockopt_getintval( bKernel, pOptVal );
					psSocket->sk_bReuseAddr = (nValue ? true : false );
					nError = 0;
					break;
				}

				case SO_OOBINLINE:
				{
					int nValue = sockopt_getintval( bKernel, pOptVal );
					psSocket->sk_bOobInline = (nValue ? true : false );
					nError = 0;
					break;
				}

				case SO_LINGER:
				case SO_RCVLOWAT:
				case SO_RCVTIMEO:
				case SO_SNDLOWAT:
				case SO_SNDTIMEO:
					nError = -EINVAL;
					break;

				case SO_KEEPALIVE:
				case SO_SNDBUF:
				case SO_RCVBUF:
				case SO_DONTROUTE:
				default:
				{
					if( psSocket->sk_psOps->setsockopt != NULL )
						nError = psSocket->sk_psOps->setsockopt( bKernel, psSocket, nLevel, nOptName, pOptVal, nOptLen );
					else
						nError = -EINVAL;
					break;
				}
			}
			break;
		}

		/* XXXKV: Should this be somewhere else? */
		case SOL_IP:
		{
			switch( nOptName )
			{
				case IP_TOS:
					nError = 0;
					break;

				default:
				{
					if( psSocket->sk_psOps->setsockopt != NULL )
						nError = psSocket->sk_psOps->setsockopt( bKernel, psSocket, nLevel, nOptName, pOptVal, nOptLen );
					else
						nError = -EINVAL;
					break;
				}
			}
			break;
		}

		/* Some other protocol, maybe the lower protocol understands it? */
		default:
		{
			if( psSocket->sk_psOps->setsockopt != NULL )
				nError = psSocket->sk_psOps->setsockopt( bKernel, psSocket, nLevel, nOptName, pOptVal, nOptLen );
			else
				nError = -EINVAL;
			break;
		}
	}

error2:
	put_fd( psFile );
error1:
	return nError;
}

int sys_setsockopt( int nFile, int nLevel, int nOptName, const void *pOptVal, int nOptLen )
{
	return do_setsockopt( false, nFile, nLevel, nOptName, pOptVal, nOptLen );
}

int setsockopt( int nFile, int nLevel, int nOptName, const void *pOptVal, int nOptLen )
{
	return do_setsockopt( true, nFile, nLevel, nOptName, pOptVal, nOptLen );
}

static void sockopt_setintval( bool bKernel, void *pOptVal, int nValue )
{
	/* Set the option value */
	if( bKernel )
		memcpy( pOptVal, &nValue, sizeof( int ) );
	else
		memcpy_to_user( pOptVal, &nValue, sizeof( int ) );
}

static void sockopt_setboolval( bool bKernel, void *pOptVal, bool bValue )
{
	/* Set the boolean option value */
	int nValue = bValue ? 1 : 0;

	sockopt_setintval( bKernel, pOptVal, nValue );
}

int do_getsockopt( bool bKernel, int nFile, int nLevel, int nOptName, void *pOptVal, int nOptLen )
{
	File_s *psFile;
	Socket_s *psSocket;
	int nError = -EINVAL;

	psFile = get_fd( bKernel, nFile );

	if ( psFile == NULL )
	{
		nError = -EBADF;
		goto error1;
	}

	psSocket = get_socket( psFile );

	if ( psSocket == NULL )
	{
		nError = -ENOTSOCK;
		goto error2;
	}

	switch( nLevel )
	{
		case SOL_SOCKET:
		{
			switch( nOptName )
			{
				case SO_DEBUG:
				{
					sockopt_setboolval( bKernel, pOptVal, psSocket->sk_bDebug );
					nError = 0;
					break;
				}

				case SO_BROADCAST:
				{
					sockopt_setboolval( bKernel, pOptVal, psSocket->sk_bBroadcast );
					nError = 0;
					break;
				}

				case SO_REUSEADDR:
				{
					sockopt_setboolval( bKernel, pOptVal, psSocket->sk_bReuseAddr );
					nError = 0;
					break;
				}

				case SO_KEEPALIVE:
				{
					sockopt_setboolval( bKernel, pOptVal, psSocket->sk_bKeep );
					nError = 0;
					break;
				}

				case SO_OOBINLINE:
				{
					sockopt_setboolval( bKernel, pOptVal, psSocket->sk_bOobInline );
					nError = 0;
					break;
				}

				case SO_ERROR:
				case SO_SNDBUF:
				case SO_RCVBUF:
				{
					if( psSocket->sk_psOps->getsockopt != NULL )
						nError = psSocket->sk_psOps->getsockopt( bKernel, psSocket, nLevel, nOptName, pOptVal, nOptLen );
					else
						nError = -EINVAL;
					break;
				}

				case SO_TYPE:
				{
					sockopt_setintval( bKernel, pOptVal, psSocket->sk_nType );
					nError = 0;
					break;
				}

				case SO_DONTROUTE:
				{
					sockopt_setboolval( bKernel, pOptVal, psSocket->sk_bDontRoute );
					nError = 0;
					break;
				}

				case SO_LINGER:
				case SO_RCVLOWAT:
				case SO_RCVTIMEO:
				case SO_SNDLOWAT:
				case SO_SNDTIMEO:
					nError = -EINVAL;
					break;

				default:
				{
					kerndbg( KERN_DEBUG, "%s: invalid option %d for protocol SOL_SOCKET\n", __FUNCTION__, nOptName );
					nError = -EINVAL;
					break;
				}
			}
			break;
		}

		/* Some other protocol, maybe the lower protocol understands it? */
		default:
		{
			if( psSocket->sk_psOps->getsockopt != NULL )
				nError = psSocket->sk_psOps->getsockopt( bKernel, psSocket, nLevel, nOptName, pOptVal, nOptLen );
			else
				nError = -EINVAL;
			break;
		}
	}

error2:
	put_fd( psFile );
error1:
	return nError;
}

int sys_getsockopt( int nFile, int nLevel, int nOptName, void *pOptVal, int nOptLen )
{
	return do_getsockopt( false, nFile, nLevel, nOptName, pOptVal, nOptLen );
}

int getsockopt( int nFile, int nLevel, int nOptName, void *pOptVal, int nOptLen )
{
	return do_getsockopt( true, nFile, nLevel, nOptName, pOptVal, nOptLen );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int do_getpeername( bool bKernel, int nFile, struct sockaddr *psName, int *pnNameLen )
{
	File_s *psFile;
	Socket_s *psSocket;
	int nError;

	psFile = get_fd( bKernel, nFile );

	if ( psFile == NULL )
	{
		nError = -EBADF;
		goto error1;
	}

	psSocket = get_socket( psFile );

	if ( psSocket == NULL )
	{
		nError = -ENOTSOCK;
		goto error2;
	}

	if ( psSocket->sk_psOps->getpeername != NULL )
	{
		nError = psSocket->sk_psOps->getpeername( psSocket, psName, pnNameLen );
	}
	else
	{
		nError = -EINVAL;
	}

      error2:
	put_fd( psFile );
      error1:
	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int sys_getpeername( int nFile, struct sockaddr *psName, int *pnNameLen )
{
	return ( do_getpeername( false, nFile, psName, pnNameLen ) );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int getpeername( int nFile, struct sockaddr *psName, int *pnNameLen )
{
	return ( do_getpeername( true, nFile, psName, pnNameLen ) );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int do_getsockname( bool bKernel, int nFile, struct sockaddr *psName, int *pnNameLen )
{
	File_s *psFile;
	Socket_s *psSocket;
	int nError;

	psFile = get_fd( bKernel, nFile );

	if ( psFile == NULL )
	{
		nError = -EBADF;
		goto error1;
	}

	psSocket = get_socket( psFile );

	if ( psSocket == NULL )
	{
		nError = -ENOTSOCK;
		goto error2;
	}

	if ( psSocket->sk_psOps->getsockname != NULL )
	{
		nError = psSocket->sk_psOps->getsockname( psSocket, psName, pnNameLen );
	}
	else
	{
		nError = -EINVAL;
	}

      error2:
	put_fd( psFile );
      error1:
	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int sys_getsockname( int nFile, struct sockaddr *psName, int *pnNameLen )
{
	return ( do_getsockname( false, nFile, psName, pnNameLen ) );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int getsockname( int nFile, struct sockaddr *psName, int *pnNameLen )
{
	return ( do_getsockname( true, nFile, psName, pnNameLen ) );
}


/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int do_connect( bool bKernel, int nFile, const struct sockaddr *psAddr, int nAddrSize )
{
	struct sockaddr_in *psInAddr = ( struct sockaddr_in * )psAddr;
	File_s *psFile;
	Socket_s *psSocket;
	int nError;

	if ( psInAddr->sin_family != AF_INET )
	{
		kerndbg( KERN_WARNING, "sys_connect() unknown family %04x\n", psInAddr->sin_family );
		nError = -EAFNOSUPPORT;
		goto error1;
	}

	psFile = get_fd( bKernel, nFile );

	if ( psFile == NULL )
	{
		nError = -EBADF;
		goto error1;
	}

	psSocket = get_socket( psFile );

	if ( psSocket == NULL )
	{
		nError = -ENOTSOCK;
		goto error2;
	}

	IP_COPYADDR( psSocket->sk_anDstAddr, psInAddr->sin_addr );
	psSocket->sk_nDstPort = ntohw( psInAddr->sin_port );

	if ( psSocket->sk_psOps->connect != NULL )
	{
		nError = psSocket->sk_psOps->connect( psSocket, psAddr, nAddrSize );
	}
	else
	{
		nError = -EINVAL;
	}

      error2:
	put_fd( psFile );
      error1:
	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int sys_connect( int nFile, const struct sockaddr *psAddr, int nAddrSize )
{
	return ( do_connect( false, nFile, psAddr, nAddrSize ) );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int connect( int nFile, const struct sockaddr *psAddr, int nAddrSize )
{
	return ( do_connect( true, nFile, psAddr, nAddrSize ) );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int do_bind( bool bKernel, int nFile, const struct sockaddr *psAddr, int nAddrSize )
{
	struct sockaddr_in *psInAddr = ( struct sockaddr_in * )psAddr;
	File_s *psFile;
	Socket_s *psSocket;
	int nError;

	if ( psInAddr->sin_family != AF_INET )
	{
		kerndbg( KERN_WARNING, "sys_bind() unknown family %04x\n", psInAddr->sin_family );
		nError = -EAFNOSUPPORT;
		goto error1;
	}

	psFile = get_fd( bKernel, nFile );

	if ( psFile == NULL )
	{
		nError = -EBADF;
		goto error1;
	}

	psSocket = get_socket( psFile );

	if ( psSocket == NULL )
	{
		nError = -ENOTSOCK;
		goto error2;
	}

	IP_COPYADDR( psSocket->sk_anSrcAddr, psInAddr->sin_addr );
	psSocket->sk_nSrcPort = ntohs( psInAddr->sin_port );
	psSocket->sk_bIsBound = true;

	if ( psSocket->sk_psOps->bind != NULL )
	{
		nError = psSocket->sk_psOps->bind( psSocket, psAddr, nAddrSize );
	}
	else
	{
		nError = -EINVAL;
	}
      error2:
	put_fd( psFile );
      error1:
	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int sys_bind( int nFile, const struct sockaddr *psAddr, int nAddrSize )
{
	return ( do_bind( false, nFile, psAddr, nAddrSize ) );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int bind( int nFile, const struct sockaddr *psAddr, int nAddrSize )
{
	return ( do_bind( true, nFile, psAddr, nAddrSize ) );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int do_listen( bool bKernel, int nFile, int nQueueSize )
{
	File_s *psFile;
	Socket_s *psSocket;
	int nError;

	psFile = get_fd( bKernel, nFile );

	if ( psFile == NULL )
	{
		nError = -EBADF;
		goto error1;
	}

	psSocket = get_socket( psFile );

	if ( psSocket == NULL )
	{
		nError = -ENOTSOCK;
		goto error2;
	}

	if ( psSocket->sk_psOps->listen != NULL )
	{
		nError = psSocket->sk_psOps->listen( psSocket, nQueueSize );
	}
	else
	{
		nError = -EINVAL;
	}
      error2:
	put_fd( psFile );
      error1:
	return ( nError );
}


int sys_listen( int nFile, int nQueueSize )
{
	return ( do_listen( false, nFile, nQueueSize ) );
}

int listen( int nFile, int nQueueSize )
{
	return ( do_listen( true, nFile, nQueueSize ) );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int do_accept( bool bKernel, int nFile, struct sockaddr *psAddr, int *pnSize )
{
	File_s *psFile;
	Socket_s *psSocket;
	int nError;

	psFile = get_fd( bKernel, nFile );

	if ( psFile == NULL )
	{
		nError = -EBADF;
		goto error1;
	}

	psSocket = get_socket( psFile );

	if ( psSocket == NULL )
	{
		nError = -ENOTSOCK;
		goto error2;
	}

	if ( psSocket->sk_psOps->accept != NULL )
	{
		nError = psSocket->sk_psOps->accept( psSocket, psAddr, pnSize );
	}
	else
	{
		nError = -EINVAL;
	}
      error2:
	put_fd( psFile );
      error1:
	return ( nError );
}

int sys_accept( int nFile, struct sockaddr *psAddr, int *pnSize )
{
	// FIXME: Validate the pointers!!!!!!!!!!!!!!!!!
	return ( do_accept( false, nFile, psAddr, pnSize ) );
}

int accept( int nFile, struct sockaddr *psAddr, int *pnSize )
{
	return ( do_accept( true, nFile, psAddr, pnSize ) );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int do_sendmsg( bool bKernel, int nFile, const struct msghdr *psMsg, int nFlags )
{
	File_s *psFile;
	Socket_s *psSocket;
	int nError;

	psFile = get_fd( bKernel, nFile );

	if ( psFile == NULL )
	{
		nError = -EBADF;
		goto error1;
	}

	psSocket = get_socket( psFile );

	if ( psSocket == NULL )
	{
		nError = -ENOTSOCK;
		goto error2;
	}

	if ( psSocket->sk_psOps->sendmsg != NULL )
	{
		nError = psSocket->sk_psOps->sendmsg( psSocket, psMsg, nFlags );
	}
	else
	{
		nError = -EINVAL;
	}

      error2:
	put_fd( psFile );
      error1:
	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static ssize_t do_recvmsg( bool bKernel, int nFile, struct msghdr *psMsg, int nFlags )
{
	File_s *psFile;
	Socket_s *psSocket;
	int nError;

	psFile = get_fd( bKernel, nFile );

	if ( psFile == NULL )
	{
		nError = -EBADF;
		goto error1;
	}

	psSocket = get_socket( psFile );

	if ( psSocket == NULL )
	{
		nError = -ENOTSOCK;
		goto error2;
	}
	if ( psSocket->sk_psOps->recvmsg != NULL )
	{
		nError = psSocket->sk_psOps->recvmsg( psSocket, psMsg, nFlags );
	}
	else
	{
		nError = -EINVAL;
	}
      error2:
	put_fd( psFile );
      error1:
	return ( nError );

//  return( udp_recvfrom( nFile, pBuffer, nSize, nFlags, psMsg->msg_name, &psMsg->msg_namelen ) );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

ssize_t sys_recvmsg( int nFile, struct msghdr *psMsg, int nFlags )
{
	return ( do_recvmsg( false, nFile, psMsg, nFlags ) );
}

ssize_t recvmsg( int nFile, struct msghdr *psMsg, int nFlags )
{
	return ( do_recvmsg( true, nFile, psMsg, nFlags ) );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

ssize_t sys_sendmsg( int nFile, const struct msghdr *psMsg, int nFlags )
{
	return ( do_sendmsg( false, nFile, psMsg, nFlags ) );
}

ssize_t sendmsg( int nFile, const struct msghdr *psMsg, int nFlags )
{
	return ( do_sendmsg( true, nFile, psMsg, nFlags ) );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int recvfrom( int nFile, void *pBuffer, size_t nSize, int nFlags, struct sockaddr *psFrom, int *pnFromLen )
{
	struct msghdr sMsg;
	struct iovec sIov;
	int nError;

	sIov.iov_base = pBuffer;
	sIov.iov_len = nSize;

	sMsg.msg_iov = &sIov;
	sMsg.msg_iovlen = 1;
	sMsg.msg_name = psFrom;
	sMsg.msg_namelen = *pnFromLen;
	nError = do_recvmsg( true, nFile, &sMsg, nFlags );
	*pnFromLen = sMsg.msg_namelen;
	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int sendto( int nFile, const void *pBuffer, size_t nSize, int nFlags, const struct sockaddr *psFrom, int pnFromLen )
{
	struct msghdr sMsg;
	struct iovec sIov;
	int nError;

	sIov.iov_base = ( void * )pBuffer;
	sIov.iov_len = nSize;

	sMsg.msg_iov = &sIov;
	sMsg.msg_iovlen = 1;
	sMsg.msg_name = ( void * )psFrom;
	sMsg.msg_namelen = pnFromLen;
	nError = do_sendmsg( true, nFile, &sMsg, nFlags );
	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int sk_read_inode( void *pVolume, ino_t nInodeNum, void **ppNode )
{
	Socket_s *psSocket;

	psSocket = ( Socket_s * )( ( int )nInodeNum );
	*ppNode = psSocket;
	return ( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int sk_write_inode( void *pVolData, void *pNode )
{
	Socket_s *psSocket = pNode;

	kfree( psSocket );
	return ( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int sk_open( void *pVolume, void *pNode, int nMode, void **ppCookie )
{
	Socket_s *psSocket = pNode;

//  kassertw( psSocket->sk_bOpen == false );
	atomic_inc( &psSocket->sk_nOpenCount );
	return ( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int sk_close( void *pVolume, void *pNode, void *pCookie )
{
	Socket_s *psSocket = pNode;
	int nError;

	atomic_dec( &psSocket->sk_nOpenCount );
	kassertw( atomic_read( &psSocket->sk_nOpenCount ) >= 0 );

	if ( atomic_read( &psSocket->sk_nOpenCount ) == 0 && psSocket->sk_psOps->close != NULL )
	{
		nError = psSocket->sk_psOps->close( psSocket );
	}
	else
	{
		nError = 0;
	}

	return ( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int sk_read( void *pVolume, void *pNode, void *pCookie, off_t nPos, void *pBuffer, size_t nSize )
{
	Socket_s *psSocket = pNode;
	struct msghdr sMsg;
	struct iovec sIov;
	int nError;

	sIov.iov_base = pBuffer;
	sIov.iov_len = nSize;

	sMsg.msg_iov = &sIov;
	sMsg.msg_iovlen = 1;
	sMsg.msg_name = NULL;
	sMsg.msg_namelen = 0;

	if ( psSocket->sk_psOps->recvmsg != NULL )
	{
		nError = psSocket->sk_psOps->recvmsg( psSocket, &sMsg, 0 );
	}
	else
	{
		nError = -EINVAL;
	}
	return ( nError );
}

static int sk_readv( void *pVolume, void *pNode, void *pCookie, off_t nPos, const struct iovec *psVector, size_t nCount )
{
	Socket_s *psSocket = pNode;
	struct msghdr sMsg;
	int nError;

	sMsg.msg_iov = ( struct iovec * )psVector;
	sMsg.msg_iovlen = nCount;
	sMsg.msg_name = NULL;
	sMsg.msg_namelen = 0;

	if ( psSocket->sk_psOps->recvmsg != NULL )
	{
		nError = psSocket->sk_psOps->recvmsg( psSocket, &sMsg, 0 );
	}
	else
	{
		nError = -EINVAL;
	}
	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int sk_write( void *pVolume, void *pNode, void *pCookie, off_t nPos, const void *pBuffer, size_t nSize )
{
	Socket_s *psSocket = pNode;
	struct msghdr sMsg;
	struct iovec sIov;
	int nError;

	sIov.iov_base = ( void * )pBuffer;
	sIov.iov_len = nSize;

	sMsg.msg_iov = &sIov;
	sMsg.msg_iovlen = 1;
	sMsg.msg_name = NULL;
	sMsg.msg_namelen = 0;

	if ( psSocket->sk_psOps->sendmsg != NULL )
	{
		nError = psSocket->sk_psOps->sendmsg( psSocket, &sMsg, 0 );
	}
	else
	{
		nError = -EINVAL;
	}
	return ( nError );
}

static int sk_writev( void *pVolume, void *pNode, void *pCookie, off_t nPos, const struct iovec *psVector, size_t nCount )
{
	Socket_s *psSocket = pNode;
	struct msghdr sMsg;
	int nError;

	sMsg.msg_iov = ( struct iovec * )psVector;
	sMsg.msg_iovlen = nCount;
	sMsg.msg_name = NULL;
	sMsg.msg_namelen = 0;

	if ( psSocket->sk_psOps->sendmsg != NULL )
	{
		nError = psSocket->sk_psOps->sendmsg( psSocket, &sMsg, 0 );
	}
	else
	{
		nError = -EINVAL;
	}
	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int sk_add_select( void *pVolume, void *pNode, void *pCookie, SelectRequest_s *psReq )
{
	Socket_s *psSocket = pNode;
	int nError;

	if ( psSocket->sk_psOps->add_select != NULL )
	{
		nError = psSocket->sk_psOps->add_select( psSocket, psReq );
	}
	else
	{
		kerndbg( KERN_WARNING, "Error: Protocol %d dont support add_select\n", psSocket->sk_nProto );
		nError = -EINVAL;
	}

	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int sk_rem_select( void *pVolume, void *pNode, void *pCookie, SelectRequest_s *psReq )
{
	Socket_s *psSocket = pNode;
	int nError;

	if ( psSocket->sk_psOps->rem_select != NULL )
	{
		nError = psSocket->sk_psOps->rem_select( psSocket, psReq );
	}
	else
	{
		kerndbg( KERN_WARNING, "Error: Protocol %d dont support rem_select\n", psSocket->sk_nProto );
		nError = -EINVAL;
	}

	return ( nError );
}



static int sk_setflags( void *pVolume, void *pNode, void *pCookie, uint32 nFlags )
{
	Socket_s *psSocket = pNode;
	int nError;

	if ( psSocket->sk_psOps->set_fflags != NULL )
	{
		nError = psSocket->sk_psOps->set_fflags( psSocket, nFlags );
	}
	else
	{
		kerndbg( KERN_WARNING, "Error: Protocol %d dont support rem_setflags\n", psSocket->sk_nProto );
		nError = -EINVAL;
	}

	return ( nError );
}

static int sk_rstat( void *pVolume, void *pNode, struct stat *psStat )
{
	Socket_s *psSocket = pNode;

	psStat->st_dev = FSID_SOCKET;
	psStat->st_ino = ( int )psSocket;
	psStat->st_size = 0;
	psStat->st_mode = S_IFSOCK | 0777;
	psStat->st_atime = 0;
	psStat->st_mtime = 0;
	psStat->st_ctime = 0;
	psStat->st_uid = 0;
	psStat->st_gid = 0;

	return ( 0 );
}

int sk_ioctl( void *pVolume, void *pNode, void *pCookie, int nCmd, void *pBuf, bool bFromKernel )
{
	Socket_s *psSocket = pNode;
	int nError;

	switch ( nCmd )
	{
	case SIOCADDRT:
		nError = add_route_entry( ( struct rtentry * )pBuf );
		break;
	case SIOCDELRT:
		nError = delete_route_entry( ( struct rtentry * )pBuf );
		break;
	case SIOCGETRTAB:
		nError = get_route_table( ( struct rttable * )pBuf );
		break;
	default:
		if ( nCmd >= SIO_FIRST_IF && nCmd <= SIO_LAST_IF )
		{
			nError = netif_ioctl( nCmd, pBuf );
		}
		else
		{
			if ( psSocket->sk_psOps->ioctl != NULL )
			{
				nError = psSocket->sk_psOps->ioctl( psSocket, nCmd, pBuf, bFromKernel );
			}
			else
			{
				nError = -ENOSYS;
			}
		}
		break;
	}
	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static FSOperations_s g_sOperations = {
	NULL,			// op_probe
	NULL,			// op_mount
	NULL,			// op_unmount
	sk_read_inode,
	sk_write_inode,
	NULL,			/* Lookup       */
	NULL,			/* access       */
	NULL,			/* create       */
	NULL,			/* mkdir        */
	NULL,			/* mknod        */
	NULL,			/* symlink      */
	NULL,			/* link         */
	NULL,			/* rename       */
	NULL,			/* unlink       */
	NULL,			/* rmdir        */
	NULL,			/* readlink     */
	NULL,			/* opendir      */
	NULL,			/* closedir     */
	NULL,			/* RewindDir    */
	NULL,			/* ReadDir      */
	sk_open,		/* open         */
	sk_close,		/* close        */
	NULL,			/* FreeCookie   */
	sk_read,		// op_read
	sk_write,		// op_write
	sk_readv,		// op_readv
	sk_writev,		// op_writev
	sk_ioctl,		// op_ioctl
	sk_setflags,		/* setflags     */
	sk_rstat,		// rfs_rstat,
	NULL,			/* wstat        */
	NULL,			/* fsync        */
	NULL,			/* initialize   */
	NULL,			/* sync         */
	NULL,			/* rfsstat      */
	NULL,			/* wfsstat      */
	NULL,			/* isatty       */
	sk_add_select,
	sk_rem_select
};

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

void init_sockets( void )
{
	Volume_s *psKVolume = kmalloc( sizeof( Volume_s ), MEMF_KERNEL | MEMF_CLEAR );

	if ( NULL == psKVolume )
	{
		kerndbg( KERN_FATAL, "Error: Failed to create socket fs-volume\n" );
		return;
	}
	psKVolume->v_psNext = NULL;
	psKVolume->v_nDevNum = FSID_SOCKET;
	psKVolume->v_pFSData = NULL;	// psVolume;
	psKVolume->v_psOperations = &g_sOperations;

	add_mount_point( psKVolume );
}
