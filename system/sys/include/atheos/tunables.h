
/*
 *  The AtheOS kernel
 *  Copyright (C) 2005 The Syllable Team
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

#ifndef __ATHEOS_TUNABLES_H__
#define __ATHEOS_TUNABLES_H__

/// Define the maximum number of CPUs supported.
#define MAX_CPU_COUNT			16

/// Maximum string lengths excluding (?) NULL terminator.
#define MAX_BUSMANAGER_NAME_LENGTH	255	///< should be 256
#define MAX_DEVICE_NAME_LENGTH		255	///< should be 256

/// This is the maximum length for certain strings stored within the kernel.
#define	OS_NAME_LENGTH	64

/// This is the maximum 
#define MAX_KERNEL_ARGS	128

/// Default debug level (KERN_DEBUG, KERN_INFO, KERN_WARNING, KERN_FATAL).
#define DEBUG_LIMIT	KERN_INFO	///< Debug level (see <atheos/kdebug.h>)

/// Kernel load address is currently @ 1MB  -- will soon be 0xfe800000
#define KERNEL_LOAD_ADDR 0x100000

/// Kernel address space goes from 1MB-2GB.  The kernel will soon move to
/// the 0xf0000000 - 0xffffffff region leaving the lower 3.75GB for users.
#define	AREA_FIRST_KERNEL_ADDRESS	0x00100000
#define	AREA_LAST_KERNEL_ADDRESS	0x7fffffff

/// User program address space is currently upper 2GB.  Will soon be moved to
/// the 0x00000000 - 0xf0000000 region leaving 256MB for the kernel.
#define	AREA_FIRST_USER_ADDRESS		0x80000000
#define AREA_LAST_USER_ADDRESS		0xffffffff

/// Dual 8259A configuration has 16 interrupts, but APIC allows for more.
#define	IRQ_COUNT	16

#endif	// __ATHEOS_TUNABLES_H__
