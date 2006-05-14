#ifndef __F_AFS_H__
#define __F_AFS_H__

/*
 *  AFS - The native AtheOS file-system
 *  Copyright(C) 1999  Kurt Skauen
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

#include <posix/dirent.h>
#include <atheos/filesystem.h>

#define AFS_PARANOIA

#define	DIRECT_BLOCK_COUNT	12
#define	BLOCKS_PER_DI_RUN	4

typedef off_t afs_off_t;

typedef struct _AfsTransaction AfsTransaction_s;
typedef struct _AfsTransBuffer AfsTransBuffer_s;
typedef struct _AfsJournalBlock AfsJournalBlock_s;
typedef struct _AfsHashEnt AfsHashEnt_s;

typedef struct
{
	int32 group;
	uint16 start;
	uint16 len;
} BlockRun_s;

#define AFS_MAX_RUN_LENGTH 65535

/** Data stream of an AFS file
 * \par Description:
 * This structure is used to find the actual data in a file.  It conists of a Direct blockrun (ie, a
 * set of DIRECT_BLOCK_COUNT (12) runs of data blocks), an Indirect blockrun (ie, a set of
 * BLOCKS_PER_DI_RUN (4) blocks of pointers, each pointing to a run of data blocks) , and a Double
 * Indirect blockrun (ie, set of BLOCKS_PER_DI_RUN (4) blocks of pointer, each pointing to a run of
 * BLOCKS_PER_DI_RUN (4) blocks of pointers, each pointing to a run of data blocks).
 ****************************************************************************/
typedef struct
{
	BlockRun_s	ds_asDirect[ DIRECT_BLOCK_COUNT ];	///< Direct range
	off_t	ds_nMaxDirectRange;			///< Number of blocks in the direct range
	BlockRun_s	ds_sIndirect;				///< Indirect range
	off_t	ds_nMaxIndirectRange;			///< Number of block in direct and indirect ranges
	BlockRun_s	ds_sDoubleIndirect;			///< Double indirect range
	off_t	ds_nMaxDoubleIndirectRange;		///< Number of blocks in direct, indirect, and double indirect ranges
	off_t ds_nSize;
} DataStream_s;

#define AFS_SUPERBLOCK_SIZE	1024
#define	SUPER_BLOCK_MAGIC1	0x41465331	/* AFS1 */
#define	SUPER_BLOCK_MAGIC2	0xdd121031
#define	SUPER_BLOCK_MAGIC3	0x15b6830e

enum
{
	AFS_FLAG_NONE = 0,
	AFS_FLAG_READ_ONLY = 1
};

enum
{
	BO_LITTLE_ENDIAN,
	BO_BIG_ENDIAN
};
typedef struct
{
	char as_zName[32];
	int32 as_nMagic1;
	int32 as_nByteOrder;
	uint32 as_nBlockSize;
	uint32 as_nBlockShift;
	off_t as_nNumBlocks;
	off_t as_nUsedBlocks;

	int32 as_nInodeSize;

	int32 as_nMagic2;
	int32 as_nBlockPerGroup;	// Number of blocks per allocation group(Max 65536)
	int32 as_nAllocGrpShift;	// Number of bits to shift a group number to get a byte address.
	int32 as_nAllocGroupCount;
	int32 as_nFlags;

	BlockRun_s as_sLogBlock;
	off_t as_nLogStart;
	int as_nValidLogBlocks;
	int as_nLogSize;
	int32 as_nMagic3;

	BlockRun_s as_sRootDir;	// Root dir inode.
	BlockRun_s as_sDeletedFiles;	// Directory containing files scheduled for deletion.
	BlockRun_s as_sIndexDir;	// Directory of index files.
	int as_nBootLoaderSize;
	int32 as_anPad[7];
} AfsSuperBlock_s;


#define JOURNAL_SIZE	256

struct _AfsJournalBlock
{
	AfsJournalBlock_s *jb_psNext;
	off_t jb_nBlock;
	void *jb_pData;
	void *jb_pNewBuffer;
};


struct _AfsTransBuffer
{
	AfsTransBuffer_s *atb_psNext;
	off_t *atb_pBuffer;
	int atb_nMaxBlockCount;
	int atb_nBlockCount;
	area_id atb_hAreaID;
};

