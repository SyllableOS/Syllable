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

int  setup_device_cache( int nDevice, fs_id nFS, off_t nBlockCount );
int  flush_cache_block( int nDev, off_t nBlockNum );
int  flush_device_cache( int nDevice, bool bOnlyLogBlocks );
int  shutdown_device_cache( int nDevice );

void* get_empty_block( int nDev, off_t nBlockNum, int nBlockSize );
void* get_cache_block( int nDev, off_t nBlockNum, int nBlockSize );
int   mark_blocks_dirty( int nDev, off_t nBlockNum, int nBlockCount );
int   set_blocks_info( int nDev, off_t* panBlocks, int nCount, bool bDoClone, cache_callback* pFunc, void* pArg );
void  release_cache_block( int nDev, off_t nBlockNum );

int  cached_read( int nDev, off_t nBlockNum, void *pBuffer, uint nBlockCount, int nBlockSize );
int  cached_write(int nDev, off_t nBlockNum, const void *pBuffer, uint nBlockCount, int nBlockSize );

int write_logged_blocks( int nDev, off_t nBlockNum, const void* pBuffer, uint nCount, int nBlockSize,
			 cache_callback* pFunc, void* pArg );

int set_block_user_data( int nDev, off_t nBlockNum, void* pData );
int read_phys_blocks( int nDev, off_t nBlockNum, void *pBuffer, uint nBlockCount, int nBlockSize);
int write_phys_blocks( int nDev, off_t nBlockNum, const void *pBuffer, uint nBlockCount, int nBlockSize );


#ifdef __cplusplus
}
#endif

#endif /* __F_ATHEOS_BCACHE_H__ */
