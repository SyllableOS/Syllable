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
#include <atheos/kernel.h>
#include <atheos/irq.h>

#include <macros.h>

#include "inc/scheduler.h"
#include "inc/areas.h"
#include "inc/sysbase.h"
#include "inc/mman.h"
#include "inc/smp.h"


/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

static void format_eflags( uint32 nFlags, char* pzBuffer )
{
  pzBuffer[0] = '\0';

  if ( nFlags & EFLG_CARRY )	 strcat( pzBuffer, "CF " );
  if ( nFlags & EFLG_PARITY )	 strcat( pzBuffer, "PF " );
  if ( nFlags & EFLG_AUX_CARRY ) strcat( pzBuffer, "AF " );
  if ( nFlags & EFLG_ZERO )	 strcat( pzBuffer, "ZF " );
  if ( nFlags & EFLG_SIGN )	 strcat( pzBuffer, "SF " );
  if ( nFlags & EFLG_TRAP )	 strcat( pzBuffer, "TF " );
  if ( nFlags & EFLG_IF )	 strcat( pzBuffer, "IF " );
  if ( nFlags & EFLG_DF )	 strcat( pzBuffer, "DF " );
  if ( nFlags & EFLG_OF )	 strcat( pzBuffer, "OF " );
  if ( nFlags & EFLG_NT )	 strcat( pzBuffer, "NT " );
  if ( nFlags & EFLG_RESUME )	 strcat( pzBuffer, "RF " );
  if ( nFlags & EFLG_VM )	 strcat( pzBuffer, "VM " );
  if ( nFlags & EFLG_AC )	 strcat( pzBuffer, "AC " );
  if ( nFlags & EFLG_VIF )	 strcat( pzBuffer, "VIF " );
  if ( nFlags & EFLG_VIP )	 strcat( pzBuffer, "VIP " );
  if ( nFlags & EFLG_ID )	 strcat( pzBuffer, "ID " );
	
/*	if ( nFlags & EFLG_IOPL )				strcat( pzBuffer, " " ); */
	
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

void print_symbol( int nIndex, uint32 nAddress )
{
    int nModule;
    char zSymName[256];
    char zModuleName[64];
    void* pSymAddr = NULL;
    uint32	nTxtAddr = 0;
    
    printk( "%d -> %08lx\n", nIndex, nAddress );

    nModule = find_module_by_address( (void*)nAddress );
    if ( nModule >= 0 ) {
	image_info sInfo;
	if ( get_image_info( nAddress < AREA_FIRST_USER_ADDRESS, nModule, -1, &sInfo ) >= 0 ) {
	    strncpy( zModuleName, sInfo.ii_name, 64 );
	    zModuleName[63] = '\0';
	    nTxtAddr = (uint32)sInfo.ii_text_addr;
	} else {
	    strcpy( zModuleName, "*unknown*" );
	}
	if ( get_symbol_by_address( nModule, (const char*) nAddress, zSymName, 256, &pSymAddr ) < 0 ) {
	    strcpy( zSymName, "*unknown*" );
	}
    } else {
	strcpy( zModuleName, "*unknown*" );
	strcpy( zSymName, "*unknown*" );
    }
    printk( "   %s + %08lx -> %s + %08lx\n", zModuleName, nAddress - nTxtAddr, zSymName, nAddress - (uint32)pSymAddr );
}

void trace_stack( uint32 nEIP, uint32* pStack )
{
  int i;

  if ( pStack == NULL ) {
      __asm__("movl %%ebp,%0":"=r" (pStack) );
  } else {
      print_symbol( 0, nEIP );
  }
  for ( i = 0 ; i < 100 ; ++i )
  {
    MemArea_s* psArea;

    if ( ((uint32)pStack) >= 1024*1024 && ((uint32)pStack + 8) < (g_sSysBase.ex_nTotalPageCount * PAGE_SIZE) ) {
      psArea = NULL;
    } else {
      psArea = verify_area( pStack, 8, false );
      if ( psArea == NULL ) {
	return;
      }
    }
    print_symbol( i + 1, pStack[1] );
    pStack = (uint32*) pStack[0];
    if ( psArea != NULL ) {
      put_area( psArea );
    }
  }
}


/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

void print_registers( SysCallRegs_s* psRegs )
{
  char	zBuffer[128];
  int	nFlg = cli();
  format_eflags( psRegs->eflags, zBuffer );
	
  printk( "EAX = %08lx : EBX = %08lx : ECX = %08lx : EDX = %08lx\n", psRegs->eax, psRegs->ebx, psRegs->ecx, psRegs->edx );
  printk( "ESI = %08lx : EDI = %08lx : EBP = %08lx\n", psRegs->esi, psRegs->edi, psRegs->ebp );
  printk( "SS::ESP = %04x::%08lx\n", psRegs->oldss, psRegs->oldesp );
  printk( "CS::EIP = %04x::%08lx\n", psRegs->cs, psRegs->eip );
  printk( "DS = %04x : ES = %04x : FS = %04x : GS = %04x\n", psRegs->ds, psRegs->es, psRegs->fs, psRegs->gs );
  printk( "EFLAGS = %08lx (%s)\n", psRegs->eflags, zBuffer );
  printk( "CPU ID = %d : kerner-stack = %08lx\n", get_processor_id(), (uint32)CURRENT_THREAD->tc_plKStack );

  trace_stack( psRegs->eip, (uint32*)psRegs->ebp );
  
  put_cpu_flags( nFlg );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

void handle_general_protection( SysCallRegs_s* psRegs, int nErrorCode )
{
  if ( psRegs->eflags & EFLG_VM ) {
    handle_v86_fault( (Virtual86Regs_s*) psRegs, nErrorCode );
    return;
  }
	
  printk( "**general protection fault**\n" );
  printk( "ERROR CODE = %08x\n", nErrorCode );
  print_registers( psRegs );
  printk( "\n" );

  do_exit( 12 << 8 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

void handle_divide_exception( SysCallRegs_s* psRegs, int nErrorCode )
{
  if ( psRegs->eflags & EFLG_VM ) {
    handle_v86_divide_exception( (Virtual86Regs_s*) psRegs, nErrorCode );
    return;
  }
    
  printk( "**Divide error**\n" );
  printk( "ERROR CODE = %08x\n", nErrorCode );
  print_registers( psRegs );
  printk( "\n" );
  
//  printk( "Areas :\n" );
//  list_areas( CURRENT_PROC->tc_psMemSeg );

  do_exit( 12 << 8 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

void handle_illega_inst_exception( SysCallRegs_s* psRegs, int nErrorCode )
{
  printk( "**Illegal instruction**\n" );
  printk( "ERROR CODE = %08x\n", nErrorCode );
  print_registers( psRegs );
  printk( "\n" );
  
  printk( "Areas :\n" );
  list_areas( CURRENT_PROC->tc_psMemSeg );

  do_exit( 12 << 8 );
}


/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

void TrackStack( uint32	*esp )
{
  int32	i,j;

  for( i = 0, j = 0 ; i < 4096 && j < 20 ; i++ )
  {
/*		if ( ((esp[i] + 4096) > CUL(&esp[i]) ) && ((esp[i] - 4096) < CUL(&esp[i]) ) )	*/
    if ( (esp[i] & 0xfff00000) == 0x00100000 )
    {
/*			printk( "%ld -> %.8lx  (%ld)\n", j, esp[i-1], i-1 );	*/
      printk( "%ld -> %.8lx  (%ld)\n", j, esp[i+0], i );
/*			printk( "%ld -> %.8lx  (%ld)\n", j, esp[i+1], i+1 );	*/
/*			printk( "\n" );	*/
      j++;
    }
  }
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

void ExceptionHand( uint16 r_ss, uint16 r_gs, uint16 r_fs, uint16 r_es, uint16 r_ds, uint32 r_edi, uint32 r_esi, uint32 r_ebp, uint32 r_esp, uint32 r_ebx, uint32 r_edx, uint32 r_ecx, uint32 r_eax, uint32 num, uint32 ecode, uint32 r_eip, uint32 r_cs, uint32 r_eflags )
{
  printk( "Error: Exception %lx (%08lx,%04lx:%08lx)\n", num, ecode, r_cs, r_eip );
  list_areas( CURRENT_PROC->tc_psMemSeg );

  if ( CURRENT_THREAD != NULL ) {
    do_exit(1);
  } else {
    Schedule();
  }
  return;
}