struct _AfsTransaction
{
	AfsTransaction_s *at_psNext;
	AfsTransaction_s *at_psPrev;
	AfsHashEnt_s *at_psFirstEntry;	// First entry in doubly linked list of journal hash entries
	AfsHashEnt_s *at_psLastEntry;	// Last entry in doubly linked list of journal hash entries
	off_t at_nLogStart;	// Absolute start block in the log(disk address)
	int at_nBlockCount;	// Number of blocks in the transaction including index blocks
	atomic_t at_nPendingLogBlocks;	// Blocks written to the cache but not yet flushed.
	atomic_t at_nWrittenBlocks;	// Number of blocks written to their final destinations.
	AfsTransBuffer_s *at_psFirstBuffer;	// Single linked list of 128K transaction buffers
	bool at_bWrittenToLog;	// true if the transaction is written to the log
	bool at_bWrittenToDisk;
};


struct _AfsHashEnt
{
	AfsHashEnt_s *he_psNext;
	AfsHashEnt_s *he_psNextLinear;
	AfsHashEnt_s *he_psNextInTrans;
	AfsHashEnt_s *he_psPrevInTrans;
	AfsTransaction_s *he_psTransaction;
	off_t he_nBlockNum;
	off_t he_nHashVal;
	AfsJournalBlock_s *he_psTransBuf;	// Used if the buffer is part of the current transaction
	void *he_pBuffer;	// Used if the buffer is part of an old transaction
};


typedef struct
{
	AfsHashEnt_s **ht_apsTable;
	int ht_nSize;		/* Size of ht_apsTable */
	int ht_nMask;		/* Used to mask out overflowing bits from the hash value */
	int ht_nCount;		/* Number of inserted elements */
} AfsHashTable_s;


typedef struct
{
	int av_nDevice;
	int av_nFsID;
	atomic_t av_nOpenFiles;
	AfsSuperBlock_s *av_psSuperBlock;
	sem_id av_hJournalLock;
	sem_id av_hJournalListsLock;
	off_t av_nSuperBlockPos;
	off_t av_nBitmapStart;
	off_t av_nLoggStart;
	off_t av_nLoggEnd;
	off_t as_nTransAllocatedBlocks;
	void **av_pFirstTmpBuffer;		/**< Pointer to temporary buffer freelist */
	int av_nTmpBufferCount;			/**< Number of temporary buffers in use */
	sem_id av_hBlockListMutex;
	sem_id av_hIndexDirMutex;

	int av_nTransactionStarted;
	int av_nJournalSize;
	AfsJournalBlock_s *av_psFirstLogBlock;
	AfsTransaction_s *av_psFirstTrans;
	AfsTransaction_s *av_psLastTrans;
	AfsHashTable_s av_sLogHashTable;
	int av_nOldBlockCount;	// Number of blocks merged into the current super-transaction
	int av_nNewBlockCount;	// Number of blocks touched since last afs_merge_transactions() call
	int av_nPendingLogBlocks;	// Total number of blocks consuming log space
	//(not including blocks in the av_psFirstLogBlock list)
	bigtime_t av_nLastJournalAccesTime;
	volatile bool av_bRunJournalFlusher;
	int av_nFlags;		// Mount flags
} AfsVolume_s;


#define	INODE_MAGIC	    0x64358428

#define	INF_USED	    0x00000001
#define	INF_ATTRIBUTES	    0x00000002
#define	INF_LOGGED	    0x00000004

#define	INF_PERMANENT_MASK  0x0000ffff

#define	INF_NO_CACHE	    0x00010000
#define	INF_WAS_WRITTEN	    0x00020000
#define	INF_NO_TRANSACTION  0x00040000
#define	INF_NOT_IN_DELME    0x00080000	/* The file is scheduled for deletion, but not moved into
					 * the "delete-me" directory. This is probably due to the
					 * disk getting full during the attempt to move it.
					 */
#define INF_STAT_CHANGED    0x00100000

typedef struct _AfsVNode AfsVNode_s;

typedef struct
{
	int32 ai_nMagic1;
	BlockRun_s ai_sInodeNum;
	int32 ai_nUID;
	int32 ai_nGID;
	int32 ai_nMode;
	int32 ai_nFlags;
	atomic_t ai_nLinkCount;
	bigtime_t ai_nCreateTime;
	bigtime_t ai_nModifiedTime;
	BlockRun_s ai_sParent;
	BlockRun_s ai_sAttribDir;
	uint32 ai_nIndexType;	/* Key data-key only used for index files */

	int32 ai_nInodeSize;
	AfsVNode_s *ai_psVNode;
	DataStream_s ai_sData;
	int32 ai_anPad[4];
	int32 ai_SmallData[1];
} AfsInode_s;

