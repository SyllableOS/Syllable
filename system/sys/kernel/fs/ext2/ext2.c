/*  
 *  The Ext2fs driver for Syllable
 *  Copyright (c) 2003 Kurre Stahlberg
 *
 *   This program is free software; you can redistribute it and/or
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


#include "ext2.h"
//#define EXT2_DEBUG

const char *	fsname 		= "ext2fs";
int32		api_version 	= FSDRIVER_API_VERSION;


static int ext2_create_volume(const char * dev, ext2_vol ** _vol) {
	ext2_vol * vol = (ext2_vol *)alloc(sizeof(ext2_vol));
	if(!vol) {
		printk("ext2: Out of memory allocatin vol\n");
		return -ENOMEM;
	}

	vol->fd = open(dev, O_RDONLY);
	if(vol->fd > 0) {
		vol->super = (ext2_superblock *)alloc(sizeof(ext2_superblock));
		if(!vol->super) {
			close(vol->fd);
			free(vol);
			printk("ext2: Out of memory allocatin vol->super\n");
			return -ENOMEM;
		}
		read_pos(vol->fd, 1024, vol->super, sizeof(ext2_superblock));
		if(vol->super->s_magic == EXT2_MAGIC) {
			char * inodes;

			vol->inodes = vol->super->s_inodes_count;
			vol->blocks = vol->super->s_blocks_count;
			vol->r_blocks = vol->super->s_r_blocks_count;
			vol->free_blocks = vol->super->s_free_blocks_count;
			vol->free_inodes = vol->super->s_free_inodes_count;
			vol->b_size = 1024 << vol->super->s_log_block_size;
			if(vol->super->s_log_frag_size < 0) vol->f_size = 1024 >> -vol->super->s_log_block_size;
			else vol->f_size = 1024 << vol->super->s_log_block_size;
			vol->bpg = vol->super->s_blocks_per_group;
			vol->fpg = vol->super->s_frags_per_group;
			vol->ipg = vol->super->s_inodes_per_group;
			vol->mtime = vol->super->s_mtime;
			vol->root_inode = 2;

			vol->fpb = vol->b_size / vol->f_size;
			vol->ipb = vol->b_size / sizeof(_ext2_inode);
			vol->itb_pg = vol->ipg / vol->ipb;
			vol->groups = vol->blocks / vol->bpg;
/*

	Does not work, as probably doesn't align properly. A shame.
			if(vol->ipg) {
				vol->ipg_bits = 1;
				while(vol->ipg >> vol->ipg_bits) {
					if((vol->ipg >> vol->ipg_bits) & 1 && (vol->ipg >> vol->ipg_bits) & ~1) {
						printk("ext2: inodes_per_group is not properly aligned! (%lu)\n", vol->ipg);
						close(vol->fd);
						free(vol->super);
						free(vol);
						return -EINVAL;
					}
					vol->ipg_bits++;
				}
				vol->ipg_bits -= 1;
			} else {
				close(vol->fd);
				free(vol->super);
				free(vol);
				printk("ext2: inodes_per_group = 0! What's this?\n");
				return -EINVAL;
			}
			vol->ipg_mask = vol->ipg - 1;			
*/
			if(vol->ipb) {
				vol->ipb_bits = 1;
				while(vol->ipb >> vol->ipb_bits) {
					if((vol->ipb >> vol->ipb_bits) & 1 && (vol->ipb >> vol->ipb_bits) & ~1) {
						printk("ext2: inodes_per_block is not properly aligned! (%lu)\n", vol->ipb);
						close(vol->fd);
						free(vol->super);
						free(vol);
						return -EINVAL;
					}
					vol->ipb_bits++;
				}
				vol->ipb_bits -= 1;
			} else {
				close(vol->fd);
				free(vol->super);
				free(vol);
				printk("ext2: inodes_per_block = 0! What's this?\n");
				return -EINVAL;
			}
			vol->ipb_mask = vol->ipb - 1;

			vol->grouptable = (ext2_groupblock *)alloc(vol->b_size);
			if(!vol->grouptable) {
				close(vol->fd);
				free(vol->super);
				free(vol);
				printk("ext2: Out of memory allocatin vol->grouptable\n");
				return -ENOMEM;
			}

			read_pos(vol->fd, (vol->super->s_first_data_block + 1) * vol->b_size, vol->grouptable, vol->b_size);

#ifdef EXT2_DEBUG
			printk("ext2: inode_table %lu, free_blocks %lu, free_inodes %lu, used_dirs %lu\n", 
				vol->grouptable[0].inode_table,
				vol->grouptable[0].free_blocks,
				vol->grouptable[0].free_inodes,
				vol->grouptable[0].used_dirs);
