/* kernel_interface - file system interface to BeOS' vnode layer
 *
 * Copyright 2001-2005, Axel Dörfler, axeld@pinc-software.de.
 * This file may be used under the terms of the MIT License.
 */


#include "Utility.h"
#include "Debug.h"
#include "Volume.h"
#include "Inode.h"
#include "Index.h"
#include "BPlusTree.h"
#include "Query.h"
#include "bfs_control.h"

#include <atheos/string.h>
#include <atheos/time.h>
#include <atheos/device.h>

#define BFS_IO_SIZE	65536

#include "kernel_cpp.h"


double
strtod(const char */*start*/, char **/*end*/)
{
	return 0;
}


//	#pragma mark -


static status_t
bfs_probe(const char *device, fs_info* psInfo )
{
	disk_super_block superBlock;
	int dev = open( device, O_RDONLY );
	block_run root;
	if( dev < 0 )
	{
		printk( "Error: bfs_probe() failed to open device %s\n", device );
		return ( dev );
	}
	
	status_t status = Volume::Identify(dev, &superBlock);
	if (status != B_OK)
	{
		printk( "Not a bfs volume\n" );
		close( dev );
		return status;
	}
	root = superBlock.root_dir;
	
	
	psInfo->fi_dev = -1;
	psInfo->fi_root = (((off_t)root.AllocationGroup()) << superBlock.AllocationGroupShift()) | (off_t)root.Start();
	psInfo->fi_flags = FS_IS_PERSISTENT | FS_IS_BLOCKBASED | FS_CAN_MOUNT | FS_HAS_MIME | FS_HAS_ATTR;
	psInfo->fi_block_size = superBlock.block_size;
	psInfo->fi_io_size = BFS_IO_SIZE;
	psInfo->fi_total_blocks = superBlock.num_blocks;
	psInfo->fi_free_blocks = superBlock.num_blocks - superBlock.used_blocks;
	psInfo->fi_free_user_blocks = superBlock.num_blocks - superBlock.used_blocks;
	psInfo->fi_total_inodes = -1;
	psInfo->fi_free_inodes = -1;
	strncpy( psInfo->fi_volume_name, superBlock.name, BFS_DISK_NAME_LENGTH );
	psInfo->fi_volume_name[255] = '\0';
	
	close( dev );
	return B_OK;
}

static status_t
bfs_mount(kdev_t mountID, const char *device, uint32 flags, const void *args, int nArgLen,
	void **_data, ino_t *_rootID)
{
	FUNCTION();

	Volume *volume = new Volume(mountID);
	if (volume == NULL)
		return B_NO_MEMORY;

	status_t status;
	if ((status = volume->Mount(device, flags)) == B_OK) {
		*_data = volume;
		*_rootID = volume->ToVnode(volume->Root());
		printk("mounted \"%s\" (root node at %Ld, device = %s)\n",
			volume->Name(), *_rootID, device);
	}
	else
		delete volume;

	RETURN_ERROR(status);
}


static status_t
bfs_unmount(void *ns)
{
	FUNCTION();
	Volume* volume = (Volume *)ns;

	status_t status = volume->Unmount();
	delete volume;

	RETURN_ERROR(status);
}


/**	Fill in bfs_info struct for device.
 */

static status_t
bfs_read_fs_stat(void *_ns, fs_info *psInfo)
{
	FUNCTION();
	if (_ns == NULL || psInfo == NULL)
		return B_BAD_VALUE;

	Volume *volume = (Volume *)_ns;

	RecursiveLocker locker(volume->Lock());
	
	psInfo->fi_dev = volume->Device();
	psInfo->fi_root = volume->ToVnode(volume->Root());
	psInfo->fi_flags = FS_IS_PERSISTENT | FS_IS_BLOCKBASED | FS_CAN_MOUNT | FS_HAS_MIME | FS_HAS_ATTR;
	psInfo->fi_block_size = volume->BlockSize();
	psInfo->fi_io_size = BFS_IO_SIZE;
	psInfo->fi_total_blocks = volume->NumBlocks();
	psInfo->fi_free_blocks = volume->FreeBlocks();
	psInfo->fi_free_user_blocks = volume->FreeBlocks();
	psInfo->fi_total_inodes = -1;
	psInfo->fi_free_inodes = -1;
	strncpy( psInfo->fi_volume_name, volume->Name(), BFS_DISK_NAME_LENGTH );
	psInfo->fi_volume_name[255] = '\0';

	return B_OK;
}


static status_t
bfs_write_fs_stat(void *_ns, const fs_info *info, uint32 mask)
{
	FUNCTION_START(("mask = %ld\n", mask));

	Volume *volume = (Volume *)_ns;
	disk_super_block &superBlock = volume->SuperBlock();

	RecursiveLocker locker(volume->Lock());

	status_t status = B_BAD_VALUE;

	if (mask & WFSSTAT_NAME) {
		strncpy(superBlock.name, info->fi_volume_name, sizeof(superBlock.name) - 1);
		superBlock.name[sizeof(superBlock.name) - 1] = '\0';

		status = volume->WriteSuperBlock();
	}
	return status;
}

static status_t
bfs_initialize(const char* pzDevPath, const char* pzVolName, void* pArgs, size_t nArgLen )
{
	status_t error = B_OK;
	FUNCTION_START(("deviceName = %s, parameter len = %ld\n", pzDevPath, nArgLen));
	
	Volume *volume = new Volume(0xffff);
	if (volume == NULL)
		return B_NO_MEMORY;

	status_t status;
	if ((status = volume->Initialize(pzDevPath, pzVolName, 2048, 0)) == B_OK) {
		printk("formated \"%s\" (device = %s)\n",
			volume->Name(), pzDevPath);
	}
	else
		delete volume;

	RETURN_ERROR(status);

	return error;
}

static status_t
bfs_sync(void *_ns)
{
	FUNCTION();
	if (_ns == NULL)
		return B_BAD_VALUE;

	Volume *volume = (Volume *)_ns;

	return volume->Sync();
}


//	#pragma mark -


/**	Reads in the node from disk and creates an inode object from it.
 */

static status_t
bfs_read_vnode(void *_ns, ino_t id, void **_node)
{
	FUNCTION_START(("vnode_id = %Ld\n", id));
	Volume *volume = (Volume *)_ns;
	
	if( volume == NULL )
	{
		
		return B_ERROR;
	}

	if (id < 0 || id > volume->NumBlocks()) {
		FATAL(("inode at %Ld requested!\n", id));
		return B_ERROR;
	}
	
	
	CachedBlock cached(volume, id);
	
	
	bfs_inode *node = (bfs_inode *)cached.Block();
	Inode *inode = NULL;
	int32 tries = 0;

restartIfBusy:
	status_t status = node->InitCheck(volume);
	
	
	if (status == B_BUSY) {
		
		inode = (Inode *)node->etc;
			// We have to use the "etc" field to get the inode object
			// (the inode is currently being constructed)
			// We would need to call something like get_vnode() again here
			// to get rid of the "etc" field - which would be nice, especially
			// for other file systems which don't have this messy field.

		// let us wait a bit and try again later
		if (tries++ < 200) {
			// wait for one second at maximum
			snooze(5000);
			goto restartIfBusy;
		}
		FATAL(("inode is not becoming unbusy (id = %Ld)\n", id));
		return status;
	} else if (status < B_OK) {
		FATAL(("inode at %Ld is corrupt!\n", id));
//		dump_inode( node );
		return status;
	}
	
	
	// If the inode is currently being constructed, we already have an inode
	// pointer (taken from the node's etc field).
	// If not, we create a new one here

	if (inode == NULL) {
		
		inode = new Inode(&cached);
		if (inode == NULL)
			return B_NO_MEMORY;
			
		
		status = inode->InitCheck(false);
		if (status < B_OK)
			delete inode;
	} else
		status = inode->InitCheck(false);

	if (status == B_OK)
		*_node = inode;

	return status;
}

static status_t
bfs_remove_vnode(void *_ns, void *_node);

static status_t
bfs_release_vnode(void *_ns, void *_node)
{
	
	Inode *inode = (Inode *)_node;
	FUNCTION_START(("node = %p %Ld\n", _node, inode->ID()));
	
	if( inode->Node()->flags & HOST_ENDIAN_TO_BFS_INT32(INODE_DELETED) )
	{
		bfs_remove_vnode( _ns, _node );
	} else
		delete inode;

	return B_NO_ERROR;
}


