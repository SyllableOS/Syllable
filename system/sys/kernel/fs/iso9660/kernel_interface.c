
/*
**		Copyright 1999, Be Incorporated.   All Rights Reserved.
**		This file may be used under the terms of the Be Sample Code License.
**		
**		2003 - Ported to Atheos/Syllable by Ville Kallioniemi (ville.kallioniemi@abo.fi)
*/

#include "iso.h"

// SYLLABLE
#include <atheos/stdlib.h>
#include <atheos/string.h>
#include <atheos/kernel.h>
#include <atheos/device.h>

#include <posix/stat.h>
#include <posix/errno.h>
#include <posix/ioctl.h>

#include <atheos/time.h>
#include <atheos/filesystem.h>
#include <atheos/bcache.h>
#include <macros.h>

// Probe is called _only_ if you don't provide the fs type when you mount a volume,
static int iso_probe( const char *devicePath, fs_info * filesystemInfo );

// Start of fundamental (read-only) required functions

static int iso_mount( kdev_t nsid, const char *device, uint32 flags, const void *parms, int len, void **data, ino_t *vnid );
static int iso_unmount( void *_ns );
static int iso_walk( void *ns, void *parentNode, const char *name, int name_len, ino_t *inode );
static int iso_read_vnode( void *_ns, ino_t vnid, void **node );
static int iso_write_vnode( void *_ns, void *_node );
static int iso_read_stat( void *_ns, void *_node, struct stat *st );

static int iso_open( void *_ns, void *_node, int omode, void **cookie );
static int iso_close( void *ns, void *node, void *cookie );

static int iso_read( void *_ns, void *_node, void *cookie, off_t pos, void *buf, size_t len );

static int iso_free_cookie( void *ns, void *node, void *cookie );

static int iso_access( void *_ns, void *_node, int mode );
static int iso_open_dir( void *_ns, void *_node, void **cookie );

// fs_readdir - read 1 or more dirents, keep state in cookie, return
//                                      0 when no more entries.
static int iso_read_dir( void *_ns, void *_node, void *cookie, int pos, struct kernel_dirent *buf, size_t bufsize );
static int iso_rewind_dir( void *_ns, void *_node, void *cookie );
static int iso_close_dir( void *_ns, void *_node, void *cookie );
static int iso_free_dircookie( void *_ns, void *_node, void *cookie );
static int iso_read_fs_stat( void *_ns, fs_info * );
static int iso_read_link( void *_ns, void *_node, char *buf, size_t bufsize );

// End of fundamental (read-only) required functions.

// NOTE!!! The commented out functions do not exist in dosfs or afs
// Are they even implemented in the vfs? 
// dosfs: open and close handle both dirs and files + freeing the cookies
static FSOperations_s iso_operations = {
	iso_probe,		// probe
	iso_mount,		// mount 
	iso_unmount,		// unmount 
	iso_read_vnode,		// read_vnode 
	iso_write_vnode,	// write_vnode 
	iso_walk,		// locate_inode
	iso_access,		// access 
	NULL,			// create
	NULL,			// mkdir
	NULL,			// mknod
	NULL,			// symlink
	NULL,			// link
	NULL,			// rename
	NULL,			// unlink
	NULL,			// rmdir
	iso_read_link,		// readlink
	NULL,			// opendir - is this used for syllable???
	NULL,			// closedir  - is this used for syllable???
	iso_rewind_dir,		// rewinddir
	iso_read_dir,		// readdir 
	iso_open,		// open file
	iso_close,		// close file
	NULL,			// free cookie 
	iso_read,		// read file 
	NULL,			// write file 
	NULL,			// readv
	NULL,			// writev
	NULL,			// ioctl 
	NULL,			// setflags file 
	iso_read_stat,		// rstat 
	NULL,			// wstat 
	NULL,			// fsync 
	NULL,			// initialize
	NULL,			// sync
	iso_read_fs_stat,	// rfsstat 
	NULL,			// wfsstat 
	NULL,			// isatty
	NULL,			// add_select_req
	NULL,			// rem_select_req
	NULL,			// open_attrdir
	NULL,			// close_attrdir
	NULL,			// rewind_attrdir
	NULL,			// read_attrdir
	NULL,			// remove_attr
	NULL,			// rename_attr
	NULL,			// stat_attr
	NULL,			// write_attr
	NULL,			// read_attr
	NULL,			// open_indexdir
	NULL,			// close_indexdir
	NULL,			// rewind_indexdir
	NULL,			// read_indexdir
	NULL,			// create_index
	NULL,			// remove_index
	NULL,			// rename_index
	NULL,			// stat_index
	NULL,			// get_file_blocks
	NULL			// truncate
};