#endif

			inodes = (char *)alloc(vol->b_size);
			if(!inodes) {
				close(vol->fd);
				free(vol->grouptable);
				free(vol->super);
				free(vol);
				printk("ext2: Out of memory allocatin inodes\n");
				return -ENOMEM;
			}
			vol->root = (ext2_inode *)alloc(sizeof(ext2_inode));
			if(!vol->root) {
				close(vol->fd);
				free(vol->grouptable);
				free(vol->super);
				free(vol);
				printk("ext2: Out of memory allocatin vol->root\n");
				free(inodes);
				return -ENOMEM;
			}

			read_pos(vol->fd, vol->grouptable[0].inode_table * vol->b_size, inodes, vol->b_size);

			if(vol->super->s_rev_level == 1) {
				printk("ext2: Using EXT2_DYNAMIC_REV. Inode size %du, first inode %lu\n", vol->super->s_inode_size, vol->super->s_first_ino);
				vol->i_size = vol->super->s_inode_size;
			} else {
				printk("ext2: Using EXT2_GOOD_OLD_REV\n");
				vol->i_size = 128;
			}

			memcpy(vol->root, inodes + sizeof(_ext2_inode), sizeof(_ext2_inode));
			vol->root->id = vol->root_inode;
			free(inodes);
			
			*_vol = vol;
			return -EOK;			

		} else {
			printk("%s is not a ext2 partition\n", dev);
			close(vol->fd);
			free(vol->super);
			free(vol);
			return -EINVAL;
		}
	} else {
		free(vol);
		printk("ext2: Can't open device %s\n", dev);
		return -EINVAL;
	}
}

static int ext2_probe(const char * dev, fs_info * fsinfo) {
	ext2_vol * vol;
	if(ext2_create_volume(dev, &vol) == -EOK) {
		ext2_read_fs_stat(vol, fsinfo);
		close(vol->fd);
		free(vol->root);
		free(vol->super);
		free(vol->grouptable);
		free(vol);
		printk("%s identified as ext2fs partition\n", dev);
		return -EOK;
	} else {
		return -EINVAL;
	}
}

static int ext2_mount(kdev_t nsid, const char * dev, uint32 flags, const void * _params, int len, void ** _data, ino_t * vnid) {
	ext2_vol * vol;
	if(ext2_create_volume(dev, &vol) == -EOK) {
		int error;
		vol->id = nsid;
		*vnid = vol->root_inode;	
		*_data = vol;
		
		if((error = setup_device_cache(vol->fd, vol->id, vol->blocks)) != -EOK) {
			close(vol->fd);
			free(vol->root);
			free(vol->super);
			free(vol->grouptable);
			free(vol);
			printk("ext2: Failed to setup dev cache\n");
			return error;
		}
		
		return -EOK;
	} else {
		return -EINVAL;
	}
}

static int ext2_unmount(void * _vol) {
	ext2_vol * vol = (ext2_vol *)_vol;
	int result;
	
	flush_device_cache(vol->fd, false);
	shutdown_device_cache(vol->fd);
	result = close(vol->fd);
	free(vol->root);
	free(vol->super);
	free(vol->grouptable);
	free(vol);
	
	return result;
}

//get_block_from_blocktab
static unsigned long ext2_gbfbtab(int fd, unsigned long tab, unsigned long b_size, unsigned long index) {
	unsigned long bpb = b_size >> 2;
	unsigned long block;
	unsigned long * blocktab;

	if(index >= bpb) return -1;

	blocktab = (unsigned long *)get_cache_block(fd, tab, b_size);
	if(!blocktab) return -1;

	block = blocktab[index];
	release_cache_block(fd, tab);

	return block;
}

static unsigned long ext2_get_block(int fd, ext2_inode * node, unsigned long b_size, unsigned long index) {
	unsigned long block;
	unsigned long bpb = b_size >> 2;

	if(index < 12) block = node->n.block[index];
	else if(index >= 12 && index < 12 + bpb) {
		block = ext2_gbfbtab(fd, node->n.block[12], b_size, index - 12);
	} else if(index >= 12 + bpb && index < 12 + bpb + bpb * bpb) {
		block = ext2_gbfbtab(fd, node->n.block[13], b_size, (index - 12 - bpb) / bpb);
		if(block < 0) return block;
		block = ext2_gbfbtab(fd, block, b_size, (index - 12 - bpb) % bpb);
	} else {
		block = ext2_gbfbtab(fd, node->n.block[14], b_size, (index - 12 - bpb - bpb * bpb) / (bpb * bpb));
		if(block < 0) return block;
		block = ext2_gbfbtab(fd, block, b_size, ((index - 12 - bpb - bpb * bpb) / bpb) % bpb);
		if(block < 0) return block;
		block = ext2_gbfbtab(fd, block, b_size, (index - 12 - bpb - bpb * bpb) % bpb);
	}

#ifdef EXT2_DEBUG
	printk("ext2: Block %d of inode %d is at %d\n", index, node->id, block);
#endif

	return block;
}

