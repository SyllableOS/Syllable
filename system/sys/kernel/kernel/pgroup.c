
/*
 *  The AtheOS kernel
 *  Copyright (C) 1999 - 2001 Kurt Skauen
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

#include <posix/errno.h>
#include <atheos/types.h>
#include <atheos/kernel.h>
#include <atheos/smp.h>
#include <atheos/irq.h>

#include "inc/scheduler.h"
#include "inc/sysbase.h"

#include <macros.h>

bool is_root( bool bFromFS )
{
	Process_s *psProc = CURRENT_PROC;

	if ( g_sSysBase.ex_bSingleUserMode || ( ( bFromFS ) ? psProc->pr_nFSUID == 0 : psProc->pr_nEUID == 0 ) )
	{
		psProc->pr_bUsedPriv = true;
		return ( true );
	}
	else
	{
		return ( false );
	}
}

bool is_in_group( gid_t grp )
{
	Process_s *psProc = CURRENT_PROC;

	if ( grp != psProc->pr_nFSGID )
	{
		int i = psProc->pr_nNumGroups;

		if ( i )
		{
			gid_t *groups = psProc->pr_anGroups;

			do
			{
				if ( *groups == grp )
					goto out;
				groups++;
				i--;
			}
			while ( i );
		}
		return ( false );
	}
      out:
	return ( true );;
}


/*
 * Unprivileged users may change the real gid to the effective gid
 * or vice versa.  (BSD-style)
 *
 * If you set the real gid at all, or set the effective gid to a value not
 * equal to the real gid, then the saved gid is set to the new effective gid.
 *
 * This makes it possible for a setgid program to completely drop its
 * privileges, which is often a useful assertion to make when you are doing
 * a security audit over a program.
 *
 * The general idea is that a program which uses just setregid() will be
 * 100% compatible with BSD.  A program which uses just setgid() will be
 * 100% compatible with POSIX with saved IDs. 
 *
 * SMP: There are not races, the GIDs are checked only by filesystem
 *      operations (as far as semantic preservation is concerned).
 */
int sys_setregid( gid_t rgid, gid_t egid )
{
	Process_s *psProc = CURRENT_PROC;
	int old_rgid = psProc->pr_nGID;
	int old_egid = psProc->pr_nEGID;

	if ( rgid != ( gid_t )-1 )
	{
		if ( ( old_rgid == rgid ) || ( psProc->pr_nEGID == rgid ) || is_root( false ) )
		{
			psProc->pr_nGID = rgid;
		}
		else
		{
			return -EPERM;
		}
	}
	if ( egid != ( gid_t )-1 )
	{
		if ( ( old_rgid == egid ) || ( psProc->pr_nEGID == egid ) || ( psProc->pr_nSGID == egid ) || is_root( false ) )
			psProc->pr_nFSGID = psProc->pr_nEGID = egid;
		else
		{
			psProc->pr_nGID = old_rgid;
			return -EPERM;
		}
	}
	if ( rgid != ( gid_t )-1 || ( egid != ( gid_t )-1 && egid != old_rgid ) )
	{
		psProc->pr_nSGID = psProc->pr_nEGID;
	}
	psProc->pr_nFSGID = psProc->pr_nEGID;
	if ( psProc->pr_nEGID != old_egid )
	{
		psProc->pr_bCanDump = false;
	}
	return ( 0 );
}


int sys_setpgid( pid_t a_hDest, pid_t a_hGroup )
{
	Thread_s *psThisThread = CURRENT_THREAD;
	Thread_s *psDstThread;
	Process_s *psThisProc;
	Process_s *psDstProc;
	pid_t hDest;
	pid_t hGroup;

	hDest = ( 0 == a_hDest ) ? psThisThread->tr_hThreadID : a_hDest;
	hGroup = ( 0 == a_hGroup ) ? hDest : a_hGroup;

	if ( hGroup < 0 )
	{
		return ( -EINVAL );
	}

	psDstThread = get_thread_by_handle( hDest );

	if ( NULL == psDstThread )
	{
		printk( "setpgid() dest thread %d not found, or %x not child of %x\n", psDstThread->tr_hThreadID, psDstThread->tr_hThreadID, psThisThread->tr_hThreadID );
		return ( -ESRCH );
	}

	psThisProc = psThisThread->tr_psProcess;
	psDstProc = psDstThread->tr_psProcess;

	if ( psDstThread->tr_hParent == psThisThread->tr_hThreadID )
	{
		if ( psDstThread->tr_bHasExeced )
		{
			return ( -EACCES );
		}
		if ( psDstProc->pr_nSession != psThisProc->pr_nSession )
		{
			return ( -EPERM );
		}
	}
	else
	{
		if ( psDstThread != psThisThread )
		{
			printk( "setpgid() or %x not same as %x\n", psDstThread->tr_hThreadID, psThisThread->tr_hThreadID );
			return ( -ESRCH );
		}
	}

	if ( psDstProc->pr_bIsGroupLeader )
	{
		return ( -EPERM );
	}
	if ( hGroup != hDest )
	{
		Thread_s *psTmp;
		thread_id hTmp;
		bool bGroupOK = false;
		int nFlg;

		// FIXME: Traverse proc's instead of threads (it is faster)

		nFlg = cli();
		sched_lock();
		FOR_EACH_THREAD( hTmp, psTmp )
		{
			if ( psTmp->tr_psProcess->pr_hPGroupID == hGroup && psTmp->tr_psProcess->pr_nSession == psThisProc->pr_nSession )
			{
				bGroupOK = true;
				break;
			}
		}
		sched_unlock();
		put_cpu_flags( nFlg );

		if ( false == bGroupOK )
		{
			return ( -EPERM );
		}
	}
	psDstProc->pr_hPGroupID = hGroup;
	return ( 0 );
}

