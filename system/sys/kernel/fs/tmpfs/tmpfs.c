
/*
 *  The Syllable kernel
 *  Copyright (C) 1999 - 2000 Kurt Skauen
 *  Copyright (C) 2004 Daniel Gryniewicz
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
#include <posix/stat.h>
#include <posix/fcntl.h>
#include <posix/dirent.h>

#include <atheos/types.h>
#include <atheos/time.h>
#include <atheos/filesystem.h>
#include <atheos/kernel.h>
#include <atheos/semaphore.h>
#include <atheos/bootmodules.h>
#include "list.h"

#include <macros.h>

#define TFS_DEBUG 1

#ifdef TFS_DEBUG
#define TFS_VOLUME_LOCK(volume) do { printk("%s locking volume; count %d\n", __FUNCTION__, (volume)->tv_nLockCount); LOCK( (volume)->tv_hLock ); (volume)->tv_nLockCount++; } while (0)
#define TFS_VOLUME_UNLOCK(volume) do { printk("%s unlocking volume; count %d\n", __FUNCTION__, (volume)->tv_nLockCount); UNLOCK( (volume)->tv_hLock ); (volume)->tv_nLockCount--; } while (0)
#define TFS_INODE_LOCK(inode) do { printk("%s locking inode %s (%#0lx); count %d\n", __FUNCTION__, (inode)->ti_zName, (inode)->ti_nInodeNum, (inode)->ti_nLockCount); LOCK( (inode)->ti_hLock ); (inode)->ti_nLockCount++; } while (0)
#define TFS_INODE_UNLOCK(inode) do { printk("%s unlocking inode %s (%#0lx); count %d\n", __FUNCTION__, (inode)->ti_zName, (inode)->ti_nInodeNum, (inode)->ti_nLockCount); UNLOCK( (inode)->ti_hLock ); (inode)->ti_nLockCount--; } while (0)
#else // TFS_DEBUG
#error "No TFS_DEBUG!"
#define TFS_VOLUME_LOCK(volume) LOCK( (volume)->tv_hLock )
#define TFS_VOLUME_UNLOCK(volume) UNLOCK( (volume)->tv_hLock )
#define TFS_INODE_LOCK(inode) LOCK( (inode)->ti_hLock )
#define TFS_INODE_UNLOCK(inode) UNLOCK( (inode)->ti_hLock )
#endif // TFS_DEBUG

typedef struct TmpFSInode TmpFSInode_s;
typedef struct TmpFSWalk TmpFSWalk_s;
typedef LIST_HEAD(, TmpFSWalk) TmpFSWalkList_s;
typedef LIST_HEAD(, TmpFSInode) TmpFSInodeList_s;

#define	TFS_MAX_NAME_LEN	64
#define TFS_MAX_FILESIZE	(1024*1024*16)

enum
{
	TFS_ROOT = 1,
	TFS_INDEXDIR = 2,
};

#define S_IFATTR 0200000
#define S_ISATTR(m) (((m) & S_IFATTR) == S_IFATTR)
#define S_IFINDEX 0400000
#define S_ISINDEX(m) (((m) & S_IFINDEX) == S_IFINDEX)

/*
 * DFG TODO:
 * - Attribute index support
 * - Check kmalloc for saneness
 *
 */

/** tmpfs Inode
 * \par Description:
 * This is an Inode on tmpfs
 ****************************************************************************/
struct TmpFSInode
{
	TmpFSInodeList_s	 ti_sChildren;	///< Contents of directory, if directory
	TmpFSInodeList_s	 ti_sAttributes;///< Contents of Attribute directory
	LIST_ENTRY(TmpFSInode)	 ti_sLink;	///< Link into ti_sChildren or ti_sAttributes list
	TmpFSInode_s		*ti_psParent;	///< Containing directory
	long			 ti_nInodeNum;	///< Inode number
	mode_t			 ti_nMode;	///< Mode and flags
	int			 ti_nSize;	///< Size of file
	time_t			 ti_nCTime;	///< Creation time
	time_t			 ti_nMTime;	///< Modification time
	uid_t			 ti_nUID;	///< Owner UID
	gid_t			 ti_nGID;	///< Owner GID
	int			 ti_nLinkCount;	///< Number of links
	int			 ti_nNameLen;	///< Length of filename
	sem_id			 ti_hLock;	///< Lock protects access to ti_pBuffer; If tv_hLock is taken too, it must be taken first
#ifdef TFS_DEBUG
	int			 ti_nLockCount;	///< Count of taken locks
#endif // TFS_DEBUG
	char			 ti_zName[TFS_MAX_NAME_LEN];	///< Filename
	char			*ti_pBuffer;	///< Contents of file
	fsattr_type		 ti_eAttrType;	///< If attribute, type of data
	TmpFSWalkList_s		 ti_sWalks;	///< Currently open walks
	bool			 ti_bIsLoaded;	///< VFS has reference to Inode
};

/** tmpfs Volume descriptor
 * \par Description:
 * This contains information about a mounted volume
 ****************************************************************************/
typedef struct
{
	kdev_t		 tv_nDevNum;	///< Device number from mount
	sem_id		 tv_hLock;	///< Lock protects inode tree changes
	TmpFSInode_s	*tv_psRootNode;	///< Root inode
	TmpFSInode_s	*tv_psIndexDir;	///< Index directory inode
	int		 tv_nFSSize;	///< Total filesystem size
#ifdef TFS_DEBUG
	int		 tv_nLockCount;	///< Count of taken locks
#endif // TFS_DEBUG
} TmpFSVolume_s;

typedef enum {
	TMPFS_WALK_DIR = 0,	///< Directory walk
	TMPFS_WALK_ATTR		///< Attribute walk
} TmpFSWalkType_e;

/** tmpfs walk
 * \par Description:
 * This structure represents a walk of a tree
 ****************************************************************************/
struct TmpFSWalk
{
	LIST_ENTRY(TmpFSWalk)	 tw_sLink;	///< Link into list of walks
	TmpFSInode_s		*tw_psTree;	///< Tree being walked
	TmpFSInode_s		*tw_psCurNode;	///< Current node
	TmpFSWalkType_e		 tw_eType;	///< Walk type
};

/** tmpfs cookie directory state
 * \par Description:
 * This is the current state of a tmpfs cookie.  This is needed because "." and ".." are not
 * actually entries in a directory, but are faked.
 ****************************************************************************/
typedef enum {
	TMPFS_COOKIE_DOT = 0,	///< print dot next
	TMPFS_COOKIE_DOT_DOT,	///< print dot dot next
	TMPFS_COOKIE_WALK	///< print next walk entry
} TmpFSCookieState_e;

/** tmpfs cookie
 * \par Description:
 * This is the cookie returned from open() and passed to read(), write(), close(), readdir(), and
 * rewinddir().  It contains the open mode, because that's necessary to see if the file was opened
 * append.  In addition, it contains the readdir state, so an O(1) walk is possible.
 ****************************************************************************/
typedef struct
{
	int			 tc_nMode;	///< Open mode
	TmpFSWalk_s		*tc_psWalk;	///< Directory walk
	TmpFSCookieState_e	 tc_nState;	///< State; 
} TmpFSCookie_s;

/*
 * These functions handle the tree code withing tmpfs.
 *
 * All of these MUST be called with the volume locked
 */
/* Walks */
static TmpFSWalk_s *tfs_tree_walk_start( TmpFSInode_s *psTree, TmpFSWalkType_e eType );
static void tfs_tree_walk_end( TmpFSWalk_s *psWalk );
static void tfs_tree_walk_reset( TmpFSWalk_s *psWalk );
static TmpFSInode_s *tfs_tree_walk_next( TmpFSWalk_s *psWalk );
static void tfs_tree_walk_fixup( TmpFSInode_s *psTree );
/* Info */
static bool tfs_tree_is_empty( TmpFSInode_s *psTree );
/* Tree maintainance */
static TmpFSInode_s *tfs_tree_find( TmpFSInode_s *psTree, const char *pzName, int nNameLen );
static void tfs_tree_insert( TmpFSInode_s *psTree, TmpFSInode_s *psNode );
static void tfs_tree_remove( TmpFSInode_s *psTree, TmpFSInode_s *psNode );
static void tfs_tree_delete_all( TmpFSInode_s *psTree );
static TmpFSInode_s *tfs_tree_find_attr( TmpFSInode_s *psTree, const char *pzName, int nNameLen );
static void tfs_tree_insert_attr( TmpFSInode_s *psTree, TmpFSInode_s *psNode );
/* Needed by tfs_tree_delete_all */
static void tfs_delete_node( TmpFSInode_s *psNode );

/** Start a new tree walk
 * \par Description:
 * Start a new tree walk for the given tree.  Returned cookie is suitable as a cookie for
 * open[_attrdir].  Walks are linked into the root of the tree they're walking, so they can be fixed
 * up when an entry in that tree is deleted.
 * \par Note:
 * \par Locks:
 * This must be called with tv_hLock (volume lock) LOCKED
 * \par Warning:
 * \param psTree	Tree to walk
 * \param eType		Walk type
 * \return Cookie for walking the tree, or NULL on -ENOMEM
 * \sa
 ****************************************************************************/
static TmpFSWalk_s *tfs_tree_walk_start( TmpFSInode_s *psTree, TmpFSWalkType_e eType )
{
	TmpFSWalk_s *psWalk;

	printk("%s\n", __FUNCTION__);
	psWalk = kmalloc( sizeof( TmpFSWalk_s ), MEMF_KERNEL | MEMF_CLEAR | MEMF_OKTOFAILHACK );
	if ( psWalk == NULL )
	{
		return ( NULL );
	}

	psWalk->tw_psTree = psTree;
	psWalk->tw_eType = eType;

	switch ( eType )
	{
		case TMPFS_WALK_DIR:
			psWalk->tw_psCurNode = LIST_FIRST( &psTree->ti_sChildren );
			break;
		case TMPFS_WALK_ATTR:
			psWalk->tw_psCurNode = LIST_FIRST( &psTree->ti_sAttributes );
			break;
	}
	LIST_ADDHEAD(&psTree->ti_sWalks, psWalk, tw_sLink);

	return ( psWalk );
}

/** Stop a tree walk
 * \par Description:
 * Stop the given tree walk.  Frees the walk pointer.  Remove the walk from the tree it's walking.
 * \par Note:
 * \par Locks:
 * This must be called with tv_hLock (volume lock) LOCKED
 * \par Warning:
 * \param psWalk	Cookie to free
 * \sa
 ****************************************************************************/
static void tfs_tree_walk_end( TmpFSWalk_s *psWalk )
{
	printk("%s\n", __FUNCTION__);
	LIST_REMOVE(psWalk, tw_sLink);
	kfree(psWalk);
}

/** Reset a directory cookie
 * \par Description:
 * Reset the given cookie to point to the beginning of it's directory
 * \par Note:
 * \par Locks:
 * This must be called with tv_hLock (volume lock) LOCKED
 * \par Warning:
 * \param psWalk	Cookie to reset
 * \sa
 ****************************************************************************/
