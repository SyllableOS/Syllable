/*
 *  The Syllable kernel
 *  Copyright (C) 2004 Kristian Van Der Vliet
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

#include <atheos/kernel.h>
#include <atheos/kdebug.h>
#include <atheos/types.h>
#include <posix/errno.h>

/* This is just a syscall stub until we implement proper ptrace() functionality,
   but having a stub allows us to implement the proper Glibc functionality instead
   of having make the modifications in the future, which is a drag.  This also
   makes it far easier for anyone to just hack up a working ptrace() without having
   to mess with Glibc (Hint hint) */
int sys_ptrace(int nRequest, pid_t nProcess, void *pAddr, void *pData)
{
	printk( "%s : Stub.\n", __FUNCTION__ );
	return -ENOSYS;
}

