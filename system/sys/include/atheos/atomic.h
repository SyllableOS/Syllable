/*
 *  The AtheOS kernel
 *  Copyright (C) 1999 - 2001 Kurt Skauen
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

#ifndef __ATHEOS_ATOMIC_H__
#define __ATHEOS_ATOMIC_H__

#ifdef __cplusplus
extern "C" {
#if 0  
} /*make emacs indention work */
#endif
#endif

typedef int atomic_t;

#include <atheos/types.h>

struct __atomic_fool_gcc_struct { int a[100]; };
#define __atomic_fool_gcc(x) (*(volatile struct __atomic_fool_gcc_struct *)x)

/** 
 * \par Description:
 *	Add nValue to *pnTarget and return the old *pnTarget. The operation
 *	is atomic and can not be affected by interrupts or other CPU's writing
 *	to *pnTarget.
 * \par Note:
 * \par Warning:
 * \param
 * \return
 * \sa
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

static inline atomic_t atomic_add( atomic_t* pnTarget, atomic_t nValue  )
{
    register atomic_t nOldVal;
    register int nTmp = 0;
    __asm__ volatile( "  movl %3,%%eax;"	/* Start with the old-value in EAX */
		      "0:;"
		      "  movl %%eax,%%edx;"	/* Put the old-value where cmpxchgl want it */
		      "  addl %4,%%edx;"
		      "  lock; cmpxchgl %%edx,%3;" /* if (*pnTarget==eax) {*pnTarget=edx;} else {eax=*pnTarget;goto 0;} */
		      "  jnz 0b;"
		      : "=&a" (nOldVal), "=m"(__atomic_fool_gcc(pnTarget)), "=&d"(nTmp)
		      : "m" (__atomic_fool_gcc(pnTarget)), "r" (nValue)
		      :  "cc" );
    return( nOldVal );
}

/** 
 * \par Description:
 *	Bitwise or nValue to *pnTarget and return the old *pnTarget. The operation
 *	is atomic and can not be affected by interrupts or other CPU's writing
 *	to *pnTarget.
 * \par Note:
 * \par Warning:
 * \param
 * \return
 * \sa
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

static inline atomic_t atomic_or( atomic_t* pnTarget, atomic_t nValue  )
{
    register atomic_t nOldVal;
    register int nTmp = 0;
    __asm__ volatile( "  movl %3,%%eax;"	/* Start with the old-value in EAX */
		      "0:;"
		      "  movl %%eax,%%edx;"	/* Put the old-value where cmpxchgl want it */
		      "  orl %4,%%edx;"
		      "  lock; cmpxchgl %%edx,%3;"	/* if (*pnTarget==eax) {*pnTarget=edx;} else {eax=*pnTarget;goto 0;} */
		      "  jnz 0b;"
		      : "=&a" (nOldVal), "=m"(__atomic_fool_gcc(pnTarget)), "=&d"(nTmp)
		      : "m" (__atomic_fool_gcc(pnTarget)), "r" (nValue)
		      : "cc" );
    return( nOldVal );
}

/** 
 * \par Description:
 *	Bitwise and nValue with *pnTarget and return the old *pnTarget. The operation
 *	is atomic and can not be affected by interrupts or other CPU's writing
 *	to *pnTarget.
 * \par Note:
 * \par Warning:
 * \param
 * \return
 * \sa
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

static inline atomic_t atomic_and( atomic_t* pnTarget, atomic_t nValue  )
{
    register atomic_t nOldVal;
    register int nTmp = 0;
    __asm__ volatile( "  movl %3,%%eax;"	/* Start with the old-value in EAX */
		      "0:;"
		      "  movl %%eax,%%edx;"	/* Put the old-value where cmpxchgl want it */
		      "  andl %4,%%edx;"
		      "  lock; cmpxchgl %%edx,%3;"	/* if (*pnTarget==eax) {*pnTarget=edx;} else {eax=*pnTarget;goto 0;} */
		      "  jnz 0b;"
		      : "=&a" (nOldVal), "=m"(__atomic_fool_gcc(pnTarget)), "=&d"(nTmp)
		      : "m" (__atomic_fool_gcc(pnTarget)), "r" (nValue)
		      : "cc" );
    return( nOldVal );
}

/** 
 * \par Description:
 *	Move nValue into *pnTarget and return the old *pnTarget. The operation
 *	is atomic and can not be affected by interrupts or other CPU's writing
 *	to *pnTarget.
 * \par Note:
 * \par Warning:
 * \param
 * \return
 * \sa
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

static inline atomic_t atomic_swap( atomic_t* pnTarget, atomic_t nValue )
{
    atomic_t nOldValue;
    __asm__ volatile( "lock; xchgl %3,%1"
		      : "=r"(nOldValue), "=m" (__atomic_fool_gcc(pnTarget))
		      : "m" (__atomic_fool_gcc(pnTarget)), "0"(nValue) );
    return( nOldValue );
}

/** 
 * \par Description:
 *	Decrement a value and return "true" if the result was <= 0
 * \par Note:
 * \par Warning:
 * \param
 * \return
 * \sa
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

static inline bool atomic_dec_and_test( atomic_t* pnTarget )
{
    int nResult;

    __asm__ volatile( "xorl %%eax,%%eax;"
		      "lock; decl %1;"
		      "setle %%al;"
		      : "=&a"(nResult)
		      : "m"(__atomic_fool_gcc(pnTarget)) );
	    
    return( (bool) nResult );
}

#ifdef __cplusplus
}
#endif

#endif /* __ATHEOS_ATOMIC_H__ */