static void tfs_tree_walk_reset( TmpFSWalk_s *psWalk )
{
	printk("%s\n", __FUNCTION__);
	switch ( psWalk->tw_eType )
	{
		case TMPFS_WALK_DIR:
			psWalk->tw_psCurNode = LIST_FIRST( &psWalk->tw_psTree->ti_sChildren );
			break;
		case TMPFS_WALK_ATTR:
			psWalk->tw_psCurNode = LIST_FIRST( &psWalk->tw_psTree->ti_sAttributes );
			break;
	}
}

/** Get the next node in a walk
 * \par Description:
 * \par Note:
 * \par Locks:
 * This must be called with tv_hLock (volume lock) LOCKED
 * \par Warning:
 * \param
 * \return
 * \sa
 ****************************************************************************/
static TmpFSInode_s *tfs_tree_walk_next( TmpFSWalk_s *psWalk )
{
	TmpFSInode_s *psNode;

	printk("%s\n", __FUNCTION__);
	psNode = psWalk->tw_psCurNode;
	if ( psWalk->tw_psCurNode != NULL )
	{
		psWalk->tw_psCurNode = LIST_NEXT(psWalk->tw_psCurNode, ti_sLink);
	}
	return (psNode);
}

/** Fixup walks
 * \par Description:
 * Fix the walks currently pointing to the given node, because it's going away.
 * \par Note:
 * \par Locks:
 * This must be called with tv_hLock (volume lock) LOCKED
 * \par Warning:
 * \param psTree	Tree node going away
 * \sa
 ****************************************************************************/
static void tfs_tree_walk_fixup( TmpFSInode_s *psTree )
{
	TmpFSWalk_s *psWalk;

	printk("%s\n", __FUNCTION__);
	if (psTree->ti_psParent == NULL)
	{
		return;
	}

	psWalk = LIST_FIRST(&psTree->ti_psParent->ti_sWalks);
	while (psWalk)
	{
		if (psWalk->tw_psCurNode == psTree)
		{
			tfs_tree_walk_next(psWalk);
		}
		psWalk = LIST_NEXT(psWalk, tw_sLink);
	}
}

/** Check to see if the given node is empty
 * \par Description:
 * Check to see if the given node has any children
 * \par Note:
 * \par Locks:
 * This must be called with tv_hLock (volume lock) LOCKED
 * \par Warning:
 * \param psTree	Node to check
 * \return true if the node has no children, false otherwise
 * \sa
 ****************************************************************************/
static bool tfs_tree_is_empty( TmpFSInode_s *psTree )
{
	return ( LIST_IS_EMPTY( &psTree->ti_sChildren ) );
}

/** Delete all the nodes from a tree
 * \par Description:
 * Delete all the nodes under the given Inode.  Recursively calls this to delete all it's children,
 * then deletes itself.
 * \par Note:
 * \par Locks:
 * Must be called with tv_hLock (volume lock) LOCKED
 * \par Warning:
 * \param psTree	Root of tree to delete
 * \sa
 ****************************************************************************/
static void tfs_tree_delete_all( TmpFSInode_s *psTree )
{
	TmpFSInode_s *psNode;

	printk("%s\n", __FUNCTION__);
	while( ( psNode = LIST_FIRST( &psTree->ti_sChildren ) ) )
	{
		tfs_tree_delete_all( psNode );
	}
	tfs_delete_node( psTree );
}

/** Look up a filename in a directory
 * \par Description:
 * Lookup the given filename in the given directory.  Currently, each directory is a doubly linked
 * list of inodes.  Walk the list comparing the given name against the names of the inodes until a
 * match is found or not.  Special case "." and ".."
 * \par Note:
 * \par Locks:
 * This must be called with tv_hLock (volume lock) LOCKED
 * \par Warning:
 * \param psTree	Directory to search
 * \param pzName	Name to find
 * \param nNameLen	Length of pzName.
 * \return Inode if found, or NULL otherwise.
 * \sa
 ****************************************************************************/
static TmpFSInode_s *tfs_tree_find( TmpFSInode_s *psTree, const char *pzName, int nNameLen )
{
	TmpFSInode_s *psNode;

	printk("%s \"%s\"/\"%s\"\n", __FUNCTION__, psTree->ti_zName, pzName);
	if( nNameLen == 1 && '.' == pzName[0] )
	{
		return ( psTree );
	}
	if( nNameLen == 2 && '.' == pzName[0] && '.' == pzName[1] )
	{
		if( psTree->ti_psParent != NULL )
		{
			return ( psTree->ti_psParent );
		}
		else
		{ // This should not happen, because the VFS handles it
			printk( "Error: tfs_tree_find() called with .. on root level\n" );
			return ( NULL );
		}
	}
	for( psNode = LIST_FIRST( &psTree->ti_sChildren ); psNode != NULL; psNode = LIST_NEXT( psNode, ti_sLink ) )
	{
		if( psNode->ti_nNameLen == nNameLen && strncmp( psNode->ti_zName, pzName, nNameLen ) == 0 )
		{
			printk("%s found\n", __FUNCTION__);
			return ( psNode );
		}
	}
	printk("%s not found\n", __FUNCTION__);
	return ( NULL );
}

/** Look up an attribute by name
 * \par Description:
 * Lookup the given attribute name in the given Inode.  Currently, each Inode is a doubly linked
 * list of attributes.  Walk the list comparing the given name against the names of the inodes until a
 * match is found or not.
 * \par Note:
 * \par Locks:
 * This must be called with tv_hLock (volume lock) LOCKED
 * \par Warning:
 * \param psNode	Inode to search
 * \param pzName	Name to find
 * \param nNameLen	Length of pzName.
 * \return Attribute if found, or NULL otherwise.
 * \sa
 ****************************************************************************/
static TmpFSInode_s *tfs_tree_find_attr( TmpFSInode_s *psNode, const char *pzName, int nNameLen )
{
	TmpFSInode_s *psAttr;

	printk("%s \"%s\"::\"%s\"\n", __FUNCTION__, psNode->ti_zName, pzName);
	for( psAttr = LIST_FIRST( &psNode->ti_sAttributes ); psAttr != NULL; psAttr = LIST_NEXT( psAttr, ti_sLink ) )
	{
		if( psAttr->ti_nNameLen == nNameLen && strncmp( psAttr->ti_zName, pzName, nNameLen ) == 0 )
		{
			printk("%s found\n", __FUNCTION__);
			return ( psAttr );
		}
	}
	printk("%s not found\n", __FUNCTION__);
	return ( NULL );
}

/** Insert an Inode into a directory
 * \par Description:
 * Insert the given Inode into the given directory.  Assumes the Inode does not exist.  Currently,
 * the directory entries are an unordered, doubly linked list.  Thus, this is just a head-insert.
 * \par Note:
 * \par Locks:
 * Must be called with tv_hLock (volume lock) LOCKED
 * \par Warning:
 * \param psTree	Directory to add to
 * \param psNode	Node to add
 * \sa
 ****************************************************************************/
static void tfs_tree_insert( TmpFSInode_s *psTree, TmpFSInode_s *psNode )
{
	LIST_ADDHEAD( &psTree->ti_sChildren, psNode, ti_sLink );
	psNode->ti_psParent = psTree;
}

/** Insert an Attribute into an Inode
 * \par Description:
 * Insert the given Attribute into the given Inode.  Assumes the Attribute does not exist.
 * Currently, the Attributes are an unordered, doubly linked list.  Thus, this is just a
 * head-insert.
 * \par Note:
 * \par Locks:
 * Must be called with tv_hLock (volume lock) LOCKED
 * \par Warning:
 * \param psTree	Directory to add to
 * \param psNode	Node to add
 * \sa
 ****************************************************************************/
static void tfs_tree_insert_attr( TmpFSInode_s *psTree, TmpFSInode_s *psNode )
{
	printk("%s \"%s\"::\"%s\"\n", __FUNCTION__, psTree->ti_zName, psNode->ti_zName);
	LIST_ADDHEAD( &psTree->ti_sAttributes, psNode, ti_sLink );
	psNode->ti_psParent = psTree;
}

/** Remove an Inode from a directory
 * \par Description:
 * Remove the given Inode from the given directory.  Currently, the directory entries are an
 * unordered, doubly linked list.  This allows O(1) unlinking of the node.  It is not freed.  After
 * this returns, none of the tree-related pointers are valid anymore.
 * \par Note:
 * \par Locks:
 * Must be called with tv_hLock (volume lock) LOCKED
 * \par Todo:
 * Make doublly linked, so this becomes O(1).
 * \par Warning:
 * \param psTree	Directory to remove from
 * \param psNode	Node to remove
 * \sa
 ****************************************************************************/
static void tfs_tree_remove( TmpFSInode_s *psTree, TmpFSInode_s *psNode )
{
	printk("%s %s\n", __FUNCTION__, psNode->ti_zName);

	if ( psNode->ti_psParent == NULL )
	{ // Nothing to do
		return;
	}

	kassertw( psNode->ti_psParent == psTree );

	// Fix up walks
	tfs_tree_walk_fixup( psNode );

	LIST_REMOVE( psNode, ti_sLink );
	psNode->ti_psParent = NULL;
}

/** Create a new Inode
 * \par Description:
 * Create a new inode.  Allocate a new Inode, If S_IFATTR is set, link it into the head of the
 * Inodes's attribute list.  Otherwise, link it into the head of the Inode's children list, and
 * initialize all it's data.
 * \par Note:
 * \par Locks:
 * Must be called with tv_hLock (volume lock) LOCKED
 * \par Warning:
 * Does not check for duplicates, assumes it's unique
 * \param psDir		Directory to contain new Inode
 * \param psName	Name of new Inode
 * \param nNameLen	Length of psName
 * \param nMode		Mode/flags of new Inode
 * \return 0 on success, negative error code on failure
 * \sa
 ****************************************************************************/
static TmpFSInode_s *tfs_create_node( TmpFSInode_s *psDir, const char *pzName, int nNameLen, int nMode )
{
	TmpFSInode_s *psNewNode;

	printk("%s\n", __FUNCTION__);
	if( nNameLen < 1 || nNameLen >= TFS_MAX_NAME_LEN )
	{
		return ( NULL );
	}


	psNewNode = kmalloc( sizeof( TmpFSInode_s ), MEMF_KERNEL | MEMF_CLEAR | MEMF_OKTOFAILHACK );
	if (!psNewNode)
	{
		return ( NULL );
	}

	psNewNode->ti_hLock = create_semaphore( "tmpfs_inode_lock", 1, SEM_RECURSIVE );
	if ( psNewNode->ti_hLock < 0 )
	{
		kfree( psNewNode );
		return ( NULL );
	}

	psNewNode->ti_nInodeNum = ( int )psNewNode;
	psNewNode->ti_nMode = nMode;
	psNewNode->ti_nSize = 0;
	psNewNode->ti_nCTime = psNewNode->ti_nMTime = get_real_time() / 1000000;
	psNewNode->ti_nLinkCount = 1;
	psNewNode->ti_nNameLen = nNameLen;
	memcpy( psNewNode->ti_zName, pzName, nNameLen );
	psNewNode->ti_zName[nNameLen] = '\0';
	LIST_INIT(&psNewNode->ti_sChildren);
	LIST_INIT(&psNewNode->ti_sAttributes);
	LIST_INIT(&psNewNode->ti_sWalks);
#ifdef TFS_DEBUG
	psNewNode->ti_nLockCount = 0;
#endif // TFS_DEBUG

	if ( S_ISATTR( nMode ) )
	{
		tfs_tree_insert_attr( psDir, psNewNode );
	}
	else
	{
		tfs_tree_insert( psDir, psNewNode );
	}

	return ( psNewNode );
}

