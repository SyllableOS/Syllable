/*
**		Copyright 1999, Be Incorporated.   All Rights Reserved.
**		This file may be used under the terms of the Be Sample Code License.
**		
**		2003 - Ported to Atheos/Syllable by Ville Kallioniemi (ville.kallioniemi@abo.fi)
*/

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

#include "iso.h"


// Probe is called if you don't provide the fs type when you mount a volume
static int iso_probe( const char* devicePath, fs_info* filesystemInfo );

// Start of fundamental (read-only) required functions

static int		iso_mount(kdev_t nsid, const char *device, uint32 flags,
						void *parms, int len, void **data, ino_t *vnid);
static int		iso_unmount(void *_ns);
static int		iso_walk( void * ns, void * parentNode, const char * name,
						int name_len, ino_t * inode );
static int		iso_read_vnode(void *_ns, ino_t vnid, void **node);
static int		iso_write_vnode(void *_ns, void *_node );
static int		iso_read_stat(void *_ns, void *_node, struct stat *st);

static int		iso_open(void *_ns, void *_node, int omode, void **cookie);
static int		iso_close(void *ns, void *node, void *cookie);

static int		iso_read(void *_ns, void *_node, void *cookie, off_t pos,
						void *buf, size_t len);

static int		iso_free_cookie(void *ns, void *node, void *cookie);

static int		iso_access(void *_ns, void *_node, int mode);
static int		iso_open_dir(void* _ns, void* _node, void** cookie );
// fs_readdir - read 1 or more dirents, keep state in cookie, return
//					0 when no more entries.
static int		iso_read_dir(void *_ns, void *_node, void *cookie,
					int pos, struct kernel_dirent *buf, size_t bufsize);
static int		iso_rewind_dir(void *_ns, void *_node, void *cookie);
static int		iso_close_dir(void *_ns, void *_node, void *cookie);
static int		iso_free_dircookie(void *_ns, void *_node, void *cookie);
static int		iso_read_fs_stat(void *_ns, fs_info *);
static int 		iso_read_link(void *_ns, void *_node, char *buf, size_t bufsize);
// End of fundamental (read-only) required functions.

// write operations ( not implemented )
#if 0
static int		iso_remove_vnode(void *ns, void *node, char r);
static int		iso_secure_vnode(void *ns, void *node);
static int		iso_create(void *ns, void *dir, const char *name,
					int perms, int omode, vnode_id *vnid, void **cookie);
static int		iso_mkdir(void *ns, void *dir, const char *name, int perms);
static int		iso_unlink(void *ns, void *dir, const char *name);
static int		iso_rmdir(void *ns, void *dir, const char *name);
static int		iso_wstat(void *ns, void *node, struct stat *st, long mask);
static int		iso_write(void *ns, void *node, void *cookie, off_t pos,
						const void *buf, size_t *len);
static int		iso_ioctl(void *ns, void *node, void *cookie, int cmd,
						void *buf, size_t len);
static int		iso_wfsstat(void *ns, struct fs_info *);
static int		iso_sync(void *ns);
static int     	iso_initialize(const char *devname, void *parms, size_t len);
#endif 