int32 api_version = FSDRIVER_API_VERSION;
static char *gFSName = "ISO9660\0";

static int iso_probe( const char *devicePath, fs_info * info )
{
	int device = -1;
	int error = -1;
	nspace *vol = 0;
	int i;

	kerndbg( KERN_DEBUG, "%s: probe( %s , %p )\n", gFSName, devicePath, info );

	device = open( devicePath, O_RDONLY );
	if( device < 0 )
	{
		kerndbg( KERN_WARNING, "iso_probe failed to open device %s\n", devicePath );
		return ( device );
	}

	error = ISOMount( devicePath, 0, &vol, false, false );
	if( error < 0 )
	{
		kerndbg( KERN_DEBUG_LOW, "iso_probe - failed to mount device %s\n -> not an iso volume", devicePath );
		close( device );
		return ( error );
	}
	else
	{
		kerndbg( KERN_DEBUG, "iso_probe - mount suceeded: %s\n is an ISO volume", devicePath );
	}

	// Fill in device id.
	info->fi_dev = -1;

	// Root vnode ID
	info->fi_root = vol->rootDirRec.id;

	// File system flags.
	info->fi_flags = FS_CAN_MOUNT;

	// FS block size.
	info->fi_block_size = vol->logicalBlkSize[FS_DATA_FORMAT];
	kerndbg( KERN_DEBUG_LOW, "iso_probe set the logical block size to %d\n", info->fi_block_size );

	// IO size - specifies buffer size for file copying
	info->fi_io_size = 65536;

	// Total blocks?
	info->fi_total_blocks = vol->volSpaceSize[FS_DATA_FORMAT];

	// Free blocks = 0, read only
	info->fi_free_blocks = 0;
	info->fi_free_user_blocks = 0;

	// Device name.
	strncpy( info->fi_device_path, vol->devicePath, sizeof( info->fi_device_path ) );

	// Volume name
	strncpy( info->fi_volume_name, vol->volIDString, sizeof( info->fi_volume_name ) );
	for( i = strlen( info->fi_volume_name ) - 1; i >= 0; i-- )
	{
		if( info->fi_volume_name[i] != ' ' )
			break;
	}

	if( i < 0 )
		strcpy( info->fi_volume_name, "UNKNOWN" );
	else
		info->fi_volume_name[i + 1] = 0;

	// File system name
	strcpy( info->fi_driver_name, gFSName );

	// free the resources
	close( device );
	free( vol );

	kerndbg( KERN_DEBUG, "iso_probe - EXIT" );

	return 0;
}



static int iso_mount( kdev_t volumeID, const char *devicePath, uint32 flags, const void *parameters, int len, void **data, ino_t *rootNodeID )
{
	int result = -EINVAL;
	nspace *volume;
	bool allow_rockridge, allow_joliet;

	kerndbg( KERN_DEBUG, "iso_mount - ENTER\n" );
	kerndbg( KERN_DEBUG, "%s: volumeID: %d device paht: %s\n", gFSName, volumeID, devicePath );

	//printk("iso_mount - Parameters(%d): %s\n", len, (char*)parameters );

	// here we should parse the options and act accordingly
	allow_rockridge = true;
	allow_joliet = true;

	if( parameters )
	{
		char *pzParameters = ( char * )kmalloc( len + 1, MEMF_KERNEL | MEMF_CLEAR );

		if( pzParameters )
		{
			memcpy( pzParameters, parameters, len );
			pzParameters[len] = '\0';

			if( strstr( pzParameters, "nojoliet" ) )
			{
				kerndbg( KERN_INFO, "iso_mount - disabling Joliet\n" );
				allow_joliet = false;
			}
			if( strstr( pzParameters, "norockridge" ) || strstr( pzParameters, "norr" ) )
			{
				kerndbg( KERN_INFO, "iso_mount - disabling RockRidge\n" );
				allow_rockridge = false;
			}

			kfree( pzParameters );
		}
		else
		{
			kerndbg( KERN_WARNING, "iso_mount - unable to allocate space for pzParameters\n" );
		}
	}

	// flags is currently unused
	result = ISOMount( devicePath, flags, &volume, allow_rockridge, allow_joliet );

	// If the mount succeeded, setup the block cache
	if( result == EOK )
	{
		int error = 0;

		// volumeSize is in blocks
		off_t volumeSize = volume->volSpaceSize[FS_DATA_FORMAT];

		// root node is a special case and has always value 1
		*rootNodeID = volume->rootDirRec.id;
		*data = ( void * )volume;
		volume->id = volumeID;

		kerndbg( KERN_DEBUG_LOW, "iso_mount - cache init: dev %d, max blocks %Ld\n", volume->fd, volumeSize );

		// setup_device_cache expects the number of blocks on the device
		error = setup_device_cache( volume->fd, volume->id, volumeSize );
		if( error < 0 )
		{
			kerndbg( KERN_WARNING, "iso_mount - failed to setup device cache.\n" );
			result = error;
			close( volume->fd );
			free( volume );
		}
	}

	kerndbg( KERN_DEBUG, "iso_mount - EXIT ( %d )\n", result );
	return result;
}

