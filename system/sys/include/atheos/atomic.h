#ifndef __ATHEOS_ATOMIC_H__
#define __ATHEOS_ATOMIC_H__

#ifdef __cplusplus
extern "C" {
#if 0
} /*make emacs indention work */
#endif
#endif

struct __atomic_fool_gcc_struct { unsigned long a[100]; };
#define __atomic_fool_gcc(x) (*(volatile struct __atomic_fool_gcc_struct *)(x))

/*
 * Atomic operations that C can't guarantee us.  Useful for
 * resource counting etc..
 */

#if 1  // CONFIG_SMP
#define LOCK_ "lock ; "
#else
#define LOCK_ ""
#endif

#include <atheos/types.h>

#define ATOMIC_INIT(i)	{ (i) }

/**
 * atomic_read - read atomic variable
 * @v: pointer of type atomic_t
 * 
 * Atomically reads the value of @v.
 */ 
#define atomic_read(v)		((v)->counter)

/**
 * atomic_set - set atomic variable
 * @v: pointer of type atomic_t
 * @i: required value
 * 
 * Atomically sets the value of @v to @i.
 */ 
#define atomic_set(v,i)		(((v)->counter) = (i))

/**
 * atomic_add - add integer to atomic variable
 * @i: integer value to add
 * @v: pointer of type atomic_t
 * 
 * Atomically adds @i to @v.
 */
static __inline__ void atomic_add( atomic_t* v, int i )
{
	__asm__ __volatile__(
		LOCK_ "addl %1,%0"
		:"=m" (v->counter)
		:"ir" (i), "m" (v->counter));
}

/**
 * atomic_sub - subtract the atomic variable
 * @i: integer value to subtract
 * @v: pointer of type atomic_t
 * 
 * Atomically subtracts @i from @v.
 */
static __inline__ void atomic_sub( atomic_t *v, int i )
{
	__asm__ __volatile__(
		LOCK_ "subl %1,%0"
		:"=m" (v->counter)
		:"ir" (i), "m" (v->counter));
}

/**
 * atomic_or - bitwise OR the atomic variable
 * @i: integer value to bitwise or
 * @v: pointer of type atomic_t
 * 
 * Atomically ORs @i from @v.
 */
static __inline__ void atomic_or( atomic_t *v, int i )
{
	__asm__ __volatile__(
		LOCK_ "orl %1,%0"
		:"=m" (v->counter)
		:"ir" (i), "m" (v->counter));
}

/**
 * atomic_and - bitwise AND the atomic variable
 * @i: integer value to bitwise and
 * @v: pointer of type atomic_t
 * 
 * Atomically ANDs @i from @v.
 */
static __inline__ void atomic_and( atomic_t *v, int i )
{
	__asm__ __volatile__(
		LOCK_ "andl %1,%0"
		:"=m" (v->counter)
		:"ir" (i), "m" (v->counter));
}

/**
 * atomic_sub_and_test - subtract value from variable and test result
 * @i: integer value to subtract
 * @v: pointer of type atomic_t
 * 
 * Atomically subtracts @i from @v and returns
 * true if the result is zero, or false for all
 * other cases.
 */
static __inline__ int atomic_sub_and_test( atomic_t *v, int i )
{
	unsigned char c;

	__asm__ __volatile__(
		LOCK_ "subl %2,%0; sete %1"
		:"=m" (v->counter), "=qm" (c)
		:"ir" (i), "m" (v->counter) : "memory");
	return c;
}

/**
 * atomic_inc - increment atomic variable
 * @v: pointer of type atomic_t
 * 
 * Atomically increments @v by 1.
 */ 
static __inline__ void atomic_inc(atomic_t *v)
{
	__asm__ __volatile__(
		LOCK_ "incl %0"
		:"=m" (v->counter)
		:"m" (v->counter));
}

/**
 * atomic_dec - decrement atomic variable
 * @v: pointer of type atomic_t
 * 
 * Atomically decrements @v by 1.
 */ 
