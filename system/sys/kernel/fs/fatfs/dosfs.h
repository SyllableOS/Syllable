/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef _DOSFS_H_
#define _DOSFS_H_

#include <atheos/semaphore.h>
#include <atheos/bcache.h>

#define new_lock( id, name ) ( (((*(id))=create_semaphore( (name), 1, SEM_RECURSIVE )) < 0) ? -EINVAL : 0 )
#define free_lock(lock) delete_semaphore( *(lock) )

#define create_sem( count, name ) create_semaphore( (name), (count), SEM_RECURSIVE )
#define delete_sem		  delete_semaphore

#define acquire_sem( id )	LOCK( (id) )
#define acquire_sem_etc( id, count, a, b ) lock_semaphore_ex( (id), (count), SEM_NOSIG , INFINITE_TIMEOUT )
#define release_sem_etc( id, count, a )	unlock_semaphore_ex( (id), (count) )
#define release_sem( id ) UNLOCK( (id) )

#define	B_MOUNT_READ_ONLY	0x0001

#define strerror(a)		"strerror_not_implemented"

#define time(a)	(*(a) = (get_real_time() / 1000000L))

#define B_OK 0
#define B_NO_ERROR 0

typedef uint32 ulong;


#ifndef __ATHEOS__
#ifdef USE_DMALLOC
	#include <dmalloc.h>
#else

/* allocate memory from swappable heap */
#define malloc smalloc
#define free sfree
#define calloc scalloc
#define realloc srealloc

#endif

#else
#define malloc(a) kmalloc( (a), MEMF_KERNEL | MEMF_OKTOFAILHACK )
#define calloc(a,b)  kmalloc( (a)*(b), MEMF_KERNEL | MEMF_OKTOFAILHACK | MEMF_CLEAR )
#define free	     kfree
#endif



//#include <KernelExport.h>

/* for multiple reader/single writer locks */
#define READERS 100000

/* Unfortunately, ino_t's are defined as signed. This causes problems with
 * programs (notably cp) that use the modulo of a ino_t as a
 * hash function to index an array. This means the high bit of every ino_t
 * is off-limits. Luckily, FAT32 is actually FAT28, so dosfs can make do with
 * only 63 bits.
 */
#define ARTIFICIAL_VNID_BITS	(0x6LL << 60)
#define DIR_CLUSTER_VNID_BITS	(0x4LL << 60)
#define DIR_INDEX_VNID_BITS		0
#define INVALID_VNID_BITS_MASK	(0x9LL << 60)

#define IS_DIR_CLUSTER_VNID(vnid) \
	(((vnid) & ARTIFICIAL_VNID_BITS) == DIR_CLUSTER_VNID_BITS)

#define IS_DIR_INDEX_VNID(vnid) \
	(((vnid) & ARTIFICIAL_VNID_BITS) == DIR_INDEX_VNID_BITS)

#define IS_ARTIFICIAL_VNID(vnid) \
	(((vnid) & ARTIFICIAL_VNID_BITS) == ARTIFICIAL_VNID_BITS)

#define IS_INVALID_VNID(vnid) \
	((!IS_DIR_CLUSTER_VNID((vnid)) && \
	  !IS_DIR_INDEX_VNID((vnid)) && \
	  !IS_ARTIFICIAL_VNID((vnid))) || \
	 ((vnid) & INVALID_VNID_BITS_MASK))

#define GENERATE_DIR_INDEX_VNID(dircluster, index) \
	(DIR_INDEX_VNID_BITS | ((ino_t)(dircluster) << 32) | (index))

#define GENERATE_DIR_CLUSTER_VNID(dircluster, filecluster) \
	(DIR_CLUSTER_VNID_BITS | ((ino_t)(dircluster) << 32) | (filecluster))

#define CLUSTER_OF_DIR_CLUSTER_VNID(vnid) \
	((uint32)((vnid) & 0xffffffff))

#define INDEX_OF_DIR_INDEX_VNID(vnid) \
	((uint32)((vnid) & 0xffffffff))

