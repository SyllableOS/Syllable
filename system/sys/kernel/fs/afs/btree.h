
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

#ifndef __F_BTREE_H__
#define __F_BTREE_H__

//#define       B_NODE_SIZE     100
#define B_NODE_SIZE	(1024)
//#define B_NODE_SIZE  (2048 + 10 * 100)
//#define B_NODE_SIZE   2048

typedef struct _BNode BNode_s;


/** Types of keys in a btree
 * \par Description:
 * A btree can contain 5 types of keys: 32-bit integer, 64-bit integer, single-precision
 * floating point, double-precision floating point, and variable-length character string.
 ****************************************************************************/
enum
{
	e_KeyTypeNone,
	e_KeyTypeInt32,
	e_KeyTypeInt64,
	e_KeyTypeFloat,
	e_KeyTypeDouble,
	e_KeyTypeString
};

#define B_MAX_KEY_SIZE	256

typedef int64 bvalue_t;

#define NULL_VAL ((bvalue_t)-1)

/** A node in a btree
 * \par Description:
 * A node can be interior, or leaf.  Leaf nodes are doubly linked together for in-order
 * traversal.  A leaf is a node at depth bt_nTreeDepth.  Keys are stored packed at
 * bn_anKeyData, from greatest to least.  In order to accommodate variable-sized keys (ie
 * strings), the next element in the node is a packed array of 16-bit integers.  Each one
 * contains the offset into the bn_anKeyData array of the associated key.  Finally, the
 * values (always 64-bit integers) are packed after the indices.  Each key/value pair
 * points to a subtree containing keys strictly less than the key (and greater than or
 * equal to the previous key). In each node, therefore, something must point to the
 * subtree with keys greater than or equal to the last key in the node.  This is pointed
 * to by bn_nOverflow.
 * +-----------------------+
 * |      NODE HEADER      |
 * +-----------------------+
 * |         KEYS          |
 * +-----------------------+
 * | ARRAY OF KEY POSITION |
 * +-----------------------+
 * |    ARRAY OF VALUES    |
 * +-----------------------+
 ****************************************************************************/
struct _BNode
{
	bvalue_t bn_nLeft;
	bvalue_t bn_nRight;
	bvalue_t bn_nOverflow;
	int bn_nKeyCount;
	int bn_nTotKeySize;
	char bn_anKeyData[1];
};

/** Entry in a transaction
 * \par Description:
 * This is an entry in a btree transaction.  It contains a pointer to a node that is
 * locked, the btree value location (disk location) of that node, and a boolean to say
 * whether or not this block is dirty.
 * \sa
 * BTransaction_s
 ****************************************************************************/
typedef struct
{
	BNode_s *be_psNode;
	bvalue_t be_nValue;
	off_t be_nBlock;
	bool be_bDirty;
} BNodeEntry_s;

#define BTREE_MAGIC 0x65768995

/** Btree header
 * \par Description:
 * This is the on-disk header for a btree.  bt_nFirstFree is a linked list of nodes that
 * have been freed from the tree.  This keeps holes of unused blockes from building up in
 * the tree.
 ****************************************************************************/
typedef struct
{
	uint32 bt_nMagic;
	bvalue_t bt_nRoot;
	int bt_nTreeDepth;
	bvalue_t bt_nLastNode;
	bvalue_t bt_nFirstFree;
} BTree_s;

/** Btree traversal stack
 * \par Description:
 * Many operations require knowledge of the parent of a node, and the parent of the
 * parent of a node, and so on.  In order to avoid the need to store locations of parents
 * in the nodes, a stack is used.  When walking down the tree, each node is added to the
 * stack.  So, the parent of a given stack position is in the position above it.
 * \par Warning:
 * This limites the depth of the tree to 100.  This may not be a problem, I haven't
 * checked.
 ****************************************************************************/
typedef struct
{
	int bn_nCurPos;
	bvalue_t bn_apnStack[100];
	BNode_s *bn_apsNodes[100];
	int bn_anPosition[100];
} BStack_s;

/** Btree transaction
 * \par Description:
 * In order to batch disk I/O to a btree, all access is done in transactions.  A
 * transaction consists of a list of touched nodes.  When an action is started, for
 * example inserting a new key, a transaction is created.  Then, all nodes touched during
 * the action are added to the transaction, and marked dirty if they are changed.  Then,
 * when the action is complete, the transaction is finalized, which will write out any
 * dirty noded to the disk.
 ****************************************************************************/
