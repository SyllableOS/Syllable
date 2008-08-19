/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

typedef long long dr9_off_t;

#include <atheos/kernel.h>
#include <atheos/filesystem.h>
#include <atheos/semaphore.h>
#include <atheos/bcache.h>

#include <atheos/string.h>
#include <posix/errno.h>

#include "dosfs.h"
#include "fat.h"
#include "util.h"

#include "file.h"
#include "vcache.h"

#define END_FAT_ENTRY 0x0ffffff8
#define BAD_FAT_ENTRY 0x0ffffff1

#define DPRINTF(a,b) if (debug_fat > (a)) printk b

static status_t mirror_fats(nspace *vol, uint32 sector, uint8 *buffer)
{
	int32 i;

	if (!vol->fat_mirrored)
		return B_OK;
	
	sector -= vol->active_fat * vol->sectors_per_fat;
	
	for (i=0;i<vol->fat_count;i++) {
		if (i == vol->active_fat)
			continue;
		cached_write(vol->fd, sector + i*vol->sectors_per_fat, buffer, 1, vol->bytes_per_sector);
	}

	return B_OK;
}

// count free: no parameters. returns int32
// get_entry: cluster #. returns int32 entry/status
// set_entry: cluster #, value. returns int32 status
// allocate: # clusters in N, returns int32 status/starting cluster

enum { _IOCTL_COUNT_FREE_, _IOCTL_GET_ENTRY_, _IOCTL_SET_ENTRY_, _IOCTL_ALLOCATE_N_ENTRIES_ };