static int ext2_walk(void * _vol, void * _parent, const char * name, int name_len, ino_t * inode) {
	ext2_vol * vol = (ext2_vol *)_vol;
	ext2_inode * node = (ext2_inode *)_parent;

#ifdef EXT2_DEBUG
	char n[EXT2_MAX_NAME_LEN];

	strncpy(n, name, name_len);
	n[name_len] = 0;
	printk("ext2_walk - name_len %d, looking for %s\n", name_len, n);
#endif

	if(node->n.mode & EXT2_S_IFDIR) {
		int ba = node->n.blocks * (vol->b_size >> 9);
		int i;

		for(i = 0; i < ba; i++) {
			int block;
			char * dirtab;
			unsigned long pos = 0;
			unsigned long totalpos = i * vol->b_size;
			ext2_dirent * dirent;

			if(totalpos >= node->n.size) return -ENOENT;

			block = ext2_get_block(vol->fd, node, vol->b_size, i);
			if(block < 0) return -ENOENT;
			
			dirtab = (char *)get_cache_block(vol->fd, block, vol->b_size);

			while(1) {
				dirent = (ext2_dirent *)dirtab;
				if(!dirent->inode) {
					i = ba;
					break;
				}

				if(totalpos >= node->n.size) break;

				if(dirent->name_len == name_len && !strncmp(dirent->name, name, name_len)) {
#ifdef EXT2_DEBUG
					strncpy(n, dirent->name, dirent->name_len);
					n[dirent->name_len] = 0;
					printk("ext2: %s in inode %d block %d totalpos %d\n", n, dirent->inode, i, totalpos);
#endif

					*inode = dirent->inode;
					release_cache_block(vol->fd, block);
					return -EOK;
				}
				
				dirtab += dirent->rec_len;
				pos += dirent->rec_len;
				totalpos += dirent->rec_len;
				if(pos >= vol->b_size) break;
			}
			
			release_cache_block(vol->fd, block);
		}
	} else {
		return -EMFILE;
	}

	return -ENOENT;
}

static int ext2_read_inode(void * _vol, ino_t inid, void ** _node) {
	ext2_vol * vol = (ext2_vol *)_vol;
	ext2_inode * node = (ext2_inode *)alloc(sizeof(ext2_inode));

#ifdef EXT2_DEBUG
	printk("ext2_read_inode\n");
#endif

	if(inid >= vol->inodes) {
		free(node);
		printk("ext2: Tried to read out of bounds inode. Request %lu, limit %lu\n", (unsigned long)inid, vol->inodes);
		return -EINVAL;
	}

	inid--;

	if(inid + 1 == vol->root_inode) {
		memcpy(node, vol->root, sizeof(ext2_inode));
	} else {
		unsigned long imipg = inid % vol->ipg;
		unsigned long group = inid  / vol->ipg;
		unsigned long block = (imipg) >> vol->ipb_bits;
		unsigned long iib = (imipg) & vol->ipb_mask;
		_ext2_inode * inodes = (_ext2_inode *)get_cache_block(vol->fd, vol->grouptable[group].inode_table + block, vol->b_size);

		memcpy(node, inodes + iib, vol->i_size);

		release_cache_block(vol->fd, vol->grouptable[group].inode_table + block);
	}

	node->id = inid + 1;
	*_node = node;

	return -EOK;
}

static int ext2_write_inode(void * _vol, void * _node) {
	ext2_vol * vol = (ext2_vol *)_vol;
	ext2_inode * node = (ext2_inode *)_node;

	if(node->id != vol->root_inode) {
		free(node);
	}

	return -EOK;
}

static int ext2_read_stat(void * _vol, void * _node, struct stat * st) {
	ext2_vol * vol = (ext2_vol *)_vol;
	ext2_inode * node = (ext2_inode *)_node;

#ifdef EXT2_DEBUG
	printk("ext2_read_stat\n");
#endif

	st->st_dev = vol->id;

	st->st_ino = node->id;
	st->st_nlink = node->n.links;
	st->st_uid = node->n.uid;
	st->st_gid = node->n.gid;
	st->st_blksize = 65536;
	st->st_mode = node->n.mode;
	st->st_size = node->n.size;
	st->st_ctime = node->n.ctime;
	st->st_mtime = node->n.mtime;
	st->st_atime = node->n.atime;

	return -EOK;
}