static status_t
bfs_remove_vnode(void *_ns, void *_node)
{
	FUNCTION();

	if (_ns == NULL || _node == NULL)
		return B_BAD_VALUE;

	Volume *volume = (Volume *)_ns;
	Inode *inode = (Inode *)_node;

	// The "chkbfs" functionality uses this flag to prevent the space used
	// up by the inode from being freed - this flag is set only in situations
	// where this is a good idea... (the block bitmap will get fixed anyway
	// in this case).
	if (inode->Flags() & INODE_DONT_FREE_SPACE) {
		delete inode;
		return B_OK;
	}

	// If the inode isn't in use anymore, we were called before
	// bfs_unlink() returns - in this case, we can just use the
	// transaction which has already deleted the inode.
	Transaction localTransaction, *transaction = NULL;

	Journal *journal = volume->GetJournal(volume->ToBlock(inode->Parent()));
	if (journal != NULL)
		transaction = journal->CurrentTransaction();

	if (transaction == NULL) {
		transaction = &localTransaction;
		localTransaction.Start(volume, inode->BlockNumber());
	}

	status_t status = inode->Free(transaction);
	if (status == B_OK) {
		if (transaction == &localTransaction)
			localTransaction.Done();

		delete inode;
	}

	return status;
}
#ifndef __SYLLABLE__

static bool
bfs_can_page(fs_volume _fs, fs_vnode _v, fs_cookie _cookie)
{
	// ToDo: we're obviously not even asked...
	return false;
}


static status_t
bfs_read_pages(fs_volume _fs, fs_vnode _node, fs_cookie _cookie, off_t pos,
	const iovec *vecs, size_t count, size_t *_numBytes)
{
	Inode *inode = (Inode *)_node;

	if (!inode->HasUserAccessableStream())
		RETURN_ERROR(B_BAD_VALUE);

	ReadLocked locked(inode->Lock());
	return file_cache_read_pages(inode->FileCache(), pos, vecs, count, _numBytes);
#if 0
	for (uint32 i = 0; i < count; i++) {
		if (pos >= inode->Size()) {
			memset(vecs[i].iov_base, 0, vecs[i].iov_len);
			pos += vecs[i].iov_len;
			*_numBytes -= vecs[i].iov_len;
		} else {
			uint32 length = vecs[i].iov_len;
			if (length > inode->Size() - pos)
				length = inode->Size() - pos;

			inode->ReadAt(pos, (uint8 *)vecs[i].iov_base, &length);

			if (length < vecs[i].iov_len) {
				memset((char *)vecs[i].iov_base + length, 0, vecs[i].iov_len - length);
				*_numBytes -= vecs[i].iov_len - length;
			}

			pos += vecs[i].iov_len;
		}
	}

	return B_OK;
#endif
}


static status_t
bfs_write_pages(fs_volume _fs, fs_vnode _node, fs_cookie _cookie, off_t pos,
	const iovec *vecs, size_t count, size_t *_numBytes)
{
	Inode *inode = (Inode *)_node;

	if (!inode->HasUserAccessableStream())
		RETURN_ERROR(B_BAD_VALUE);

	ReadLocked locked(inode->Lock());
	return file_cache_write_pages(inode->FileCache(), pos, vecs, count, _numBytes);
}


static status_t
bfs_get_file_map(fs_volume _fs, fs_vnode _node, off_t offset, size_t size,
	struct file_io_vec *vecs, size_t *_count)
{
	Volume *volume = (Volume *)_fs;
	Inode *inode = (Inode *)_node;

	int32 blockShift = volume->BlockShift();
	size_t index = 0, max = *_count;
	block_run run;
	off_t fileOffset;

	//FUNCTION_START(("offset = %Ld, size = %lu\n", offset, size));

	while (true) {
		status_t status = inode->FindBlockRun(offset, run, fileOffset);
		if (status != B_OK)
			return status;

		vecs[index].offset = volume->ToOffset(run) + offset - fileOffset;
		vecs[index].length = (run.Length() << blockShift) - offset + fileOffset;

		offset += vecs[index].length;

		// are we already done?
		if (size <= vecs[index].length
			|| offset >= inode->Size()) {
			*_count = index + 1;
			return B_OK;
		}

		size -= vecs[index].length;
		index++;

		if (index >= max) {
			// we're out of file_io_vecs; let's bail out
			*_count = index;
			return B_BUFFER_OVERFLOW;
		}
	}

	// can never get here
	return B_ERROR;
}
#endif

//	#pragma mark -


/**	the walk function just "walks" through a directory looking for the
 *	specified file. It calls get_vnode() on its vnode-id to init it
 *	for the kernel.
 */

static status_t
bfs_lookup(void *_ns, void *_directory, const char *file, int namlen, ino_t *_vnodeID)
{
	FUNCTION_START(("file = %s %i\n", file, namlen));
	if (_ns == NULL || _directory == NULL || file == NULL || _vnodeID == NULL)
		return B_BAD_VALUE;

	Volume *volume = (Volume *)_ns;
	Inode *directory = (Inode *)_directory;

	// check access permissions
	status_t status = directory->CheckPermissions(R_OK);
	if (status < B_OK)
		RETURN_ERROR(status);

	BPlusTree *tree;
	if (directory->GetTree(&tree) != B_OK)
		RETURN_ERROR(B_BAD_VALUE);

	if ((status = tree->Find((uint8 *)file, namlen, _vnodeID)) < B_OK) {
		PRINT(("bfs_walk() could not find %Ld:\"%s\": %s\n", directory->BlockNumber(), file, strerror(status)));
		return status;
	}
	
	RecursiveLocker locker(volume->Lock());
		// we have to hold the volume lock in order to not
		// interfere with new_vnode() here

	Inode *inode;
	if ((status = get_vnode(volume->ID(), *_vnodeID, (void **)&inode)) != B_OK) {
		REPORT_ERROR(status);
		return B_ENTRY_NOT_FOUND;
	}
	
	//*_type = inode->Mode();
	put_vnode( volume->ID(), *_vnodeID );

	return B_OK;
}

#ifndef __SYLLABLE__
static status_t
bfs_get_vnode_name(fs_volume _fs, fs_vnode _node, char *buffer, size_t bufferSize)
{
	Inode *inode = (Inode *)_node;

	return inode->GetName(buffer, bufferSize);
}


static status_t
bfs_ioctl(void *_fs, void *_node, void *_cookie, ulong cmd, void *buffer, size_t bufferLength)
{
	FUNCTION_START(("node = %p, cmd = %lu, buf = %p, len = %ld\n", _node, cmd, buffer, bufferLength));

	Volume *volume = (Volume *)_fs;
	Inode *inode = (Inode *)_node;

	switch (cmd) {
		case BFS_IOCTL_VERSION:
		{
			uint32 *version = (uint32 *)buffer;

			*version = 0x10000;
			return B_OK;
		}
		case BFS_IOCTL_START_CHECKING:
		{
			// start checking
			BlockAllocator &allocator = volume->Allocator();
			check_control *control = (check_control *)buffer;

			status_t status = allocator.StartChecking(control);
			if (status == B_OK && inode != NULL)
				inode->Node().flags |= HOST_ENDIAN_TO_BFS_INT32(INODE_CHKBFS_RUNNING);

			return status;
		}
		case BFS_IOCTL_STOP_CHECKING:
		{
			// stop checking
			BlockAllocator &allocator = volume->Allocator();
			check_control *control = (check_control *)buffer;

			status_t status = allocator.StopChecking(control);
			if (status == B_OK && inode != NULL)
				inode->Node().flags &= HOST_ENDIAN_TO_BFS_INT32(~INODE_CHKBFS_RUNNING);

			return status;
		}
		case BFS_IOCTL_CHECK_NEXT_NODE:
		{
			// check next
			BlockAllocator &allocator = volume->Allocator();
			check_control *control = (check_control *)buffer;

			return allocator.CheckNextNode(control);
		}
#ifdef DEBUG
		case 56742:
		{
			// allocate all free blocks and zero them out (a test for the BlockAllocator)!
			BlockAllocator &allocator = volume->Allocator();
			Transaction transaction(volume, 0);
			CachedBlock cached(volume);
			block_run run;
			while (allocator.AllocateBlocks(transaction, 8, 0, 64, 1, run) == B_OK) {
				PRINT(("write block_run(%ld, %d, %d)\n", run.allocation_group, run.start, run.length));
				for (int32 i = 0;i < run.length;i++) {
					uint8 *block = cached.SetToWritable(transaction, run);
					if (block != NULL)
						memset(block, 0, volume->BlockSize());
				}
			}
			return B_OK;
		}
		case 56743:
			dump_super_block(&volume->SuperBlock());
			return B_OK;
		case 56744:
			if (inode != NULL)
				dump_inode(&inode->Node());
			return B_OK;
		case 56745:
			if (inode != NULL) {
				NodeGetter node(volume, inode);
				dump_block((const char *)node.Node(), volume->BlockSize());
			}
			return B_OK;
#endif
	}
	return B_BAD_VALUE;
}
#endif

