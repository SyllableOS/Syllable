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

#define	ENUMLIST( list, node )	for ( node = (APTR) ( (struct List*) (list) )->lh_Head ; ((struct Node*)node)->ln_Succ ; node = (APTR) ((struct Node*)node)->ln_Succ )


#define TOGGLEBITS(a,b) ( (a) ^= (b) )
#define SETBITS(a,b)    ( (a) |= (b) )
#define CLRBITS(a,b)    ( (a) &= ~(b) )
#define	CHANGEBITS( d, m, f ) ( (d) = (f) ? ((d) | (m)) : ((d) & ~(m)) ) /* Set bits if f == TRUE clear otherwise*/

#define BITSIZE(n)      (((n) + 31) & ~31)
#define SETBIT(a,i,v)   *(((uint32_t*)(a))+(i)/32) = (*(((uint32_t*)(a))+(i)/32) & ~(1<<((i)%32))) | (v<<((i)%32))
#define GETBIT(a,i)     ((*(((uint32_t*)(a))+(i)/32) & (1<<((i)%32))) >> ((i)%32))

#define	ABS(a)	(((a)>=0) ? (a) : (-(a)))
#define	SUMADR(a,b)	((void*)(((uintptr_t)(a))+((uintptr_t)(b))))

/* ROUND_UP macro from Linux fs/select.c */
#define ROUND_UP(x,y) (((x)+(y)-1)/(y))

#ifndef __cplusplus
#define	min(a,b)	(((a)<(b)) ? (a) : (b) )
#define	max(a,b)	(((a)>(b)) ? (a) : (b) )
#endif

#if ( __GNUC__ >= 3 )
#define __likely( expr )	__builtin_expect( !!(expr), 1 )
#define __unlikely( expr )	__builtin_expect( !!(expr), 0 )
#else
#define __likely( expr )	(expr)
#define __unlikely( expr )	(expr)
#endif

#ifdef __KERNEL__
#define	kassertw( expr ) do {if ( !( __likely(expr) ) ) { 	\
  printk( "Assertion failure (" #expr ")\n"			\
	  "file: " __FILE__ " function: %s() line: %d\n",	\
	  __FUNCTION__, __LINE__ ); trace_stack( 0, NULL ); } } while(0)
#else
#define	__assertw( expr ) do {if ( !( __likely(expr) ) ) 					\
  dbprintf( "Assertion failure (" #expr ")\n"							\
	  "file: " __FILE__ " function: %s() line: %d\n%p\n%p\n",				\
	  __FUNCTION__, __LINE__, __builtin_return_address(0), __builtin_return_address(1)	\
	  ); } while(0)
#endif