static __inline__ void atomic_dec(atomic_t *v)
{
	__asm__ __volatile__(
		LOCK_ "decl %0"
		:"=m" (v->counter)
		:"m" (v->counter));
}

/**
 * atomic_dec_and_test - decrement and test
 * @v: pointer of type atomic_t
 * 
 * Atomically decrements @v by 1 and
 * returns true if the result is 0, or false for all other
 * cases.
 */ 
static __inline__ int atomic_dec_and_test(atomic_t *v)
{
	unsigned char c;

	__asm__ __volatile__(
		LOCK_ "decl %0; sete %1"
		:"=m" (v->counter), "=qm" (c)
		:"m" (v->counter) : "memory");
	return c != 0;
}

/**
 * atomic_inc_and_test - increment and test 
 * @v: pointer of type atomic_t
 * 
 * Atomically increments @v by 1
 * and returns true if the result is zero, or false for all
 * other cases.
 */ 
static __inline__ int atomic_inc_and_test(atomic_t *v)
{
	unsigned char c;

	__asm__ __volatile__(
		LOCK_ "incl %0; sete %1"
		:"=m" (v->counter), "=qm" (c)
		:"m" (v->counter) : "memory");
	return c != 0;
}

/**
 * atomic_inc_and_read - increment and return old value
 * @v: pointer of type atomic_t
 * 
 * Atomically increments @v by 1
 * and returns the previous value.
 */ 
static __inline__ int atomic_inc_and_read(atomic_t *v)
{
	int nOldVal;
	int nTmp = 0;

	__asm__ __volatile__(
		"movl %3,%0\n\t"	// Start with the old value in EAX
		"0:\n\t"
		"movl %0,%2\n\t"	// Copy the old value to nTmp
		"incl %2\n\t"
		LOCK_ "cmpxchgl %2,%3\n\t"	// Update v if value hasn't changed, else copy new v into EAX
		"jnz 0b"
		:"=&a" (nOldVal), "=m" (v->counter), "=&q" (nTmp)
		:"m" (v->counter) : "memory");
	return nOldVal;
}

/**
 * atomic_add_negative - add and test if negative
 * @v: pointer of type atomic_t
 * @i: integer value to add
 * 
 * Atomically adds @i to @v and returns true
 * if the result is negative, or false when
 * result is greater than or equal to zero.
 */ 
static __inline__ int atomic_add_negative( atomic_t *v, int i )
{
	unsigned char c;

	__asm__ __volatile__(
		LOCK_ "addl %2,%0; sets %1"
		:"=m" (v->counter), "=qm" (c)
		:"ir" (i), "m" (v->counter) : "memory");
	return c;
}

/**
 * atomic_swap - swap two values
 * @v: pointer to int or atomic_t value to swap
 * @i: integer value to swap
 *
 * Atomically swaps @i into @v and returns the old value at @v.
 * Note: no "lock" prefix even on SMP: xchg always implies lock.
 */
static __inline__ int atomic_swap( volatile void *v, int i )
{
	__asm__ __volatile__("xchgl %0,%1"
		:"=r" (i)
		:"m" (__atomic_fool_gcc(v)), "0" (i)
		:"memory");
	return i;
}

static __inline__ int atomic_cmp_and_set( volatile void *dst, int exp, int src )
{
	int res = exp;

	__asm__ __volatile__ (
	"	lock ;			"
	"	cmpxchgl %1,%2 ;	"
	"       setz	%%al ;		"
	"	movzbl	%%al,%0 ;	"
	"1:				"
	"# atomic_cmpset_int"
	: "+a" (res)							/* 0 (result) */
	: "r" (src),							/* 1 */
	  "m" (__atomic_fool_gcc(dst))			/* 2 */
	: "memory");				 

	return res;
}

#ifdef __cplusplus
}
#endif

#endif /* __ATHEOS_ATOMIC_H__ */