/** Delete an Inode from tmpfs
 * \par Description:
 * Delete the given Inode from the given tmpfs volume.  Unlink the node from the sibling list it's
 * in.  Free it's buffer and itself.
 * \par Note:
 * This function recursively calls itself to delete all the attributes.  Since attributes cannot
 * have children or attributes, there is only one level of recursion for this function
 * \par Locks:
 * Must be called with tv_hLock (volume lock) LOCKED
 * Must be called with psNode->ti_hLock UNLOCKED.
 * \par Warning:
 * This assumes the Inode has no children
 * \param psNode	Inode to delete
 * \sa
 ****************************************************************************/
static void tfs_delete_node( TmpFSInode_s *psNode )
{
	TmpFSWalk_s *psWalk;
	TmpFSInode_s *psAttr;

	printk("%s %s\n", __FUNCTION__, psNode->ti_zName);
	if( psNode->ti_nLinkCount != 0 )
	{
		tfs_tree_remove( psNode->ti_psParent, psNode );
		psNode->ti_nLinkCount--;
	}

	printk("%s linkcount %d\n", __FUNCTION__, psNode->ti_nLinkCount);
	kassertw( psNode->ti_nLinkCount == 0 );

	if( psNode->ti_bIsLoaded == false )
	{
		/* Lock node */
		TFS_INODE_LOCK( psNode );
		psWalk = tfs_tree_walk_start( psNode, TMPFS_WALK_ATTR );
		psAttr = tfs_tree_walk_next( psWalk );
		while ( psAttr )
		{
			tfs_delete_node( psAttr );
			psAttr = tfs_tree_walk_next( psWalk );
		}
		tfs_tree_walk_end( psWalk );

		if ( psNode->ti_pBuffer )
			kfree( psNode->ti_pBuffer );
		// XXX assumes it's safe to delete a locked semaphore
		delete_semaphore( psNode->ti_hLock );
		kfree( psNode );
	}
}

/** Lookup a name on a tmpfs volume
 * \par Description:
 * Lookup the given name in the given directory.  On success, <pnResInode> contains the Inode number of
 * the correct Inode. 
 * \par Note:
 * \par Locks:
 * \par Warning:
 * \param pVolume	tmpfs Volume
 * \param pDir		Directory to search
 * \param pzName	Name to lookup
 * \param nNameLen	Length of <pzName>
 * \param pnResInode	Out param: the inode number matching the name
 * \return 0 on success, negative error code on failure
 * \sa
 ****************************************************************************/
static int tfs_lookup( void *pVolume, void *pDir, const char *pzName, int nNameLen, ino_t *pnResInode )
{
	TmpFSVolume_s *psVolume = pVolume;
	TmpFSInode_s *psDir = pDir;
	TmpFSInode_s *psNode;

	printk("%s \"%s\"/\"%s\" (%d)\n", __FUNCTION__, psDir->ti_zName, pzName, nNameLen);
	*pnResInode = 0;

	TFS_VOLUME_LOCK( psVolume );
	psNode = tfs_tree_find( psDir, pzName, nNameLen );
	TFS_VOLUME_UNLOCK( psVolume );

	if ( psNode == NULL )
	{
		return ( -ENOENT );
	}

	*pnResInode = psNode->ti_nInodeNum;
	return ( 0 );
}

/** Read from a tmpfs file
 * \par Description:
 * Read from the given file starting at the given position, for the given amount.
 * \par Note:
 * \par Locks:
 * \par Warning:
 * \param pVolume	tmpfs Volume
 * \param pNode		Inode of file to read
 * \param pCookie	Unused (cookie from open)
 * \param nPos		Position to start reading
 * \param pBuffer	Out param: Buffer to read into
 * \param nSize		Size of <pBuffer>
 * \return Amount read on success, negative error code on failure
 * \sa
 ****************************************************************************/
static int tfs_read( void *pVolume, void *pNode, void *pCookie, off_t nPos, void *pBuffer, size_t nSize )
{
	TmpFSInode_s *psNode = pNode;
	int nError;

	printk("%s \"%s\" pos %u size %u\n", __FUNCTION__, psNode->ti_zName, (unsigned)nPos, (unsigned)nSize);
	TFS_INODE_LOCK( psNode );
	if( psNode->ti_pBuffer == NULL || nPos >= psNode->ti_nSize )
	{
		nError = 0;
		goto error;
	}
	if( nPos + nSize > psNode->ti_nSize )
	{
		nSize = psNode->ti_nSize - nPos;
	}
	memcpy( pBuffer, psNode->ti_pBuffer + nPos, nSize );
	nError = nSize;
error:
	TFS_INODE_UNLOCK( psNode );
	printk("%s returning %d\n", __FUNCTION__, nError);
	return ( nError );
}

/** Write to a tmpfs file
 * \par Description:
 * Write to the given file starting at the given position from the given buffer for the given
 * amount.  If the new data won't fit into the current buffer, allocate a new one of the correct
 * size.  Write from the given buffer for the given amount.
 * \par Note:
 * \par Locks:
 * \par Warning:
 * \param pVolume	tmpfs Volume
 * \param pNode		Inode to write into
 * \param pCookie	Unused (cookie from open)
 * \param nPos		Position at which to start writing
 * \param pBuffer	Buffer to write from
 * \param nSize		Amount to write
 * \return Amount written on success, negative error code on failure
 * \sa
 ****************************************************************************/
static int tfs_write( void *pVolume, void *pNode, void *pCookie, off_t nPos, const void *pBuffer, size_t nSize )
{
	TmpFSVolume_s *psVolume = pVolume;
	TmpFSInode_s *psNode = pNode;
	TmpFSCookie_s *psCookie = pCookie;
	int nError;

	printk("%s \"%s\" pos %u size %u\n", __FUNCTION__, psNode->ti_zName, (unsigned)nPos, (unsigned)nSize);

	/* If opened for append, set position to end */
	if ( psCookie && psCookie->tc_nMode & O_APPEND )
	{
		nPos = psNode->ti_nSize;
		printk("%s append: new pos %u\n", __FUNCTION__, (unsigned)nPos);
	}

	if( nPos + nSize > TFS_MAX_FILESIZE )
	{
		return ( -EFBIG );
	}
	if( nPos < 0 )
	{
		return ( -EINVAL );
	}

	TFS_INODE_LOCK( psNode );
	if( nPos + nSize > psNode->ti_nSize || psNode->ti_pBuffer == NULL )
	{
		void *pBuffer = kmalloc( nPos + nSize, MEMF_KERNEL | MEMF_CLEAR | MEMF_OKTOFAILHACK );

		if( pBuffer == NULL )
		{
			nError = -ENOMEM;
			goto error;
		}
		if( psNode->ti_pBuffer != NULL )
		{
			memcpy( pBuffer, psNode->ti_pBuffer, psNode->ti_nSize );
			kfree( psNode->ti_pBuffer );
		}
		psVolume->tv_nFSSize += nPos + nSize - psNode->ti_nSize;
		psNode->ti_pBuffer = pBuffer;
		psNode->ti_nSize = nPos + nSize;
		printk("%s size %d\n", __FUNCTION__, psNode->ti_nSize);
	}
	memcpy( psNode->ti_pBuffer + nPos, pBuffer, nSize );
	nError = nSize;
error:
	TFS_INODE_UNLOCK( psNode );
	printk("%s returning %d\n", __FUNCTION__, nError);
	return ( nError );
}

/** Rewind an open directory
 * \par Description:
 * Rewind a directory opened by open so that the next readdir returns the first entry (".")
 * \par Note:
 * \par Warning:
 * \param pVolume	tmpfs Volume
 * \param pDir		Directory being rewound open
 * \param pCookie	cookie returned from open
 * \return 0 on success, negative error code on failure
 * \sa
 ****************************************************************************/
int tfs_rewinddir( void *pVolume, void *pDir, void *pCookie )
{
	TmpFSVolume_s *psVolume = pVolume;
	TmpFSInode_s *psDir = pDir;
	TmpFSCookie_s *psCookie = pCookie;

	if ( !psCookie->tc_psWalk || S_ISDIR( psDir->ti_nMode ) == false )
	{
		return ( -ENOTDIR );
	}

	TFS_VOLUME_LOCK( psVolume );
	tfs_tree_walk_reset( psCookie->tc_psWalk );
	TFS_VOLUME_UNLOCK( psVolume );

	psCookie->tc_nState = TMPFS_COOKIE_DOT;

	return ( 0 );
}

/** Read an entry from a directory
 * \par Description:
 * Read the next entry from a directory.  Special case "." and "..".  Then, return the next walk
 * entry.
 * \par Note:
 * \par Locks:
 * \par Warning:
 * \param pVolume	tmpfs Volume
 * \param pDir		Directory to read
 * \param pCookie	Cookie from open
 * \param nPos		Position of next entry
 * \param psFileInfo	Out param: Directory entry data to fill
 * \param nBufSize	Unused (size of <psFileInfo>, always sizeof(kernel_dirent))
 * \return 0 if directory is empty, positive number of entries on success
 * \sa
 ****************************************************************************/