static int ext2_open(void * _vol, void * _node, int mode, void ** _cookie) {
//	ext2_vol * vol = (ext2_vol *)_vol;
	ext2_inode * node = (ext2_inode *)_node;

#ifdef EXT2_DEBUG
	printk("ext2_open\n");
#endif

	if (mode == O_WRONLY || mode == O_RDWR || mode & O_TRUNC || mode & O_CREAT) 
		return -EROFS; 

	if(node->n.mode & EXT2_S_IFDIR) {
		ext2_dircookie * cookie = (ext2_dircookie *)alloc(sizeof(ext2_dircookie));
		cookie->inode = node;
		cookie->block = 0;
		cookie->pos = 0;
		cookie->totalpos = 0;
		*_cookie = cookie;
	}

	return -EOK;
}

static int ext2_close(void * _vol, void * _node, void * _cookie) {
//	ext2_vol * vol = (ext2_vol *)_vol;
	ext2_inode * node = (ext2_inode *)_node;
//	ext2_dircookie * cookie = (ext2_dircookie *)_cookie;

#ifdef EXT2_DEBUG
	printk("ext2_close\n");
#endif

	if(node->n.mode & EXT2_S_IFDIR) ext2_free_cookie(_vol, _node, _cookie);

	return -EOK;
}

static int ext2_read(void * _vol, void * _node, void * _cookie, off_t pos, void * _buf, size_t len) {
	ext2_vol * vol = (ext2_vol *)_vol;
	ext2_inode * node = (ext2_inode *)_node;
//	int rv = -EINVAL;
	unsigned long start_block;
	unsigned long end_block;
	unsigned long block;
	unsigned long pos_in_buf = 0;
	char * buf = (void *)_buf;
	unsigned long startpos = 0;
	unsigned long endpos = 0;
	char * b;
	int i;

#ifdef EXT2_DEBUG
	printk("ext2_read - inode %lu, pos %lu, len %lu\n", node->id, (unsigned long)pos, (unsigned long)len);
#endif

	if(node->n.mode & EXT2_S_IFDIR) return -EISDIR;
	if(pos < 0) pos = 0;
	if(len + pos > node->n.size) len = node->n.size - pos;

	if(len <= 0) return 0;

	start_block = pos / vol->b_size;
	end_block = (pos + len) / vol->b_size;
	block = ext2_get_block(vol->fd, node, vol->b_size, start_block);
	if(block < 0) return -EIO;

	startpos = pos % vol->b_size;
	endpos = (pos + len) % vol->b_size;

	if(start_block == end_block) {
		b = (char *)get_cache_block(vol->fd, block, vol->b_size);
		if(!b) return -EIO;
		memcpy(buf, b + startpos, len);
		release_cache_block(vol->fd, block);
		return len;
	}

	b = (char *)get_cache_block(vol->fd, block, vol->b_size);
	memcpy(buf, b + startpos, vol->b_size - startpos);
	release_cache_block(vol->fd, block);
	pos_in_buf += vol->b_size - startpos;

	for(i = start_block + 1; i < end_block; i++) {
		block = ext2_get_block(vol->fd, node, vol->b_size, i);
		if(block < 0) {
			return -EIO;
		}

		b = (char *)get_cache_block(vol->fd, block, vol->b_size);
		if(!b) return -EIO;
		memcpy(buf + pos_in_buf, b, vol->b_size);
		release_cache_block(vol->fd, block);
		pos_in_buf += vol->b_size;
	}

	if(endpos > 0) {
		block = ext2_get_block(vol->fd, node, vol->b_size, end_block);

		if(block < 0) return -EIO;
		b = (char *)get_cache_block(vol->fd, block, vol->b_size);
		if(!b) return -EIO;
		memcpy(buf + pos_in_buf, b, endpos);
		release_cache_block(vol->fd, block);
	}

	return len;
}

static int ext2_free_cookie(void * _vol, void * _node, void * _cookie) {
	ext2_dircookie * cookie = (ext2_dircookie *)_cookie;

#ifdef EXT2_DEBUG
	printk("ext2_free_cookie\n");
#endif

	if(cookie) {
		free(cookie);
	}

	return -EOK;
}

static int ext2_access(void * _vol, void * _node, int mode) {

#ifdef EXT2_DEBUG
	printk("ext2_access\n");
#endif

	return -EOK;
}

