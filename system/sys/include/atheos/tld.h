/*
 *  The AtheOS kernel
 *  Copyright (C) 2001 Kurt Skauen
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

#ifndef	__F_ATHEOS_TLD_H__
#define	__F_ATHEOS_TLD_H__

#include <atheos/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TLD_SIZE	(PAGE_SIZE * 4)
#define TLD_THID	0
#define TLD_PRID	4
#define TLD_ERRNO_ADDR	8
#define TLD_ERRNO	12
#define	TLD_USER	1024

#ifdef __KERNEL__

int alloc_tld( void );
void  set_tld( int nHandle, void* pValue );
void* get_tld( int nHandle );

#else /* __KERNEL__ */

int   alloc_tld( void* pDestructor );
void  set_tld( int nHandle, void* pValue );
void* get_tld( int nHandle );

#endif /* __KERNEL__ */

int free_tld( int nHandle );

#ifdef __cplusplus
}
#endif


#endif /* __F_ATHEOS_TLD_H__ */