static int32 _fat_ioctl_(nspace *vol, uint32 action, uint32 cluster, int32 N)
{
    int32 result = 0;
    uint32 n = 0, first = 0, last = 0;
    uint32 i;
    uint32 sector;
    uint32 off, val = 0; /* quiet warning */
    uint8 *block1, *block2 = NULL; /* quiet warning */

      // mark end of chain for allocations
    uint32 V = (action == _IOCTL_SET_ENTRY_) ? N : 0x0ffffff8;

    ASSERT((action >= _IOCTL_COUNT_FREE_) && (action <= _IOCTL_ALLOCATE_N_ENTRIES_));

    if (check_nspace_magic(vol, "_fat_ioctl_")) return -EINVAL;

    DPRINTF(3, ("_fat_ioctl_: action %x, cluster %x, N %x\n", action, cluster, N));

    if (action == _IOCTL_COUNT_FREE_)
	cluster = 2;

    if (action == _IOCTL_ALLOCATE_N_ENTRIES_)
	cluster = vol->last_allocated;

    if (action != _IOCTL_COUNT_FREE_) {
	if (!IS_DATA_CLUSTER(cluster)) {
	    DPRINTF(0, ("_fat_ioctl_ called with invalid cluster (%x)\n", cluster));
	    return -EINVAL;
	}
    }

    off = cluster * vol->fat_bits / 8;
    sector = vol->reserved_sectors + vol->active_fat * vol->sectors_per_fat +
	off / vol->bytes_per_sector;
    off %= vol->bytes_per_sector;

    if ((block1 = (uint8 *)get_cache_block(vol->fd, sector, vol->bytes_per_sector)) == NULL) {
	DPRINTF(0, ("_fat_ioctl_: error reading fat (sector %x)\n", sector));
	return -EIO;
    }

    for (i=0;i<vol->total_clusters;i++) {
	ASSERT(IS_DATA_CLUSTER(cluster));
	ASSERT(off == ((cluster * vol->fat_bits / 8) % vol->bytes_per_sector));

	if (vol->fat_bits == 12) {
	    if (off == vol->bytes_per_sector - 1) {
		if ((block2 = (uint8 *)get_cache_block(vol->fd, ++sector, vol->bytes_per_sector)) == NULL) {
		    DPRINTF(0, ("_fat_ioctl_: error reading fat (sector %x)\n", sector));
		    result = -EIO;
		    goto bi;
		}
	    }
	    if (action != _IOCTL_SET_ENTRY_) {
		if (off == vol->bytes_per_sector - 1) {
		    val = block1[off] + 0x100*block2[0];
		} else
		    val = block1[off] + 0x100*block1[off+1];
		if (cluster & 1) {
		    val >>= 4;
		} else {
		    val &= 0xfff;
		}
		if (val > 0xff0) val |= 0x0ffff000;
	    }
	    if (((action == _IOCTL_ALLOCATE_N_ENTRIES_) && (val == 0)) ||
		(action == _IOCTL_SET_ENTRY_)) {
		uint32 andmask, ormask;
		if (cluster & 1) {
		    ormask = (V & 0xfff) << 4;
		    andmask = 0xf;
		} else {
		    ormask = V & 0xfff;
		    andmask = 0xf000;
		}
		block1[off] &= (andmask & 0xff);
		block1[off] |= (ormask & 0xff);
		if (off == vol->bytes_per_sector - 1) {
		    mark_blocks_dirty(vol->fd, sector - 1, 1);
		    mirror_fats(vol, sector - 1, block1);
		    block2[0] &= (andmask >> 8);
		    block2[0] |= (ormask >> 8);
		} else {
		    block1[off+1] &= (andmask >> 8);
		    block1[off+1] |= (ormask >> 8);
		}
	    }
			
	    if (off == vol->bytes_per_sector - 1) {
		off = (cluster & 1) ? 1 : 0;
		release_cache_block(vol->fd, sector - 1);
		block1 = block2;
	    } else {
		off += (cluster & 1) ? 2 : 1;
	    }
	} else if (vol->fat_bits == 16) {
	    if (action != _IOCTL_SET_ENTRY_) {
		val = block1[off] + 0x100*block1[off+1];
		if (val > 0xfff0) val |= 0x0fff0000;
	    }
	    if (((action == _IOCTL_ALLOCATE_N_ENTRIES_) && (val == 0)) ||
		(action == _IOCTL_SET_ENTRY_)) {
		block1[off] = V & 0xff;
		block1[off+1] = (V >> 8) & 0xff;
	    }
	    off += 2;
	} else if (vol->fat_bits == 32) {
	    if (action != _IOCTL_SET_ENTRY_) {
		val = block1[off] + 0x100*block1[off+1] +
		    0x10000*block1[off+2] + 0x1000000*(block1[off+3]&0x0f);
//				if (val > 0x0ffffff0) val |= 0x00000000;
	    }
	    if (((action == _IOCTL_ALLOCATE_N_ENTRIES_) && (val == 0)) ||
		(action == _IOCTL_SET_ENTRY_)) {
		ASSERT((V & 0xf0000000) == 0);
		block1[off] = V & 0xff;
		block1[off+1] = (V >> 8) & 0xff;
		block1[off+2] = (V >> 16) & 0xff;
		block1[off+3] = (V >> 24) & 0x0f;
		ASSERT(V == (block1[off] + 0x100*block1[off+1] + 0x10000*block1[off+2] + 0x1000000*block1[off+3]));
	    }
	    off += 4;
	} else
	    ASSERT(0);

	if (action == _IOCTL_COUNT_FREE_) {
	    if (val == 0)
		result++;
	} else if (action == _IOCTL_GET_ENTRY_) {
	    result = val;
	    goto bi;
	} else if (action == _IOCTL_SET_ENTRY_) {
	    mark_blocks_dirty(vol->fd, sector, 1);
	    mirror_fats(vol, sector, block1);
	    goto bi;
	} else if ((action == _IOCTL_ALLOCATE_N_ENTRIES_) && (val == 0)) {
	    vol->free_clusters--;
	    mark_blocks_dirty(vol->fd, sector, 1);
	    mirror_fats(vol, sector, block1);
	    if (n == 0) {
		ASSERT(first == 0);
		first = last = cluster;
	    } else {
		ASSERT(IS_DATA_CLUSTER(first));
		ASSERT(IS_DATA_CLUSTER(last));
		  // set last cluster to point to us

		if ((result = _fat_ioctl_(vol,_IOCTL_SET_ENTRY_,last,cluster)) < 0) {
		    ASSERT(0);
		    goto bi;
		}

		last = cluster;
	    }

	    if (++n == N)
		goto bi;
	}

	  // iterate cluster and sector if needed
	if (++cluster == vol->total_clusters + 2) {
	    release_cache_block(vol->fd, sector);
			
	    cluster = 2;
	    off = 2 * vol->fat_bits / 8;
	    sector = vol->reserved_sectors + vol->active_fat * vol->sectors_per_fat;

	    block1 = (uint8 *)get_cache_block(vol->fd, sector, vol->bytes_per_sector);
	}

	if (off >= vol->bytes_per_sector) {
	    release_cache_block(vol->fd, sector);
	    off -= vol->bytes_per_sector; sector++;
	    ASSERT(sector < vol->reserved_sectors + (vol->active_fat + 1) * vol->sectors_per_fat);
	    block1 = (uint8 *)get_cache_block(vol->fd, sector, vol->bytes_per_sector);
	}
		
	if (block1 == NULL) {
	    DPRINTF(0, ("_fat_ioctl_: error reading fat (sector %x)\n", sector));
	    result = -EIO;
	    goto bi;
	}
    }

bi:
    if (block1) release_cache_block(vol->fd, sector);

    if (action == _IOCTL_ALLOCATE_N_ENTRIES_) {
	if (result < 0) {
	    DPRINTF(0, ("pooh. there is a problem. clearing chain (%x)\n", first));
	    if (first != 0) clear_fat_chain(vol, first);
	} else if (n != N) {
	    DPRINTF(0, ("not enough free entries (%x/%x found)\n", n, N));
	    if (first != 0) clear_fat_chain(vol, first);
	    result = -ENOSPC;
	} else if (result == 0) {
	    vol->last_allocated = cluster;
	    result = first;
	    ASSERT(IS_DATA_CLUSTER(first));
	}
    }

    if (result < B_OK)
	DPRINTF(0, ("_fat_ioctl_ error: action = %x cluster = %x N = %x (%s)\n", action, cluster, N, strerror(result)));

    return result;
}

