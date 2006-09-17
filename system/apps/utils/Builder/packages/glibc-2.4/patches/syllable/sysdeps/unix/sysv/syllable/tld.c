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

#include <malloc.h>
#include <sysdep.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <bits/libc-lock.h>
#include <atheos/tld.h>

typedef struct tld_destructor_node tld_destructor_node;
typedef void tld_destructor(void* data, int tld);

struct tld_destructor_node
{
	tld_destructor_node* next;
	tld_destructor* destructor;
	int tld;
};

static __libc_lock_t destructor_list_mutex = MUTEX_INITIALIZER;
static tld_destructor_node* destructor_list_head = NULL;

int alloc_tld(void* destructor)
{
  tld_destructor_node *node = NULL, *tail = NULL;
  int tld;

  if(NULL != destructor)
  {
    node = malloc(sizeof(tld_destructor_node));
    if( NULL == node )
      return(-1);
  }
  tld = INLINE_SYSCALL(alloc_tld, 0);
  if( tld < 0 )
  {
    if(NULL != node)
      free(node);
    return(tld);
  }
  else
    set_tld(tld, NULL);

  if(NULL != node)
  {
    int error;
    while((error = __lock_lock(&destructor_list_mutex)) < 0 && error != EINTR);
    if(error < 0)
    {
      free_tld(tld);
      free(node);
      return(error);
    }
    node->tld = tld;
    node->destructor = destructor;
    node->next = NULL;

    if(NULL != destructor_list_head)
    {
      tail = destructor_list_head;
      while(NULL != tail->next)
        tail = tail->next;
      tail->next = node;
    }
    else
      destructor_list_head = node;

    __lock_unlock(&destructor_list_mutex);
  }
  return(tld);
}

/* KV: free_tld should be calling the destructor */
int free_tld(int tld)
{
  int error;

  if(NULL != destructor_list_head)
  {
    while((error = __lock_lock(&destructor_list_mutex)) < 0 && error != EINTR);
    if(error >= 0)
    {
      tld_destructor_node *node, *prev = NULL;
      for(node = destructor_list_head;NULL != node;node = node->next)
      {
        if(tld == node->tld)
	{
          if(NULL != prev)
	  {
            prev->next = node->next;
	  }
          else
	    /* If prev is NULL then node must be the first item in the list E.g. list head */
            destructor_list_head = node->next;

	  free(node);
	  break;
	}
	prev = node;
      }
    }
    __lock_unlock(&destructor_list_mutex);
  }
  error = INLINE_SYSCALL(free_tld, 1, tld);
  return(error);
}

void set_tld(int tld, const void* value)
{
  __asm__("movl %0,%%gs:(%1)" : : "r" ((int)value), "r" (tld) );
}

void* get_tld(int tld)
{
  register int value;
  __asm__("movl %%gs:(%1),%0" : "=r" (value) : "r" (tld) );
  return((void*)value);
}

void * get_tld_addr( int nHandle )
{
  register int *tld_base;

  if ( ( nHandle & 3 ) || nHandle >= TLD_SIZE )
  {
    __set_errno (EINVAL);
    return NULL;
  }

  __asm__( "movl %%gs:(%1),%0" : "=r" (tld_base) : "r" (TLD_BASE) );
  return ( void* )( tld_base + nHandle );
}