/** Sets the open-mode flags for the open file cookie - only
 *	supports O_APPEND currently, but that should be sufficient
 *	for a file system.
 */

static status_t
bfs_set_flags(void *_ns, void *_node, void *_cookie, int flags)
{
	FUNCTION_START(("node = %p, flags = %d", _node, flags));

	file_cookie *cookie = (file_cookie *)_cookie;
	cookie->open_mode = (cookie->open_mode & ~O_APPEND) | (flags & O_APPEND);

	return B_OK;
}


#if 0
static status_t
bfs_select(void *ns, void *node, void *cookie, uint8 event, uint32 ref, selectsync *sync)
{
	FUNCTION_START(("event = %d, ref = %lu, sync = %p\n", event, ref, sync));
	notify_select_event(sync, ref);

	return B_OK;
}


static status_t
bfs_deselect(void *ns, void *node, void *cookie, uint8 event, selectsync *sync)
{
	FUNCTION();
	return B_OK;
}
#endif

static status_t
bfs_fsync(void *_ns, void *_node)
{
	FUNCTION();
	if (_node == NULL)
		return B_BAD_VALUE;

	Inode *inode = (Inode *)_node;
	return inode->Sync();
}


/**	Fills in the stat struct for a node
 */

static status_t
bfs_read_stat(void *_ns, void *_node, struct stat *st)
{
	FUNCTION();

	Volume *volume = (Volume *)_ns;
	Inode *inode = (Inode *)_node;
	bfs_inode *node = inode->Node();

	st->st_dev = volume->ID();
	st->st_ino = inode->ID();
	st->st_nlink = 1;
	st->st_blksize = BFS_IO_SIZE;

	st->st_uid = node->UserID();
	st->st_gid = node->GroupID();
	st->st_mode = node->Mode();
	st->st_size = node->data.Size();

	st->st_atime = get_real_time() / 1000000;
	st->st_mtime = st->st_ctime = (time_t)(node->LastModifiedTime() >> INODE_TIME_SHIFT);
	st->st_ctime = (time_t)(node->CreateTime() >> INODE_TIME_SHIFT);
	return B_OK;
}


static status_t
bfs_write_stat(void *_ns, void *_node, const struct stat *stat, uint32 mask)
{
	FUNCTION();

	if (_ns == NULL || _node == NULL || stat == NULL)
		RETURN_ERROR(B_BAD_VALUE);

	Volume *volume = (Volume *)_ns;
	Inode *inode = (Inode *)_node;
	
	
	// that may be incorrect here - I don't think we need write access to
	// change most of the stat...
	// we should definitely check a bit more if the new stats are correct and valid...
	
	status_t status = inode->CheckPermissions(W_OK);
	if (status < B_OK)
		RETURN_ERROR(status);

#ifdef UNSAFE_GET_VNODE
	RecursiveLocker locker(volume->Lock());
#endif

	WriteLocked locked(inode->Lock());
	if (locked.IsLocked() < B_OK)
		RETURN_ERROR(B_ERROR);

	Transaction transaction(volume, inode->BlockNumber());

	bfs_inode *node = inode->Node();

	if (mask & WSTAT_SIZE) {
		// Since WSTAT_SIZE is the only thing that can fail directly, we
		// do it first, so that the inode state will still be consistent
		// with the on-disk version
		if (inode->IsDirectory())
			return B_IS_A_DIRECTORY;

		if (inode->Size() != stat->st_size) {
			status = inode->SetFileSize(&transaction, stat->st_size);
			if (status < B_OK)
				return status;

			// fill the new blocks (if any) with zeros
			inode->FillGapWithZeros(inode->OldSize(), inode->Size());

			Index index(volume);
			index.UpdateSize(&transaction, inode);

			if ((mask & WSTAT_MTIME) == 0)
			{
				index.UpdateLastModified(&transaction, inode);
			}
		}
	}

	if (mask & WSTAT_MODE) {
		PRINT(("original mode = %ld, stat->st_mode = %d\n", node->mode, stat->st_mode));
		node->mode = node->mode & ~S_IUMSK | stat->st_mode & S_IUMSK;
	}

	if (mask & WSTAT_UID)
		node->uid = stat->st_uid;
	if (mask & WSTAT_GID)
		node->gid = stat->st_gid;

	if (mask & WSTAT_MTIME) {
		if (!inode->IsDeleted()) {
			// Index::UpdateLastModified() will set the new time in the inode
			Index index(volume);
			index.UpdateLastModified(&transaction, inode,
				(bigtime_t)stat->st_mtime << INODE_TIME_SHIFT);
		}
	}
	if (mask & WSTAT_CTIME) {
		node->create_time = (bigtime_t)stat->st_ctime << INODE_TIME_SHIFT;
	}

	if ((status = inode->WriteBack(&transaction)) == B_OK)
		transaction.Done();

	//notify_stat_changed(volume->ID(), inode->ID(), mask);
	notify_node_monitors( NWEVENT_STAT_CHANGED, volume->ID(), 0, 0, inode->ID(), NULL, 0 );
	

	return status;
}


status_t 
bfs_create(void *_ns, void *_directory, const char *name, int namelen, int openMode, int perm,
	ino_t *vnodeID, void **_cookie)
{
	FUNCTION_START(("name = \"%s\", perms = %d, openMode = %d\n", name, perm, openMode));

	if (_ns == NULL || _directory == NULL || _cookie == NULL
		|| name == NULL || *name == '\0')
		RETURN_ERROR(B_BAD_VALUE);

	Volume *volume = (Volume *)_ns;
	Inode *directory = (Inode *)_directory;

	if (!directory->IsDirectory())
		RETURN_ERROR(B_BAD_TYPE);
		
	status_t status = directory->CheckPermissions(W_OK);
	if (status < B_OK)
		RETURN_ERROR(status);	

	// We are creating the cookie at this point, so that we don't have
	// to remove the inode if we don't have enough free memory later...
	file_cookie *cookie = (file_cookie *)malloc(sizeof(file_cookie));
	if (cookie == NULL)
		RETURN_ERROR(B_NO_MEMORY); 

	// initialize the cookie
	cookie->open_mode = openMode;
	cookie->last_size = 0;
	cookie->last_notification = get_system_time();

#ifdef UNSAFE_GET_VNODE
	RecursiveLocker locker(volume->Lock());
#endif
	Transaction transaction(volume, directory->BlockNumber());

	status = Inode::Create(&transaction, directory, name, S_FILE | (perm & S_IUMSK),
		openMode, 0, vnodeID);

	if (status >= B_OK) {
		put_vnode(volume->ID(), *vnodeID);
		transaction.Done();
		
		// register the cookie
		*_cookie = cookie;

//		notify_entry_created(volume->ID(), directory->ID(), name, *vnodeID);
		notify_node_monitors( NWEVENT_CREATED, volume->ID(), directory->ID(), 0, *vnodeID, name, namelen);
	} else
		free(cookie);

	return status;
}


