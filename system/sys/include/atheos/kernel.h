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

#ifndef __F_SYLLABLE_KERNEL_H__
#define __F_SYLLABLE_KERNEL_H__

#ifdef __KERNEL__
# error "kernel.h is deprecated."
#else
# warning "kernel.h is deprecated."
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* The user-space parts of kernel.h has moved to one of the following headers */
#include <syllable/sysinfo.h>
#include <syllable/threads.h>
#include <syllable/areas.h>
#include <syllable/kdebug.h>
#include <syllable/msgport.h>
#include <syllable/filesystem.h>
#include <syllable/power.h>
#include <syllable/v86.h>
#include <syllable/time.h>

#ifdef __cplusplus
}
#endif

#endif	/* __F_SYLLABLE_KERNEL_H__ */
