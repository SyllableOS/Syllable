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
 * The first part of "struct _fpstate" is just the
 * normal i387 hardware setup, the extra "status"
 * word is used to save the coprocessor status word
 * before entering the handler.
 */

typedef struct {
    unsigned short significand[4];
    unsigned short exponent;
} FPURegisters_s;

typedef struct _fpstate {
    unsigned long  cw;
    unsigned long  sw;
    unsigned long  tag;
    unsigned long  ipoff;
    unsigned long  cssel;
    unsigned long  dataoff;
    unsigned long  datasel;
    FPURegisters_s _st[8];
    unsigned long  status;
} FPUState_s;

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
} SigContext_s;

#ifdef __cplusplus
}
#endif

#endif /* __F_ATHEOS_SIGCONTEXT_H__ */
