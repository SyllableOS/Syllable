#ifndef __F_KERNEL_SEQLOCK_H_
#define __F_KERNEL_SEQLOCK_H_

/** \file seqlock.h
 * Reader/writer consistent mechanism without starving writers. This type of
 * lock for data where the reader wants a consitent set of information
 * and is willing to retry if the information changes.  Readers never
 * block but they may have to retry if a writer is in
 * progress. Writers do not wait for readers. 
 *
 * This is not as cache friendly as brlock. Also, this will not work
 * for data that contains pointers, because any writer could
 * invalidate a pointer that a reader was following.
 *
 * Expected reader usage:
 * 	do {
 *	    seq = read_seqbegin(&foo);
 * 	...
 *      } while (read_seqretry(&foo, seq));
 *
 *
 * On non-SMP the spin locks disappear but the writer still needs
 * to increment the sequence variables because an interrupt routine could
 * change the state of the data.
 *
 * Based on x86_64 vsyscall gettimeofday 
 * by Keith Owens and Andrea Arcangeli
 */

#include <kernel/spinlock.h>

/*
 * SMP memory barriers.  Force strict CPU ordering of reads.  No write
 * barrier is needed for Intel processors.
 */
#define smp_rmb() __asm__ __volatile__( "lock; addl $0,0(%%esp)" ::: "memory" )
#define smp_wmb() __asm__ __volatile__( "" : : : "memory" )

typedef struct {
	uint32_t sl_nSequence;
	SpinLock_s sl_sLock;
} SeqLock_s;

/*
 * These macros triggered gcc-3.x compile-time problems.  We think these are
 * OK now.  Be cautious.
 */
#define SEQ_LOCK( var, name ) SeqLock_s var = { 0, INIT_SPIN_LOCK( name ) }
#define INIT_SEQ_LOCK( name ) { 0, INIT_SPIN_LOCK( name ) }

static inline void seqlock_init( SeqLock_s *psLock, const char *pzName )
{
	psLock->sl_nSequence = 0;
	spinlock_init( &psLock->sl_sLock, pzName );
}

/* Lock out other writers and update the count.
 * Acts like a normal spin_lock/unlock.
 * Don't need preempt_disable() because that is in the spin_lock already.
 */
static inline void write_seqlock(SeqLock_s *sl)
{
	spinlock( &sl->sl_sLock );
	++sl->sl_nSequence;
	smp_wmb();			
}	

static inline void write_sequnlock(SeqLock_s *sl) 
{
	smp_wmb();
	sl->sl_nSequence++;
	spinunlock( &sl->sl_sLock );
}

static inline int write_tryseqlock(SeqLock_s *sl)
{
	int ret = spin_trylock(&sl->sl_sLock);

	if ( ret == 0 ) {
		++sl->sl_nSequence;
		smp_wmb();			
	}
	return ret;
}

/* Start of read calculation -- fetch last complete writer token */
static inline uint32_t read_seqbegin(const SeqLock_s *sl)
{
	uint32_t ret = sl->sl_nSequence;
	smp_rmb();
	return ret;
}

/* Test if reader processed invalid data.
 * If initial values is odd, 
 *	then writer had already started when section was entered
 * If sequence value changed
 *	then writer changed data while in section
 *    
 * Using xor saves one conditional branch.
 */
static inline int read_seqretry(const SeqLock_s *sl, uint32_t iv)
{
	smp_rmb();
	return (iv & 1) | (sl->sl_nSequence ^ iv);
}


/*
 * Version using sequence counter only.
 * This can be used when code has its own mutex protecting the
 * updating starting before the write_seqcountbeqin() and ending
 * after the write_seqcount_end().
 */

typedef struct seqcount {
	uint32_t sc_nSequence;
} seqcount_t;

#define SEQCNT_ZERO { 0 }
#define seqcount_init(x)	do { *(x) = (seqcount_t) SEQCNT_ZERO; } while (0)

/* Start of read using pointer to a sequence counter only.  */
static inline uint32_t read_seqcount_begin(const seqcount_t *s)
{
	uint32_t ret = s->sc_nSequence;
	smp_rmb();
	return ret;
}

/* Test if reader processed invalid data.
 * Equivalent to: iv is odd or sequence number has changed.
 *                (iv & 1) || (*s != iv)
 * Using xor saves one conditional branch.
 */
static inline int read_seqcount_retry(const seqcount_t *s, uint32_t iv)
{
	smp_rmb();
	return (iv & 1) | (s->sc_nSequence ^ iv);
}


/*
 * Sequence counter only version assumes that callers are using their
 * own mutexing.
 */
static inline void write_seqcount_begin(seqcount_t *s)
{
	s->sc_nSequence++;
	smp_wmb();
}

static inline void write_seqcount_end(seqcount_t *s)
{
	smp_wmb();
	s->sc_nSequence++;
}

/*
 * Possible sw/hw IRQ protected versions of the interfaces.
 */

static inline int write_seqlock_disable( SeqLock_s* psLock )
{
	uint32_t nFlg = cli();
	write_seqlock( psLock );
	return( nFlg );
}

static inline void write_sequnlock_enable( SeqLock_s* psLock, uint32_t nFlg )
{
	write_sequnlock( psLock );
	put_cpu_flags( nFlg );
}

#define read_seqbegin_cli(lock, flags)					\
	({ flags = cli();   read_seqbegin(lock); })

#define read_seqretry_restore(lock, iv, flags)				\
	({								\
		int ret = read_seqretry(lock, iv);			\
		put_cpu_flags(flags);					\
		ret;							\
	})

#endif	/* __F_KERNEL_SEQLOCK_H_ */
