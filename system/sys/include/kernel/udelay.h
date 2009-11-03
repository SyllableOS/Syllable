/*
 * Copyright (C) 1993 Linus Torvalds
 *
 * Modified to fit into the AtheOS kernel by Kurt Skauen 08 Jan 2000
 */

#ifndef __F_KERNEL_UDELAY_H__
#define __F_KERNEL_UDELAY_H__

extern void __udelay(unsigned long usecs);
extern void __ndelay(unsigned long nsecs);
extern void __const_udelay(unsigned long usecs);
extern void __delay(unsigned long loops);

#define udelay(n) (__builtin_constant_p(n) ? \
	(__const_udelay((n) * 0x10c7ul)) : __udelay(n))

#define ndelay(n) (__builtin_constant_p(n) ? \
	(__const_udelay((n) * 5ul)) : __ndelay(n))

#endif	/* __F_KERNEL_UDELAY_H__ */