static int iso_unmount( void *_volume )
{
	int result = -EOK;

	nspace *volume = ( nspace * ) _volume;

	kerndbg( KERN_DEBUG, "iso_unmount - ENTER\n" );
	kerndbg( KERN_DEBUG_LOW, "iso_unmount - flush the device cache\n" );
	flush_device_cache( volume->fd, false );
	kerndbg( KERN_DEBUG_LOW, "iso_unmount - shutdown device cache\n" );
	shutdown_device_cache( volume->fd );
	kerndbg( KERN_DEBUG_LOW, "iso_unmount - closing volume\n" );
	result = close( volume->fd );
	kerndbg( KERN_DEBUG_LOW, "iso_unmount - freeing volume data\n" );
	free( volume );
	kerndbg( KERN_DEBUG, "iso_unmount - EXIT ( %d )\n", result );
	return result;
}

// fs_rfsstat - Fill in fs_info struct for device.
static int iso_read_fs_stat( void *_volume, fs_info * fsInfo )
{
	nspace *volume = ( nspace * ) _volume;
	int i;

	kerndbg( KERN_DEBUG, "iso_read_fs_stat - ENTER\n" );

	// Device ID
	fsInfo->fi_dev = volume->id;

	// Root inode number
	fsInfo->fi_root = volume->rootDirRec.id;

	// File system flags.
	fsInfo->fi_flags = FS_IS_PERSISTENT | FS_IS_READONLY | FS_IS_BLOCKBASED | FS_IS_REMOVABLE;

	// FS block size.
	fsInfo->fi_block_size = volume->logicalBlkSize[FS_DATA_FORMAT];

	// IO size - specifies buffer size for file copying
	fsInfo->fi_io_size = 65536;

	// Total blocks?
	fsInfo->fi_total_blocks = volume->volSpaceSize[FS_DATA_FORMAT];

	// Free blocks = 0, read only
	fsInfo->fi_free_blocks = 0;

	// Number of blocks available for non-root users
	fsInfo->fi_free_user_blocks = 0;

	// Total number of inodes (-1 if inodes are allocated dynamically)
	fsInfo->fi_total_inodes = -1;

	// Number of free inodes (-1 if inodes are allocated dynamically)
	fsInfo->fi_free_inodes = -1;


	// Device name.
	strncpy( fsInfo->fi_device_path, volume->devicePath, sizeof( fsInfo->fi_device_path ) );

	strncpy( fsInfo->fi_volume_name, volume->volIDString, sizeof( fsInfo->fi_volume_name ) );
	for( i = strlen( fsInfo->fi_volume_name ) - 1; i >= 0; i-- )
	{
		if( fsInfo->fi_volume_name[i] != ' ' )
			break;
	}

	if( i < 0 )
		strcpy( fsInfo->fi_volume_name, "UNKNOWN" );
	else
		fsInfo->fi_volume_name[i + 1] = 0;

	// File system name
	strcpy( fsInfo->fi_driver_name, gFSName );

	kerndbg( KERN_DEBUG, "iso_read_fs_stat - EXIT\n" );
	return -EOK;
}