int tfs_readdir( void *pVolume, void *pDir, void *pCookie, int nPos, struct kernel_dirent *psFileInfo, size_t nBufSize )
{
	TmpFSVolume_s *psVolume = pVolume;
	TmpFSInode_s *psDir = pDir;
	TmpFSCookie_s *psCookie = pCookie;
	TmpFSInode_s *psNode;
	int nNumFound = 0;

	printk("%s \"%s\" pos %d bufsize %d\n", __FUNCTION__, psDir->ti_zName, nPos, nBufSize);

	if ( !psCookie )
	{
		return ( -ENOMEM );
	}

	if ( !psCookie->tc_psWalk || S_ISDIR( psDir->ti_nMode ) == false )
	{
		return ( -ENOTDIR );
	}

	TFS_VOLUME_LOCK( psVolume );
	while ( nNumFound * sizeof ( struct kernel_dirent ) < nBufSize )
	{
		switch ( psCookie->tc_nState )
		{
			case TMPFS_COOKIE_DOT:
				strcpy( psFileInfo->d_name, "." );
				psFileInfo->d_namlen = 1;
				psFileInfo->d_ino = psDir->ti_nInodeNum;
				psCookie->tc_nState = TMPFS_COOKIE_DOT_DOT;
				break;
			case TMPFS_COOKIE_DOT_DOT:
				strcpy( psFileInfo->d_name, ".." );
				psFileInfo->d_namlen = 2;
				if ( psDir->ti_psParent != NULL )
				{
					psFileInfo->d_ino = psDir->ti_psParent->ti_nInodeNum;
				}
				else
				{
					psFileInfo->d_ino = psDir->ti_nInodeNum;
				}
				psCookie->tc_nState = TMPFS_COOKIE_WALK;
				break;
			case TMPFS_COOKIE_WALK:
				psNode = tfs_tree_walk_next( psCookie->tc_psWalk );
				if ( psNode == NULL )
				{
					goto error;
				}
				strcpy( psFileInfo->d_name, psNode->ti_zName );
				psFileInfo->d_namlen = psNode->ti_nNameLen;
				psFileInfo->d_ino = psNode->ti_nInodeNum;
				break;
		}
		nNumFound++;
	}

error:
	TFS_VOLUME_UNLOCK( psVolume );
	return ( nNumFound );
}

/** Read Stat on a tmpfs file
 * \par Description:
 * Fill the given stat structure with information about the given file.
 * \par Note:
 * Atime is not currently saved, and current time is returned
 * \par Locks:
 * \par Warning:
 * \param pVolume	tmpfs Volume
 * \param pNode		Inode to stat
 * \param psStat	Out param: Stat structure to fill
 * \return 0 on success, negative error code on failure
 * \sa
 ****************************************************************************/
static int tfs_rstat( void *pVolume, void *pNode, struct stat *psStat )
{
	TmpFSVolume_s *psVolume = pVolume;
	TmpFSInode_s *psNode = pNode;

	printk("%s \"%s\"\n", __FUNCTION__, psNode->ti_zName);
	psStat->st_dev = psVolume->tv_nDevNum;
	psStat->st_ino = psNode->ti_nInodeNum;
	psStat->st_mode = psNode->ti_nMode;
	psStat->st_nlink = psNode->ti_nLinkCount;
	psStat->st_uid = psNode->ti_nUID;
	psStat->st_gid = psNode->ti_nGID;
	psStat->st_size = psNode->ti_nSize;
	psStat->st_atime = get_real_time() / 1000000;
	psStat->st_mtime = psNode->ti_nMTime;
	psStat->st_ctime = psNode->ti_nCTime;

	return ( 0 );
}

/** Write Stat on a tmpfs file
 * \par Description:
 * Save info from the given stat structure into the given file. 
 * \par Note:
 * Atime is not currently saved
 * \par Locks:
 * \par Warning:
 * \param pVolume	tmpfs Volume
 * \param pNode		Inode to stat
 * \param psStat	Stat structure read
 * \param nMask		Mask of stat fields to set
 * \return 0 on success, negative error code on failure
 * \sa
 ****************************************************************************/
static int tfs_wstat( void *pVolume, void *pNode, const struct stat *psStat, uint32 nMask )
{
	TmpFSInode_s *psNode = pNode;

	printk("%s \"%s\"\n", __FUNCTION__, psNode->ti_zName);
	if( nMask & WSTAT_MODE )
	{
		psNode->ti_nMode = ( psNode->ti_nMode & S_IFMT ) | ( psStat->st_mode & ~S_IFMT );
	}
	if( nMask & WSTAT_UID )
	{
		psNode->ti_nUID = psStat->st_uid;
	}
	if( nMask & WSTAT_GID )
	{
		psNode->ti_nGID = psStat->st_gid;
	}
	if( nMask & WSTAT_MTIME )
	{
		psNode->ti_nMTime = psStat->st_mtime;
	}
	if( nMask & WSTAT_CTIME )
	{
		psNode->ti_nCTime = psStat->st_ctime;
	}

	return ( 0 );
}

/** Open a tmpfs file
 * \par Description:
 * Open the given file.  This allocates a cookie that stores open state, currenly only the open
 * mode.
 * \par Note:
 * \par Locks:
 * \par Warning:
 * \param pVolume	tmpfs Volume
 * \param pNode		Inode to open
 * \param nMode		Open mode
 * \param ppCookie	Out param: cookie from open
 * \return 0 on success, negative error code on failure
 * \sa
 ****************************************************************************/
static int tfs_open( void *pVolume, void *pNode, int nMode, void **ppCookie )
{
	TmpFSVolume_s *psVolume = pVolume;
	TmpFSInode_s *psNode = pNode;
	TmpFSCookie_s *psCookie;

	printk("%s \"%s\"\n", __FUNCTION__, psNode->ti_zName);
	psCookie = kmalloc( sizeof( TmpFSCookie_s ), MEMF_KERNEL | MEMF_CLEAR | MEMF_OKTOFAILHACK );
	if ( psCookie == NULL )
	{
		return ( -ENOMEM );
	}

	psCookie->tc_nMode = nMode;
	*ppCookie = psCookie;

	if( S_ISDIR( psNode->ti_nMode ) == true )
	{ // This is a directory.  Set up a walk
		TFS_VOLUME_LOCK( psVolume );
		psCookie->tc_psWalk = tfs_tree_walk_start( psNode, TMPFS_WALK_DIR );
		TFS_VOLUME_UNLOCK( psVolume );
		if ( psCookie->tc_psWalk == NULL )
		{
			kfree( psCookie);
			return ( -ENOMEM );
		}
		psCookie->tc_nState = TMPFS_COOKIE_DOT;
	}

	return ( 0 );
}

/** Close an open tmpfs file
 * \par Description:
 * Close the given file.  Free the cookie from open
 * \par Note:
 * \par Locks:
 * \par Warning:
 * \param pVolume	tmpfs Volume
 * \param pNode		Inode to close
 * \param pCookie	Cookie from open
 * \return 0 on success, negative error code on failure
 * \sa tfs_open
 ****************************************************************************/
static int tfs_close( void *pVolume, void *pNode, void *pCookie )
{
	TmpFSVolume_s *psVolume = pVolume;
	TmpFSInode_s *psNode = pNode;
	TmpFSCookie_s *psCookie = pCookie;

	printk("%s \"%s\"\n", __FUNCTION__, psNode->ti_zName);

	if( S_ISDIR( psNode->ti_nMode ) == true )
	{ // This is a directory.  Clean up a walk
		TFS_VOLUME_LOCK( psVolume );
		tfs_tree_walk_end( psCookie->tc_psWalk );
		TFS_VOLUME_UNLOCK( psVolume );
	}

	kfree(psCookie);
	return ( 0 );
}

/** Read a tmpfs Inode
 * \par Description:
 * "Read" the Inode with the given number from "disk".  Inode numbers are the pointer to the Inode
 * structure, so just de-reference it.  Do some sanity checks, and mark the Inode as in use by the
 * VFS.  Return it in <ppNode>.
 * \par Note:
 * \par Locks:
 * \par Warning:
 * \param pVolume	tmpfs Volume
 * \param nInodeNum	Inode number (address of inode structure or 1 for Root)
 * \param ppNode	Out param: Inode pointer
 * \return 0 on success, negative error code on failure
 * \sa
 ****************************************************************************/
static int tfs_read_inode( void *pVolume, ino_t nInodeNum, void **ppNode )
{
	TmpFSVolume_s *psVolume = pVolume;
	TmpFSInode_s *psNode;

	printk("%s\n", __FUNCTION__);
	switch ( nInodeNum )
	{
		case TFS_ROOT:
			psNode = psVolume->tv_psRootNode;
			break;
		case TFS_INDEXDIR:
			psNode = psVolume->tv_psIndexDir;
			break;
		default:
			psNode = ( TmpFSInode_s * )( ( long )nInodeNum );
			if( psNode->ti_nInodeNum != ( long )psNode )
			{
				printk( "tfs_read_inode() invalid inode %Lx\n", nInodeNum );
				psNode = NULL;
			}
			break;
	}

	if( psNode != NULL )
	{
		printk("%s \"%s\"\n", __FUNCTION__, psNode->ti_zName);
		kassertw( psNode->ti_bIsLoaded == false );
		psNode->ti_bIsLoaded = true;
		*ppNode = psNode;
		return ( 0 );
	}
	else
	{
		*ppNode = NULL;
		return ( -EINVAL );
	}
}

/** Write a tmpfs Inode
 * \par Description:
 * "Write" out the given inode.  Mark it as unused, and delete it if it's link count is 0.
 * \par Note:
 * \par Locks:
 * \par Warning:
 * \param pVolume	tmpfs Volume
 * \param pNode		pointer to Inode to "write"
 * \return 0 on success, negative error code on failure
 * \sa
 ****************************************************************************/
static int tfs_write_inode( void *pVolume, void *pNode )
{
	TmpFSVolume_s *psVolume = pVolume;
	TmpFSInode_s *psNode = pNode;

	printk("%s \"%s\"\n", __FUNCTION__, psNode->ti_zName);
	psNode->ti_bIsLoaded = false;

	if( psNode->ti_nLinkCount == 0 )
	{
		TFS_VOLUME_LOCK( psVolume );
		tfs_delete_node( psNode );
		TFS_VOLUME_UNLOCK( psVolume );
	}
	return ( 0 );
}

/** Create a tmpfs directory
 * \par Description:
 * Create a directory with the given name in the given directory.
 * \par Note:
 * \par Locks:
 * \par Warning:
 * \param pVolume	tmpfs Volume
 * \param pDir		Directory to contain new directory
 * \param pzName	Name of new directory
 * \param nNameLen	Length of <pzName>
 * \param nPerms	Creation permissions
 * \return 0 on success, negative error code on failure
 * \sa
 ****************************************************************************/
static int tfs_mkdir( void *pVolume, void *pDir, const char *pzName, int nNameLen, int nPerms )
{
	TmpFSVolume_s *psVolume = pVolume;
	TmpFSInode_s *psDir = pDir;
	TmpFSInode_s *psNode;
	int nError = 0;

	printk("%s \"%s\"\n", __FUNCTION__, pzName);
	TFS_VOLUME_LOCK( psVolume );
	psNode = tfs_tree_find( psDir, pzName, nNameLen );
	if ( psNode != NULL )
	{
		nError = -EEXIST;
		goto error;
	}

	psNode = tfs_create_node( psDir, pzName, nNameLen, S_IFDIR | ( nPerms & S_IRWXUGO ) );
	if (!psNode)
	{
		nError = -ENOMEM;
		goto error;
	}
error:
	TFS_VOLUME_UNLOCK( psVolume );
	return ( nError );
}