static status_t 
bfs_create_symlink(void *_ns, void *_directory, const char *name, int namelen, const char *path/*, int mode*/)
{
	FUNCTION_START(("name = \"%s\", path = \"%s\"\n", name, path));

	if (_ns == NULL || _directory == NULL || path == NULL
		|| name == NULL || *name == '\0')
		RETURN_ERROR(B_BAD_VALUE);

	Volume *volume = (Volume *)_ns;
	Inode *directory = (Inode *)_directory;

	if (!directory->IsDirectory())
		RETURN_ERROR(B_BAD_TYPE);

	status_t status = directory->CheckPermissions(W_OK);
	if (status < B_OK)
		RETURN_ERROR(status);

#ifdef UNSAFE_GET_VNODE
	RecursiveLocker locker(volume->Lock());
#endif
	Transaction transaction(volume, directory->BlockNumber());

	Inode *link;
	off_t id;
	status = Inode::Create(&transaction, directory, name, S_SYMLINK | 0777, 0, 0, &id, &link);
	if (status < B_OK)
		RETURN_ERROR(status);

	size_t length = strlen(path);
	if (length < SHORT_SYMLINK_NAME_LENGTH) {
		strcpy(link->Node()->short_symlink, path);
		status = link->WriteBack(&transaction);
	} else {
		link->Node()->flags |= HOST_ENDIAN_TO_BFS_INT32(INODE_LONG_SYMLINK | INODE_LOGGED);
		// The following call will have to write the inode back, so
		// we don't have to do that here...
		status = link->WriteAt(&transaction, 0, (const uint8 *)path, &length);
	}
	// ToDo: would be nice if Inode::Create() would let the INODE_NOT_READY
	//	flag set until here, so that it can be accessed directly

	// Inode::Create() left the inode locked in memory
	put_vnode(volume->ID(), id);

	if (status == B_OK) {
		transaction.Done();

		//notify_entry_created(volume->ID(), directory->ID(), name, id);
		notify_node_monitors( NWEVENT_CREATED, volume->ID(), directory->ID(), 0, id, name, namelen);
	}

	return status;
}


status_t 
bfs_link(void *ns, void *dir, const char *name, int namelen, void *node)
{
	FUNCTION_START(("name = \"%s\"\n", name));

	// This one won't be implemented in a binary compatible BFS

	return B_ERROR;
}


status_t 
bfs_unlink(void *_ns, void *_directory, const char *name, int namelen)
{
	FUNCTION_START(("name = \"%s\"\n", name));

	if (_ns == NULL || _directory == NULL || name == NULL || *name == '\0')
		return B_BAD_VALUE;
	if (!strcmp(name, "..") || !strcmp(name, "."))
		return B_NOT_ALLOWED;

	Volume *volume = (Volume *)_ns;
	Inode *directory = (Inode *)_directory;

	status_t status = directory->CheckPermissions(W_OK);
	if (status < B_OK)
		return status;

#ifdef UNSAFE_GET_VNODE
	RecursiveLocker locker(volume->Lock());
#endif
	Transaction transaction(volume, directory->BlockNumber());
	off_t id;
	if( bfs_lookup( _ns, _directory, name, namelen, &id) == B_OK )
	{
		notify_node_monitors( NWEVENT_DELETED, volume->ID(), directory->ID(), 0, id, name, namelen);
	}
	
	if ((status = directory->Remove(&transaction, name, &id)) == B_OK) {
		transaction.Done();

		//notify_entry_removed(volume->ID(), directory->ID(), name, id);		
	}
	return status;
}


status_t 
bfs_rename(void *_ns, void *_oldDir, const char *oldName, int oldnamelen, void *_newDir, const char *newName, int newnamelen, bool bMustBeDir )
{
	FUNCTION_START(("oldDir = %p, oldName = \"%s\", newDir = %p, newName = \"%s\"\n", _oldDir, oldName, _newDir, newName));

	// there might be some more tests needed?!
	if (_ns == NULL || _oldDir == NULL || _newDir == NULL
		|| oldName == NULL || *oldName == '\0'
		|| newName == NULL || *newName == '\0'
		|| !strcmp(oldName, ".") || !strcmp(oldName, "..")
		|| !strcmp(newName, ".") || !strcmp(newName, "..")
		|| strchr(newName, '/') != NULL)
		RETURN_ERROR(B_BAD_VALUE);

	Volume *volume = (Volume *)_ns;
	Inode *oldDirectory = (Inode *)_oldDir;
	Inode *newDirectory = (Inode *)_newDir;
	
	// are we already done?
	if (oldDirectory == newDirectory && !strcmp(oldName, newName))
		return B_OK;

	RecursiveLocker locker(volume->Lock());
	
	// get the directory's tree, and a pointer to the inode which should be changed
	BPlusTree *tree;
	status_t status = oldDirectory->GetTree(&tree);
	if (status < B_OK)
		RETURN_ERROR(status);
		
	off_t id;
	status = tree->Find((const uint8 *)oldName, oldnamelen, &id);
	if (status < B_OK)
		RETURN_ERROR(status);
		
	Vnode vnode(volume, id);
	Inode *inode;
	if (vnode.Get(&inode) < B_OK)
		return B_IO_ERROR;
		
	// Don't move a directory into one of its children - we soar up
	// from the newDirectory to either the root node or the old
	// directory, whichever comes first.
	// If we meet our inode on that way, we have to bail out.

	if (oldDirectory != newDirectory) {
		vnode_id parent = volume->ToVnode(newDirectory->Parent());
		vnode_id root = volume->RootNode()->ID();

		while (true) {
			if (parent == id)
				return B_BAD_VALUE;
			else if (parent == root || parent == oldDirectory->ID())
				break;

			Vnode vnode(volume, parent);
			Inode *parentNode;
			if (vnode.Get(&parentNode) < B_OK)
				return B_ERROR;

			parent = volume->ToVnode(parentNode->Parent());
		}
	}

	// Everything okay? Then lets get to work...

	Transaction transaction(volume, oldDirectory->BlockNumber());
	
	// First, try to make sure there is nothing that will stop us in
	// the target directory - since this is the only non-critical
	// failure, we will test this case first
	BPlusTree *newTree = tree;
	if (newDirectory != oldDirectory) {
		status = newDirectory->GetTree(&newTree);
		if (status < B_OK)
			RETURN_ERROR(status);
	}
	
	status = newTree->Insert(&transaction, (const uint8 *)newName, newnamelen, id);
	if (status == B_NAME_IN_USE) {
		// If there is already a file with that name, we have to remove
		// it, as long it's not a directory with files in it
		off_t clobber;
		if (newTree->Find((const uint8 *)newName, newnamelen, &clobber) < B_OK)
			return B_NAME_IN_USE;
		
		if (clobber == id)
			return B_BAD_VALUE;
		
		Vnode vnode(volume, clobber);
		
		Inode *other;
		if (vnode.Get(&other) < B_OK)
			return B_NAME_IN_USE;
		
		status = newDirectory->Remove(&transaction, newName, NULL, other->IsDirectory());
		if (status < B_OK)
			return status;
		
		//notify_entry_removed(volume->ID(), newDirectory->ID(), newName, clobber);
		notify_node_monitors( NWEVENT_DELETED, volume->ID(), newDirectory->ID(), 0, clobber, newName, newnamelen);
		
		status = newTree->Insert(&transaction, (const uint8 *)newName, newnamelen, id);
		
	}
	if (status < B_OK)
		return status;

	// If anything fails now, we have to remove the inode from the
	// new directory in any case to restore the previous state
	status_t bailStatus = B_OK;

	// update the name only when they differ
	bool nameUpdated = false;
	if (strcmp(oldName, newName)) {
		status = inode->SetName(&transaction, newName);
		if (status == B_OK) {
			Index index(volume);
			index.UpdateName(&transaction, oldName, newName, inode);
			nameUpdated = true;
		}
	}
	
	if (status == B_OK) {
		status = tree->Remove(&transaction, (const uint8 *)oldName, oldnamelen, id);
		if (status == B_OK) {
			inode->Parent() = newDirectory->BlockRun();
			
			// if it's a directory, update the parent directory pointer
			// in its tree if necessary
			BPlusTree *movedTree = NULL;
			if (oldDirectory != newDirectory
				&& inode->IsDirectory()
				&& (status = inode->GetTree(&movedTree)) == B_OK)
				status = movedTree->Replace(&transaction, (const uint8 *)"..", 2, newDirectory->ID());

			if (status == B_OK)
				status = inode->WriteBack(&transaction);

			if (status == B_OK) {
				transaction.Done();

				//notify_entry_moved(volume->ID(), oldDirectory->ID(), oldName,
					//newDirectory->ID(), newName, id);
				notify_node_monitors(NWEVENT_MOVED, volume->ID(), oldDirectory->ID(), newDirectory->ID(), id, newName, newnamelen);
				return B_OK;
			}
			
			// If we get here, something has gone wrong already!

			// Those better don't fail, or we switch to a read-only
			// device for safety reasons (Volume::Panic() does this
			// for us)
			// Anyway, if we overwrote a file in the target directory
			// this is lost now (only in-memory, not on-disk)...
			bailStatus = tree->Insert(&transaction, (const uint8 *)oldName, oldnamelen, id);
			if (movedTree != NULL)
				movedTree->Replace(&transaction, (const uint8 *)"..", 2, oldDirectory->ID());
		}
	}
	
	if (bailStatus == B_OK && nameUpdated) {
		bailStatus = inode->SetName(&transaction, oldName);
		if (status == B_OK) {
			// update inode and index
			inode->WriteBack(&transaction);

			Index index(volume);
			index.UpdateName(&transaction, newName, oldName, inode);
		}
	}
	
	if (bailStatus == B_OK)
		bailStatus = newTree->Remove(&transaction, (const uint8 *)newName, newnamelen, id);

	if (bailStatus < B_OK)
		volume->Panic();

	return status;
}

