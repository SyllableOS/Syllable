
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

#ifndef	EXEC_INTEL_H
#define	EXEC_INTEL_H

#include "inc/virt86.h"

#ifdef __cplusplus
extern "C"
{
#endif
#if 0
}				/* Make Emacs auto-indent work */
#endif

#include <atheos/types.h>

static const uint32_t EFLG_CARRY	= 0x00000001;
static const uint32_t EFLG_PARITY	= 0x00000004;
static const uint32_t EFLG_AUX_CARRY	= 0x00000010;
static const uint32_t EFLG_ZERO		= 0x00000040;
static const uint32_t EFLG_SIGN		= 0x00000080;
static const uint32_t EFLG_TRAP		= 0x00000100;	///< Trap (Single step)
static const uint32_t EFLG_IF		= 0x00000200;	///< Interrupt enable
static const uint32_t EFLG_DF		= 0x00000400;	///< Direction (String ops.)
static const uint32_t EFLG_OF		= 0x00000800;	///< Overflow
static const uint32_t EFLG_IOPL		= 0x00003000;	///< I/O Privilege level
static const uint32_t EFLG_NT		= 0x00004000;	///< Nested task
static const uint32_t EFLG_RESUME	= 0x00010000;
static const uint32_t EFLG_VM		= 0x00020000;	///< Virtual 86 mode
static const uint32_t EFLG_AC		= 0x00040000;
static const uint32_t EFLG_VIF		= 0x00080000;	///< Virtual interrupt flag
static const uint32_t EFLG_VIP		= 0x00100000;	///< Virtual interrupt pending
static const uint32_t EFLG_ID		= 0x00200000;

/*
 * Return values for the 'vm86()' system call
 */

#ifdef __KERNEL__

  /* The four data and code selectors used in AtheOS */
static const uint16_t CS_KERNEL	= 0x08;	/* Kernel (Ring 0) code segment */
static const uint16_t CS_USER		= 0x13;	/* User (Ring 3) code segment */
static const uint16_t DS_KERNEL	= 0x18;	/* Kernel (Ring 0) data segment */
static const uint16_t DS_USER		= 0x23;	/* User (Ring 3) data segment */


/**** END OF VM86 STUFF ***************************************/


typedef struct i3Task
{
	uint16 previous;
	uint16 r1;
	uint32 *esp0;
	uint16 ss0;
	uint16 r2;
	uint32 *esp1;
	uint16 ss1;
	uint16 r3;
	uint32 *esp2;
	uint16 ss2;
	uint16 r4;
	uint32 *cr3;
	void *eip;
	uint32 eflags;
	uint32 eax;
	uint32 ecx;
	uint32 edx;
	uint32 ebx;
	uint32 *esp;
	uint32 ebp;
	uint32 esi;
	uint32 edi;
	uint16 es;
	uint16 r5;
	uint16 cs;
	uint16 r6;
	uint16 ss;
	uint16 r7;
	uint16 ds;
	uint16 r8;
	uint16 fs;
	uint16 r9;
	uint16 gs;
	uint16 r10;
	uint16 ldt;
	uint16 r11;
	uint16 ctrl;
	uint16 IOMapBase;
	uint8 IOMap[8192];
} TaskStateSeg_s;


typedef struct i3DescrTable
{
	uint16 Limit __attribute__ ( ( packed ) );
	uint32 Base __attribute__ ( ( packed ) );
} DescriptorTable_s;

struct i3IntrGate
{
	uint16 igt_offl;	/* 0-15 of base         */
	uint16 igt_sel;		/* selector                             */
	uint16 igt_ctrl;	/* P/DPL                                        */
	uint16 igt_offh;	/* 16-31 of base        */
};

struct i3Desc
{
	uint16 desc_lml;	/* limit 0-15           */
	uint16 desc_bsl;	/* base 0-15            */
	uint8 desc_bsm;		/* base 16-23           */
	uint8 desc_acc;		/* type/dpl/p           */
	uint8 desc_lmh;		/* limit 16-19  */
	uint8 desc_bsh;		/* base 24-31           */
};

