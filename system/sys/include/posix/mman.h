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

#ifndef __F_POSIX_MMAN_H__
#define __F_POSIX_MMAN_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Protections are chosen from these bits, OR'd together.  The
   implementation does not necessarily support PROT_EXEC or PROT_WRITE
   without PROT_READ.  The only guarantees are that no writing will be
   allowed without PROT_WRITE and no access will be allowed for PROT_NONE. */

#define PROT_READ	0x1		/* Page can be read.  */
#define PROT_WRITE	0x2		/* Page can be written.  */
#define PROT_EXEC	0x4		/* Page can be executed.  */
#define PROT_NONE	0x0		/* Page can not be accessed.  */

#if defined(__USE_MISC) || defined(__KERNEL__)
# define PROT_TYPE	0x0f	/* Mask for protection flags.  */
# define PROT_FLAGS(x)	(x & PROT_TYPE)
#endif

/* Sharing types (must choose one and only one of these).  */
#define MAP_SHARED	0x01 << 4		/* Share changes.  */
#define MAP_PRIVATE	0x02 << 4		/* Changes are private.  */
#if defined(__USE_MISC) || defined(__KERNEL__)
# define MAP_TYPE	0xf0			/* Mask for type of mapping.  */
# define MAP_FLAGS(x)	(x & MAP_TYPE)
#endif

/* Other flags.  */
#define MAP_FIXED		0x04 << 4		/* Interpret addr exactly.  */
#if defined(__USE_MISC) || defined(__KERNEL__)
# define MAP_FILE		0
# define MAP_ANONYMOUS	0x08 << 4		/* Don't use a file.  */
# define MAP_ANON	MAP_ANONYMOUS
#endif

#ifdef __KERNEL__
#include <posix/types.h>

#define MAP_FAILED	-1

void* sys_mmap( void *pAddr, size_t nLen, int nFlags, int nFile, off_t nOffset );
int sys_munmap( void *pAddr, size_t nLen );
int sys_mprotect( void *pAddr, size_t nLen, int nProt );
#endif

#ifdef __cplusplus
}
#endif

#endif /* __F_POSIX_MMAN_H__ */
