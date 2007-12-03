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

/* We need to fool ldsodefs.h into thinking we're in the RTLD at this point,
   as we declare rtld_global & rtld_global_ro which are usually created as
   part of ld.so on other systems */
#define IS_IN_rtld 1

#include <ldsodefs.h>

#ifdef SHARED

#include <dl-procinfo.h>
#include <fpu_control.h>

/* Taken from elf/rtld.c because we don't use the RTLD in Syllable */

/* This variable is similar to _rtld_local, but all values are
   read-only after relocation.  */
struct rtld_global_ro _rtld_global_ro =
  {
    /* Get architecture specific initializer.  */
#include <dl-procinfo.c>
    ._dl_hwcap_mask = HWCAP_IMPORTANT,
    ._dl_fpu_control = _FPU_DEFAULT,
  };

/* If we would use strong_alias here the compiler would see a
   non-hidden definition.  This would undo the effect of the previous
   declaration.  So spell out was strong_alias does plus add the
   visibility attribute.  */
extern struct rtld_global_ro _rtld_local_ro;
#endif