/** Remove a tmpfs directory
 * \par Description:
 * Delete the directory with the given name from the given directory.  Search the given directory
 * for a directory with the given name.  If it's not empty, unlink it.  If it's not open, free it.
 * \par Note:
 * \par Locks:
 * \par Warning:
 * \param pVolume	tmpfs Volume
 * \param pDir		Directory containing the directory to remove
 * \param pzName	Name of directory to remove
 * \param nNameLen	Length of <pzName>
 * \return 0 on success, negative error code on failure
 * \sa
 ****************************************************************************/
static int tfs_rmdir( void *pVolume, void *pDir, const char *pzName, int nNameLen )
{
	TmpFSVolume_s *psVolume = pVolume;
	TmpFSInode_s *psDir = pDir;
	TmpFSInode_s *psNode = NULL;
	int nError = 0;

	printk("%s \"%s\"\n", __FUNCTION__, pzName);
	TFS_VOLUME_LOCK( psVolume );
	psNode = tfs_tree_find( psDir, pzName, nNameLen );
	if ( !psNode )
	{
		nError = -ENOENT;
		goto error;
	}
	TFS_INODE_LOCK( psNode );
	if( S_ISDIR( psNode->ti_nMode ) == false )
	{
		nError = -ENOTDIR;
		goto error;
	}
	if( !tfs_tree_is_empty( psNode ) )
	{
		nError = -ENOTEMPTY;
		goto error;
	}
	tfs_delete_node( psNode );
	psNode = NULL;
error:
	if (psNode)
	{
		TFS_INODE_UNLOCK( psNode );
	}
	TFS_VOLUME_UNLOCK( psVolume );
	return ( nError );
}

/** Delete a tmpfs file
 * \par Description:
 * Delete the file with the given name from the given directory.  The file must not be a directory.
 * Find it in the directory, and unlink it.  If it's not currently open, free it.
 * \par Note:
 * \par Locks:
 * \par Warning:
 * \param pVolume	tmpfs Volume
 * \param pDir		Dirctory containg file
 * \param pzName	Name of file to unlink
 * \param nNameLen	Length of <pzName>
 * \return 0 on success, negative error code on failure
 * \sa
 ****************************************************************************/
static int tfs_unlink( void *pVolume, void *pDir, const char *pzName, int nNameLen )
{
	TmpFSVolume_s *psVolume = pVolume;
	TmpFSInode_s *psDir = pDir;
	TmpFSInode_s *psNode = NULL;
	int nError = 0;

	printk("%s \"%s\"\n", __FUNCTION__, pzName);
	TFS_VOLUME_LOCK( psVolume );
	psNode = tfs_tree_find( psDir, pzName, nNameLen );
	if ( !psNode )
	{
		nError = -ENOENT;
		goto error;
	}
	TFS_INODE_LOCK( psNode );
	if( S_ISDIR( psNode->ti_nMode ) == true )
	{
		nError = -EISDIR;
		goto error;
	}
	tfs_delete_node( psNode );
	psNode = NULL;
error:
	if (psNode)
	{
		TFS_INODE_UNLOCK( psNode );
	}
	TFS_VOLUME_UNLOCK( psVolume );
	return ( nError );
}

/** Create a tmpfs file
 * \par Description:
 * Create and open a file with the given name in the given directory.
 * \par Note:
 * \par Locks:
 * \par Warning:
 * \param pVolume	tmpfs Volume
 * \param pDir		Directory to contain the new file
 * \param pzName	New file name
 * \param nNameLen	Length of <pzName>
 * \param nMode		(unused) Open mode flags
 * \param nPerm		File creation permissions
 * \param pnInodeNum	Out param: inode number of new file
 * \param ppCookie	Out param: (unused) Open cookie for newly created/opened file
 * \return 0 on success, negative error code on failure
 * \sa
 ****************************************************************************/
static int tfs_create( void *pVolume, void *pDir, const char *pzName, int nNameLen, int nMode, int nPerms, ino_t *pnInodeNum, void **ppCookie )
{
	TmpFSVolume_s *psVolume = pVolume;
	TmpFSInode_s *psDir = pDir;
	TmpFSInode_s *psNode;
	int nError = 0;

	printk("%s \"%s\"\n", __FUNCTION__, pzName);
	*ppCookie = 0;

	TFS_VOLUME_LOCK( psVolume );
	psNode = tfs_tree_find( psDir, pzName, nNameLen );
	if (psNode != NULL)
	{
		nError = -EEXIST;
		goto error;
	}

	psNode = tfs_create_node( psDir, pzName, nNameLen, S_IFREG | ( nPerms & S_IRWXUGO ) );
	if( psNode != NULL )
	{
		*pnInodeNum = psNode->ti_nInodeNum;
	}
	else
	{
		nError = -ENOMEM;
	}

error:
	TFS_VOLUME_UNLOCK( psVolume );
	return ( nError );
}

/** Create a symbolic link on tmpfs
 * \par Description:
 * Create a symlink in the given directory with the given name to the given path.
 * \par Note:
 * \par Locks:
 * \par Warning:
 * \param pVolume	tmpfs Volume
 * \param pDir		Directory to contain the new symlink
 * \param pzName	Name of new symlink
 * \param nNameLen	Length of <pzName>
 * \param pzNewPath	Path to put into new symlink
 * \return 0 on success, negative error code on failure
 * \sa
 ****************************************************************************/
static int tfs_symlink( void *pVolume, void *pDir, const char *pzName, int nNameLen, const char *pzNewPath )
{
	TmpFSVolume_s *psVolume = pVolume;
	TmpFSInode_s *psDir = pDir;
	TmpFSInode_s *psNode;
	int nError = 0;

	printk("%s \"%s\"\n", __FUNCTION__, pzName);
	TFS_VOLUME_LOCK( psVolume );
	psNode = tfs_tree_find( psDir, pzName, nNameLen );
	if (psNode != NULL)
	{
		nError = -EEXIST;
		goto error;
	}

	psNode = tfs_create_node( psDir, pzName, nNameLen, S_IFLNK | S_IRUGO | S_IWUGO );

	if( psNode == NULL )
	{
		nError = -ENOMEM;
		goto error;
	}

	TFS_INODE_LOCK( psNode );
	psNode->ti_nSize = strlen( pzNewPath ) + 1;
	psNode->ti_pBuffer = kmalloc( psNode->ti_nSize, MEMF_KERNEL | MEMF_OKTOFAILHACK );

	if( psNode->ti_pBuffer == NULL )
	{
		/* Will delete Inode lock */
		tfs_delete_node( psNode );
		nError = -ENOMEM;
		goto error;
	}

	strcpy( psNode->ti_pBuffer, pzNewPath );
	psVolume->tv_nFSSize += psNode->ti_nSize;
	TFS_INODE_UNLOCK( psNode );

error:
	TFS_VOLUME_UNLOCK( psVolume );
	return ( nError );
}

/** Read a tmpfs symlink
 * \par Description:
 * Read the contents of the given symlink, and return it in the given buffer.
 * \par Note:
 * \par Locks:
 * \par Warning:
 * \param pVolume	tmpfs Volume
 * \param pNode		symlink to read
 * \param pzBuf		Buffer to read into
 * \param nBufSize	size of <pzBuf>
 * \return amount read on success, negative error code on failure
 * \sa
 ****************************************************************************/
static int tfs_readlink( void *pVolume, void *pNode, char *pzBuf, size_t nBufSize )
{
	TmpFSInode_s *psNode = pNode;
	int nNumRead;

	printk("%s \"%s\"\n", __FUNCTION__, psNode->ti_zName);
	if( S_ISLNK( psNode->ti_nMode ) == false )
	{
		return ( -EINVAL );
	}
	TFS_INODE_LOCK( psNode );
	if( psNode->ti_pBuffer == NULL )
	{
		nNumRead = 0;
		goto error;
	}
	nNumRead = min( nBufSize, psNode->ti_nSize );
	memcpy( pzBuf, psNode->ti_pBuffer, nNumRead );
error:
	TFS_INODE_UNLOCK( psNode );
	return ( nNumRead );
}

/** Rename a tmpfs file/directory
 * \par Description:
 * Rename the file in the given old directory with the given old name to the new name in the new
 * directory.  The old file must exist, the new file must not be a directory.
 * \par Note:
 * \par Locks:
 * \par Warning:
 * \param pVolume	tmpfs Volume
 * \param pOldDir	Directory containing file to be renamed
 * \param pzOldName	Name of file to be renamed
 * \param nOldNameLen	Length of <pzOldName>
 * \param pNewDir	Directory to contain file under new name
 * \param pzNewName	New Name of file
 * \param nNewNameLen	Length of <pzNewName>
 * \param bMustBeDir	If true, the file being renamed must be a directory
 * \return 0 on success, negative error code on failure
 * \sa
 ****************************************************************************/
static int tfs_rename( void *pVolume, void *pOldDir, const char *pzOldName, int nOldNameLen, void *pNewDir, const char *pzNewName, int nNewNameLen, bool bMustBeDir )
{
	TmpFSVolume_s *psVolume = pVolume;
	TmpFSInode_s *psOldDir = pOldDir;
	TmpFSInode_s *psNewDir = pNewDir;
	TmpFSInode_s *psNode;
	TmpFSInode_s *psDstNode;

	printk("%s \"%s\"/\"%s\" -> \"%s\"/\"%s\"\n", __FUNCTION__, psOldDir->ti_zName, pzOldName, psNewDir->ti_zName, pzNewName);
	printk("%s\n", __FUNCTION__);
	if ( nNewNameLen > TFS_MAX_NAME_LEN )
	{
		return ( -ENOMEM );
	}

	TFS_VOLUME_LOCK( psVolume );
	psNode = tfs_tree_find( psOldDir, pzOldName, nOldNameLen );
	TFS_VOLUME_UNLOCK( psVolume );
	if( psNode == NULL )
	{
		return ( -ENOENT );
	}
	if( bMustBeDir && S_ISDIR( psNode->ti_nMode ) == false )
	{
		return ( -ENOTDIR );
	}
	TFS_VOLUME_LOCK( psVolume );
	psDstNode = tfs_tree_find( psNewDir, pzNewName, nNewNameLen );
	TFS_VOLUME_UNLOCK( psVolume );
	if( psDstNode != NULL )
	{
		if( S_ISDIR( psDstNode->ti_nMode ) )
		{
			return ( -EISDIR );
		}
		// Volume and node must be unlocked
		tfs_unlink( psVolume, pNewDir, pzNewName, nNewNameLen );
	}

	TFS_VOLUME_LOCK( psVolume );
	memcpy( psNode->ti_zName, pzNewName, nNewNameLen );
	psNode->ti_zName[nNewNameLen] = '\0';
	psNode->ti_nNameLen = nNewNameLen;
	if( psOldDir != psNewDir )
	{
		tfs_tree_remove( psOldDir, psNode );
		tfs_tree_insert( psNewDir, psNode );
	}
	TFS_VOLUME_UNLOCK( psVolume );
	return ( 0 );
}

