
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
#include <posix/wait.h>

#include <atheos/types.h>
#include <atheos/sigcontext.h>

#include <macros.h>

#include "inc/scheduler.h"
#include "inc/smp.h"

/*****************************************************************************
 * NAME:
 * DESC:    Save i387 state into FPUState_s.
 * NOTE:    Based on Linux.
 * SEE ALSO:
 ****************************************************************************/

static inline unsigned long twd_fxsr_to_i387( struct i3FXSave_t *fxsave )
{
	struct _fpxreg *st = NULL;
	unsigned long twd = (unsigned long)fxsave->twd;
	unsigned long tag;
	unsigned long ret = 0xffff0000u;
	int i;

#define FPREG_ADDR(f, n)	((char *)&(f)->st_space + (n) * 16);

	for ( i = 0; i < 8; i++ )
	{
		if ( twd & 0x1 )
		{
			st = (struct _fpxreg *) FPREG_ADDR( fxsave, i );

			switch ( st->exponent & 0x7fff )
			{
				case 0x7fff:
					tag = 2;		// Special
					break;
				case 0x0000:
					if ( !st->significand[0] &&
					     !st->significand[1] &&
					     !st->significand[2] &&
					     !st->significand[3] )
					{
						tag = 1;	// Zero
					}
					else
					{
						tag = 2;	// Special
					}
					break;
				default:
					if ( st->significand[3] & 0x8000 )
					{
						tag = 0;	// Valid
					}
					else
					{
						tag = 2;	// Special
					}
					break;
			}
		}
		else
		{
			tag = 3;	// Empty
		}
		ret |= (tag << (2 * i));
		twd = twd >> 1;
	}
	return ret;
}

static inline void convert_fxsr_to_user( struct _fpstate *buf, struct i3FXSave_t *fxsave )
{
	unsigned long env[7];
	struct _fpreg *to;
	struct _fpxreg *from;
	int i;

	env[0] = (unsigned long)fxsave->cwd | 0xffff0000ul;
	env[1] = (unsigned long)fxsave->swd | 0xffff0000ul;
	env[2] = twd_fxsr_to_i387(fxsave);
	env[3] = fxsave->fip;
	env[4] = fxsave->fcs | ((unsigned long)fxsave->fop << 16);
	env[5] = fxsave->foo;
	env[6] = fxsave->fos;

	memcpy_to_user( buf, env, 7 * sizeof( unsigned long ) );

	to = &buf->_st[0];
	from = (struct _fpxreg *) &fxsave->st_space[0];
	for ( i = 0; i < 8; i++, to++, from++ )
	{
		unsigned long *t = (unsigned long *)to;
		unsigned long *f = (unsigned long *)from;
		*t = *f;
		*(t + 1) = *(f + 1);
		to->exponent = from->exponent;
	}
}

static inline void save_i387_fxsave( struct _fpstate *buf, Thread_s *psThread )
{
	convert_fxsr_to_user( buf, &psThread->tc_FPUState.fpu_sFXSave );
	buf->status = psThread->tc_FPUState.fpu_sFXSave.swd;
	buf->magic = X86_FXSR_MAGIC;
	memcpy_to_user( &buf->_fxsr_env[0], &psThread->tc_FPUState.fpu_sFXSave, sizeof( struct i3FXSave_t ) );
}

static inline void save_i387_fsave( struct _fpstate *buf, Thread_s *psThread )
{
	psThread->tc_FPUState.fpu_sFSave.status = psThread->tc_FPUState.fpu_sFSave.swd;
	memcpy_to_user( buf, &psThread->tc_FPUState.fpu_sFSave, sizeof( struct i3FSave_t ) );
}

int save_i387( struct _fpstate *buf )
{
	Thread_s *psThread = CURRENT_THREAD;
	if ( ( psThread->tr_nFlags & TF_FPU_USED ) == 0 )
	{
		return ( 0 );
	}

	// This will cause a "finit" to be triggered by the next
	// attempted FPU operation by the current thread.
	psThread->tr_nFlags &= ~TF_FPU_USED;
	
	if ( psThread->tr_nFlags & TF_FPU_DIRTY )
	{
		save_fpu_state( &psThread->tc_FPUState );
		psThread->tr_nFlags &= ~TF_FPU_DIRTY;
		stts();
	}

	if ( g_asProcessorDescs[g_nBootCPU].pi_bHaveFXSR )
		save_i387_fxsave( buf, psThread );
	else
		save_i387_fsave( buf, psThread );

	return ( 1 );
}


/*****************************************************************************
 * NAME:
 * DESC:    Restore i387 state from FPUState_s.
 * NOTE:    Based on Linux.
 * SEE ALSO:
 ****************************************************************************/

