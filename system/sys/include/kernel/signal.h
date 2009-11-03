/*
 *  The Syllable kernel
 *  Copyright (C) 2009 Kristian Van Der Vliet
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

#ifndef	__F_KERNEL_SIGNAL_H_
#define	__F_KERNEL_SIGNAL_H_

#define _NSIG		64		/* Biggest signal number */
#define _NSIG_BPW	32
#define _NSIG_WORDS	( _NSIG / _NSIG_BPW )

#include <kernel/types.h>

#include <posix/signal.h>

/*
 * sa_flags values: SA_STACK is not currently supported, but will allow the
 * usage of signal stacks by using the (now obsolete) sa_restorer field in
 * the sigaction structure as a stack pointer. This is now possible due to
 * the changes in signal handling. LBT 010493.
 * SA_INTERRUPT is a no-op, but left due to historical reasons. Use the
 * SA_RESTART flag to get restarting signals (which were the default long ago)
 * SA_SHIRQ flag is for shared interrupt support on PCI and EISA.
 * SA_NOCLDSTOP turns off SIGCHLD signals when children stop.
 * SA_NOCLDWAIT set on SIGCHLD prevents zombie processes from being created.
 * SA_SIGINFO invokes the signal handler with 3 arguments instead of one.
 */

#define SA_NOCLDSTOP	1
#define SA_NOCLDWAIT	2
#define SA_SIGINFO	4
#define SA_SHIRQ	0x04000000u
#define SA_STACK	0x08000000u
#define SA_RESTART	0x10000000u
#define SA_INTERRUPT	0x20000000u
#define SA_NOMASK	0x40000000u
#define SA_NODEFER	SA_NOMASK
#define SA_ONESHOT	0x80000000u

/*
 * These values of sa_flags are used only by the kernel as part of the
 * irq handling routines.
 *
 * SA_INTERRUPT is also used by the irq handling routines.
 */
#define SA_PROBE SA_ONESHOT
#define SA_SAMPLE_RANDOM SA_RESTART

#define SIG_BLOCK	 0		/* for blocking signals */
#define SIG_UNBLOCK	 1		/* for unblocking signals */
#define SIG_SETMASK	 2		/* for setting the signal mask */

/* Size of sigset_t in words.  */

/* The kernel sigset_t is smaller than the version passed from glibc.  */
typedef	struct
{
  unsigned long int __val[_NSIG_WORDS];
} sigset_t;

# define _SIGSET_NWORDS (1024 / (8 * sizeof (unsigned long int)))
typedef struct
{
  unsigned long int __val[_SIGSET_NWORDS];
} libc_sigset_t;

typedef struct sigaction
{
  sighandler_t	sa_handler;
  sigset_t 	sa_mask;
  unsigned long sa_flags;
  void 		(*sa_restorer)(void);
} SigAction_s;

typedef struct libc_sigaction
{
  sighandler_t	sa_handler;
  libc_sigset_t	sa_mask;
  unsigned long sa_flags;
  void 		(*sa_restorer)(void);
};

typedef struct sigaltstack
{
  void*		ss_sp;
  size_t	ss_size;
  int		ss_flags;
} stack_t;

/* Values returned from GetSignalMode() */

#define SIG_HANDLED	1
#define SIG_IGNORED	2
#define SIG_DEFAULT	3
#define SIG_BLOCKED	4

int	get_signal_mode( int nSigNum );

int	is_signal_pending( void );
void send_alarm_signals( bigtime_t nCurTime );
void send_timer_signals( bigtime_t nCurTime );

int	raise(int _sig);

void	(*signal(int nSig, void (*pFunc)(int)))(int);
void	(*sys_signal(int nSig, void (*pFunc)(int)))(int);

void	EnterSyscall( int dummy );
void	ExitSyscall( int dummy );
int	sys_sig_return( int dummy );

int sys_kill( pid_t nPid, int nSig );
int sys_sigaction( int nSig, const struct libc_sigaction *__restrict psAct,
		   struct libc_sigaction *__restrict psOldAct );
int sys_sigpending( libc_sigset_t *pSet );
int sys_sigprocmask( int nHow, const libc_sigset_t *__restrict pSet,
		     libc_sigset_t *__restrict pOldSet );

int sys_sigsuspend( unsigned long int nSetHigh, unsigned long int nSetMid,
		    unsigned long int nSetLow );

/* Obsolete signal set functions.  */
int sys_sigaddset( sigset_t *pSet, int nSigNo );
int sys_sigdelset( sigset_t *pSet, int nSigNo );
int sys_sigemptyset( sigset_t *pSet );
int sys_sigfillset( sigset_t *pSet );
int sys_sigismember( const sigset_t *pSet, int nSigNo );

/* Define inline signal set functions (based on glibc <bits/sigset.h>).  */

/* Return a mask that includes the bit for SIG only.  */
#define __sigmask(sig) \
  (((unsigned long int) 1) << (((sig) - 1) % (8 * sizeof (unsigned long int))))

/* Return the word index for SIG.  */
#define __sigword(sig) (((sig) - 1) / (8 * sizeof (unsigned long int)))

#define sigemptyset(set) \
  (__extension__ ({ sigset_t *__set = (set);				\
		    __set->__val[0] = 0;     __set->__val[1] = 0;	\
		    0; }))
#define sigfillset(set) \
  (__extension__ ({ sigset_t *__set = (set);				\
		    __set->__val[0] = ~0UL;  __set->__val[1] = ~0UL;	\
		    0; }))

#define sigandset(dest, left, right) \
  (__extension__ ({ sigset_t *__dest = (dest);				\
		    const sigset_t *__left = (left);			\
		    const sigset_t *__right = (right);			\
		    __dest->__val[0] = (__left->__val[0] & __right->__val[0]); \
		    __dest->__val[1] = (__left->__val[1] & __right->__val[1]); \
		    0; }))
#define sigandnotset(dest, left, right) \
  (__extension__ ({ sigset_t *__dest = (dest);				\
		    const sigset_t *__left = (left);			\
		    const sigset_t *__right = (right);			\
		    __dest->__val[0] = (__left->__val[0] & ~(__right->__val[0])); \
		    __dest->__val[1] = (__left->__val[1] & ~(__right->__val[1])); \
		    0; }))
#define sigorset(dest, left, right) \
  (__extension__ ({ sigset_t *__dest = (dest);				\
		    const sigset_t *__left = (left);			\
		    const sigset_t *__right = (right);			\
		    __dest->__val[0] = (__left->__val[0] | __right->__val[0]); \
		    __dest->__val[1] = (__left->__val[1] | __right->__val[1]); \
		    0; }))

#define __SIGSETFN(NAME, BODY, CONST)					\
  extern inline int							\
  NAME (CONST sigset_t *__set, int __sig)				\
  {									\
    unsigned long int __mask = __sigmask (__sig);			\
    unsigned long int __word = __sigword (__sig);			\
    return BODY;							\
  }

__SIGSETFN (sigismember, (__set->__val[__word] & __mask) ? 1 : 0, const)
__SIGSETFN (sigaddset, ((__set->__val[__word] |= __mask), 0), )
__SIGSETFN (sigdelset, ((__set->__val[__word] &= ~__mask), 0), )

#undef __SIGSETFN

#endif	/* __F_KERNEL_SIGNAL_H_ */
