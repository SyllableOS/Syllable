
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

#include <posix/errno.h>
#include <atheos/types.h>
#include <atheos/kernel.h>
#include <macros.h>

#include "afs.h"
#include "btree.h"


static bvalue_t bt_alloc_node( AfsVolume_s * psVolume, AfsInode_s * psInode, BTransaction_s * psTrans );

static int g_nTotNodeCount = 0;

bool bt_validate_tree_header( BTree_s * psTree )
{
	bool bResult = true;

	if( psTree == NULL )
	{
		panic( "bt_validate_tree_header() called with NULL pointer\n" );
		return( false );
	}

	if( psTree->bt_nMagic != BTREE_MAGIC )
	{
		bResult = false;
		printk( "Error: bt_validate_tree_header() bad magic: %08x (Should be %08x)\n", psTree->bt_nMagic, BTREE_MAGIC );
	}
	return( bResult );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int bt_begin_transaction( BStack_s * psStack, BTransaction_s * psTrans )
{
	memset( psStack, 0, sizeof( BStack_s ) );
	memset( psTrans, 0, sizeof( BTransaction_s ) );
	return( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int bt_end_transaction( AfsVolume_s * psVolume, AfsInode_s * psInode, BTransaction_s * psTrans, bool bOkToWrite )
{
	int nBlockSize;
	BTree_s *psTree;
	int nError = 0;
	bool bChanged = false;
	int i;

#ifdef AFS_PARANOIA
	if( psVolume == NULL )
	{
		panic( "bt_end_transaction() calle with psVolume == NULL\n" );
		return( -EINVAL );
	}
	if( psInode == NULL )
	{
		panic( "bt_end_transaction() called with psInode == NULL\n" );
		return( -EINVAL );
	}
	if( psTrans == NULL )
	{
		panic( "bt_end_transaction() called with psTrans == NULL\n" );
		return( -EINVAL );
	}
#endif
	nBlockSize = psVolume->av_psSuperBlock->as_nBlockSize;
	psTree =( BTree_s * ) bt_load_node( psVolume, psInode, psTrans, 0 );

	if( psTree == NULL )
	{
		printk( "Error: bt_end_transaction() failed to load tree header\n" );
		return( -EIO );
	}

	if( bt_validate_tree_header( psTree ) == false )
	{
		panic( "bt_end_transaction() invalid three header!\n" );
		return( -EINVAL );
	}

	if( psTrans->bt_nLockedNodeCnt >= 100 )
	{
		panic( "bt_end_transaction() touched to many nodes: %d\n", psTrans->bt_nLockedNodeCnt );
		return( -EINVAL );
	}
	if( bOkToWrite )
	{
		if( ( afs_get_inode_block_count( psInode ) * nBlockSize ) < psTree->bt_nLastNode )
		{
			off_t nDeltaSize =( ( psTree->bt_nLastNode + nBlockSize - 1 ) / nBlockSize ) - afs_get_inode_block_count( psInode );

			nError = afs_expand_stream( psVolume, psInode, nDeltaSize + 64 );

			if( nError < 0 /*&& nError != -ENOSPC && nError != -EFBIG */  )
			{
				bOkToWrite = false;
				printk( "Error: bt_end_transaction:1() Failed to expand b+tree!\n" );
			}
			else
			{
				if( ( afs_get_inode_block_count( psInode ) * nBlockSize ) < psTree->bt_nLastNode )
				{
					printk( "Error: bt_end_transaction:2() Failed to expand b+tree!\n" );
					nError = -ENOSPC;
					bOkToWrite = false;
					goto error;
				}
				else
				{
					psInode->ai_sData.ds_nSize = psTree->bt_nLastNode;
					psInode->ai_nFlags |= INF_STAT_CHANGED;
					nError = 0;
				}
				nError = afs_do_write_inode( psVolume, psInode );
				if( nError < 0 )
				{
					printk( "Error: bt_end_transaction() failed to write inode\n" );
					bOkToWrite = false;
				}
			}
		}
		else if( psInode->ai_sData.ds_nSize != psTree->bt_nLastNode )
		{
			psInode->ai_sData.ds_nSize = psTree->bt_nLastNode;
			nError = afs_do_write_inode( psVolume, psInode );
			if( nError < 0 )
			{
				printk( "Error: bt_end_transaction() failed to write inode\n" );
				bOkToWrite = false;
			}
		}
	}
      error:
	for( i = 0; i < psTrans->bt_nLockedNodeCnt; ++i )
	{
		if( NULL_VAL == psTrans->bt_apsLockedNodes[i].be_nValue )
		{
			panic( "bt_end_transaction() locked node %d is not allocated!\n", i );
			nError = -EINVAL;
			break;
		}
		if( psTrans->bt_apsLockedNodes[i].be_nValue > ( 1024 * 1024 * 50 ) )
		{
			panic( "bt_end_transaction() locked node %d has insane position!\n", i );
			nError = -EINVAL;
			break;
		}
		if( psTrans->bt_apsLockedNodes[i].be_bDirty )
		{
			if( bOkToWrite )
			{
				nError = afs_do_write( psVolume, psInode,( char * )psTrans->bt_apsLockedNodes[i].be_psNode, psTrans->bt_apsLockedNodes[i].be_nValue, B_NODE_SIZE );
				if( nError < 0 )
				{
					printk( "Error: bt_end_transaction() failed to write tree node to disk\n" );
					bOkToWrite = false;
				}
				else
				{
					bChanged = true;
				}
			}
			else
			{
				printk( "Error: bt_end_transaction() not allowed to write back dirty node %d\n",( int )psTrans->bt_apsLockedNodes[i].be_nValue );
			}
		}
		kfree( psTrans->bt_apsLockedNodes[i].be_psNode );
	}

	psTrans->bt_nLockedNodeCnt = 0;

	if( bChanged && NULL != psInode->ai_psVNode )
	{
		psInode->ai_psVNode->vn_nBTreeVersion++;
	}
	return( nError );
}

/** Allocate a temporary node buffer.
 * \par Description:
 *	Allocate temporary storage for a tree node.
 *	The node must be released by bt_free_tmp_node()
 *	when not needed anymore.
 * \par Note:
 *	No disk-space is reserved for the node.
 *
 * \return
 *	Pointer to the new node or NULL if no memory was available.
 *
 * \author	Kurt Skauen(kurt@atheos.cx)
 *****************************************************************************/

static BNode_s *bt_alloc_tmp_node()
{
	BNode_s *psNode = kmalloc( B_NODE_SIZE + 4, MEMF_KERNEL | MEMF_OKTOFAILHACK );

	if( psNode == NULL )
	{
		printk( "Error: bt_alloc_tmp_node() no memory for new node\n" );
		return( NULL );
	}
	g_nTotNodeCount++;

	memset( psNode, 0, B_HEADER_SIZE );

	psNode->bn_nLeft = NULL_VAL;
	psNode->bn_nRight = NULL_VAL;
	psNode->bn_nOverflow = NULL_VAL;
	psNode->bn_nKeyCount = 0;
	psNode->bn_nTotKeySize = 0;

	(( int * )( ( ( char * )psNode ) + B_NODE_SIZE ) )[0] = 0x87654321;

	return( psNode );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static void bt_free_tmp_node( BNode_s *psNode )
{
	if( psNode == NULL )
	{
		panic( "bt_free_tmp_node() called with psNode == NULL\n" );
		return;
	}
	kassertw(( ( int * )( ( ( char * )psNode ) + B_NODE_SIZE ) )[0] = 0x87654321 );
	kfree( psNode );

	g_nTotNodeCount--;
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static BNode_s *bt_malloc_node()
{
	BNode_s *psNode = kmalloc( B_NODE_SIZE + 4, MEMF_KERNEL | MEMF_OKTOFAILHACK );

	if( psNode == NULL )
	{
		printk( "bt_malloc_node() no memory for new node\n" );
		return( NULL );
	}

	memset( psNode, 0, B_HEADER_SIZE );

	psNode->bn_nLeft = NULL_VAL;
	psNode->bn_nRight = NULL_VAL;
	psNode->bn_nOverflow = NULL_VAL;
	psNode->bn_nKeyCount = 0;
	psNode->bn_nTotKeySize = 0;

	(( int * )( ( ( char * )psNode ) + B_NODE_SIZE ) )[0] = 0x12345678;

	return( psNode );
}

static int bt_init_tree( AfsVolume_s * psVolume, AfsInode_s * psInode, BTransaction_s * psTrans )
{
	BTree_s *psTree;

#ifdef AFS_PARANOIA
	// Late "compile-time" check
	if( sizeof( BTree_s ) > B_NODE_SIZE )
	{
		panic( "bt_init_tree() BTree_s structure has grown to larg(%d vs %d)\n", sizeof( BTree_s ), B_NODE_SIZE );
	}
	if( psVolume == NULL )
	{
		panic( "bt_init_tree() called with psVolume == NULL\n" );
		return( -EINVAL );
	}
	if( psInode == NULL )
	{
		panic( "bt_init_tree() called with psInode == NULL\n" );
		return( -EINVAL );
	}
	if( psTrans == NULL )
	{
		panic( "bt_init_tree() called with psTrans == NULL\n" );
		return( -EINVAL );
	}
#endif
	psTree = kmalloc( B_NODE_SIZE, MEMF_KERNEL | MEMF_OKTOFAILHACK );

	if( psTree == NULL )
	{
		return( -ENOMEM );
	}

	memset( psTree, 0, sizeof( BTree_s ) );

	psTree->bt_nMagic = BTREE_MAGIC;
	psTree->bt_nTreeDepth = 1;
	psTree->bt_nFirstFree = NULL_VAL;
	psTree->bt_nLastNode = B_NODE_SIZE;	// Skip the tree header

	psTrans->bt_apsLockedNodes[0].be_psNode =( BNode_s * )psTree;
	psTrans->bt_apsLockedNodes[0].be_nValue = 0;
	psTrans->bt_apsLockedNodes[0].be_bDirty = true;
	psTrans->bt_nLockedNodeCnt = 1;

	psTree->bt_nRoot = bt_alloc_node( psVolume, psInode, psTrans );

	if( psTree->bt_nRoot == NULL_VAL )
	{
		psTrans->bt_nLockedNodeCnt = 0;
		kfree( psTree );
		return( -ENOMEM );
	}
	return( 0 );
}

/** Load a tree node.
 * \par Description:
 *	bt_load_node() will first check if the node is loaded already
 *	and return the old copy if so. If the node is not found a
 *	new node-buffer will be allocated and the node will be loaded.
 *
 *	If the tree has not yet been initialized the tree-header will
 *	be allocated and initialized.
 * \par Note:
 *	The returned node will be automatically released in
 *	bt_end_transaction() and should not be released
 *	manually.
 *
 * \param
 * \return
 * \sa
 * \author	Kurt Skauen(kurt@atheos.cx)
 *****************************************************************************/

BNode_s *bt_load_node( AfsVolume_s * psVolume, AfsInode_s * psInode, BTransaction_s * psTrans, bvalue_t nNode )
{
	BNode_s *psNode;
	int nError;
	int i;

#ifdef AFS_PARANOIA
	if( psVolume == NULL )
	{
		panic( "bt_load_node() called with psVolume == NULL\n" );
		return( NULL );
	}
	if( psInode == NULL )
	{
		panic( "bt_load_node() called with psInode == NULL\n" );
		return( NULL );
	}
	if( psTrans == NULL )
	{
		panic( "bt_load_node() called with psTrans == NULL\n" );
		return( NULL );
	}
	if( nNode == NULL_VAL )
	{
		panic( "bt_load_node() asked to load a NULL_VAL node!!\n" );
		return( NULL );
	}
#endif

	/* Check if the node has been loaded already */
	for( ;; )
	{
		for( i = 0; i < psTrans->bt_nLockedNodeCnt; ++i )
		{
			if( nNode == psTrans->bt_apsLockedNodes[i].be_nValue )
			{
				if( nNode == 0 )
				{
					if( bt_validate_tree_header( ( BTree_s * ) psTrans->bt_apsLockedNodes[i].be_psNode ) == false )
					{
						printk( "Error: bt_load_node() found bad tree header in node cache\n" );
						return( NULL );
					}
				}
				return( psTrans->bt_apsLockedNodes[i].be_psNode );
			}
		}
		if( 0 == psInode->ai_sData.ds_nSize && psTrans->bt_nLockedNodeCnt == 0 )
		{
			nError = bt_init_tree( psVolume, psInode, psTrans );
			if( nError < 0 )
			{
				printk( "Error: bt_load_node() failed to initialize b+tree %d\n", nError );
				return( NULL );
			}
		}
		else
		{
			break;
		}
	}
	/* Could not find the node. Go on and allocate/load it. */
	psNode = bt_malloc_node();

	if( psNode == NULL )
	{
		printk( "Error: bt_load_node() failed to allocate memory for node\n" );
		return( NULL );
	}
	if( psInode->ai_sData.ds_nSize < nNode + B_NODE_SIZE )
	{
		panic( "bt_load_node() node lives outside the stream\n" );
		goto error;
	}
	nError = afs_read_pos( psVolume, psInode, psNode, nNode, B_NODE_SIZE );
	if( nError != B_NODE_SIZE )
	{
		printk( "Error: bt_load_node() failed to load node %d(flen=%d). Err = %d\n", ( int )nNode, ( int )psInode->ai_sData.ds_nSize, nError );
		trace_stack( 0, NULL );
		goto error;
	}
	if( nNode == 0 )
	{
		if( bt_validate_tree_header( ( BTree_s * ) psNode ) == false )
		{
			printk( "Error: bt_load_node() loaded bad tree header\n" );
			goto error;
		}
	}

	psTrans->bt_apsLockedNodes[i].be_psNode = psNode;
	psTrans->bt_apsLockedNodes[i].be_nValue = nNode;
	psTrans->bt_apsLockedNodes[i].be_bDirty = false;
	psTrans->bt_nLockedNodeCnt++;

	return( psNode );
      error:
	kfree( psNode );
	return( NULL );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

void bt_mark_node_dirty( BTransaction_s * psTrans, bvalue_t nNode )
{
	int i;

#ifdef AFS_PARANOIA
	if( psTrans == NULL )
	{
		panic( "bt_mark_node_dirty() called with psTrans == NULL\n" );
		return;
	}
	if( nNode == NULL_VAL )
	{
		panic( "bt_mark_node_dirty() called with nNode == NULL_VAL\n" );
		return;
	}
#endif

	for( i = 0; i < psTrans->bt_nLockedNodeCnt; ++i )
	{
		if( nNode == psTrans->bt_apsLockedNodes[i].be_nValue )
		{
			psTrans->bt_apsLockedNodes[i].be_bDirty = true;
			return;
		}
	}
	panic( "bt_mark_node_dirty() could not find the node\n" );
}

/** Allocate a new tree-node.
 * \par Description:
 * \par Note:
 * \par Warning:
 * \param
 * \return
 * \par Error codes:
 * \sa
 * \author	Kurt Skauen(kurt@atheos.cx)
 *****************************************************************************/

static bvalue_t bt_alloc_node( AfsVolume_s * psVolume, AfsInode_s * psInode, BTransaction_s * psTrans )
{
	BTree_s *psTree;
	bvalue_t nResult = NULL_VAL;

#ifdef AFS_PARANOIA
	if( psVolume == NULL )
	{
		panic( "bt_alloc_node() called with psVolume == NULL\n" );
		return( NULL_VAL );
	}
	if( psInode == NULL )
	{
		panic( "bt_alloc_node() called with psInode == NULL\n" );
		return( NULL_VAL );
	}
	if( psTrans == NULL )
	{
		panic( "bt_alloc_node() called with psTrans == NULL\n" );
		return( NULL_VAL );
	}
#endif
	psTree =( BTree_s * ) bt_load_node( psVolume, psInode, psTrans, 0 );

	if( psTree == NULL )
	{
		printk( "Error: bt_alloc_node() failed to load tree header\n" );
		return( NULL_VAL );
	}

	if( bt_validate_tree_header( psTree ) == false )
	{
		panic( "bt_alloc_node() invalid three header!\n" );
		return( NULL_VAL );
	}

	if( psTree->bt_nFirstFree != NULL_VAL )
	{
		BNode_s *psNode;

		psNode = bt_load_node( psVolume, psInode, psTrans, psTree->bt_nFirstFree );

		kassertw( psNode != NULL );	// Make some noice

		if( psNode != NULL )
		{
			nResult = psTree->bt_nFirstFree;

			psTree->bt_nFirstFree = psNode->bn_nRight;

			psNode->bn_nLeft = NULL_VAL;
			psNode->bn_nRight = NULL_VAL;
			psNode->bn_nOverflow = NULL_VAL;
			psNode->bn_nTotKeySize = 0;
			bt_mark_node_dirty( psTrans, nResult );

			if( psNode->bn_nKeyCount != 0x12345678 )
			{
				printk( "bt_alloc_node() bn_nKeyCount = %x\n", psNode->bn_nKeyCount );
			}
			kassertw( psNode->bn_nKeyCount == 0x12345678 );
			psNode->bn_nKeyCount = 0;
			bt_mark_node_dirty( psTrans, 0 );	// Mark the tree header as dirty

			g_nTotNodeCount++;
		}
	}
	else
	{
		BNode_s *psNode;
		int i = psTrans->bt_nLockedNodeCnt;

		psNode = bt_malloc_node();

		if( psNode == NULL )
		{
			printk( "Error: bt_load_node() failed to alloc memory for node\n" );
			return( NULL_VAL );
		}

		psTrans->bt_nLockedNodeCnt++;
		psTrans->bt_apsLockedNodes[i].be_psNode = psNode;
		psTrans->bt_apsLockedNodes[i].be_nValue = psTree->bt_nLastNode;
		psTrans->bt_apsLockedNodes[i].be_bDirty = true;
		psTree->bt_nLastNode += B_NODE_SIZE;

		nResult = psTrans->bt_apsLockedNodes[i].be_nValue;

		bt_mark_node_dirty( psTrans, 0 );	// Mark the tree header as dirty
		g_nTotNodeCount++;
	}
	return( nResult );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

status_t bt_free_node( AfsVolume_s * psVolume, AfsInode_s * psInode, BTransaction_s * psTrans, bvalue_t nNode )
{
	BTree_s *psTree;
	BNode_s *psNode;

#ifdef AFS_PARANOIA
	if( psVolume == NULL )
	{
		panic( "bt_free_node() called with psVolume == NULL\n" );
		return( -EINVAL );
	}
	if( psInode == NULL )
	{
		panic( "bt_free_node() called with psInode == NULL\n" );
		return( -EINVAL );
	}
	if( psTrans == NULL )
	{
		panic( "bt_free_node() called with psTrans == NULL\n" );
		return( -EINVAL );
	}
	if( nNode == NULL_VAL )
	{
		panic( "bt_free_node() called with nNode == NULL_VAL\n" );
		return( -EINVAL );
	}
#endif

	psTree =( BTree_s * ) bt_load_node( psVolume, psInode, psTrans, 0 );

	if( psTree == NULL )
	{
		printk( "Error: bt_free_node() failed to load tree header!\n" );
		return( -EIO );
	}

	if( bt_validate_tree_header( psTree ) == false )
	{
		panic( "bt_end_transaction() invalid three header!\n" );
		return( -EINVAL );
	}

	psNode = bt_load_node( psVolume, psInode, psTrans, nNode );

	if( psNode == NULL )
	{
		printk( "Error: bt_free_node() failed to load node!\n" );
		return( -EIO );
	}

	bt_assert_valid_node( psNode );

	kassertw(( ( int * )( ( ( char * )psNode ) + B_NODE_SIZE ) )[0] = 0x12345678 );

	if( nNode >= psTree->bt_nLastNode )
	{
		panic( "bt_free_node() node lives outside the valid range\n" );
		return( -EINVAL );
	}

	psNode->bn_nKeyCount = 0x12345678;
	psNode->bn_nRight = psTree->bt_nFirstFree;
	psTree->bt_nFirstFree = nNode;

	bt_mark_node_dirty( psTrans, nNode );
	bt_mark_node_dirty( psTrans, 0 );

	g_nTotNodeCount--;
	return( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

/*
static void bt_get_min_max( BNode_s* psRoot, int* pnMin, int* pnMax )
{
  int16*		pnKeyIndexes = B_KEY_INDEX_OFFSET( psRoot );
  BNode_s**	apsValues		 = B_KEY_VALUE_OFFSET( psRoot );
  char*			pKey			= psRoot->bn_anKeyData;
  int				i;
	
  for( i = 0 ; i < psRoot->bn_nKeyCount ; ++i )
  {
    int nCurKeyLen = pnKeyIndexes[i] -((i>0) ? pnKeyIndexes[i-1] : 0);

    if( *((int*)pKey) < *pnMin ) {
      *pnMin = *((int*)pKey);
    }
    if( *((int*)pKey) > *pnMax ) {
      *pnMax = *((int*)pKey);
    }
		
    if( NULL != apsValues[i] )
    {
      bt_get_min_max( apsValues[i], pnMin, pnMax );
    }
    pKey += nCurKeyLen;
  }
  if( NULL != psRoot->bn_psOverflow ) {
    bt_get_min_max( psRoot->bn_psOverflow, pnMin, pnMax );
  }
}
*/

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

/*
void	bt_validate_tree( BNode_s* psRoot )
{
  int16*    pnKeyIndexes = B_KEY_INDEX_OFFSET( psRoot );
  BNode_s** apsValues	 = B_KEY_VALUE_OFFSET( psRoot );
  char*	    pKey	 = psRoot->bn_anKeyData;
  char*	    pPrevKey	 = NULL;
  int	    nMin;
  int	    nMax;
  int	    i;

  for( i = 0 ; i < psRoot->bn_nKeyCount ; ++i )
  {
    int nCurKeyLen = pnKeyIndexes[i] -((i>0) ? pnKeyIndexes[i-1] : 0);
		
    if( NULL != apsValues[i] )
    {
      nMin = INT_MAX;
      nMax = 0;
			
      bt_get_min_max( apsValues[i], &nMin, &nMax );

      if( nMax >= *((int*)pKey) ) {
	printk( "Found value(%d) larger than LE key %d at index %d\n", nMax, *((int*)pKey), i );
      }
      if( i > 0 )
      {
	if( nMin < *((int*)pPrevKey) ) {
	  printk( "Found value(%d) lesser than previous GE key %d at index %d\n", nMin, *((int*)pPrevKey), i );
	}
      }
    }
    pPrevKey = pKey;
    pKey += nCurKeyLen;
  }
  if( NULL != psRoot->bn_psOverflow )
  {
    nMin = INT_MAX;
    nMax = 0;
		
    bt_get_min_max( psRoot->bn_psOverflow, &nMin, &nMax );
    if( i > 0 )
    {
      if( nMin < *((int*)pPrevKey) ) {
	printk( "Found value(%d) lesser than previous GE key %d at overflow ptr\n", nMin, *((int*)pPrevKey) );
      }
    }
  }
}
*/

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/


#ifdef _BT_DEBUG
static void bt_print_node( int nKeyType, const BNode_s *psNode )
{
	int i;
	void *pPrevKey = NULL;

	if( NULL_VAL != psNode->bn_nLeft )
	{
		printk( "<" );
	}
	else
	{
		printk( "[" );
	}
//      if( psNode->bn_nKeyCount < 50 )
	{
		for( i = 0; i < psNode->bn_nKeyCount; ++i )
		{
			int nKeySize;
			const void *pKey;

			bt_get_key( psNode, i, &pKey, &nKeySize );

			kassertw( pPrevKey < pKey );

			switch( nKeyType )
			{
			case e_KeyTypeInt32:
				printk( "%d", *(( int * )pKey ) );
				break;
			case e_KeyTypeString:
				{
					char zBuffer[258];

					memcpy( zBuffer, pKey, nKeySize );
					zBuffer[nKeySize] = '\0';
					printk( zBuffer );
					break;
				}
			default:
				printk( "<<unknown_type>>" );
				break;
			}
//                      printk( "%d(%d)", *((int*)pKey), apsValues[i] );
			if( i != psNode->bn_nKeyCount - 1 )
			{
				printk( " " );
			}
		}
	}
	if( NULL_VAL != psNode->bn_nOverflow )
	{
		printk( " *" );
	}
	if( NULL_VAL != psNode->bn_nRight )
	{
		printk( ">" );
	}
	else
	{
		printk( "]" );
	}
}
#endif // _BT_DEBUG

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

bool bt_will_key_fit( const BNode_s *psNode, int nKeySize )
{
	int nNewSize;

#ifdef AFS_PARANOIA
	if( psNode == NULL )
	{
		panic( "bt_will_key_fit() called with psNode == NULL\n" );
		return( false );
	}
	if( nKeySize <= 0 || nKeySize > B_MAX_KEY_SIZE )
	{
		panic( "bt_will_key_fit() called with nKeySize == %d\n", nKeySize );
		return( false );
	}
#endif
	nNewSize =( psNode->bn_nTotKeySize + nKeySize + 3 ) & ~3;

	nNewSize +=( psNode->bn_nKeyCount + 1 ) * ( B_SIZEOF_VALUE + B_SIZEOF_KEYOFFSET );
	nNewSize += B_HEADER_SIZE;
	return( nNewSize <= B_NODE_SIZE );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int bt_cmp_keys( int nType, const void *pKey1, int nKey1Size, const void *pKey2, int nKey2Size )
{
	switch( nType )
	{
	case e_KeyTypeString:
		{
			const char *pzKey1 = pKey1;
			const char *pzKey2 = pKey2;
			const int nMinLen = min( nKey1Size, nKey2Size );
			int nRes = 0;
			int i;

			for( i = 0; i < nMinLen; ++i )
			{
				if( ( nRes = ( ( uint )pzKey1[i] ) - ( ( uint )pzKey2[i] ) ) != 0 )
					break;
			}
			if( 0 == nRes && i == nMinLen )
			{
				nRes = nKey1Size - nKey2Size;
			}
			return( nRes );
		}
	case e_KeyTypeInt32:
		return( *( ( int32 * )pKey1 ) - *( ( int32 * )pKey2 ) );
	case e_KeyTypeInt64:
		return( *( ( int64 * )pKey1 ) - *( ( int64 * )pKey2 ) );
	case e_KeyTypeFloat:
		{
			float vRes = *(( float * )pKey1 ) - *( ( float * )pKey2 );

			if( vRes == 0.0f )
			{
				return( 0 );
			}
			else
			{
				return( ( vRes < 0.0f ) ? -1 : 1 );
			}
		}
	case e_KeyTypeDouble:
		{
			double vRes = *(( double * )pKey1 ) - *( ( double * )pKey2 );

			if( vRes == 0.0 )
			{
				return( 0 );
			}
			else
			{
				return( ( vRes < 0.0 ) ? -1 : 1 );
			}
		}
	default:
		printk( "Error : bt_cmp_keys() called with invalid type %d\n", nType );
		return( 0 );
	}
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

status_t bt_append_key( BNode_s *psNode, const void *pKey, int nKeySize, bvalue_t nValue )
{
	int nExtraSize;
	int16 *pnKeyIndexes;
	int i;

#ifdef AFS_PARANOIA
	if( psNode == NULL )
	{
		panic( "bt_append_key() called with psNode == NULL\n" );
		return( -EINVAL );
	}
	if( pKey == NULL )
	{
		panic( "bt_append_key() called with pKey == NULL\n" );
		return( -EINVAL );
	}
	if( nKeySize <= 0 || nKeySize > B_MAX_KEY_SIZE )
	{
		panic( "bt_append_key() called with invalid key size %d\n", nKeySize );
		return( -EINVAL );
	}
#endif
	nExtraSize =( ( psNode->bn_nTotKeySize + nKeySize + 3 ) & ~3 ) - ( ( psNode->bn_nTotKeySize + 3 ) & ~3 );
	pnKeyIndexes = B_KEY_INDEX_OFFSET( psNode );


	if( bt_will_key_fit( psNode, nKeySize ) == false )
	{
		panic( "bt_append_key() key won't fit in node(%d)\n", nKeySize );
		return( -EINVAL );
	}

	memmove(( ( char * )B_KEY_VALUE_OFFSET( psNode ) ) + nExtraSize + B_SIZEOF_KEYOFFSET, B_KEY_VALUE_OFFSET( psNode ), psNode->bn_nKeyCount * B_SIZEOF_VALUE );
	memmove(( ( char * )B_KEY_INDEX_OFFSET( psNode ) ) + nExtraSize, B_KEY_INDEX_OFFSET( psNode ), psNode->bn_nKeyCount * B_SIZEOF_KEYOFFSET );
	memcpy( psNode->bn_anKeyData + psNode->bn_nTotKeySize, pKey, nKeySize );

	psNode->bn_nTotKeySize += nKeySize;
	psNode->bn_nKeyCount++;

	B_KEY_INDEX_OFFSET( psNode )[psNode->bn_nKeyCount - 1] = psNode->bn_nTotKeySize;
	B_KEY_VALUE_OFFSET( psNode )[psNode->bn_nKeyCount - 1] = nValue;

	if( psNode->bn_nKeyCount > 1 )
	{
		kassertw( B_KEY_INDEX_OFFSET( psNode )[psNode->bn_nKeyCount - 1] > B_KEY_INDEX_OFFSET( psNode )[psNode->bn_nKeyCount - 2] );
	}

	pnKeyIndexes = B_KEY_INDEX_OFFSET( psNode );

	for( i = 1; i < psNode->bn_nKeyCount; ++i )
	{
		kassertw( pnKeyIndexes[i] > pnKeyIndexes[i - 1] );
	}
	return( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 *	Same as bt_append_key() but will insert the key in a sorted order.
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

status_t bt_insert_key_to_node( AfsInode_s * psInode, BNode_s *psNode, const void *a_pKey, int a_nKeySize, bvalue_t nValue )
{
	int nExtraSize;
	int16 *pnKeyIndexes;
	bvalue_t *apsValues;
	const void *pKey;
	int nKeySize;
	int nKeyOffset = 0;
	bool bInserted = false;
	int i;

#ifdef AFS_PARANOIA
	if( psInode == NULL )
	{
		panic( "bt_insert_key_to_node() called with psInode == NULL\n" );
		return( -EINVAL );
	}
	if( psNode == NULL )
	{
		panic( "bt_insert_key_to_node() called with psNode == NULL\n" );
		return( -EINVAL );
	}
	if( a_pKey == NULL )
	{
		panic( "bt_insert_key_to_node() called with a_pKey == NULL\n" );
		return( -EINVAL );
	}
	if( a_nKeySize <= 0 || a_nKeySize > B_MAX_KEY_SIZE )
	{
		panic( "bt_insert_key_to_node() called with invalid key-size %d\n", a_nKeySize );
		return( -EINVAL );
	}
#endif
	nExtraSize =( ( psNode->bn_nTotKeySize + a_nKeySize + 3 ) & ~3 ) - ( ( psNode->bn_nTotKeySize + 3 ) & ~3 );
	pnKeyIndexes = B_KEY_INDEX_OFFSET( psNode );
	apsValues = B_KEY_VALUE_OFFSET( psNode );

	if( bt_will_key_fit( psNode, a_nKeySize ) == false )
	{
		panic( "bt_insert_key_to_node() key won't fit in the node\n" );
		return( -EINVAL );
	}
	if( psNode->bn_nKeyCount == 0 )
	{
		return( bt_append_key( psNode, a_pKey, a_nKeySize, nValue ) );
	}

	// Just append if key is equal or greater than last entry in node

	bt_get_key( psNode, psNode->bn_nKeyCount - 1, &pKey, &nKeySize );
	if( bt_cmp_keys( psInode->ai_nIndexType, a_pKey, a_nKeySize, pKey, nKeySize ) >= 0 )
	{
		return( bt_append_key( psNode, a_pKey, a_nKeySize, nValue ) );
	}

	bt_assert_valid_node( psNode );

	// Move key index array, and values array to make place for the key
	memmove(( ( char * )apsValues ) + nExtraSize + B_SIZEOF_KEYOFFSET, apsValues, psNode->bn_nKeyCount * B_SIZEOF_VALUE );
	memmove(( ( char * )pnKeyIndexes ) + nExtraSize, pnKeyIndexes, psNode->bn_nKeyCount * B_SIZEOF_KEYOFFSET );

	pnKeyIndexes =( int16 * )( ( ( char * )pnKeyIndexes ) + nExtraSize );
	apsValues =( bvalue_t * )( ( ( char * )apsValues ) + nExtraSize + 2 );

	for( i = 0; i <= psNode->bn_nKeyCount; ++i )
	{
		if( i > 0 )
		{
			nKeySize = pnKeyIndexes[i] - pnKeyIndexes[i - 1];
		}
		else
		{
			nKeySize = pnKeyIndexes[i];
		}
		if( bInserted == false )
		{
			if( bt_cmp_keys( psInode->ai_nIndexType, a_pKey, a_nKeySize, &psNode->bn_anKeyData[nKeyOffset], nKeySize ) < 0 )
			{
				memmove( &apsValues[i + 1], &apsValues[i], B_SIZEOF_VALUE *( psNode->bn_nKeyCount - i ) );
				memmove( &pnKeyIndexes[i + 1], &pnKeyIndexes[i], B_SIZEOF_KEYOFFSET *( psNode->bn_nKeyCount - i ) );
				memmove( &psNode->bn_anKeyData[nKeyOffset + a_nKeySize], &psNode->bn_anKeyData[nKeyOffset], psNode->bn_nTotKeySize - nKeyOffset );

				memcpy( &psNode->bn_anKeyData[nKeyOffset], a_pKey, a_nKeySize );
				apsValues[i] = nValue;
				pnKeyIndexes[i] = nKeyOffset + a_nKeySize;

				if( i > 1 )
				{
					kassertw( pnKeyIndexes[i] > pnKeyIndexes[i - 1] );
				}

				bInserted = true;
			}
		}
		else
		{
			pnKeyIndexes[i] += a_nKeySize;	// Adjust the remainding key indices
		}
		nKeyOffset += nKeySize;
	}
	psNode->bn_nTotKeySize += a_nKeySize;
	psNode->bn_nKeyCount++;

	for( i = 1; i < psNode->bn_nKeyCount; ++i )
	{
		kassertw( pnKeyIndexes[i] > pnKeyIndexes[i - 1] );
	}
	return( 0 );
}


/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/
#if _BT_DEBUG
static void bt_test_node()
{
	BNode_s *psNode = bt_alloc_node();
	int nKey;
	int i;


	printk( "Header size = %d\n", B_HEADER_SIZE );
	for( i = 0; i < 4; ++i )
	{
		printk( "Will fit = %d\n", bt_will_key_fit( psNode, 5 ) );
		nKey = i * 2;
//              nKey = random() & 0xffff;
//              bt_append_key( psNode, &nKey, 4, NULL );
		bt_insert_key_to_node( psNode, &nKey, 4,( BNode_s * )( i * 100 ) );
		bt_print_node( psNode );
		printk( "\n" );
	}

	nKey = 5;
	bt_replace_key( psNode, 2, &nKey, 5 );
	bt_print_node( psNode );
	printk( "\n" );

	bt_delete_key_from_node( psNode, 0 );
	bt_print_node( psNode );
	printk( "\n" );

	bt_delete_key_from_node( psNode, 0 );
	bt_print_node( psNode );
	printk( "\n" );

	bt_delete_key_from_node( psNode, 0 );
	bt_print_node( psNode );
	printk( "\n" );

	bt_delete_key_from_node( psNode, 0 );
	bt_print_node( psNode );
	printk( "\n" );


/*
  nKey = 30;
  bt_append_key( psNode, &nKey, 4, NULL );
	
  nKey = 40;
  bt_append_key( psNode, &nKey, 4, NULL );
	
  bt_print_node( psNode );
  printk( "\n" );
  nKey = 50;
  bt_insert_key_to_node( psNode, &nKey, 4, NULL );
	
  nKey = 4;
//	bt_append_key( psNode, &nKey, 4, NULL );
bt_print_node( psNode );
printk( "\n" );
*/
	bt_free_node( psTree, psNode );
}
#endif

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int bt_find_key( AfsVolume_s * psVolume, AfsInode_s * psInode, BTransaction_s * psTrans, BStack_s * psStack, const void *pKey, int nKeySize, BNode_s **ppsNode )
{
	int bFound = 0;
	BTree_s *psTree;
	bvalue_t nRoot;
	BNode_s *psRoot;
	int i;

#ifdef AFS_PARANOIA
	if( psVolume == NULL )
	{
		panic( "bt_find_key() called with psVolume == NULL\n" );
		return( -EINVAL );
	}
	if( psInode == NULL )
	{
		panic( "bt_find_key() called with psInode == NULL\n" );
		return( -EINVAL );
	}
	if( psTrans == NULL )
	{
		panic( "bt_find_key() called with psTrans == NULL\n" );
		return( -EINVAL );
	}
	if( psStack == NULL )
	{
		panic( "bt_find_key() called with psStack == NULL\n" );
		return( -EINVAL );
	}
	if( pKey == NULL )
	{
		panic( "bt_find_key() called with pKey == NULL\n" );
		return( -EINVAL );
	}
	if( nKeySize <= 0 || nKeySize > B_MAX_KEY_SIZE )
	{
		panic( "bt_find_key() called with invalid key-size %d\n", nKeySize );
		return( -EINVAL );
	}
#endif
	psTree =( BTree_s * ) bt_load_node( psVolume, psInode, psTrans, 0 );

	if( psTree == NULL )
	{
		printk( "Error: bt_find_key() failed to load tree header\n" );
		return( -EIO );
	}

	if( bt_validate_tree_header( psTree ) == false )
	{
		panic( "bt_find_key() invalid three header!\n" );
		return( -EIO );
	}

	nRoot = psTree->bt_nRoot;
	psRoot = bt_load_node( psVolume, psInode, psTrans, nRoot );

	if( psRoot == NULL )
	{
		printk( "Error: bt_find_key() failed to load root-node\n" );
		return( -EIO );
	}

	bt_assert_valid_node( psRoot );

	if( 0 == psRoot->bn_nKeyCount && NULL_VAL == psRoot->bn_nOverflow )
	{
		psStack->bn_apnStack[psStack->bn_nCurPos] = nRoot;
		psStack->bn_apsNodes[psStack->bn_nCurPos] = psRoot;
		psStack->bn_anPosition[psStack->bn_nCurPos] = 0;

		if( NULL != ppsNode )
		{
			*ppsNode = psRoot;
		}
		return( 0 );
	}
	i = 0;
	for( ;; )
	{
		int16 *pnKeyIndexes = B_KEY_INDEX_OFFSET( psRoot );
		bvalue_t *apsValues = B_KEY_VALUE_OFFSET( psRoot );
		int nKeyOffset = 0;
		bool bIsLeaf = psStack->bn_nCurPos == psTree->bt_nTreeDepth - 1;

		bFound = 0;

		for( i = 0; i <= psRoot->bn_nKeyCount; ++i )
		{
			int nCurKeySize = pnKeyIndexes[i] -( ( i > 0 ) ? pnKeyIndexes[i - 1] : 0 );
			int nCmpRes = 0;

			if( i < psRoot->bn_nKeyCount )
			{
				nCmpRes = bt_cmp_keys( psInode->ai_nIndexType, pKey, nKeySize, &psRoot->bn_anKeyData[nKeyOffset], nCurKeySize );
				bFound = bFound || nCmpRes == 0;	/* Check if we got a direct hit */
			}
			if( i == psRoot->bn_nKeyCount || nCmpRes < 0 || ( bIsLeaf && nCmpRes == 0 ) )
			{
				if( false == bIsLeaf )
				{	// Check if we got to a leaf node
					psStack->bn_apnStack[psStack->bn_nCurPos] = nRoot;
					psStack->bn_apsNodes[psStack->bn_nCurPos] = psRoot;
					psStack->bn_anPosition[psStack->bn_nCurPos] = i;
					psStack->bn_nCurPos++;

					if( i < psRoot->bn_nKeyCount )
					{
						nRoot = apsValues[i];
					}
					else
					{
						nRoot = psRoot->bn_nOverflow;
					}
					psRoot = bt_load_node( psVolume, psInode, psTrans, nRoot );
					if( psRoot == NULL )
					{
						printk( "Error: bt_find_key() failed to load node\n" );
						return( -EIO );
					}
					bt_assert_valid_node( psRoot );
					i = 0;
					break;
				}
				else
				{
					psStack->bn_apnStack[psStack->bn_nCurPos] = nRoot;
					psStack->bn_apsNodes[psStack->bn_nCurPos] = psRoot;
					psStack->bn_anPosition[psStack->bn_nCurPos] = i;
					if( NULL != ppsNode )
					{
						*ppsNode = psRoot;
					}
					return( bFound );
				}
			}
			nKeyOffset += nCurKeySize;
		}
	}
	kassertw( 0 );
	return( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static bool bt_merge_node( AfsInode_s * psInode, BNode_s *psDstNode, const BNode_s *psSrcNode, int *pnStartIndex, int *pnStartOffset, int nMaxSize, const void *pKey, int nKeyLen, bvalue_t psValue, bool bInserted )
{
	int nKeyOffset;
	const int16 *pnKeyIndexes;
	const bvalue_t *apsValues;
	int i;

#ifdef AFS_PARANOIA
	if( psInode == NULL )
	{
		panic( "bt_merge_node() called with psInode == NULL\n" );
		return( false );
	}
	if( psDstNode == NULL )
	{
		panic( "bt_merge_node() called with psDstNode == NULL\n" );
		return( false );
	}
	if( psSrcNode == NULL )
	{
		panic( "bt_merge_node() called with psSrcNode == NULL\n" );
		return( false );
	}
	if( pnStartIndex == NULL )
	{
		panic( "bt_merge_node() called with pnStartIndex == NULL\n" );
		return( false );
	}
	if( pnStartOffset == NULL )
	{
		panic( "bt_merge_node() called with pnStartOffset == NULL\n" );
		return( false );
	}
	if( psValue == NULL_VAL )
	{
		panic( "bt_merge_node() called with psValue == NULL_VAL\n" );
		return( false );
	}
#endif

	nKeyOffset = *pnStartOffset;
	pnKeyIndexes = B_KEY_INDEX_OFFSET_CONST( psSrcNode );
	apsValues = B_KEY_VALUE_OFFSET_CONST( psSrcNode );

	if( psSrcNode->bn_nKeyCount < 2 )
	{
		panic( "bt_merge_node() source node has to few keys: %d\n", psSrcNode->bn_nKeyCount );
		return( false );
	}

	for( i = *pnStartIndex; i < psSrcNode->bn_nKeyCount && B_TOT_KEY_SIZE( psDstNode ) < nMaxSize; ++i )
	{
		int nCurKeySize = pnKeyIndexes[i] -( ( i > 0 ) ? pnKeyIndexes[i - 1] : 0 );

		if( false == bInserted && bt_cmp_keys( psInode->ai_nIndexType, pKey, nKeyLen, &psSrcNode->bn_anKeyData[nKeyOffset], nCurKeySize ) < 0 )
		{
			if( nKeyLen <= 0 || nKeyLen > B_MAX_KEY_SIZE )
			{
				panic( "bt_merge_node() invalid key size %d\n", nKeyLen );
				break;
			}

			bt_assert_valid_node( psDstNode );
			if( bt_will_key_fit( psDstNode, nKeyLen ) == false )
			{
				panic( "bt_merge_node:1() key won't fit: %d\n", nKeyLen );
				break;
			}
			bt_append_key( psDstNode, pKey, nKeyLen, psValue );
			bt_assert_valid_node( psDstNode );
			bInserted = true;
		}
		bt_assert_valid_node( psDstNode );

		if( bt_will_key_fit( psDstNode, nCurKeySize ) == false )
		{
			break;
		}
		if( bt_will_key_fit( psDstNode, nCurKeySize ) == false )
		{
			panic( "bt_merge_node:2() key won't fit: %d\n", nCurKeySize );
			break;
		}

		bt_append_key( psDstNode, &psSrcNode->bn_anKeyData[nKeyOffset], nCurKeySize, apsValues[i] );
		bt_assert_valid_node( psDstNode );

		nKeyOffset += nCurKeySize;

		if( nMaxSize < B_NODE_SIZE && ( psSrcNode->bn_nKeyCount - i ) < 4 )
		{
			i++;
			break;
		}
	}
	*pnStartIndex = i;
	*pnStartOffset = nKeyOffset;
	return( bInserted );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

status_t bt_add_key_to_node( AfsVolume_s * psVolume, AfsInode_s * psInode, BTransaction_s * psTrans, BStack_s * psStack, BNode_s *psNode, bvalue_t nNode, const void *pKey, int nKeyLen, bvalue_t psValue )
{
	int nStackLevel;
	status_t nError = 0;

#ifdef AFS_PARANOIA
	if( psVolume == NULL )
	{
		panic( "bt_add_key_to_node() called with psVolume == NULL\n" );
		return( -EINVAL );
	}
	if( psInode == NULL )
	{
		panic( "bt_add_key_to_node() called with psInode == NULL\n" );
		return( -EINVAL );
	}
	if( psTrans == NULL )
	{
		panic( "bt_add_key_to_node() called with psTrans == NULL\n" );
		return( -EINVAL );
	}
	if( psStack == NULL )
	{
		panic( "bt_add_key_to_node() called with psStack == NULL\n" );
		return( -EINVAL );
	}
	if( psNode == NULL )
	{
		panic( "bt_add_key_to_node() called with psNode == NULL\n" );
		return( -EINVAL );
	}
	if( nNode == NULL_VAL )
	{
		panic( "bt_add_key_to_node() called with nNode == NULL_VAL\n" );
		return( -EINVAL );
	}
	if( pKey == NULL )
	{
		panic( "bt_add_key_to_node() called with pKey == NULL\n" );
		return( -EINVAL );
	}
	if( nKeyLen <= 0 || nKeyLen > B_MAX_KEY_SIZE )
	{
		panic( "bt_add_key_to_node() called with invalid key-length %d\n", nKeyLen );
		return( -EINVAL );
	}
#endif
	nStackLevel = psStack->bn_nCurPos;

	for( ;; )
	{
		if( bt_will_key_fit( psNode, nKeyLen ) )
		{
			nError = bt_insert_key_to_node( psInode, psNode, pKey, nKeyLen, psValue );
			if( nError >= 0 )
			{
				bt_assert_valid_node( psNode );
				bt_mark_node_dirty( psTrans, nNode );
			}
			break;
		}
		else
		{
			BNode_s *psNewNode1;
			bvalue_t nNewNode1 = nNode;
			bvalue_t nNewNode2;
			BNode_s *psNewNode2;
			int16 *pnKeyIndexes = B_KEY_INDEX_OFFSET( psNode );
			bvalue_t *apsValues = B_KEY_VALUE_OFFSET( psNode );
			bool bInserted = false;
			char anParentKey[256];
			const void *pParentKey;
			int nParentKeyLen;
			int nKeyOffset = 0;
			int i;

			psNewNode1 = bt_alloc_tmp_node();
			if( psNewNode1 == NULL )
			{
				nError = -ENOMEM;
				break;
			}
			nNewNode2 = bt_alloc_node( psVolume, psInode, psTrans );
			if( nNewNode2 == NULL_VAL )
			{
				bt_free_tmp_node( psNewNode1 );
				nError = -ENOMEM;
				break;
			}
			psNewNode2 = bt_load_node( psVolume, psInode, psTrans, nNewNode2 );
			if( psNewNode2 == NULL )
			{
				bt_free_tmp_node( psNewNode1 );
				nError = -ENOMEM;
				break;
			}

			bt_assert_valid_node( psNewNode2 );

			// Move first half of psNode into psNewNode1
			// Might also insert the key, if it's less than the greatest key moved into psNewNode1

			i = 0;
			bInserted = bt_merge_node( psInode, psNewNode1, psNode, &i, &nKeyOffset,( ( B_NODE_SIZE - B_HEADER_SIZE ) / 2 ), pKey, nKeyLen, psValue, bInserted );



			nParentKeyLen = pnKeyIndexes[i] -( ( i > 0 ) ? pnKeyIndexes[i - 1] : 0 );

			if( nParentKeyLen <= 0 || nParentKeyLen > B_MAX_KEY_SIZE )
			{
				panic( "bt_add_key_to_node:1() Parent key has invalid size: %d\n", nParentKeyLen );
				return( -EINVAL );
			}

			// If the key is lesser than the next key to be fetched from psNode the key will be associated
			// with the pointer to the new node from our parent.
			// If the key is already swallowed by psNewNode1, or it is greater than the next available
			// key it will be inserted into psNewNode2, and the next available key from psNode will be used
			// in our parent.

			if( false == bInserted && bt_cmp_keys( psInode->ai_nIndexType, pKey, nKeyLen, &psNode->bn_anKeyData[nKeyOffset], nParentKeyLen ) < 0 )
			{
				pParentKey = pKey;
				nParentKeyLen = nKeyLen;
				psNewNode1->bn_nOverflow = psValue;
				bInserted = true;
			}
			else
			{
				pParentKey = &psNode->bn_anKeyData[nKeyOffset];
				psNewNode1->bn_nOverflow = apsValues[i];

//                              printk( "Parent key = %d\n", *((int*)pParentKey) );
				if( nParentKeyLen <= 0 || nParentKeyLen > B_MAX_KEY_SIZE )
				{
					panic( "bt_add_key_to_node:1() Parent key has invalid size: %d\n", nParentKeyLen );
					return( -EINVAL );
				}

				nKeyOffset += nParentKeyLen;
				++i;
			}
			// Copy the last half of psNode, and(if not already used) the new key into psNewNode2

			bInserted = bt_merge_node( psInode, psNewNode2, psNode, &i, &nKeyOffset, B_NODE_SIZE * 2, pKey, nKeyLen, psValue, bInserted );
			// If the key surwived all the previous attach it must be greater than the last key
			// found in psNode, so we just append it to psNewNode2.


			if( false == bInserted )
			{
				if( bt_will_key_fit( psNewNode2, nKeyLen ) == false )
				{
					panic( "bt_add_key_to_node() Key won't fit: %d\n", nKeyLen );
					return( -EINVAL );
				}
				bt_append_key( psNewNode2, pKey, nKeyLen, psValue );
			}
			psNewNode2->bn_nOverflow = psNode->bn_nOverflow;

			// pParentKey may point into psNode, so we must make a backup before we
			// change psNode. Also note that pKey may point to anParentKey, so we
			// can't overwrite it until we are finished using pKey.

			memcpy( anParentKey, pParentKey, nParentKeyLen );

			memcpy( psNode, psNewNode1, B_NODE_SIZE );
			bt_free_tmp_node( psNewNode1 );
			psNewNode1 = psNode;

			// nNewNode2 is market dirty by bt_alloc_node() so we skip it here.
			bt_mark_node_dirty( psTrans, nNewNode1 );

			kassertw( psNewNode1->bn_nKeyCount > 0 );
			kassertw( psNewNode2->bn_nKeyCount > 0 );

			if( nStackLevel > 0 )
			{
				BNode_s *psParent;
				bvalue_t nParent;

				nStackLevel--;

				// The largest key in psNewNode2 is still larger than the GE key pointing
				// to psNode from our parent, so we replace the parents pointer to psNode
				// with a pointer to psNewNode2. Then we run the loop again to insert
				// psNewNode1 int our parent.

				psParent = psStack->bn_apsNodes[nStackLevel];
				nParent = psStack->bn_apnStack[nStackLevel];

				apsValues = B_KEY_VALUE_OFFSET( psParent );

				if( psParent->bn_nOverflow == nNode )
				{
					psParent->bn_nOverflow = nNewNode2;
				}
				else
				{
					for( i = 0; i < psParent->bn_nKeyCount; ++i )
					{
						if( apsValues[i] == nNode )
						{
							apsValues[i] = nNewNode2;
							break;
						}
					}
					kassertw( i < psParent->bn_nKeyCount );
				}
				// The parent node will be marked dirty during
				// the next iteration so we skip it here.
				pKey = anParentKey;
				nKeyLen = nParentKeyLen;
				psValue = nNewNode1;
				psNode = psParent;
				nNode = nParent;

				continue;
			}
			else
			{
				BTree_s *psTree;
				bvalue_t nNewParent;
				BNode_s *psNewParent;

				psTree =( BTree_s * ) bt_load_node( psVolume, psInode, psTrans, 0 );

				if( psTree == NULL )
				{
					printk( "bt_add_key_to_node() failed to load tree header\n" );
					nError = -ENOMEM;
					break;
				}

				if( bt_validate_tree_header( psTree ) == false )
				{
					panic( "bt_add_key_to_node() invalid three header!\n" );
					nError = -EINVAL;
					break;
				}

				nNewParent = bt_alloc_node( psVolume, psInode, psTrans );	// Will mark the node dirty.

				if( nNewParent == NULL_VAL )
				{
					printk( "Error: bt_add_key_to_node() failed to alloc new root\n" );
					nError = -ENOMEM;
					break;
				}
				psNewParent = bt_load_node( psVolume, psInode, psTrans, nNewParent );
				if( psNewParent == NULL )
				{
					printk( "Error: bt_add_key_to_node() failed to load newly allocated root node\n" );
					nError = -EIO;
					break;
				}
				bt_assert_valid_node( psNewParent );
				psTree->bt_nRoot = nNewParent;
				psTree->bt_nTreeDepth++;
				kassertw( bt_will_key_fit( psNewParent, nParentKeyLen ) );
				nError = bt_append_key( psNewParent, anParentKey, nParentKeyLen, nNewNode1 );
				if( nError < 0 )
				{
					printk( "Error: bt_add_key_to_node() failed to append node to new root\n" );
					break;
				}
				bt_assert_valid_node( psNewParent );

				psNewParent->bn_nOverflow = nNewNode2;
				bt_mark_node_dirty( psTrans, 0 );
				break;
			}
		}
	}
	return( nError );
}


#ifdef _BT_DEBUG

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int _bt_print_tree( AfsVolume_s * psVolume, AfsInode_s * psInode, BStack_s * psStack, const BNode_s *psRoot, int nLevel )
{
	int i;

//      printk( "Print tree %p\n", psRoot );

	if( NULL == psRoot )
	{
		return( 0 );
	}

	if( 0 == nLevel )
	{
		bt_print_node( psInode->ai_nIndexType, psRoot );
//              printk( " " );

		return( 0 != psRoot->bn_nKeyCount );
	}
	else
	{
		const bvalue_t *apsValues = B_KEY_VALUE_OFFSET_CONST( psRoot );
		int bDidPrint = 0;

		for( i = 0; i < psRoot->bn_nKeyCount; ++i )
		{
			if( NULL_VAL != apsValues[i] )
			{
				bDidPrint = _bt_print_tree( psVolume, psInode, psStack, bt_load_node( psVolume, psInode, psStack, apsValues[i] ), nLevel - 1 ) || bDidPrint;
				if( 1 == nLevel && i != psRoot->bn_nKeyCount - 1 )
				{
//                                      printk( "-" );
				}
			}
		}
		if( NULL_VAL != psRoot->bn_nOverflow )
		{
			bDidPrint = _bt_print_tree( psVolume, psInode, psStack, bt_load_node( psVolume, psInode, psStack, psRoot->bn_nOverflow ), nLevel - 1 ) || bDidPrint;
			printk( "-" );
		}
		return( bDidPrint );
	}
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static void bt_print_tree( AfsVolume_s * psVolume, AfsInode_s * psInode )
{
	BStack_s sStack;
	BTree_s *psTree;
	int i = 0;

	bt_begin_transaction( &sStack );
	psTree =( BTree_s * ) bt_load_node( psVolume, psInode, &sStack, 0 );
	while( _bt_print_tree( psVolume, psInode, &sStack, bt_load_node( psVolume, psInode, &sStack, psTree->bt_nRoot ), i++ ) )
	{
		printk( "*\n" );
	}
	bt_end_transaction( psVolume, psInode, &sStack, false );
	printk( "\n" );
}

/*****************************************************************************
 * NAME:
 * DESC:
 *	Just for debugging(validate the regular lookup routine)
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int bt_find_linear( AfsVolume_s * psVolume, AfsInode_s * psInode, const void *pKey, int nKeySize, BIterator_s * psIterator )
{
	int nError;

	nError = bt_find_first_key( psVolume, psInode, psIterator );

	if( nError >= 0 )
	{
		do
		{
			char zBuffer[B_MAX_KEY_SIZE + 1];

			memcpy( zBuffer, psIterator->bi_anKey, psIterator->bi_nKeySize );
			zBuffer[psIterator->bi_nKeySize] = 0;
			if( bt_cmp_keys( psInode->ai_nIndexType, pKey, nKeySize, psIterator->bi_anKey, psIterator->bi_nKeySize ) == 0 )
			{
				return( 1 );
			}
		}
		while( bt_get_next_key( psVolume, psInode, psIterator ) == 0 );
	}
	else
	{
		printk( "Failed to find first key\n" );
	}
	return( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static int bt_list_tree( AfsVolume_s * psVolume, AfsInode_s * psInode )
{
	BIterator_s sIterator;
	int nError;

	nError = bt_find_first_key( psVolume, psInode, &sIterator );

	if( nError >= 0 )
	{
		int i = 0;
		int j = 0;

		do
		{
			char zBuffer[B_MAX_KEY_SIZE + 1];

			memcpy( zBuffer, sIterator.bi_anKey, sIterator.bi_nKeySize );
			zBuffer[sIterator.bi_nKeySize] = 0;
			printk( "Key %d::%d =  %s\n", j, i++, zBuffer );
		}
		while( bt_get_next_key( psVolume, psInode, &sIterator ) == 0 );
	}
	else
	{
		printk( "Failed to find first key\n" );
	}
	return( nError );
}
#endif // _BT_DEBUG


/******************************************************************************/

/*** The B+-TREE interface to the rest of the file-system *********************/

/******************************************************************************/



/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int bt_insert_key( AfsVolume_s * psVolume, AfsInode_s * psInode, const void *pKey, int nKeySize, bvalue_t nValue )
{
	BNode_s *psNode;
	bvalue_t nNode;
	BStack_s sStack;
	BStack_s *psStack = &sStack;
	BTransaction_s sTrans;
	BTransaction_s *psTrans = &sTrans;
	int nError;
	int nTmpErr;

#ifdef AFS_PARANOIA
	if( psVolume == NULL )
	{
		panic( "bt_insert_key() called with psVolume == NULL\n" );
		return( -EINVAL );
	}
	if( psInode == NULL )
	{
		panic( "bt_insert_key() called with psInode == NULL\n" );
		return( -EINVAL );
	}
	if( pKey == NULL )
	{
		panic( "bt_insert_key() called with pKey == NULL\n" );
		return( -EINVAL );
	}
	if( nKeySize <= 0 || nKeySize >= B_MAX_KEY_SIZE )
	{
		panic( "bt_insert_key() called with invalid key-size %d\n", nKeySize );
		return( -EINVAL );
	}
#endif
	memset( &sStack, 0, sizeof( sStack ) );

	nError = bt_begin_transaction( psStack, psTrans );
	if( nError < 0 )
	{
		return( nError );
	}

	if( psStack->bn_nCurPos < 0 )
	{
		panic( "bt_insert_key:1() invalid stack position: %d\n", psStack->bn_nCurPos );
		return( -EINVAL );
	}

	nError = bt_find_key( psVolume, psInode, psTrans, psStack, pKey, nKeySize, &psNode );
	if( nError < 0 )
	{
		printk( "bt_insert_key() failed to search for key\n" );
		goto error;
	}
	if( nError > 0 )
	{
		nError = -EEXIST;
		goto error;
	}
	kassertw( psNode == psStack->bn_apsNodes[psStack->bn_nCurPos] );
	nNode = psStack->bn_apnStack[psStack->bn_nCurPos];

	if( psStack->bn_nCurPos < 0 )
	{
		panic( "bt_insert_key:2() invalid stack position: %d\n", psStack->bn_nCurPos );
		nError = -EINVAL;
		goto error;
	}

	if( bt_will_key_fit( psNode, nKeySize ) )
	{
		bt_assert_valid_node( psNode );
		nError = bt_insert_key_to_node( psInode, psNode, pKey, nKeySize, nValue );
		if( nError >= 0 )
		{
			bt_assert_valid_node( psNode );
			bt_mark_node_dirty( psTrans, nNode );
		}
	}
	else
	{
		BNode_s *psNewNode1;
		bvalue_t nNewNode1 = nNode;
		BNode_s *psNewNode2;
		bvalue_t nNewNode2;
		int bInserted = false;
		int nKeyOffset = 0;
		int i;

		psNewNode1 = bt_alloc_tmp_node();

		if( psNewNode1 == NULL )
		{
			printk( "Error: bt_insert_key() faile to allocate temporary node buffer\n" );
			nError = -ENOMEM;
			goto error;
		}
		nNewNode2 = bt_alloc_node( psVolume, psInode, psTrans );
		if( nNewNode2 == NULL_VAL )
		{
			bt_free_tmp_node( psNewNode1 );
			printk( "Error: bt_insert_key() failed to alloc new node\n" );
			nError = -ENOMEM;
			goto error;
		}
		psNewNode2 = bt_load_node( psVolume, psInode, psTrans, nNewNode2 );
		if( psNewNode2 == NULL )
		{
			bt_free_tmp_node( psNewNode1 );
			printk( "Error: bt_insert_key() failed to load newly allocated node\n" );
			nError = -EIO;
			goto error;
		}
		bt_assert_valid_node( psNewNode2 );

		// Fix the linear linking of leaf nodes.
		psNewNode2->bn_nLeft = nNewNode1;
		psNewNode2->bn_nRight = psNode->bn_nRight;
		psNewNode1->bn_nRight = nNewNode2;
		psNewNode1->bn_nLeft = psNode->bn_nLeft;

		if( psNode->bn_nRight != NULL_VAL )
		{
			BNode_s *psTmp = bt_load_node( psVolume, psInode, psTrans, psNode->bn_nRight );

			if( psTmp == NULL )
			{
				bt_free_tmp_node( psNewNode1 );
				printk( "Error: bt_insert_key() failed to load tree node\n" );
				nError = -ENOMEM;
				goto error;
			}
			psTmp->bn_nLeft = nNewNode2;
			bt_mark_node_dirty( psTrans, psNode->bn_nRight );
		}

		// First half in psNewNode1

		i = 0;
		bInserted = bt_merge_node( psInode, psNewNode1, psNode, &i, &nKeyOffset,( B_NODE_SIZE - B_HEADER_SIZE ) / 2, pKey, nKeySize, nValue, bInserted );

		// Second half in psNewNode2
		bInserted = bt_merge_node( psInode, psNewNode2, psNode, &i, &nKeyOffset, B_NODE_SIZE * 2, pKey, nKeySize, nValue, bInserted );
		if( bInserted == false )
		{
			kassertw( bt_will_key_fit( psNewNode2, nKeySize ) );
			bt_append_key( psNewNode2, pKey, nKeySize, nValue );
		}

		bt_assert_valid_node( psNewNode1 );
		bt_assert_valid_node( psNewNode2 );

		memcpy( psNode, psNewNode1, B_NODE_SIZE );
		bt_free_tmp_node( psNewNode1 );

		psNewNode1 = psNode;

		bt_mark_node_dirty( psTrans, nNewNode1 );
		bt_mark_node_dirty( psTrans, nNewNode2 );

		if( psStack->bn_nCurPos < 0 )
		{
			panic( "bt_insert_key:3() invalid stack position: %d\n", psStack->bn_nCurPos );
			nError = -EINVAL;
			goto error;
		}
		if( psStack->bn_nCurPos > 0 )
		{
			BNode_s *psParent;
			bvalue_t nParent;
			bvalue_t *apsValues;

			psStack->bn_nCurPos--;
			psParent = psStack->bn_apsNodes[psStack->bn_nCurPos];
			nParent = psStack->bn_apnStack[psStack->bn_nCurPos];

			kassertw( psStack->bn_nCurPos >= 0 );

			apsValues = B_KEY_VALUE_OFFSET( psParent );

			// Replace the pointer in our parent to psNode with a pointer to psNewNode2
			if( psParent->bn_nOverflow == nNode )
			{
				psParent->bn_nOverflow = nNewNode2;
			}
			else
			{
				for( i = 0; i < psParent->bn_nKeyCount; ++i )
				{
					if( apsValues[i] == nNode )
					{
						apsValues[i] = nNewNode2;
						break;
					}
				}
				kassertw( i < psParent->bn_nKeyCount );
			}
			// Then add psNewNode1 with the first key of psNewNode2 as key
			// bt_add_key_to_node() will mark th node dirty for us.
			nError = bt_add_key_to_node( psVolume, psInode, psTrans, psStack, psParent, nParent, &psNewNode2->bn_anKeyData[0], B_KEY_INDEX_OFFSET( psNewNode2 )[0], nNewNode1 );
		}
		else
		{
			BTree_s *psTree =( BTree_s * ) bt_load_node( psVolume, psInode, psTrans, 0 );
			bvalue_t nNewParent;
			BNode_s *psNewParent;

			if( psTree == NULL )
			{
				printk( "Error: bt_insert_key() failed to load tree header\n" );
				nError = -ENOMEM;
				goto error;
			}
			if( bt_validate_tree_header( psTree ) == false )
			{
				panic( "bt_insert_key() invalid three header!\n" );
				nError = -EINVAL;
				goto error;
			}
			nNewParent = bt_alloc_node( psVolume, psInode, psTrans );
			if( nNewParent == NULL_VAL )
			{
				printk( "Error: bt_insert_key() failed to allocate new root node\n" );
				nError = -ENOMEM;
				goto error;
			}
			psNewParent = bt_load_node( psVolume, psInode, psTrans, nNewParent );
			if( psNewParent == NULL )
			{
				printk( "Error: bt_insert_key() failed to load newley allocated root node\n" );
				nError = -ENOMEM;
				goto error;
			}
			bt_assert_valid_node( psNewParent );
//                      printk( "Split - make new root\n" );

			psTree->bt_nRoot = nNewParent;
			psTree->bt_nTreeDepth++;

			if( bt_will_key_fit( psNewParent, B_KEY_INDEX_OFFSET( psNewNode2 )[0] ) == false )
			{
				panic( "bt_insert_key() new root node to small for key!\n" );
				nError = -EINVAL;
				goto error;
			}
			bt_append_key( psNewParent, &psNewNode2->bn_anKeyData[0], B_KEY_INDEX_OFFSET( psNewNode2 )[0], nNewNode1 );
			bt_assert_valid_node( psNewParent );

			psNewParent->bn_nOverflow = nNewNode2;

			bt_mark_node_dirty( psTrans, 0 );
		}
	}
      error:
	nTmpErr = bt_end_transaction( psVolume, psInode, psTrans, nError >= 0 );
	if( nTmpErr < 0 )
	{
		nError = nTmpErr;
	}
	return( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int bt_lookup_key( AfsVolume_s * psVolume, AfsInode_s * psInode, const void *pKey, int nKeySize, BIterator_s * psIterator )
{
	BStack_s sStack;
	BTransaction_s sTrans;
	int nError;

#ifdef AFS_PARANOIA
	if( psVolume == NULL )
	{
		panic( "bt_lookup_key() called with psVolume == NULL\n" );
		return( -EINVAL );
	}
	if( psInode == NULL )
	{
		panic( "bt_lookup_key() called with psInode == NULL\n" );
		return( -EINVAL );
	}
	if( pKey == NULL )
	{
		panic( "bt_lookup_key() called with pKey == NULL\n" );
		return( -EINVAL );
	}
	if( nKeySize <= 0 || nKeySize > B_MAX_KEY_SIZE )
	{
		panic( "bt_lookup_key() called with invalid key-size %d\n", nKeySize );
		return( -EINVAL );
	}
	if( psIterator == NULL )
	{
		panic( "bt_lookup_key() called with psIterator == NULL\n" );
		return( -EINVAL );
	}
#endif
	if( psInode->ai_sData.ds_nSize == 0 )
	{
		if( psInode->ai_psVNode != NULL )
		{
			psIterator->bi_nVersion = psInode->ai_psVNode->vn_nBTreeVersion;
		}
		return( 0 );
	}
	nError = bt_begin_transaction( &sStack, &sTrans );
	if( nError < 0 )
	{
		return( nError );
	}

	nError = bt_find_key( psVolume, psInode, &sTrans, &sStack, pKey, nKeySize, NULL );
	if( nError < 0 )
	{
		printk( "Error: bt_lookup_key() failed to search for key\n" );
		goto error;
	}
	if( NULL != psInode->ai_psVNode )
	{
		psIterator->bi_nVersion = psInode->ai_psVNode->vn_nBTreeVersion;
	}
	if( nError >= 0 )
	{
		const void *pKey;

		psIterator->bi_nCurNode = sStack.bn_apnStack[sStack.bn_nCurPos];
		psIterator->bi_nCurKey = sStack.bn_anPosition[sStack.bn_nCurPos];
		if( psIterator->bi_nCurKey < sStack.bn_apsNodes[sStack.bn_nCurPos]->bn_nKeyCount )
		{
			bt_get_key( sStack.bn_apsNodes[sStack.bn_nCurPos], psIterator->bi_nCurKey, &pKey, &psIterator->bi_nKeySize );
			memcpy( psIterator->bi_anKey, pKey, psIterator->bi_nKeySize );

			psIterator->bi_nCurValue = B_KEY_VALUE_OFFSET( sStack.bn_apsNodes[sStack.bn_nCurPos] )[psIterator->bi_nCurKey];
		}
	}
      error:
	bt_end_transaction( psVolume, psInode, &sTrans, false );
	return( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int bt_find_first_key( AfsVolume_s * psVolume, AfsInode_s * psInode, BIterator_s * psIterator )
{
	BStack_s sStack;
	BTransaction_s sTrans;
	BTree_s *psTree;
	bvalue_t nNode;
	BNode_s *psNode;
	int nError;
	int i;

#ifdef AFS_PARANOIA
	if( psVolume == NULL )
	{
		panic( "bt_find_first_key() called with psVolume == NULL\n" );
		return( -EINVAL );
	}
	if( psInode == NULL )
	{
		panic( "bt_find_first_key() called with psInode == NULL\n" );
		return( -EINVAL );
	}
	if( psIterator == NULL )
	{
		panic( "bt_find_first_key() called with psIterator == NULL\n" );
		return( -EINVAL );
	}
#endif

	if( psInode->ai_sData.ds_nSize == 0 )
	{
		return( -ENOENT );
	}
	nError = bt_begin_transaction( &sStack, &sTrans );
	if( nError < 0 )
	{
		return( nError );
	}
	if( psInode->ai_psVNode != NULL )
	{
		psIterator->bi_nVersion = psInode->ai_psVNode->vn_nBTreeVersion;
	}
	psTree =( BTree_s * ) bt_load_node( psVolume, psInode, &sTrans, 0 );
	if( psTree == NULL )
	{
		printk( "Error: bt_find_first_key() failed load tree header\n" );
		nError = -EIO;
		goto error;
	}
	if( bt_validate_tree_header( psTree ) == false )
	{
		panic( "bt_find_first_key() invalid three header!\n" );
		nError = -EINVAL;
		goto error;
	}
	nNode = psTree->bt_nRoot;
	psNode = bt_load_node( psVolume, psInode, &sTrans, nNode );
	if( psNode == NULL )
	{
		printk( "Error: bt_find_first_key() failed to load tree node\n" );
		nError = -EIO;
		goto error;
	}
	bt_assert_valid_node( psNode );

	// Follow the leftmost branch from nNode until we reach a leaf node
	for( i = 0; i < psTree->bt_nTreeDepth - 1; ++i )
	{
		nNode = B_KEY_VALUE_OFFSET_CONST( psNode )[0];
		psNode = bt_load_node( psVolume, psInode, &sTrans, nNode );
		if( psNode == NULL )
		{
			nError = -EIO;
			goto error;
		}
		bt_assert_valid_node( psNode );
	}

	kassertw( NULL_VAL == psNode->bn_nLeft );
	kassertw( NULL_VAL == psNode->bn_nOverflow );

	if( psNode->bn_nKeyCount > 0 )
	{
		const void *pKey;

		psIterator->bi_nCurNode = nNode;
		psIterator->bi_nCurKey = 0;
		bt_get_key( psNode, psIterator->bi_nCurKey, &pKey, &psIterator->bi_nKeySize );
		memcpy( psIterator->bi_anKey, pKey, psIterator->bi_nKeySize );

		psIterator->bi_nCurValue = B_KEY_VALUE_OFFSET( psNode )[0];
		nError = 0;
	}
	else
	{
		nError = -ENOENT;
	}
      error:
	bt_end_transaction( psVolume, psInode, &sTrans, false );
	return( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

int bt_get_next_key( AfsVolume_s * psVolume, AfsInode_s * psInode, BIterator_s * psIterator )
{
	BStack_s sStack;
	BTransaction_s sTrans;
	BNode_s *psNode;
	int nError;

#ifdef AFS_PARANOIA
	if( psVolume == NULL )
	{
		panic( "bt_get_next_key() called with psVolume == NULL\n" );
		return( -EINVAL );
	}
	if( psInode == NULL )
	{
		panic( "bt_get_next_key() called with psInode == NULL\n" );
		return( -EINVAL );
	}
	if( psIterator == NULL )
	{
		panic( "bt_get_next_key() called with psIterator == NULL\n" );
		return( -EINVAL );
	}
#endif
	nError = bt_begin_transaction( &sStack, &sTrans );

	if( nError < 0 )
	{
		return( nError );
	}

	// Revalidate the iterator if someone inserted or deleted some keys from the tree
	if( NULL != psInode->ai_psVNode && psIterator->bi_nVersion != psInode->ai_psVNode->vn_nBTreeVersion )
	{
		nError = bt_find_key( psVolume, psInode, &sTrans, &sStack, psIterator->bi_anKey, psIterator->bi_nKeySize, NULL );

		if( nError < 0 )
		{
			printk( "bt_get_next_key() faile to search for old key\n" );
			goto done;
		}

		psIterator->bi_nVersion = psInode->ai_psVNode->vn_nBTreeVersion;
		psIterator->bi_nCurNode = sStack.bn_apnStack[sStack.bn_nCurPos];
		psIterator->bi_nCurKey = sStack.bn_anPosition[sStack.bn_nCurPos];

		if( psIterator->bi_nCurKey < sStack.bn_apsNodes[sStack.bn_nCurPos]->bn_nKeyCount )
		{
			const void *pKey;

			bt_get_key( sStack.bn_apsNodes[sStack.bn_nCurPos], psIterator->bi_nCurKey, &pKey, &psIterator->bi_nKeySize );
			memcpy( psIterator->bi_anKey, pKey, psIterator->bi_nKeySize );

			psIterator->bi_nCurValue = B_KEY_VALUE_OFFSET( sStack.bn_apsNodes[sStack.bn_nCurPos] )[psIterator->bi_nCurKey];
		}
		else
		{
			nError = -ENOENT;
			goto done;
		}
		if( 0 == nError )
		{
			goto done;
		}
	}
	psNode = bt_load_node( psVolume, psInode, &sTrans, psIterator->bi_nCurNode );

	if( psNode == NULL )
	{
		nError = -EIO;
		goto done;
	}
	psIterator->bi_nCurKey++;
	if( psIterator->bi_nCurKey < psNode->bn_nKeyCount )
	{
		const void *pKey;

		psIterator->bi_nCurValue = B_KEY_VALUE_OFFSET( psNode )[psIterator->bi_nCurKey];

		bt_get_key( psNode, psIterator->bi_nCurKey, &pKey, &psIterator->bi_nKeySize );
		memcpy( psIterator->bi_anKey, pKey, psIterator->bi_nKeySize );
		nError = 0;
	}
	else
	{
		const void *pKey;

		if( psNode->bn_nRight == NULL_VAL )
		{
			nError = -ENOENT;
			goto done;
		}
		psIterator->bi_nCurNode = psNode->bn_nRight;
		psNode = bt_load_node( psVolume, psInode, &sTrans, psNode->bn_nRight );
		if( psNode == NULL )
		{
			nError = -EIO;
			goto done;
		}
		psIterator->bi_nCurKey = 0;
		bt_get_key( psNode, psIterator->bi_nCurKey, &pKey, &psIterator->bi_nKeySize );
		memcpy( psIterator->bi_anKey, pKey, psIterator->bi_nKeySize );
		psIterator->bi_nCurValue = B_KEY_VALUE_OFFSET( psNode )[0];
		nError = 0;
	}
      done:
	bt_end_transaction( psVolume, psInode, &sTrans, false );
	return( nError );
}
