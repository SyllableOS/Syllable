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

#define alloc(a) kmalloc((a), MEMF_KERNEL | MEMF_OKTOFAILHACK)
#define free kfree


#define	EXT2_MAGIC			0xEF53
#define	EXT2_MAX_NAME_LEN	255

//File types
#define	EXT2_S_IFMT			0xF000
#define	EXT2_S_IFSOCK		0xC000
#define	EXT2_S_IFLNK		0xA000
#define	EXT2_S_IFREG		0x8000
#define	EXT2_S_IFBLK		0x6000
#define	EXT2_S_IFDIR		0x4000
#define	EXT2_S_IFCHR		0x2000
#define	EXT2_S_IFIFO			0x1000

//Access
#define	EXT2_S_ISUID		0x0800
#define	EXT2_S_ISGID		0x0400
#define	EXT2_S_ISVTX		0x0200
#define	EXT2_S_IRWXU		0x01C0
#define	EXT2_S_IRUSR		0x0100
#define	EXT2_S_IWUSR		0x0080
#define	EXT2_S_IXUSR		0x0040
#define	EXT2_S_IRWXG		0x0038
#define	EXT2_S_IRGRP		0x0020
#define	EXT2_S_IWGRP		0x0010
#define	EXT2_S_IXGRP		0x0008
#define	EXT2_S_IRWXO		0x0007
#define	EXT2_S_IROTH		0x0004
#define	EXT2_S_IWOTH		0x0002
#define	EXT2_S_IXOTH		0x0001

typedef struct {
	unsigned long	s_inodes_count;
	unsigned long	s_blocks_count;
	unsigned long	s_r_blocks_count;
	unsigned long	s_free_blocks_count;
	unsigned long	s_free_inodes_count;
	unsigned long	s_first_data_block;
	unsigned long	s_log_block_size;
	long			s_log_frag_size;
	unsigned long	s_blocks_per_group;
 	unsigned long	s_frags_per_group;
	unsigned long	s_inodes_per_group;
	unsigned long	s_mtime;
	unsigned long	s_wtime;
	unsigned short	s_mnt_count;
	short		s_max_mnt_count;
	unsigned short	s_magic;
	unsigned short	s_state;
	unsigned short	s_errors;
	unsigned short	s_minor_rev_level;
	unsigned long	s_lastcheck;
	unsigned long	s_checkinterval;
	unsigned long	s_creator_os;
	unsigned long	s_rev_level;
	unsigned short	s_def_resuid;
	unsigned short	s_def_resgid;
	unsigned long	s_first_ino;
	unsigned short	s_inode_size;
	unsigned short	s_block_group_nr;
	unsigned long	s_feature_compat;
	unsigned long	s_feature_incompat;
	unsigned long	s_feature_ro_compat;
	unsigned long	s_uuid[4];
	char			s_volume_name[16];
	unsigned long	s_last_mounted[16];
	unsigned long	s_algo_bitmap;
	char			s_prealloc_blocks;
	char			s_prealloc_dir_blocks;
	unsigned short	alignment;
	unsigned long	s_journal_uuid[4];
	unsigned long	s_journal_inum;
	unsigned long	s_journal_dev;
	unsigned long	s_last_orphan;
	unsigned char	s_reserved[788];
} ext2_superblock;

typedef struct {
	unsigned short	mode;
	unsigned short	uid;
	unsigned long	size;
	unsigned long	atime;
	unsigned long	ctime;
	unsigned long	mtime;
	unsigned long	dtime;
	unsigned short	gid;
	unsigned short	links;
	unsigned long	blocks;
	unsigned long	flags;
	unsigned long	osd1;
	unsigned long	block[15];
	unsigned long	generation;
	unsigned long	file_acl;
	unsigned long	dir_acl;
	unsigned long	faddr;
	unsigned long	osd2[3];
} _ext2_inode;

typedef struct {
	_ext2_inode n;
	unsigned long id;
} ext2_inode;

typedef struct {
	unsigned long	block_bitmap;
	unsigned long	inode_bitmap;
	unsigned long	inode_table;
	unsigned short	free_blocks;
	unsigned short	free_inodes;
	unsigned short	used_dirs;
	unsigned short	pad;
	unsigned long	reserved[3];
} ext2_groupblock;	

typedef struct {
	kdev_t		id;
	int			fd;
	unsigned long	inodes;
	unsigned long	blocks;
	unsigned long	r_blocks;
	unsigned long	free_blocks;
	unsigned long	free_inodes;
	unsigned long	root_inode;
	unsigned long	bpg;
 	unsigned long	fpg;
	unsigned long	ipg;
	unsigned long	mtime;
	unsigned long	i_size;

	unsigned long	b_size;
	unsigned long	f_size;
	unsigned long	fpb;
	unsigned long	ipb;
	unsigned long	itb_pg;
	unsigned long	groups;

	unsigned short	ipg_bits;
	unsigned short	ipb_bits;
	unsigned long	ipg_mask;
	unsigned long	ipb_mask;

	ext2_superblock * super;
	ext2_groupblock * grouptable;
	ext2_inode * root;
} ext2_vol;

typedef struct {
	unsigned long	inode;
	unsigned short	rec_len;
	unsigned char	name_len;
	unsigned char	file_type;
	char			name[EXT2_MAX_NAME_LEN];
} ext2_dirent;

typedef struct {
	ext2_inode  *	inode;
	unsigned long	block;
	unsigned long	pos;
	unsigned long	totalpos;
} ext2_dircookie;
	

static int ext2_probe(const char * dev, fs_info * fsinfo);
static int ext2_mount(kdev_t nsid, const char * dev, uint32 flags, const void * _params, int len, void ** _data, ino_t * vnid);
static int ext2_unmount(void * _vol);
static int ext2_walk(void * _vol, void * _parent, const char * name, int name_len, ino_t * inode);
static int ext2_read_inode(void * _vol, ino_t inid, void ** _node);
static int ext2_write_inode(void * _vol, void * _node);
static int ext2_read_stat(void * _vol, void * _node, struct stat * st);
static int ext2_open(void * _vol, void * _node, int mode, void ** _cookie);
static int ext2_close(void * _vol, void * _node, void * _cookie);
static int ext2_read(void * _vol, void * _node, void * _cookie, off_t pos, void * _buf, size_t len);
static int ext2_free_cookie(void * _vol, void * _node, void * _cookie);
static int ext2_access(void * _vol, void * _node, int mode);
static int ext2_read_dir(void * _vol, void * _node, void * _cookie, int pos, struct kernel_dirent * ent, size_t entsize);
static int ext2_rewind_dir(void * _vol, void * _node, void * _cookie);
static int ext2_read_fs_stat(void * _vol, fs_info * fsinfo);
static int ext2_read_link(void * _vol, void * _node, char * buf, size_t bufsize);

static FSOperations_s ext2_operations =  {
	ext2_probe,
	ext2_mount,
	ext2_unmount,
	ext2_read_inode,
	ext2_write_inode,
	ext2_walk,
	ext2_access,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	ext2_read_link,
	NULL,
	NULL,
	ext2_rewind_dir,
	ext2_read_dir,
	ext2_open,
	ext2_close,
	NULL,
	ext2_read,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	ext2_read_stat,
	NULL,
	NULL,
	NULL,
	NULL,
	ext2_read_fs_stat,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};