struct i3FSave_t
{
	long cwd;
	long swd;
	long twd;
	long fip;
	long fcs;
	long foo;
	long fos;
	long st_space[20];	/* 8*10 bytes for each FP-reg = 80 bytes */
	long status;		/* software status information		 */
};

struct i3FXSave_t
{
	unsigned short cwd;
	unsigned short swd;
	unsigned short twd;
	unsigned short fop;
	long fip;
	long fcs;
	long foo;
	long fos;
	long mxcsr;
	long mxcsr_mask;
	long st_space[32];	/* 8*16 bytes for each FP-reg = 128 bytes */
	long xmm_space[32];	/* 8*16 bytes for each XMM-reg = 128 bytes */
	long padding[56];
}; // __attribute__ ((aligned (16)));

union i3FPURegs_u
{
	struct i3FSave_t	fpu_sFSave;
	struct i3FXSave_t	fpu_sFXSave;
};

struct i3MTRRDesc
{
	uint32 nBaseLow;
	uint32 nBaseHigh;
	uint32 nMaskLow;
	uint32 nMaskHigh;
};

struct i3MTRRDescrTable
{
	int nNumDesc;
	struct i3MTRRDesc sDesc[16];
};


/** Intel x86 page directory manages 4MB page tables (1024 x 4K pages) */
#define PGDIR_SHIFT	22
#define PGDIR_SIZE	(1UL << PGDIR_SHIFT)
#define PGDIR_MASK	(~(PGDIR_SIZE-1))

#define PTE_PRESENT		0x001  ///< dir/page is present/valid
#define PTE_WRITE		0x002  ///< dir/page is writeable
#define PTE_USER		0x004  ///< only accessible by supervisor
#define PTE_ACCESSED	0x020  ///< the page has been accessed (R or W)
#define PTE_DIRTY		0x040  ///< the page has been written to
#define PTE_AVAILABLE	0xE00  ///< free for system use (NOT APPLICATIONS)
#define PTE_DELETED		0x400  ///< has been deleted


/*
 *  Bit definitions in the exception error code generated by a page fault
 */

static const uint32_t PFE_PROTVIOL	= 0x0001;
			/* if set, the page-fault was caused by a protection
			 * violation, otherwise by a not present page
			 */
static const uint32_t PFE_WRITE		= 0x0002;  /* if set fault caused by a write, otherwise by a read  */
static const uint32_t PFE_USER		= 0x0004;  /* if set fault caused in user-mode otherwise in supervisor-mode */

static inline void set_page_directory_base_reg( void *pPageTable )
{
	__asm__ __volatile__( "movl %0,%%cr3" : : "r" (pPageTable) );
}


inline void load_fpu_state( union i3FPURegs_u *pState );

inline void save_fpu_state( union i3FPURegs_u *pState );

static inline void clts( void )
{
	__asm__ __volatile__( "clts" );		// Clear TS bit -- enable FPU
}

static inline void stts( void )
{
	unsigned int dummy;			// Set TS bit -- disable FPU
	__asm__ __volatile__( "movl %%cr0,%0; orl $0x08,%0; movl %0,%%cr0" : "=r" (dummy) );
}

static inline void SetIDT( DescriptorTable_s * psTable )
{
	__asm__ __volatile__( "lidt %0" : : "m" ( *psTable ) );
}

static inline void SetGDT( DescriptorTable_s * psTable )
{
	__asm__ __volatile__( "lgdt %0" : : "m" ( *psTable ) );
}

static inline void SetTR( int nTaskReg )
{
	__asm__ __volatile__( "ltr %%ax" : : "a" (nTaskReg) );
}

/* CR4 register */