static int iso_walk( void *_volume, void *_baseNode, const char *_fileName, int _nameLength, ino_t *_vNodeID )
{
	nspace *volume = ( nspace * ) _volume;
	vnode *baseNode = ( vnode * )_baseNode;
	size_t nameLength = _nameLength;
	ino_t *vNodeID = _vNodeID;

	//vnode *       newNode = NULL;
	char fileName[nameLength + 1];
	bool done = false;
	int result = -ENOENT;

	char *directoryName = baseNode->fileIDString;


	kerndbg( KERN_DEBUG, "iso_walk - Enter\n" );

	if( volume == NULL || baseNode == NULL )
	{
		done = true;
		kerndbg( KERN_WARNING, "iso_walk - ERROR!\n" );
		kerndbg( KERN_WARNING, "iso_walk - volume = %p , baseNode = %p!!\n", volume, baseNode );
	}

	if( !done && ( nameLength > ISO_MAX_FILENAME_LENGTH ) )
	{
		done = true;
		result = -ENAMETOOLONG;
		kerndbg( KERN_WARNING, "iso_walk - fileName length (%d) too long!\n", nameLength );
	}
	else
	{
		strncpy( fileName, _fileName, nameLength + 1 );
		fileName[nameLength] = '\0';
		kerndbg( KERN_DEBUG, "iso_walk - In directory: %s\n", directoryName );
		kerndbg( KERN_DEBUG, "iso_walk - Looking for file: %s of length: %d \n", fileName, nameLength );
	}

	if( !done )
	{
		size_t totalBytesRead = 0;
		size_t directoryLength = baseNode->dataLen[FS_DATA_FORMAT];
		off_t block = baseNode->startLBN[FS_DATA_FORMAT];
		size_t blockSize = volume->logicalBlkSize[FS_DATA_FORMAT];

		// read in blocks until the file is found or we've reached the end of the directory
		while( !done && ( totalBytesRead < directoryLength ) )
		{
			bool blockReleased = false;
			char *blockData = get_cache_block( volume->fd, block, blockSize );

			kerndbg( KERN_DEBUG_LOW, "iso_walk - read %d / %d of directory %s\n", totalBytesRead, directoryLength, directoryName );

			if( blockData == NULL )
			{
				done = true;
				// prevent trying to release a block that we weren't able to read
				blockReleased = true;
				result = -ENOENT;	// ??
			}
			else
			{
				off_t blockBytesRead = 0;

				// move inside a block
				// ECMA:"Each Directory Record shall end in the Logical Sector in which it begins."
				// ECMA:"Unused byte positions after the last Directory Record in a Logical Sector
				//       shall be set to (00)."

				while( !done && ( blockData[0] != 0 ) && ( ( totalBytesRead + blockBytesRead ) < directoryLength ) && ( blockBytesRead < blockSize ) )
				{
					size_t nodeBytesRead = 0;
					vnode node;
					char nodeFileName[ISO_MAX_FILENAME_LENGTH];
					char nodeSymbolicLinkName[ISO_MAX_FILENAME_LENGTH];

					node.fileIDString = nodeFileName;
					node.attr.slName = nodeSymbolicLinkName;

					result = InitNode( volume, &node, blockData, &nodeBytesRead, volume->joliet_level );

					if( result != EOK )
					{
						done = true;
						result = -ENOENT;
						kerndbg( KERN_DEBUG, "iso_walk - couldn't initialize a node\n" );
					}
					else
					{
						if( ( strlen( node.fileIDString ) == strlen( fileName ) ) && ( strncmp( node.fileIDString, fileName, strlen( fileName ) ) == 0 ) )
						{
							done = true;
							release_cache_block( volume->fd, block );
							blockReleased = true;
							kerndbg( KERN_DEBUG_LOW, "iso_walk - success, found vnode at block %Ld, pos %Ld\n", block, blockBytesRead );

							*vNodeID = BLOCK_TO_INO( block, blockBytesRead );

							kerndbg( KERN_DEBUG_LOW, "iso_walk - New vnode id is %Ld\n", *vNodeID );

							if( fileName[0] == '.' && fileName[1] == '.' )
							{
								vnode *parentNode = NULL;

								iso_read_vnode( volume, *vNodeID, ( void ** )&parentNode );

								if( parentNode->startLBN[FS_DATA_FORMAT] == volume->rootBlock )
								{
									kerndbg( KERN_DEBUG, "iso_walk : Directory entry for volume root\n" );
									*vNodeID = volume->rootDirRec.id;
								}
								else
								{
									kerndbg( KERN_DEBUG_LOW, "iso_walk : Directory entry for non volume root\n" );
									*vNodeID = BLOCK_TO_INO( parentNode->startLBN[FS_DATA_FORMAT], 0 );
								}
								
								iso_write_vnode( volume, parentNode );
							}
							result = -EOK;
						}
						else
						{
							kerndbg( KERN_DEBUG_LOW, "iso_walk - Not the file we're looking for\n" );
							result = -ENOENT;
						}
					}	// InitNode
					blockData += nodeBytesRead;
					blockBytesRead += nodeBytesRead;
				}	// while inside a block
				if( blockReleased == false )
					release_cache_block( volume->fd, block );
				totalBytesRead += blockSize;
				block++;
			}	// cacheBlock != NULL
		}		// while inside a directory
		kerndbg( KERN_DEBUG_LOW, "iso_walk - read a total of %d / %d\n", totalBytesRead, directoryLength );
	}			// traversing the dir


	kerndbg( KERN_DEBUG, "iso_walk - EXIT (%d)\n", result );
	return result;
}