static status_t
bfs_open_dir(void *_ns, void *_node, void **_cookie);

/**	Opens the file with the specified mode.
 */

static status_t
bfs_open(void *_fs, void *_node, int openMode, void **_cookie)
{
	FUNCTION();

	Volume *volume = (Volume *)_fs;
	Inode *inode = (Inode *)_node;
	
	if( inode->IsDirectory() )
	{
		//printk( "bfs_open() called for a directory" );
		return bfs_open_dir( _fs, _node, _cookie );
	}

	// opening a directory read-only is allowed, although you can't read
	// any data from it.
	if (inode->IsDirectory() && openMode & O_RWMASK) {
		openMode = openMode & ~O_RWMASK;
		// ToDo: for compatibility reasons, we don't return an error here...
		// e.g. "copyattr" tries to do that
		//return B_IS_A_DIRECTORY;
	}

	status_t status = inode->CheckPermissions(oModeToAccess(openMode));
	if (status < B_OK)
		RETURN_ERROR(status);

	// we could actually use the cookie to keep track of:
	//	- the last block_run
	//	- the location in the data_stream (indirect, double indirect,
	//	  position in block_run array)
	//
	// This could greatly speed up continuous reads of big files, especially
	// in the indirect block section.

	file_cookie *cookie = (file_cookie *)malloc(sizeof(file_cookie));
	if (cookie == NULL)
		RETURN_ERROR(B_NO_MEMORY); 

	// initialize the cookie
	cookie->open_mode = openMode;
		// needed by e.g. bfs_write() for O_APPEND
	cookie->last_size = inode->Size();
	cookie->last_notification = get_system_time();

	// Should we truncate the file?
	if (openMode & O_TRUNC) {
		WriteLocked locked(inode->Lock());
		Transaction transaction(volume, inode->BlockNumber());

		status_t status = inode->SetFileSize(&transaction, 0);
		if (status < B_OK) {
			// bfs_free_cookie() is only called if this function is successful
			free(cookie);
			return status;
		}

		transaction.Done();
	}

	*_cookie = cookie;
	return B_OK;
}


/**	Read a file specified by node, using information in cookie
 *	and at offset specified by pos. read len bytes into buffer buf.
 */

static status_t
bfs_read(void *_ns, void *_node, void *_cookie, off_t pos, void *buffer, size_t _length)
{
	FUNCTION();
	status_t error;
	Inode *inode = (Inode *)_node;

	if (!inode->HasUserAccessableStream()) {
		_length = 0;
		RETURN_ERROR(B_BAD_VALUE);
	}

	ReadLocked locked(inode->Lock());
	error = inode->ReadAt(pos, (uint8 *)buffer, &_length);
	
	if( error < B_OK )
		return error;
	return _length;
}


static status_t
bfs_write(void *_ns, void *_node, void *_cookie, off_t pos, const void *buffer, size_t _length)
{
	FUNCTION();
	// uncomment to be more robust against a buggy vnode layer ;-)
	//if (_ns == NULL || _node == NULL || _cookie == NULL)
	//	return B_BAD_VALUE;
	
	Volume *volume = (Volume *)_ns;
	Inode *inode = (Inode *)_node;

	if (!inode->HasUserAccessableStream()) {
		//*_length = 0;
		RETURN_ERROR(B_BAD_VALUE);
	}

	file_cookie *cookie = (file_cookie *)_cookie;

	if (cookie->open_mode & O_APPEND)
		pos = inode->Size();

	WriteLocked locked(inode->Lock());
	if (locked.IsLocked() < B_OK)
		RETURN_ERROR(B_ERROR);

	Transaction transaction;
		// We are not starting the transaction here, since
		// it might not be needed at all (the contents of
		// regular files aren't logged)

	status_t status = inode->WriteAt(&transaction, pos, (const uint8 *)buffer, &_length);

	if (status == B_OK)
		transaction.Done();

	if (status == B_OK) {
		// uncached files don't cause notifications during access, and
		// never want to write back any cached blocks

		// periodically notify if the file size has changed
		// ToDo: should we better test for a change in the last_modified time only?
		if (cookie->last_size != inode->Size()
			&& get_system_time() > cookie->last_notification + INODE_NOTIFICATION_INTERVAL) {
//			notify_listener(B_STAT_CHANGED, volume->ID(), 0, 0, inode->ID(), NULL);
			notify_node_monitors( NWEVENT_STAT_CHANGED, volume->ID(), 0, 0, inode->ID(), NULL, 0 );
			cookie->last_size = inode->Size();
			cookie->last_notification = get_system_time();
		}
		
		// This will flush the dirty blocks to disk from time to time.
		// It's done here and not in Inode::WriteAt() so that it won't
		// add to the duration of a transaction - it might even be a
		// good idea to offload those calls to another thread
		volume->WriteCachedBlocksIfNecessary();
	}

	if( status < B_OK )
		return status;
	return _length;
}


/**	Do whatever is necessary to close a file, EXCEPT for freeing
 *	the cookie!
 */
 
static status_t
bfs_free_cookie(void *_ns, void *_node, void *_cookie); 

static status_t
bfs_close_dir(void * ns, void * node, void * _cookie);

static status_t
bfs_close(void *_ns, void *_node, void *_cookie)
{
	FUNCTION();
	if (_ns == NULL || _node == NULL || _cookie == NULL)
		return B_BAD_VALUE;
		
	Inode *inode = (Inode *)_node;
	if( inode->IsDirectory() )
	{
		//printk( "bfs_closed() called on a directory!\n" );
		return bfs_close_dir( _ns, _node, _cookie );
	}

	bfs_free_cookie( _ns, _node, _cookie );

	return B_OK;
}


static status_t
bfs_free_cookie(void *_ns, void *_node, void *_cookie)
{
	FUNCTION();

	if (_ns == NULL || _node == NULL || _cookie == NULL)
		return B_BAD_VALUE;

	file_cookie *cookie = (file_cookie *)_cookie;

	Volume *volume = (Volume *)_ns;
	Inode *inode = (Inode *)_node;
	
	if( inode->Node()->flags & HOST_ENDIAN_TO_BFS_INT32(INODE_DELETED) )
		return( 0 );

	if (cookie->open_mode & O_RWMASK) {
#ifdef UNSAFE_GET_VNODE
		RecursiveLocker locker(volume->Lock());
#endif
		ReadLocked locked(inode->Lock());

		// trim the preallocated blocks and update the size,
		// and last_modified indices if needed

		Transaction transaction(volume, inode->BlockNumber());
		status_t status = B_OK;
		bool changed = false;
		Index index(volume);

		if (inode->OldSize() != inode->Size()) {
			status = inode->Trim(&transaction);
			if (status < B_OK)
				FATAL(("Could not trim preallocated blocks!"));
	
			index.UpdateSize(&transaction, inode);
			changed = true;
		}
		if (inode->OldLastModified() != inode->LastModified()) {
			index.UpdateLastModified(&transaction, inode, inode->LastModified());
			changed = true;
			
			// updating the index doesn't write back the inode
			inode->WriteBack(&transaction);
		}

		if (status == B_OK)
			transaction.Done();

		if (changed)
			notify_node_monitors( NWEVENT_STAT_CHANGED, volume->ID(), 0, 0, inode->ID(), NULL, 0 );
			//notify_stat_changed(volume->ID(), inode->ID(),
				//(changedTime ? B_STAT_MODIFICATION_TIME : 0) | (changedSize ? B_STAT_SIZE : 0));
	}

	if (inode->Flags() & INODE_CHKBFS_RUNNING) {
		// "chkbfs" exited abnormally, so we have to stop it here...
		FATAL(("check process was aborted!\n"));
		volume->Allocator().StopChecking(NULL);
	}

	return B_OK;
}


