/*
 * Copyright (c) 1995-1999 by Internet Software Consortium.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND INTERNET SOFTWARE CONSORTIUM DISCLAIMS
 * ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL INTERNET SOFTWARE
 * CONSORTIUM BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
 * ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 */

#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/nameser.h>
#include <resolv.h>


/* The following bit is copied from res_data.c (where it is #ifdef'ed
   out) since res_init() should go into libc.so but the rest of that
   file should not.  */

int
res_init(void) {
	extern int __res_vinit(res_state, int);

	/*
	 * These three fields used to be statically initialized.  This made
	 * it hard to use this code in a shared library.  It is necessary,
	 * now that we're doing dynamic initialization here, that we preserve
	 * the old semantics: if an application modifies one of these three
	 * fields of _res before res_init() is called, res_init() will not
	 * alter them.  Of course, if an application is setting them to
	 * _zero_ before calling res_init(), hoping to override what used
	 * to be the static default, we can't detect it and unexpected results
	 * will follow.  Zero for any of these fields would make no sense,
	 * so one can safely assume that the applications were already getting
	 * unexpected results.
	 *
	 * _res.options is tricky since some apps were known to diddle the bits
	 * before res_init() was first called. We can't replicate that semantic
	 * with dynamic initialization (they may have turned bits off that are
	 * set in RES_DEFAULT).  Our solution is to declare such applications
	 * "broken".  They could fool us by setting RES_INIT but none do (yet).
	 */
	if (!_res.retrans)
		_res.retrans = RES_TIMEOUT;
	if (!_res.retry)
		_res.retry = 4;
	if (!(_res.options & RES_INIT))
		_res.options = RES_DEFAULT;

	/*
	 * This one used to initialize implicitly to zero, so unless the app
	 * has set it to something in particular, we can randomize it now.
	 */
	if (!_res.id)
		_res.id = res_randomid();

	return (__res_vinit(&_res, 1));
}

/* This needs to be after the use of _res in res_init, above.  */
#undef _res

#include <tls.h>

#if USE___THREAD
/* With __thread support, this per-thread variable is used in all cases.  */
__thread struct __res_state _res;
extern __thread struct __res_state __libc_res __attribute__ ((alias ("_res")))
  attribute_hidden;
# define _res __libc_res
#else
/* The resolver state for use by single-threaded programs.
   This differs from plain `struct __res_state _res;' in that it doesn't
   create a common definition, but a plain symbol that resides in .bss,
   which can have an alias.  */
struct __res_state _res __attribute__((section (".bss")));

/* We declare this with compat_symbol so that it's not
   visible at link time.  Programs must use the accessor functions.  */
# if defined HAVE_ELF && defined SHARED && defined DO_VERSIONING
#  include <shlib-compat.h>
compat_symbol (libc, _res, _res, GLIBC_2_0);
# endif
#endif

#include <shlib-compat.h>

#if SHLIB_COMPAT(libc, GLIBC_2_0, GLIBC_2_2)
# undef res_init
extern int __res_init_weak (void);
weak_extern (__res_init_weak);
strong_alias (__res_init, __res_init_weak);
compat_symbol (libc, __res_init_weak, res_init, GLIBC_2_0);
#endif