static int iso_read_vnode( void *_ns, ino_t vnid, void **node )
{
	off_t block = 0;
	off_t pos = 0;
	nspace *ns = ( nspace * ) _ns;
	int result = -EOK;
	vnode *newNode = ( vnode * )calloc( sizeof( vnode ), 1 );


	pos = ( vnid & 0x3FFFFFFF );
	block = ( vnid >> 30 );

	kerndbg( KERN_DEBUG, "iso_read_vnode - ENTER\n" );
	kerndbg( KERN_DEBUG_LOW, "iso_read_vnode - block = %Ld, pos = %Ld, raw = %Ld node %p\n", block, pos, vnid, newNode );

	if( newNode != NULL )
	{
		// root node is stored in the volume struct
		if( vnid == ns->rootDirRec.id )
		{
			kerndbg( KERN_DEBUG, "iso_read_vnode - root node requested.\n" );
			memcpy( newNode, &( ns->rootDirRec ), sizeof( vnode ) );
			*node = ( void * )newNode;
		}
		else
		{
			// OK, it's something non-trivial
			// check that the position makes sense
			if( pos > ns->logicalBlkSize[FS_DATA_FORMAT] )
			{
				kerndbg( KERN_WARNING, "iso_read_vnode - position is outside of the block!\n" );
				result = -EINVAL;
			}
			else
			{
				off_t cachedBlock = block;
				char *blockData = NULL;

				newNode->fileIDString = ( char * )calloc( ISO_MAX_FILENAME_LENGTH, 1 );
				newNode->attr.slName = ( char * )calloc( ISO_MAX_FILENAME_LENGTH, 1 );
				blockData = ( char * )get_cache_block( ns->fd, block, ns->logicalBlkSize[FS_DATA_FORMAT] );
				if( blockData != NULL )
				{
					result = InitNode( ns, newNode, ( blockData + pos ), NULL, ns->joliet_level );
					newNode->id = vnid;
					kerndbg( KERN_DEBUG_LOW, "iso_read_vnode - init result is %d\n", result );
					*node = ( void * )newNode;
					kerndbg( KERN_DEBUG_LOW, "iso_read_vnode - new file %s, size %d\n", newNode->fileIDString, newNode->dataLen[FS_DATA_FORMAT] );
					release_cache_block( ns->fd, cachedBlock );
				}
				else
				{
					free( newNode->fileIDString );
					free( newNode->attr.slName );
					free( newNode );
					kerndbg( KERN_DEBUG, "iso_read_vnode - Could not read the block!\n" );
					return ( -EIO );	// is this the right error code ??? FIXME
				}
			}
		}
	}
	else
	{
		result = -ENOMEM;
	}

	kerndbg( KERN_DEBUG, "iso_read_vnode - EXIT (%d)\n", result );
	return result;
}


static int iso_write_vnode( void *_volume, void *_node )
{

	int result = -EOK;
	vnode *node = ( vnode * )_node;
	nspace *vol = ( nspace * ) _volume;

	// free the resources owned by the vnode
	kerndbg( KERN_DEBUG, "iso_write_vnode - ENTER \n" );

	if( node != NULL )
	{
		if( node->id != vol->rootDirRec.id )
		{
			if( node->fileIDString != NULL )
				free( node->fileIDString );
			if( node->attr.slName != NULL )
				free( node->attr.slName );
			free( node );
			node = NULL;
		}
	}
	else
	{
		kerndbg( KERN_WARNING, "iso_write_vnode - called with node = NULL!!!\n" );
	}
	kerndbg( KERN_DEBUG, "iso_write_vnode - EXIT (%d)\n", result );
	return result;
}