int setpgid( pid_t a_hDest, pid_t a_hGroup )
{
	return ( sys_setpgid( a_hDest, a_hGroup ) );
}

int sys_getpgid( pid_t hPid )
{
	Thread_s *psThread;

	if ( 0 == hPid )
	{
		psThread = CURRENT_THREAD;
	}
	else
	{
		psThread = get_thread_by_handle( hPid );
	}

	if ( NULL != psThread )
	{
		return ( psThread->tr_psProcess->pr_hPGroupID );
	}
	else
	{
		return -ESRCH;
	}
}

int getpgid( pid_t hPid )
{
	return ( sys_getpgid( hPid ) );
}

pid_t sys_getpgrp( void )
{
	return ( CURRENT_PROC->pr_hPGroupID );
}

pid_t getpgrp( void )
{
	return ( sys_getpgrp() );
}

pid_t sys_getppid( void )
{
	if ( CURRENT_THREAD->tr_hRealParent == 0 )
		return ( CURRENT_THREAD->tr_hParent );
	else
		return ( CURRENT_THREAD->tr_hRealParent );
}

pid_t getppid( void )
{
	return ( sys_getppid() );
}

int sys_getsid( pid_t hPid )
{
	Thread_s *psThread;

	if ( 0 == hPid )
	{
		psThread = CURRENT_THREAD;
	}
	else
	{
		psThread = get_thread_by_handle( hPid );
	}

	if ( NULL != psThread )
	{
		return ( psThread->tr_psProcess->pr_nSession );
	}
	else
	{
		return -ESRCH;
	}
}

int getsid( pid_t hPid )
{
	return ( sys_getsid( hPid ) );
}

int sys_setsid( void )
{
	Thread_s *psThisThread = CURRENT_THREAD;
	Process_s *psThisProc = CURRENT_PROC;
	Thread_s *psTmp;
	thread_id hTmp;
	int nFlg;

      /*** Make sure the process group we are about to make don't already exist ***/

	nFlg = cli();
	sched_lock();

	FOR_EACH_THREAD( hTmp, psTmp )
	{
		if ( psTmp->tr_psProcess->pr_hPGroupID == psThisThread->tr_hThreadID )
		{
			sched_unlock();
			put_cpu_flags( nFlg );
			return ( -EPERM );
		}
	}
	psThisProc->pr_bIsGroupLeader = true;
	psThisProc->pr_nSession = psThisProc->pr_hPGroupID = psThisThread->tr_hThreadID;
	sched_unlock();
	put_cpu_flags( nFlg );


	clear_ctty();
	psThisProc->pr_nOldTTYPgrp = 0;

	return ( psThisProc->pr_hPGroupID );
}

int setsid( void )
{
	return ( sys_setsid() );
}

gid_t sys_getegid( void )
{
	return ( CURRENT_PROC->pr_nEGID );
}

gid_t getegid( void )
{
	return ( sys_getegid() );
}

uid_t sys_geteuid( void )
{
	return ( CURRENT_PROC->pr_nEUID );
}

uid_t geteuid( void )
{
	return ( sys_geteuid() );
}

gid_t sys_getgid( void )
{
	return ( CURRENT_PROC->pr_nGID );
}

gid_t getgid( void )
{
	return ( sys_getgid() );
}

uid_t sys_getuid( void )
{
	return ( CURRENT_PROC->pr_nUID );
}

uid_t getuid( void )
{
	return ( sys_getuid() );
}

