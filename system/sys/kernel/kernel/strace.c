/*
 *  The AtheOS kernel
 *  Copyright (C) 1999  Kurt Skauen
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

#include <atheos/types.h>
#include <atheos/kernel.h>
#include <atheos/syscall.h>
#include <atheos/smp.h>
#include <posix/signal.h>

#include "inc/scheduler.h"

const char* sys_siglist[] =
{
  "INVALID",
  "HUP",
  "INT",
  "QUIT",
  "ILL",
  "TRAP",
  "ABRT", /* 	"SIGIOT", */
  "BUS",
  "FPE",
  "KILL",
  "USR1",
  "SEGV",
  "USR2",
  "PIPE",
  "ALRM",
  "TERM",
  "STKFLT",
  "CHLD",
  "CONT",
  "STOP",
  "TSTP",
  "TTIN",
  "TTOU",
  "URG",
  "XCPU",
  "XFSZ",
  "VTALRM",
  "PROF",
  "WINCH",
  "IO",
  "PWR",
  "UNUSED1",
  "UNUSED2"
};

/****** exec.library/ *************************************************
 *
 *   NAME
 *
 *   SYNOPSIS
 *
 *   FUNCTION
 *
 *   NOTE
 *
 *   INPUTS
 *
 *   RESULT
 *
 *   SEE ALSO
 *
 ****************************************************************************/

void strsigset( int nSigMask, char* pzBuf )
{
  int	i;

  if ( ~0 == nSigMask )
  {
    strcpy( pzBuf, "ALL_SIGNALS" );
  }
  else
  {
    if ( 0 != nSigMask )
    {
      pzBuf[0] = '\0';
	
      for ( i = 0 ; i < NSIG ; ++i )
      {
	if ( nSigMask & (1L << i) )
	{
	  if ( pzBuf[0] != '\0' ) {
	    strcat( pzBuf, "|" );
	  }
	  if ( (strlen( pzBuf ) + strlen( sys_siglist[ i + 1 ] )) > 480 ) {
	    strcat( pzBuf, " ..." );
	    return;
	  } else {
	    strcat( pzBuf, sys_siglist[ i + 1 ] );
	  }
	}
      }
    }
    else
    {
      strcpy( pzBuf, "0" );
    }
  }
}