static int iso_read_stat( void *_volume, void *_node, struct stat *st )
{
	nspace *volume = ( nspace * ) _volume;
	vnode *node = ( vnode * )_node;
	int result = -EOK;
	time_t time;
	vnode *parentNode;

	kerndbg( KERN_DEBUG, "iso_rstat - ENTER\n" );

	st->st_dev = volume->id;
//      st->st_ino = node->id;

	// KV
	get_vnode( volume->id, node->id, ( void ** )&parentNode );
	st->st_ino = BLOCK_TO_INO( parentNode->startLBN[FS_DATA_FORMAT], 0 );

	st->st_nlink = node->attr.stat[FS_DATA_FORMAT].st_nlink;
	st->st_uid = node->attr.stat[FS_DATA_FORMAT].st_uid;
	st->st_gid = node->attr.stat[FS_DATA_FORMAT].st_gid;
	st->st_blksize = 65536;
	st->st_mode = node->attr.stat[FS_DATA_FORMAT].st_mode;
	// Same for file/dir in ISO9660
	st->st_size = node->dataLen[FS_DATA_FORMAT];
	if( ConvertRecDate( &( node->recordDate ), &time ) == -EOK )
		st->st_ctime = st->st_mtime = st->st_atime = time;

	put_vnode( volume->id, node->id );
	kerndbg( KERN_DEBUG, "iso_rstat - EXIT (%d)\n", result );

	return result;
}


static int iso_read( void *_ns, void *_node, void *cookie, off_t pos, void *buf, size_t len )
{
	nspace *ns = ( nspace * ) _ns;
	vnode *node = ( vnode * )_node;
	uint16 blockSize = ns->logicalBlkSize[FS_DATA_FORMAT];
	uint32 startBlock = node->startLBN[FS_DATA_FORMAT] + ( pos / blockSize );
	off_t blockPos = pos % blockSize;
	off_t numBlocks = 0;
	uint32 dataLen = node->dataLen[FS_DATA_FORMAT];
	int result = -EOK;
	size_t endLen = 0;
	size_t reqLen = len;
	size_t startLen = 0;
	size_t bytesRead = 0;

	kerndbg( KERN_DEBUG, "iso_read - ENTER\n" );
	if( ( node->flags & ISO_ISDIR ) )
	{
		kerndbg( KERN_WARNING, "iso_read - tried to read  a directory\n" );
		return ( -EISDIR );
	}

	if( pos < 0 )
		pos = 0;

	// If passed-in requested length is bigger than file size, change it to file size.
	if( ( reqLen + pos ) > dataLen )
	{
		reqLen = dataLen - pos;
	}

	// Compute the length of the partial start-block read, if any.
	if( ( blockPos + reqLen ) <= blockSize )
	{
		startLen = reqLen;
	}
	else if( blockPos > 0 )
	{
		startLen = blockSize - blockPos;
	}

	if( blockPos == 0 && reqLen >= blockSize )
	{
		kerndbg( KERN_DEBUG_LOW, "Setting startLen to 0, even block read\n" );
		startLen = 0;
	}
	// Compute the length of the partial end-block read, if any.
	if( reqLen + blockPos > blockSize )
	{
		endLen = ( reqLen + blockPos ) % blockSize;
	}

	// Compute the number of middle blocks to read.
	numBlocks = ( ( reqLen - endLen - startLen ) / blockSize );

	// If pos >= file length, return length of 0.
	if( pos >= dataLen )
	{
		kerndbg( KERN_DEBUG_LOW, "iso_read - End of file reached\n" );
	}
	else
	{
		// Read in the first, potentially partial, block.
		if( startLen > 0 )
		{
			off_t cachedBlock = startBlock;
			char *blockData = ( char * )get_cache_block( ns->fd, startBlock, blockSize );

			kerndbg( KERN_DEBUG_LOW, "iso_read - getting block %u\n", startBlock );
			if( blockData != NULL )
			{
				kerndbg( KERN_DEBUG_LOW, "iso_read - copying first block, len is %d.\n", startLen );
				memcpy( buf, blockData + blockPos, startLen );
				bytesRead += startLen;
				release_cache_block( ns->fd, cachedBlock );
				startBlock++;
			}
			else
				result = -EIO;
		}

		// Read in the middle blocks. (Should read to a buffer and copied to uspace?)
		if( numBlocks > 0 && result == -EOK )
		{
			kerndbg( KERN_DEBUG_LOW, "iso_read - getting middle blocks\n" );
			result = cached_read( ns->fd, startBlock, ( ( char * )buf ) + startLen, numBlocks, blockSize );
			if( result == -EOK )
			{
				bytesRead += blockSize * numBlocks;
			}
		}

		// Read in the last partial block.
		if( result == -EOK && endLen > 0 )
		{
			off_t endBlock = startBlock + numBlocks;
			char *endBlockData = ( char * )get_cache_block( ns->fd, endBlock, blockSize );

			kerndbg( KERN_DEBUG_LOW, "iso_read - getting end block\n" );
			if( endBlockData != NULL )
			{
				char *endBuf = ( ( char * )buf ) + ( reqLen - endLen );

				memcpy( endBuf, endBlockData, endLen );
				release_cache_block( ns->fd, endBlock );
				bytesRead += endLen;
			}
			else
				result = -EIO;
		}
	}
	kerndbg( KERN_DEBUG_LOW, "iso_read - read %d bytes\n", bytesRead );
	kerndbg( KERN_DEBUG, "iso_read - EXIT (%d)\n", result );
	if( result == -EOK )
		result = bytesRead;
	return result;
}

