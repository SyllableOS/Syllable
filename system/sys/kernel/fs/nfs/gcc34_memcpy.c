/*
 * GCC 3.4.3 generates code which requires us to have strcpy() and memcpy()
 * available as (non-inlined) functions, so supply them here.
 */

#include <atheos/types.h>

char * strcpy( char* dest, const char* src )
{
	int d0, d1, d2;
	__asm__ __volatile__(
		"1:\tlodsb\n\t"
		"stosb\n\t"
		"testb %%al,%%al\n\t"
		"jne 1b"
		: "=&S" (d0), "=&D" (d1), "=&a" (d2)
		: "0" (src), "1" (dest) : "memory" );
	return dest;
}

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