void HandleSTrace( int dummy )
{
  Thread_s* psThread = CURRENT_THREAD;

  if ( psThread->tr_nSysTraceLevel > 0 )
  {
    SysCallRegs_s* psRegs = (SysCallRegs_s*) &dummy;

    if ( psThread->tr_nSysTraceLevel < 0  && psRegs->eax >= 0 ) {
      return;
    }

    switch( abs( psThread->tr_nSysTraceLevel ) )
    {
      default:
      {
	switch( psRegs->orig_eax )
	{
/*					
					case __NR_AllocVec:
					printk( "---->>%p = AllocVec(%d, %x)\n", psRegs->eax, psRegs->ebx, psRegs->ecx, psRegs->edx );
					break;
					case __NR_FreeVec:
					printk( "---->>%d = FreeVec(%p)\n", psRegs->eax, psRegs->ebx, psRegs->ecx, psRegs->edx );
					break;
					*/						
	  case __NR_sbrk:
	    printk( "---->>%ld = sbrk(%ld)\n", psRegs->eax, psRegs->ebx );
	    break;
	}
      }
      case 3:
      {
	switch( psRegs->orig_eax )
	{
	  case __NR_read:
	    printk( "---->>%ld = read(%ld, %p, %ld)\n", psRegs->eax, psRegs->ebx, (void*)psRegs->ecx, psRegs->edx );
	    break;
	  case __NR_write:
	    printk( "---->>%ld = write(%ld, %p, %ld)\n", psRegs->eax, psRegs->ebx, (void*)psRegs->ecx, psRegs->edx );
	    break;
	  case __NR_sigaction:
	    printk( "---->>%ld = sigaction(%ld, %p, %p)\n", psRegs->eax, psRegs->ebx, (void*)psRegs->ecx, (void*)psRegs->edx );
	    break;
	}
      }
      case 2:
      {
	switch( psRegs->orig_eax )
	{
//	  case __NR_idle:
//	    break;
	  case __NR_open:
	    printk( "---->>%ld = open( '%s', %lx, %ld )\n", psRegs->eax, (char*)psRegs->ebx, psRegs->ecx, psRegs->edx );
	    break;
	  case __NR_close:
	    printk( "---->>%ld = close( %ld )\n", psRegs->eax, psRegs->ebx );
	    break;
	  case __NR_Fork:
	    printk( "---->>%ld = Fork(%s)\n", psRegs->eax, (char*)psRegs->ebx );
	    break;
	  case __NR_exit:
	    printk( "---->>%ld = exit( %ld )\n", psRegs->eax, psRegs->ebx );
	    break;
	  case __NR_rename:
	    printk( "---->>%ld = rename('%s', '%s')\n", psRegs->eax, (char*)psRegs->ebx, (char*)psRegs->ecx );
	    break;
	  case __NR_getdents:
	    printk( "---->>%ld = getdents(%ld, %p, %ld)\n", psRegs->eax, psRegs->ebx, (void*)psRegs->ecx, psRegs->edx );
	    break;
	  case __NR_alarm:
	    printk( "---->>%ld = alarm(%ld)\n", psRegs->eax, psRegs->ebx );
	    break;
	  case __NR_wait4:
	    printk( "---->>%ld = wait4(%ld, %p, %ld, %p)\n", psRegs->eax, psRegs->ebx, (void*)psRegs->ecx, psRegs->edx, (void*)psRegs->esi );
	    break;
	  case __NR_fstat:
	    printk( "---->>%ld = fstat(%ld, %p)\n", psRegs->eax, psRegs->ebx, (void*)psRegs->ecx );
	    break;
	  case __NR_FileLength:
	    printk( "-*-->>%ld = FileLength()\n", psRegs->eax );
	    break;
	  case __NR_mkdir:
	    printk( "---->>%ld = mkdir('%s', %ld)\n", psRegs->eax, (char*)psRegs->ebx, psRegs->ecx );
	    break;
	  case __NR_rmdir:
	    printk( "---->>%ld = rmdir('%s')\n", psRegs->eax, (char*)psRegs->ebx );
	    break;
	  case __NR_dup:
	    printk( "---->>%ld = dup(%ld)\n", psRegs->eax, psRegs->ebx );
	    break;
	  case __NR_dup2:
	    printk( "---->>%ld = dup2(%ld, %ld)\n", psRegs->eax, psRegs->ebx, psRegs->ecx );
	    break;
	  case __NR_fchdir:
	    printk( "---->>%ld = fchdir(%ld)\n", psRegs->eax, psRegs->ebx );
	    break;
	  case __NR_chdir:
	    printk( "---->>%ld = chdir('%s')\n", psRegs->eax, (char*)psRegs->ebx );
	    break;
	  case __NR_unlink:
	    printk( "---->>%ld = unlink('%s')\n", psRegs->eax, (char*)psRegs->ebx );
	    break;
	  case __NR_get_thread_info:
	    printk( "-*-->>%ld = get_thread_info()\n", psRegs->eax/*, psRegs->ebx, psRegs->ecx, psRegs->edx*/ );
	    break;
	  case __NR_get_thread_proc:
	    printk( "-*-->>%ld = get_thread_proc()\n", psRegs->eax/*, psRegs->ebx, psRegs->ecx, psRegs->edx*/ );
	    break;
	  case __NR_get_next_thread_info:
	    printk( "-*-->>%ld = get_next_thread_info(%ld)\n", psRegs->eax, psRegs->ebx );
	    break;
	  case __NR_get_thread_id:
	    printk( "-*-->>%ld = get_thread_info()\n", psRegs->eax/*, psRegs->ebx, psRegs->ecx, psRegs->edx*/ );
	    break;
	  case __NR_send_data:
	    printk( "-*-->>%ld = send_data()\n", psRegs->eax/*, psRegs->ebx, psRegs->ecx, psRegs->edx*/ );
	    break;
	  case __NR_receive_data:
	    printk( "-*-->>%ld = read_data()\n", psRegs->eax/*, psRegs->ebx, psRegs->ecx, psRegs->edx*/ );
	    break;
	  case __NR_thread_data_size:
	    printk( "-*-->>%ld = thread_data_size()\n", psRegs->eax/*, psRegs->ebx, psRegs->ecx, psRegs->edx*/ );
	    break;
	  case __NR_has_data:
	    printk( "-*-->>%ld = has_data()\n", psRegs->eax/*, psRegs->ebx, psRegs->ecx, psRegs->edx*/ );
	    break;
	  case __NR_SetThreadExitCode:
	    printk( "-*-->>%ld = SetThreadExitCode(%ld)\n", psRegs->eax, psRegs->ebx );
	    break;
	  case __NR_spawn_thread:
	    printk( "-*-->>%ld = spawn_thread()\n", psRegs->eax/*, psRegs->ebx, psRegs->ecx, psRegs->edx*/ );
	    break;
	  case __NR_GetToken:
	    printk( "---->>%ld = GetToken()\n", psRegs->eax );
	    break;
/*						
						case __NR_GetSymAddress:
						printk( "-*-->>%d = GetSymAddress()\n", psRegs->eax, psRegs->ebx, psRegs->ecx, psRegs->edx );
						break;
						case __NR_FindImageSymbol:
						printk( "-*-->>%d = FindImageSymbol()\n", psRegs->eax, psRegs->ebx, psRegs->ecx, psRegs->edx );
						break;
						case __NR_LoadImage:
						printk( "-*-->>%d = LoadImage()\n", psRegs->eax, psRegs->ebx, psRegs->ecx, psRegs->edx );
						break;
						case __NR_CloseImage:
						printk( "-*-->>%d = CloseImage()\n", psRegs->eax, psRegs->ebx, psRegs->ecx, psRegs->edx );
						break;
						*/						
	  case __NR_DebugPrint:
	    break;
	  case __NR_realint:
	    printk( "---->>%ld = realint(%ld, %p)\n", psRegs->eax, psRegs->ebx, (void*)psRegs->ecx );
	    break;
	  case __NR_get_system_path:
	    printk( "---->>%ld = get_system_path( %s, %ld )\n", psRegs->eax, (char*)psRegs->ebx, psRegs->ecx );
	    break;
	  case __NR_get_app_server_port:
	    printk( "---->>%ld = get_app_server_port()\n", psRegs->eax );
	    break;
	  case __NR_create_area:
	    printk( "-*-->>%ld = CreateArea()\n", psRegs->eax/*, psRegs->ebx, psRegs->ecx, psRegs->edx*/ );
	    break;
	  case __NR_remap_area:
	    printk( "-*-->>%ld = RemapArea()\n", psRegs->eax/*, psRegs->ebx, psRegs->ecx, psRegs->edx*/ );
	    break;
	  case __NR_get_area_info:
	    printk( "-*-->>%ld = GetAreaAddress()\n", psRegs->eax/*, psRegs->ebx, psRegs->ecx, psRegs->edx*/ );
	    break;
/*						
	  case __NR_TranslatePortID:
	    printk( "-*-->>%d = TranslatePortID()\n", psRegs->eax, psRegs->ebx, psRegs->ecx, psRegs->edx );
	    break;
	  case __NR_FindThreadPort:
	    printk( "-*-->>%d = FindThreadPort()\n", psRegs->eax, psRegs->ebx, psRegs->ecx, psRegs->edx );
	    break;
						case __NR_SetPortSig:
						printk( "-*-->>%d = SetPortSig()\n", psRegs->eax, psRegs->ebx, psRegs->ecx, psRegs->edx );
						break;
						case __NR_GetPortSig:
						printk( "-*-->>%d = GetPortSig()\n", psRegs->eax, psRegs->ebx, psRegs->ecx, psRegs->edx );
						break;
						*/
						
	  case __NR_create_port:
	    printk( "---->>%ld = create_port( '%s', %ld)\n", psRegs->eax, (char*)psRegs->ebx, psRegs->ecx );
	    break;
	  case __NR_delete_port:
	    printk( "---->>%ld = delete_port( %ld )\n", psRegs->eax, psRegs->ebx );
	    break;
/*	    
	  case __NR_send_msg:
	    printk( "---->>%d = send_msg( %d, %d, %p, %d )\n", psRegs->eax,
		      psRegs->ebx, psRegs->ecx, psRegs->edx, psRegs->esi );
	    break;
	  case __NR_raw_send_msg_x:
	    printk( "---->>%d = raw_send_msg_x( %d, %d, %p, %d, %p )\n", psRegs->eax,
		      psRegs->ebx, psRegs->ecx, psRegs->edx, psRegs->esi, psRegs->edi );
	    break;
	  case __NR_get_msg:
	    if ( 0 != psRegs->ecx ) {
	      printk( "---->>%d = get_msg( %d, %d, %p, %d )\n", psRegs->eax,
			psRegs->ebx, *((uint32*)psRegs->ecx), psRegs->edx, psRegs->esi );
	    } else {
	      printk( "---->>%d = get_msg( %d, %s, %p, %d )\n", psRegs->eax,
			psRegs->ebx, "NULL", psRegs->edx, psRegs->esi );
	    }
	    break;
	  case __NR_raw_get_msg_x:
	    if ( 0 != psRegs->ecx ) {
	      printk( "---->>%d = raw_get_msg_x( %d, %d, %p, %d )\n", psRegs->eax,
			psRegs->ebx, *((uint32*)psRegs->ecx), psRegs->edx, psRegs->esi, psRegs->edi );
	    } else {
	      printk( "---->>%d = raw_get_msg_x( %d, %s, %p, %d )\n", psRegs->eax,
			psRegs->ebx, "NULL", psRegs->edx, psRegs->esi, psRegs->edi );
	    }
	    break;
						
						
	  case __NR_create_semaphore:
	    printk( "-*-->>%d = create_semaphore()\n", psRegs->eax, psRegs->ebx, psRegs->ecx, psRegs->edx );
	    break;
	  case __NR_delete_semaphore:
	    printk( "-*-->>%d = delete_semaphore()\n", psRegs->eax, psRegs->ebx, psRegs->ecx, psRegs->edx );
	    break;
	  case __NR_lock_semaphore:
	    printk( "-*-->>%d = lock_semaphore()\n", psRegs->eax, psRegs->ebx, psRegs->ecx, psRegs->edx );
	    break;
	  case __NR_raw_lock_semaphore_x:
	    printk( "-*-->>%d = raw_lock_semaphore_x()\n", psRegs->eax, psRegs->ebx, psRegs->ecx, psRegs->edx );
	    break;
	  case __NR_unlock_semaphore_x:
	    printk( "-*-->>%d = unlock_semaphore_x()\n", psRegs->eax, psRegs->ebx, psRegs->ecx, psRegs->edx );
	    break;
	  case __NR_get_semaphore_info:
	    printk( "-*-->>%d = get_semaphore_info()\n", psRegs->eax, psRegs->ebx, psRegs->ecx, psRegs->edx );
	    break;
	    */	    
/*						
						case __NR_Exit:
						printk( "---->>%d = Exit(%d)\n", psRegs->eax, psRegs->ebx, psRegs->ecx, psRegs->edx );
						break;
						*/						
	  case __NR_execve:
	    printk( "---->>%ld = execve('%s', %p, %p)\n", psRegs->eax, (char*)psRegs->ebx, (void*)psRegs->ecx, (void*)psRegs->edx );
	    break;
/*	    
	  case __NR_GetProcArgLen:
	    printk( "-*-->>%d = GetProcArgLen()\n", psRegs->eax, psRegs->ebx, psRegs->ecx, psRegs->edx );
	    break;
	  case __NR_GetProcArgs:
	    printk( "-*-->>%d = GetProcArgs()\n", psRegs->eax, psRegs->ebx, psRegs->ecx, psRegs->edx );
	    break;
	  case __NR_SetProcArgs:
	    printk( "-*-->>%d = SetProcArgs()\n", psRegs->eax, psRegs->ebx, psRegs->ecx, psRegs->edx );
	    break;
	    */	    
	  case __NR_GetTime:
	    printk( "-*-->>%ld = GetTime()\n", psRegs->eax/*, psRegs->ebx, psRegs->ecx, psRegs->edx*/ );
	    break;
	  case __NR_SetTime:
	    printk( "-*-->>%ld = SetTime()\n", psRegs->eax/*, psRegs->ebx, psRegs->ecx, psRegs->edx*/ );
	    break;
/*						
						case __NR_AllocSignal:
						break;
						case __NR_FreeSignal:
						break;
						case __NR_WaitSigSet:
						break;
						case __NR_WaitSigNum:
						break;
						case __NR_SendSigNum:
						break;
						*/					
	  case __NR_raw_read_pci_config:
	    printk( "-*-->>%ld = raw_read_pci_config()\n", psRegs->eax/*, psRegs->ebx, psRegs->ecx, psRegs->edx*/ );
	    break;
	  case __NR_raw_write_pci_config:
	    printk( "-*-->>%ld = raw_write_pci_config()\n", psRegs->eax/*, psRegs->ebx, psRegs->ecx, psRegs->edx*/ );
	    break;
	  case __NR_get_pci_info:
	    printk( "-*-->>%ld = get_pci_info()\n", psRegs->eax/*, psRegs->ebx, psRegs->ecx, psRegs->edx*/ );
	    break;
	  case __NR_sig_return:
	    break;
	  case __NR_kill:
	    printk( "---->>%ld = kill(%ld, %ld)\n", psRegs->eax, psRegs->ebx, psRegs->ecx );
	    break;
	  case __NR_sigpending:
	    break;
	  case __NR_sigprocmask:
	  {
	    char*	pzHow;
	    char	zBuf[ 512 ];
						
	    switch ( psRegs->ebx )
	    {
	      case SIG_BLOCK:
		pzHow = "SIG_BLOCK";
		break;
	      case SIG_UNBLOCK:
		pzHow = "SIG_UNBLOCK";
		break;
	      case SIG_SETMASK:
		pzHow = "SIG_SETMASK";
		break;
	      default:
		pzHow = "INVALID";
	    }
	    strsigset( *((uint32*)psRegs->ecx), zBuf );
						
	    printk( "---->>%ld = sigprocmask( %s, %s, %lx )\n", psRegs->eax, pzHow, zBuf, psRegs->edx );
	    break;
	  }
	  case __NR_sigsuspend:
	    break;
	  case __NR_set_thread_priority:
	    printk( "---->>%ld = set_thread_priority(%ld, %ld)\n",
		    psRegs->eax, psRegs->ebx, psRegs->ecx );
	    break;
	  case __NR_suspend_thread:
	  {
	    char* pzName;
	    Thread_s* psThread = get_thread_by_handle( psRegs->eax );

	    if ( NULL != psThread ) {
	      pzName = psThread->tr_zName;
	    } else {
	      pzName = "INVALID";
	    }
						
	    printk( "---->>%ld = suspend_thread(%ld (%s))\n", psRegs->eax, psRegs->ebx, pzName );
	    break;
	  }
	  case __NR_resume_thread:
	  {
	    char* pzName;
	    Thread_s* psThread = get_thread_by_handle( psRegs->eax );
	    if ( NULL != psThread ) {
	      pzName = psThread->tr_zName;
	    } else {
	      pzName = "INVALID";
	    }
						
	    printk( "---->>%ld = resume_thread(%ld (%s))\n", psRegs->eax, psRegs->ebx, pzName );
	    break;
	  }
	  case __NR_wait_for_thread:
	    printk( "---->>%ld = wait_for_thread(%ld)\n", psRegs->eax, psRegs->ebx );
	    break;
/*						
	  case __NR_Forbid:
	    printk( "---->>%d = Forbid()\n", psRegs->eax, psRegs->ebx, psRegs->ecx, psRegs->edx );
	    break;
	  case __NR_Permit:
	    printk( "---->>%d = Permit()\n", psRegs->eax, psRegs->ebx, psRegs->ecx, psRegs->edx );
	    break;
						case __NR_Disable:
						printk( "---->>%d = Disable()\n", psRegs->eax, psRegs->ebx, psRegs->ecx, psRegs->edx );
						break;
						case __NR_Enable:
						printk( "---->>%d = Enable()\n", psRegs->eax, psRegs->ebx, psRegs->ecx, psRegs->edx );
						break;
						*/						
	  case __NR_get_process_id:
	    printk( "-*-->>%ld = GetProcID()\n", psRegs->eax/*, psRegs->ebx, psRegs->ecx, psRegs->edx*/ );
	    break;
	  case __NR_isatty:
	    printk( "---->>%ld = isatty(%ld)\n", psRegs->eax, psRegs->ebx );
	    break;
	  case __NR_fcntl:
	    printk( "---->>%ld = fcntl(%ld, %ld, %ld)\n", psRegs->eax, psRegs->ebx, psRegs->ecx, psRegs->edx );
	    break;
	  case __NR_ioctl:
	    printk( "---->>%ld = ioctl(%ld, %ld, %ld)\n", psRegs->eax, psRegs->ebx, psRegs->ecx, psRegs->edx );
	    break;
	  case __NR_pipe:
	    printk( "---->>%ld = pipe(%p)\n", psRegs->eax, (void*)psRegs->ebx );
	    break;
	  case __NR_access:
	    printk( "---->>%ld = access('%s', %ld)\n", psRegs->eax, (char*)psRegs->ebx, psRegs->ecx );
	    break;
/*
  default:
  printk( "---->> ERROR : Unknown syscall %d returned %d\n", psRegs->orig_eax, psRegs->eax );
  break;
  */
	}
	break;
      }
    }
  }
}