// NOTE!!! The commented out functions do not exist in dosfs or afs
// Are they even implemented in the vfs? 
// dosfs: open and close handle both dirs and files + freeing the cookies
static FSOperations_s iso_operations =  {
	iso_probe,							// probe
	iso_mount,							// mount 
	iso_unmount,							// unmount 
	iso_read_vnode,						// read_vnode 
	iso_write_vnode,						// write_vnode 
	iso_walk, 							// locate_inode
	iso_access,							// access 
	NULL,								// create
	NULL,								// mkdir
	NULL,								// mknod
	NULL,								// symlink
	NULL,								// link
	NULL,								// rename
	NULL,								// unlink
	NULL,								// rmdir
	iso_read_link,						// readlink
	NULL, /*iso_opendir,*/						// opendir - is this used for syllable???
	NULL, /*iso_closedir,*/						// closedir  - is this used for syllable???
	iso_rewind_dir,						// rewinddir
	iso_read_dir,							// readdir 
	iso_open,							// open file
	iso_close,							// close file
	NULL, /*iso_free_cookie,*/						// free cookie 
	iso_read,							// read file 
	NULL, 								// write file 
	NULL, 								// readv
	NULL, 								// writev
	NULL,								// ioctl 
	NULL,								// setflags file 
	iso_read_stat,						// rstat 
	NULL, 								// wstat 
	NULL,								// fsync 
	NULL,								// initialize
	NULL,								// sync
	iso_read_fs_stat,							// rfsstat 
	NULL,								// wfsstat 
	NULL,								// isatty;
    NULL,								// add_select_req;
    NULL,								// rem_select_req;

    NULL,								// open_attrdir;
    NULL,								// close_attrdir;
    NULL,								// rewind_attrdir;
    NULL,								// read_attrdir;
    NULL,								// remove_attr;
    NULL,								// rename_attr;
    NULL,								// stat_attr;
    NULL,								// write_attr;
    NULL,								// read_attr;
    NULL,								// open_indexdir;
    NULL,								// close_indexdir;
    NULL,								// rewind_indexdir;
	NULL,								// read_indexdir;
    NULL,								// create_index;
	NULL,								// remove_index;
    NULL,								// rename_index;
    NULL,								// stat_index;
    NULL								// get_file_blocks;
};

int32			api_version = FSDRIVER_API_VERSION;
static char* 	gFSName = "ISO9660\0";

static int
iso_probe( const char* devicePath, fs_info* info )
{
	int device = -1;
	int error = -1;
	nspace * vol = 0;
	int i;
	dprintf( "%s: probe( %s , %p )\n", gFSName, devicePath, info );
	
	device = open( devicePath, O_RDONLY );
	if ( device < 0 )
	{
		printk( "iso_probe failed to open device %s\n", devicePath );
		return ( device );
	}

	error = ISOMount( devicePath, 0, &vol );
	if ( error < 0 )
	{
		printk( "iso_probe - failed to mount device %s\n -> not an iso volume", devicePath );
		close( device );
		return ( error );
	} 
	else 
	{
		dprintf( "iso_probe - mount suceeded: %s\n is an ISO volume", devicePath );
	}

	// Fill in device id.
	info->fi_dev = -1;

	// Root vnode ID
	info->fi_root = ISO_ROOTNODE_ID;
	
	// File system flags.
	info->fi_flags = FS_CAN_MOUNT;
	
	// FS block size.
	info->fi_block_size = vol->logicalBlkSize[FS_DATA_FORMAT];
	dprintf( "iso_probe set the logical block size to %d\n", info->fi_block_size );
	
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
	for (i=strlen(info->fi_volume_name)-1;i>=0;i--)
	{
		if ( info->fi_volume_name[i] != ' ' )
			break;
	}
	
	if ( i < 0 )
		strcpy( info->fi_volume_name, "UNKNOWN" );
	else
		info->fi_volume_name[i+1] = 0;
	
	// File system name
	strcpy( info->fi_driver_name, gFSName );
	
	// free the resources
	close( device );
	free( vol );
	
	return 0;
}



static int 
iso_mount( kdev_t volumeID, const char *devicePath, uint32 flags, void *parameters,
		int len, void **data, ino_t *rootNodeID)
{	
	int 		result = -EINVAL;
	nspace*		volume;
	
	dprintf("iso_mount - ENTER\n");	
	dprintf("%s: volumeID: %d device paht: %s\n",gFSName, volumeID, devicePath);
	
	//printk("iso_mount - Parameters(%d): %s\n", len, (char*)parameters );

	// flags is currently unused
	result = ISOMount( devicePath, flags, &volume );
	
	// If the mount succeeded, setup the block cache
	if ( result == -EOK )
	{
		int error = 0;
		// volumeSize is in blocks
		off_t volumeSize = volume->volSpaceSize[FS_DATA_FORMAT];

		// here we should parse the options and act accordingly
		volume->rockRidge = true;
		
		// root node is a special case and has always value 1
		*rootNodeID = ISO_ROOTNODE_ID;
		*data = (void*)volume;
		volume->id = volumeID;
		// initialize the cache
		dprintf("iso_mount - cache init: dev %d, max blocks %ld\n", volume->fd, volumeSize );
				
		// setup_device_cache expects the number of blocks on the device
		error = setup_device_cache(volume->fd, volume->id, volumeSize );
		if (  error < 0 )
		{
			dprintf( "iso_mount - failed to setup device cache.\n" );
			result = error;
			close( volume->fd );
			free( volume );
		}
	}	

	dprintf("iso_mount - EXIT, result code is %d\n", result );
	return result;
}