/**	Checks access permissions, return B_NOT_ALLOWED if the action
 *	is not allowed.
 */

static status_t
bfs_access(void *_ns, void *_node, int accessMode)
{
	FUNCTION();
	
	if (_ns == NULL || _node == NULL)
		return B_BAD_VALUE;

	Inode *inode = (Inode *)_node;
	status_t status = inode->CheckPermissions(accessMode);
	if (status < B_OK)
		RETURN_ERROR(status);

	return B_OK;
}


static status_t
bfs_read_link(void *_ns, void *_node, char *buffer, size_t bufferSize)
{
	FUNCTION();

	Inode *inode = (Inode *)_node;
	
	if (!inode->IsSymLink())
		RETURN_ERROR(B_BAD_VALUE);

	if (inode->Flags() & INODE_LONG_SYMLINK) {
		status_t status = inode->ReadAt(0, (uint8 *)buffer, &bufferSize);
		if (status < B_OK)
			RETURN_ERROR(status);
		
		return( inode->Size() );	}

	size_t numBytes = strlen((char *)&inode->Node()->short_symlink);
	uint32 bytes = numBytes;
	if (bytes > bufferSize)
		bytes = bufferSize;

	memcpy(buffer, inode->Node()->short_symlink, bytes);
	return( bytes );
}


//	#pragma mark -
//	Directory functions


static status_t
bfs_create_dir(void *_ns, void *_directory, const char *name, int namelen, int mode)
{
	FUNCTION_START(("name = \"%s\", perms = %d\n", name, mode));

	if (_ns == NULL || _directory == NULL
		|| name == NULL || *name == '\0')
		RETURN_ERROR(B_BAD_VALUE);

	Volume *volume = (Volume *)_ns;
	Inode *directory = (Inode *)_directory;

	if (!directory->IsDirectory())
		RETURN_ERROR(B_BAD_TYPE);

	status_t status = directory->CheckPermissions(W_OK);
	if (status < B_OK)
		RETURN_ERROR(status);

#ifdef UNSAFE_GET_VNODE
	RecursiveLocker locker(volume->Lock());
#endif
	Transaction transaction(volume, directory->BlockNumber());

	// Inode::Create() locks the inode if we pass the "id" parameter, but we
	// need it anyway
	off_t id;
	status = Inode::Create(&transaction, directory, name, S_DIRECTORY | (mode & S_IUMSK), 0, 0, &id);
	if (status == B_OK) {
		put_vnode(volume->ID(), id);
		transaction.Done();

		notify_node_monitors( NWEVENT_CREATED, volume->ID(), directory->ID(), 0, id, name, namelen);
		//notify_entry_created(volume->ID(), directory->ID(), name, id);
	}

	return status;
}


static status_t
bfs_remove_dir(void *_ns, void *_directory, const char *name, int namelen)
{
	FUNCTION_START(("name = \"%s\"\n", name));
	
	if (_ns == NULL || _directory == NULL || name == NULL || *name == '\0')
		return B_BAD_VALUE;

	Volume *volume = (Volume *)_ns;
	Inode *directory = (Inode *)_directory;
	
	
#ifdef UNSAFE_GET_VNODE
	RecursiveLocker locker(volume->Lock());
#endif
	
	Transaction transaction(volume, directory->BlockNumber());
	
	
	off_t id;
	if( bfs_lookup( _ns, _directory, name, namelen, &id) == B_OK )
	{
		notify_node_monitors( NWEVENT_DELETED, volume->ID(), directory->ID(), 0, id, name, namelen);
	}

	status_t status = directory->Remove(&transaction, name, &id, true);
	if (status == B_OK) {
		
		transaction.Done();
		//notify_entry_removed(volume->ID(), directory->ID(), name, id);
	}
	

	return status;
}


/**	Opens a directory ready to be traversed.
 *	bfs_open_dir() is also used by bfs_open_index_dir().
 */

static status_t
bfs_open_dir(void *_ns, void *_node, void **_cookie)
{
	FUNCTION();
	
	if (_ns == NULL || _node == NULL || _cookie == NULL)
		RETURN_ERROR(B_BAD_VALUE);
	
	Inode *inode = (Inode *)_node;

	// we don't ask here for directories only, because the bfs_open_index_dir()
	// function utilizes us (so we must be able to open indices as well)
	if (!inode->IsContainer())
		RETURN_ERROR(B_BAD_VALUE);
		
	BPlusTree *tree;
	if (inode->GetTree(&tree) != B_OK)
		RETURN_ERROR(B_BAD_VALUE);
		
	TreeIterator *iterator = new TreeIterator(tree);
	if (iterator == NULL)
		RETURN_ERROR(B_NO_MEMORY);
		
	*_cookie = iterator;
	return B_OK;
}


static status_t
bfs_read_dir(void *_ns, void *_node, void *_cookie, int pos, struct kernel_dirent *dirent, 
	size_t bufferSize)
{
	FUNCTION();

	TreeIterator *iterator = (TreeIterator *)_cookie;
	if (iterator == NULL)
		RETURN_ERROR(B_BAD_VALUE);

	uint16 length;
	vnode_id id;
	status_t status = iterator->GetNextEntry(dirent->d_name, &length, bufferSize, &id);
	if (status == B_ENTRY_NOT_FOUND) {
		//*_num = 0;
		//return B_OK;
		return( 0 );
	} else if (status != B_OK)
		RETURN_ERROR(status);

	Volume *volume = (Volume *)_ns;

	//dirent->d_dev = volume->ID();
	dirent->d_ino = id;
	dirent->d_namlen = length;
	
//	dirent->d_reclen = sizeof(struct kernel_dirent) + length;

	//*_num = 1;
	//return B_OK;
	return( 1 );
}


/** Sets the TreeIterator back to the beginning of the directory
 */

static status_t
bfs_rewind_dir(void * /*ns*/, void * /*node*/, void *_cookie)
{
	FUNCTION();
	TreeIterator *iterator = (TreeIterator *)_cookie;

	if (iterator == NULL)
		RETURN_ERROR(B_BAD_VALUE);
	
	return iterator->Rewind();
}

static status_t
bfs_free_dir_cookie(void *ns, void *node, void *_cookie);

static status_t
bfs_close_dir(void * ns, void * node, void * _cookie)
{
	
	FUNCTION();
	
	bfs_free_dir_cookie( ns, node, _cookie );
	
	// Do whatever you need to to close a directory, but DON'T free the cookie!
	return B_OK;
}


static status_t
bfs_free_dir_cookie(void *ns, void *node, void *_cookie)
{
	FUNCTION();
	
	TreeIterator *iterator = (TreeIterator *)_cookie;
	
	if (iterator == NULL)
		RETURN_ERROR(B_BAD_VALUE);

	delete iterator;
	return B_OK;
}


//	#pragma mark -
//	Attribute functions


static status_t
bfs_open_attr_dir(void *_ns, void *_node, void **_cookie)
{
	Inode *inode = (Inode *)_node;

	FUNCTION_START();

	AttributeIterator *iterator = new AttributeIterator(inode);
	if (iterator == NULL)
		RETURN_ERROR(B_NO_MEMORY);

	*_cookie = iterator;
	return B_OK;
}

static status_t
bfs_free_attr_dir_cookie(void *ns, void *node, void *_cookie);

static status_t
bfs_close_attr_dir(void *ns, void *node, void *cookie)
{
	FUNCTION_START();
	
	bfs_free_attr_dir_cookie( ns, node, cookie );
	
	return B_OK;
}


static status_t
bfs_free_attr_dir_cookie(void *ns, void *node, void *_cookie)
{
	FUNCTION_START();
	AttributeIterator *iterator = (AttributeIterator *)_cookie;

	if (iterator == NULL)
		RETURN_ERROR(B_BAD_VALUE);

	delete iterator;
	return B_OK;
}


