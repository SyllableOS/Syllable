/*
 *  AFS - The native AtheOS file-system
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

#ifndef __F_BTREE_H__
#define __F_BTREE_H__

//#define	B_NODE_SIZE	100
#define B_NODE_SIZE	(1024)
//#define B_NODE_SIZE	(2048 + 10 * 100)
//#define B_NODE_SIZE	2048

typedef struct _BNode BNode_s;



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

typedef struct
{
    BNode_s*	be_psNode;
    bvalue_t	be_nValue;
    off_t	be_nBlock;
    bool	be_bDirty;
} BNodeEntry_s;

#define BTREE_MAGIC 0x65768995

typedef struct
{
    uint32	bt_nMagic;
    bvalue_t 	bt_nRoot;
    int	   	bt_nTreeDepth;
    bvalue_t	bt_nLastNode;
    bvalue_t	bt_nFirstFree;
} BTree_s;

typedef struct
{
    int	     bn_nCurPos;
    bvalue_t bn_apnStack[ 100 ];
    BNode_s* bn_apsNodes[ 100 ];
    int	     bn_anPosition[ 100 ];
} BStack_s;

typedef struct
{
    BNodeEntry_s bt_apsLockedNodes[ 100 ];
    int	   	 bt_nLockedNodeCnt;
} BTransaction_s;

typedef struct
{
    bvalue_t	bi_nCurNode;
    int		bi_nCurKey;
    bvalue_t	bi_nCurValue;
    char	bi_anKey[B_MAX_KEY_SIZE];
    int		bi_nKeySize;
    uint32	bi_nVersion;	/* Tree version used to validate the iterator. */
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
#define	B_KEY_INDEX_OFFSET( node ) ((int16*) ((((int)(node)) + B_HEADER_SIZE + (node)->bn_nTotKeySize + 3) & ~3))
#define	B_KEY_VALUE_OFFSET( node ) ((bvalue_t*) (((int)B_KEY_INDEX_OFFSET( node )) + (node)->bn_nKeyCount * 2))
#else

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

#endif

#define	B_SIZEOF_VALUE	   sizeof(bvalue_t)
#define	B_SIZEOF_KEYOFFSET sizeof(int16)

#define	B_TOT_KEY_SIZE( node ) ((((node)->bn_nTotKeySize + 3) & ~3) + \
				(node)->bn_nKeyCount * (B_SIZEOF_KEYOFFSET + B_SIZEOF_VALUE))

#define bt_assert_valid_node( psNode ) \
{ \
  const int16*	pnKeyIndexes = B_KEY_INDEX_OFFSET_CONST( psNode ); \
  int			_i_; \
  kassertw( psNode != NULL );						\
  kassertw( ((int*)(((char*)psNode) + B_NODE_SIZE))[0] = 0x12345678 ); \
  kassertw( (psNode)->bn_nKeyCount < 1000 ); \
  for ( _i_ = 1 ; _i_ < psNode->bn_nKeyCount ; ++_i_ ) { \
    kassertw( pnKeyIndexes[_i_] > pnKeyIndexes[_i_-1] ); \
  } \
}

static inline void bt_get_key( const BNode_s* psNode, int nIndex, const void** ppKey, int* pnSize )
{
    const int16* pnKeyIndexes = B_KEY_INDEX_OFFSET_CONST( psNode );
    const int16  nKeyEnd      = pnKeyIndexes[nIndex];
    const int16  nKeyStart    = ( nIndex > 0 ) ? pnKeyIndexes[nIndex - 1] : 0;

    *ppKey  = &psNode->bn_anKeyData[nKeyStart];
    *pnSize = nKeyEnd - nKeyStart;
}



/*** This functions are callable from anywhere in the filesystem ***/

int bt_insert_key( AfsVolume_s* psVolume, AfsInode_s* psInode,
		   const void* pKey, int nKeySize, bvalue_t nValue );
int bt_find_first_key( AfsVolume_s* psVolume, AfsInode_s* psInode, BIterator_s* psIterator );
int bt_lookup_key( AfsVolume_s* psVolume, AfsInode_s* psInode,
		   const void* pKey, int nKeySize, BIterator_s* psIterator );
int bt_get_next_key( AfsVolume_s* psVolume, AfsInode_s* psInode, BIterator_s* psIterator );

status_t bt_delete_key( AfsVolume_s* psVolume, AfsInode_s* psInode, const void* pKey, int nKeyLen );


/*** PRIVATE FUNCTIONS FOR THE B+TREE IMPLEMENTATION ***/

int 	 bt_begin_transaction( BStack_s* psStack, BTransaction_s* psTrans );
int 	 bt_end_transaction( AfsVolume_s* psVolume, AfsInode_s* psInode, BTransaction_s* psTrans, bool bOkToWrite );

BNode_s* bt_load_node( AfsVolume_s* psVolume, AfsInode_s* psInode, BTransaction_s* psTrans, bvalue_t nNode );
void 	 bt_mark_node_dirty( BTransaction_s* psTrans, bvalue_t nNode );
status_t bt_free_node( AfsVolume_s* psVolume, AfsInode_s* psInode, BTransaction_s* psTrans, bvalue_t nNode );

int 	 bt_find_key( AfsVolume_s* psVolume, AfsInode_s* psInode, BTransaction_s* psTrans, BStack_s* psStack,
		      const void* pKey, int nKeySize, BNode_s** ppsNode );
bool	 bt_will_key_fit( const BNode_s* psNode, int nKeySize );

status_t bt_append_key( BNode_s* psNode, const void* pKey, int nKeySize, bvalue_t nValue );
status_t bt_insert_key_to_node( AfsInode_s* psInode, BNode_s* psNode,
				const void* a_pKey, int a_nKeySize, bvalue_t nValue );

status_t bt_add_key_to_node( AfsVolume_s* psVolume, AfsInode_s* psInode, BTransaction_s* psTrans,
			     BStack_s* psStack, BNode_s* psNode, bvalue_t nNode,
			     const void* pKey, int nKeyLen, bvalue_t psValue );


/* Some debug functions */
int bt_list_tree( AfsVolume_s* psVolume, AfsInode_s* psInode );
int test_btree( AfsVolume_s* psVolume, AfsInode_s* psInode );


#endif /* __F_BTREE_H__ */