/** Mount a tmpfs filesystem
 * \par Description:
 * This will create and mount a tmpfs filesystem.
 * \par Locks:
 * No locking necessary, because nothing can have pointers to this volume yet
 * \par Warning:
 * \param nDevNum	Device number
 * \param pzDevPath	Unused (Path to device to mount)
 * \param nFlags	Mount flags
 * \param pArgs		Unused (Mount arguments)
 * \param nArgLen	Unused (Length of arguemnts)
 * \param ppVolData	Volume data cookie to fill
 * \param pnRootIno	Root directory inode to fill
 * \return 0 on success, negative error code on failure
 * \sa
 ****************************************************************************/
static int tfs_mount( kdev_t nDevNum, const char *pzDevPath, uint32 nFlags, void *pArgs, int nArgLen, void **ppVolData, ino_t *pnRootIno )
{
	TmpFSVolume_s *psVolume = kmalloc( sizeof( TmpFSVolume_s ), MEMF_KERNEL | MEMF_CLEAR | MEMF_OKTOFAILHACK );
	TmpFSInode_s *psRootNode, *psIndexDir;

	printk("%s\n", __FUNCTION__);
	if( psVolume == NULL )
	{
		return ( -ENOMEM );
	}
	psVolume->tv_hLock = create_semaphore( "tmpfs_volume_lock", 1, SEM_RECURSIVE );
	if( psVolume->tv_hLock < 0 )
	{
		kfree( psVolume );
		return ( -ENOMEM );
	}
	psRootNode = kmalloc( sizeof( TmpFSInode_s ), MEMF_KERNEL | MEMF_CLEAR | MEMF_OKTOFAILHACK );
	if( psRootNode == NULL )
	{
		delete_semaphore( psVolume->tv_hLock );
		kfree( psVolume );
		return ( -ENOMEM );
	}
	psRootNode->ti_hLock = create_semaphore( "tmpfs_inode_lock", 1, SEM_RECURSIVE );
	if( psRootNode->ti_hLock < 0 )
	{
		delete_semaphore( psVolume->tv_hLock );
		kfree( psVolume );
		kfree( psRootNode );
		return ( -ENOMEM );
	}
	psIndexDir = kmalloc( sizeof( TmpFSInode_s ), MEMF_KERNEL | MEMF_CLEAR | MEMF_OKTOFAILHACK );
	if( psIndexDir == NULL )
	{
		delete_semaphore( psVolume->tv_hLock );
		kfree( psVolume );
		return ( -ENOMEM );
	}
	psIndexDir->ti_hLock = create_semaphore( "tmpfs_inode_lock", 1, SEM_RECURSIVE );
	if( psIndexDir->ti_hLock < 0 )
	{
		delete_semaphore( psRootNode->ti_hLock );
		delete_semaphore( psVolume->tv_hLock );
		kfree( psVolume );
		kfree( psRootNode );
		kfree( psIndexDir );
		return ( -ENOMEM );
	}

	psVolume->tv_psRootNode = psRootNode;
	psVolume->tv_psIndexDir = psIndexDir;
	psVolume->tv_nDevNum = nDevNum;
#ifdef TFS_DEBUG
	psVolume->tv_nLockCount = 0;
#endif // TFS_DEBUG

	psRootNode->ti_nCTime = psRootNode->ti_nMTime = get_real_time() / 1000000;
	psRootNode->ti_zName[0] = '/';
	psRootNode->ti_zName[1] = '\0';
	psRootNode->ti_nNameLen = 1;
	psRootNode->ti_nLinkCount = 1;
	psRootNode->ti_nMode = S_IRWXUGO | S_IFDIR;
	psRootNode->ti_nInodeNum = TFS_ROOT;
	LIST_INIT( &psRootNode->ti_sChildren );
	LIST_INIT( &psRootNode->ti_sAttributes );
	LIST_INIT( &psRootNode->ti_sWalks );
	psRootNode->ti_psParent = NULL;
#ifdef TFS_DEBUG
	psRootNode->ti_nLockCount = 0;
#endif // TFS_DEBUG

	psIndexDir->ti_nCTime = psIndexDir->ti_nMTime = get_real_time() / 1000000;
	psIndexDir->ti_zName[0] = ':';
	psIndexDir->ti_zName[1] = '\0';
	psIndexDir->ti_nNameLen = 1;
	psIndexDir->ti_nLinkCount = 1;
	psIndexDir->ti_nMode = S_IRWXUGO | S_IFDIR;
	psIndexDir->ti_nInodeNum = TFS_INDEXDIR;
	LIST_INIT( &psIndexDir->ti_sChildren );
	LIST_INIT( &psIndexDir->ti_sAttributes );
	LIST_INIT( &psIndexDir->ti_sWalks );
	psIndexDir->ti_psParent = NULL;
#ifdef TFS_DEBUG
	psIndexDir->ti_nLockCount = 0;
#endif // TFS_DEBUG

	*pnRootIno = TFS_ROOT;
	*ppVolData = psVolume;

	return ( 0 );
}

/** Get stats from a tmpfs filesystem
 * \par Description:
 * Get stat info from the given filesystem.  Currently, these are mostly just hardcoded numbers.
 * \par Note:
 * Shouldn't total_blocks be the total possible blocks, not the total used?
 * \par Locks:
 * Currently no locking
 * \par Warning:
 * \param pVolume	tmpfs Volume
 * \param psInfo	Out param: FS Stat info to fill
 * \return 0 on success, negative error code on failure
 * \sa
 ****************************************************************************/
int tfs_rfsstat( void *pVolume, fs_info * psInfo )
{
	TmpFSVolume_s *psVolume = pVolume;

	printk("%s\n", __FUNCTION__);
	psInfo->fi_dev = psVolume->tv_nDevNum;
	psInfo->fi_root = TFS_ROOT;
	psInfo->fi_flags = 0;
	psInfo->fi_block_size = 512;
	psInfo->fi_io_size = 65536;
	psInfo->fi_total_blocks = ( psVolume->tv_nFSSize + 511 ) / 512;
	psInfo->fi_free_blocks = 0;
	psInfo->fi_free_user_blocks = 0;
	psInfo->fi_total_inodes = -1;
	psInfo->fi_free_inodes = -1;
	strcpy( psInfo->fi_volume_name, "tmpfs" );
	return ( 0 );
}

/** Open the attribute directory of a tmpfs Inode
 * \par Description:
 * Open the attribute directory of the given Inode.  Returns a cookie used for walking the attribute
 * directory.
 * \par Note:
 * \par Locks:
 * \par Warning:
 * \param pVolume	tmpfs Volume
 * \param pNode		Inode whose attribute directory to open
 * \param ppCookie	Out param: cookie to be passed to read/rewind
 * \return 0 on success, negative error code on failure
 * \sa
 ****************************************************************************/
int tfs_open_attrdir( void *pVolume, void *pNode, void **ppCookie )
{
	TmpFSVolume_s *psVolume = pVolume;
	TmpFSInode_s *psNode = pNode;
	TmpFSCookie_s *psCookie;

	printk("%s \"%s\"\n", __FUNCTION__, psNode->ti_zName);
	psCookie = kmalloc( sizeof( TmpFSCookie_s ), MEMF_KERNEL | MEMF_CLEAR | MEMF_OKTOFAILHACK );
	if ( psCookie == NULL )
	{
		return ( -ENOMEM );
	}

	*ppCookie = psCookie;

	TFS_VOLUME_LOCK( psVolume );
	psCookie->tc_psWalk = tfs_tree_walk_start( psNode, TMPFS_WALK_ATTR );
	TFS_VOLUME_UNLOCK( psVolume );
	if ( psCookie->tc_psWalk == NULL )
	{
		kfree( psCookie);
		return ( -ENOMEM );
	}

	return ( 0 );
}

/** Close the attribute directory of a tmpfs Inode
 * \par Description:
 * Close the attribute directory of the given Inode that was opened by tfs_open_attrdir.  Free the
 * cookie.
 * \par Note:
 * \par Locks:
 * \par Warning:
 * \param pVolume	tmpfs Volume
 * \param pNode		Inode whose attribute directory to open
 * \param pCookie	cookie returned by open_attrdir
 * \return 0 on success, negative error code on failure
 * \sa
 ****************************************************************************/
static int tfs_close_attrdir( void *pVolume, void *pNode, void *pCookie )
{
	TmpFSVolume_s *psVolume = pVolume;
	TmpFSInode_s *psNode = pNode;
	TmpFSCookie_s *psCookie = pCookie;

	printk("%s \"%s\"\n", __FUNCTION__, psNode->ti_zName);

	TFS_VOLUME_LOCK( psVolume );
	tfs_tree_walk_end( psCookie->tc_psWalk );
	TFS_VOLUME_UNLOCK( psVolume );

	kfree(psCookie);
	return ( 0 );
}

/** Rewind an open attribute directory
 * \par Description:
 * Rewind the attribute directory opened by open_attrdir so that the next read_attrdir returns the
 * first entry
 * \par Note:
 * \par Warning:
 * \param pVolume	tmpfs Volume
 * \param pNode		Node being rewound
 * \param pCookie	cookie returned from open_attrdir
 * \return 0 on success, negative error code on failure
 * \sa
 ****************************************************************************/
int tfs_rewind_attrdir( void *pVolume, void *pNode, void *pCookie )
{
	TmpFSVolume_s *psVolume = pVolume;
	TmpFSInode_s *psNode = pNode;
	TmpFSCookie_s *psCookie = pCookie;

	printk("%s \"%s\"\n", __FUNCTION__, psNode->ti_zName);
	if ( !psCookie->tc_psWalk )
	{
		return ( -EINVAL );
	}

	TFS_VOLUME_LOCK( psVolume );
	tfs_tree_walk_reset( psCookie->tc_psWalk );
	TFS_VOLUME_UNLOCK( psVolume );

	return ( 0 );
}

/** Read an entry from an attribute directory
 * \par Description:
 * Read the next entry from an attribute directory.  Since we keep a walk, just return the next walk
 * entry.
 * \par Note:
 * \par Locks:
 * \par Warning:
 * \param pVolume	tmpfs Volume
 * \param pNode		Node owning attribute directory to read
 * \param pCookie	Cookie from open
 * \param psFileInfo	Out param: Directory entry data to fill
 * \param nBufSize	Unused (size of <psFileInfo>, always sizeof(kernel_dirent))
 * \return 0 if directory is empty, positive number of entries on success
 * \sa
 ****************************************************************************/