int32 count_free_clusters(nspace *vol)
{
	return _fat_ioctl_(vol, _IOCTL_COUNT_FREE_, 0, 0);
}

static int32 get_fat_entry(nspace *vol, uint32 cluster)
{
	int32 value = _fat_ioctl_(vol, _IOCTL_GET_ENTRY_, cluster, 0);

	if (value < 0)
		return value;

	if ((value == 0) || IS_DATA_CLUSTER(value))
		return value;

	if (value > 0x0ffffff7)
		return END_FAT_ENTRY;
	
	if (value > 0x0ffffff0)
		return BAD_FAT_ENTRY;

	DPRINTF(0, ("invalid fat entry: %x\n", value));
	return BAD_FAT_ENTRY;
}

static status_t set_fat_entry(nspace *vol, uint32 cluster, int32 value)
{
	return _fat_ioctl_(vol, _IOCTL_SET_ENTRY_, cluster, value);
}

// traverse n fat entries
int32 get_nth_fat_entry(nspace *vol, int32 cluster, uint32 n)
{
	if (check_nspace_magic(vol, "get_nth_fat_entry")) return -EINVAL;

	while (n--) {
		cluster = get_fat_entry(vol, cluster);

		if (!IS_DATA_CLUSTER(cluster))
			break;
	}

	ASSERT(cluster != 0);
	
	return cluster;
}

// count number of clusters in fat chain starting at given cluster
// should only be used for calculating directory sizes because it doesn't
// return proper error codes
uint32 count_clusters(nspace *vol, int32 cluster)
{
	int32 count = 0;

	DPRINTF(2, ("count_clusters %x\n", cluster));
	
	if (check_nspace_magic(vol, "count_clusters")) return 0;

	// not intended for use on root directory
	if (!IS_DATA_CLUSTER(cluster)) {
		DPRINTF(0, ("count_clusters called on invalid cluster (%x)\n", cluster));
		return 0;
	}

	while (IS_DATA_CLUSTER(cluster)) {
		count++;

		// break out of circular fat chains in a sketchy manner
		if (count == vol->total_clusters)
			return 0;

		cluster = get_fat_entry(vol, cluster);
	}

	DPRINTF(2, ("count_clusters %x = %x\n", cluster, count));

	if (cluster == END_FAT_ENTRY)
		return count;

	printk("dosfs: count_clusters: cluster = %x\n", cluster);
	ASSERT(0);

	return 0;
}

status_t clear_fat_chain(nspace *vol, uint32 cluster)
{
	int32 c;
	status_t result;

	if (!IS_DATA_CLUSTER(cluster)) {
		DPRINTF(0, ("clear_fat_chain called on invalid cluster (%x)\n", cluster));
		return -EINVAL;
	}

	ASSERT(count_clusters(vol, cluster) != 0);

	DPRINTF(2, ("clearing fat chain: %x", cluster));
	while (IS_DATA_CLUSTER(cluster)) {
		if ((c = get_fat_entry(vol, cluster)) < 0) {
			DPRINTF(0, ("clear_fat_chain: error clearing fat entry for cluster %x (%s)\n", cluster, strerror(c)));
			return c;
		}
		if ((result = set_fat_entry(vol, cluster, 0)) != B_OK) {
			DPRINTF(0, ("clear_fat_chain: error clearing fat entry for cluster %x (%s)\n", cluster, strerror(result)));
			return result;
		}
		vol->free_clusters++;
		cluster = c;
		DPRINTF(2, (", %x", cluster));
	}
	DPRINTF(2, ("\n"));

	if (cluster != END_FAT_ENTRY)
		printk("dosfs: clear_fat_chain: fat chain terminated improperly with %x\n", cluster);

	return 0;
}