typedef struct
{
	BNodeEntry_s bt_apsLockedNodes[100];
	int bt_nLockedNodeCnt;
} BTransaction_s;

/** Btree iterator
 * \par Description:
 * Some actions are required to be split up into separate events for concurancy reasons.
 * For example, reading a directory is done by reoccuring calls to readdir().  In order
 * to accomplish this, the first call will allocate and return an iterator, which is used
 * to record the current position of the walk, allowing the next dataset to be returned.
 ****************************************************************************/
typedef struct
{
	bvalue_t bi_nCurNode;
	int bi_nCurKey;
	bvalue_t bi_nCurValue;
	char bi_anKey[B_MAX_KEY_SIZE];
	int bi_nKeySize;
	uint32 bi_nVersion;	/* Tree version used to validate the iterator. */
} BIterator_s;

#define	B_HEADER_SIZE ((int)&(((BNode_s*)0)->bn_anKeyData[0]))

/*
  +-----------------------+
  |      NODE HEADER      |
  +-----------------------+
  |         KEYS          |
  +-----------------------+
  | ARRAY OF KEY POSITION |
  +-----------------------+
  |    ARRAY OF VALUES    |
  +-----------------------+
 */

#if 0
#define	B_KEY_INDEX_OFFSET( node )((int16*) ((((int)(node)) + B_HEADER_SIZE + (node)->bn_nTotKeySize + 3) & ~3))
#define	B_KEY_VALUE_OFFSET( node )((bvalue_t*) (((int)B_KEY_INDEX_OFFSET( node )) + (node)->bn_nKeyCount * 2))
#else

/** Get the index array from a node
 * \par Description:
 * Given a node, return the index array in the node.
 * \par Note:
 * \par Warning:
 * \param node	Node
 * \return index array of node
 * \sa
 ****************************************************************************/
static inline int16 *B_KEY_INDEX_OFFSET( BNode_s *node )
{
	return( ( ( int16 * )( ( ( ( int )( node ) ) + B_HEADER_SIZE + ( node )->bn_nTotKeySize + 3 ) & ~3 ) ) );
}

/** Get the (const) index array from a node
 * \par Description:
 * Given a node, return a const reference to the index array in the node.
 * \par Note:
 * \par Warning:
 * \param node	Node
 * \return const index array of node
 * \sa
 ****************************************************************************/
static inline const int16 *B_KEY_INDEX_OFFSET_CONST( const BNode_s *node )
{
	return( ( ( const int16 * )( ( ( ( int )( node ) ) + B_HEADER_SIZE + ( node )->bn_nTotKeySize + 3 ) & ~3 ) ) );
}

/** Get the value array from a node
 * \par Description:
 * Given a node, return the value array in the node.
 * \par Note:
 * \par Warning:
 * \param node	Node
 * \return value array of node
 * \sa
 ****************************************************************************/
static inline bvalue_t *B_KEY_VALUE_OFFSET( BNode_s *node )
{
	return( ( ( bvalue_t * )( ( ( int )B_KEY_INDEX_OFFSET( node ) ) + ( node )->bn_nKeyCount * 2 ) ) );
}

/** Get the (const) value array from a node
 * \par Description:
 * Given a node, return a const reference to the value array in the node.
 * \par Note:
 * \par Warning:
 * \param node	Node
 * \return const value array of node
 * \sa
 ****************************************************************************/
static inline const bvalue_t *B_KEY_VALUE_OFFSET_CONST( const BNode_s *node )
{
	return( ( ( const bvalue_t * )( ( ( int )B_KEY_INDEX_OFFSET_CONST( node ) ) + ( node )->bn_nKeyCount * 2 ) ) );
}

#endif

#define	B_SIZEOF_VALUE	   sizeof(bvalue_t)
#define	B_SIZEOF_KEYOFFSET sizeof(int16)

#define	B_TOT_KEY_SIZE( node )((((node)->bn_nTotKeySize + 3) & ~3) + \
				(node)->bn_nKeyCount *(B_SIZEOF_KEYOFFSET + B_SIZEOF_VALUE))