static int		
iso_unmount(void * _volume)
{
	int result = -EOK;
	
	nspace* volume = (nspace*)_volume;
	dprintf("iso_unmount - ENTER\n");
	dprintf("iso_unmount - flush the device cache\n");
	flush_device_cache( volume->fd, false );
	dprintf("iso_unmount - shutdown device cache\n" );
	shutdown_device_cache( volume->fd );
	dprintf("iso_unmount - closing volume\n");
	result = close( volume->fd );
	dprintf("iso_unmount - freeing volume data\n");
	free( volume );
	dprintf("iso_unmount - EXIT, result is %d\n", result );
	return result;
}

// fs_rfsstat - Fill in fs_info struct for device.
static int		
iso_read_fs_stat(void *_volume,  fs_info * fsInfo)
{
	nspace* volume = (nspace*)_volume;
	int i;
	
	dprintf("iso_read_fs_stat - ENTER\n");
	
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
	
	// Device name.
	strncpy(fsInfo->fi_device_path, volume->devicePath, sizeof(fsInfo->fi_device_path));

	strncpy(fsInfo->fi_volume_name, volume->volIDString, sizeof(fsInfo->fi_volume_name));
	for (i=strlen(fsInfo->fi_volume_name)-1;i>=0;i--)
	{
		if (fsInfo->fi_volume_name[i] != ' ')
			break;
	}

	if (i < 0)
		strcpy(fsInfo->fi_volume_name, "UNKNOWN");
	else
		fsInfo->fi_volume_name[i+1] = 0;
	
	// File system name
	strcpy( fsInfo->fi_driver_name, gFSName );
	
	dprintf("iso_read_fs_stat - EXIT\n");
	return 0;
}