gid_t getfsgid( void )
{
	return ( CURRENT_PROC->pr_nFSGID );
}

uid_t getfsuid( void )
{
	return ( CURRENT_PROC->pr_nFSUID );
}


/*
 * Supplementary group IDs
 */
int sys_getgroups( int gidsetsize, gid_t *grouplist )
{
	Process_s *psProc = CURRENT_PROC;
	int i;

	/*
	 *        SMP: Nobody else can change our grouplist. Thus we are
	 *        safe.
	 */

	if ( gidsetsize < 0 )
		return -EINVAL;
	i = psProc->pr_nNumGroups;

	if ( gidsetsize )
	{
		if ( i > gidsetsize )
			return -EINVAL;
		if ( memcpy_to_user( grouplist, psProc->pr_anGroups, sizeof( gid_t ) * i ) )
			return -EFAULT;
	}
	return i;
}

int getgroups( int gidsetsize, gid_t *grouplist )
{
	return ( sys_getgroups( gidsetsize, grouplist ) );
}

/*
 *	SMP: Our groups are not shared. We can copy to/from them safely
 *	without another task interfering.
 */

int sys_setgroups( int gidsetsize, gid_t *grouplist )
{
	Process_s *psProc = CURRENT_PROC;

	if ( !is_root( false ) )
		return -EPERM;
	if ( ( unsigned )gidsetsize > NGROUPS )
		return -EINVAL;
	if( gidsetsize > 0 )
	{
		if ( memcpy_from_user( psProc->pr_anGroups, grouplist, gidsetsize * sizeof( gid_t ) ) )
			return -EFAULT;

		psProc->pr_nNumGroups = gidsetsize;
	}
	return 0;
}


/*
  
int sys_getgroups(int _size, gid_t *grouplist)
{
  return 0;
}

static char default_login[] = "user";

char* getlogin(void)
{
  char *p;

  p = getenv("USER");
  if (!p)
    p = getenv("LOGNAME");
  if (!p)
    p = default_login;
  return p;
}
*/



/*
 * setgid() is implemented like SysV w/ SAVED_IDS 
 *
 * SMP: Same implicit races as above.
 */

int sys_setgid( gid_t gid )
{
	Process_s *psProc = CURRENT_PROC;

	int old_egid = psProc->pr_nEGID;

	if ( is_root( false ) )
		psProc->pr_nGID = psProc->pr_nEGID = psProc->pr_nSGID = psProc->pr_nFSGID = gid;
	else if ( ( gid == psProc->pr_nGID ) || ( gid == psProc->pr_nSGID ) )
		psProc->pr_nEGID = psProc->pr_nFSGID = gid;
	else
		return -EPERM;

	if ( psProc->pr_nEGID != old_egid )
	{
		psProc->pr_bCanDump = false;
	}
	return 0;
}

int setgid( gid_t gid )
{
	return ( sys_setgid( gid ) );
}

/*
 * Unprivileged users may change the real uid to the effective uid
 * or vice versa.  (BSD-style)
 *
 * If you set the real uid at all, or set the effective uid to a value not
 * equal to the real uid, then the saved uid is set to the new effective uid.
 *
 * This makes it possible for a setuid program to completely drop its
 * privileges, which is often a useful assertion to make when you are doing
 * a security audit over a program.
 *
 * The general idea is that a program which uses just setreuid() will be
 * 100% compatible with BSD.  A program which uses just setuid() will be
 * 100% compatible with POSIX with saved IDs. 
 */
int sys_setreuid( uid_t ruid, uid_t euid )
{
	Process_s *psProc = CURRENT_PROC;
	int old_ruid, old_euid, old_suid, new_ruid;

	new_ruid = old_ruid = psProc->pr_nUID;
	old_euid = psProc->pr_nEUID;
	old_suid = psProc->pr_nSUID;

	if ( ruid != ( uid_t )-1 )
	{
		if ( ( old_ruid == ruid ) || ( psProc->pr_nEUID == ruid ) || is_root( false ) )
			new_ruid = ruid;
		else
			return -EPERM;
	}
	if ( euid != ( uid_t )-1 )
	{
		if ( ( old_ruid == euid ) || ( psProc->pr_nEUID == euid ) || ( psProc->pr_nSUID == euid ) || is_root( false ) )
			psProc->pr_nFSUID = psProc->pr_nEUID = euid;
		else
			return -EPERM;
	}
	if ( ruid != ( uid_t )-1 || ( euid != ( uid_t )-1 && euid != old_ruid ) )
		psProc->pr_nSUID = psProc->pr_nEUID;
	psProc->pr_nFSUID = psProc->pr_nEUID;

	if ( psProc->pr_nEUID != old_euid )
	{
		psProc->pr_bCanDump = false;
	}

	if ( new_ruid != old_ruid )
	{
		/* What if a process setreuid()'s and this brings the
		 * new uid over his NPROC rlimit?  We can check this now
		 * cheaply with the new uid cache, so if it matters
		 * we should be checking for it.  -DaveM
		 */
//    free_uid(current);
		psProc->pr_nUID = new_ruid;
//    alloc_uid(current);
	}

//  if (!issecure(SECURE_NO_SETUID_FIXUP)) {
//    cap_emulate_setxuid(old_ruid, old_euid, old_suid);
//  }

	return 0;
}


