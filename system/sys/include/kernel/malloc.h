/*
 *  The Syllable kernel
 *  Copyright (C) 2009 Kristian Van Der Vliet
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

#ifndef	__F_KERNEL_MALLOC_H_
#define	__F_KERNEL_MALLOC_H_

#include <kernel/types.h>
#include <kernel/areas.h>

void init_kmalloc( void );

void* kmalloc( size_t nSize, int nFlags );
int __kfree( void* pBlock );

#define kfree(p) kassertw( __kfree(p) == 0 )

void* alloc_real( uint32 nSize, uint32 nFlags );
void free_real( void* Block );

void protect_dos_mem( void );
void unprotect_dos_mem( void );

#endif	/* __F_KERNEL_MALLOC_H_ */
