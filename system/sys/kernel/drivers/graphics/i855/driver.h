/*
 * From AGPGART
 * Copyright (C) 2002-2003 Dave Jones
 * Copyright (C) 1999 Jeff Hartmann
 * Copyright (C) 1999 Precision Insight, Inc.
 * Copyright (C) 1999 Xi Graphics, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * JEFF HARTMANN, OR ANY OTHER CONTRIBUTORS BE LIABLE FOR ANY CLAIM, 
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR 
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE 
 * OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */
 
#ifndef _DEVICE_H_
#define _DEVICE_H_


/* Intel i830 registers */
#define I830_GMCH_MEM_MASK		0x1
#define I830_GMCH_MEM_64M		0x1
#define I830_GMCH_MEM_128M		0
#define I830_GMCH_GMS_MASK		0x70
#define I830_GMCH_GMS_DISABLED		0x00
#define I830_GMCH_GMS_LOCAL		0x10
#define I830_GMCH_GMS_STOLEN_512	0x20
#define I830_GMCH_GMS_STOLEN_1024	0x30
#define I830_GMCH_GMS_STOLEN_8192	0x40
#define I830_RDRAM_CHANNEL_TYPE		0x03010
#define I830_RDRAM_ND(x)		(((x) & 0x20) >> 5)
#define I830_RDRAM_DDT(x)		(((x) & 0x18) >> 3)

/* Intel i845 - i945 registers */
#define I855_GMCH_CTRL			0x52
#define I855_GMCH_ENABLED		0x4
#define I855_GMCH_GMS_MASK		0x70
#define I855_GMCH_GMS_STOLEN_0M		0x0
#define I855_GMCH_GMS_STOLEN_1M		(0x1 << 4)
#define I855_GMCH_GMS_STOLEN_4M		(0x2 << 4)
#define I855_GMCH_GMS_STOLEN_8M		(0x3 << 4)
#define I855_GMCH_GMS_STOLEN_16M	(0x4 << 4)
#define I855_GMCH_GMS_STOLEN_32M	(0x5 << 4)
#define I915_GMCH_GMS_STOLEN_48M	(0x6 << 4)
#define I915_GMCH_GMS_STOLEN_64M	(0x7 << 4)
#define I85X_CAPID			0x44
#define I85X_VARIANT_MASK		0x7
#define I85X_VARIANT_SHIFT		5
#define I855_GME			0x0
#define I855_GM				0x4
#define I852_GME			0x2
#define I852_GM				0x5


#define I855_GMADDR		0x10
#define I855_MMADDR		0x14
#define I915_GMADDR		0x18
#define I915_MMADDR		0x10
#define I915_PTEADDR	0x1C
#define I855_PGETBL_CTL		0x2020
#define I855_PGETBL_ENABLED	0x00000001
#define I855_PTE_BASE		0x10000
#define I855_PTE_VALID		0x00000001

#define OUTREG32(mmap, addr, val)	__raw_writel((val), (mmap)+(addr))
#define OUTREG16(mmap, addr, val)	__raw_writew((val), (mmap)+(addr))
#define OUTREG8(mmap, addr, val)	__raw_writeb((val), (mmap)+(addr))

#define INREG32(mmap, addr)		__raw_readl((mmap)+(addr))
#define INREG16(mmap, addr)		__raw_readw((mmap)+(addr))
#define INREG8(mmap, addr)		__raw_readb((mmap)+(addr))

#define KB(x)	((x) * 1024)
#define MB(x)	(KB (KB (x)))
#define GB(x)	(MB (KB (x)))

#endif
