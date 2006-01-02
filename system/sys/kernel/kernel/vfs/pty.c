
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
#include <posix/termios.h>
#include <posix/stat.h>
#include <posix/fcntl.h>
#include <posix/dirent.h>
#include <posix/signal.h>

#include <atheos/types.h>
#include <atheos/time.h>

#include <atheos/kernel.h>
#include <atheos/smp.h>
#include <atheos/filesystem.h>
#include <atheos/schedule.h>
#include <atheos/irq.h>

#include <atheos/ctype.h>
#include <atheos/semaphore.h>

#include <macros.h>

#include "inc/scheduler.h"
#include "inc/sysbase.h"
#include "inc/smp.h"

typedef struct FileNode FileNode_s;
typedef struct SuperInfo SuperInfo_s;

#define PTY_MAX_NAME_LEN	64

static void pty_notify_read_select( FileNode_s *psNode );
static void pty_notify_write_select( FileNode_s *psNode );
static sem_id g_hCTTYMutex = -1;

enum
{
	PTY_ROOT = 1,
	PTY_SLAVE,
	PTY_MASTER
};

enum
{
	FLAG_XOFF  = 1,		// XON/XOFF mode and stop key (^S) was pressed
	FLAG_LNEXT = 2		// lnext key (^V) pressed; next char is literal
};

static const size_t PTY_BUFFER_SIZE = 1024;
#define NUM_SAVED_TABS	16	// saved cursor position before pressing TAB
static const size_t TAB_SIZE = 8;

#define	 IS_MASTER( node )	(NULL != (node)->fn_psParent && PTY_MASTER == (node)->fn_psParent->fn_nInodeNum )
#define	 IS_SLAVE( node )	(NULL != (node)->fn_psParent && PTY_SLAVE == (node)->fn_psParent->fn_nInodeNum )

#define INlToCr		( psTermios->c_iflag & INLCR )
#define IIgnoreCr	( psTermios->c_iflag & IGNCR )
#define ICrToNl		( psTermios->c_iflag & ICRNL )
#define IUcToLc		( psTermios->c_iflag & IUCLC )
#define Echo		( psTermios->c_lflag & ECHO )
#define EchoCtl		( psTermios->c_lflag & ECHOCTL )
#define EchoNl		( psTermios->c_lflag & ECHONL )
#define ICanon		( psTermios->c_lflag & ICANON )
#define ISignals	( psTermios->c_lflag & ISIG )
#define IXonXoff	( psTermios->c_iflag & IXON )

typedef struct
{
	struct termios sTermios;
	struct winsize sWinSize;
	int nSessionID;
	pid_t hPGroupID;
	uint32 nCtrlStatus;
} TermInfo_s;

struct FileNode
{
	FileNode_s *fn_psNextHash;
	FileNode_s *fn_psNextSibling;
	FileNode_s *fn_psParent;
	FileNode_s *fn_psFirstChild;

	FileNode_s *fn_psPartner;

	int fn_nInodeNum;
	int fn_nMode;
	uid_t fn_nUID;
	gid_t fn_nGID;
	time_t fn_nCTime;
	time_t fn_nMTime;
	time_t fn_nATime;
	int fn_nSize;
	int fn_nNewLineCount;
	int fn_nCsrPos;
	int fn_nFlags;
	uint8_t fn_nCsrPosBeforeTab[NUM_SAVED_TABS];

	int fn_nOpenCount;

	sem_id fn_hMutex;
	sem_id fn_hWriteMutex;

	sem_id fn_hReadQueue;
	sem_id fn_hWriteQueue;

	SelectRequest_s *fn_psFirstReadSelReq;
	SelectRequest_s *fn_psFirstWriteSelReq;

	char *fn_pzBuffer;

	uint fn_nReadPos;
	uint fn_nWritePos;

	TermInfo_s *fn_psTermInfo;
	char fn_zName[PTY_MAX_NAME_LEN];
	bool fn_bPacketMode;
	bool fn_bReleased;	// true when write_inode() has been called.
};

typedef struct
{
	kdev_t nDevNum;
	FileNode_s *psRootNode;		///<  /dev/pty
	FileNode_s *psMasterNode;	///<  /dev/pty/mst
	FileNode_s *psSlaveNode;	///<  /dev/pty/slv
} PtyVolume_s;

typedef struct
{
	int nFlags;
} PtyFileCookie_s;