static int	iso_walk( void * _volume, void * _baseNode, const char * _fileName
						, int _nameLength, ino_t * _vNodeID )
{
	nspace	*	volume = (nspace*)_volume;
	vnode	*	baseNode = (vnode*)_baseNode;
	size_t		nameLength = _nameLength;
	ino_t*	vNodeID = _vNodeID;
	vnode	*	newNode = NULL;
	char	*	directoryName = baseNode->fileIDString;
	char		fileName[nameLength+1];
	bool		done = false;
	int			result = -ENOENT;
	
	dprintf( "iso_walk - NEW - Enter\n" );
	
	dprintf( "iso_walk - _fileName = %s\n", _fileName );
	
	if ( volume == NULL || baseNode == NULL )
	{
		done = true;
		dprintf( "iso_walk - ERROR!\n" );
		dprintf( "iso_walk - volume = %p , baseNode = %p!!\n", volume, baseNode );	
	}

	if( !done && (nameLength > ISO_MAX_FILENAME_LENGTH) )
	{
		done = true;
		result = -ENAMETOOLONG;
		dprintf( "iso_walk - fileName length (%d) too long!\n", nameLength );
	}
	else
	{
		strncpy( fileName, _fileName, nameLength );
		fileName[nameLength] = '\0';
		dprintf( "iso_walk - In directory: %s\n", directoryName );
		if( fileName[nameLength] == '/' )
		{
			fileName[nameLength] = '\0';
			nameLength--;
			dprintf( "iso_walk - deleted trailing '/'\n" );
		}
		dprintf( "iso_walk - Looking for file: %s of length: %d \n", fileName, nameLength );
	}
	
	// handle "." ...
	if( !done && (strcmp( ".", fileName ) == 0) )
	{
		done = true;
		dprintf( "iso_walk - Success, found %s\n", fileName );
		*vNodeID = baseNode->id;
		result = -EOK;

	}
	//  and ".."
	if( !done && (strcmp( "..", fileName ) == 0) )
	{
		done = true;
		dprintf( "iso_walk - Success, found %s\n", fileName );
		*vNodeID = baseNode->parID;
		result = -EOK;
	}
	
	// ok, we'll have to actually walk the directory
	if( !done )
	{
		size_t		totalBytesRead = 0;
		size_t		directoryLength = baseNode->dataLen[FS_DATA_FORMAT];
		off_t	block = baseNode->startLBN[FS_DATA_FORMAT];
		size_t		blockSize = volume->logicalBlkSize[FS_DATA_FORMAT];
		
		dprintf( "iso_walk - Inside %s.\n", directoryName );	
		dprintf( "iso_walk - directory length: %d\n", directoryLength );
		dprintf( "iso_walk - starting at block #%d\n", (int)block );
		dprintf( "iso_walk - block size: %d\n", blockSize );
		
		// read in blocks until the file is found or we've reached the end of the directory
		while( !done && (totalBytesRead < directoryLength) )
		{
			bool	blockReleased = false;
			char	*blockData = get_cache_block( volume->fd, block, blockSize ); 
			
			dprintf( "iso_walk - read %d / %d of directory %s\n", totalBytesRead, directoryLength, directoryName ); 
			
			if ( blockData == NULL )
			{
				done = true;
				// prevent trying to release a block that we weren't able to read
				blockReleased = true;
				result = -ENOENT; // ??
			}
			else
			{
				off_t	blockBytesRead = 0;
				// move inside a block
				// ECMA:"Each Directory Record shall end in the Logical Sector in which it begins."
				// ECMA:"Unused byte positions after the last Directory Record in a Logical Sector
				//       shall be set to (00)."
				
				while ( !done && 
						(blockData[0] != 0) && 
						((totalBytesRead+blockBytesRead) < directoryLength) &&
						(blockBytesRead < blockSize)
					  )
				{ 
					size_t	nodeBytesRead = 0;
					vnode	node;
					char	nodeFileName[ISO_MAX_FILENAME_LENGTH];
					char	nodeSymbolicLinkName[ISO_MAX_FILENAME_LENGTH];
				
					node.fileIDString = nodeFileName;
					node.attr.slName = nodeSymbolicLinkName;
				
					result = InitNode( volume, &node, blockData, &nodeBytesRead );

					if ( result != EOK )
					{
						done = true;
						result = -ENOENT;
						dprintf( "iso_walk - couldn't initialize a node\n" );
					}
					else
					{
						dprintf( "iso_walk - InitVnode read %d bytes\n", nodeBytesRead );
						dprintf( "iso_walk - file searched: %s\n", fileName );
						dprintf( "iso_walk - filename length %d\n", nameLength );
						dprintf( "iso_walk - strlen( file searched ) = %d\n", strlen( fileName ) );
						dprintf( "iso_walk - strlen( node.fileIDString ) = %d\n", strlen( node.fileIDString ) );
						dprintf( "iso_walk - strncmp( node.fileIDString, fileName, strlen( fileName ) ) = %d \n", strncmp( node.fileIDString, fileName, strlen( fileName ) ) ); 
						
						if ( (strlen( node.fileIDString ) == strlen( fileName ) ) &&
							 (strncmp( node.fileIDString, fileName, strlen( fileName ) ) == 0)
						   )
						{
							done = true;
							release_cache_block( volume->fd, block );
							blockReleased = true;
							dprintf("iso_walk - success, found vnode at block %Ld, pos %Ld\n", block, blockBytesRead);
							//was: blockBytesRead & 0xFFFFFFFF
							*vNodeID = (( block << 30 ) + ( blockBytesRead & 0x3FFFFFFF ));
							dprintf( "iso_walk - New vnode id is %Ld\n", *vNodeID );
							result = -EOK;
						}
						else
						{
							dprintf( "iso_walk - Not the file we're looking for\n" );
							result = -ENOENT;
						}
					} // InitNode
					blockData 		+= nodeBytesRead;
					blockBytesRead 	+= nodeBytesRead;
					
					dprintf( "iso_walk - PROGRESS***************************\n" );
					dprintf( "iso_walk - read %Ld / %d of block # %Ld\n", blockBytesRead, blockSize, block );
					dprintf( "iso_walk - total %d  / %d of directory %s\n", totalBytesRead, directoryLength, directoryName );
					dprintf( "iso_walk - ***********************************\n" );
				} // while inside a block
				if ( blockReleased == false ) 
					release_cache_block( volume->fd, block );
				totalBytesRead 	+= blockSize;	
				block++;
			} // cacheBlock != NULL
		} // while inside a directory
		dprintf( "iso_walk - read a total of %d / %d\n", totalBytesRead, directoryLength );
	} // traversing the dir
	

	if (newNode != NULL)
	{ 
		if (S_ISLNK(newNode->attr.stat[FS_DATA_FORMAT].st_mode))
		{ 
			dprintf( "fs_walk - symbolic link file \'%s\' requested.\n", newNode->attr.slName ); 	
			// Should we do something here ??? FIXME!!!
		} 
	} 

	dprintf( "iso_walk - Done, return: %d.\n", result );
	return result;
}

