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

#include <stdint.h>
#include <syllable/inttypes.h>

#include <stddef.h>

#include <posix/types.h>

#ifndef __cplusplus
# include <stdbool.h>
#endif

/* XXXKV: I want to get rid of this */
#ifndef __KERNEL__
# include <syllable/pthreadtypes.h>
#endif

#endif	/* __F_ATHEOS_TYPES_H__ */