static inline unsigned short twd_i387_to_fxsr( unsigned short twd )
{
	unsigned int tmp;	// to avoid 16 bit prefixes in the code

	// Transform each pair of bits into 01 (valid) or 00 (empty)
	tmp = ~twd;
	tmp = (tmp | (tmp >> 1)) & 0x5555;	// 0V0V0V0V0V0V0V0V
	// and move the valid bits to the lower byte.
	tmp = (tmp | (tmp >> 1)) & 0x3333;	// 00VV00VV00VV00VV
	tmp = (tmp | (tmp >> 2)) & 0x0f0f;	// 0000VVVV0000VVVV
	tmp = (tmp | (tmp >> 4)) & 0x00ff;	// 00000000VVVVVVVV
	return tmp;
}

static inline void convert_fxsr_from_user( struct i3FXSave_t *fxsave, struct _fpstate *buf )
{
	unsigned long env[7];
	struct _fpxreg *to;
	struct _fpreg *from;
	int i;

	memcpy_from_user( env, buf, 7 * sizeof(long) );

	fxsave->cwd = (unsigned short)( env[0] & 0xffff );
	fxsave->swd = (unsigned short)( env[1] & 0xffff );
	fxsave->twd = twd_i387_to_fxsr( (unsigned short)( env[2] & 0xffff ) );
	fxsave->fip = env[3];
	fxsave->fop = (unsigned short)( ( env[4] & 0xffff0000ul ) >> 16 );
	fxsave->fcs = ( env[4] & 0xffff );
	fxsave->foo = env[5];
	fxsave->fos = env[6];

	to = (struct _fpxreg *) &fxsave->st_space[0];
	from = &buf->_st[0];
	for ( i = 0; i < 8; i++, to++, from++ )
	{
		unsigned long *t = (unsigned long *)to;
		unsigned long *f = (unsigned long *)from;
		*t = *f;
		*(t + 1) = *(f + 1);
		to->exponent = from->exponent;
	}
}

static inline void restore_i387_fxsave( struct _fpstate *buf, Thread_s *psThread )
{
	memcpy_from_user( &psThread->tc_FPUState.fpu_sFXSave, &buf->_fxsr_env[0], sizeof( struct i3FXSave_t ) );
	// mxcsr reserved bits must be masked to zero for security reasons
	psThread->tc_FPUState.fpu_sFXSave.mxcsr &= 0x0000ffbf;
	convert_fxsr_from_user( &psThread->tc_FPUState.fpu_sFXSave, buf );
}

static inline void restore_i387_fsave( struct _fpstate *buf, Thread_s *psThread )
{
	memcpy_from_user( &psThread->tc_FPUState.fpu_sFSave, buf, sizeof( struct i3FSave_t ) );
}

void restore_i387( struct _fpstate *buf )
{
	Thread_s *psThread = CURRENT_THREAD;
	if ( psThread->tr_nFlags & TF_FPU_DIRTY )
	{
		__asm__ __volatile__( "fnclex; fwait" );
		psThread->tr_nFlags &= ~TF_FPU_DIRTY;
		stts();
	}

	if ( g_asProcessorDescs[g_nBootCPU].pi_bHaveFXSR )
		restore_i387_fxsave( buf, psThread );
	else
		restore_i387_fsave( buf, psThread );

	psThread->tr_nFlags |= TF_FPU_USED;
}


inline void load_fpu_state( union i3FPURegs_u *pState )
{
	if ( g_asProcessorDescs[g_nBootCPU].pi_bHaveFXSR )
	{
		if ( ( ( unsigned int )&pState->fpu_sFXSave & 0x0000000f ) != 0 )
		{
			printk( "Panic: fxsave struct is misaligned: %p\n", &pState->fpu_sFXSave );
			return;
		}
		__asm__ __volatile__( "fxrstor %0" : : "m" ( pState->fpu_sFXSave ) );
	}
	else
	{
		__asm__ __volatile__( "frstor %0" : : "m" ( pState->fpu_sFSave ) );
	}
}

inline void save_fpu_state( union i3FPURegs_u *pState )
{
	if ( g_asProcessorDescs[g_nBootCPU].pi_bHaveFXSR )
	{
		if ( ( ( unsigned int )&pState->fpu_sFXSave & 0x0000000f ) != 0 )
		{
			printk( "Panic: fxsave struct is misaligned: %p\n", &pState->fpu_sFXSave );
			return;
		}
		__asm__ __volatile__( "fxsave %0; fnclex" : "=m" ( pState->fpu_sFXSave ) );
	}
	else
	{
		__asm__ __volatile__( "fnsave %0; fwait" : "=m" ( pState->fpu_sFSave ) );
	}
}