/*
 * setuid() is implemented like SysV with SAVED_IDS 
 * 
 * Note that SAVED_ID's is deficient in that a setuid root program
 * like sendmail, for example, cannot set its uid to be a normal 
 * user and then switch back, because if you're root, setuid() sets
 * the saved uid too.  If you don't like this, blame the bright people
 * in the POSIX committee and/or USG.  Note that the BSD-style setreuid()
 * will allow a root program to temporarily drop privileges and be able to
 * regain them by swapping the real and effective uid.  
 */

int sys_setuid( uid_t uid )
{
	Process_s *psProc = CURRENT_PROC;

	int old_euid = psProc->pr_nEUID;
	int old_ruid, old_suid, new_ruid;

	old_ruid = new_ruid = psProc->pr_nUID;
	old_suid = psProc->pr_nSUID;
	if ( is_root( false ) )
		new_ruid = psProc->pr_nEUID = psProc->pr_nSUID = psProc->pr_nFSUID = uid;
	else if ( ( uid == psProc->pr_nUID ) || ( uid == psProc->pr_nSUID ) )
		psProc->pr_nFSUID = psProc->pr_nEUID = uid;
	else
		return -EPERM;

	if ( psProc->pr_nEUID != old_euid )
	{
		psProc->pr_bCanDump = false;
	}

	if ( new_ruid != old_ruid )
	{
		/* See comment above about NPROC rlimit issues... */
//    free_uid(current);
		psProc->pr_nUID = new_ruid;
//    alloc_uid(current);
	}

//  if (!issecure(SECURE_NO_SETUID_FIXUP)) {
//    cap_emulate_setxuid(old_ruid, old_euid, old_suid);
//  }

	return 0;
}

int setuid( uid_t uid )
{
	return ( sys_setuid( uid ) );
}


/*
 * This function implements a generic ability to update ruid, euid,
 * and suid.  This allows you to implement the 4.4 compatible seteuid().
 */

int sys_setresuid( uid_t ruid, uid_t euid, uid_t suid )
{
	Process_s *psProc = CURRENT_PROC;

//  int old_ruid = psProc->pr_nUID;
//  int old_euid = psProc->pr_nEUID;
//  int old_suid = psProc->pr_nSUID;

	if ( !is_root( false ) )
	{
		if ( ( ruid != ( uid_t )-1 ) && ( ruid != psProc->pr_nUID ) && ( ruid != psProc->pr_nEUID ) && ( ruid != psProc->pr_nSUID ) )
			return -EPERM;
		if ( ( euid != ( uid_t )-1 ) && ( euid != psProc->pr_nUID ) && ( euid != psProc->pr_nEUID ) && ( euid != psProc->pr_nSUID ) )
			return -EPERM;
		if ( ( suid != ( uid_t )-1 ) && ( suid != psProc->pr_nUID ) && ( suid != psProc->pr_nEUID ) && ( suid != psProc->pr_nSUID ) )
			return -EPERM;
	}
	if ( ruid != ( uid_t )-1 )
	{
		/* See above commentary about NPROC rlimit issues here. */
//    free_uid(current);
		psProc->pr_nUID = ruid;
//    alloc_uid(current);
	}
	if ( euid != ( uid_t )-1 )
	{
		if ( euid != psProc->pr_nEUID )
		{
			psProc->pr_bCanDump = false;
		}
		psProc->pr_nEUID = euid;
		psProc->pr_nFSUID = euid;
	}
	if ( suid != ( uid_t )-1 )
		psProc->pr_nSUID = suid;

//  if (!issecure(SECURE_NO_SETUID_FIXUP)) {
//    cap_emulate_setxuid(old_ruid, old_euid, old_suid);
//  }

	return 0;
}

