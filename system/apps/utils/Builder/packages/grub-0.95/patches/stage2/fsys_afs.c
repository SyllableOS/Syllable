#ifdef FSYS_AFS

#define assert(a)

#include "shared.h"
#include "filesys.h"

#include "afs.h"

#define g_psSuperBlock  ((AfsSuperBlock_s*)FSYS_BUF)
#define g_psCurInode ((AfsInode_s*)(((char*)FSYS_BUF)+sizeof(AfsSuperBlock_s)))
#define g_psTreeNode ((BNode_s*)(((char*)FSYS_BUF)+sizeof(AfsSuperBlock_s)+sizeof(AfsInode_s)))
#define g_anBlockBuffer (((char*)FSYS_BUF)+sizeof(AfsSuperBlock_s)+sizeof(AfsInode_s)+B_NODE_SIZE)


#define ENOENT 1
#define EINVAL 2
#define EIO    3

/*
enum {false,true};
*/
#define false 0
#define true 1

#define	min(a,b) (((a)<(b)) ? (a) : (b) )


static int read_blocks( void* pBuffer, afs_off_t nOffset, int nSize )
{
    if (!devread( nOffset / 512, nOffset % 512, nSize, pBuffer )) {
	return( -1 );
    } else {
	return( nSize );
    }
}

static afs_off_t afs_run_to_num( const AfsSuperBlock_s* psSuperBlock, const
 BlockRun_s* psRun )
{
    return( (afs_off_t)psRun->group * psSuperBlock->as_nBlockPerGroup +
 psRun->start );
}

static void bt_get_key( BNode_s* psNode, int nIndex, char** ppKey, int* pnSize )
{
    const int16* pnKeyIndexes = B_KEY_INDEX_OFFSET_CONST( psNode );
    const int16  nKeyEnd      = pnKeyIndexes[nIndex];
    const int16  nKeyStart    = ( nIndex > 0 ) ? pnKeyIndexes[nIndex - 1] : 0;

    *ppKey  = &psNode->bn_anKeyData[nKeyStart];
    *pnSize = nKeyEnd - nKeyStart;
}

static int afs_validate_superblock( AfsSuperBlock_s* psSuperBlock )
{
    int	bResult = true;
	
    if ( psSuperBlock->as_nMagic1 != SUPER_BLOCK_MAGIC1 ) {
	bResult = false;
    }
    if ( psSuperBlock->as_nMagic2 != SUPER_BLOCK_MAGIC2 ) {
	bResult = false;
    }
    if ( psSuperBlock->as_nMagic3 != SUPER_BLOCK_MAGIC3 ) {
	bResult = false;
    }
    if ( psSuperBlock->as_nByteOrder != BO_LITTLE_ENDIAN &&
 psSuperBlock->as_nByteOrder != BO_BIG_ENDIAN ) {
	bResult = false;
    }
    if ( 1 << psSuperBlock->as_nBlockShift != psSuperBlock->as_nBlockSize ) {
	bResult = false;
    }
    if ( psSuperBlock->as_nUsedBlocks > psSuperBlock->as_nNumBlocks ) {
	bResult = false;
    }
    if ( psSuperBlock->as_nInodeSize != psSuperBlock->as_nBlockSize ) {
	bResult = false;
    }
    if ( 0 == psSuperBlock->as_nBlockSize ||
	 (1 << psSuperBlock->as_nAllocGrpShift) / psSuperBlock->as_nBlockSize !=
 psSuperBlock->as_nBlockPerGroup ) {
	bResult = false;
    }
    if ( psSuperBlock->as_nAllocGroupCount * psSuperBlock->as_nBlockPerGroup <
 psSuperBlock->as_nNumBlocks ) {
	bResult = false;
    }
    if ( psSuperBlock->as_sLogBlock.len != psSuperBlock->as_nLogSize ) {
	bResult = false;
    }
    if ( psSuperBlock->as_nValidLogBlocks > psSuperBlock->as_nLogSize ) {
	bResult = false;
    }
    return( bResult );
}

