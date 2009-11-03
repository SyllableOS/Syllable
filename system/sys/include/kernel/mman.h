/*
 *  The Syllable kernel
 *  Copyright (C) 2009 The Syllable Team
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

#ifndef __F_KERNEL_MMAN_H__
#define __F_KERNEL_MMAN_H__

#define __USE_MISC
#include <posix/mman.h>
#undef __USE_MISC

#include <posix/types.h>

#define MAP_FAILED	-1

void* sys_mmap( void *pAddr, size_t nLen, int nFlags, int nFile, off_t nOffset );
int sys_munmap( void *pAddr, size_t nLen );
int sys_mprotect( void *pAddr, size_t nLen, int nProt );

#endif	/* __F_KERNEL_MMAN_H__ */