int sys_getresuid( uid_t *ruid, uid_t *euid, uid_t *suid )
{
	Process_s *psProc = CURRENT_PROC;
	int retval;

	if ( !( retval = memcpy_to_user( ruid, &psProc->pr_nUID, sizeof( psProc->pr_nUID ) ) ) && !( retval = memcpy_to_user( euid, &psProc->pr_nEUID, sizeof( psProc->pr_nEUID ) ) ) )
		retval = memcpy_to_user( suid, &psProc->pr_nSUID, sizeof( psProc->pr_nSUID ) );

	return retval;
}

/*
 * Same as above, but for rgid, egid, sgid.
 */
int sys_setresgid( gid_t rgid, gid_t egid, gid_t sgid )
{
	Process_s *psProc = CURRENT_PROC;

	if ( !is_root( false ) )
	{
		if ( ( rgid != ( gid_t )-1 ) && ( rgid != psProc->pr_nGID ) && ( rgid != psProc->pr_nEGID ) && ( rgid != psProc->pr_nSGID ) )
			return -EPERM;
		if ( ( egid != ( gid_t )-1 ) && ( egid != psProc->pr_nGID ) && ( egid != psProc->pr_nEGID ) && ( egid != psProc->pr_nSGID ) )
			return -EPERM;
		if ( ( sgid != ( gid_t )-1 ) && ( sgid != psProc->pr_nGID ) && ( sgid != psProc->pr_nEGID ) && ( sgid != psProc->pr_nSGID ) )
			return -EPERM;
	}
	if ( rgid != ( gid_t )-1 )
		psProc->pr_nGID = rgid;
	if ( egid != ( gid_t )-1 )
	{
		if ( egid != psProc->pr_nEGID )
		{
			psProc->pr_bCanDump = false;
		}
		psProc->pr_nEGID = egid;
		psProc->pr_nFSGID = egid;
	}
	if ( sgid != ( gid_t )-1 )
		psProc->pr_nSGID = sgid;
	return 0;
}

int sys_getresgid( gid_t *rgid, gid_t *egid, gid_t *sgid )
{
	Process_s *psProc = CURRENT_PROC;
	int retval;

	if ( !( retval = memcpy_to_user( rgid, &psProc->pr_nGID, sizeof( psProc->pr_nGID ) ) ) && !( retval = memcpy_to_user( egid, &psProc->pr_nEGID, sizeof( psProc->pr_nEGID ) ) ) )
		retval = memcpy_to_user( sgid, &psProc->pr_nSGID, sizeof( psProc->pr_nSGID ) );

	return retval;
}


/*
 * "setfsuid()" sets the fsuid - the uid used for filesystem checks. This
 * is used for "access()" and for the NFS daemon (letting nfsd stay at
 * whatever uid it wants to). It normally shadows "euid", except when
 * explicitly set by setfsuid() or for access..
 */
int sys_setfsuid( uid_t uid )
{
	Process_s *psProc = CURRENT_PROC;

	int old_fsuid;

	old_fsuid = psProc->pr_nFSUID;
	if ( uid == psProc->pr_nUID || uid == psProc->pr_nEUID || uid == psProc->pr_nSUID || uid == psProc->pr_nFSUID || is_root( false ) )
		psProc->pr_nFSUID = uid;
	if ( psProc->pr_nFSUID != old_fsuid )
	{
		psProc->pr_bCanDump = false;
	}

	/* We emulate fsuid by essentially doing a scaled-down version
	 * of what we did in setresuid and friends. However, we only
	 * operate on the fs-specific bits of the process' effective
	 * capabilities 
	 *
	 * FIXME - is fsuser used for all CAP_FS_MASK capabilities?
	 *          if not, we might be a bit too harsh here.
	 */

/*	
	if (!issecure(SECURE_NO_SETUID_FIXUP)) {
	if (old_fsuid == 0 && current->fsuid != 0) {
	cap_t(current->cap_effective) &= ~CAP_FS_MASK;
	}
	if (old_fsuid != 0 && current->fsuid == 0) {
	cap_t(current->cap_effective) |=
	(cap_t(current->cap_permitted) & CAP_FS_MASK);
	}
	}
	*/
	return old_fsuid;
}

int sys_setfsgid( gid_t gid )
{
	Process_s *psProc = CURRENT_PROC;

	int old_fsgid;

	old_fsgid = psProc->pr_nFSGID;
	if ( gid == psProc->pr_nGID || gid == psProc->pr_nEGID || gid == psProc->pr_nSGID || gid == psProc->pr_nFSGID || is_root( false ) )
		psProc->pr_nFSGID = gid;
	if ( psProc->pr_nFSGID != old_fsgid )
	{
		psProc->pr_bCanDump = false;
	}

	return old_fsgid;
}