int afs_mount()
{
    read_blocks( g_psSuperBlock, 1024, sizeof(AfsSuperBlock_s) );
    if ( afs_validate_superblock( g_psSuperBlock ) == false ) {
	read_blocks( g_psSuperBlock, 0, sizeof(AfsSuperBlock_s) );
	if ( afs_validate_superblock( g_psSuperBlock ) == false ) {
	    return( 0 );
	}
    }
    return( 1 );
}

static int get_stream_blocks( AfsSuperBlock_s* psSuperBlock, DataStream_s*
 psStream,
			      int nPos, int nRequestLen, afs_off_t* pnStart, int* pnActualLength )
{
    int	      nBlockSize = psSuperBlock->as_nBlockSize;
    afs_off_t nStart     = -1;
    int       nLen       = 0;
    int	      nError     = 0;
  
    if ( nPos < psStream->ds_nMaxDirectRange ) {
	int nCurPos = 0;
	int i;

	for ( i = 0 ; i < DIRECT_BLOCK_COUNT ; ++i ) {
	    if ( nPos >= nCurPos && nPos < nCurPos + psStream->ds_asDirect[ i ].len ) {
		int nOff = nPos - nCurPos;
		int j;
	
		nStart = afs_run_to_num( psSuperBlock, &psStream->ds_asDirect[ i ] ) + nOff;
		nLen   = psStream->ds_asDirect[ i ].len - nOff;
	  
		assert( nLen > 0 );
	
		for( j = i + 1 ; j < DIRECT_BLOCK_COUNT && nLen < nRequestLen ; ++j ) {
		    int nBlkNum = afs_run_to_num( psSuperBlock, &psStream->ds_asDirect[ j ] );

		    if ( nStart + nLen == nBlkNum ) {
			nLen += psStream->ds_asDirect[ j ].len;
		    } else {
			break;
		    }
		}
		break;
	    }
	    nCurPos += psStream->ds_asDirect[ i ].len;
	}
    } else {
	if ( nPos < psStream->ds_nMaxIndirectRange ) {
	    afs_off_t   nBlk        = afs_run_to_num( psSuperBlock,
 &psStream->ds_sIndirect );
	    int         nCurPos     = psStream->ds_nMaxDirectRange;
	    int         nPtrsPerBlk = nBlockSize / sizeof(BlockRun_s);
	    BlockRun_s* psBlock;
	    int         i;

	    psBlock = (BlockRun_s*)g_anBlockBuffer;

      
	    for ( i = 0 ; i < psStream->ds_sIndirect.len && -1 == nStart ; ++i, ++nBlk
 ) {
		int  j;
		nError = read_blocks( psBlock, nBlk*nBlockSize, nBlockSize );

		if ( nError < 0 ) {
		    goto error;
		}
		for ( j = 0 ; j < nPtrsPerBlk ; ++j ) {
		    if ( psBlock[ j ].len <= 0 ) {
			nError = -EINVAL;
			break;
		    }
		    if ( nPos >= nCurPos && nPos < nCurPos + psBlock[ j ].len ) {
			int nOff = nPos - nCurPos;
			int k;

			nStart = afs_run_to_num( psSuperBlock, &psBlock[ j ] ) + nOff;
			nLen   = psBlock[ j ].len - nOff;

			assert( nLen > 0 );
	      
			for( k = j + 1 ; k < nPtrsPerBlk && nLen < nRequestLen ; ++k ) {
			    afs_off_t nBlkNum = afs_run_to_num( psSuperBlock, &psBlock[k] );

			    if ( nStart + nLen == nBlkNum ) {
				nLen += psBlock[k].len;
			    } else {
				break;
			    }
			}
			break;
		    }
		    nCurPos += psBlock[ j ].len;
		}
	    }
	} else {
	    int nPtrsPerBlk = nBlockSize / sizeof(BlockRun_s);

	      //  ([nIDBlk][nIDPtr])([nDBlk][nDPtr])[nBlk]

	    const int   nCurPos = nPos - psStream->ds_nMaxIndirectRange;
	    const int   nDPtrSize  = BLOCKS_PER_DI_RUN;
	    const int   nDBlkSize  = BLOCKS_PER_DI_RUN * nPtrsPerBlk;
	    const int   nIDPtrSize = BLOCKS_PER_DI_RUN * nPtrsPerBlk *
 BLOCKS_PER_DI_RUN;
	    const int   nIDBlkSize = BLOCKS_PER_DI_RUN * nPtrsPerBlk *
 BLOCKS_PER_DI_RUN * nPtrsPerBlk;

	    const int       nOff  = nCurPos % BLOCKS_PER_DI_RUN;
	    const int       nDPtr = (nCurPos / nDPtrSize) % nPtrsPerBlk;
	    const int       nDBlk = (nCurPos / nDBlkSize) % BLOCKS_PER_DI_RUN;
	    const int       nIDPtr = (nCurPos / nIDPtrSize) % nPtrsPerBlk;
	    const int       nIDBlk = (nCurPos / nIDBlkSize);
	    const afs_off_t nIndirectBlk = afs_run_to_num( psSuperBlock,
 &psStream->ds_sDoubleIndirect ) + nIDBlk;
	    afs_off_t 	    nDirectBlk;
	    BlockRun_s*     psBlock;
	    int 	    i;

	    psBlock = (BlockRun_s*) g_anBlockBuffer;

	      // Load the indirect block
	    nError = read_blocks( psBlock, nIndirectBlk*nBlockSize, nBlockSize );
	    if ( nError < 0 ) {
		goto error;
	    }
	      // Fetch the direct block pointer and read it into the buffer
	    nDirectBlk = afs_run_to_num( psSuperBlock, &psBlock[nIDPtr] ) + nDBlk;
	    nError = read_blocks( psBlock, nDirectBlk*nBlockSize, nBlockSize );
	    if ( nError < 0 ) {
		goto error;
	    }
	  
	    nStart = afs_run_to_num( psSuperBlock, &psBlock[nDPtr] ) + nOff;
	    nLen   = psBlock[ nDPtr ].len - nOff;

	    assert( nLen > 0 );

	    for( i = nDPtr + 1 ; i < nPtrsPerBlk && nLen < nRequestLen ; ++i ) {
		afs_off_t nBlkNum = afs_run_to_num( psSuperBlock, &psBlock[i] );
	
		if ( nStart + nLen == nBlkNum ) {
		    nLen += psBlock[i].len;
		} else {
		    break;
		}
	    }
	}
    }

    *pnStart = nStart;
    *pnActualLength = nLen;
error:
    return( nError );
}



