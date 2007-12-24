#include <sys/types.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <malloc.h>
#include <errno.h>
#include <unistd.h>
#include <utime.h>
#include <string.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <atheos/kernel.h>
#include <atheos/time.h>
#include <sys/uio.h>

#include "../../sys/kernel/fs/nfs/nfs.h"

#define MAX_PACKET_SIZE 1518

static int g_nLastHandle = 1;
struct sockaddr_in g_sSrvAddr;
int g_nSocket = -1;

typedef struct _NfsFileNode NfsFileNode_s;

struct _NfsFileNode
{
	NfsFileNode_s *psNext;
	NfsFileNode_s *psFirstChild;
	NfsFileNode_s *psParent;
	NfsDirEntry_s *psFirstDirEnt;
	int nFile;
	int nHandle;
	bigtime_t nDirReadTime;
	char zName[256];
};

NfsFileNode_s *g_psFirstNode = NULL;
NfsFileNode_s *g_psRoot;



int recvfrom_nfs( int nFile, void *pBuffer, size_t nSize, int nFlags, struct sockaddr *psFrom, int *pnFromLen )
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
	nError = recvmsg( nFile, &sMsg, nFlags );
	*pnFromLen = sMsg.msg_namelen;
	return ( nError );
}

int sendto_nfs( int nFile, const void *pBuffer, size_t nSize, int nFlags, const struct sockaddr *psFrom, int pnFromLen )
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
	nError = sendmsg( nFile, &sMsg, nFlags );
	return ( nError );
}


NfsFileNode_s *find_node( int nHandle )
{
	NfsFileNode_s *psNode;

	if( nHandle == -1 )
	{
		return ( g_psRoot );
	}

	for( psNode = g_psFirstNode; psNode != NULL; psNode = psNode->psNext )
	{
		if( psNode->nHandle == nHandle )
		{
			return ( psNode );
		}
	}
	return ( NULL );
}

int get_abs_path( char *pzBuffer, NfsFileNode_s * psNode )
{
	if( NULL != psNode->psParent )
	{
		int nLen;

		nLen = get_abs_path( pzBuffer, psNode->psParent );

		if( -1 != nLen )
		{
			pzBuffer[nLen] = '/';
			nLen++;
			strcpy( &pzBuffer[nLen], psNode->zName );

			return ( nLen + strlen( psNode->zName ) );
		}
		else
		{
			return ( -1 );
		}
	}
	else
	{
		strcpy( pzBuffer, psNode->zName );
		return ( strlen( psNode->zName ) );
	}
}

int srv_create_node( NfsFileNode_s * psParent, const char *pzName )
{
	NfsFileNode_s *psNode;

	psNode = malloc( sizeof( NfsFileNode_s ) );

	if( psNode == NULL )
	{
		return ( -ENOMEM );
	}
	memset( psNode, 0, sizeof( NfsFileNode_s ) );

	psNode->nHandle = g_nLastHandle++;
	if( g_nLastHandle < 0 )
	{
		g_nLastHandle = 1;
	}

	strcpy( psNode->zName, pzName );

	psNode->psParent = psParent;

	psNode->psNext = g_psFirstNode;
	g_psFirstNode = psNode;

	return ( psNode->nHandle );
}