#define DIR_OF_VNID(vnid) \
	((uint32)(((vnid) >> 32) & ~0xf0000000))

#define VNODE_PARENT_DIR_CLUSTER(vnode) \
	CLUSTER_OF_DIR_CLUSTER_VNID((vnode)->dir_vnid)

//#include <lock.h>

#define VNODE_MAGIC 'treB'

typedef struct vnode
{
    uint32	magic;
    ino_t	vnid; 		// self id
    ino_t 	dir_vnid;	// parent vnode id (directory containing entry)
    bool	deleted;
      /* iteration is incremented each time the fat chain changes. it's used by
       * the file read/write code to determine if it needs to retraverse the
       * fat chain
       */
    uint32	iteration;

      /* any changes to this block of information should immediately be reflected
       * on the disk (or at least in the cache) so that get_next_dirent continues
       * to function properly
       */
    uint32	sindex, eindex;	// starting and ending index of directory entry	
    uint32	cluster;	// starting cluster of the data
    uint32	fn_nMode;	// dos-style attributes
    off_t	st_size;	// in bytes
    time_t	st_time;

    uint32	end_cluster;	// last cluster of the data

    const char *mime;		// mime type (null if none)
} vnode;

// mode bits
#define FAT_READ_ONLY	1
#define FAT_HIDDEN		2
#define FAT_SYSTEM		4
#define FAT_VOLUME		8
#define FAT_SUBDIR		16
#define FAT_ARCHIVE		32

#define NSPACE_MAGIC 'smaI'

struct vcache_entry;

typedef struct _nspace
{
    uint32	magic;
    fs_id	id;				// ID passed in to fs_mount
    int		fd;				// File descriptor
    char	device[256];
    uint32	flags;			// see <atheos/filesystem.h> for flags
	
      // info from bpb
    uint32	bytes_per_sector;
    uint32	sectors_per_cluster;
    uint32	reserved_sectors;
    uint32	fat_count;
    uint32	root_entries_count;
    uint32	total_sectors;
    uint32	sectors_per_fat;
    uint8	media_descriptor;
    uint16	fsinfo_sector;

    uint32	total_clusters;			// data clusters, that is
    uint32	free_clusters;
    uint8	fat_bits;
    bool	fat_mirrored;			// true if fat mirroring on
    uint8	active_fat;

    uint32  root_start;				// for fat12 + fat16 only
    uint32	root_sectors;			// for fat12 + fat16 only
    vnode	root_vnode;				// root directory
    int32	vol_entry;				// index in root directory
    char	vol_label[12];			// lfn's need not apply

    uint32	data_start;
    uint32  last_allocated;			// last allocated cluster

    sem_id	vlock;

      // vcache state
    struct {
	sem_id vc_sem;
	ino_t cur_vnid;
	uint32 cache_size;
	struct vcache_entry **by_vnid, **by_loc;
    } vcache;

    struct {
	uint32	entries;
	uint32	allocated;
	ino_t *vnid_list;
    } dlist;
} nspace;

#define LOCK_VOL(vol) \
	if (vol == NULL) { printk("null vol\n"); return -EINVAL; } else LOCK((vol)->vlock)

#define UNLOCK_VOL(vol) \
	UNLOCK((vol)->vlock)

#define CHECK_MAGIC(name,struc,magick) \
	int check_##name##_magic(struc *t, char *funcname) \
	{ \
		if (t == NULL) { \
			printk("%s passed null " #name " pointer\n", funcname); \
			return -EINVAL; \
		} else if (t->magic != magick) { \
			printk(#name " (%x) passed to %s has invalid magic number\n", (int)t, funcname); \
			return -EINVAL; \
		} else \
			return 0; \
	}

int check_vnode_magic(struct vnode *t, char *funcname);
int check_nspace_magic(struct _nspace *t, char *funcname);

#define TOUCH(x) ((void)(x))

/* debug levels */
extern int debug_attr, debug_dir, debug_dlist, debug_dosfs, debug_encodings,
		debug_fat, debug_file, debug_iter, debug_vcache;

#endif