static const uint32_t X86_CR4_VME	  = 0x0001; // enable vm86 extensions
static const uint32_t X86_CR4_PVI	  = 0x0002; // enable virtual interrupts flag
static const uint32_t X86_CR4_TSD	  = 0x0004; // disable time stamp at ipl 3
static const uint32_t X86_CR4_DE	  = 0x0008; // enable debugging extensions
static const uint32_t X86_CR4_PSE	  = 0x0010; // enable page size extensions
static const uint32_t X86_CR4_PAE	  = 0x0020; // enable physical address extensions
static const uint32_t X86_CR4_MCE	  = 0x0040; // enable machine check exceptions
static const uint32_t X86_CR4_PGE	  = 0x0080; // enable global page feature
static const uint32_t X86_CR4_PCE	  = 0x0100; // enable perf counters at ipl 3
static const uint32_t X86_CR4_OSFXSR	  = 0x0200; // enable fast FPU save and restore
static const uint32_t X86_CR4_OSXMMEXCPT = 0x0400; // enable unmasked SSE exceptions

static inline void set_in_cr4( unsigned int nFlags )
{
	uint32 nCR4;
	__asm__ __volatile__( "movl %%cr4,%0" : "=r" (nCR4) );
	__asm__ __volatile__( "movl %0,%%cr4" : : "r" ( nCR4 | nFlags ) );
}

/* MSR registers */

#define MSR_REG_APICBASE			0x1b
#define MSR_REG_APICBASE_BSP		(1<<8)
#define MSR_REG_APICBASE_ENABLE		(1<<11)
#define MSR_REG_APICBASE_BASE		(0xfffff<<12)
#define MSR_REG_MTRR_CAP			0x0fe
#define MSR_REG_MTRR_CAP_NUM		0xff
#define MSR_REG_MTRR_CAP_WRCOMP		(1<<10)
#define MSR_REG_MTRR_DEFTYPE		0x2ff
#define MSR_REG_MTRR_BASE( nReg ) ( 0x200 + 2 * ( nReg ) )
#define MSR_REG_MTRR_MASK( nReg ) ( 0x200 + 2 * ( nReg ) + 1 )

#define rdmsr( nReg, nLow, nHigh ) \
	__asm__ __volatile__( "rdmsr" \
			  : "=a" ( nLow ), "=d" ( nHigh ) \
			  : "c" ( nReg ) )

#define wrmsr( nReg, nLow, nHigh ) \
	__asm__ __volatile__("wrmsr" \
			  : /* no outputs */ \
			  : "c" ( nReg ), "a" ( nLow ), "d" ( nHigh ) )


void init_cpuid( void );

void init_descriptors( void );
void enable_mmu( void );
bool set_gdt_desc_base( uint16 desc, uint32 base );
uint32 get_gdt_desc_base( uint16 desc );
bool set_gdt_desc_limit( uint16 desc, uint32 limit );
uint32 get_gdt_desc_limit( uint16 desc );
bool set_gdt_desc_access( uint16 desc, uint8 acc );
uint8 get_gdt_desc_access( uint16 desc );
uint16 alloc_gdt_desc( int32 table );
void free_gdt_desc( uint16 desc );
void write_mtrr_descs( void );
status_t alloc_mtrr_desc( uint64 nBase, uint64 nSize, int nType );
status_t free_mtrr_desc( uint64 nBase );

void load_debug_regs( uint32 regs[8] );
void clear_debug_regs( void );
uint32 read_debug_status( void );
#endif /* __KERNEL__ */

// Read the TSC clock

#define rdtsc(low,high) \
     __asm__ __volatile__( "rdtsc" : "=a" (low), "=d" (high) )

#define rdtscl(low) \
     __asm__ __volatile__( "rdtsc" : "=a" (low) : : "edx" )

#define rdtscll(val) \
     __asm__ __volatile__( "rdtsc" : "=A" (val) )


static inline uint64 read_pentium_clock( void )
{
	unsigned long long val;
	rdtscll( val );
	return val;
}

// REP NOP (PAUSE) is a good thing to insert into busy-wait loops.
static inline void rep_nop( void )
{
	__asm__ __volatile__( "rep; nop" : : : "memory" );
}

#ifdef __cplusplus
}
#endif

#endif /*       EXEC_INTEL_H    */