static int		
iso_read_vnode(void *_ns, ino_t vnid, void **node)
{
	off_t 	block = 0;
	off_t	pos = 0;
	nspace*		ns = (nspace*)_ns;
	int			result = -EOK;
	vnode* 		newNode = (vnode*)calloc(sizeof(vnode), 1);
	
	
	pos = (vnid & 0x3FFFFFFF); 
	block = (vnid >> 30);
	
	dprintf("iso_read_vnode - ENTER\n" );
	dprintf("iso_read_vnode - block = %Ld, pos = %Ld, raw = %Ld node %p\n", block, pos, vnid, newNode);
	
	if (newNode != NULL)
	{
		// root node is stored in the volume struct
		if (vnid == ISO_ROOTNODE_ID)
		{
			dprintf("iso_read_vnode - root node requested.\n");
			memcpy(newNode, &(ns->rootDirRec), sizeof(vnode));
			*node = (void*)newNode;
		}
		else
		{
			// OK, it's something non-trivial
			// check that the position makes sense
			if (pos > ns->logicalBlkSize[FS_DATA_FORMAT]) 
			{
				dprintf("iso_read_vnode - position is outside of the block!\n" );
				result = -EINVAL;
		 	}
		 	else
		 	{
		 		off_t cachedBlock = block;
		 		char* blockData = NULL;
		 		newNode->fileIDString = (char*)calloc( ISO_MAX_FILENAME_LENGTH, 1 );
		 		newNode->attr.slName = (char*)calloc( ISO_MAX_FILENAME_LENGTH, 1 );
				blockData = (char*)get_cache_block(ns->fd, block, ns->logicalBlkSize[FS_DATA_FORMAT]);
		 		if (blockData != NULL)  
		 		{
					result = InitNode(ns, newNode, (blockData + pos), NULL);
					newNode->id = vnid;
					dprintf("iso_read_vnode - init result is %d\n", result );
					*node = (void*)newNode;
					dprintf("iso_read_vnode - new file %s, size %ld\n", newNode->fileIDString, newNode->dataLen[FS_DATA_FORMAT]);
					release_cache_block(ns->fd, cachedBlock);
				}
				else
				{
					free( newNode->fileIDString );
					free( newNode->attr.slName );
					free( newNode );
					dprintf( "iso_read_vnode - Could not read the block!\n" );
					return ( -EIO ); // is this the right error code ??? FIXME
				}
			}
		}
	}
	else
	{
		result = -ENOMEM;
	}
	
	dprintf( "iso_read_vnode - EXIT, result is %d\n", result );
	return result;
}


static int		
iso_write_vnode(void *_volume, void *_node )
{

	int 		result = -EOK;
	vnode* 		node = (vnode*)_node;
	
	// free the resources owned by the vnode
	dprintf("iso_write_vnode - ENTER \n" );

	if ( node != NULL )
	{ 
		if ( node->id != ISO_ROOTNODE_ID )
		{
			if (node->fileIDString != NULL) free (node->fileIDString);
			if (node->attr.slName != NULL) free (node->attr.slName);
			free(node);
			node = NULL;
		}
	}
	else
	{
		dprintf( "iso_write_vnode - called with node = NULL!!!\n" );
	}
	dprintf("iso_write_vnode - EXIT\n");
	return result;
}


