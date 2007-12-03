/* libc-internal interface for thread-specific data.  Syllable version.
   Copyright (C) 1998,2001,02 Free Software Foundation, Inc.
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

#ifndef _BITS_LIBC_TSD_H
#define _BITS_LIBC_TSD_H 1

/* This file defines the following macros for accessing a small fixed
   set of thread-specific `void *' data used only internally by libc.

   __libc_tsd_define(CLASS, KEY)	-- Define or declare a `void *' datum
   					   for KEY.  CLASS can be `static' for
					   keys used in only one source file,
					   empty for global definitions, or
					   `extern' for global declarations.
   __libc_tsd_address(KEY)		-- Return the `void **' pointing to
   					   the current thread's datum for KEY.
   __libc_tsd_get(KEY)			-- Return the `void *' datum for KEY.
   __libc_tsd_set(KEY, VALUE)		-- Set the datum for KEY to VALUE.

   The set of available KEY's will usually be provided as an enum,
   and contains (at least):
		_LIBC_TSD_KEY_MALLOC
		_LIBC_TSD_KEY_DL_ERROR
		_LIBC_TSD_KEY_RPC_VARS
   All uses must be the literal _LIBC_TSD_* name in the __libc_tsd_* macros.
   Some implementations may not provide any enum at all and instead
   using string pasting in the macros.  */

#include <atheos/tld.h>

/* __LIBC_TSD_COUNT must always be the last item. */
enum __libc_tsds
{
	CTYPE_B,
	CTYPE_TOLOWER,
	CTYPE_TOUPPER,
	RPC_VARS,
	MALLOC,
	LOCALE,
	__LIBC_TSD_COUNT
};

/* Convert a __libc_tsd into a valid user TLD */
#define __libc_tsd_to_tld(KEY)	(((KEY)*4)+TLD_LIBC)

/* The TLD's for the libc TSD slots are all pre-allocated so this is an empty macro. */
# define __libc_tsd_define(CLASS, KEY)

# define __libc_tsd_address(KEY)	(void**)(get_tld_addr(__libc_tsd_to_tld(KEY)))
# define __libc_tsd_get(KEY)		(get_tld(__libc_tsd_to_tld(KEY)))
# define __libc_tsd_set(KEY, VALUE)	(set_tld(__libc_tsd_to_tld(KEY),VALUE))

#endif	/* bits/libc-tsd.h */