static int ext2_read_dir(void * _vol, void * _node, void * _cookie, int po, struct kernel_dirent * ent, size_t entsize) {
	ext2_vol * vol = (ext2_vol *)_vol;
	ext2_inode * node = (ext2_inode *)_node;
	ext2_dircookie * cookie = (ext2_dircookie *)_cookie;
	char * block;
	unsigned long b;
	ext2_dirent * dirent;
	int ba = node->n.blocks * (vol->b_size >> 9);

#ifdef EXT2_DEBUG
	printk("ext2_read_dir - pos %d, block %d, nodeblocks %d size %d total %d\n", cookie->pos, cookie->block, node->n.blocks, node->n.size, cookie->totalpos);
#endif
	
	while(cookie->block < ba) {
		if(cookie->totalpos >= node->n.size) {
#ifdef EXT2_DEBUG
			printk("ext2: Read past end of directory. Stop here. %d %d\n", cookie->totalpos, node->n.size);
#endif
			return 0;
		}

		b =  ext2_get_block(vol->fd, node, vol->b_size, cookie->block);
		if(b < 0) return 0;

		block =(char *)get_cache_block(vol->fd, b, vol->b_size);

		block += cookie->pos;
		dirent = (ext2_dirent *)block;
		
		if(dirent->inode && (cookie->pos + dirent->rec_len) <= vol->b_size) {
			int name_buf_size = (entsize - ((sizeof(dev_t) << 1) + (sizeof(ino_t) << 1) +
					sizeof(unsigned short)));

			if(name_buf_size > dirent->name_len) name_buf_size = dirent->name_len;
			ent->d_ino = dirent->inode;
			ent->d_namlen = name_buf_size;
			strncpy(ent->d_name, dirent->name, name_buf_size);
			
			cookie->pos += dirent->rec_len;
			cookie->totalpos += dirent->rec_len;
			release_cache_block(vol->fd, b);
			return 1;

		} else {
			cookie->pos = 0;
			cookie->block++;
			
			cookie->totalpos = cookie->block  * vol->b_size;
			if(!dirent->inode) cookie->block = ba;
		}

		release_cache_block(vol->fd, b);
	}

	return 0;
}

static int ext2_rewind_dir(void * _vol, void * _node, void * _cookie) {
//	ext2_vol * vol = (ext2_vol *)_vol;
//	ext2_inode * node = (ext2_inode *)_node;
	ext2_dircookie * cookie = (ext2_dircookie *)_cookie;

#ifdef EXT2_DEBUG
	printk("ext2_rewind_dir\n");
#endif

	if(cookie) {
		cookie->block = 0;
		cookie->pos = 0;
		cookie->totalpos = 0;
	}

	return -EOK;
}

static int ext2_read_fs_stat(void * _vol, fs_info * fsinfo) {
	ext2_vol * vol = (ext2_vol *)_vol;

#ifdef EXT2_DEBUG
	printk("ext2_read_fs_stat\n");
#endif

	fsinfo->fi_dev = vol->id;
	fsinfo->fi_root = vol->root_inode;
	fsinfo->fi_flags = FS_IS_PERSISTENT | FS_IS_READONLY | FS_IS_BLOCKBASED | FS_CAN_MOUNT;
	fsinfo->fi_block_size = vol->b_size;
	fsinfo->fi_io_size = 65536;
	fsinfo->fi_total_blocks = vol->blocks;
	fsinfo->fi_free_blocks = 0;
	fsinfo->fi_free_user_blocks = 0;
	fsinfo->fi_total_inodes = vol->inodes;
	fsinfo->fi_free_inodes = vol->free_inodes;
	
	
//	strncpy(fsinfo->fi_device_path, volume->devicePath, sizeof(fsInfo->fi_device_path));
	strncpy(fsinfo->fi_volume_name, vol->super->s_volume_name, 16);
	strcpy( fsinfo->fi_driver_name, fsname);
	
	return -EOK;
}

static int ext2_read_link(void * _vol, void * _node, char * buf, size_t bufsize) {
//	ext2_vol * vol = (ext2_vol *)_vol;
	ext2_inode * node = (ext2_inode *)_node;
	int rv = -EINVAL;

#ifdef EXT2_DEBUG
	printk("ext2_read_link\n");
#endif

	if(node->n.mode & EXT2_S_IFLNK) {
		rv = node->n.size;
		if(rv <= 15 * sizeof(unsigned long)) {
			memcpy(buf, node->n.block, rv);
		} else {
			if(bufsize < rv) {
				rv = bufsize;
			}
			ext2_read(_vol, _node, NULL, 0, (void *)buf, rv);
		}
	}

	return rv;
}

int fs_init( const char* name, FSOperations_s** ops ) {
	*ops = &ext2_operations;
	return( api_version );
}