#include "btree.h"


typedef struct
{
	BIterator_s aa_sBIterator;
	int aa_nPos;
} AfsAttrIterator_s;

typedef struct
{
	uint32 sd_nType;
	uint16 sd_nNameSize;
	uint16 sd_nDataSize;
} AfsSmallData_s;

#define SD_DATA( sd )(((char*)((sd)+1)) + (sd)->sd_nNameSize)
#define SD_SIZE( sd )(sizeof(*(sd)) + (sd)->sd_nNameSize + (sd)->sd_nDataSize)
#define NEXT_SD( sd )((AfsSmallData_s*)(((char*)((sd)+1)) + (sd)->sd_nNameSize + (sd)->sd_nDataSize))

typedef struct
{
	BIterator_s fc_sBIterator;
	int fc_nOMode;
} AfsFileCookie_s;

struct _AfsVNode
{
	AfsInode_s *vn_psInode;
	sem_id vn_hMutex;
	int vn_nBTreeVersion;	/* Increased each time a node is inserted or deleted from a b+tree
				 * Used to invalidate iterators
				 */
};

#include <atheos/threads.h>
#define AFS_LOCK( vnode ) do { lock_semaphore((vnode)->vn_hMutex, SEM_NOSIG, INFINITE_TIMEOUT ); } while(0)
#define AFS_UNLOCK( vnode ) do { unlock_semaphore((vnode)->vn_hMutex ); } while(0)


static inline off_t __afs_run_to_num( const AfsSuperBlock_s * psSuperBlock, const BlockRun_s * psRun, const char *pzFile, const char *pzFunction, int nLine )
{
	off_t nNum =( off_t )psRun->group * psSuperBlock->as_nBlockPerGroup + psRun->start;

	if( nNum < 0 || nNum >= psSuperBlock->as_nNumBlocks )
	{
		printk( "PANIC : afs_run_to_num() called with run %d %d %d which gives a blocknum of %Ld.\n", psRun->group, psRun->start, psRun->len, nNum );
		printk( "PANIC : The drive only contains %Ld blocks!!\n", psSuperBlock->as_nNumBlocks );
		printk( "Called from file: %s func: %s line %d\n", pzFile, pzFunction, nLine );
	}

	return( nNum );
}

#define afs_run_to_num( psSuperBlock, psRun ) __afs_run_to_num( psSuperBlock, psRun, __FILE__, __FUNCTION__, __LINE__ )

static inline void afs_num_to_run( const AfsSuperBlock_s * psSuperBlock, BlockRun_s * psRun, off_t nNum )
{
	psRun->group = nNum / psSuperBlock->as_nBlockPerGroup;
	psRun->start = nNum % psSuperBlock->as_nBlockPerGroup;
	psRun->len = 1;
	kassertw( nNum >= 0 && nNum < psSuperBlock->as_nNumBlocks );
}

int afs_start_journal_flusher( AfsVolume_s * psVolume );

int afs_begin_transaction( AfsVolume_s * psVolume );
int afs_checkpoint_journal( AfsVolume_s * psVolume );
int afs_end_transaction( AfsVolume_s * psVolume, bool bSubmitChanges );
int afs_add_block_to_log( AfsVolume_s * psVolume, off_t nBlockNum, int nBlockCount );
void afs_flush_journal( AfsVolume_s * psVolume );
int afs_replay_log( AfsVolume_s * psVolume );

int afs_init_hash_table( AfsHashTable_s * psTable );
void afs_delete_hash_table( AfsHashTable_s * psTable );
int afs_logged_read( AfsVolume_s * psVolume, void *pBuffer, off_t nBlock );
int afs_logged_write( AfsVolume_s * psVolume, const void *pBuffer, off_t nBlock );
int afs_wait_for_block_to_leave_log( AfsVolume_s * psVolume, off_t nBlock );

void *afs_alloc_block_buffer( AfsVolume_s * psVolume );
void afs_free_block_buffer( AfsVolume_s * psVolume, void *pBuffer );

off_t afs_count_used_blocks( AfsVolume_s * psVolume );

int afs_validate_inode( const AfsVolume_s * psVolume, const AfsInode_s * psInode );
int afs_validate_file( AfsVolume_s * psVolume, AfsInode_s * psInode );
int afs_validate_stream( AfsVolume_s * psVolume, AfsInode_s * psInode, uint32 *pBitmap );

