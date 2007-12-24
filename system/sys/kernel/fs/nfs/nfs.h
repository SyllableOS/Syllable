#ifndef __F_NFS_H__
#define __F_NFS_H__

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

#define NFS_MAX_READ_BUF 1024

enum
{
	NFS_OPEN = 1,
	NFS_CLOSE = 2,
	NFS_TRUNC = 3,
	NFS_READ = 4,
	NFS_WRITE = 5,
	NFS_CREATE = 6,
	NFS_MKDIR = 7,
	NFS_RMDIR = 8,
	NFS_UNLINK = 9,
	NFS_READLINK = 10,
	NFS_WSTAT = 11,
	NFS_STAT = 12,
	NFS_READ_DIR = 13,
	NFS_RENAME = 14
};

typedef struct
{
	ino_t nInode;
	dev_t nDevice;
	int nHandle;
} NfsFileHandle_s;


typedef struct
{
	int nCmd;
	atomic_t nSeqNum;
} NfsPacketHdr_s;

/*****************************************************************************/

typedef struct
{
	NfsPacketHdr_s sHdr;
	int nParent;
	char zPath[256];
} NfsOpen_s;

typedef struct
{
	NfsPacketHdr_s sHdr;
	int nHandle;
} NfsOpenReply_s;

/*****************************************************************************/

typedef struct
{
	NfsPacketHdr_s sHdr;
	int nHandle;
} NfsClose_s;

typedef struct
{
	NfsPacketHdr_s sHdr;
	int nError;
} NfsCloseReply_s;

/*****************************************************************************/

typedef struct
{
	NfsPacketHdr_s sHdr;
	int nParent;
	int nPerms;
	char zName[256];
} NfsCreate_s;

typedef struct
{
	NfsPacketHdr_s sHdr;
	int nHandle;
} NfsCreateReply_s;

/*****************************************************************************/

typedef struct
{
	NfsPacketHdr_s sHdr;
	int nParent;
	int nPerms;
	char zName[256];
} NfsMkDir_s;

typedef struct
{
	NfsPacketHdr_s sHdr;
	int nError;
} NfsMkDirReply_s;

/*****************************************************************************/

typedef struct
{
	NfsPacketHdr_s sHdr;
	int nParent;
	char zName[256];
} NfsRmDir_s;

typedef struct
{
	NfsPacketHdr_s sHdr;
	int nError;
} NfsRmDirReply_s;

/*****************************************************************************/

typedef struct
{
	NfsPacketHdr_s sHdr;
	int nParent;
	char zName[256];
} NfsUnlink_s;

typedef struct
{
	NfsPacketHdr_s sHdr;
	int nError;
} NfsUnlinkReply_s;

/*****************************************************************************/

typedef struct
{
	NfsPacketHdr_s sHdr;
	int nHandle;
} NfsStat_s;

typedef struct
{
	NfsPacketHdr_s sHdr;
	int nError;
	struct stat sStat;
} NfsStatReply_s;

/*****************************************************************************/

typedef struct
{
	NfsPacketHdr_s sHdr;
	int nHandle;
	struct stat sStat;
} NfsWStat_s;

typedef struct
{
	NfsPacketHdr_s sHdr;
	int nError;
} NfsWStatReply_s;

/*****************************************************************************/

typedef struct
{
	NfsPacketHdr_s sHdr;
	int nHandle;
} NfsReadLink_s;

typedef struct
{
	NfsPacketHdr_s sHdr;
	int nError;
	char zBuffer[1024];
} NfsReadLinkReply_s;


/*****************************************************************************/

typedef struct
{
	NfsPacketHdr_s sHdr;
	int nHandle;
	int /*off_t */ nPos;
	int nSize;
} NfsRead_s;

typedef struct
{
	NfsPacketHdr_s sHdr;
	int nError;
	char anBuffer[NFS_MAX_READ_BUF];
} NfsReadReply_s;

/*****************************************************************************/

typedef struct
{
	NfsPacketHdr_s sHdr;
	int nHandle;
	int /*off_t */ nPos;
	int nSize;
	char anBuffer[NFS_MAX_READ_BUF];
} NfsWrite_s;

typedef struct
{
	NfsPacketHdr_s sHdr;
	int nError;
} NfsWriteReply_s;

/*****************************************************************************/

typedef struct
{
	NfsPacketHdr_s sHdr;
	int nHandle;
	int nPos;
} NfsReadDir_s;

typedef struct
{
	NfsPacketHdr_s sHdr;
	int nError;
	char zName[256];
} NfsReadDirReply_s;

/*****************************************************************************/

typedef struct
{
	NfsPacketHdr_s sHdr;
	int nOldDir;
	int nNewDir;

	char zOldName[256];
	char zNewName[256];
} NfsRename_s;

typedef struct
{
	NfsPacketHdr_s sHdr;
	int nError;
} NfsRenameReply_s;

/*****************************************************************************/

typedef struct _NfsPendingReq NfsPendingReq_s;

struct _NfsPendingReq
{
	NfsPendingReq_s *psNext;
	atomic_t nSeqNum;
	sem_id hSema;
	int nReplySize;
	void *pReply;
	bool bFinnished;
};

typedef struct _NfsInode NfsInode_s;

struct _NfsInode
{
	NfsInode_s *ni_psNext;
	NfsInode_s *ni_psFirstChild;
	NfsInode_s *ni_psParent;
	char ni_zName[256];
	ino_t ni_nInode;
	sem_id ni_hLock;
	int ni_nServerHandle;
	bool ni_bValid;
};

typedef struct
{
	int nv_nFsID;
	NfsInode_s *nv_psRoot;
	int nv_nRxPort;
	int nv_nTxPort;
	atomic_t nv_nCurSequence;
	sem_id nv_hMsgListSema;
	NfsPendingReq_s *nv_psFirstPending;
	int nv_nSocket;
	struct sockaddr_in nv_sSrvAddr;
} NfsVolume_s;


typedef struct _NfsDirEntry NfsDirEntry_s;

struct _NfsDirEntry
{
	NfsDirEntry_s *psNext;
	char zName[256];
};

typedef struct
{
	NfsDirEntry_s *nc_psFirstDEnt;
} NfsCookie_s;

#endif /* __F_NFS_H__ */