static status_t
bfs_rewind_attr_dir(void *_ns, void *_node, void *_cookie)
{
	FUNCTION_START();
	
	AttributeIterator *iterator = (AttributeIterator *)_cookie;
	if (iterator == NULL)
		RETURN_ERROR(B_BAD_VALUE);

	RETURN_ERROR(iterator->Rewind());
}


static status_t
bfs_read_attr_dir(void *_ns, void *node, void *_cookie, struct kernel_dirent *dirent,
	size_t bufferSize/*, uint32 *_num*/)
{
	FUNCTION_START();
	AttributeIterator *iterator = (AttributeIterator *)_cookie;
	char buffer[4096];
	ino_t ino;
	if (iterator == NULL)
		RETURN_ERROR(B_BAD_VALUE);
		
	uint32 type;
	size_t length;
	status_t status = iterator->GetNext(buffer, &length, &type, &dirent->d_ino);
	if (status == B_ENTRY_NOT_FOUND) {
		//*_num = 0;
		//return B_OK;
		return( 0 );
	} else if (status != B_OK) {
		RETURN_ERROR(status);
	}
	
	strncpy( dirent->d_name, buffer, length );
	

	//dirent->d_dev = volume->ID();
	//dirent->d_reclen = sizeof(struct dirent) + length;
	dirent->d_namlen = length;
	dirent->d_name[dirent->d_namlen] = '\0';
	//*_num = 1;
	//return B_OK;
	return( 1 );
}

static status_t
bfs_read_attr(void* volume, void *_node, const char* name, int namelen, int type,
			   void* buffer, off_t pos, size_t len)
{
	FUNCTION_START();
	Inode *inode = (Inode *)_node;

	if (inode == NULL || name == NULL || *name == '\0' || buffer == NULL)
		RETURN_ERROR(B_BAD_VALUE);

	status_t status = inode->CheckPermissions(R_OK);
	if (status < B_OK)
		return status;

	status = inode->ReadAttribute(name, type, pos, (uint8 *)buffer, &len);
	if( status < B_OK )
		return status;
	return len;
}


static int
bfs_write_attr(void *_ns, void *_node, const char *name, int namelen, int flags, int type, const void *buffer,
	off_t pos, size_t length)
{
	FUNCTION_START(("name = \"%s\"\n", name));
	if (_ns == NULL || _node == NULL || name == NULL || *name == '\0')
		RETURN_ERROR(B_BAD_VALUE);

	// Writing the name attribute using this function is not allowed,
	// also using the reserved indices name, last_modified, and size
	// shouldn't be allowed.
	// ToDo: we might think about allowing to update those values, but
	//	really change their corresponding values in the bfs_inode structure
	if (name[0] == FILE_NAME_NAME && name[1] == '\0'
		|| !strcmp(name, "name")
		|| !strcmp(name, "last_modified")
		|| !strcmp(name, "size"))
		RETURN_ERROR(B_NOT_ALLOWED);

	Volume *volume = (Volume *)_ns;
	Inode *inode = (Inode *)_node;

	status_t status = inode->CheckPermissions(W_OK);
	if (status < B_OK)
		return status;

#ifdef UNSAFE_GET_VNODE
	RecursiveLocker locker(volume->Lock());
#endif
	Transaction transaction(volume, inode->BlockNumber());

	status = inode->WriteAttribute(&transaction, name, type, pos, (const uint8 *)buffer, &length);
	if (status == B_OK) {
		transaction.Done();
		return length;
	}

	return status;
}


static status_t
bfs_read_attr_stat(void *ns, void *_node, const char *name, int nNameLen, struct attr_info *attrInfo)
{
	FUNCTION_START(("name = \"%s\"\n", name));

	Inode *inode = (Inode *)_node;
	if (inode == NULL || inode->Node() == NULL)
		RETURN_ERROR(B_ERROR);

	// first, try to find it in the small data region
	small_data *smallData = NULL;
	if (inode->SmallDataLock().Lock() == B_OK) {
		if ((smallData = inode->FindSmallData((const char *)name)) != NULL) {
			attrInfo->ai_type = smallData->Type();
			attrInfo->ai_size = smallData->DataSize();
		}
		inode->SmallDataLock().Unlock();
	}
	if (smallData != NULL)
		return B_OK;

	// then, search in the attribute directory
	Inode *attribute;
	status_t status = inode->GetAttribute(name, &attribute);
	if (status == B_OK) {
		attrInfo->ai_type = attribute->Type();
		attrInfo->ai_size = attribute->Size();
		inode->ReleaseAttribute(attribute);
		return B_OK;
	}

	RETURN_ERROR(status);
}



static status_t
bfs_rename_attr(void *ns, void *node, const char *oldname, int oldnamelen, 
				const char *newname, int newnamelen)
{
	FUNCTION_START(("name = \"%s\", to = \"%s\"\n", oldname, newname));

	// ToDo: implement bfs_rename_attr()!
	// I'll skip the implementation here, and will do it for Haiku - at least
	// there will be an API to move one attribute to another file, making that
	// function much more complicated - oh joy ;-)

	RETURN_ERROR(B_ENTRY_NOT_FOUND);
}


static int
bfs_remove_attr(void *_ns, void *_node, const char *name, int namelen)
{
	FUNCTION_START(("name = \"%s\"\n", name));

	if (_ns == NULL || _node == NULL || name == NULL)
		return B_BAD_VALUE;

	Volume *volume = (Volume *)_ns;
	Inode *inode = (Inode *)_node;

	status_t status = inode->CheckPermissions(W_OK);
	if (status < B_OK)
		return status;

#ifdef UNSAFE_GET_VNODE
	RecursiveLocker locker(volume->Lock());
#endif
	Transaction transaction(volume, inode->BlockNumber());

	status = inode->RemoveAttribute(&transaction, name);
	if (status == B_OK) {
		transaction.Done();
	}

	RETURN_ERROR(status);
}
#if 0

//	#pragma mark -
//	Index functions


static status_t
bfs_open_index_dir(void *_ns, void **_cookie)
{
	FUNCTION();
	if (_ns == NULL || _cookie == NULL)
		RETURN_ERROR(B_BAD_VALUE);

	Volume *volume = (Volume *)_ns;

	if (volume->IndicesNode() == NULL)
		RETURN_ERROR(B_ENTRY_NOT_FOUND);

	// Since the indices root node is just a directory, and we are storing
	// a pointer to it in our Volume object, we can just use the directory
	// traversal functions.
	// In fact we're storing it in the Volume object for that reason.

	RETURN_ERROR(bfs_open_dir(_ns, volume->IndicesNode(), _cookie));
}


static status_t
bfs_close_index_dir(fs_volume _fs, fs_cookie _cookie)
{
	FUNCTION();
	if (_fs == NULL || _cookie == NULL)
		RETURN_ERROR(B_BAD_VALUE);

	Volume *volume = (Volume *)_fs;
	RETURN_ERROR(bfs_close_dir(_fs, volume->IndicesNode(), _cookie));
}


static status_t
bfs_free_index_dir_cookie(fs_volume _fs, fs_cookie _cookie)
{
	FUNCTION();
	if (_fs == NULL || _cookie == NULL)
		RETURN_ERROR(B_BAD_VALUE);

	Volume *volume = (Volume *)_fs;
	RETURN_ERROR(bfs_free_dir_cookie(_fs, volume->IndicesNode(), _cookie));
}


static status_t
bfs_rewind_index_dir(fs_volume _fs, fs_cookie _cookie)
{
	FUNCTION();
	if (_fs == NULL || _cookie == NULL)
		RETURN_ERROR(B_BAD_VALUE);

	Volume *volume = (Volume *)_fs;
	RETURN_ERROR(bfs_rewind_dir(_fs, volume->IndicesNode(), _cookie));
}


static status_t
bfs_read_index_dir(fs_volume _fs, fs_cookie _cookie, struct dirent *dirent,
	size_t bufferSize, uint32 *_num)
{
	FUNCTION();
	if (_fs == NULL || _cookie == NULL)
		RETURN_ERROR(B_BAD_VALUE);

	Volume *volume = (Volume *)_fs;
	RETURN_ERROR(bfs_read_dir(_fs, volume->IndicesNode(), _cookie, dirent, bufferSize, _num));
}


