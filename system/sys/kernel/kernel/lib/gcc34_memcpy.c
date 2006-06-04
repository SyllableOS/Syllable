/*
 * GCC 3.4.3 generates code which requires us to have memcpy() available
 * as a non-inlined function, even with -ffreestanding flag (GCC bug?).
 */

#include <atheos/types.h>


void * memcpy( void* to, const void* from, size_t n )
{
	int d0, d1, d2;
	__asm__ __volatile__(
		"rep ; movsl\n\t"
		"testb $2,%b4\n\t"
		"je 1f\n\t"
		"movsw\n"
		"1:\ttestb $1,%b4\n\t"
		"je 2f\n\t"
		"movsb\n"
		"2:"
		: "=&c" (d0), "=&D" (d1), "=&S" (d2)
		: "0" (n/4), "q" (n), "1" ((long) to), "2" ((long) from)
		: "memory" );
	return (to);
}







