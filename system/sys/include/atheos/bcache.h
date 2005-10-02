/*
 *  The AtheOS kernel
 *  Copyright (C) 1999 - 2000 Kurt Skauen
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

#ifndef __F_ATHEOS_BCACHE_H__
#define __F_ATHEOS_BCACHE_H__

#include <atheos/types.h>

#ifdef __cplusplus
extern "C" {
#if 0  
} /*make emacs indention work */
#endif
#endif

typedef void cache_callback( off_t nBlockNum, int nNumBlocks, void* arg );

status_t  setup_device_cache( dev_t nDevice, fs_id nFS, off_t nBlockCount );
status_t  flush_cache_block( dev_t nDev, off_t nBlockNum );
status_t  flush_device_cache( dev_t nDevice, bool bOnlyLogBlocks );
status_t  flush_locked_device_cache( dev_t nDevice, bool bOnlyLogBlocks );
status_t  shutdown_device_cache( dev_t nDevice );

void* get_empty_block( dev_t nDev, off_t nBlockNum, size_t nBlockSize );
void* get_cache_block( dev_t nDev, off_t nBlockNum, size_t nBlockSize );
status_t mark_blocks_dirty( dev_t nDev, off_t nBlockNum, count_t nBlockCount );
status_t set_blocks_info( dev_t nDev, off_t* panBlocks, count_t nCount, bool bDoClone, cache_callback* pFunc, void* pArg );
void  release_cache_block( dev_t nDev, off_t nBlockNum );

status_t cached_read( dev_t nDev, off_t nBlockNum, void *pBuffer,
		      count_t nBlockCount, size_t nBlockSize );
status_t cached_write( dev_t nDev, off_t nBlockNum, const void *pBuffer,
		       count_t nBlockCount, size_t nBlockSize );

status_t write_logged_blocks( dev_t nDev, off_t nBlockNum, const void* pBuffer, count_t nCount, size_t nBlockSize, cache_callback* pFunc, void* pArg );

status_t set_block_user_data( dev_t nDev, off_t nBlockNum, void* pData );
ssize_t read_phys_blocks( dev_t nDev, off_t nBlockNum, void *pBuffer,
			   count_t nBlockCount, size_t nBlockSize);
ssize_t write_phys_blocks( dev_t nDev, off_t nBlockNum, const void *pBuffer,
			   count_t nBlockCount, size_t nBlockSize );


#ifdef __cplusplus
}
#endif

#endif /* __F_ATHEOS_BCACHE_H__ */