static int iso_open( void *_volume, void *_node, int omode, void **cookie )
{
	int result = -EOK;
	vnode *node = ( vnode * )_node;

	kerndbg( KERN_DEBUG, "iso_open - ENTER\n" );

	if( _volume == NULL )
		kerndbg( KERN_WARNING, "iso_open - called with volume == NULL\n" );

	//afs returns -EISDIR when trying to open a dir, but doesn't have a opendir call...
	//FIXME!!!
	//dosfs handles both dirs and files here as in the close too..

	// Do not allow any of the write-like open modes to get by 
	if( ( omode == O_WRONLY ) || ( omode == O_RDWR ) )
		result = -EROFS;
	else if( ( omode & O_TRUNC ) || ( omode & O_CREAT ) )
		result = -EROFS;


	if( ( node->flags & ISO_ISDIR ) )
	{
		kerndbg( KERN_DEBUG_LOW, "iso_open - tried to open a directory -> calling iso_open_dir\n" );
		result = iso_open_dir( _volume, _node, cookie );
	}

	// No cookie info needed for files, just return.
	kerndbg( KERN_DEBUG, "iso_open - EXIT (%d)\n", result );

	return result;
}

static int iso_close( void *_volume, void *_node, void *cookie )
{
	int result = -EOK;
	vnode *node = ( vnode * )_node;
	nspace *volume = ( nspace * ) _volume;

	kerndbg( KERN_DEBUG, "iso_close - ENTER\n" );

	if( ( node->flags & ISO_ISDIR ) )
	{
		kerndbg( KERN_DEBUG_LOW, "iso_close - this is a directory...\n" );
		result = iso_close_dir( volume, node, cookie );
		iso_free_dircookie( volume, node, cookie );
	}
	else
	{
		iso_free_cookie( volume, node, cookie );
	}

	kerndbg( KERN_DEBUG, "iso_close - EXIT (%d)\n", result );
	return result;
}

static int iso_free_cookie( void *ns, void *node, void *cookie )
{

	kerndbg( KERN_DEBUG, "iso_free_cookie - ENTER\n" );
	if( cookie != NULL )
	{
		kerndbg( KERN_DEBUG_LOW, "iso_free_cookie - aha! a directory, let's take away it's cookie...\n" );
		iso_free_dircookie( ns, node, cookie );
	}
	kerndbg( KERN_DEBUG, "iso_free_cookie - EXIT\n" );
	return 0;
}

// fs_access - checks permissions for access.
static int iso_access( void *volume, void *node, int mode )
{
	( void )volume;
	( void )node;
	( void )mode;

	kerndbg( KERN_DEBUG, "iso_access - ENTER\n" );
	kerndbg( KERN_DEBUG, "iso_access - EXIT\n" );

	return -EOK;
}

static int iso_read_link( void *_volume, void *_node, char *buffer, size_t bufferSize )
{
	int result = -EINVAL;
	vnode *node = ( vnode * )_node;
	size_t length = 0;

	kerndbg( KERN_DEBUG, "iso_readlink - ENTER\n" );

	if( S_ISLNK( node->attr.stat[FS_DATA_FORMAT].st_mode ) )
	{
		length = strlen( node->attr.slName );
		if( length > bufferSize )
		{
			// bufsize was length which seems to be a bug..
			kerndbg( KERN_WARNING, "iso_read_link - The provided buffer is smaller than the filename length!!!\n" );
			memcpy( buffer, node->attr.slName, bufferSize );
		}
		else
		{
			memcpy( buffer, node->attr.slName, length );
		}
		result = -EOK;
	}

	kerndbg( KERN_DEBUG, "iso_read_link: %s\n", buffer );

	if( result == -EOK )
		result = length;
	kerndbg( KERN_DEBUG, "iso_readlink - EXIT (%d)\n", result );
	return result;
}

