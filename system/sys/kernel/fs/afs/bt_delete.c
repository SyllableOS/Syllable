
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

#include <atheos/types.h>
#include <atheos/kernel.h>
#include <posix/errno.h>

#include <macros.h>

#include "afs.h"
#include "btree.h"

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static status_t bt_delete_key_from_node( BNode_s *psNode, int nIndex )
{
	int16 *pnKeyIndexes;
	int16 *pnNewKeyIndexes;
	bvalue_t *apsNewValues;
	int nKeyOffset;
	int nKeySize;
	int i;

#ifdef AFS_PARANOIA
	if( psNode == NULL )
	{
		panic( "bt_delete_key_from_node() called with psNode == NULL\n" );
		return( -EINVAL );
	}
	if( nIndex < 0 || nIndex >= psNode->bn_nKeyCount )
	{
		panic( "bt_delete_key_from_node() called with invlid index %d. KeyCount: %d\n", nIndex, psNode->bn_nKeyCount );
		return( -EINVAL );
	}
#endif
	pnKeyIndexes = B_KEY_INDEX_OFFSET( psNode );
	nKeyOffset =( nIndex > 0 ) ? pnKeyIndexes[nIndex - 1] : 0;
	nKeySize = pnKeyIndexes[nIndex] - nKeyOffset;

	memmove( &psNode->bn_anKeyData[nKeyOffset], &psNode->bn_anKeyData[pnKeyIndexes[nIndex]], psNode->bn_nTotKeySize - pnKeyIndexes[nIndex] );

	psNode->bn_nTotKeySize -= nKeySize;
	psNode->bn_nKeyCount--;

	pnNewKeyIndexes = B_KEY_INDEX_OFFSET( psNode );
	apsNewValues = B_KEY_VALUE_OFFSET( psNode );

	memmove( pnNewKeyIndexes, pnKeyIndexes,( psNode->bn_nKeyCount + 1 ) * ( B_SIZEOF_VALUE + B_SIZEOF_KEYOFFSET ) );

	for( i = nIndex; i < psNode->bn_nKeyCount - 1; ++i )
	{
		pnNewKeyIndexes[i] = pnNewKeyIndexes[i + 1] - nKeySize;
	}
	pnNewKeyIndexes[psNode->bn_nKeyCount - 1] = psNode->bn_nTotKeySize;
	memmove( apsNewValues,( ( char * )apsNewValues ) + B_SIZEOF_KEYOFFSET, ( psNode->bn_nKeyCount + 1 ) * B_SIZEOF_VALUE );
	memmove( &apsNewValues[nIndex], &apsNewValues[nIndex + 1],( psNode->bn_nKeyCount - nIndex ) * B_SIZEOF_VALUE );
	return( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static status_t bt_replace_key( AfsVolume_s * psVolume, AfsInode_s * psInode, BTransaction_s * psTrans, BStack_s * psStack, BNode_s *psNode, bvalue_t nNode, int nIndex, const void *pNewKey, int nNewKeyLen )
{
	bvalue_t psValue;
	status_t nError;

#ifdef AFS_PARANOIA
	if( psVolume == NULL )
	{
		panic( "bt_replace_key() called with psVolume == NULL\n" );
		return( -EINVAL );
	}
	if( psInode == NULL )
	{
		panic( "bt_replace_key() called with psInode == NULL\n" );
		return( -EINVAL );
	}
	if( psTrans == NULL )
	{
		panic( "bt_replace_key() called with psTrans == NULL\n" );
		return( -EINVAL );
	}
	if( psStack == NULL )
	{
		panic( "bt_replace_key() called with psStack == NULL\n" );
		return( -EINVAL );
	}
	if( psNode == NULL )
	{
		panic( "bt_replace_key() called with psNode == NULL\n" );
		return( -EINVAL );
	}
	if( nNode == NULL_VAL )
	{
		panic( "bt_replace_key() called with nNode == NULL_VAL\n" );
		return( -EINVAL );
	}
	if( nIndex < 0 || nIndex >= psNode->bn_nKeyCount )
	{
		panic( "bt_replace_key() called with invalid index %d. KeyCount: %d\n", nIndex, psNode->bn_nKeyCount );
		return( -EINVAL );
	}
	if( pNewKey == NULL )
	{
		panic( "bt_replace_key() called with pNewKey == NULL\n" );
		return( -EINVAL );
	}
	if( nNewKeyLen <= 0 || nNewKeyLen > B_MAX_KEY_SIZE )
	{
		panic( "bt_replace_key() called with invalid key-length %d\n", nNewKeyLen );
		return( -EINVAL );
	}
#endif
	psValue = B_KEY_VALUE_OFFSET( psNode )[nIndex];

	nError = bt_delete_key_from_node( psNode, nIndex );

	if( nError < 0 )
	{
		return( nError );
	}

	if( bt_will_key_fit( psNode, nNewKeyLen ) )
	{
		bt_assert_valid_node( psNode );
		nError = bt_insert_key_to_node( psInode, psNode, pNewKey, nNewKeyLen, psValue );
		bt_assert_valid_node( psNode );
	}
	else
	{
		nError = bt_add_key_to_node( psVolume, psInode, psTrans, psStack, psNode, nNode, pNewKey, nNewKeyLen, psValue );
	}
	bt_mark_node_dirty( psTrans, nNode );
	return( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static status_t bt_rename_parent_key( AfsVolume_s * psVolume, AfsInode_s * psInode, BTransaction_s * psTrans, BStack_s * psStack, bvalue_t nNode )
{
	BTree_s *psTree;
	BNode_s *psParent;
	bvalue_t nParent;
	const BNode_s *psChild;
	const void *pNewKey;
	int nNewKeyLen;
	status_t nError;
	int i;

#ifdef AFS_PARANOIA
	if( psVolume == NULL )
	{
		panic( "bt_rename_parent_key() called with psVolume == NULL\n" );
		return( -EINVAL );
	}
	if( psInode == NULL )
	{
		panic( "bt_rename_parent_key() called with psInode == NULL\n" );
		return( -EINVAL );
	}
	if( psTrans == NULL )
	{
		panic( "bt_rename_parent_key() called with psTrans == NULL\n" );
		return( -EINVAL );
	}
	if( psStack == NULL )
	{
		panic( "bt_rename_parent_key() called with psStack == NULL\n" );
		return( -EINVAL );
	}
	if( nNode == NULL_VAL )
	{
		panic( "bt_rename_parent_key() called with nNode == NULL_VAL\n" );
		return( -EINVAL );
	}
#endif
	psTree =( BTree_s * ) bt_load_node( psVolume, psInode, psTrans, 0 );
	if( psTree == NULL )
	{
		printk( "Error: bt_rename_parent_key() failed to load tree header\n" );
		return( -ENOMEM );
	}
	kassertw( psStack->bn_nCurPos >= 0 );

	psParent = psStack->bn_apsNodes[psStack->bn_nCurPos];
	nParent = psStack->bn_apnStack[psStack->bn_nCurPos];

	// Follow the leftmost branch from nNode until we reach a leaf node
	// Then we use this key as the GT key pointing to nNode from its parent.

	psChild = bt_load_node( psVolume, psInode, psTrans, nNode );

	if( psChild == NULL )
	{
		printk( "bt_rename_parent_key() failed to load child node\n" );
		return( -EIO );
	}
	bt_assert_valid_node( psChild );

	for( i = psStack->bn_nCurPos + 1; i < psTree->bt_nTreeDepth - 1; ++i )
	{
		psChild = bt_load_node( psVolume, psInode, psTrans, B_KEY_VALUE_OFFSET_CONST( psChild )[0] );
		if( psChild == NULL )
		{
			printk( "bt_rename_parent_key() failed to load child node\n" );
			return( -EIO );
		}
		bt_assert_valid_node( psChild );
	}

	kassertw( psChild->bn_nKeyCount > 0 );

	pNewKey = &psChild->bn_anKeyData[0];
	nNewKeyLen = B_KEY_INDEX_OFFSET_CONST( psChild )[0];

	kassertw( psChild->bn_nKeyCount > 0 );

	// Search the parent node for a pointer to nNode.
	// Then replace the key associated with the previous pointer.
	// bt_replace_key() will mark the node dirty.

	if( nNode == psParent->bn_nOverflow )
	{
		nError = bt_replace_key( psVolume, psInode, psTrans, psStack, psParent, nParent, psParent->bn_nKeyCount - 1, pNewKey, nNewKeyLen );
	}
	else
	{
		const bvalue_t *apsValues = B_KEY_VALUE_OFFSET_CONST( psParent );
		bool bFound = false;
		int j;

		nError = 0;

		// FIXME: Faster to start at 1, but this way we can assert that the node was found.
		for( j = 0; j < psParent->bn_nKeyCount; ++j )
		{
			if( nNode == apsValues[j] )
			{
				if( j > 0 )
				{
					nError = bt_replace_key( psVolume, psInode, psTrans, psStack, psParent, nParent, j - 1, pNewKey, nNewKeyLen );
				}
				bFound = true;
				break;
			}
		}
		kassertw( bFound );
	}
	return( nError );
}


/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static bool bt_join_right_to_left( const BNode_s *psParent, BNode_s *psLeft, bvalue_t nLeft, BNode_s *psRight, bool bIsLeaf )
{
#ifdef AFS_PARANOIA
	if( psParent == NULL )
	{
		panic( "bt_join_right_to_left() called with psParent == NULL\n" );
		return( false );
	}
	if( psLeft == NULL )
	{
		panic( "bt_join_right_to_left() called with psLeft == NULL\n" );
		return( false );
	}
	if( nLeft == NULL_VAL )
	{
		panic( "bt_join_right_to_left() called with nLeft == NULL_VAL\n" );
		return( false );
	}
	if( psRight == NULL )
	{
		panic( "bt_join_right_to_left() called with psRight == NULL\n" );
		return( false );
	}
	if( psLeft == psRight )
	{
		panic( "bt_join_right_to_left() called with psLeft == psRight\n" );
		return( false );
	}
#endif
	// If the overflow pointer in the left node is used, we must invent a new key
	// Vi must calculate the size of the extra key before we decide if the nodes
	// can be fully joined.

	if( psLeft->bn_nOverflow != NULL_VAL )
	{
		const void *pOfKey;
		int nOfSize;
		int i;

		if( nLeft == psParent->bn_nOverflow )
		{
			bt_get_key( psParent, psParent->bn_nKeyCount - 1, &pOfKey, &nOfSize );
		}
		else
		{
			for( i = 0; i < psParent->bn_nKeyCount; ++i )
			{
				if( B_KEY_VALUE_OFFSET_CONST( psParent )[i] == nLeft )
				{
					bt_get_key( psParent, i, &pOfKey, &nOfSize );
					break;
				}
			}
			kassertw( i < psParent->bn_nKeyCount );
		}

		kassertw( bt_will_key_fit( psLeft, nOfSize ) );
		bt_append_key( psLeft, pOfKey, nOfSize, psLeft->bn_nOverflow );

		psLeft->bn_nOverflow = NULL_VAL;
	}

	if( ( B_TOT_KEY_SIZE( psRight ) + B_TOT_KEY_SIZE( psLeft ) ) <= ( B_NODE_SIZE - B_HEADER_SIZE ) )
	{
		bvalue_t *apsValues = B_KEY_VALUE_OFFSET( psRight );
		int16 *pnKeyIndexes = B_KEY_INDEX_OFFSET( psRight );
		int nKeyOffset = 0;
		int i;

		for( i = 0; i < psRight->bn_nKeyCount; ++i )
		{
			int nCurKeyLen = pnKeyIndexes[i] -( ( i > 0 ) ? pnKeyIndexes[i - 1] : 0 );

			kassertw( bt_will_key_fit( psLeft, nCurKeyLen ) );
			bt_append_key( psLeft, &psRight->bn_anKeyData[nKeyOffset], nCurKeyLen, apsValues[i] );
			nKeyOffset += nCurKeyLen;
		}
		psLeft->bn_nOverflow = psRight->bn_nOverflow;
		return( true );
	}
	else
	{
		while( ( B_TOT_KEY_SIZE( psLeft ) <= B_TOT_KEY_SIZE( psRight ) || psLeft->bn_nKeyCount < 2 ) && psRight->bn_nKeyCount > 3 )
		{
			int16 *pnKeyIndexes = B_KEY_INDEX_OFFSET( psRight );
			bvalue_t *apsValues = B_KEY_VALUE_OFFSET( psRight );

			if( bt_will_key_fit( psLeft, pnKeyIndexes[0] ) == false )
			{
				break;
			}

			bt_append_key( psLeft, &psRight->bn_anKeyData[0], pnKeyIndexes[0], apsValues[0] );
			bt_delete_key_from_node( psRight, 0 );
		}

		if( false == bIsLeaf )
		{
			if( psRight->bn_nOverflow == NULL_VAL && psRight->bn_nKeyCount > 1 )
			{
				psRight->bn_nOverflow = B_KEY_VALUE_OFFSET( psRight )[psRight->bn_nKeyCount - 1];
				bt_delete_key_from_node( psRight, psRight->bn_nKeyCount - 1 );
			}

			if( psLeft->bn_nOverflow == NULL_VAL && psLeft->bn_nKeyCount > 1 )
			{
				psLeft->bn_nOverflow = B_KEY_VALUE_OFFSET( psLeft )[psLeft->bn_nKeyCount - 1];
				bt_delete_key_from_node( psLeft, psLeft->bn_nKeyCount - 1 );
			}
			if( psLeft->bn_nOverflow == NULL_VAL )
			{
				kassertw( psRight->bn_nKeyCount > 1 );
				psLeft->bn_nOverflow = B_KEY_VALUE_OFFSET( psRight )[0];
				bt_delete_key_from_node( psRight, 0 );
			}
			kassertw( psLeft->bn_nKeyCount > 0 && NULL_VAL != psLeft->bn_nOverflow );
			kassertw( psRight->bn_nKeyCount > 0 && NULL_VAL != psRight->bn_nOverflow );
		}
		return( false );
	}
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static bool bt_join_left_to_right( AfsInode_s * psInode, const BNode_s *psParent, BNode_s *psRight, BNode_s *psLeft, bvalue_t nLeft, bool bIsLeaf )
{
	const void *pOfKey;
	int nOfSize = 0;

#ifdef AFS_PARANOIA
	if( psInode == NULL )
	{
		panic( "bt_join_left_to_right() called with psInode == NULL\n" );
		return( false );
	}
	if( psParent == NULL )
	{
		panic( "bt_join_left_to_right() called with psParent == NULL\n" );
		return( false );
	}
	if( psRight == NULL )
	{
		panic( "bt_join_left_to_right() called with psRight == NULL\n" );
		return( false );
	}
	if( psLeft == NULL )
	{
		panic( "bt_join_left_to_right() called with psLeft == NULL\n" );
		return( false );
	}
#endif
	// If the overflow pointer in the left node is used, we must invent a new key
	// Vi must calculate the size of the extra key before we decide if the nodes
	// can be fully joined.

	kassertw( false == bIsLeaf || NULL_VAL == psLeft->bn_nOverflow );

	if( psLeft->bn_nOverflow != NULL_VAL )
	{
		int i;

		if( nLeft == psParent->bn_nOverflow )
		{
			bt_get_key( psParent, psParent->bn_nKeyCount - 1, &pOfKey, &nOfSize );
		}
		else
		{
			for( i = 0; i < psParent->bn_nKeyCount; ++i )
			{
				if( B_KEY_VALUE_OFFSET_CONST( psParent )[i] == nLeft )
				{
					bt_get_key( psParent, i, &pOfKey, &nOfSize );
					break;
				}
			}
			kassertw( i < psParent->bn_nKeyCount );
		}

//              printk( "Add overflow with key %d\n", *((int*)psChild->bn_anKeyData) );
		kassertw( nOfSize > 0 );
		kassertw( nOfSize <= 256 );
		kassertw( bt_will_key_fit( psRight, nOfSize ) );

		bt_insert_key_to_node( psInode, psRight, pOfKey, nOfSize, psLeft->bn_nOverflow );
//              printk( "Add overflow with key %d\n", *((int*)pOfKey) );
		psLeft->bn_nOverflow = NULL_VAL;
	}

	if( nOfSize > 0 )
	{
		nOfSize += B_SIZEOF_KEYOFFSET + B_SIZEOF_VALUE;
	}
	nOfSize = 0;

	if( bIsLeaf == false )
	{
		if( psRight->bn_nOverflow == NULL_VAL && psRight->bn_nKeyCount > 1 )
		{
			if( psRight->bn_nKeyCount == 0 )
			{
				psRight->bn_nOverflow = B_KEY_VALUE_OFFSET( psRight )[psRight->bn_nKeyCount - 1];
				bt_delete_key_from_node( psRight, psRight->bn_nKeyCount - 1 );
			}
			else
			{
				psRight->bn_nOverflow = B_KEY_VALUE_OFFSET( psLeft )[psLeft->bn_nKeyCount - 1];
				bt_delete_key_from_node( psLeft, psLeft->bn_nKeyCount - 1 );
			}
			kassertw( 0 );
		}
	}

	if( ( B_TOT_KEY_SIZE( psLeft ) + B_TOT_KEY_SIZE( psRight ) - nOfSize ) <= ( B_NODE_SIZE - B_HEADER_SIZE ) )
	{
		bvalue_t *apsValues = B_KEY_VALUE_OFFSET( psLeft );
		int16 *pnKeyIndexes = B_KEY_INDEX_OFFSET( psLeft );
		int nKeyOffset = 0;
		int i;

		for( i = 0; i < psLeft->bn_nKeyCount; ++i )
		{
			int nCurKeyLen = pnKeyIndexes[i] -( ( i > 0 ) ? pnKeyIndexes[i - 1] : 0 );

			kassertw( nCurKeyLen > 0 );
			kassertw( nCurKeyLen <= 256 );
			kassertw( bt_will_key_fit( psRight, nCurKeyLen ) );
			bt_insert_key_to_node( psInode, psRight, &psLeft->bn_anKeyData[nKeyOffset], nCurKeyLen, apsValues[i] );
			nKeyOffset += nCurKeyLen;
		}
		return( true );
	}
	else
	{
		while( ( B_TOT_KEY_SIZE( psRight ) <= B_TOT_KEY_SIZE( psLeft ) || psRight->bn_nKeyCount < 2 ) && psLeft->bn_nKeyCount > 3 )
		{
			int16 *pnKeyIndexes = B_KEY_INDEX_OFFSET( psLeft );
			bvalue_t *apsValues = B_KEY_VALUE_OFFSET( psLeft );
			int nIndex = psLeft->bn_nKeyCount - 1;
			int nCurKeyLen = pnKeyIndexes[nIndex] -( ( nIndex > 0 ) ? pnKeyIndexes[nIndex - 1] : 0 );

			kassertw( nIndex >= 0 );
			kassertw( nCurKeyLen > 0 );
			kassertw( nCurKeyLen <= 256 );

			if( bt_will_key_fit( psRight, nCurKeyLen ) == false )
			{
				break;
			}
			bt_insert_key_to_node( psInode, psRight, &psLeft->bn_anKeyData[psLeft->bn_nTotKeySize - nCurKeyLen], nCurKeyLen, apsValues[nIndex] );
			bt_delete_key_from_node( psLeft, nIndex );
		}
		if( bIsLeaf == false )
		{
			if( psLeft->bn_nOverflow == NULL_VAL && psLeft->bn_nKeyCount > 1 )
			{
				psLeft->bn_nOverflow = B_KEY_VALUE_OFFSET( psLeft )[psLeft->bn_nKeyCount - 1];
				bt_delete_key_from_node( psLeft, psLeft->bn_nKeyCount - 1 );
			}

			if( !( psLeft->bn_nKeyCount > 0 && psLeft->bn_nOverflow != NULL_VAL ) )
			{
				kassertw( psRight->bn_nKeyCount > 1 );
				psLeft->bn_nOverflow = B_KEY_VALUE_OFFSET( psRight )[0];
				bt_delete_key_from_node( psRight, 0 );
			}
			kassertw( psLeft->bn_nKeyCount > 0 && NULL_VAL != psLeft->bn_nOverflow );
			kassertw( psRight->bn_nKeyCount > 0 && NULL_VAL != psRight->bn_nOverflow );
		}
		return( false );
	}
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static status_t bt_remove_subnode( AfsVolume_s * psVolume, AfsInode_s * psInode, BTransaction_s * psTrans, BStack_s * psStack, bvalue_t nNode )
{
	status_t nError = 0;

#ifdef AFS_PARANOIA
	if( psVolume == NULL )
	{
		panic( "bt_remove_subnode() called with psVolume == NULL\n" );
		return( -EINVAL );
	}
	if( psInode == NULL )
	{
		panic( "bt_remove_subnode() called with psInode == NULL\n" );
		return( -EINVAL );
	}
	if( psTrans == NULL )
	{
		panic( "bt_remove_subnode() called with psTrans == NULL\n" );
		return( -EINVAL );
	}
	if( psStack == NULL )
	{
		panic( "bt_remove_subnode() called with psStack == NULL\n" );
		return( -EINVAL );
	}
	if( nNode == NULL_VAL )
	{
		panic( "bt_remove_subnode() called with nNode == NULL_VAL\n" );
		return( -EINVAL );
	}
#endif
	for( ;; )
	{
		BNode_s *psParent = psStack->bn_apsNodes[psStack->bn_nCurPos];
		bvalue_t nParent = psStack->bn_apnStack[psStack->bn_nCurPos];
		bvalue_t *apsValues = B_KEY_VALUE_OFFSET( psParent );
		BNode_s *psNode;
		int nIndex = -1;

		if( nNode == psParent->bn_nOverflow )
		{
			psParent->bn_nOverflow = NULL_VAL;
			if( psParent->bn_nKeyCount > 1 )
			{
				nIndex = psParent->bn_nKeyCount - 1;
				psParent->bn_nOverflow = B_KEY_VALUE_OFFSET( psParent )[nIndex];
				bt_delete_key_from_node( psParent, nIndex );
			}
		}
		else
		{
			int i;

			for( i = 0; i < psParent->bn_nKeyCount; ++i )
			{
				if( nNode == apsValues[i] )
				{
					bt_delete_key_from_node( psParent, i );
					break;
				}
			}
			nIndex = i;
		}
		bt_mark_node_dirty( psTrans, nParent );
		nError = bt_free_node( psVolume, psInode, psTrans, nNode );
		if( nError < 0 )
		{
			break;
		}

		nNode = nParent;
		psNode = psParent;

		psStack->bn_nCurPos--;

		if( psStack->bn_nCurPos >= 0 )
		{
			psParent = psStack->bn_apsNodes[psStack->bn_nCurPos];

			if( B_TOT_KEY_SIZE( psNode ) < ( ( B_NODE_SIZE - B_HEADER_SIZE ) / 2 ) )
			{
				int nPos = 0;

				// If the node become less than half filled we steels some keys from one of it's neighbour siblings.

				apsValues = B_KEY_VALUE_OFFSET( psParent );

				nPos = psStack->bn_anPosition[psStack->bn_nCurPos];

				if( nPos > 0 )
				{
					bvalue_t nSibling = apsValues[nPos - 1];
					BNode_s *psSibling = bt_load_node( psVolume, psInode, psTrans, nSibling );

					if( psSibling == NULL )
					{
						printk( "error: bt_remove_subnode:1() failed to load tree node\n" );
						nError = -EIO;
						goto error;
					}

					bt_assert_valid_node( psSibling );

					bt_mark_node_dirty( psTrans, nNode );
					bt_mark_node_dirty( psTrans, nSibling );

					//        Join bode with left sibling.
					if( bt_join_left_to_right( psInode, psParent, psNode, psSibling, nSibling, false ) )
					{
						kassertw( psNode->bn_nKeyCount > 0 );

						nNode = nSibling;

						continue;
					}
					else
					{
						// First key changed, so we have to rename the GE key pointing at the node
						kassertw( psNode->bn_nKeyCount > 0 );
						kassertw( psSibling->bn_nKeyCount > 0 );
						nError = bt_rename_parent_key( psVolume, psInode, psTrans, psStack, nNode );
						if( nError < 0 )
						{
							printk( "Error: bt_remove_subnode:1() failed to rename parent key\n" );
							goto error;
						}
						break;
					}
				}
				else
				{
					bvalue_t nSibling =( psParent->bn_nKeyCount > 1 ) ? apsValues[1] : psParent->bn_nOverflow;
					BNode_s *psSibling = bt_load_node( psVolume, psInode, psTrans, nSibling );

					if( psSibling == NULL )
					{
						printk( "Error: bt_remove_subnode:2() failed to load tree node\n" );
						nError = -EIO;
						goto error;
					}

					bt_assert_valid_node( psSibling );

					kassertw( psSibling != psNode );
					kassertw( psParent->bn_nKeyCount > 0 );

					bt_mark_node_dirty( psTrans, nNode );
					bt_mark_node_dirty( psTrans, nSibling );

					if( bt_join_right_to_left( psParent, psNode, nNode, psSibling, false ) )
					{
						kassertw( psNode->bn_nKeyCount > 0 );
						memcpy( psSibling, psNode, B_NODE_SIZE );
						continue;
					}
					else
					{
						kassertw( psNode->bn_nKeyCount > 0 );
						kassertw( psSibling->bn_nKeyCount > 0 );
						nError = bt_rename_parent_key( psVolume, psInode, psTrans, psStack, nSibling );
						if( nError < 0 )
						{
							printk( "Error: bt_remove_subnode:2() failed to rename parent key\n" );
							goto error;
						}
						break;
					}
				}
			}
			else
			{
				// If the node still is more than half filled we just fix the parent if needed.
				if( 0 == nIndex )
				{
					// If the first key is changed we must modify the GE key pointing to us from the parent
					kassertw( psNode->bn_nKeyCount > 0 );
					nError = bt_rename_parent_key( psVolume, psInode, psTrans, psStack, nNode );
					if( nError < 0 )
					{
						printk( "Error: bt_remove_subnode:3() failed to rename parent key\n" );
						goto error;
					}
				}
				break;
			}
		}
		else
		{
			// We delete a key from the root node. If the old root contained only one pointer
			// We set that pointer as the new root, and delete the old one.
			// This is the only way to decrease the height of a b+tree.
			bvalue_t nNewRoot = NULL_VAL;

			if( 0 == psNode->bn_nKeyCount )
			{
				nNewRoot = psNode->bn_nOverflow;
				kassertw( NULL_VAL != nNewRoot );
			}
			if( 1 == psNode->bn_nKeyCount && NULL_VAL == psNode->bn_nOverflow )
			{
				nNewRoot = B_KEY_VALUE_OFFSET( psNode )[0];
			}
			if( nNewRoot != NULL_VAL )
			{
				BTree_s *psTree =( BTree_s * ) bt_load_node( psVolume, psInode, psTrans, 0 );

				if( psTree == NULL )
				{
					printk( "Error: bt_remove_subnode() failed to load tree header\n" );
					nError = -EIO;
					goto error;
				}
				psTree->bt_nRoot = nNewRoot;
				psTree->bt_nTreeDepth--;

				kassertw( psTree->bt_nTreeDepth >= 0 );
				nError = bt_free_node( psVolume, psInode, psTrans, nNode );
				if( nError < 0 )
				{
					printk( "Error: bt_remove_subnode() failed to free node\n" );
					goto error;
				}
				bt_mark_node_dirty( psTrans, 0 );
			}
			break;
		}
	}
      error:
	return( nError );
}

/*****************************************************************************
 * NAME:
 * DESC:
 *	Lookup and delete one key from the tree.
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

status_t bt_delete_key( AfsVolume_s * psVolume, AfsInode_s * psInode, const void *pKey, int nKeyLen )
{
	BNode_s *psNode;
	bvalue_t nNode;
	BStack_s sStack;
	BStack_s *psStack = &sStack;
	BTransaction_s sTrans;
	BTransaction_s *psTrans = &sTrans;
	int nIndex;
	status_t nError;
	status_t nOldErr;

#ifdef AFS_PARANOIA
	if( psVolume == NULL )
	{
		panic( "bt_delete_key() called with psVolume == NULL\n" );
		return( -EINVAL );
	}
	if( psInode == NULL )
	{
		panic( "bt_delete_key() called with psInode == NULL\n" );
		return( -EINVAL );
	}
	if( pKey == NULL )
	{
		panic( "bt_delete_key() called with pKey == NULL\n" );
		return( -EINVAL );
	}
	if( nKeyLen <= 0 || nKeyLen > B_MAX_KEY_SIZE )
	{
		panic( "bt_delete_key() called with invalid key length %d\n", nKeyLen );
		return( -EINVAL );
	}
#endif
	memset( &sStack, 0, sizeof( sStack ) );

	if( psInode->ai_sData.ds_nSize == 0 )
	{
		printk( "Error: Attempt to remove key from empty b+tree!!\n" );
		return( -ENOENT );
	}
	nError = bt_begin_transaction( psStack, psTrans );
	if( nError < 0 )
	{
		return( nError );
	}
	kassertw( psStack->bn_nCurPos >= 0 );

	nError = bt_find_key( psVolume, psInode, psTrans, psStack, pKey, nKeyLen, &psNode );
	if( nError != 1 )
	{
		if( nError >= 0 )
		{
			nError = -ENOENT;
		}
		goto error;
	}
	kassertw( psStack->bn_nCurPos >= 0 );
	nIndex = psStack->bn_anPosition[psStack->bn_nCurPos];

	kassertw( psNode == psStack->bn_apsNodes[psStack->bn_nCurPos] );
	nNode = psStack->bn_apnStack[psStack->bn_nCurPos];

	kassertw( nIndex >= 0 );

	nError = bt_delete_key_from_node( psNode, nIndex );
	if( nError < 0 )
	{
		goto error;
	}
	bt_mark_node_dirty( psTrans, nNode );

	// If the node ends up less than half filled we joins it with one of it's siblings.

	if( psStack->bn_nCurPos > 0 && B_TOT_KEY_SIZE( psNode ) < ( ( B_NODE_SIZE - B_HEADER_SIZE ) / 2 ) )
	{
		BNode_s *psParent = psStack->bn_apsNodes[psStack->bn_nCurPos - 1];
		int nPos = psStack->bn_anPosition[psStack->bn_nCurPos - 1];
		bvalue_t *apsValues = B_KEY_VALUE_OFFSET( psParent );

		psStack->bn_nCurPos--;

		// Unless the node is the leftmost child of it's parent
		// we will steal some keys from the right sibling
		// If it is the leftmost we steal keys from the right sibling.
		if( nPos > 0 )
		{
			bvalue_t nSibling = apsValues[nPos - 1];
			BNode_s *psSibling = bt_load_node( psVolume, psInode, psTrans, nSibling );

			if( psSibling == NULL )
			{
				printk( "bt_delete_key:1() failed to load tree node\n" );
				nError = -EIO;
				goto error;
			}
			bt_assert_valid_node( psSibling );

			bt_mark_node_dirty( psTrans, nSibling );

			// Steel keys from the left sibling.
			if( bt_join_left_to_right( psInode, psParent, psNode, psSibling, nSibling, true ) )
			{
				// If we stole all keys from the left sibling we deletes it.
				// But first we must fix the doubly linked list of leaf nodes.

				psNode->bn_nLeft = psSibling->bn_nLeft;

				if( NULL_VAL != psSibling->bn_nLeft )
				{
					BNode_s *psTmp = bt_load_node( psVolume, psInode, psTrans, psSibling->bn_nLeft );

					if( psTmp == NULL )
					{
						printk( "bt_delete_key:2() failed to load tree node\n" );
						nError = -EIO;
						goto error;
					}
					bt_assert_valid_node( psTmp );
					psTmp->bn_nRight = nNode;
					bt_mark_node_dirty( psTrans, psSibling->bn_nLeft );
				}
				nError = bt_remove_subnode( psVolume, psInode, psTrans, psStack, nSibling );
				if( nError < 0 )
				{
					goto error;
				}
			}
			else
			{
				// If we changed the first key of a node, we must change the GE key
				// pointing to us to reflect the change.
				nError = bt_rename_parent_key( psVolume, psInode, psTrans, psStack, nNode );
				if( nError < 0 )
				{
					goto error;
				}
			}
		}
		else
		{
			bvalue_t nSibling =( psParent->bn_nKeyCount > 1 ) ? apsValues[1] : psParent->bn_nOverflow;
			BNode_s *psSibling = bt_load_node( psVolume, psInode, psTrans, nSibling );

			if( psSibling == NULL )
			{
				printk( "bt_delete_key:3() failed to load tree node\n" );
				nError = -EIO;
				goto error;
			}
			bt_assert_valid_node( psSibling );
			bt_mark_node_dirty( psTrans, nSibling );

			kassertw( NULL != psSibling );

			//  Join with right sibling.
			if( bt_join_right_to_left( psParent, psNode, nNode, psSibling, true ) )
			{
				bvalue_t nTmp;

				if( NULL_VAL != psNode->bn_nLeft )
				{
					BNode_s *psTmp = bt_load_node( psVolume, psInode, psTrans, psNode->bn_nLeft );

					if( psTmp == NULL )
					{
						printk( "bt_delete_key:4() failed to load tree node\n" );
						nError = -EIO;
						goto error;
					}
					bt_assert_valid_node( psTmp );
					psTmp->bn_nRight = nSibling;
					bt_mark_node_dirty( psTrans, psNode->bn_nLeft );
				}
				nTmp = psSibling->bn_nRight;
				memcpy( psSibling, psNode, B_NODE_SIZE );
				//    psSibling->bn_nLeft are copyed by the memcpy() above
				psSibling->bn_nRight = nTmp;
				nError = bt_remove_subnode( psVolume, psInode, psTrans, psStack, nNode );
				if( nError < 0 )
				{
					goto error;
				}
			}
			else
			{
				nError = bt_rename_parent_key( psVolume, psInode, psTrans, psStack, nSibling );
				if( nError < 0 )
				{
					goto error;
				}
			}
		}
	}
	else
	{
		if( nIndex == 0 && psStack->bn_nCurPos > 0 )
		{
			psStack->bn_nCurPos--;
			nError = bt_rename_parent_key( psVolume, psInode, psTrans, psStack, nNode );
			if( nError < 0 )
			{
				goto error;
			}
		}
	}
      error:
	nOldErr = nError;
	nError = bt_end_transaction( psVolume, psInode, psTrans, nError >= 0 );
	if( nError >= 0 )
	{
		return( nOldErr );
	}
	else
	{
		return( nError );
	}
}