static int		
iso_read_stat(void *_volume, void *_node, struct stat *st)
{
	nspace* volume = (nspace*)_volume;
	vnode*	node = (vnode*)_node;
	int 	result = -EOK;
	time_t	time;
	
	dprintf("iso_rstat - ENTER\n");
	st->st_dev = volume->id;
	st->st_ino = node->id;
	st->st_nlink = node->attr.stat[FS_DATA_FORMAT].st_nlink;
	st->st_uid = node->attr.stat[FS_DATA_FORMAT].st_uid;
	st->st_gid = node->attr.stat[FS_DATA_FORMAT].st_gid;
	st->st_blksize = 65536;
	st->st_mode = node->attr.stat[FS_DATA_FORMAT].st_mode;
	// Same for file/dir in ISO9660
	st->st_size = node->dataLen[FS_DATA_FORMAT];
	if (ConvertRecDate(&(node->recordDate), &time) == -EOK) 
		st->st_ctime = st->st_mtime = st->st_atime = time;
	dprintf("iso_rstat - EXIT, result is %d\n", result );
	
	return result;
}


static int		
iso_read(void *_ns, void *_node, void *cookie, off_t pos, void *buf, 
			size_t len)
{
	nspace* 	ns = (nspace*)_ns;
	vnode* 		node = (vnode*)_node;
	uint16		blockSize = ns->logicalBlkSize[FS_DATA_FORMAT];
	uint32	 	startBlock = node->startLBN[FS_DATA_FORMAT]  + 
							(pos / blockSize);
	off_t	blockPos = pos %blockSize;
	off_t 	numBlocks = 0;
	uint32		dataLen = node->dataLen[FS_DATA_FORMAT];
	int			result = -EOK;
	size_t		endLen = 0;
	size_t		reqLen = len;
	size_t		startLen =  0;
	size_t		bytesRead = 0;

	dprintf( "iso_read - ENTER\n" );
	if ( (node->flags & ISO_ISDIR) )
	{
		dprintf( "iso_read - tried to read  a directory\n" );		
		return ( -EISDIR );
	}

	if (pos < 0)
		pos = 0;

	// If passed-in requested length is bigger than file size, change it to
	// file size.
	if ( (reqLen + pos) > dataLen )
	{
		reqLen = dataLen - pos;
	}
	// Compute the length of the partial start-block read, if any.
	
	if ( (blockPos + reqLen) <= blockSize )
	{
		startLen = reqLen;
	}
	else if (blockPos > 0)
	{
			startLen = blockSize - blockPos;
	}

	if (blockPos == 0 && reqLen >= blockSize)
	{
		dprintf("Setting startLen to 0, even block read\n");
		startLen = 0;
	}
	// Compute the length of the partial end-block read, if any.
	if (reqLen + blockPos > blockSize)
	{
		endLen = (reqLen + blockPos) % blockSize;
	}

	// Compute the number of middle blocks to read.
	numBlocks = ((reqLen - endLen - startLen) /  blockSize);
	
	
	dprintf("iso_read - ENTER, pos is %Ld, reqLen is %d\n", pos, reqLen);
	dprintf("iso_read - filename is %s\n", node->fileIDString);
	dprintf("iso_read - total file length is %lu\n", dataLen);
	dprintf("iso_read - start block of file is %lu\n", node->startLBN[FS_DATA_FORMAT]);
	dprintf("iso_read - block pos is %Lu\n", blockPos);
	dprintf("iso_read - read block will be %lu\n", startBlock);
	dprintf("iso_read - startLen is %d\n", startLen);
	dprintf("iso_read - endLen is %d\n", endLen);
	dprintf("iso_read - num blocks to read is %Ld\n", numBlocks);
	
	// If pos >= file length, return length of 0.
	if (pos >= dataLen)
	{
		dprintf("iso_read - End of file reached\n");
	}
	else
	{
		// Read in the first, potentially partial, block.
		if (startLen > 0)
		{
			off_t	cachedBlock = startBlock;
			char*		blockData = (char*)get_cache_block(ns->fd, startBlock, blockSize);
			dprintf("iso_read - getting block %lu\n", startBlock);
			if (blockData != NULL)
			{
				dprintf("iso_read - copying first block, len is %d.\n", startLen);
				memcpy(buf, blockData+blockPos, startLen);
				bytesRead += startLen;
				release_cache_block(ns->fd, cachedBlock);
				startBlock++;	
			}
			else result = -EIO;
		}

		// Read in the middle blocks. (Should read to a buffer and copied to uspace?)
		if (numBlocks > 0 && result == -EOK)
		{
			dprintf("iso_read - getting middle blocks\n");
			result = cached_read(ns->fd, startBlock, 
						((char*)buf) + startLen, 
						numBlocks, 
						blockSize);
			if (result == -EOK)
			{
				bytesRead += blockSize * numBlocks;
			}
		}

		// Read in the last partial block.
		if (result == -EOK && endLen > 0)
		{
			off_t	endBlock = startBlock + numBlocks;
			char*		endBlockData = (char*)get_cache_block(ns->fd, endBlock, blockSize);
			dprintf("iso_read - getting end block\n");
			if (endBlockData != NULL)
			{
				char* endBuf = ((char*)buf) + (reqLen - endLen);
				
				memcpy(endBuf, endBlockData, endLen);
				release_cache_block(ns->fd, endBlock);
				bytesRead += endLen;
			}
			else result = -EIO;
		}
	}
	dprintf("iso_read - read %d bytes\n", bytesRead );
	dprintf("iso_read - EXIT, result is %d\n", result );
	if ( result == -EOK )
		result = bytesRead; 
	return result;
}