status_t allocate_n_fat_entries(nspace *vol, int32 n, int32 *start)
{
	int32 c;

	ASSERT(n > 0);

	DPRINTF(2, ("allocating %x fat entries\n", n));

	c = _fat_ioctl_(vol, _IOCTL_ALLOCATE_N_ENTRIES_, 0, n);
	if (c < 0)
		return c;

	ASSERT(IS_DATA_CLUSTER(c));
	ASSERT(count_clusters(vol, c) == n);

	DPRINTF(2, ("allocated %x fat entries at %x\n", n, c));

	*start = c;
	return 0;
}

status_t set_fat_chain_length(nspace *vol, vnode *node, uint32 clusters,
		bool update_vcache)
{
	status_t result;
	int32 i, c, n;

	DPRINTF(1, ("set_fat_chain_length: %Lx to %x clusters (%x)\n", node->vnid, clusters, node->cluster));
	
	if (IS_FIXED_ROOT(node->cluster) || (!IS_DATA_CLUSTER(node->cluster) && (node->cluster != 0))) {
		DPRINTF(0, ("set_fat_chain_length called on invalid cluster (%x)\n", node->cluster));
		return -EINVAL;
	}

	if (clusters == 0) {
		DPRINTF(1, ("truncating node to zero bytes\n"));
		if (node->cluster == 0)
			return B_OK;
		// truncate to zero bytes
		c = node->cluster;
		node->cluster = 0;
		node->end_cluster = 0;
		result = clear_fat_chain(vol, c);
		if (update_vcache && (result == B_OK))
			result = vcache_set_entry(vol, node->vnid,
					GENERATE_DIR_INDEX_VNID(node->dir_vnid, node->sindex));

		/* write to disk so that get_next_dirent doesn't barf */
		write_vnode_entry(vol, node);

		return result;
	}

	if (node->cluster == 0) {
		DPRINTF(1, ("node has no clusters. adding %x clusters\n", clusters));

		if ((result = allocate_n_fat_entries(vol, clusters, &n)) != B_OK)
			return result;
		node->cluster = n;
		node->end_cluster = get_nth_fat_entry(vol, n, clusters - 1);

		if (update_vcache)
			result = vcache_set_entry(vol, node->vnid,
					GENERATE_DIR_CLUSTER_VNID(node->dir_vnid, node->cluster));

		/* write to disk so that get_next_dirent doesn't barf */
		write_vnode_entry(vol, node);

		return result;
	}

	i = (node->st_size + vol->bytes_per_sector * vol->sectors_per_cluster - 1) /
			vol->bytes_per_sector / vol->sectors_per_cluster;
	if (i == clusters) return B_OK;

	if (clusters > i) {
		// add new fat entries
		DPRINTF(1, ("adding %x new fat entries\n", clusters - i));
		if ((result = allocate_n_fat_entries(vol, clusters - i, &n)) != B_OK)
			return result;

		ASSERT(IS_DATA_CLUSTER(n));

		c = node->end_cluster;
		node->end_cluster = get_nth_fat_entry(vol, n, clusters - i - 1);

		return set_fat_entry(vol, c, n);
	}

	// traverse fat chain
	c = node->cluster;
	n = get_fat_entry(vol,c);
	for (i=1;i<clusters;i++) {
		if (!IS_DATA_CLUSTER(n))
			break;
		c = n;
		n = get_fat_entry(vol,c);
	}

	ASSERT(i == clusters); ASSERT(n != END_FAT_ENTRY);
	if ((i == clusters) && (n == END_FAT_ENTRY)) return B_OK;

	if (n < 0) return n;
	if ((n != END_FAT_ENTRY) && !IS_DATA_CLUSTER(n)) return -EINVAL;

	// clear trailing fat entries
	DPRINTF(1, ("clearing trailing fat entries\n"));
	if ((result = set_fat_entry(vol, c, 0x0ffffff8)) != B_OK)
		return result;
	node->end_cluster = c;
	return clear_fat_chain(vol, n);
}

void dump_fat_chain(nspace *vol, uint32 cluster)
{
	printk("dosfs: fat chain: %x", cluster);
	while (IS_DATA_CLUSTER(cluster)) {
		cluster = get_fat_entry(vol, cluster);
		printk(" %x", cluster);
	}
	printk("\n");
}
