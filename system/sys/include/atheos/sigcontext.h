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

#ifndef __F_ATHEOS_SIGCONTEXT_H__
#define __F_ATHEOS_SIGCONTEXT_H__

#ifdef __cplusplus
extern "C" {
#if 0  
} /*make emacs indention work */
#endif
#endif

/*
 * As documented in the iBCS2 standard..
 *
 * The first part of "struct _fpstate" is just the normal i387
 * hardware setup, the extra "status" word is used to save the
 * coprocessor status word before entering the handler.
 *
 * Pentium III FXSR, SSE support
 *	Gareth Hughes <gareth@valinux.com>, May 2000
 *
 * The FPU state data structure has had to grow to accomodate the
 * extended FPU state required by the Streaming SIMD Extensions.
 * There is no documented standard to accomplish this at the moment.
 */

struct _fpreg {
    unsigned short significand[4];
    unsigned short exponent;
};

struct _fpxreg {
    unsigned short significand[4];
    unsigned short exponent;
    unsigned short padding[3];
};

struct _xmmreg {
    unsigned long element[4];
};

typedef struct _fpstate {
    /* Regular FPU environment */
    unsigned long  cw;
    unsigned long  sw;
    unsigned long  tag;
    unsigned long  ipoff;
    unsigned long  cssel;
    unsigned long  dataoff;
    unsigned long  datasel;
    struct _fpreg  _st[8];
    unsigned short  status;
    unsigned short  magic;	    /* 0xffff = regular FPU data only */

    /* FXSR FPU environment */
    unsigned long   _fxsr_env[6];   /* FXSR FPU env is ignored */
    unsigned long   mxcsr;
    unsigned long   reserved;
    struct _fpxreg  _fxsr_st[8];    /* FXSR FPU reg data is ignored */
    struct _xmmreg  _xmm[8];
    unsigned long   padding[56];
} FPUState_s;

#define X86_FXSR_MAGIC		0x0000

typedef struct sigcontext
{
    unsigned short gs, __gsh;
    unsigned short fs, __fsh;
    unsigned short es, __esh;
    unsigned short ds, __dsh;
    unsigned long  edi;
    unsigned long  esi;
    unsigned long  ebp;
    unsigned long  esp;
    unsigned long  ebx;
    unsigned long  edx;
    unsigned long  ecx;
    unsigned long  eax;
    unsigned long  trapno;
    unsigned long  err;
    unsigned long  eip;
    unsigned short cs, __csh;
    unsigned long  eflags;
    unsigned long  esp_at_signal;
    unsigned short ss, __ssh;
    FPUState_s*    fpstate;
    unsigned long  oldmask;
    unsigned long  cr2;
    unsigned long  oldmask2;	// mask for signals 33-64
} SigContext_s;

#ifdef __cplusplus
}
#endif

#endif /* __F_ATHEOS_SIGCONTEXT_H__ */