int tfs_read_attrdir( void *pVolume, void *pNode, void *pCookie, struct kernel_dirent *psFileInfo, size_t nBufSize )
{
	TmpFSVolume_s *psVolume = pVolume;
	TmpFSInode_s *psNode = pNode;
	TmpFSCookie_s *psCookie = pCookie;
	TmpFSInode_s *psAttr;
	int nNumFound = 0;

	printk("%s \"%s\" bufsize %d\n", __FUNCTION__, psNode->ti_zName, nBufSize);

	if ( !psCookie )
	{
		return ( -ENOMEM );
	}

	if ( !psCookie->tc_psWalk )
	{
		return ( -EINVAL );
	}

	TFS_VOLUME_LOCK( psVolume );
	psAttr = tfs_tree_walk_next( psCookie->tc_psWalk );
	if ( psAttr == NULL )
	{
		goto error;
	}
	printk("%s adding %s\n", __FUNCTION__, psAttr->ti_zName);
	strcpy( psFileInfo->d_name, psAttr->ti_zName );
	psFileInfo->d_namlen = psAttr->ti_nNameLen;
	psFileInfo->d_ino = psAttr->ti_nInodeNum;
	nNumFound++;

error:
	TFS_VOLUME_UNLOCK( psVolume );
	return ( nNumFound );
}

/** Remove an attribute from a tmpfs Inode
 * \par Description:
 * Remove the attribute with the given name from the given Inode
 * \par Note:
 * \par Locks:
 * \par Warning:
 * \param pVolume	tmpfs Volume
 * \param pNode		Inode owning attribute to remove
 * \param pzName	Name of attribute to remove
 * \param nNameLen	Length of <pzName>
 * \return 0 on success, negative error code on failure
 * \sa
 ****************************************************************************/
int tfs_remove_attr( void *pVolume, void *pNode, const char *pzName, int nNameLen )
{
	TmpFSVolume_s *psVolume = pVolume;
	TmpFSInode_s *psNode = pNode;
	TmpFSInode_s *psAttr;
	int nError = 0;

	printk("%s \"%s\"::\"%s\" (%d)\n", __FUNCTION__, psNode->ti_zName, pzName, nNameLen);
	TFS_VOLUME_LOCK( psVolume );
	psAttr = tfs_tree_find_attr( psNode, pzName, nNameLen );
	if ( psAttr == NULL )
	{
		nError = -ENOENT;
		goto error;
	}

	tfs_tree_remove( psNode, psAttr );

error:
	TFS_VOLUME_UNLOCK( psVolume );
	return ( nError );
}

/** Stat an attribute of a tmpfs Inode
 * \par Description:
 * Stat the attribute of the given Inode with the given name.
 * \par Note:
 * \par Locks:
 * \par Warning:
 * \param pVolume	tmpfs Volume
 * \param pNode		Inode owning attribute
 * \param pzName	Name of attribute to stat
 * \param nNameLen	Length of <pzName>
 * \param psBuffer	Out param: attr_info buffer to fill
 * \return 0 on success, negative error code on failure
 * \sa
 ****************************************************************************/
int tfs_stat_attr( void *pVolume, void *pNode, const char *pzName, int nNameLen, struct attr_info *psBuffer )
{
	TmpFSVolume_s *psVolume = pVolume;
	TmpFSInode_s *psNode = pNode;
	TmpFSInode_s *psAttr;
	int nError = 0;

	printk("%s \"%s\"::\"%s\" (%d)\n", __FUNCTION__, psNode->ti_zName, pzName, nNameLen);
	TFS_VOLUME_LOCK( psVolume );
	psAttr = tfs_tree_find_attr( psNode, pzName, nNameLen );
	if ( psAttr == NULL )
	{
		nError = -ENOENT;
		goto error;
	}

	psBuffer->ai_type = psAttr->ti_eAttrType;
	psBuffer->ai_size = psAttr->ti_nSize;

error:
	TFS_VOLUME_UNLOCK( psVolume );
	return ( nError );
}

/** Write into an attribute of a tmpfs Inode
 * \par Description:
 * Write the given data into the attribute of the given Inode with the given name.  Look up the
 * attribute, and create it if it does not exist.  Write the data.
 * \par Note:
 * \par Locks:
 * \par Warning:
 * \param pVolume	tmpfs Volume
 * \param pNode		Inode owning attribute
 * \param pzName	Name of attribute to write
 * \param nNameLen	Length of <pzName>
 * \param nFlags	Write flags (O_TRUNC, etc.)
 * \param nType		Type of data to write
 * \param pBuffer	Data to write
 * \param nPos		Offset into attribute at which to start writing
 * \param nSize		Length of <pBuffer>
 * \return Number of octets written on success, negative error code on failure
 * \sa
 ****************************************************************************/
int tfs_write_attr( void *pVolume, void *pNode, const char *pzName, int nNameLen, int nFlags, int nType, const void *pBuffer, off_t nPos, size_t nSize )
{
	TmpFSVolume_s *psVolume = pVolume;
	TmpFSInode_s *psNode = pNode;
	TmpFSInode_s *psAttr;
	int nError = 0;

	printk("%s \"%s\"::\"%s\" pos %u size %u\n", __FUNCTION__, psNode->ti_zName, pzName, (unsigned)nPos, (unsigned)nSize);
	if( nPos + nSize > TFS_MAX_FILESIZE )
	{
		return ( -EFBIG );
	}
	if( nPos < 0 )
	{
		return ( -EINVAL );
	}

	TFS_VOLUME_LOCK( psVolume );
	psAttr = tfs_tree_find_attr( psNode, pzName, nNameLen );
	if ( psAttr == NULL )
	{
		psAttr = tfs_create_node( psNode, pzName, nNameLen, S_IFATTR );
	}
	TFS_VOLUME_UNLOCK( psVolume );

	if ( psAttr == NULL )
	{
		return ( -ENOMEM );
	}

	/* Now lock the attribute itself */
	TFS_INODE_LOCK ( psAttr );

	if ( ( nFlags & O_TRUNC ) && psAttr->ti_pBuffer )
	{ // Truncate the data area
		psAttr->ti_nSize = 0;
		kfree( psAttr->ti_pBuffer );
		psAttr->ti_pBuffer = NULL;
	}

	if( nPos + nSize > psAttr->ti_nSize || psAttr->ti_pBuffer == NULL )
	{
		void *pBuffer = kmalloc( nPos + nSize, MEMF_KERNEL | MEMF_CLEAR | MEMF_OKTOFAILHACK );

		if( pBuffer == NULL )
		{
			nError = -ENOMEM;
			goto error;
		}
		if( psAttr->ti_pBuffer != NULL )
		{
			memcpy( pBuffer, psAttr->ti_pBuffer, psAttr->ti_nSize );
			kfree( psAttr->ti_pBuffer );
		}
		psVolume->tv_nFSSize += nPos + nSize - psAttr->ti_nSize;
		psAttr->ti_pBuffer = pBuffer;
		psAttr->ti_nSize = nPos + nSize;
		printk("%s size %d\n", __FUNCTION__, psAttr->ti_nSize);
	}
	memcpy( psAttr->ti_pBuffer + nPos, pBuffer, nSize );
	nError = nSize;
	psAttr->ti_eAttrType = nType;

error:
	TFS_INODE_UNLOCK ( psAttr );
	return ( nError );
}

/** Read from an attribute of a tmpfs Inode
 * \par Description:
 * Read from the attribute of the given Inode with the give name into the given buffer.
 * \par Note:
 * \par Locks:
 * \par Warning:
 * \param pVolume	tmpfs Volume
 * \param pNode		Inode owning attribute
 * \param pzName	Name of attribute to read
 * \param nNameLen	Length of <pzName>
 * \param nType		Type of data to read
 * \param pBuffer	Out param: buffer to read into
 * \param nPos		Offset into attribute at which to start reading
 * \param nSize		Length of <pBuffer>
 * \return number of octets read on success, negative error code on failure
 * \sa
 ****************************************************************************/
int tfs_read_attr( void *pVolume, void *pNode, const char *pzName, int nNameLen, int nType, void *pBuffer, off_t nPos, size_t nSize )
{
	TmpFSVolume_s *psVolume = pVolume;
	TmpFSInode_s *psNode = pNode;
	TmpFSInode_s *psAttr;
	int nError = 0;

	printk("%s \"%s\"::\"%s\" pos %u size %u\n", __FUNCTION__, psNode->ti_zName, pzName, (unsigned)nPos, (unsigned)nSize);

	if( nPos < 0 )
	{
		return ( -EINVAL );
	}

	TFS_VOLUME_LOCK( psVolume );
	psAttr = tfs_tree_find_attr( psNode, pzName, nNameLen );
	TFS_VOLUME_UNLOCK( psVolume );

	if ( psAttr == NULL )
	{
		return ( -ENOENT );
	}

	if ( psAttr->ti_eAttrType != nType )
	{
		return ( -EINVAL );
	}

	/* Now lock the attribute itself */
	TFS_INODE_LOCK ( psAttr );
	if( psAttr->ti_pBuffer == NULL || nPos >= psAttr->ti_nSize )
	{
		nError = 0;
		goto error;
	}
	if( nPos + nSize > psAttr->ti_nSize )
	{
		nSize = psAttr->ti_nSize - nPos;
	}
	memcpy( pBuffer, psAttr->ti_pBuffer + nPos, nSize );
	nError = nSize;

error:
	TFS_INODE_UNLOCK ( psAttr );
	return ( nError );
}

/** Open the volume index directory
 * \par Description:
 * Open the index directory on the given volume.  Create a walk to use as the cookie.
 * \par Note:
 * \par Locks:
 * \par Warning:
 * \param pVolume	tmpfs Volume
 * \param ppCookie	Out param: cookie to be passed to read/rewind
 * \return 0 on success, negative error code on failure
 * \sa
 ****************************************************************************/
int tfs_open_indexdir( void *pVolume, void **ppCookie )
{
	TmpFSVolume_s *psVolume = pVolume;
	TmpFSCookie_s *psCookie;

	printk("%s\n", __FUNCTION__);
	psCookie = kmalloc( sizeof( TmpFSCookie_s ), MEMF_KERNEL | MEMF_CLEAR | MEMF_OKTOFAILHACK );
	if ( psCookie == NULL )
	{
		return ( -ENOMEM );
	}

	TFS_VOLUME_LOCK( psVolume );
	psCookie->tc_psWalk = tfs_tree_walk_start( psVolume->tv_psIndexDir, TMPFS_WALK_DIR );
	TFS_VOLUME_UNLOCK( psVolume );
	if ( psCookie->tc_psWalk == NULL )
	{
		kfree( psCookie);
		return ( -ENOMEM );
	}
	/* Indexdir doesn't have "." or "..".  Not that this should be passed to readdir, but... */
	psCookie->tc_nState = TMPFS_COOKIE_WALK;
	*ppCookie = psCookie;

	return ( 0 );
}

/** Close the volume index directory
 * \par Description:
 * Close the index directory on the given volume, previously opened.  Free the cookie.
 * \par Note:
 * \par Locks:
 * \par Warning:
 * \param pVolume	tmpfs Volume
 * \param pCookie	Cookie from open_indexdir
 * \return 0 on success, negative error code on failure
 * \sa
 ****************************************************************************/
int tfs_close_indexdir( void *pVolume, void *pCookie )
{
	TmpFSVolume_s *psVolume = pVolume;
	TmpFSCookie_s *psCookie = pCookie;

	printk("%s\n", __FUNCTION__);

	TFS_VOLUME_LOCK( psVolume );
	tfs_tree_walk_end( psCookie->tc_psWalk );
	TFS_VOLUME_UNLOCK( psVolume );

	kfree(psCookie);
	return ( 0 );
}