static int		
iso_open(void *_volume, void *_node, int omode, void **cookie)
{
	int		result = -EOK;
	vnode	*node = (vnode*) _node;
	
	dprintf("iso_open - ENTER\n");

	if ( _volume == NULL )
		dprintf( "iso_open - called with volume == NULL\n" );
	
	//afs returns -EISDIR when trying to open a dir, but doesn't have a opendir call...
	//FIXME!!!
	//dosfs handles both dirs and files here as in the close too..

	// Do not allow any of the write-like open modes to get by 
	if ((omode == O_WRONLY) || (omode == O_RDWR)) 
		result = -EROFS; 
	else if((omode & O_TRUNC) || (omode & O_CREAT)) 
		result = -EROFS; 


	if ((node->flags & ISO_ISDIR))
	{
		dprintf( "iso_open - tried to open a directory\n" );
		dprintf( "iso_open - call iso_opendir\n" );
		result = iso_open_dir( _volume, _node, cookie );
	}

	// No cookie info needed for files, just return.
	dprintf("iso_open - EXIT\n");

	return result;
}

static int		
iso_close(void *_volume, void *_node, void *cookie)
{
	int result = -EOK;
	vnode * node = (vnode*)_node;
	nspace* volume = (nspace*)_volume;
	
	dprintf("iso_close - ENTER\n");
	if ( (node->flags & ISO_ISDIR) )
	{
		dprintf( "iso_close - this is a directory...\n" );
		result = iso_close_dir( volume, node, cookie );
		iso_free_dircookie( volume, node, cookie );
	}
	else
	{
		iso_free_cookie( volume, node, cookie );
	}

	dprintf("iso_close - EXIT\n");
	return result;
}

static int		
iso_free_cookie(void *ns, void *node, void *cookie)
{

	dprintf("iso_free_cookie - ENTER\n");
	if (cookie != NULL) 
	{
		dprintf("iso_free_cookie - aha! a directory, let's take away it's cookie...\n" );
		iso_free_dircookie( ns, node, cookie );
	}
	dprintf("iso_free_cookie - EXIT\n");
	return 0;
}

// fs_access - checks permissions for access.
static int		
iso_access(void *volume, void *node, int mode)
{
	(void)volume;
	(void)node;
	(void)mode;
	
	dprintf("iso_access - ENTER\n");
	dprintf("iso_access - EXIT\n");

	return -EOK;
}

