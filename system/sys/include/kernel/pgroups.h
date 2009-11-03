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

#ifndef __F_KERNEL_PGROUPS_H__
#define __F_KERNEL_PGROUPS_H__

#include <kernel/types.h>

int handle_signals( int dummy );
int is_orphaned_pgrp( int nProcessGroup );

int	sys_killpg( const pid_t nGrp, const int nSigNum );
int	sys_kill_proc( proc_id hProcess, int nSigNum );

int	killpg( pid_t nGrp, int nSigNum );
int	kill_proc( proc_id hProcess, int nSigNum );

int setpgid( pid_t a_hDest, pid_t a_hGroup );
int getpgid( pid_t hPid );
pid_t getpgrp(void);
pid_t getppid(void);
int getsid( pid_t hPid );
int setsid(void);
gid_t getegid(void);
uid_t geteuid(void);
gid_t getfsgid(void);
uid_t getfsuid(void);
gid_t getgid(void);
int getgroups(int _size, gid_t *grouplist);
uid_t getuid(void);
int setgid(gid_t _gid);
int setuid( uid_t uid );

#endif	/* __F_KERNEL_PGROUPS_H__ */
