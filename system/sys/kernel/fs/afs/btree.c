
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

/** Validate the consistancey of a BTree head
 * \par Description:
 * Check whether the given Btree head is internally consistant
 * \par Note:
 * \par Warning:
 * \param psTree	BTree head
 * \return true if the tree head is consistant, false otherwise
 * \sa
 *****************************************************************************/
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

/** Initialize a BTree transaction
 * \par Description:
 * Initialize the the given stack and transaction structures for a new transaction
 * \par Note:
 * \par Warning:
 * \param psStack	BTree stack to initialize
 * \param psTrans	BTree transaction to initialize
 * \return 0 on success, negative error code on failure
 * \sa
 *****************************************************************************/
int bt_begin_transaction( BStack_s * psStack, BTransaction_s * psTrans )
{
	memset( psStack, 0, sizeof( BStack_s ) );
	memset( psTrans, 0, sizeof( BTransaction_s ) );
	return( 0 );
}

/** Finalize a BTree transaction
 * \par Description:
 * Write out the results of the given transaction to the given inode.  First is the
 * transaction runs off the end of the btree, then expand the btree.  Next write out the
 * changed inode. Finally, for each block in the transaction, write out the block.
 * \par Note:
 * \par Warning:
 * \param psVolume		AFS filesystem pointer
 * \param psInode		AFS Inode of BTree
 * \param psTrans		Transaction to finalize
 * \param bOkToWrite	Whether or not psInode should be written
 * \return 0 on success, negative error code on failure
 * \sa
 *****************************************************************************/
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

/** Allocate a temporary BTree node buffer
 * \par Description:
 * Allocate and initialize a temporary BTree node buffer.  The node must be released
 * by bt_free_temp_node.
 * \par Note:
 *	No disk-space is reserved for the node.
 * Nodes allocated by this cannot be released by bt_free_node or written to disk.
 * \par Warning:
 * \param psVolume	AFS filesystem pointer
 * \return temporary node, or NULL on error
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

/** Free a temprorary BTree node buffer
 * \par Description:
 * Free a temporary BTree node buffer allocated by bt_alloc_tmp_node.
 * \par Note:
 * The assert appears to ensure the node was allocated by bt_alloc_tmp_node
 * \par Warning:
 * \param psNode	BTree node buffer to free
 * \return 0 on success, negative error code on failure
 * \sa
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

/** Allocate a BTree node buffer
 * \par Description:
 * Allocate a BTree node buffer.  This is intended to be committed to disk.  The node can
 * be released by bt_free_node
 * \par Note:
 * Nodes allocated by this canot be released by bt_free_tmp_node
 * \par Warning:
 * \return BTree node on success, NULL on failure
 * \sa
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

/** Allocate and initialize a BTree
 * \par Description:
 * Allocate a BTree structure, initialize it, add it to the given transaction, and
 * allocate space for it on the given volume.
 * \par Note:
 * \par Warning:
 * \param psVolume	AFS filesystem pointer
 * \param psInode	Inode to store BTree in
 * \param psTrans	Transaction to add BTree too
 * \return 0 on success, negative error code on failure
 * \sa
 ****************************************************************************/
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