static int
iso_read_link(void *_volume, void *_node, char *buffer, size_t bufferSize)
{
	int		result = -EINVAL;
	vnode*	node = (vnode*)_node;
	size_t	length = 0;
	dprintf( "iso_readlink - ENTER\n" );	

	if (S_ISLNK(node->attr.stat[FS_DATA_FORMAT].st_mode))
	{
		length = strlen(node->attr.slName);
		if (length > bufferSize)
		{
			// bufsize was length which seems to be a bug..
			dprintf( "iso_read_link - The provided buffer is smaller than the filename length!!!\n" );
			memcpy(buffer, node->attr.slName, bufferSize);
		}
		else
		{
			memcpy(buffer, node->attr.slName, length);
		}
		result = -EOK;
	}
	
	if ( result == -EOK )
		result = length;
	dprintf( "iso_readlink - EXIT (%d)\n", result );	
	return result;
}

static int		
iso_open_dir(void *_volume, void *_node, void **cookie)
{

//	nspace* 				volume = (nspace*)_volume;
	vnode*					node = (vnode*)_node;
	int						result = -EOK;
	dircookie* 				dirCookie = (dircookie*)malloc(sizeof(dircookie));

	dprintf("iso_opendir - ENTER, node is %p\n", node); 
	if (!(node->flags & ISO_ISDIR))
	{
		dprintf("iso_opendir - What we have here is a file...\n" );
		return( -EMFILE );
	}
	
	if ( NULL != dirCookie )
	{
		dprintf("iso_opendir - filling in the dircookie...\n" );
		dirCookie->startBlock = node->startLBN[FS_DATA_FORMAT];
		dirCookie->block = node->startLBN[FS_DATA_FORMAT];
		dirCookie->totalSize = node->dataLen[FS_DATA_FORMAT];
		dirCookie->pos = 0;
		dirCookie->id = node->id;
		*cookie = (void*)dirCookie;
	}
	else
	{
		dprintf( "iso_opendir - Could not locate memory for dircookie.\n" );
		result = -ENOMEM;
	}
	
	dprintf("iso_opendir - EXIT\n");
	return result;
}

static int		
iso_read_dir(void *_volume, void *_node, void *_cookie, int position, struct kernel_dirent *buf, size_t bufsize )
{
	int 		result = -EOK;
//	vnode* 		node = (vnode*)_node;
	nspace* 	volume = (nspace*)_volume;
	dircookie* 	dirCookie = (dircookie*)_cookie;
	
	dprintf("iso_readdir - ENTER\n");
	
	dprintf("iso_readdir - position = %d\n", position );
	
	// what should we do with the position argument
	
	if ( dirCookie == NULL )
	{
		dprintf( "iso_readdir - Called with cookie == NULL!\n" );
		return -EINVAL;
	}
	
	result = ISOReadDirEnt( volume, dirCookie, buf, bufsize);
	
	// If we succeeded, return 1, the number of dirents we read
	if (result == -EOK)
	{
		result = 1;
	}
	else 
	{
		result = 0;
	}
	
	dprintf("iso_readdir - EXIT, result is %d\n", result);
	return result;
}

static int		
iso_rewind_dir(void *_volume, void *_node, void* _cookie)
{
	int			result = -EINVAL;
	dircookie*	cookie = (dircookie*)_cookie;
	dprintf("iso_rewinddir - ENTER\n");
	
	if (cookie != NULL)
	{
		cookie->block = cookie->startBlock;
		cookie->pos = 0;
		result = -EOK;
	} 
	else
	{
		dprintf( "iso_rewinddir - dircookie pointer is NULL\n" );
	}
	
	dprintf( "iso_rewinddir - EXIT, result is %d\n", result );
	return result;
}


static int		
iso_close_dir(void *_volume, void *_node, void *_cookie)
{
	dprintf( "iso_closedir - ENTER\n" );
	dprintf( "iso_closedir - EXIT\n" );
	return -EOK;
}

static int		
iso_free_dircookie(void *_volume, void *_node, void *_cookie)
{
	dprintf("iso_free_dircookie - ENTER\n");
	
	if ( _cookie != NULL )
	{
		free(_cookie);
	}
	dprintf("iso_free_dircookie - EXIT\n");
	return 0;
}

int fs_init( const char* name, FSOperations_s** ppsOps )
{
	dprintf( "Initializing %s filesystem\n", gFSName );
	dprintf( "name = %s\n", name );
	*ppsOps = &iso_operations;
	return( api_version );
}








