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

#include <errno.h>
#include <sysdep.h>
#include <string.h>
#include <sys/debug.h>
#include <sys/syscall.h>
#include <bits/libc-tsd.h>

#include <atheos/threads.h>
#include <atheos/kernel.h>

/* This is a bad hack but required by the way the LOCALE TLD is used. */
#include "../../../../locale/localeinfo.h"

typedef int thread_entry( void *arg );

struct thread_entry_args{
  thread_entry *entry;
  void *arg;
};

static int __thread_start( struct thread_entry_args *entry_args )
{
  thread_entry *entry = entry_args->entry;
  void *arg = entry_args->arg;
  
  free(entry_args);
  __libc_tsd_set(LOCALE,&_nl_global_locale);

  exit_thread(entry(arg));

  /* exit_thread() should never return.  Log it. */
  dbprintf("%s, %s: panic: exit_thread() returned!\n",__FILE__,__FUNCTION__);
  return 1;
}

thread_id __spawn_thread( const char *name, void* const entry,
                          const int pri, int stack_size, void* const arg )
{
  struct thread_entry_args *entry_args;
  thread_id id;
  
  entry_args = malloc(sizeof(struct thread_entry_args));
  if( NULL == entry_args )
    return -1;
  
  entry_args->entry = entry;
  entry_args->arg = arg;

  id = INLINE_SYSCALL(spawn_thread,5,name,__thread_start,pri,stack_size,entry_args);
  if( id < 0 )
    free(entry_args);
  return id;
}
weak_alias(__spawn_thread,spawn_thread)