/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static FileNode_s *pty_create_node( FileNode_s *psParent, const char *pzName, int nNameLen, int nMode )
{
	FileNode_s *psNode;

	if ( nNameLen < 1 || nNameLen >= PTY_MAX_NAME_LEN )
	{
		return ( NULL );
	}

	psNode = kmalloc( sizeof( FileNode_s ), MEMF_KERNEL | MEMF_CLEAR );
	if ( psNode == NULL )
	{
		return ( NULL );
	}
	psNode->fn_hMutex = create_semaphore( "pty_node_mutex", 1, SEM_RECURSIVE );
	if ( psNode->fn_hMutex < 0 )
	{
		goto error1;
	}
	psNode->fn_hWriteMutex = create_semaphore( "pty_write_mutex", 1, SEM_RECURSIVE );
	if ( psNode->fn_hWriteMutex < 0 )
	{
		goto error2;
	}
	psNode->fn_hReadQueue = create_semaphore( "pty_read_queue", 0, 0 );
	if ( psNode->fn_hReadQueue < 0 )
	{
		goto error3;
	}
	psNode->fn_hWriteQueue = create_semaphore( "pty_write_queue", 0, 0 );
	if ( psNode->fn_hWriteQueue < 0 )
	{
		goto error4;
	}

	memcpy( psNode->fn_zName, pzName, nNameLen );
	psNode->fn_zName[nNameLen] = 0;
	psNode->fn_nMode = nMode;

	psNode->fn_psNextSibling = psParent->fn_psFirstChild;
	psParent->fn_psFirstChild = psNode;
	psNode->fn_psParent = psParent;
	psNode->fn_nInodeNum = ( int )psNode;
	psNode->fn_nCTime = get_real_time() / 1000000;
	psNode->fn_nMTime = psNode->fn_nCTime;
	psNode->fn_nATime = psNode->fn_nCTime;
	return ( psNode );
      error4:
	delete_semaphore( psNode->fn_hReadQueue );
      error3:
	delete_semaphore( psNode->fn_hWriteMutex );
      error2:
	delete_semaphore( psNode->fn_hMutex );
      error1:
	kfree( psNode );
	return ( NULL );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static void pty_delete_node( FileNode_s *psNode )
{
	FileNode_s *psParent = psNode->fn_psParent;
	FileNode_s **ppsTmp;

	if ( NULL == psParent )
	{
		printk( "iiik, sombody tried to delete /dev/pty !!!\n" );
		return;
	}

	for ( ppsTmp = &psParent->fn_psFirstChild; NULL != *ppsTmp; ppsTmp = &( ( *ppsTmp )->fn_psNextSibling ) )
	{
		if ( *ppsTmp == psNode )
		{
			*ppsTmp = psNode->fn_psNextSibling;
			break;
		}
	}
	delete_semaphore( psNode->fn_hMutex );
	delete_semaphore( psNode->fn_hWriteMutex );
	delete_semaphore( psNode->fn_hReadQueue );
	delete_semaphore( psNode->fn_hWriteQueue );

	kfree( psNode );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int pty_mount( kdev_t nDevNum, const char *pzDevPath, uint32 nFlags, const void *pArgs, int nArgLen, void **ppVolData, ino_t *pnRootIno )
{
	PtyVolume_s *psVolume = kmalloc( sizeof( PtyVolume_s ), MEMF_KERNEL | MEMF_CLEAR );
	FileNode_s *psRootNode;
	FileNode_s *psMasterNode;
	FileNode_s *psSlaveNode;

	if ( ( psRootNode = kmalloc( sizeof( FileNode_s ), MEMF_KERNEL | MEMF_CLEAR ) ) )
	{
		*ppVolData = psVolume;

		psRootNode->fn_zName[0] = '\0';

		psRootNode->fn_nMode = S_IFDIR;
		psRootNode->fn_nInodeNum = PTY_ROOT;

		psRootNode->fn_psNextSibling = NULL;
		psRootNode->fn_psParent = NULL;
		psRootNode->fn_hMutex = create_semaphore( "pty_root", 1, SEM_RECURSIVE );
		psRootNode->fn_hWriteMutex = create_semaphore( "pty_root_write", 1, SEM_RECURSIVE );

		psVolume->nDevNum = nDevNum;

		psMasterNode = pty_create_node( psRootNode, "mst", 3, S_IFDIR | S_IRWXUGO );
		psMasterNode->fn_nInodeNum = PTY_MASTER;

		psSlaveNode = pty_create_node( psRootNode, "slv", 3, S_IFDIR | S_IRWXUGO );
		psSlaveNode->fn_nInodeNum = PTY_SLAVE;

		psVolume->psRootNode = psRootNode;
		psVolume->psMasterNode = psMasterNode;
		psVolume->psSlaveNode = psSlaveNode;

		*pnRootIno = psRootNode->fn_nInodeNum;

		return ( 0 );
	}
	return ( -ENOMEM );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static FileNode_s *pty_find_child_node( FileNode_s *psParentNode, const char *pzName, int nNameLen )
{
	FileNode_s *psNewNode;

	for ( psNewNode = psParentNode->fn_psFirstChild; NULL != psNewNode; psNewNode = psNewNode->fn_psNextSibling )
	{
		if ( strncmp( psNewNode->fn_zName, pzName, nNameLen ) == 0 )
		{
			return ( psNewNode );
		}
	}
	return ( NULL );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int pty_lookup( void *pVolume, void *pParent, const char *pzName, int nNameLen, ino_t *pnResInode )
{
	FileNode_s *psParentNode = pParent;
	FileNode_s *psNewNode;

	*pnResInode = 0;

	if ( nNameLen == 2 && '.' == pzName[0] && '.' == pzName[1] )
	{
		if ( NULL != psParentNode->fn_psParent )
		{
			*pnResInode = psParentNode->fn_psParent->fn_nInodeNum;
			return ( 0 );
		}
		else
		{
			return ( -ENOENT );
		}
	}

	LOCK( psParentNode->fn_hMutex );
	for ( psNewNode = psParentNode->fn_psFirstChild; NULL != psNewNode; psNewNode = psNewNode->fn_psNextSibling )
	{
		if ( strncmp( psNewNode->fn_zName, pzName, nNameLen ) == 0 )
		{
			*pnResInode = psNewNode->fn_nInodeNum;
			break;
		}
	}
	UNLOCK( psParentNode->fn_hMutex );

	if ( 0L != *pnResInode )
	{
		return ( 0 );
	}
	else
	{
		return ( -ENOENT );
	}
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int pty_can_read( FileNode_s *psNode )
{
	bool bNeedNewLine = false;
	bool bXonXoff = false;

	if ( psNode->fn_psParent == NULL || psNode->fn_psTermInfo == NULL )
	{
		return ( -EINVAL );
	}

	if ( NULL == psNode->fn_psPartner || 0 == psNode->fn_psPartner->fn_nOpenCount )
	{
		return ( -EIO );
	}

	if ( IS_SLAVE( psNode ) && psNode->fn_psTermInfo->sTermios.c_lflag & ICANON )
	{
		bNeedNewLine = true;
	}
	if ( IS_MASTER( psNode ) && psNode->fn_psTermInfo->sTermios.c_iflag & IXON )
	{
		bXonXoff = true;
	}

	if ( 0 == psNode->fn_nSize || ( bNeedNewLine && 0 == psNode->fn_nNewLineCount ) || ( bXonXoff && ( psNode->fn_nFlags & FLAG_XOFF ) ) )
	{
		return ( 0 );
	}
	else
	{
		return ( 1 );
	}
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int pty_can_write( FileNode_s *psNode, size_t nSize )
{
	if ( NULL == psNode->fn_psPartner || 0 == psNode->fn_psPartner->fn_nOpenCount )
	{
		return ( -EIO );
	}

	if ( nSize > ( PTY_BUFFER_SIZE - psNode->fn_nSize ) )
	{
		return ( 0 );
	}
	else
	{
		return ( 1 );
	}
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int pty_do_read( FileNode_s *psNode, char *pzBuf, size_t nLen, int nFlags )
{
	bool bNeedNewLine = false;
	bool bXonXoff = false;
	int i;
	int nError;

	if ( psNode->fn_psPartner == NULL || psNode->fn_psTermInfo == NULL )
	{
		return ( -EINVAL );
	}
	if ( 0 == nLen )
	{
		return ( 0 );
	}

	if ( NULL == psNode->fn_pzBuffer )
	{
		return ( -EINVAL );
	}

	LOCK( psNode->fn_hMutex );

	if ( IS_SLAVE( psNode ) && psNode->fn_psTermInfo->sTermios.c_lflag & ICANON )
	{
		bNeedNewLine = true;
	}
	if ( IS_MASTER( psNode ) && psNode->fn_psTermInfo->sTermios.c_iflag & IXON )
	{
		bXonXoff = true;
	}
	while ( 0 == psNode->fn_nSize || ( bNeedNewLine && 0 == psNode->fn_nNewLineCount ) || ( bXonXoff && ( psNode->fn_nFlags & FLAG_XOFF ) ) )
	{
		if ( NULL == psNode->fn_psPartner || 0 == psNode->fn_psPartner->fn_nOpenCount )
		{
			nError = -EIO;
			goto error;
		}
		if ( nFlags & O_NONBLOCK )
		{
			nError = -EWOULDBLOCK;
			goto error;
		}
		nError = unlock_and_suspend( psNode->fn_hReadQueue, psNode->fn_hMutex );
		LOCK( psNode->fn_hMutex );
		if ( nError < 0 )
		{
			goto error;
		}
	}
	if ( psNode->fn_bPacketMode )
	{
		if ( nLen > 0 )
		{
			pzBuf[0] = TIOCPKT_DATA;
			pzBuf++;
			nLen--;
		}
	}
	nError = min( nLen, psNode->fn_nSize );
	for ( i = 0; i < nError; ++i )
	{
		struct termios *psTermios = &psNode->fn_psTermInfo->sTermios;

		if ( bNeedNewLine && psNode->fn_pzBuffer[psNode->fn_nReadPos] ==
		     psTermios->c_cc[ VEOF ] )
		{
			psNode->fn_nNewLineCount--;
			psNode->fn_nSize--;
			psNode->fn_nReadPos = ( psNode->fn_nReadPos + 1 ) & ( PTY_BUFFER_SIZE - 1 );
			nError = i;
			break;
		}
		if ( psNode->fn_pzBuffer[psNode->fn_nReadPos] == '\n' )
		{
			psNode->fn_nNewLineCount--;
		}
		pzBuf[i] = psNode->fn_pzBuffer[psNode->fn_nReadPos];
		psNode->fn_nReadPos = ( psNode->fn_nReadPos + 1 ) & ( PTY_BUFFER_SIZE - 1 );
	}
	psNode->fn_nSize -= nError;

	wakeup_sem( psNode->fn_hWriteQueue, false );
	pty_notify_write_select( psNode );

	if ( psNode->fn_bPacketMode )
	{
		nError++;	// Count the extra packet byte
	}
      error:
	UNLOCK( psNode->fn_hMutex );
	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int pty_read( void *pVolume, void *pNode, void *pCookie, off_t nPos, void *pBuf, size_t nLen )
{
	PtyFileCookie_s *psCookie = pCookie;
	FileNode_s *psNode = pNode;
	int nError;

	if ( NULL == psNode->fn_psParent )
	{
		return ( -EINVAL );
	}
	nError = pty_do_read( psNode, pBuf, nLen, psCookie->nFlags );
	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int pty_do_write( FileNode_s *psNode, const char *pzBuf, size_t nLen, int nFlags )
{
	int nBytesWritten = 0;
	int nBytesToWrite = nLen;
	bool bNeedNewLine = false;
	int nError;

	if ( psNode->fn_psParent == NULL )
	{
		return ( -EINVAL );
	}

	if ( IS_SLAVE( psNode ) )
	{
		if ( NULL == psNode->fn_psTermInfo )
		{
			panic( "pty_do_write() PTY slave node has no terminfo struct!\n" );
			return ( -EINVAL );
		}
		if ( psNode->fn_psTermInfo->sTermios.c_lflag & ICANON )
		{
			bNeedNewLine = true;
		}
	}

	LOCK( psNode->fn_hMutex );
	while ( nBytesToWrite > 0 )
	{
		int nCurLen;

		while ( psNode->fn_nSize == PTY_BUFFER_SIZE )
		{
			// check for closed partner before blocking
			if ( NULL == psNode->fn_psPartner || 0 == psNode->fn_nOpenCount )
			{
				// return an error when writing to closed master
				if ( IS_MASTER( psNode ) )
				{
					nError = -EIO;
					goto error;
				}
			}
			if ( nFlags & O_NONBLOCK )
			{
				nError = -EWOULDBLOCK;
				goto error;
			}
			nError = unlock_and_suspend( psNode->fn_hWriteQueue, psNode->fn_hMutex );
			LOCK( psNode->fn_hMutex );
			if ( nError < 0 )
			{
				goto error;
			}
		}

		// check for closed partner before writing
		if ( NULL == psNode->fn_psPartner || 0 == psNode->fn_nOpenCount )
		{
			// return an error when writing to closed master
			if ( IS_MASTER( psNode ) )
			{
				nError = -EIO;
				goto error;
			}
		}

		nCurLen = PTY_BUFFER_SIZE - psNode->fn_nSize;
		if ( nCurLen > nBytesToWrite )
		{
			nCurLen = nBytesToWrite;
		}
		psNode->fn_nSize += nCurLen;
		nBytesToWrite -= nCurLen;

		struct termios *psTermios = &psNode->fn_psTermInfo->sTermios;
		while ( nCurLen > 0 )
		{
			// EOF acts like newline for purposes of waking up
			if ( pzBuf[nBytesWritten] == '\n' ||
			     ( ICanon && pzBuf[nBytesWritten] == psTermios->c_cc[ VEOF ] ) )
			{
				psNode->fn_nNewLineCount++;
			}
			psNode->fn_pzBuffer[psNode->fn_nWritePos++] = pzBuf[nBytesWritten++];
			psNode->fn_nWritePos &= ( PTY_BUFFER_SIZE - 1 );
			nCurLen--;
		}
		if ( false == bNeedNewLine || psNode->fn_nNewLineCount > 0 )
		{
			wakeup_sem( psNode->fn_hReadQueue, false );
			pty_notify_read_select( psNode );
		}
		else
		{
			break;
		}
	}
	UNLOCK( psNode->fn_hMutex );
	return ( nBytesWritten );
      error:
	UNLOCK( psNode->fn_hMutex );
	return ( ( nBytesWritten == 0 ) ? nError : nBytesWritten );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int pty_write_to_master( FileNode_s *psMaster, FileNode_s *psSlave, const char *pzBuf, size_t nLen, int nFlags, bool bEchoMode )
{
	struct termios *psTermios = &psMaster->fn_psTermInfo->sTermios;
	bool bNlToCrNl = false;
	bool bCrToNl = false;
	bool bNoCrAtCol0 = false;	/* Dont output CR at col 0      */
	bool bNoCr = false;		/* Dont output CR       */
	int nBytesWritten = 0;
	int nError;
	int i;

	if ( psTermios->c_oflag & OPOST )
	{
		if ( psTermios->c_oflag & ONLCR )
		{
			bNlToCrNl = true;
		}
		if ( psTermios->c_oflag & OCRNL )
		{
			bCrToNl = true;
		}
		if ( psTermios->c_oflag & ONOCR )
		{
			bNoCrAtCol0 = true;
		}
		if ( psTermios->c_oflag & ONLRET )
		{
			bNoCr = true;
		}
	}
	for ( i = 0; i < nLen; ++i )
	{
		switch ( pzBuf[i] )
		{
		case '\n':
			if ( bNlToCrNl )
			{
				nError = pty_do_write( psMaster, "\r\n", 2, nFlags );
				if ( nError > 0 )
				{
					nError = 1;
				}
			}
			else
			{
				nError = pty_do_write( psMaster, "\n", 1, nFlags );
			}
			psMaster->fn_nCsrPos = 0;
			break;
		case '\r':
			nError = 1;
			if ( bNoCr == false /* && !( bNoCrAtCol0 && 0 == psMaster->fn_nCsrPos ) */ )
			{
				nError = pty_do_write( psMaster, ( bCrToNl ) ? "\n" : "\r", 1, nFlags );
				psMaster->fn_nCsrPos = 0;
			}
			break;
		case 0x08:	/* BACKSPACE */
			if ( bEchoMode && ( psTermios->c_lflag & ICANON ) )
			{
				nError = pty_do_write( psMaster, "\x08 \x08", 3, nFlags );
				if ( nError > 0 )
				{
					nError = 1;
				}
			}
			else
			{
				nError = pty_do_write( psMaster, &pzBuf[i], 1, nFlags );
			}
			if ( psMaster->fn_nCsrPos > 0 )
			{
				psMaster->fn_nCsrPos--;
			}
			break;
		case 0x09:	/* TAB */
			nError = pty_do_write( psMaster, &pzBuf[i], 1, nFlags );
			int nSavedIndex = ( psMaster->fn_nCsrPos / TAB_SIZE );
			if ( nSavedIndex < NUM_SAVED_TABS )
			{
				psMaster->fn_nCsrPosBeforeTab[ nSavedIndex ] = psMaster->fn_nCsrPos;
			}
			psMaster->fn_nCsrPos = ( psMaster->fn_nCsrPos + TAB_SIZE ) & ~( TAB_SIZE - 1 );
			break;
		default:
			nError = pty_do_write( psMaster, &pzBuf[i], 1, nFlags );
			if ( pzBuf[i] >= ' ' && pzBuf[i] != '\x7f' )
			{
				psMaster->fn_nCsrPos++;
			}
			break;
		}
		kassertw( nError <= 1 );
		if ( nError < 1 )
		{
			if ( nBytesWritten > 0 )
			{
				return ( nBytesWritten );
			}
			else
			{
				return ( nError );
			}
		}
		else
		{
			nBytesWritten++;
		}
	}
	return ( nBytesWritten );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static void pty_erase_one_char( FileNode_s *psMaster, FileNode_s *psSlave, char nChar, int nNewPos )
{
	int nNumErased = 0;
	struct termios *psTermios = &psMaster->fn_psTermInfo->sTermios;

	psSlave->fn_nSize--;
	psSlave->fn_nWritePos = nNewPos;

	if ( nChar == '\t' )
	{
		int nSavedIndex = ( psMaster->fn_nCsrPos / TAB_SIZE ) - 1;
		if ( nSavedIndex >= 0 && nSavedIndex < NUM_SAVED_TABS )
		{
			nNumErased = psMaster->fn_nCsrPos -
				psMaster->fn_nCsrPosBeforeTab[ nSavedIndex ];
			kassertw( nNumErased > 0 );
			kassertw( nNumErased <= TAB_SIZE );
			if ( nNumErased < 0 || nNumErased > TAB_SIZE )
				nNumErased = 0;
		}
	}
	else if ( nChar < ' ' || nChar == '\x7f' )
	{
		if ( EchoCtl )
			nNumErased = 2;
	}
	else
	{
		nNumErased = 1;
	}
	if ( Echo && nNumErased )
	{
		for ( ; nNumErased; --nNumErased )
			pty_write_to_master( psMaster, psSlave, "\x08", 1, O_NONBLOCK, true );
	}
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static void pty_echo_one_char( FileNode_s *psMaster, FileNode_s *psSlave, char nChar )
{
	struct termios *psTermios = &psMaster->fn_psTermInfo->sTermios;

	if ( Echo && !( IXonXoff && psSlave->fn_nFlags & FLAG_XOFF ) )
	{
		if ( nChar == '\r' && ICrToNl )
		{
			nChar = '\n';
		}
		if ( EchoCtl && ( nChar < ' ' || nChar == '\x7f' ) && nChar != '\n' && nChar != '\t' )
		{
			char buf[2];
			buf[0] = '^';
			buf[1] = ( nChar == '\x7f' ) ? '?' : ( nChar + '@' );
			pty_write_to_master( psMaster, psSlave, buf, 2, O_NONBLOCK, true );
		}
		else
		{
			pty_write_to_master( psMaster, psSlave, &nChar, 1, O_NONBLOCK, true );
		}
	}
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int pty_write_to_slave( FileNode_s *psMaster, FileNode_s *psSlave, const char *pzBuf, size_t nLen, int nFlags )
{
	struct termios *psTermios = &psMaster->fn_psTermInfo->sTermios;
	int nBytesWritten = 0;
	int nError;
	int i;

	for ( i = 0; i < nLen; ++i )
	{
		char nChar = ( IUcToLc ) ? tolower( pzBuf[i] ) : pzBuf[i];
		if ( nChar == '\n' )
		{
			nError = pty_do_write( psSlave, ( INlToCr ) ? "\r" : "\n", 1, nFlags );
			if ( Echo || EchoNl )
			{
				pty_write_to_master( psMaster, psSlave, "\n", 1, O_NONBLOCK, true );
			}
			goto noecho;
		}
		else if ( nChar == '\r' )
		{
			if ( !IIgnoreCr )
			{
				nError = pty_do_write( psSlave, ( ICrToNl ) ? "\n" : "\r", 1, nFlags );
			}
			else
			{
				nError = 1;
			}
			goto echo;
		}
		else if ( psSlave->fn_nFlags & FLAG_LNEXT )
		{
			psSlave->fn_nFlags &= ~FLAG_LNEXT;
			goto write_and_echo;
		}
		else if ( nChar == '\0' )
		{
			goto write_and_echo;
		}
		else if ( ISignals && nChar == psTermios->c_cc[ VINTR ] )
		{
			if ( psSlave->fn_psTermInfo->hPGroupID != -1 )
			{
				//printk( "send SIGINT to %x\n", psSlave->fn_psTermInfo->hPGroupID );
				sys_kill( -psSlave->fn_psTermInfo->hPGroupID, SIGINT );
			}
			nError = 1;
			goto noecho;
		}
		else if ( ISignals && nChar == psTermios->c_cc[ VQUIT ] )
		{
			if ( psSlave->fn_psTermInfo->hPGroupID != -1 )
			{
				//printk( "send SIGQUIT to %x\n", psSlave->fn_psTermInfo->hPGroupID );
				sys_kill( -psSlave->fn_psTermInfo->hPGroupID, SIGQUIT );
			}
			nError = 1;
			goto noecho;
		}
		else if ( ICanon && nChar == psTermios->c_cc[ VERASE ] )
		{
			int nNewPos;
			char nChar;

			LOCK( psSlave->fn_hMutex );
			nNewPos = ( psSlave->fn_nWritePos + PTY_BUFFER_SIZE - 1 ) & ( PTY_BUFFER_SIZE - 1 );
			nChar = psSlave->fn_pzBuffer[nNewPos];
			if ( psSlave->fn_nSize > 0 && nChar != '\n' )
			{
				pty_erase_one_char( psMaster, psSlave, nChar, nNewPos );
			}
			nError = 1;
			wakeup_sem( psSlave->fn_hWriteQueue, false );
			pty_notify_write_select( psSlave );
			UNLOCK( psSlave->fn_hMutex );
			goto noecho;
		}
		else if ( ICanon && nChar == psTermios->c_cc[ VKILL ] )
		{
			int nNewPos;
			char nChar;

			LOCK( psSlave->fn_hMutex );
			nNewPos = ( psSlave->fn_nWritePos + PTY_BUFFER_SIZE - 1 ) & ( PTY_BUFFER_SIZE - 1 );
			nChar = psSlave->fn_pzBuffer[nNewPos];
			while ( psSlave->fn_nSize > 0 && nChar != '\n' )
			{
				pty_erase_one_char( psMaster, psSlave, nChar, nNewPos );
				nNewPos = ( nNewPos + PTY_BUFFER_SIZE - 1 ) & ( PTY_BUFFER_SIZE - 1 );
				nChar = psSlave->fn_pzBuffer[nNewPos];
			}
			nError = 1;
			wakeup_sem( psSlave->fn_hWriteQueue, false );
			pty_notify_write_select( psSlave );
			UNLOCK( psSlave->fn_hMutex );
			goto noecho;
		}
		else if ( ICanon && nChar == psTermios->c_cc[ VEOF ] )
		{
			nError = pty_do_write( psSlave, &nChar, 1, nFlags );
			goto noecho;
		}
		else if ( IXonXoff && nChar == psTermios->c_cc[ VSTART ] )
		{
			psMaster->fn_nFlags &= ~FLAG_XOFF;
			wakeup_sem( psMaster->fn_hReadQueue, false );
			pty_notify_read_select( psMaster );
			nError = 1;
			goto noecho;
		}
		else if ( IXonXoff && nChar == psTermios->c_cc[ VSTOP ] )
		{
			psMaster->fn_nFlags |= FLAG_XOFF;
			nError = 1;
			goto noecho;
		}
		else if ( ISignals && nChar == psTermios->c_cc[ VSUSP ] )
		{
			if ( psSlave->fn_psTermInfo->hPGroupID != -1 )
			{
				//printk( "send SIGSTOP to %x\n", psSlave->fn_psTermInfo->hPGroupID );
				sys_kill( -psSlave->fn_psTermInfo->hPGroupID, SIGSTOP );
			}
			nError = 1;
			goto noecho;
		}
		else if ( ICanon && nChar == psTermios->c_cc[ VREPRINT ] )
		{
			// first echo the redraw character
			pty_echo_one_char( psMaster, psSlave, nChar );
			// then print a newline
			pty_echo_one_char( psMaster, psSlave, '\n' );
			// now find the position to start printing from
			LOCK( psSlave->fn_hMutex );
			int nPos = ( psSlave->fn_nWritePos + PTY_BUFFER_SIZE - 1 ) & ( PTY_BUFFER_SIZE - 1 );
			int nSize = psSlave->fn_nSize;
			while ( nSize > 0 && psSlave->fn_pzBuffer[nPos] != '\n' )
			{
				nPos = ( nPos + PTY_BUFFER_SIZE - 1 ) & ( PTY_BUFFER_SIZE - 1 );
				nSize--;
			}
			// skip forward to first character on the line
			nPos = ( nPos + 1 ) & ( PTY_BUFFER_SIZE - 1 );
			// finally, echo each character in the buffer
			while ( nPos != psSlave->fn_nWritePos )
			{
				pty_echo_one_char( psMaster, psSlave, psSlave->fn_pzBuffer[nPos] );
				nPos = ( nPos + 1 ) & ( PTY_BUFFER_SIZE - 1 );
			}
			nError = 1;
			UNLOCK( psSlave->fn_hMutex );
			goto noecho;
		}
		else if ( ICanon && nChar == psTermios->c_cc[ VWERASE ] )
		{
			int nNewPos;
			char nChar;

			LOCK( psSlave->fn_hMutex );
			nNewPos = ( psSlave->fn_nWritePos + PTY_BUFFER_SIZE - 1 ) & ( PTY_BUFFER_SIZE - 1 );
			nChar = psSlave->fn_pzBuffer[nNewPos];
			// first delete any whitespace after the word
			while ( psSlave->fn_nSize > 0 && ( nChar == ' ' || nChar == '\t' ) )
			{
				pty_erase_one_char( psMaster, psSlave, nChar, nNewPos );
				nNewPos = ( nNewPos + PTY_BUFFER_SIZE - 1 ) & ( PTY_BUFFER_SIZE - 1 );
				nChar = psSlave->fn_pzBuffer[nNewPos];
			}
			// now delete the word
			while ( psSlave->fn_nSize > 0 && nChar != '\n' && nChar != ' ' && nChar != '\t' )
			{
				pty_erase_one_char( psMaster, psSlave, nChar, nNewPos );
				nNewPos = ( nNewPos + PTY_BUFFER_SIZE - 1 ) & ( PTY_BUFFER_SIZE - 1 );
				nChar = psSlave->fn_pzBuffer[nNewPos];
			}
			nError = 1;
			wakeup_sem( psSlave->fn_hWriteQueue, false );
			pty_notify_write_select( psSlave );
			UNLOCK( psSlave->fn_hMutex );
			goto noecho;
		}
		else if ( ICanon && nChar == psTermios->c_cc[ VLNEXT ] )
		{
			psSlave->fn_nFlags |= FLAG_LNEXT;
			pty_write_to_master( psMaster, psSlave, "^\x08", 2, O_NONBLOCK, false );
			nError = 1;
			goto noecho;
		}

	write_and_echo:
		nError = pty_do_write( psSlave, &nChar, 1, nFlags );

	echo:
		pty_echo_one_char( psMaster, psSlave, nChar );

	noecho:

		kassertw( nError <= 1 );
		if ( nError < 1 )
		{
			if ( nBytesWritten > 0 )
			{
				return ( nBytesWritten );
			}
			else
			{
				return ( nError );
			}
		}
		else
		{
			nBytesWritten++;
		}
	}
	return ( nBytesWritten );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int pty_write( void *pVolume, void *pNode, void *pCookie, off_t nPos, const void *pBuf, size_t nLen )
{
	PtyFileCookie_s *psCookie = pCookie;
	FileNode_s *psNode = pNode;
	int nBytesWritten = 0;
	int nError;

	if ( NULL == psNode )
	{
		return ( -EINVAL );
	}

	nError = lock_semaphore( psNode->fn_hWriteMutex, 0, INFINITE_TIMEOUT );

	if ( nError < 0 )
	{
		printk( "pty_write() failed to lock node for writing Err = %d\n", nError );
		return ( nError );
	}

	switch ( psNode->fn_psParent->fn_nInodeNum )
	{
	case PTY_SLAVE:
		nBytesWritten = pty_write_to_master( psNode->fn_psPartner, psNode, pBuf, nLen, psCookie->nFlags, false );
		break;
	case PTY_MASTER:
		nBytesWritten = pty_write_to_slave( psNode, psNode->fn_psPartner, pBuf, nLen, psCookie->nFlags );
		break;
	default:
		nBytesWritten = -EINVAL;
		break;
	}
	unlock_semaphore( psNode->fn_hWriteMutex );
	return ( nBytesWritten );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int pty_read_dir( void *pVolume, void *pNode, void *pCookie, int nPos, struct kernel_dirent *psFileInfo, size_t nBufSize )
{
	FileNode_s *psParentNode = pNode;
	FileNode_s *psNode;
	int nCurPos = nPos;

	LOCK( psParentNode->fn_hMutex );
	for ( psNode = psParentNode->fn_psFirstChild; NULL != psNode; psNode = psNode->fn_psNextSibling )
	{
		if ( nCurPos == 0 )
		{
			strcpy( psFileInfo->d_name, psNode->fn_zName );
			psFileInfo->d_namlen = strlen( psFileInfo->d_name );
			psFileInfo->d_ino = psNode->fn_nInodeNum;
			UNLOCK( psParentNode->fn_hMutex );
			return ( 1 );
		}
		nCurPos--;
	}
	UNLOCK( psParentNode->fn_hMutex );
	return ( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int pty_rstat( void *pVolume, void *pNode, struct stat *psStat )
{
	PtyVolume_s *psVolume = pVolume;
	FileNode_s *psNode = pNode;

	psStat->st_dev = psVolume->nDevNum;
	psStat->st_ino = psNode->fn_nInodeNum;
	psStat->st_size = psNode->fn_nSize;
	psStat->st_mode = psNode->fn_nMode;
	psStat->st_atime = psNode->fn_nATime;
	psStat->st_mtime = psNode->fn_nMTime;
	psStat->st_ctime = psNode->fn_nCTime;
	psStat->st_uid = psNode->fn_nUID;
	psStat->st_gid = psNode->fn_nGID;

	return ( 0 );
}

static int pty_wstat( void *pVolume, void *pNode, const struct stat *psStat, uint32 nMask )
{
	FileNode_s *psNode = pNode;
	int nError = 0;

	LOCK( psNode->fn_hMutex );
	if ( nMask & WSTAT_MODE )
	{
		psNode->fn_nMode = ( psNode->fn_nMode & S_IFMT ) | ( psStat->st_mode & ~S_IFMT );
	}

	if ( nMask & WSTAT_ATIME )
	{
		psNode->fn_nATime = psStat->st_atime;
	}
	if ( nMask & WSTAT_MTIME )
	{
		psNode->fn_nMTime = psStat->st_mtime;
	}
	if ( nMask & WSTAT_UID )
	{
		psNode->fn_nUID = psStat->st_uid;
	}
	if ( nMask & WSTAT_GID )
	{
		psNode->fn_nGID = psStat->st_gid;
	}
	UNLOCK( psNode->fn_hMutex );
	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int pty_open( void *pVolData, void *pNode, int nMode, void **ppCookie )
{
	PtyVolume_s *psVolume = pVolData;
	FileNode_s *psNode = pNode;
	PtyFileCookie_s *psCookie;

	if ( NULL != psNode->fn_psParent && PTY_MASTER == psNode->fn_psParent->fn_nInodeNum )
	{
		return ( -ENOENT );
	}

	psCookie = kmalloc( sizeof( PtyFileCookie_s ), MEMF_KERNEL | MEMF_CLEAR );

	if ( NULL == psCookie )
	{
		return ( -ENOMEM );
	}

	*ppCookie = psCookie;
	psCookie->nFlags = nMode;

	psNode->fn_nOpenCount++;

	if ( IS_SLAVE( psNode ) == false )
	{
		return ( 0 );
	}
	if ( NULL == psNode->fn_psTermInfo )
	{
		panic( "pty_open() PTY slave node has no terminfo struct!\n" );
		return ( -EINVAL );
	}
	LOCK( psNode->fn_hMutex );
	if ( psNode->fn_psTermInfo->nSessionID == -1 )
	{
		Process_s *psProc = CURRENT_PROC;

		LOCK( g_hCTTYMutex );
		if ( ( nMode & O_NOCTTY ) == 0 && psProc->pr_bIsGroupLeader && psProc->pr_psIoContext->ic_psCtrlTTY == NULL )
		{
			psProc->pr_psIoContext->ic_psCtrlTTY = get_inode( psVolume->nDevNum, psNode->fn_nInodeNum, false );
			psProc->pr_nOldTTYPgrp = 0;
			psNode->fn_psTermInfo->nSessionID = psProc->pr_nSession;
			psNode->fn_psTermInfo->hPGroupID = psProc->pr_hPGroupID;
		}
		UNLOCK( g_hCTTYMutex );
	}
	UNLOCK( psNode->fn_hMutex );
	return ( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int pty_close( void *pVolData, void *pNode, void *pCookie )
{
	FileNode_s *psNode = pNode;
	PtyFileCookie_s *psCookie = pCookie;

	kassertw( NULL != psCookie );

	LOCK( psNode->fn_hMutex );
	psNode->fn_nOpenCount--;
	if ( 0 == psNode->fn_nOpenCount )
	{
		if ( psNode->fn_psPartner != NULL )
		{
			wakeup_sem( psNode->fn_psPartner->fn_hReadQueue, true );
			wakeup_sem( psNode->fn_psPartner->fn_hWriteQueue, true );
			pty_notify_read_select( psNode->fn_psPartner );
			pty_notify_write_select( psNode->fn_psPartner );
		}
	}
	UNLOCK( psNode->fn_hMutex );

	if ( NULL != psCookie )
	{
		kfree( psCookie );
	}

	return ( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int pty_read_inode( void *pVolume, ino_t nInodeNum, void **ppNode )
{
	PtyVolume_s *psVolume = pVolume;
	FileNode_s *psNode;

	switch ( nInodeNum )
	{
	case PTY_ROOT:
		psNode = psVolume->psRootNode;
		break;
	case PTY_MASTER:
		psNode = psVolume->psMasterNode;
		break;
	case PTY_SLAVE:
		psNode = psVolume->psSlaveNode;
		break;
	default:
		psNode = ( FileNode_s * )( ( int )nInodeNum );
		if ( psNode->fn_nInodeNum != ( int )psNode )
		{
			printk( "pty_read_inode() invalid inode %Lx\n", nInodeNum );
			psNode = NULL;
		}
		break;
	}

	if ( NULL != psNode )
	{
		psNode->fn_bReleased = false;	// allow slave to be reopened
		*ppNode = psNode;
		return ( 0 );
	}
	else
	{
		*ppNode = NULL;
		return ( -EINVAL );
	}
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int pty_write_inode( void *pVolume, void *pNode )
{
	FileNode_s *psNode = pNode;


	if ( psNode->fn_psParent->fn_nInodeNum == PTY_MASTER || psNode->fn_psParent->fn_nInodeNum == PTY_SLAVE )
	{
		FileNode_s *psParent = psNode->fn_psParent;
		FileNode_s *psPartner = psNode->fn_psPartner;

		LOCK( psParent->fn_hMutex );
		if ( psNode->fn_bReleased )
		{
			UNLOCK( psParent->fn_hMutex );
			printk( "PANIC : pty_write_inode() inode %s written twice!\n", psNode->fn_zName );
			return ( 0 );
		}

		if ( psPartner->fn_bReleased )
		{
			kfree( psNode->fn_pzBuffer );
			kfree( psPartner->fn_pzBuffer );
			kfree( psNode->fn_psTermInfo );

			pty_delete_node( psNode );
			pty_delete_node( psPartner );
		}
		else
		{
			psNode->fn_bReleased = true;
		}
		UNLOCK( psParent->fn_hMutex );
	}

	return ( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int pty_create( void *pVolume, void *pParentNode, const char *pzName, int nNameLen, int nMode, int nPerms, ino_t *pnInodeNum, void **ppCookie )
{
	PtyVolume_s *psVolume = pVolume;
	FileNode_s *psParentNode = pParentNode;
	PtyFileCookie_s *psCookie;
	FileNode_s *psMasterNode;
	FileNode_s *psSlaveNode;
	int nError;

	if ( PTY_MASTER != psParentNode->fn_nInodeNum )
	{
		printk( "ERROR : Attempt to create pty node in slave directory!\n" );
		return ( -EINVAL );
	}
	LOCK( psParentNode->fn_hMutex );

	psMasterNode = pty_find_child_node( psParentNode, pzName, nNameLen );

	if ( psMasterNode != NULL )
	{
		UNLOCK( psParentNode->fn_hMutex );
		return ( -EEXIST );
	}
	psCookie = kmalloc( sizeof( PtyFileCookie_s ), MEMF_KERNEL | MEMF_CLEAR );
	if ( psCookie == NULL )
	{
		UNLOCK( psParentNode->fn_hMutex );
		return ( -ENOMEM );
	}
	psCookie->nFlags = nMode;

	psMasterNode = pty_create_node( psParentNode, pzName, nNameLen, S_IFCHR | S_IRUGO | S_IWUGO );
	if ( psMasterNode == NULL )
	{
		nError = -ENOMEM;
		goto error1;
	}
	psSlaveNode = pty_create_node( psVolume->psSlaveNode, pzName, nNameLen, S_IFCHR | S_IRUGO | S_IWUGO );
	if ( psSlaveNode == NULL )
	{
		nError = -ENOMEM;
		goto error2;
	}
	psMasterNode->fn_nUID = getfsuid();
	psMasterNode->fn_nGID = getfsgid();
	psSlaveNode->fn_nUID = psMasterNode->fn_nUID;
	psSlaveNode->fn_nGID = psMasterNode->fn_nGID;

	psMasterNode->fn_pzBuffer = kmalloc( PTY_BUFFER_SIZE, MEMF_KERNEL | MEMF_LOCKED );

	if ( psMasterNode->fn_pzBuffer == NULL )
	{
		nError = -ENOMEM;
		goto error3;
	}

	psSlaveNode->fn_pzBuffer = kmalloc( PTY_BUFFER_SIZE, MEMF_KERNEL | MEMF_LOCKED );

	if ( psSlaveNode->fn_pzBuffer == NULL )
	{
		nError = -ENOMEM;
		goto error4;
	}

	psMasterNode->fn_psTermInfo = kmalloc( sizeof( TermInfo_s ), MEMF_KERNEL | MEMF_CLEAR );
	psSlaveNode->fn_psTermInfo = psMasterNode->fn_psTermInfo;

	if ( psMasterNode->fn_psTermInfo == NULL )
	{
		nError = -ENOMEM;
		goto error5;
	}

	psMasterNode->fn_psPartner = psSlaveNode;
	psSlaveNode->fn_psPartner = psMasterNode;
	psMasterNode->fn_nOpenCount = 1;

	psMasterNode->fn_psTermInfo->sTermios.c_iflag = BRKINT | IGNPAR | ISTRIP | ICRNL | IXON;
	psMasterNode->fn_psTermInfo->sTermios.c_oflag = OPOST | ONLCR;
	psMasterNode->fn_psTermInfo->sTermios.c_cflag = CREAD | HUPCL | CLOCAL;
	psMasterNode->fn_psTermInfo->sTermios.c_lflag = ISIG | ICANON | ECHO | ECHOE | ECHOK | IEXTEN | ECHOCTL | ECHOKE;
	psMasterNode->fn_psTermInfo->sTermios.c_cc[VINTR] = '\x03';	// ^C
	psMasterNode->fn_psTermInfo->sTermios.c_cc[VQUIT] = '\x1c';	// ^\. 
	psMasterNode->fn_psTermInfo->sTermios.c_cc[VERASE] = '\x08';	// ^H
	psMasterNode->fn_psTermInfo->sTermios.c_cc[VKILL] = '\x15';	// ^U
	psMasterNode->fn_psTermInfo->sTermios.c_cc[VEOF] = '\x04';	// ^D
	psMasterNode->fn_psTermInfo->sTermios.c_cc[VSTART] = '\x11';	// ^Q
	psMasterNode->fn_psTermInfo->sTermios.c_cc[VSTOP] = '\x13';	// ^S
	psMasterNode->fn_psTermInfo->sTermios.c_cc[VSUSP] = '\x1a';	// ^Z
	psMasterNode->fn_psTermInfo->sTermios.c_cc[VREPRINT] = '\x12';	// ^R
	psMasterNode->fn_psTermInfo->sTermios.c_cc[VWERASE] = '\x17';	// ^W
	psMasterNode->fn_psTermInfo->sTermios.c_cc[VLNEXT] = '\x16';	// ^V

	psMasterNode->fn_psTermInfo->sWinSize.ws_row = 80;
	psMasterNode->fn_psTermInfo->sWinSize.ws_col = 25;
	psMasterNode->fn_psTermInfo->sWinSize.ws_xpixel = 80 * 8;
	psMasterNode->fn_psTermInfo->sWinSize.ws_ypixel = 25 * 8;

	psMasterNode->fn_psTermInfo->hPGroupID = -1;
	psMasterNode->fn_psTermInfo->nSessionID = -1;

	UNLOCK( psParentNode->fn_hMutex );

	*pnInodeNum = psMasterNode->fn_nInodeNum;
	*ppCookie = psCookie;
	return ( 0 );
      error5:
	kfree( psSlaveNode->fn_pzBuffer );
      error4:
	kfree( psMasterNode->fn_pzBuffer );
      error3:
	pty_delete_node( psSlaveNode );
      error2:
	pty_delete_node( psMasterNode );
      error1:
	kfree( psCookie );
	UNLOCK( psParentNode->fn_hMutex );
	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 *	This function is typically called only by the session leader, when
 *	it wants to disassociate itself from its controlling tty.
 *
 *	It performs the following functions:
 *		(1)  Sends a SIGHUP and SIGCONT to the foreground process group
 *		(2)  Clears the tty from being controlling the session
 *		(3)  Clears the controlling tty for all processes in the
 * 		     session group.
 *
 *	The argument bOnExit is set to true if called when a process is
 *	exiting; it is false if called by the ioctl TIOCNOTTY.
 *
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

void disassociate_ctty( bool bOnExit )
{
	Process_s *psProc = CURRENT_PROC;
	Thread_s *psTmp;
	thread_id hTmp;
	FileNode_s *psNode;
	dev_t nDevID;
	int nTTYPgrp = -1;
	int nSession = psProc->pr_nSession;
	int nFlg;

	LOCK( g_hCTTYMutex );
	if ( psProc->pr_psIoContext->ic_psCtrlTTY == NULL )
	{
		UNLOCK( g_hCTTYMutex );
		if ( psProc->pr_nOldTTYPgrp != 0 )
		{
//      printk( "disassociate_ctty() send signals to old pgroup\n" );
			sys_killpg( psProc->pr_nOldTTYPgrp, SIGHUP );
			sys_killpg( psProc->pr_nOldTTYPgrp, SIGCONT );
		}
		return;
	}
//  printk( "disassociate_ctty from tty %08x\n", psProc->pr_nCtrlTTY );

	nDevID = psProc->pr_psIoContext->ic_psCtrlTTY->i_psVolume->v_nDevNum;
	if ( get_vnode( nDevID, psProc->pr_psIoContext->ic_psCtrlTTY->i_nInode, ( void ** )&psNode ) < 0 )
	{
		UNLOCK( g_hCTTYMutex );
		printk( "Error: disassociate_ctty() failed to get inode\n" );
		return;
	}
	nTTYPgrp = psNode->fn_psTermInfo->hPGroupID;

	if ( nTTYPgrp > 0 )
	{
//    printk( "disassociate_ctty() send signals to current pgroup\n" );
		sys_killpg( nTTYPgrp, SIGHUP );
		if ( bOnExit == false )
		{
			sys_killpg( nTTYPgrp, SIGCONT );
		}
	}

	psProc->pr_nOldTTYPgrp = 0;
	psNode->fn_psTermInfo->nSessionID = 0;
	psNode->fn_psTermInfo->hPGroupID = -1;

	nFlg = cli();
	sched_lock();
	hTmp = get_next_thread( 0 );
	sched_unlock();
	put_cpu_flags( nFlg );
	while ( hTmp != -1 )
	{
		Inode_s *psPTY = NULL;

		nFlg = cli();
		sched_lock();
		psTmp = get_thread_by_handle( hTmp );
		hTmp = get_next_thread( hTmp );
		if ( psTmp != NULL && psTmp->tr_psProcess->pr_nSession == nSession && psTmp->tr_psProcess->pr_psIoContext != NULL )
		{
			psPTY = psTmp->tr_psProcess->pr_psIoContext->ic_psCtrlTTY;
			psTmp->tr_psProcess->pr_psIoContext->ic_psCtrlTTY = NULL;
		}
		sched_unlock();
		put_cpu_flags( nFlg );

		if ( psPTY != NULL )
		{
			UNLOCK( g_hCTTYMutex );
			put_inode( psPTY );
			LOCK( g_hCTTYMutex );
		}
	}
	UNLOCK( g_hCTTYMutex );

	put_vnode( nDevID, psNode->fn_nInodeNum );
}

void clear_ctty( void )
{
	Process_s *psThisProc = CURRENT_PROC;

	LOCK( g_hCTTYMutex );

	if ( psThisProc->pr_psIoContext->ic_psCtrlTTY != NULL )
	{
		put_inode( psThisProc->pr_psIoContext->ic_psCtrlTTY );
		psThisProc->pr_psIoContext->ic_psCtrlTTY = NULL;
	}
	UNLOCK( g_hCTTYMutex );
}

void clone_ctty( IoContext_s * psDst, IoContext_s * psSrc )
{
	if( g_hCTTYMutex >= 0 )
		LOCK( g_hCTTYMutex );
	psDst->ic_psCtrlTTY = psSrc->ic_psCtrlTTY;
	if ( psDst->ic_psCtrlTTY != NULL )
	{
		atomic_inc( &psDst->ic_psCtrlTTY->i_nCount );
	}
	if( g_hCTTYMutex >= 0 )
		UNLOCK( g_hCTTYMutex );
}

Inode_s *get_ctty( void )
{
	Process_s *psThisProc = CURRENT_PROC;
	Inode_s *psInode;

	LOCK( g_hCTTYMutex );
	psInode = psThisProc->pr_psIoContext->ic_psCtrlTTY;
	if ( psInode != NULL )
	{
		atomic_inc( &psInode->i_nCount );
	}
	UNLOCK( g_hCTTYMutex );
	return ( psInode );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int pty_ioctl( void *pVolData, void *pNode, void *pCookie, int nCmd, void *pBuf, bool bFromKernel )
{
	FileNode_s *psNode = pNode;
	PtyFileCookie_s *psCookie = pCookie;
	int nError = 0;

	LOCK( psNode->fn_hMutex );

	if ( NULL == psNode->fn_psTermInfo )
	{
		UNLOCK( psNode->fn_hMutex );
		return ( -EINVAL );
	}

	switch ( nCmd )
	{
	case TCSETA:
	case TCSETAW:
	case TCSETAF:
		{
			if ( bFromKernel )
			{
				psNode->fn_psTermInfo->sTermios = *( ( struct termios * )pBuf );
			}
			else
			{
				struct termios sTermios;

				nError = memcpy_from_user( &sTermios, pBuf, sizeof( sTermios ) );
				if ( nError >= 0 )
				{
					psNode->fn_psTermInfo->sTermios = sTermios;
				}
			}
			break;
		}
	case TCGETA:
		{
			struct termios *psTermios = pBuf;

			if ( bFromKernel )
			{
				*psTermios = psNode->fn_psTermInfo->sTermios;
			}
			else
			{
				nError = memcpy_to_user( psTermios, &psNode->fn_psTermInfo->sTermios, sizeof( psNode->fn_psTermInfo->sTermios ) );
			}
			break;
		}

	case TIOCSWINSZ:
		{

			if ( bFromKernel )
			{
				psNode->fn_psTermInfo->sWinSize = *( ( struct winsize * )pBuf );
			}
			else
			{
				struct winsize sSize;

				nError = memcpy_from_user( &sSize, pBuf, sizeof( psNode->fn_psTermInfo->sWinSize ) );
				if ( nError >= 0 )
				{
					psNode->fn_psTermInfo->sWinSize = sSize;
				}
			}
			if ( nError >= 0 && IS_MASTER( psNode ) && -1 != psNode->fn_psTermInfo->hPGroupID )
			{
				sys_kill( -psNode->fn_psTermInfo->hPGroupID, SIGWINCH );
			}
			break;
		}
	case TIOCGWINSZ:
		{
			struct winsize *psSize = pBuf;

			if ( bFromKernel )
			{
				*psSize = psNode->fn_psTermInfo->sWinSize;
			}
			else
			{
				nError = memcpy_to_user( psSize, &psNode->fn_psTermInfo->sWinSize, sizeof( psNode->fn_psTermInfo->sWinSize ) );
			}
			break;
		}
	case TIOCSPGRP:	// Set foreground process group
		{
			Process_s *psProc = CURRENT_PROC;
			pid_t hPGroup;

			if ( bFromKernel )
			{
				hPGroup = *( ( pid_t * )pBuf );
			}
			else
			{
				nError = memcpy_from_user( &hPGroup, pBuf, sizeof( hPGroup ) );
			}
			if ( nError >= 0 )
			{
				LOCK( g_hCTTYMutex );

				// FIXME: Check that it's the master side
				if ( psProc->pr_psIoContext->ic_psCtrlTTY == NULL || psProc->pr_psIoContext->ic_psCtrlTTY->i_nInode != psNode->fn_nInodeNum || psProc->pr_nSession != psNode->fn_psTermInfo->nSessionID )
				{
					printk( "pty_ioctl() rejecting TIOCSPGRP. ctrl-tty = %p, proc-session = %d, pty-session = %d\n", psProc->pr_psIoContext->ic_psCtrlTTY, psProc->pr_nSession, psNode->fn_psTermInfo->nSessionID );
					nError = -ENOTTY;
				}
				UNLOCK( g_hCTTYMutex );
				if ( nError >= 0 )
				{
					psNode->fn_psTermInfo->hPGroupID = hPGroup;
				}
			}
			break;
		}
	case TIOCGPGRP:	// Get foreground process group
		{
			Process_s *psProc = CURRENT_PROC;

			LOCK( g_hCTTYMutex );
			if ( psProc->pr_psIoContext->ic_psCtrlTTY == NULL || psProc->pr_psIoContext->ic_psCtrlTTY->i_nInode != psNode->fn_nInodeNum )
			{
				nError = -ENOTTY;
			}
			UNLOCK( g_hCTTYMutex );
			if ( nError >= 0 )
			{
				if ( bFromKernel )
				{
					*( ( pid_t * )pBuf ) = psNode->fn_psTermInfo->hPGroupID;
				}
				else
				{
					nError = memcpy_to_user( pBuf, &psNode->fn_psTermInfo->hPGroupID, sizeof( psNode->fn_psTermInfo->hPGroupID ) );
				}
			}
			break;
		}
	case TIOCNOTTY:
		{
			Process_s *psProc = CURRENT_PROC;

			LOCK( g_hCTTYMutex );

			if ( psProc->pr_psIoContext->ic_psCtrlTTY == NULL || psProc->pr_psIoContext->ic_psCtrlTTY->i_nInode != psNode->fn_nInodeNum )
			{
				nError = -ENOTTY;
			}
			if ( nError >= 0 )
			{
				if ( psProc->pr_bIsGroupLeader )
				{
					disassociate_ctty( false );
				}
				put_inode( psProc->pr_psIoContext->ic_psCtrlTTY );
				psProc->pr_psIoContext->ic_psCtrlTTY = NULL;
			}
			UNLOCK( g_hCTTYMutex );
			break;
		}
	case FIONBIO:
		{
			int nArg;

			if ( bFromKernel )
			{
				nArg = *( ( int * )pBuf );
			}
			else
			{
				nError = memcpy_from_user( &nArg, pBuf, sizeof( nArg ) );
			}
			if ( nError >= 0 )
			{
				if ( nArg == false )
				{
					psCookie->nFlags &= ~O_NONBLOCK;
				}
				else
				{
					psCookie->nFlags |= O_NONBLOCK;
				}
			}
			break;
		}
	case FIONREAD:
		{
			int nSize;

			if ( PTY_SLAVE == psNode->fn_psParent->fn_nInodeNum && ( psNode->fn_psTermInfo->sTermios.c_lflag & ICANON ) )
			{
				nSize = ( psNode->fn_nNewLineCount > 0 ) ? psNode->fn_nSize : 0;
			}
			else
			{
				nSize = psNode->fn_nSize;
			}
			if ( bFromKernel )
			{
				*( ( int * )pBuf ) = nSize;
			}
			else
			{
				nError = memcpy_to_user( pBuf, &nSize, sizeof( nSize ) );
			}
			break;
		}
	case TIOCPKT:
		{
			int nArg;

			if ( IS_MASTER( psNode ) == false )
			{
				nError = -ENOTTY;
				break;
			}

			if ( bFromKernel )
			{
				nArg = *( ( int * )pBuf );
			}
			else
			{
				nError = memcpy_from_user( &nArg, pBuf, sizeof( nArg ) );
			}
			if ( nError >= 0 )
			{
				if ( nArg == false )
				{
					psNode->fn_bPacketMode = false;
				}
				else
				{
					if ( psNode->fn_bPacketMode == false )
					{
						psNode->fn_bPacketMode = true;
						psNode->fn_psTermInfo->nCtrlStatus = 0;
					}
				}
			}
			break;
		}
	default:
		printk( "pty_ioctl() Unknown command %d\n", nCmd );
		nError = -EINVAL;
		break;

	}
	UNLOCK( psNode->fn_hMutex );
	return ( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int pty_set_flags( void *pVolume, void *pNode, void *pCookie, uint32 nFlags )
{
	PtyFileCookie_s *psCookie = pCookie;

	kassertw( NULL != psCookie );
	psCookie->nFlags = nFlags & O_NONBLOCK;
	return ( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int pty_isatty( void *pVolData, void *pNode )
{
	FileNode_s *psNode = pNode;

	kassertw( NULL != psNode );

	if ( psNode->fn_psParent == NULL )
	{
		return ( 0 );
	}
	switch ( psNode->fn_psParent->fn_nInodeNum )
	{
	case PTY_SLAVE:
	case PTY_MASTER:
		return ( 1 );
	default:
		return ( 0 );
	}
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static void pty_notify_read_select( FileNode_s *psNode )
{
	SelectRequest_s *psReq;

	for ( psReq = psNode->fn_psFirstReadSelReq; NULL != psReq; psReq = psReq->sr_psNext )
	{
		psReq->sr_bReady = true;
		unlock_semaphore( psReq->sr_hSema );
	}
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static void pty_notify_write_select( FileNode_s *psNode )
{
	SelectRequest_s *psReq;

	for ( psReq = psNode->fn_psFirstWriteSelReq; NULL != psReq; psReq = psReq->sr_psNext )
	{
		psReq->sr_bReady = true;
		unlock_semaphore( psReq->sr_hSema );
	}
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int pty_add_select_req( void *pVolume, void *pNode, void *pCookie, SelectRequest_s *psReq )
{
	FileNode_s *psNode = pNode;

	LOCK( psNode->fn_hMutex );

	switch ( psReq->sr_nMode )
	{
	case SELECT_READ:
		psReq->sr_psNext = psNode->fn_psFirstReadSelReq;
		psNode->fn_psFirstReadSelReq = psReq;
		if ( pty_can_read( psNode ) != 0 )
		{
			pty_notify_read_select( psNode );
		}
		break;

	case SELECT_WRITE:
		psReq->sr_psNext = psNode->fn_psFirstWriteSelReq;
		psNode->fn_psFirstWriteSelReq = psReq;
		if ( pty_can_write( psNode, 1 ) != 0 )
		{
			pty_notify_write_select( psNode );
		}
		break;

	case SELECT_EXCEPT:
		break;

	default:
		kerndbg( KERN_DEBUG, "pty_add_select_req(): unknown mode %d\n", psReq->sr_nMode );
		break;
	}
	UNLOCK( psNode->fn_hMutex );

	return ( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int pty_rem_select_req( void *pVolume, void *pNode, void *pCookie, SelectRequest_s *psReq )
{
	FileNode_s *psNode = pNode;
	SelectRequest_s **ppsTmp = NULL;

	LOCK( psNode->fn_hMutex );

	switch ( psReq->sr_nMode )
	{
	case SELECT_READ:
		ppsTmp = &psNode->fn_psFirstReadSelReq;
		break;

	case SELECT_WRITE:
		ppsTmp = &psNode->fn_psFirstWriteSelReq;
		break;

	case SELECT_EXCEPT:
		break;

	default:
		kerndbg( KERN_DEBUG, "pty_rem_select_req(): unknown mode %d\n", psReq->sr_nMode );
		break;
	}

	if ( NULL != ppsTmp )
	{
		for ( ; NULL != *ppsTmp; ppsTmp = &( ( *ppsTmp )->sr_psNext ) )
		{
			if ( *ppsTmp == psReq )
			{
				*ppsTmp = psReq->sr_psNext;
				break;
			}
		}
	}

	UNLOCK( psNode->fn_hMutex );

	return ( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static FSOperations_s g_sOperations = {
	NULL,			// op_probe
	pty_mount,		// op_mount
	NULL,			// op_unmount
	pty_read_inode,
	pty_write_inode,
	pty_lookup,		/* Lookup       */
	NULL,			/* access       */
	pty_create,		/* create       */
	NULL,			/* mkdir        */
	NULL,			/* mknod        */
	NULL,			/* symlink      */
	NULL,			/* link         */
	NULL,			/* rename       */
	NULL,			/* unlink       */
	NULL,			/* rmdir        */
	NULL,			/* readlink     */
	NULL,			/* ppendir      */
	NULL,			/* closedir     */
	NULL,			/* RewindDir    */
	pty_read_dir,		/* ReadDir      */
	pty_open,
	pty_close,
	NULL,			/* FreeCookie   */
	pty_read,		// op_read
	pty_write,		// op_write
	NULL,			// op_readv
	NULL,			// op_writev
	pty_ioctl,		/* ioctl        */
	pty_set_flags,		/* setflags     */
	pty_rstat,
	pty_wstat,		/* wstat        */
	NULL,			/* fsync        */
	NULL,			/* initialize   */
	NULL,			/* sync         */
	NULL,			/* rfsstat      */
	NULL,			/* wfsstat      */
	pty_isatty,		/* isatty       */
	pty_add_select_req,	/* add_select_req     */
	pty_rem_select_req,	/* rem_select_req     */
	NULL,			/* open_attrdir       */
	NULL,			/* close_attrdir      */
	NULL,			/* rewind_attrdir     */
	NULL,			/* read_attrdir       */
	NULL,			/* remove_attr        */
	NULL,			/* rename_attr        */
	NULL,			/* stat_attr          */
	NULL,			/* write_attr         */
	NULL,			/* read_attr          */
	NULL,			/* open_indexdir      */
	NULL,			/* close_indexdir     */
	NULL,			/* rewind_indexdir    */
	NULL,			/* read_indexdir      */
	NULL,			/* create_index       */
	NULL,			/* remove_index       */
	NULL,			/* rename_index       */
	NULL,			/* stat_index         */
	NULL,			/* get_file_blocks    */
	NULL			/* truncate           */
};

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

void init_pty( void )
{
	g_hCTTYMutex = create_semaphore( "ctty_mutex", 1, SEM_RECURSIVE );
	register_file_system( "_pty_device_", &g_sOperations, FSDRIVER_API_VERSION );
}