/** Rewind the volume index directory
 * \par Description:
 * Rewind the index directory on the given volume, prevously opened, so that the next call to
 * read_indexdir returns the first entry.
 * \par Note:
 * \par Warning:
 * \param pVolume	tmpfs Volume
 * \param pCookie	cookie returned from open_indexdir
 * \return 0 on success, negative error code on failure
 * \sa
 ****************************************************************************/
int tfs_rewind_indexdir( void *pVolume, void *pCookie )
{
	TmpFSVolume_s *psVolume = pVolume;
	TmpFSCookie_s *psCookie = pCookie;

	printk("%s\n", __FUNCTION__);
	if ( !psCookie->tc_psWalk )
	{
		return ( -EINVAL );
	}

	TFS_VOLUME_LOCK( psVolume );
	tfs_tree_walk_reset( psCookie->tc_psWalk );
	TFS_VOLUME_UNLOCK( psVolume );

	return ( 0 );
}

/** Read an entry from the volume index directory
 * \par Description:
 * Read the next entry from the index directory of the given volume.  Since we keep a walk, just
 * return the next walk entry.
 * \par Note:
 * \par Locks:
 * \par Warning:
 * \param pVolume	tmpfs Volume
 * \param pCookie	Cookie from open
 * \param psFileInfo	Out param: Directory entry data to fill
 * \param nBufSize	Unused (size of <psFileInfo>, always sizeof(kernel_dirent))
 * \return 0 if directory is empty, positive number of entries on success
 * \sa
 ****************************************************************************/
int tfs_read_indexdir( void *pVolume, void *pCookie, struct kernel_dirent *psFileInfo, size_t nBufSize )
{
	TmpFSVolume_s *psVolume = pVolume;
	TmpFSCookie_s *psCookie = pCookie;
	TmpFSInode_s *psIndex;
	int nNumFound = 0;

	printk("%s\n", __FUNCTION__);

	if ( !psCookie )
	{
		return ( -ENOMEM );
	}

	if ( !psCookie->tc_psWalk )
	{
		return ( -EINVAL );
	}

	TFS_VOLUME_LOCK( psVolume );
	psIndex = tfs_tree_walk_next( psCookie->tc_psWalk );
	if ( psIndex == NULL )
	{
		goto error;
	}
	printk("%s adding %s\n", __FUNCTION__, psIndex->ti_zName);
	strcpy( psFileInfo->d_name, psIndex->ti_zName );
	psFileInfo->d_namlen = psIndex->ti_nNameLen;
	psFileInfo->d_ino = psIndex->ti_nInodeNum;
	nNumFound++;

error:
	TFS_VOLUME_UNLOCK( psVolume );
	return ( nNumFound );
}

/** Create an attribute index
 * \par Description:
 * Create an attribute index with the given name and type on the given volume.
 * \par Note:
 * \par Locks:
 * \par Warning:
 * \param pVolume	tmpfs Volume
 * \param pzName	Name of index to create
 * \param nNameLen	Length of <pzName>
 * \param nType		Type of attribute to index
 * \param nFlags	Write flags (O_TRUNC, etc.)
 * \return 0 on success, negative error code on failure
 * \sa
 ****************************************************************************/
int tfs_create_index( void *pVolume, const char *pzName, int nNameLen, int nType, int nFlags )
{
	TmpFSVolume_s *psVolume = pVolume;
	TmpFSInode_s *psIndex;

	printk("%s \"%s\"::\"%s\" type %u\n", __FUNCTION__, psVolume->tv_psIndexDir->ti_zName, pzName, (unsigned)nType);

	TFS_VOLUME_LOCK( psVolume );
	psIndex = tfs_tree_find_attr( psVolume->tv_psIndexDir, pzName, nNameLen );
	TFS_VOLUME_UNLOCK( psVolume );
	if ( psIndex != NULL )
	{
		return ( -EEXIST );
	}

	TFS_VOLUME_LOCK( psVolume );
	psIndex = tfs_create_node( psVolume->tv_psIndexDir, pzName, nNameLen, S_IFINDEX );
	TFS_VOLUME_UNLOCK( psVolume );

	if ( psIndex == NULL )
	{
		return ( -ENOMEM );
	}

	psIndex->ti_eAttrType = nType;
	psIndex->ti_nUID = 0;
	psIndex->ti_nGID = 0;

	return ( 0 );
}

/** Remove an attribute index
 * \par Description:
 * Remove the attribute index with the given name from the given volume
 * \par Note:
 * \par Locks:
 * \par Warning:
 * \param pVolume	tmpfs Volume
 * \param pzName	Name of index to create
 * \param nNameLen	Length of <pzName>
 * \return 0 on success, negative error code on failure
 * \sa
 ****************************************************************************/
int tfs_remove_index( void *pVolume, const char *pzName, int nNameLen )
{
	TmpFSVolume_s *psVolume = pVolume;
	TmpFSInode_s *psIndex;
	int nError = 0;

	printk("%s \"%s\"::\"%s\"\n", __FUNCTION__, psVolume->tv_psIndexDir->ti_zName, pzName);

	TFS_VOLUME_LOCK( psVolume );
	psIndex = tfs_tree_find_attr( psVolume->tv_psIndexDir, pzName, nNameLen );
	if ( psIndex == NULL )
	{
		nError = -ENOENT;
		goto error;
	}

	tfs_tree_remove( psVolume->tv_psIndexDir, psIndex );

error:
	TFS_VOLUME_UNLOCK( psVolume );
	return ( nError );
}

/** Get stat info on an attribute index
 * \par Description:
 * Get the stat info on the attribute index with the given name on the given volume.
 * \par Note:
 * \par Locks:
 * \par Warning:
 * \param pVolume	tmpfs Volume
 * \param pzName	Name of index to create
 * \param nNameLen	Length of <pzName>
 * \param psBuffer	Out param: Index_info struct to fill
 * \return 0 on success, negative error code on failure
 * \sa
 ****************************************************************************/
int tfs_stat_index( void *pVolume, const char *pzName, int nNameLen, struct index_info *psBuffer )
{
	TmpFSVolume_s *psVolume = pVolume;
	TmpFSInode_s *psIndex;

	printk("%s \"%s\"::\"%s\"\n", __FUNCTION__, psVolume->tv_psIndexDir->ti_zName, pzName);

	TFS_VOLUME_LOCK( psVolume );
	psIndex = tfs_tree_find_attr( psVolume->tv_psIndexDir, pzName, nNameLen );
	TFS_VOLUME_UNLOCK( psVolume );
	if ( psIndex == NULL )
	{
		return ( -ENOENT );
	}

	psBuffer->ii_type = psIndex->ti_eAttrType;
	psBuffer->ii_size = psIndex->ti_nSize;
	psBuffer->ii_mtime = psIndex->ti_nMTime;
	psBuffer->ii_ctime = psIndex->ti_nCTime;
	psBuffer->ii_uid = psIndex->ti_nUID;
	psBuffer->ii_gid = psIndex->ti_nGID;

	return ( 0 );
}

/** Unmount a tmpfs volume
 * \par Description:
 * Unmount the given tmpfs volume.  Basically, walk the entire filesystem freeing all the data.
 * \par Note:
 * \par Locks:
 * \par Warning:
 * \param pVolume	Volume pointer
 * \return 0 on success, negative error code on failure
 * \sa
 ****************************************************************************/
static int tfs_unmount( void *pVolume )
{
	TmpFSVolume_s *psVolume = pVolume;

	printk("%s\n", __FUNCTION__);
	TFS_VOLUME_LOCK( psVolume );
	tfs_tree_delete_all( psVolume->tv_psIndexDir );
	tfs_tree_delete_all( psVolume->tv_psRootNode );
	TFS_VOLUME_UNLOCK( psVolume );
	delete_semaphore( psVolume->tv_hLock );
	kfree( psVolume );
	return ( 0 );
}

/** FSOperations_s for tmpfs
 * \par Description:
 * This is the list of all possible filesystem operations for tmpfs.
 ****************************************************************************/
static FSOperations_s g_sOperations = {
	NULL,			///< op_probe
	tfs_mount,		///< op_mount
	tfs_unmount,		///< op_unmount
	tfs_read_inode,		///< op_read_inode
	tfs_write_inode,	///< op_write_inode
	tfs_lookup,		///< op_locate_indoe
	NULL,			///< op_access
	tfs_create,		///< op_create
	tfs_mkdir,		///< op_mkdir
	NULL,			///< op_mknod
	tfs_symlink,		///< op_symlink
	NULL,			///< op_link
	tfs_rename,		///< op_rename
	tfs_unlink,		///< op_unlink
	tfs_rmdir,		///< op_rmdir
	tfs_readlink,		///< op_readlink
	NULL,			///< op_oppendir
	NULL,			///< op_closedir
	tfs_rewinddir,		///< op_rewinddir
	tfs_readdir,		///< op_readdir
	tfs_open,		///< op_open
	tfs_close,		///< op_close
	NULL,			///< op_free_cookie
	tfs_read,		///< op_read
	tfs_write,		///< op_write
	NULL,			///< op_readv
	NULL,			///< op_writev
	NULL,			///< op_ioctl
	NULL,			///< op_setflags
	tfs_rstat,		///< op_rstat
	tfs_wstat,		///< op_wstat
	NULL,			///< op_fsync
	NULL,			///< op_initialize
	NULL,			///< op_sync
	tfs_rfsstat,		///< op_rfsstat
	NULL,			///< op_wfsstat
	NULL,			///< op_isatty
	NULL,			///< op_add_select_req
	NULL,			///< op_rem_select_req
	tfs_open_attrdir,	///< op_open_attrdir
	tfs_close_attrdir,	///< op_close_attrdir
	tfs_rewind_attrdir,	///< op_rewind_attrdir
	tfs_read_attrdir,	///< op_read_attrdir
	tfs_remove_attr,	///< op_remove_attr
	NULL,			///< op_rename_attr (unused)
	tfs_stat_attr,		///< op_stat_attr
	tfs_write_attr,		///< op_write_attr
	tfs_read_attr,		///< op_read_attr
	tfs_open_indexdir,	///< op_open_indexdir
	tfs_close_indexdir,	///< op_close_indexdir
	tfs_rewind_indexdir,	///< op_rewind_indexdir
	tfs_read_indexdir,	///< op_read_indexdir
	tfs_create_index,	///< op_create_index
	tfs_remove_index,	///< op_remove_index
	NULL,			///< op_rename_index
	tfs_stat_index,		///< op_stat_index
	NULL			///< op_get_file_blocks
};


int fs_init( const char *pzName, FSOperations_s ** ppsOps )
{
	printk("%s version 1.6\n", __FUNCTION__);
	*ppsOps = &g_sOperations;
	return ( FSDRIVER_API_VERSION );
}