int afs_create_inode( AfsVolume_s * psVolume, AfsInode_s * psParent, int nMode, int nFlags, int nIndexType, const char *pzName, int nNameLen, AfsInode_s ** ppsRes );	// ino_t* pnInodeNum );
int afs_delete_inode( AfsVolume_s * psVolume, AfsInode_s * psInode );

int afs_mkdir( AfsVolume_s * psVolume, AfsInode_s * psParent, const char *pzName, int nNameLen, int nMode );
int afs_lookup_inode( AfsVolume_s * psVolume, AfsInode_s * psParent, const char *pzName, int nNameLen, ino_t *pnInodeNum );


int afs_alloc_blocks( AfsVolume_s * psVolume, BlockRun_s * psRun, BlockRun_s * psPrevRun, int nGroup, int nCount, int nModulo );
int afs_free_blocks( AfsVolume_s * psVolume, const BlockRun_s * psRun );
bool afs_is_block_free( AfsVolume_s * psVolume, off_t nBlock );

off_t afs_get_inode_block_count( const AfsInode_s * psInode );
int afs_get_stream_blocks( AfsVolume_s * psVolume, DataStream_s * psStream, off_t nPos, int nRequestLen, off_t *pnStart, int *pnActualLength );

//int afs_get_inode( AfsVolume_s* psVolume, ino_t nInodeNum, AfsInode_s** ppsRes );
//int afs_put_inode( AfsVolume_s* psVolume, AfsInode_s* psInode );

status_t afs_do_read_inode( AfsVolume_s * psVolume, BlockRun_s * psInodePtr, AfsInode_s ** ppsRes );
status_t afs_do_write_inode( AfsVolume_s * psVolume, AfsInode_s * psInode );
status_t afs_revert_inode( AfsVolume_s * psVolume, AfsInode_s * psInode );

int afs_release_inode( AfsVolume_s * psVolume, AfsInode_s * psInode );
int afs_do_unlink( AfsVolume_s * psVolume, AfsInode_s * psParent, AfsInode_s * psInode, const char *pzName, int nNameLen );

int afs_read_pos( AfsVolume_s * psVolume, AfsInode_s * psInode, void *pBuffer, off_t nPos, size_t nSize );
int afs_do_write( AfsVolume_s * psVolume, AfsInode_s * psInode, const char *pBuffer, off_t nPos, size_t nSize );

int afs_expand_stream( AfsVolume_s * psVolume, AfsInode_s * psInode, off_t nDeltaSize );
int afs_truncate_stream( AfsVolume_s * psVolume, AfsInode_s * psInode );

int afs_do_read_dir( AfsVolume_s * psVolume, AfsInode_s * psInode, BIterator_s * psIterator, int nPos, struct kernel_dirent *psFileInfo, size_t nBufSize );

int afs_open_attrdir( void *pVolume, void *pNode, void **pCookie );
int afs_close_attrdir( void *pVolume, void *pNode, void *pCookie );
int afs_rewind_attrdir( void *pVolume, void *pNode, void *pCookie );
int afs_read_attrdir( void *pVolume, void *pNode, void *pCookie, struct kernel_dirent *psBuffer, size_t bufsize );
int afs_remove_attr( void *pVolume, void *pNode, const char *pzName, int nNameLen );
int afs_stat_attr( void *pVolume, void *pNode, const char *pzName, int nNameLen, struct attr_info *psBuffer );
int afs_write_attr( void *pVolume, void *pNode, const char *pzName, int nNameLen, int nFlags, int nType, const void *pBuffer, off_t nPos, size_t nLen );
int afs_read_attr( void *pVolume, void *pNode, const char *pzName, int nNameLen, int nType, void *pBuffer, off_t nPos, size_t nLen );
int afs_delete_file_attribs( AfsVolume_s * psVolume, AfsInode_s * psInode, bool bNoLock );


int afs_open_indexdir( void *pVolume, void **pCookie );
int afs_close_indexdir( void *pVolume, void *pCookie );
int afs_rewind_indexdir( void *pVolume, void *pCookie );
int afs_read_indexdir( void *pVolume, void *pCookie, struct kernel_dirent *psBuffer, size_t nBufferSize );
int afs_create_index( void *pVolume, const char *pzName, int nNameLen, int nType, int nFlags );
int afs_remove_index( void *pVolume, const char *pzName, int nNameLen );
int afs_stat_index( void *pVolume, const char *pzName, int nNameLen, struct index_info *psBuffer );


#endif /* __F_AFS_H__ */