static int iso_open_dir( void *_volume, void *_node, void **cookie )
{

//      nspace*                                 volume = (nspace*)_volume;
	vnode *node = ( vnode * )_node;
	int result = -EOK;
	dircookie *dirCookie = ( dircookie * )malloc( sizeof( dircookie ) );

	kerndbg( KERN_DEBUG, "iso_opendir - ENTER, node is %p\n", node );
	if( !( node->flags & ISO_ISDIR ) )
	{
		kerndbg( KERN_WARNING, "iso_opendir - What we have here is a file...\n" );
		return ( -EMFILE );
	}

	if( NULL != dirCookie )
	{
		kerndbg( KERN_DEBUG_LOW, "iso_opendir - filling in the dircookie...\n" );
		dirCookie->startBlock = node->startLBN[FS_DATA_FORMAT];
		dirCookie->block = node->startLBN[FS_DATA_FORMAT];
		dirCookie->totalSize = node->dataLen[FS_DATA_FORMAT];
		dirCookie->pos = 0;
		dirCookie->id = node->id;
		*cookie = ( void * )dirCookie;
	}
	else
	{
		kerndbg( KERN_WARNING, "iso_opendir - Could not locate memory for dircookie.\n" );
		result = -ENOMEM;
	}

	kerndbg( KERN_DEBUG, "iso_opendir - EXIT (%d)\n", result );
	return result;
}

static int iso_read_dir( void *_volume, void *_node, void *_cookie, int position, struct kernel_dirent *buf, size_t bufsize )
{
	int result = -EOK;

//      vnode*          node = (vnode*)_node;
	nspace *volume = ( nspace * ) _volume;
	dircookie *dirCookie = ( dircookie * )_cookie;

	vnode *parentNode = NULL;

	kerndbg( KERN_DEBUG, "iso_readdir - ENTER\n" );

	kerndbg( KERN_DEBUG_LOW, "iso_readdir - position = %d\n", position );

	// what should we do with the position argument

	if( dirCookie == NULL )
	{
		kerndbg( KERN_WARNING, "iso_readdir - Called with cookie == NULL!\n" );
		return -EINVAL;
	}

	result = ISOReadDirEnt( volume, dirCookie, buf, bufsize );

	// KV
	iso_read_vnode( volume, buf->d_ino, ( void ** )&parentNode );
	buf->d_ino = BLOCK_TO_INO( parentNode->startLBN[FS_DATA_FORMAT], 0 );

	// If we succeeded, return 1, the number of dirents we read
	if( result == -EOK )
	{
		result = 1;
	}
	else
	{
		result = 0;
	}
	
	iso_write_vnode( volume, parentNode );

	kerndbg( KERN_DEBUG, "iso_readdir - EXIT (%d)\n", result );
	return result;
}

static int iso_rewind_dir( void *_volume, void *_node, void *_cookie )
{
	int result = -EINVAL;
	dircookie *cookie = ( dircookie * )_cookie;

	kerndbg( KERN_DEBUG, "iso_rewinddir - ENTER\n" );

	if( cookie != NULL )
	{
		cookie->block = cookie->startBlock;
		cookie->pos = 0;
		result = -EOK;
	}
	else
	{
		kerndbg( KERN_WARNING, "iso_rewinddir - dircookie pointer is NULL\n" );
	}

	kerndbg( KERN_DEBUG, "iso_rewinddir - EXIT (%d)\n", result );
	return result;
}


static int iso_close_dir( void *_volume, void *_node, void *_cookie )
{
	kerndbg( KERN_DEBUG, "iso_closedir - ENTER\n" );
	kerndbg( KERN_DEBUG, "iso_closedir - EXIT\n" );
	return -EOK;
}

static int iso_free_dircookie( void *_volume, void *_node, void *_cookie )
{
	kerndbg( KERN_DEBUG, "iso_free_dircookie - ENTER\n" );

	if( _cookie != NULL )
	{
		free( _cookie );
	}
	kerndbg( KERN_DEBUG, "iso_free_dircookie - EXIT\n" );
	return 0;
}

int fs_init( const char *name, FSOperations_s ** ppsOps )
{
	kerndbg( KERN_DEBUG, "Initializing %s filesystem\n", gFSName );
	kerndbg( KERN_DEBUG_LOW, "name = %s\n", name );
	*ppsOps = &iso_operations;
	return ( api_version );
}