static int do_read( void* a_pBuffer, int nPos, int nSize )
{
    char*	pBuffer = a_pBuffer;
    const int	nBlockSize = g_psSuperBlock->as_nBlockSize;
    int		nFirstBlock = nPos / nBlockSize;
    int		nLastBlock  = (nPos + nSize + nBlockSize - 1)  / nBlockSize;
    int		nNumBlocks  = nLastBlock - nFirstBlock + 1;
    int		nOffset = nPos % nBlockSize;
    afs_off_t	nBlockAddr;
    char*	pBlock = NULL;
    int		nRunLength;
    int		nReadSize;
    int		nError;


    if ( nPos > g_psCurInode->ai_sData.ds_nSize ) {
	return( 0 );
    }
  
    if ( (nPos + nSize) > g_psCurInode->ai_sData.ds_nSize ) {
	nSize = g_psCurInode->ai_sData.ds_nSize - nPos;
    }

    if ( nSize <= 0 ) {
	return( 0 );
    }
    nReadSize = nSize;
    
    nError = get_stream_blocks( g_psSuperBlock, &g_psCurInode->ai_sData,
 nFirstBlock,
				nNumBlocks, &nBlockAddr, &nRunLength );

    
    pBlock = g_anBlockBuffer;
    
    if ( nError >= 0 && 0 != nOffset )
    {
//#ifndef STAGE1_5
	disk_read_func = disk_read_hook;
//#endif /* STAGE1_5 */
	nError = read_blocks( pBlock, nBlockAddr*nBlockSize, nBlockSize );
//#ifndef STAGE1_5
	disk_read_func = NULL;
//#endif /* STAGE1_5 */
		
	if ( nError >= 0 ) {
	    int nLen = min( nSize, nBlockSize - nOffset );
			
	    memcpy( pBuffer, pBlock + nOffset, nLen );

	    pBuffer += nLen;
	    nSize	  -= nLen;
	}
	nBlockAddr++;
	nFirstBlock++;
	nRunLength--;
	nNumBlocks--;
    }
  
    for ( ; nSize >= nBlockSize && nError >= 0 ; ) {
	if ( 0 == nRunLength ) {
	    nError = get_stream_blocks( g_psSuperBlock, &g_psCurInode->ai_sData,
 nFirstBlock,
					nNumBlocks, &nBlockAddr, &nRunLength );
	}
	if ( nError >= 0 ) {
	    int nLen = min( nSize / nBlockSize, nRunLength );
	    int i;
	    nError = 0;
	    for ( i = 0 ; i < nLen ; ++i ) {
//#ifndef STAGE1_5
		disk_read_func = disk_read_hook;
//#endif /* STAGE1_5 */
		nError = read_blocks( pBuffer, nBlockAddr*nBlockSize, nBlockSize );
//#ifndef STAGE1_5
		disk_read_func = NULL;
//#endif /* STAGE1_5 */
		if ( nError < 0 ) {
		    break;
		}
		nRunLength--;
		nNumBlocks--;
		nFirstBlock++;
		nBlockAddr++;
		nSize   -= nBlockSize;
		pBuffer += nBlockSize;
		assert( nRunLength >= 0 );
		assert( nSize >= 0 );
	    }
	}
    }
    if ( nSize > 0 && nError >= 0 ) {
	if ( 0 == nRunLength ) {
	    nError = get_stream_blocks( g_psSuperBlock, &g_psCurInode->ai_sData,
 nFirstBlock,
					nNumBlocks, &nBlockAddr, &nRunLength );
	}
	if ( nError >= 0 ) {
//#ifndef STAGE1_5
	    disk_read_func = disk_read_hook;
//#endif /* STAGE1_5 */
	    nError = read_blocks( pBlock, nBlockAddr*nBlockSize, nBlockSize );
//#ifndef STAGE1_5
	    disk_read_func = NULL;
//#endif /* STAGE1_5 */
	    if ( nError >= 0 ) {
		memcpy( pBuffer, pBlock, nSize );
	    }
	}
    }
    return( (nError<0) ? nError : nReadSize );
}

