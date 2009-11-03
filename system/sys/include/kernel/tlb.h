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

#ifndef __F_KERNEL_TLB_H__
#define __F_KERNEL_TLB_H__

/* Invalidate local page table */
static inline void flush_tlb( void )
{
	unsigned int dummy;
	__asm__ __volatile__( "movl %%cr3,%0; movl %0,%%cr3" : "=r" (dummy) : : "memory" );
}

/* Flush one local pte */
static inline void flush_tlb_page( uint32 nVirtualAddress )
{
	__asm__ __volatile__( "invlpg %0" : : "m" ( *( char * ) nVirtualAddress ) );
}

void flush_tlb_global( void );	/* Invalidate page table on all processors	*/

#endif	/* __F_KERNEL_TLB_H__ */
