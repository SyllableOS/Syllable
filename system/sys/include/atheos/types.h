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

#ifndef	__F_ATHEOS_TYPES_H__
#define	__F_ATHEOS_TYPES_H__

#include <stddef.h>
#ifdef __KERNEL__
#include <atheos/stdint.h>		// kernel C99 integer types
#else
#include <stdint.h>			// C99 standard integer types
#endif
#include <posix/types.h>
#ifndef __cplusplus
#include <stdbool.h>			// C99 boolean type
#endif

#include <atheos/tunables.h>		// values which may be changed

#ifdef __cplusplus
extern "C" {
#if 0  
} /*make emacs indention work */
#endif
#endif

/// old types ///
typedef int8_t			int8;
typedef int16_t			int16;
typedef int32_t			int32;
typedef int64_t			int64;
typedef volatile int		vint;
typedef volatile int8_t		vint8;
typedef volatile int16_t	vint16;
typedef volatile int32_t	vint32;
typedef volatile int64_t	vint64;
typedef uint8_t			uint8;
typedef uint16_t		uint16;
typedef uint32_t		uint32;
typedef uint64_t		uint64;
typedef volatile uint8_t	vuint8;
typedef volatile uint16_t	vuint16;
typedef volatile uint32_t	vuint32;
typedef volatile uint64_t	vuint64;
typedef unsigned int		uint;
typedef unsigned char		uchar;
/// end old types ///

typedef	int_fast64_t		bigtime_t;

typedef	int_fast32_t		status_t;     ///< neg. numbers are error codes
typedef uint_fast32_t           flags_t;      ///< default type for flag bits
typedef uint_fast32_t           count_t;      ///< non-neg. count and loop vars
typedef uint_fast32_t		irq_t;        ///< h/w IRQ number or INT number
typedef int_fast32_t		handle_t;     ///< Handle to some kernel struct

typedef uint_fast32_t		cpu_id;       ///< Processor ID
typedef	int_fast32_t		port_id;      ///< Message Port ID
typedef	int_fast32_t		thread_id;    ///< Thread ID
typedef	int_fast32_t		proc_id;      ///< Process ID
typedef	int_fast32_t		area_id;      ///< Area ID
typedef	int_fast32_t		sem_id;       ///< Semaphore ID
typedef	int_fast32_t		timer_id;     ///< Timer ID
typedef	int_fast32_t		image_id;     ///< Image ID
typedef int_fast32_t		fs_id;        ///< Filesystem ID

/*
 * Make sure gcc doesn't try to be clever and move things around
 * on us. We need to use _exactly_ the address the user gave us,
 * not some alias that contains the same information.
 */
typedef struct { volatile int_fast32_t counter; } atomic_t;

/// Maximum string length including NULL terminator.
//#define	OS_NAME_LENGTH	64

#define INFINITE_TIMEOUT		(9223372036854775807LL)

#ifndef NULL
#define NULL ((void*)0)
#endif

#ifdef __KERNEL__
typedef void (*sighandler_t)(int);
#endif

#ifndef __KERNEL__
#include <atheos/pthreadtypes.h>
#endif

#ifdef __cplusplus
}
#endif

#endif	/* __F_ATHEOS_TYPES_H__ */