int afs_read( char* pBuffer, int nLen )
{
    int nError = do_read( pBuffer, filepos, nLen );
    
    if ( nError >= 0 ) {
	filepos += nError;
	return( nError );
    } else {
	errnum = ERR_FSYS_CORRUPT;
	return( 0 );
    }
}

int afs_dir( char* pzPath )
{
    BTree_s  sTreeHeader;
    char*    pzName;
    bvalue_t nNode;
    int64    nCurInode;
    int      nError;
    char     ch;
    int	     i;
    
# ifndef STAGE1_5
    int   bDoPrint = false;
#endif    

    filepos = 0;
    filemax = MAXINT;
    nCurInode = afs_run_to_num( g_psSuperBlock, &g_psSuperBlock->as_sRootDir );

again:
    while (*pzPath == '/') {
	pzPath++;
    }
    
    read_blocks( g_psCurInode, nCurInode*g_psSuperBlock->as_nBlockSize,
 sizeof(AfsInode_s) );

    if ( print_possibilities == 0 ) {
	if (!*pzPath || isspace (*pzPath)) {
	    if ( AFS_S_ISDIR( g_psCurInode->ai_nMode ) ) {
		errnum = ERR_BAD_FILETYPE;
		return 0;
	    }
	    filepos = 0;
	    filemax = g_psCurInode->ai_sData.ds_nSize;
	    return 1;
	}
    }
    
    if ( AFS_S_ISDIR( g_psCurInode->ai_nMode ) == false ) {
	if (print_possibilities < 0) {
	    return 1;
	}
	errnum = ERR_BAD_FILETYPE;
	return 0;
    }

    for (pzName = pzPath; (ch = *pzName) && !isspace(ch) && ch != '/';
 pzName++);
  
    *pzName = 0;

# ifndef STAGE1_5
    if (print_possibilities && ch != '/') {
	bDoPrint = 1;
    }
#endif

    if ( 0 == g_psCurInode->ai_sData.ds_nSize ) {
	return( -ENOENT );
    }

    nError = do_read( &sTreeHeader, 0, sizeof(BTree_s) );
    if ( nError < 0 ) {
	errnum = ERR_READ;
	return( 0 );
    }
    

      // Find the first B+tree node
    nNode = sTreeHeader.bt_nRoot;

    nError = do_read( g_psTreeNode, nNode, B_NODE_SIZE );

    if ( nError < 0 ) {
	errnum = ERR_READ;
	return( 0 );
    }
    
      // Follow the leftmost branch from nNode until we reach a leaf node
  
    for ( i = 0 ; i < sTreeHeader.bt_nTreeDepth - 1 ; ++i ) {
	nNode = B_KEY_VALUE_OFFSET_CONST( g_psTreeNode )[0];
	nError = do_read( g_psTreeNode, nNode, B_NODE_SIZE );
	if ( nError < 0 ) {
	    errnum = ERR_READ;
	    return( 0 );
	}
    }
    if ( g_psTreeNode->bn_nKeyCount > 0 ) {
	char* pzFileName;
	int   nKeySize;
	int   nCurKey  = 0;

	  // All leaf nodes in the tree is linked together in a linear list.
	  // This comes in handy now, so we don't need a full-blown B+tree
	  // search algorithm in here. We just iterate the nodes lineary.
	while( true ) {
	    char nFNTerm;
	    nCurInode = B_KEY_VALUE_OFFSET( g_psTreeNode )[ nCurKey ];
	
	    bt_get_key( g_psTreeNode, nCurKey++, &pzFileName, &nKeySize );
	    nFNTerm = pzFileName[nKeySize];
	    pzFileName[nKeySize] = '\0';
# ifndef STAGE1_5
	    if ( bDoPrint && substring( pzPath, pzFileName ) <= 0 ) {
		if (print_possibilities > 0)
		    print_possibilities = -print_possibilities;
		print_a_completion( pzFileName );
	    }
#endif
	    if ( substring( pzPath, pzFileName ) == 0 ) {
		*(pzPath = pzName) = ch;
		goto again;
	    }
	    pzFileName[nKeySize] = nFNTerm;
	
	    if ( nCurKey >= g_psTreeNode->bn_nKeyCount ) {
		if ( NULL_VAL == g_psTreeNode->bn_nRight ) {
		    break;
		}
    		nNode = g_psTreeNode->bn_nRight;
		nError = do_read( g_psTreeNode, g_psTreeNode->bn_nRight, B_NODE_SIZE );
		if ( nError < 0 ) {
		    errnum = ERR_READ;
		    return( 0 );
		}
		nCurKey  = 0;
	    }
	    nError = 0;
	}
    }
    *pzName = ch;
# ifndef STAGE1_5
    if (print_possibilities < 0)
    {
	return 1;
    }
#endif    
    errnum = ERR_FILE_NOT_FOUND;
    return( 0 );
}

int afs_embed (int *start_sector, int needed_sectors )
{
    read_blocks( g_psSuperBlock, 1024, sizeof(AfsSuperBlock_s) );
    if ( afs_validate_superblock( g_psSuperBlock ) == false ) {
	return 0;
    }
    if ( needed_sectors > g_psSuperBlock->as_nBootLoaderSize ) {
	return 0;
    }
    *start_sector = 4;
    return( 1 );
}
#endif /* FSYS_AFS */