/** Load a BTree node from a volume
 * \par Description:
 * Load the node with the given number from the given inode on the given volume into the
 * given transaction.  First, the transaction is searched to see if the requested node is
 * already loaded.  If so, it is returned.  If, in the process, it is determined that the
 * tree itself is not loaded, it will be loaded.  If the node is not found in the
 * transaction, space is allocated for it, and it is read in from the given inode/volume.
 * If it is a BTree header, it is then validated.
 * \par Note:
 * Nodes allocated by bt_load_node are released automatically in bt_end_transaction()
 * \par Warning:
 * If the tree is loaded, it involves a recursive call back into bt_load_node()
 * \param psVolume	AFS filesystem pointer
 * \param psInode	Inode containing the node
 * \param psTrans	Transaction containing the BTree containing the node
 * \param nNode		The number of the node
 * \return A pointer to the loaded node on success, NULL on failure
 * \sa
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

/** Mark a BTree node dirty
 * \par Description:
 * Mark the given node dirty in the given transaction.  When a transaction is finalized,
 * all the dirty nodes are written out.  It's a panic if the node is not in the
 * transaction.
 * \par Note:
 * Should probably be changed to return an error code, rather than panic.
 * \par Warning:
 * \param psTrans	BTree transaction containing node
 * \param nNode		The number of the node to mark dirty
 * \return nothing.
 * \sa
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

/** Allocate a BTree node
 * \par Description:
 * Allocate a node from the given inode on the given volume and add it to the given
 * transaction.  If there is a free node in the tree, it is used.  Otherwise a new node
 * is allocated.  Either way, it is marked dirty.
 * \par Note:
 * \par Warning:
 * \param psVolume	AFS filesystem pointer
 * \param psInode	AFS inode containing BTree
 * \param psTrans	Transaction to add node to
 * \return node number on success, NULL_VAL on failure
 * \sa
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

/** Free a BTree node
 * \par Description:
 * Free the node with the given location from the tree in the given inode on the given
 * volume, using the given transaction.  The tree header is loaded and validated.  Next,
 * the node to be freed is loaded and validated.  Finally, the node is marked free
 * (bn_nKeyCount = 0x12345678), and put into the free node list.
 * \par Note:
 * \par Warning:
 * \param psVolume	AFS filesystem pointer
 * \param psInode	AFS inode containing the BTree
 * \param psTrans	Transaction containing the BTree
 * \param nNode		Number of the node to free.
 * \return 0 on success, negative error code on failure
 * \sa
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

#if 0				//  Make if _BT_DEBUG

/** Get the minimum and maximum keys in a tree
 * \par Description:
 * Walk all the keys in the given tree, finding the minimum and maximum keys when
 * treated as integers.  For each node in the tree, check it's keys for minimum or
 * maximum, and then recursively call on the subtree rooted in each of them, then
 * recursively call on the subtree rooted in the overflow node, if it exists.  The
 * result is the minimum and maximum keys in the entire tree.
 * \par Note:
 * \par Warning:
 * \param psRoot	The root of the subtree to check
 * \param pnMin		The current minimum (and returned minimum)
 * \param pnMax		The current maximum (and returned maximum)
 * \return No return value, min and max are out arguments.
 * \sa
 ****************************************************************************/
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

#endif

#if 0				//  Make if _BT_DEBUG

/** Check the internal consistancy of a BTree subtree
 * \par Description:
 * Check the key validity of the given tree.  For each key in the node, get the min
 * and max of the subtree rooted in that key.  If the max is treater than the key,
 * or the min is less than the previous key, the tree is invalid.  Then, check the
 * overflow, if it exists, to ensure it's greater than the highest key in the node.
 * \par Note:
 * This should do a recursive call on all subnodes to be useful.
 * \par Warning:
 * \param psVolume	AFS filesystem pointer
 * \return 0 on success, negative error code on failure
 * \sa
 ****************************************************************************/
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
#endif

/** Print the contents of a BTree node
 * \par Description:
 * Print out the contents of the given node.  Print an indicator of left sibling, then
 * each of the keys in the node, then an indicator of overflow, and finally an indicator
 * of a right sibling.
 * \par Note:
 * \par Warning:
 * \param nKeyType	The type of the key in the Node
 * \param psNode	The Node to print
 * \return 0 on success, negative error code on failure
 * \sa
 *****************************************************************************/
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