int srv_open( int nParent, const char *pzPath )
{
	NfsFileNode_s *psParent = find_node( nParent );

//  NfsFileNode_s* psNode;
	int nFile;
	int nError;
	char zAbsPath[1024];

	if( psParent == NULL )
	{
		printf( "srv_open() invalid parent %d\n", nParent );
		return ( -ESTALE );
	}
	get_abs_path( zAbsPath, psParent );
	strcat( zAbsPath, "/" );
	strcat( zAbsPath, pzPath );
	nFile = open( zAbsPath, O_RDONLY );

	if( nFile < 0 )
	{
		nError = -errno;
		goto error1;
	}
	close( nFile );
	return ( srv_create_node( psParent, pzPath ) );
      error1:
	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

void srv_inval_dir_cache( NfsFileNode_s * psNode )
{
	while( psNode->psFirstDirEnt != NULL )
	{
		NfsDirEntry_s *psEntry;

		psEntry = psNode->psFirstDirEnt;
		psNode->psFirstDirEnt = psEntry->psNext;
		free( psEntry );
	}
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int srv_create( int nParent, const char *pzPath, int nPerms )
{
	NfsFileNode_s *psParent = find_node( nParent );
	int nFile;
	int nError;
	char zAbsPath[1024];

	if( psParent == NULL )
	{
		printf( "srv_create() invalid parent %d\n", nParent );
		return ( -ESTALE );
	}
	get_abs_path( zAbsPath, psParent );
	strcat( zAbsPath, "/" );
	strcat( zAbsPath, pzPath );
	nFile = open( zAbsPath, O_WRONLY | O_CREAT, nPerms );

	if( nFile < 0 )
	{
		nError = -errno;
		goto error1;
	}
	close( nFile );

	srv_inval_dir_cache( psParent );
	return ( srv_create_node( psParent, pzPath ) );
      error1:
	return ( nError );
}

int srv_mkdir( int nParent, const char *pzPath, int nPerms )
{
	NfsFileNode_s *psParent = find_node( nParent );

//  NfsFileNode_s* psNode;
	int nError;
	char zAbsPath[1024];

	if( psParent == NULL )
	{
		printf( "srv_mkdir() invalid parent %d\n", nParent );
		return ( -ESTALE );
	}
	get_abs_path( zAbsPath, psParent );
	strcat( zAbsPath, "/" );
	strcat( zAbsPath, pzPath );
	nError = mkdir( zAbsPath, nPerms );

	if( nError < 0 )
	{
		nError = -errno;
	}

	srv_inval_dir_cache( psParent );

	return ( nError );
}

int srv_rmdir( int nParent, const char *pzPath )
{
	NfsFileNode_s *psParent = find_node( nParent );
	int nError;
	char zAbsPath[1024];

	if( psParent == NULL )
	{
		printf( "srv_rmdir() invalid parent %d\n", nParent );
		return ( -ESTALE );
	}
	get_abs_path( zAbsPath, psParent );
	strcat( zAbsPath, "/" );
	strcat( zAbsPath, pzPath );
	nError = rmdir( zAbsPath );

	if( nError < 0 )
	{
		nError = -errno;
	}

	srv_inval_dir_cache( psParent );

	return ( nError );
}

int srv_unlink( int nParent, const char *pzPath )
{
	NfsFileNode_s *psParent = find_node( nParent );
	int nError;
	char zAbsPath[1024];

	if( psParent == NULL )
	{
		printf( "srv_unlink() invalid parent %d\n", nParent );
		return ( -1 );
	}
	get_abs_path( zAbsPath, psParent );
	strcat( zAbsPath, "/" );
	strcat( zAbsPath, pzPath );
	nError = unlink( zAbsPath );

	if( nError < 0 )
	{
		nError = -errno;
	}

	srv_inval_dir_cache( psParent );

	return ( nError );
}


/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int srv_readdir( int nHandle, int nPos, char *pzBuffer )
{
	NfsFileNode_s *psNode = find_node( nHandle );
	NfsDirEntry_s *psEntry;

	if( psNode == NULL )
	{
		printf( "srv_readdir() invalid handle %d\n", nHandle );
		return ( -ESTALE );
	}
//  printf( "read dir %s - %p\n", psNode->zName, psNode->psFirstDirEnt );

	if( psNode->psFirstDirEnt != NULL && ( ( get_real_time() - psNode->nDirReadTime ) > 5000000LL ) )
	{
		srv_inval_dir_cache( psNode );
	}

	if( psNode->psFirstDirEnt == NULL )
	{
		char zAbsPath[1024];
		DIR *pDir;
		struct dirent *psDirEnt;

		get_abs_path( zAbsPath, psNode );

		pDir = opendir( zAbsPath );
		if( NULL == pDir )
		{
			printf( "Failed to open directory %s\n", zAbsPath );
			return ( -ENOENT );
		}
		while( ( psDirEnt = readdir( pDir ) ) != NULL )
		{
			psEntry = malloc( sizeof( NfsDirEntry_s ) );

			if( psEntry == NULL )
			{
				printf( "no memory for direntry\n" );
				break;
			}
			strcpy( psEntry->zName, psDirEnt->d_name );
			psEntry->psNext = psNode->psFirstDirEnt;
			psNode->psFirstDirEnt = psEntry;
		}
		closedir( pDir );
		psNode->nDirReadTime = get_real_time();
	}
	for( psEntry = psNode->psFirstDirEnt; psEntry != NULL; psEntry = psEntry->psNext )
	{
		if( nPos-- == 0 )
		{
			strcpy( pzBuffer, psEntry->zName );
			return ( 1 );
		}
	}
	return ( 0 );
}

int srv_read( int nHandle, void *pBuffer, off_t nPos, int nSize )
{
	NfsFileNode_s *psNode = find_node( nHandle );
	char zAbsPath[1024];
	int nFile;
	int nError;

	if( psNode == NULL )
	{
		printf( "srv_read() invalid handle %d\n", nHandle );
		return ( -ESTALE );
	}

	get_abs_path( zAbsPath, psNode );

	nFile = open( zAbsPath, O_RDONLY );

	if( nFile < 0 )
	{
		return ( -errno );
	}

	lseek( nFile, nPos, SEEK_SET );
	nError = read( nFile, pBuffer, nSize );
	close( nFile );

	if( nError < 0 )
	{
		nError = -errno;
	}

	return ( nError );
}

int srv_write( int nHandle, void *pBuffer, off_t nPos, int nSize )
{
	NfsFileNode_s *psNode = find_node( nHandle );
	char zAbsPath[1024];
	int nFile;
	int nError;

	if( psNode == NULL )
	{
		printf( "srv_write() invalid handle %d\n", nHandle );
		return ( -ESTALE );
	}

	get_abs_path( zAbsPath, psNode );

	nFile = open( zAbsPath, O_WRONLY );

	if( nFile < 0 )
	{
		return ( -errno );
	}

//  printf( "Write %d bytes at %d\n", nSize, nPos );
	lseek( nFile, nPos, SEEK_SET );
	nError = write( nFile, pBuffer, nSize );

	if( nError < 0 )
	{
		nError = -errno;
	}

	close( nFile );


	return ( nError );
}

int srv_readlink( int nHandle, char *pzBuffer )
{
	NfsFileNode_s *psNode = find_node( nHandle );
	char zAbsPath[1024];
	int nError;

	if( psNode == NULL )
	{
		printf( "srv_stat() invalid handle %d\n", nHandle );
		return ( -ESTALE );
	}

	get_abs_path( zAbsPath, psNode );

//  printf( "readlink %s\n", zAbsPath );

	pzBuffer[0] = 0;
	nError = readlink( zAbsPath, pzBuffer, NFS_MAX_READ_BUF );
	if( nError < 0 )
	{
		nError = -errno;
	}
//  printf( "readlink %s - %s (%d)\n", zAbsPath, pzBuffer, nError );

	return ( nError );
}

int srv_stat( int nHandle, struct stat *psStat )
{
	NfsFileNode_s *psNode = find_node( nHandle );
	char zAbsPath[1024];
	int nError;

	if( psNode == NULL )
	{
		printf( "srv_stat() invalid handle %d\n", nHandle );
		return ( -ESTALE );
	}

	get_abs_path( zAbsPath, psNode );

//  printf( "Stat %s\n", zAbsPath );

	nError = lstat( zAbsPath, psStat );
	if( nError < 0 )
	{
		nError = -errno;
	}
	else
	{
		if( S_ISLNK( psStat->st_mode ) )
		{
		}
	}

	return ( nError );
}

int srv_wstat( int nHandle, struct stat *psStat )
{
	NfsFileNode_s *psNode = find_node( nHandle );
	char zAbsPath[1024];
	struct utimbuf sUTime;
	int nError;

	if( psNode == NULL )
	{
		printf( "srv_wstat() invalid handle %d\n", nHandle );
		return ( -ESTALE );
	}

	get_abs_path( zAbsPath, psNode );

	sUTime.modtime = psStat->st_mtime;
	sUTime.actime = psStat->st_atime;

	nError = utime( zAbsPath, &sUTime );

	if( nError < 0 )
	{
		nError = -errno;
	}

	return ( nError );
}

int srv_rename( int nOldDir, const char *pzOldName, int nNewDir, const char *pzNewName )
{
	NfsFileNode_s *psOldDir = find_node( nOldDir );
	NfsFileNode_s *psNewDir = find_node( nNewDir );
	char zOldPath[1024];
	char zNewPath[1024];
	int nError;

	if( psOldDir == NULL )
	{
		printf( "srv_wstat() invalid old-dir handle %d\n", nOldDir );
		return ( -ESTALE );
	}
	if( psNewDir == NULL )
	{
		printf( "srv_wstat() invalid new-dir handle %d\n", nNewDir );
		return ( -ESTALE );
	}

	get_abs_path( zOldPath, psOldDir );
	get_abs_path( zNewPath, psNewDir );

	strcat( zOldPath, "/" );
	strcat( zOldPath, pzOldName );
	strcat( zNewPath, "/" );
	strcat( zNewPath, pzNewName );

	if( rename( zOldPath, zNewPath ) < 0 )
	{
		nError = -errno;
	}
	else
	{
		srv_inval_dir_cache( psOldDir );
		srv_inval_dir_cache( psNewDir );
		nError = 0;
	}
	return ( nError );
}

int main( int argc, char **argv )
{
	char anBuffer[MAX_PACKET_SIZE];
	NfsPacketHdr_s *psHdr = ( NfsPacketHdr_s * ) anBuffer;
	struct sockaddr_in sClientAddr;

	srandom( get_real_time() );
	g_nLastHandle = random() % 10000000;

	if( g_nLastHandle < 0 )
	{
		g_nLastHandle = 1;
	}

	g_psRoot = malloc( sizeof( NfsFileNode_s ) );

	memset( g_psRoot, 0, sizeof( NfsFileNode_s ) );
	g_psRoot->nHandle = -1;
	g_psRoot->zName[0] = '/';
	g_psRoot->zName[1] = '\0';

	g_nSocket = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP );
	if( g_nSocket < 0 )
	{
		printf( "Failed to create socket: %s\n", strerror( errno ) );
		return ( 1 );
	}

	memset( &g_sSrvAddr, 0, sizeof( g_sSrvAddr ) );

	g_sSrvAddr.sin_family = AF_INET;
	if( argc == 2 )
	{
		g_sSrvAddr.sin_port = htons( atol( argv[1] ) );
	}
	else
	{
		g_sSrvAddr.sin_port = htons( 100 );
	}

	printf( "Start ANFS server at port %d\n", ntohs( g_sSrvAddr.sin_port ) );

	if( bind( g_nSocket, ( struct sockaddr * )&g_sSrvAddr, sizeof( g_sSrvAddr ) ) < 0 )
	{
		printf( "Failed to bind socket: %s\n", strerror( errno ) );
		return ( 1 );
	}

	for( ;; )
	{
		int nAddrLen = sizeof( sClientAddr );
		int nSize = recvfrom_nfs( g_nSocket, anBuffer, MAX_PACKET_SIZE, 0, ( struct sockaddr * )&sClientAddr, &nAddrLen );

		if( nSize <= 0 )
		{
			continue;
		}
    //printf( "Got cmd %d (%i) from %d\n", psHdr->nCmd, psHdr->nSeqNum, ntohs( sClientAddr.sin_port ) );

		switch ( psHdr->nCmd )
		{
		case NFS_OPEN:
			{
				NfsOpen_s *psMsg = ( NfsOpen_s * ) psHdr;
				NfsOpenReply_s sReply;

//      printf( "Got open req\n" );

				sReply.nHandle = srv_open( psMsg->nParent, psMsg->zPath );
				sReply.sHdr.nSeqNum = psMsg->sHdr.nSeqNum;

				sendto_nfs( g_nSocket, &sReply, sizeof( sReply ), 0, ( struct sockaddr * )&sClientAddr, sizeof( sClientAddr ) );

				break;
			}
		case NFS_CLOSE:
			{
				NfsClose_s *psMsg = ( NfsClose_s * ) psHdr;
				NfsCloseReply_s sReply;

				sReply.nError = 0;
				sReply.sHdr.nSeqNum = psMsg->sHdr.nSeqNum;

				sendto_nfs( g_nSocket, &sReply, sizeof( sReply ), 0, ( struct sockaddr * )&sClientAddr, sizeof( sClientAddr ) );
				break;
			}
		case NFS_CREATE:
			{
				NfsCreate_s *psMsg = ( NfsCreate_s * ) psHdr;
				NfsCreateReply_s sReply;

				sReply.nHandle = srv_create( psMsg->nParent, psMsg->zName, psMsg->nPerms );
				sReply.sHdr.nSeqNum = psMsg->sHdr.nSeqNum;

				sendto_nfs( g_nSocket, &sReply, sizeof( sReply ), 0, ( struct sockaddr * )&sClientAddr, sizeof( sClientAddr ) );
				break;
			}
		case NFS_MKDIR:
			{
				NfsMkDir_s *psMsg = ( NfsMkDir_s * ) psHdr;
				NfsMkDirReply_s sReply;

				sReply.nError = srv_mkdir( psMsg->nParent, psMsg->zName, psMsg->nPerms );
				sReply.sHdr.nSeqNum = psMsg->sHdr.nSeqNum;

				sendto_nfs( g_nSocket, &sReply, sizeof( sReply ), 0, ( struct sockaddr * )&sClientAddr, sizeof( sClientAddr ) );
				break;
			}
		case NFS_RMDIR:
			{
				NfsRmDir_s *psMsg = ( NfsRmDir_s * ) psHdr;
				NfsRmDirReply_s sReply;

				sReply.nError = srv_rmdir( psMsg->nParent, psMsg->zName );
				sReply.sHdr.nSeqNum = psMsg->sHdr.nSeqNum;

				sendto_nfs( g_nSocket, &sReply, sizeof( sReply ), 0, ( struct sockaddr * )&sClientAddr, sizeof( sClientAddr ) );
				break;
			}
		case NFS_UNLINK:
			{
				NfsUnlink_s *psMsg = ( NfsUnlink_s * ) psHdr;
				NfsUnlinkReply_s sReply;

				sReply.nError = srv_unlink( psMsg->nParent, psMsg->zName );
				sReply.sHdr.nSeqNum = psMsg->sHdr.nSeqNum;

				sendto_nfs( g_nSocket, &sReply, sizeof( sReply ), 0, ( struct sockaddr * )&sClientAddr, sizeof( sClientAddr ) );
				break;
			}
		case NFS_READ_DIR:
			{
				NfsReadDir_s *psMsg = ( NfsReadDir_s * ) psHdr;
				NfsReadDirReply_s sReply;

//      printf( "Got readdir (%d) %d - %d\n", nSize, psMsg->nHandle, psMsg->nPos );
				sReply.nError = srv_readdir( psMsg->nHandle, psMsg->nPos, sReply.zName );
				sReply.sHdr.nSeqNum = psMsg->sHdr.nSeqNum;

				sendto_nfs( g_nSocket, &sReply, sizeof( sReply ), 0, ( struct sockaddr * )&sClientAddr, sizeof( sClientAddr ) );
				break;
			}
		case NFS_READLINK:
			{
				NfsReadLink_s *psMsg = ( NfsReadLink_s * ) psHdr;
				NfsReadLinkReply_s sReply;

				sReply.nError = srv_readlink( psMsg->nHandle, sReply.zBuffer );
				sReply.sHdr.nSeqNum = psMsg->sHdr.nSeqNum;
				sendto_nfs( g_nSocket, &sReply, sizeof( sReply ), 0, ( struct sockaddr * )&sClientAddr, sizeof( sClientAddr ) );
				break;
			}
		case NFS_STAT:
			{
				NfsStat_s *psMsg = ( NfsStat_s * ) psHdr;
				NfsStatReply_s sReply;

				sReply.nError = srv_stat( psMsg->nHandle, &sReply.sStat );
				sReply.sHdr.nSeqNum = psMsg->sHdr.nSeqNum;
				sendto_nfs( g_nSocket, &sReply, sizeof( sReply ), 0, ( struct sockaddr * )&sClientAddr, sizeof( sClientAddr ) );
				break;
			}
		case NFS_WSTAT:
			{
				NfsWStat_s *psMsg = ( NfsWStat_s * ) psHdr;
				NfsWStatReply_s sReply;

				sReply.nError = srv_wstat( psMsg->nHandle, &psMsg->sStat );
				sReply.sHdr.nSeqNum = psMsg->sHdr.nSeqNum;
				sendto_nfs( g_nSocket, &sReply, sizeof( sReply ), 0, ( struct sockaddr * )&sClientAddr, sizeof( sClientAddr ) );
				break;
			}

		case NFS_READ:
			{
				NfsRead_s *psMsg = ( NfsRead_s * ) psHdr;
				NfsReadReply_s sReply;

				sReply.nError = srv_read( psMsg->nHandle, sReply.anBuffer, psMsg->nPos, psMsg->nSize );
				sReply.sHdr.nSeqNum = psMsg->sHdr.nSeqNum;
				sendto_nfs( g_nSocket, &sReply, sizeof( sReply ), 0, ( struct sockaddr * )&sClientAddr, sizeof( sClientAddr ) );
				break;
			}
		case NFS_WRITE:
			{
				NfsWrite_s *psMsg = ( NfsWrite_s * ) psHdr;
				NfsWriteReply_s sReply;

				sReply.nError = srv_write( psMsg->nHandle, psMsg->anBuffer, psMsg->nPos, psMsg->nSize );
				sReply.sHdr.nSeqNum = psMsg->sHdr.nSeqNum;
				sendto_nfs( g_nSocket, &sReply, sizeof( sReply ), 0, ( struct sockaddr * )&sClientAddr, sizeof( sClientAddr ) );
				break;
			}
		case NFS_RENAME:
			{
				NfsRename_s *psMsg = ( NfsRename_s * ) psHdr;
				NfsRenameReply_s sReply;

				sReply.nError = srv_rename( psMsg->nOldDir, psMsg->zOldName, psMsg->nNewDir, psMsg->zNewName );
				sReply.sHdr.nSeqNum = psMsg->sHdr.nSeqNum;
				sendto_nfs( g_nSocket, &sReply, sizeof( sReply ), 0, ( struct sockaddr * )&sClientAddr, sizeof( sClientAddr ) );
				break;
			}
		}
	}
}
