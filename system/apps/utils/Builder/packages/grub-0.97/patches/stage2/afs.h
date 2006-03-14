#ifndef __F_AFS_H__
#define __F_AFS_H__

/*
 *  AFS - The native AtheOS file-system
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


#define	DIRECT_BLOCK_COUNT	12
#define	BLOCKS_PER_DI_RUN	4

typedef long long afs_off_t;

typedef signed char		int8;	    /* signed 8-bit quantity */
typedef signed short int	int16;	    /* signed 16-bit quantity */
typedef signed long	int	int32;	    /* signed 32-bit quantity */

typedef unsigned char		uint8;	  	/* unsigned 8-bit quantity */
typedef unsigned short int	uint16;	  	/* unsigned 16-bit quantity */
typedef unsigned long	int	uint32;		/* unsigned 32-bit quantity */

typedef signed long	long	int64;	    /* signed 64-bit quantity */
typedef unsigned long	long	uint64;	    /* unsigned 64-bit quantity */
typedef	long long		bigtime_t;



#define AFS_S_IFMT  00170000
#define AFS_S_IFLNK 0120000
	
#define AFS_S_IFREG  0100000
#define AFS_S_IFDIR  0040000
#define AFS_S_IFIFO  0010000
  

#define AFS_S_ISLNK(m)	(((m) & AFS_S_IFMT) == AFS_S_IFLNK)
#define AFS_S_ISREG(m)	(((m) & AFS_S_IFMT) == AFS_S_IFREG)
#define AFS_S_ISDIR(m)	(((m) & AFS_S_IFMT) == AFS_S_IFDIR)
#define AFS_S_ISFIFO(m)	(((m) & AFS_S_IFMT) == AFS_S_IFIFO)




typedef struct
{
  int32	 group;
  uint16 start;
  uint16 len;
} BlockRun_s;


typedef struct
{
    BlockRun_s	ds_asDirect[ DIRECT_BLOCK_COUNT ];
    afs_off_t	ds_nMaxDirectRange;
    BlockRun_s	ds_sIndirect;
    afs_off_t	ds_nMaxIndirectRange;
    BlockRun_s	ds_sDoubleIndirect;
    afs_off_t	ds_nMaxDoubleIndirectRange;
    afs_off_t	ds_nSize;
} DataStream_s;

#define	SUPER_BLOCK_MAGIC1	0x41465331	/* AFS1 */
#define	SUPER_BLOCK_MAGIC2	0xdd121031
#define	SUPER_BLOCK_MAGIC3	0x15b6830e

enum
{
  BO_LITTLE_ENDIAN,
  BO_BIG_ENDIAN
};

typedef struct
{
    char	as_zName[32];
    int32	as_nMagic1;
    int32	as_nByteOrder;
    uint32	as_nBlockSize;
    uint32	as_nBlockShift;
    afs_off_t	as_nNumBlocks;
    afs_off_t	as_nUsedBlocks;

    int32	as_nInodeSize;

    int32	as_nMagic2;
    int32	as_nBlockPerGroup;	// Number of blocks per allocation group (Max 65536)
    int32	as_nAllocGrpShift;	// Number of bits to shift a group number to get a byte address.
    int32	as_nAllocGroupCount;
    int32	as_nFlags;

    BlockRun_s	as_sLogBlock;
    afs_off_t	as_nLogStart;
    int		as_nValidLogBlocks;
    int		as_nLogSize;
    int32	as_nMagic3;

    BlockRun_s	as_sRootDir;		// Root dir inode.
    BlockRun_s	as_sDeletedFiles;	// Directory containing files scheduled for deletion.
    BlockRun_s	as_sIndexDir;		// Directory of index files.
    int		as_nBootLoaderSize;
    int32	as_anPad[7];
} AfsSuperBlock_s;


#define	INODE_MAGIC	0x64358428

#define	INF_USED	    0x00000001
#define	INF_ATTRIBUTES	    0x00000002
#define	INF_LOGGED	    0x00000004

#define	INF_PERMANENT_MASK  0x0000ffff

#define	INF_NO_CACHE	    0x00010000
#define	INF_WAS_WRITTEN	    0x00020000
#define	INF_NO_TRANSACTION  0x00040000
#define	INF_NOT_IN_DELME    0x00080000   /* The file is scheduled for deletion, but not moved into
					  * the "delete-me" directory. This is propably due to the
					  * disk getting full during the attempt to move it.
					  */
typedef	struct _AfsVNode AfsVNode_s;

typedef struct
{
    int32	 ai_nMagic1;
    BlockRun_s	 ai_sInodeNum;
    int32	 ai_nUID;
    int32	 ai_nGID;
    int32	 ai_nMode;
    int32	 ai_nFlags;
    int32	 ai_nLinkCount;
    bigtime_t	 ai_nCreateTime;
    bigtime_t	 ai_nModifiedTime;
    BlockRun_s	 ai_sParent;
    BlockRun_s	 ai_sAttribDir;
    uint32	 ai_nIndexType;		/* Key data-key only used for index files */

    int32	 ai_nInodeSize;
    AfsVNode_s*	 ai_psVNode;
    DataStream_s ai_sData;
    int32	 ai_anPad[4];
    int32	 ai_SmallData[1];
} AfsInode_s;


#define SD_DATA( sd ) (((char*)((sd)+1)) + (sd)->sd_nNameSize)
#define SD_SIZE( sd ) (sizeof(*(sd)) + (sd)->sd_nNameSize + (sd)->sd_nDataSize)
#define NEXT_SD( sd ) ((AfsSmallData_s*)(((char*)((sd)+1)) + (sd)->sd_nNameSize + (sd)->sd_nDataSize))


#define B_NODE_SIZE	(1024)

typedef struct _BNode BNode_s;

#define B_MAX_KEY_SIZE	256

typedef int64	bvalue_t;
#define NULL_VAL ((bvalue_t)-1)

struct _BNode
{
    bvalue_t bn_nLeft;
    bvalue_t bn_nRight;
    bvalue_t bn_nOverflow;
    int	     bn_nKeyCount;
    int	     bn_nTotKeySize;
    char     bn_anKeyData[1];
};

#define BTREE_MAGIC 0x65768995

typedef struct
{
    uint32	bt_nMagic;
    bvalue_t 	bt_nRoot;
    int	   	bt_nTreeDepth;
    bvalue_t	bt_nLastNode;
    bvalue_t	bt_nFirstFree;
} BTree_s;

#define	B_HEADER_SIZE ((int)&(((BNode_s*)0)->bn_anKeyData[0]))


static inline int16* B_KEY_INDEX_OFFSET( BNode_s* node )
{
    return( ((int16*) ((((int)(node)) + B_HEADER_SIZE + (node)->bn_nTotKeySize + 3) & ~3)) );
}

static inline const int16* B_KEY_INDEX_OFFSET_CONST( const BNode_s* node )
{
    return( ((const int16*) ((((int)(node)) + B_HEADER_SIZE + (node)->bn_nTotKeySize + 3) & ~3)) );
}

static inline bvalue_t* B_KEY_VALUE_OFFSET( BNode_s* node )
{
    return( ((bvalue_t*) (((int)B_KEY_INDEX_OFFSET( node )) + (node)->bn_nKeyCount * 2)) );
}

static inline const bvalue_t* B_KEY_VALUE_OFFSET_CONST( const BNode_s* node )
{
    return( ((const bvalue_t*) (((int)B_KEY_INDEX_OFFSET_CONST( node )) + (node)->bn_nKeyCount * 2)) );
}

#endif /* __F_AFS_H__ */
