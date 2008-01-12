/* POSIX threads for Syllable - A simple PThreads implementation             */
/*                                                                           */
/* Copyright (C) 2002 Kristian Van Der Vliet (vanders@users.sourceforge.net) */
/* Copyright (C) 1996 Xavier Leroy (Xavier.Leroy@inria.fr)                   */
/*                                                                           */
/* This program is free software; you can redistribute it and/or             */
/* modify it under the terms of the GNU Library General Public License       */
/* as published by the Free Software Foundation; either version 2            */
/* of the License, or (at your option) any later version.                    */
/*                                                                           */
/* This program is distributed in the hope that it will be useful,           */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of            */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             */
/* GNU Library General Public License for more details.                      */

#if !defined __F_ATHEOS_TYPES_H__ && !defined __F_SYLLABLE_PTHREAD_H_
	#error "Never include <atheos/pthreadtypes.h> directly; use <atheos/types.h> instead."
#endif

#ifndef __F_SYLLABLE_PTHREADTYPES_H_
#define __F_SYLLABLE_PTHREADTYPES_H_

#ifdef __cplusplus
extern "C"{
#endif

#include <atheos/semaphore.h>	/* Semaphores and mutexes */
#include <sched.h>				/* struct sched_param */

/* Keep track of the all the data regarding the threads cancellation status */
struct __pthread_cancel_s
{
  int state;
  int type;
  bool cancelled;
};

/* Thread attribute object. */
typedef struct __pthread_attr_s
{
  size_t __stacksize;
  void *__stackaddr;
  size_t __guardsize;
  int __schedpriority;
  char *__name;
  int __detachstate;
  struct sched_param *__sched_param;
  struct __pthread_cancel_s *cancellation;
  bool internal_malloc;
  bool destroyed;
} pthread_attr_t;

/* Thread. */
struct __pthread_s
{
	thread_id id;
	pthread_attr_t *attr;
};
typedef struct __pthread_s * pthread_t;

/* Condition variable attribute object. */
typedef struct
{
  int __pshared;
} pthread_condattr_t;

/* The following is used internally by pthread_cond_t */
typedef struct __pt_thread_list_s
{
	pthread_t __thread_id;
	struct __pt_thread_list_s* __prev;
	struct __pt_thread_list_s* __next;
} __pt_thread_list_t;

/* Condition variable. */
typedef struct __pthread_cond_s
{
  pthread_t __owner;
  unsigned long __count;
  pthread_condattr_t* __attr;
  pthread_condattr_t __def_attr;
  __pt_thread_list_t* __head;
  sem_id __lock;
} pthread_cond_t;

/* Thread-specific data keys. */
typedef int pthread_key_t;

/* Mutex attribute object. */
typedef struct
{
  int __mutexkind;
  int __pshared;
} pthread_mutexattr_t;

/* Mutexes. */
typedef struct __pthread_mutex_s
{
  sem_id __mutex;
  pthread_t __owner;
  unsigned long __count;
  pthread_mutexattr_t *__attr;
  pthread_mutexattr_t __def_attr;
} pthread_mutex_t;

/* Dynamic package initialisation. */
typedef atomic_t pthread_once_t;

/* Read/Write locks. */
typedef int pthread_rwlock_t;

/* Read/Write lock attributes. */
typedef struct
{
  int __lockkind;
  int __pshared;
} pthread_rwlockattr_t;

#ifdef __cplusplus
}
#endif

#endif	/* __F_SYLLABLE_PTHREADTYPES_H_ */