#define bt_assert_valid_node( psNode ) \
{ \
  const int16*	pnKeyIndexes = B_KEY_INDEX_OFFSET_CONST( psNode ); \
  int			_i_; \
  kassertw( psNode != NULL );						\
  kassertw(((int*)(((char*)psNode) + B_NODE_SIZE))[0] = 0x12345678 ); \
  kassertw((psNode)->bn_nKeyCount < 1000 ); \
  for( _i_ = 1 ; _i_ < psNode->bn_nKeyCount ; ++_i_ ) { \
    kassertw( pnKeyIndexes[_i_] > pnKeyIndexes[_i_-1] ); \
  } \
}

/** Get a key from a node
 * \par Description:
 * Get the key with the given index from the given node.  Return it in ppKey and it's
 * length in pnSize.
 * \par Note:
 * \par Warning:
 * \param psNode	Node containing key
 * \param nIndex	Index of desired key
 * \param ppKey		Pointer to return key
 * \param pnSize	Return of size of key
 * \return none
 * \sa
 ****************************************************************************/
static inline void bt_get_key( const BNode_s *psNode, int nIndex, const void **ppKey, int *pnSize )
{
	const int16 *pnKeyIndexes = B_KEY_INDEX_OFFSET_CONST( psNode );
	const int16 nKeyEnd = pnKeyIndexes[nIndex];
	const int16 nKeyStart =( nIndex > 0 ) ? pnKeyIndexes[nIndex - 1] : 0;

	*ppKey = &psNode->bn_anKeyData[nKeyStart];
	*pnSize = nKeyEnd - nKeyStart;
}



/*** This functions are callable from anywhere in the filesystem ***/

int bt_insert_key( AfsVolume_s * psVolume, AfsInode_s * psInode, const void *pKey, int nKeySize, bvalue_t nValue );
int bt_find_first_key( AfsVolume_s * psVolume, AfsInode_s * psInode, BIterator_s * psIterator );
int bt_lookup_key( AfsVolume_s * psVolume, AfsInode_s * psInode, const void *pKey, int nKeySize, BIterator_s * psIterator );
int bt_get_next_key( AfsVolume_s * psVolume, AfsInode_s * psInode, BIterator_s * psIterator );

status_t bt_delete_key( AfsVolume_s * psVolume, AfsInode_s * psInode, const void *pKey, int nKeyLen );


/*** PRIVATE FUNCTIONS FOR THE B+TREE IMPLEMENTATION ***/

int bt_begin_transaction( BStack_s * psStack, BTransaction_s * psTrans );
int bt_end_transaction( AfsVolume_s * psVolume, AfsInode_s * psInode, BTransaction_s * psTrans, bool bOkToWrite );

BNode_s *bt_load_node( AfsVolume_s * psVolume, AfsInode_s * psInode, BTransaction_s * psTrans, bvalue_t nNode );
void bt_mark_node_dirty( BTransaction_s * psTrans, bvalue_t nNode );
status_t bt_free_node( AfsVolume_s * psVolume, AfsInode_s * psInode, BTransaction_s * psTrans, bvalue_t nNode );

int bt_find_key( AfsVolume_s * psVolume, AfsInode_s * psInode, BTransaction_s * psTrans, BStack_s * psStack, const void *pKey, int nKeySize, BNode_s **ppsNode );
bool bt_will_key_fit( const BNode_s *psNode, int nKeySize );

status_t bt_append_key( BNode_s *psNode, const void *pKey, int nKeySize, bvalue_t nValue );
status_t bt_insert_key_to_node( AfsInode_s * psInode, BNode_s *psNode, const void *a_pKey, int a_nKeySize, bvalue_t nValue );

status_t bt_add_key_to_node( AfsVolume_s * psVolume, AfsInode_s * psInode, BTransaction_s * psTrans, BStack_s * psStack, BNode_s *psNode, bvalue_t nNode, const void *pKey, int nKeyLen, bvalue_t psValue );


/* Some debug functions */
int bt_list_tree( AfsVolume_s * psVolume, AfsInode_s * psInode );
int test_btree( AfsVolume_s * psVolume, AfsInode_s * psInode );


#endif /* __F_BTREE_H__ */
