/* Copyright (C) 1996, 1997, 1998, 1999, 2002 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.  */

#ifndef __SYS_IMAGE_H__

#include <atheos/types.h>
#include <atheos/image.h>

void __init_main_image( void );
libc_hidden_proto( __init_main_image )
void __deinit_main_image( void );
libc_hidden_proto( __deinit_main_image )

/* External functions */
__BEGIN_DECLS

extern int init_library( int lib ) __THROW;
extern int deinit_library( int lib ) __THROW;
extern int load_library( const char* path, uint32 flags ) __THROW;
extern int unload_library( int lib ) __THROW;
extern int get_image_id( void ) __THROW;

__END_DECLS

#endif /* __SYS_IMAGE_H__ */
