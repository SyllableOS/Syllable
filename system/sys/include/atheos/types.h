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
#include <posix/types.h>

#ifdef __cplusplus
extern "C" {
#if 0  
} /*make emacs indention work */
#endif
#endif

typedef signed char		int8;	    /* signed 8-bit quantity */
typedef signed short int	int16;	    /* signed 16-bit quantity */
typedef signed long	int	int32;	    /* signed 32-bit quantity */

typedef volatile int		vint;
typedef volatile signed char	vint8;
typedef volatile short		vint16;
typedef volatile long		vint32;


typedef unsigned char		uint8;	  	/* unsigned 8-bit quantity */
typedef unsigned short int	uint16;	  	/* unsigned 16-bit quantity */
typedef unsigned long	int	uint32;		/* unsigned 32-bit quantity */
#ifdef __KERNEL__
typedef unsigned int		uint;		/* unsigned 32-bit quantity */
#endif

typedef volatile unsigned char	vuchar;
typedef volatile unsigned char	vuint8;
typedef volatile unsigned short	vuint16;
typedef volatile unsigned long	vuint32;



typedef signed long	long		int64;	    /* signed 64-bit quantity */
typedef unsigned long	long		uint64;	    /* unsigned 64-bit quantity */
typedef volatile long long		vint64;
typedef volatile unsigned long long	vuint64;
typedef	long long			bigtime_t;

typedef unsigned char			uchar;


#ifndef __cplusplus
typedef char	bool;
#if !defined(false) && !defined(true)
enum {false,true};
#endif
#endif

typedef	int	status_t;

typedef	int	port_id;
typedef	int	thread_id;	/* Process ID		*/
typedef	int	proc_id;	/* Process ID		*/
typedef	int	area_id;
typedef	int	sem_id;
typedef	int	timer_id;
typedef	int	image_id;
typedef int	fs_id;

/*
 * Make sure gcc doesn't try to be clever and move things around
 * on us. We need to use _exactly_ the address the user gave us,
 * not some alias that contains the same information.
 */
typedef struct { volatile int counter; } atomic_t;

#define	OS_NAME_LENGTH	64

#define INFINITE_TIMEOUT		(9223372036854775807LL)

#ifndef TRUE
#define TRUE		1
#endif
#ifndef FALSE
#define FALSE		0
#endif

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