static status_t
bfs_create_index(fs_volume _fs, const char *name, uint32 type, uint32 flags)
{
	FUNCTION_START(("name = \"%s\", type = %ld, flags = %ld\n", name, type, flags));
	if (_fs == NULL || name == NULL || *name == '\0')
		return B_BAD_VALUE;

	Volume *volume = (Volume *)_fs;

	if (volume->IsReadOnly())
		return B_READ_ONLY_DEVICE;

	// only root users are allowed to create indices
	if (geteuid() != 0)
		return B_NOT_ALLOWED;

#ifdef UNSAFE_GET_VNODE
	RecursiveLocker locker(volume->Lock());
#endif
	Transaction transaction(volume, volume->Indices());

	Index index(volume);
	status_t status = index.Create(transaction, name, type);

	if (status == B_OK)
		transaction.Done();

	RETURN_ERROR(status);
}


static status_t
bfs_remove_index(void *_ns, const char *name)
{
	FUNCTION();
	if (_ns == NULL || name == NULL || *name == '\0')
		return B_BAD_VALUE;

	Volume *volume = (Volume *)_ns;

	if (volume->IsReadOnly())
		return B_READ_ONLY_DEVICE;

	// only root users are allowed to remove indices
	if (geteuid() != 0)
		return B_NOT_ALLOWED;

	Inode *indices;
	if ((indices = volume->IndicesNode()) == NULL)
		return B_ENTRY_NOT_FOUND;

#ifdef UNSAFE_GET_VNODE
	RecursiveLocker locker(volume->Lock());
#endif
	Transaction transaction(volume, volume->Indices());

	status_t status = indices->Remove(transaction, name);
	if (status == B_OK)
		transaction.Done();

	RETURN_ERROR(status);
}


static status_t
bfs_stat_index(fs_volume _fs, const char *name, struct stat *stat)
{
	FUNCTION_START(("name = %s\n", name));
	if (_fs == NULL || name == NULL || stat == NULL)
		RETURN_ERROR(B_BAD_VALUE);

	Volume *volume = (Volume *)_fs;
	Index index(volume);
	status_t status = index.SetTo(name);
	if (status < B_OK)
		RETURN_ERROR(status);

	bfs_inode &node = index.Node()->Node();

	stat->st_type = index.Type();
	stat->st_size = node.data.Size();
	stat->st_mode = node.Mode();

	stat->st_nlink = 1;
	stat->st_blksize = 65536;

	stat->st_uid = node.UserID();
	stat->st_gid = node.GroupID();

	stat->st_atime = time(NULL);
	stat->st_mtime = stat->st_ctime = (time_t)(node.LastModifiedTime() >> INODE_TIME_SHIFT);
	stat->st_crtime = (time_t)(node.CreateTime() >> INODE_TIME_SHIFT);

	return B_OK;
}


//	#pragma mark -
//	Query functions


static status_t
bfs_open_query(void *_fs, const char *queryString, uint32 flags, port_id port,
	uint32 token, void **_cookie)
{
	FUNCTION_START(("bfs_open_query(\"%s\", flags = %lu, port_id = %ld, token = %ld)\n",
		queryString, flags, port, token));

	Volume *volume = (Volume *)_fs;

	Expression *expression = new Expression((char *)queryString);
	if (expression == NULL)
		RETURN_ERROR(B_NO_MEMORY);

	if (expression->InitCheck() < B_OK) {
		INFORM(("Could not parse query \"%s\", stopped at: \"%s\"\n",
			queryString, expression->Position()));

		delete expression;
		RETURN_ERROR(B_BAD_VALUE);
	}

	Query *query = new Query(volume, expression, flags);
	if (query == NULL) {
		delete expression;
		RETURN_ERROR(B_NO_MEMORY);
	}

	if (flags & B_LIVE_QUERY)
		query->SetLiveMode(port, token);

	*_cookie = (void *)query;

	return B_OK;
}


static status_t
bfs_close_query(void *fs, void *cookie)
{
	FUNCTION();
	return B_OK;
}


static status_t
bfs_free_query_cookie(void *fs, void *cookie)
{
	FUNCTION();
	if (cookie == NULL)
		RETURN_ERROR(B_BAD_VALUE);

	Query *query = (Query *)cookie;
	Expression *expression = query->GetExpression();
	delete query;
	delete expression;

	return B_OK;
}


static status_t
bfs_read_query(void */*fs*/, void *cookie, struct dirent *dirent, size_t bufferSize, uint32 *_num)
{
	FUNCTION();
	Query *query = (Query *)cookie;
	if (query == NULL)
		RETURN_ERROR(B_BAD_VALUE);

	status_t status = query->GetNextEntry(dirent, bufferSize);
	if (status == B_OK)
		*_num = 1;
	else if (status == B_ENTRY_NOT_FOUND)
		*_num = 0;
	else
		return status;

	return B_OK;
}


static status_t
bfs_rewind_query(void */*fs*/, void *cookie)
{
	FUNCTION();
	Query *query = (Query *)cookie;
	if (query == NULL)
		RETURN_ERROR(B_BAD_VALUE);

	return query->Rewind();
}


//	#pragma mark -


static status_t
bfs_std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
#ifdef DEBUG
			add_debugger_commands();
#endif
			return B_OK;
		case B_MODULE_UNINIT:
#ifdef DEBUG
			remove_debugger_commands();
#endif
			return B_OK;

		default:
			return B_ERROR;
	}
}

#endif

static FSOperations_s g_sOperations = { bfs_probe,	// op_probe
	bfs_mount,		// op_mount
	bfs_unmount,		// op_unmount
	bfs_read_vnode,		// op_read_inode
	bfs_release_vnode,	// op_write_inode
	bfs_lookup,		// op_locate_inode
	NULL,			// op_access
	bfs_create, bfs_create_dir, NULL/*afs_mknod*/, bfs_create_symlink, /* op_symlink           */
		NULL, /* op_link              */
		bfs_rename, /* op_rename            */
		bfs_unlink, /* op_unlink            */
		bfs_remove_dir, /* op_rmdir             */
		bfs_read_link, /* op_readlink          */
		bfs_open_dir, /* op_open_dir          */
		bfs_close_dir, /* op_CloseDir          */
		bfs_rewind_dir, /* op_rewinddir         */
		bfs_read_dir, bfs_open, bfs_close, NULL,	// op_free_cookie
	bfs_read,		// op_read
	bfs_write,		// op_write
	NULL,		// op_readv
	NULL,		// op_writev
	NULL, /* op_ioctl             */
		NULL, /* op_setflags          */
		bfs_read_stat, /* op_rstat             */
		bfs_write_stat, /* op_wstat             */
		bfs_fsync, /* op_fsync             */
		bfs_initialize, /* op_initialize        */
		bfs_sync, /* op_sync              */
		bfs_read_fs_stat, /* op_rfsstat           */
		NULL, /* op_wfsstat           */
		NULL, /* op_isatty            */
		NULL, /* op_add_select_req    */
		NULL, /* op_rem_select_req    */
		bfs_open_attr_dir, /* op_open_attrdir      */
		bfs_close_attr_dir, /* op_close_attrdir     */
		bfs_rewind_attr_dir, /* op_rewind_attrdir    */
		bfs_read_attr_dir, /* op_read_attrdir      */
		bfs_remove_attr, /* op_remove_attr       */
		bfs_rename_attr, /* op_rename_attr       */
		bfs_read_attr_stat, /* op_stat_attr         */
		bfs_write_attr, /* op_write_attr        */
		bfs_read_attr, /* op_read_attr         */
		NULL/*afs_open_indexdir*/, /* op_open_indexdir     */
		NULL/*afs_close_indexdir*/, /* op_close_indexdir    */
		NULL/*afs_rewind_indexdir*/, /* op_rewind_indexdir   */
		NULL/*afs_read_indexdir*/, /* op_read_indexdir     */
		NULL/*afs_create_index*/, /* op_create_index      */
		NULL/*afs_remove_index*/, /* op_remove_index      */
		NULL, /* op_rename_index */
		NULL/*afs_stat_index*/, /* op_stat_index        */
		NULL /* op_get_file_blocks */,
		NULL /* op_truncate */
};
int fs_init( const char *pzName, FSOperations_s ** ppsOps )
{
	printk( "initialize_fs called in bfs\n" );
	*ppsOps = &g_sOperations;
	return ( FSDRIVER_API_VERSION );
}