/** Determine if a key will fit in a BTree node
 * \par Description:
 * Determine if a key of the given size will fit in the given node.  The current total
 * key size is added to the requested size, the total value size (including the new key),
 * the total index size (including the new index), and the header size.  If the resulting
 * value is less than the total node size, return true.
 * \par Note:
 * \par Warning:
 * \param psNode	BTree node to check for fit
 * \param nKeySize	Desired size of key
 * \return true if the key will fit, false otherwise
 * \sa
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

/** Compare two BTree keys for ordering
 * \par Description:
 * Compare the two given keys of the given type for ordering.  If they are equal,
 * return 0. If they pKey1 is less than pKey2, return a negative number.  Otherwise,
 * return a positive number.  For integer types, just subtract pKey2 from pKey1.
 * For floats, subtract the keys, and return 0 on equality, -1 on less, or 1 on
 * greater.  For strings, subtract each octet of pKey2 from pKey1, and, if they are
 * the same up to them minimum length of the two keys, return the difference in the
 * sizes.  Else, return the difference.
 * \par Note:
 * The string version could probably be optimized ala memcmp.
 * \par Warning:
 * \param nType		The type of the keys (e_KeyType*)
 * \param pKey1		The first key to compare
 * \param nKey1Size	The size of the first key in octets
 * \param pKey2		The second key to compare
 * \param nKey2Size	The size of the second key in octets
 * \return 0 on equality, negative if pKey1 is less than pKey2, positive otherwise
 * \sa
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

/** Append a key/value pair onto a BTree node
 * \par Description:
 * Append the given key/value pair into the given node.  First, compute the size the new
 * key will take, and ensure it will fit in the node.  Next, make space for the key by
 * moving the value array and index array down by the required amount.  Then, copy the
 * key and value data into the appropriate locations.  Finally, ensure the validity of
 * the key index by walking the entire thing and checking for ordering.
 * \par Note:
 * This does not change the overflow pointer.  Since this key is last, the overflow might
 * not be valid anymore.
 * \par Warning:
 * The key *must* fit into the node, and it must be last.  This is not checked, but is
 * asserted.
 * \param psNode	BTree node to append to
 * \param pKey		Key to append
 * \param nKeySize	Size of pKey in octets
 * \param nValue	Value to append
 * \return 0 on success, negative error code on failure
 * \sa
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

/** Insert a key/value pair into a BTree node in order
 * \par Description:
 * Insert the given key/value pair into the given node in sorted order.  First, caluclate
 * the amount of space necessary, and ensure the new key will fit.  Next, if the node is
 * empty, or the new key is greater than any extant key, append it.  Otherwise, validate
 * the node and move the key index array and the values array to make space for the new
 * key.  Then, walk all the existing keys, finding the correct spot, and inserting the
 * new key.  All other key indices after that spot are updated.  Finally, verify the
 * ordering of the key indices.
 * \par Note:
 * If the key is greater than any in the node, this will call bt_append_key, so the
 * overflow pointer is not handled at all.  The caller must handle this.
 * \par Note:
 * This should be combined with bt_add_key_to_node, with one function deciding if the key
 * can fit and whether or not the tree needs rebalancing.  Having them separate results
 * in code duplication.
 * \par Warning:
 * The key *must* fit into the node.
 * \param psInode		AFS Inode of BTree
 * \param psNode		Node to insert key into
 * \param a_pKey		Key to insert
 * \param a_nKeySize	Size of Key in octets
 * \param nValue		Value to insert
 * \return 0 on success, negative error code on failure
 * \sa
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

/** Find a key location or insertion position
 * \par Description:
 * Find the BTree leaf node that contains (or should contain) the given key in tree on
 * the given volume with the given inode, using the given transaction.  Return it in the
 * given node pointer, with the path to it in the given stack.  First, load and validate
 * the root of the tree.  Next, if the tree is empty, return the root node and false.
 * Otherwise, walk the tree, looking for the key.  For each non-leaf node, walk the keys
 * in the node, looking for one that is greater than the given key.  If we find it, we
 * follow it, otherwise, we follow the overflow node.  Either way, the current node is
 * added to the stack.  When a leaf node is reached, it is the correct node.  Add it to
 * the stack, and search it for the exact match of the key.  If it's found, return true,
 * otherwise return false.
 * \par Note:
 * \par Warning:
 * \param psVolume	AFS filesystem pointer
 * \param psInode	AFS inode containing the tree to search
 * \param psTrans	Transaction for the tree
 * \param psStack	Stack to fill with the path to the returned node
 * \param pKey		The key to find
 * \param nKeySize	The size of pKey in octets
 * \param ppsNode	Return argument for the found node
 * \return 0 and the insertion node if the key was not found, 1 and the node if it was
 * \sa
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

/** Merge keys from one BTree node to another, possibly inserting another key.
 * \par Description:
 * Merge keys from the given source node starting at the given key index and offset, into
 * the given destination node, possibly inserting the given key into the correct position
 * in the process.  The destination node will be filled up until either the source node
 * is empty, of the destination node reaches the given maximum size.  First, ensure that
 * the source node has at least 2 keys in it.  Then, walk the keys in the source node
 * starting with the correct one, and move them one at a time into the destination node.
 * If the key about to be moved is larger than the given key, and bInserted is false,
 * insert the given key first into the destination node, then, the key under
 * consideration.  Fill pnStartIndex with the first empty index in the destination node,
 * and fill pnStartOffset with the first empty key offset in the destination node.
 * \par Note:
 * \par Warning:
 * \param psInode		AFS Inode containing the tree
 * \param psDstNode		Destination node for the merge
 * \param psSrcNode		Source node for the merge
 * \param pnStartIndex	Key index in the source node to start merging from
 * \param pnStartOffset	Key offset in the source node to start merging from
 * \param nMaxSize		Maximum size of destination node in octets
 * \param pKey			Key to possibly insert during the merge
 * \param nkeyLen		Length of pKey in octets
 * \param psValue		Value for pKey
 * \param bInserted		If false, attempt to insert pKey into psDstNode
 * \return True if pKey was inserted (or bInserted was true), false otherwise
 * \sa
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

/** Add a key to a BTree node and rebalance the tree
 * \par Description:
 * Insert the given key/value pair into the given node with the given stack of the tree
 * on the given volume with the given inode and transaction, rebalancing as necessary.
 * This is a giant loop, that starts with the given node and walks up the stack repeating
 * as necessary.  Here's the loop.  First, if the current key will fit in the current
 * node, no further rebalancing is necessary.  Insert it, validate the resulting node,
 * mark it dirty, and return.  Otherwise, this node needs to be split, and the parent
 * subtree rebalanced.  Start by allocating a temporary node, and copying the first half
 * of the current node into it, merging the current key if necessary.  The key to be
 * inserted into the parent node of the newly split node is now either the first key left
 * in the current node, or the current key.  If it's in the current node, skip it, and in
 * either case record it for later use.  Then, allocate a new node and copy the second
 * half of the current node into it, again possibly merging the current key into it.  If
 * the current key has not been merged and is not the parent key, append it onto the new
 * node.  Make a copy of the parent key, then copy the temporary node into the current
 * node Mark the current node dirty, and unwind the next level of the stack to insert the
 * parent key.  If the stack is not empty, then reset all the state to point to the
 * parent node and the parent key to be inserted in that node, and repeat the loop.
 * Otherwise, we're at the root of the tree.  Load and validate the tree head.  Allocate
 * a new node to split the root node into, insert it into the root pointer of the head,
 * and add the parent key to it, pointing at the former root node.  Validate the new
 * root, and mark the former root dirty.  The loop terminates either here, when the
 * entire tree has been rebalanced, or in the first part where a subtree no longer needs
 * rebalancing.
 * \par Note:
 * Possibly move the body of the loop into a separate function?  Or even two functions,
 * one to rebalance a subtree, one to split the root?
 * \par Warning:
 * \param psVolume	AFS filesystem pointer
 * \param psInode	AFS inode containing the tree
 * \param psTrans	Transaction for the tree
 * \param psNode	Node to add key to
 * \param nNode		Node number of psNode
 * \param pKey		Key to add
 * \param nKeyLen	Length of pKey in octets
 * \param psValue	Value of pKey
 * \return 0 on success, negative error code on failure
 * \sa
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

/** Debug: Print out a BTree level
 * \par Description:
 * Print out a single level of the given tree with the given inode, stack, and root node,
 * on the given volume  If nLevel is zero, just print the root node.  Otherwise, walk all
 * the children of the root node, recursively calling this function with the subtrees
 * rooted in them, decrementing level.  Finally, if the overflow node is not empty, walk
 * that as well.  The result is the printing of an entire level of the tree represented
 * by nLevel for the first calling of this function.
 * \par Note:
 * Rename to bt_print_level?
 * \par Warning:
 * \param psVolume	AFS filesystem pointer
 * \param psInode	AFS indode containing tree
 * \param psStack	Stack for this tree
 * \param psRoot	Root of this tree
 * \param nLevel	Depth in tree to stop printing.
 * \return false if the last level of the tree was printed, true otherwise.
 * \sa
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

/** Debug: Print out a BTree
 * \par Description:
 * Print out the tree represented by the given inode on the given volume.  This is
 * largely a wrapper for _bt_print_tree.  Start a transaction, and load the root of the
 * tree.  Then, while the entire tree was not printed, print one level deeper of the
 * tree.  Finally, end the transaction.  This will result in a level order printing of
 * the tree.
 * \par Note:
 * \par Warning:
 * \param psVolume	AFS filesystem pointer
 * \return 0 on success, negative error code on failure
 * \sa
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

/** Debug: Find a BTree key via linear search
 * \par Description:
 * This is an alternative search for a key.  Find the given key in the given tree
 * via linear search of all keys.  This is used to check other find functions.  Get
 * the first key in the tree.  Then, while we have more keys left and have not found
 * the key we're looking for, get the next key out of the tree.
 * \par Note:
 * \par Warning:
 * The return value is wrong.  It should be zero if not found and there's no error.
 * \param psVolume		AFS filesystem pointer
 * \param psInode		AFS inode of tree
 * \param pKey			Key to find
 * \param nKeySize		Size of pKey
 * \param psIterator	Iterator used for getting next key
 * \return 1 if found, 0 if not found, negative error code on failure
 * \sa
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

/** Debug: Print out all the keys in a BTree
 * \par Description:
 * Print out all the keys in the tree in the given inode.  Get the first key.  While
 * we have more keys, print out the key.
 * \par Note:
 * \par Warning:
 * \param psVolume	AFS filesystem pointer
 * \param psInode	AFS inode for the tree
 * \return >=0 on success, negative error code on failure
 * \sa
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

/** Entry: Insert a key/value pair into a BTree
 * \par Description:
 * Insert the given key/value pair into the BTree with the given inode on the given
 * volume.  First, begin a new transaction.  Next, if the key already exists in the tree,
 * fail with -EEXIST.  The find will also return the insertion node.  If the key will fit
 * in the correct node, just add it, mark the node dirty, and return success.  Otherwise,
 * allocate a temporary node and a new node, linking the new node with the current node.
 * Copy the first half of the current node into the temporary node, possibly merging the
 * key.  Copy the second half of the current node into the new node, again possibly
 * merging the key.  If the key was not merged, append it to the new node.  Copy the
 * temporary node back over the current node, and free the temporary node.  Mark the
 * current and new nodes dirty.  If we're not at the root, replace the parent pointer to
 * the current node with a pointer to the new node, and add the current node into the
 * tree with the first key in the new node pointing to it.  Otherwise, this is the root
 * of the tree.  Load the tree header.  Allocate a new parent pointer, and add the first
 * key in the new node to the parent, pointing to the new node.  Finally, end the
 * transaction and return the error code.
 * \par Note:
 * I don't yet understand why this doesn't just wrap bt_add_key_to_node
 * \par Warning:
 * \param psVolume	AFS filesystem pointer
 * \param psInode	AFS inode containing the tree
 * \param pKey		Key to add
 * \param nKeySize	Size of pKey in octets
 * \param nValue	Value to add
 * \return 0 on success, negative error code on failure
 * \sa
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

/** Entry: Lookup the value associated with a key in a BTree
 * \par Description:
 * Lookup the given key in the tree with the given inode on the given volume, and return
 * the key/value pair in the given iterator.  First, start a transaction.  Then, find the
 * node containing the key.  Then, if it's found, store the node, key number, and a copy
 * of the key and value into the given iterator.  Finally, finish the transaction.
 * \par Note:
 * Included inline in bt_get_next_key.  Should be factored  out into helper
 * function.
 * \par Warning:
 * \param psVolume		AFS filesystem pointer
 * \param psInode		AFS inode containing the tree
 * \param pKey			Key to search for
 * \param nKeySize		Size of pKey in octets
 * \param psIterator	Iterator to return key/value pair and containing node.
 * \return 0 on success, negative error code on failure
 * \sa
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

/** Entry: Find the first key in a BTree
 * \par Description:
 * Find the first key/value pair in the tree with the given inode on the given volume,
 * and return them in the given iterator.  First, start a transaction.  Next, get and
 * validate the tree header, and the root node.  Then, follow the tree to the leftmost
 * leaf node by: loading the node pointed to by the first key in the current node, and
 * making it the current node.  Store that leftmost leaf node, the first key in it, and
 * the value associated with that first key, in the given iterator.  Finally, finish the
 * transaction.
 * \par Note:
 * \par Warning:
 * \param psVolume		AFS filesystem pointer
 * \param psInode		AFS inode containing the tree
 * \param psIterator	Iterator to return the key/value pair and node
 * \return 0 on success, negative error code on failure
 * \sa
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

/** Entry: Get the next key from a BTree
 * \par Description:
 * Find the next key after the one in the given iterator in the tree with the given inode
 * on the given volume.  First, start a new transaction.  Then, check to see if the tree
 * has changed since the key in the iterator was loaded.  If it has, reload the key/value
 * pair and node into the iterator.  Next, load the node containing the current key.
 * Increment the key number of the current key, and check to see if the new key is in the
 * current node.  If it is, get the copy the key and value into the iterator.  If it is
 * not, load the next leafnode, and copy the first key/value pair and the new node into
 * the iterator.  Finish the transaction.
 * \par Note:
 * Why include pretty much all of bt_lookup_key inline?  Should be factored  out
 * into helper function.
 * \par Warning:
 * \param psVolume		AFS filesystem pointer
 * \param psInode		AFS inode containing the tree
 * \param psIterator	Iterator containing current key/to return next key/value pair
 * \return 0 on success, negative error code on failure
 * \sa
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
