#ifndef UTILITY_H
#define UTILITY_H
/* Utility - some helper classes
**
** Initial version by Axel Dörfler, axeld@pinc-software.de
** This file may be used under the terms of the OpenBeOS License.
*/



#include <atheos/kernel.h>
#include <atheos/filesystem.h>
#include <atheos/fs_attribs.h>
#include <posix/stat.h>
#include <posix/limits.h>
#include <posix/unistd.h>
#include <posix/errno.h>
#include <posix/dirent.h>
#include <macros.h>



#define B_OK 0
#define B_NO_ERROR 0
#define B_ERROR -1
#define B_NO_INIT -EINITFAILED
#define B_IO_ERROR -EIO
#define B_NO_MEMORY -ENOMEM
#define B_BAD_VALUE -EINVAL
#define B_DEVICE_FULL -ENOSPC
#define B_ENTRY_NOT_FOUND -ENOENT
#define B_NAME_IN_USE -EEXIST
#define B_INTERRUPTED -EINTR
#define B_NOT_ALLOWED -EPERM
#define B_BAD_INDEX -EINVAL
#define B_BUSY -EBUSY
#define B_READ_ONLY_DEVICE -EROFS
#define B_FILE_ERROR -EIO
#define B_NOT_A_DIRECTORY -ENOTDIR
#define B_IS_A_DIRECTORY -EISDIR
#define B_DIRECTORY_NOT_EMPTY -ENOTEMPTY
#define B_FILE_EXISTS -EEXIST
#define B_BUFFER_OVERFLOW -EOVERFLOW

#define _PACKED __attribute__((packed))
#define B_FILE_NAME_LENGTH PATH_MAX
#define B_ATTR_NAME_LENGTH MAXNAMLEN

#define S_IUMSK 07777

enum
{
	S_ALLOW_DUPS = 00002000000,
	
	S_STR_INDEX = 00100000000,
	S_INT_INDEX = 00200000000,
	S_UINT_INDEX = 00400000000,
	S_LONG_LONG_INDEX = 00010000000,
	S_ULONG_LONG_INDEX = 00020000000,
	S_FLOAT_INDEX = 00040000000,
	S_DOUBLE_INDEX = 00001000000,
	S_INDEX_DIR = 04000000000,
	S_ATTR_DIR = 01000000000,
	S_ATTR = 02000000000
};

enum
{
	B_STRING_TYPE = ATTR_TYPE_STRING,
	B_SSIZE_T_TYPE = ATTR_TYPE_INT32,
	B_INT32_TYPE = ATTR_TYPE_INT32,
	B_SIZE_T_TYPE = ATTR_TYPE_INT32,
	B_UINT32_TYPE = ATTR_TYPE_INT32,
	B_OFF_T_TYPE = ATTR_TYPE_INT64,
	B_INT64_TYPE = ATTR_TYPE_INT64,
	B_UINT64_TYPE = ATTR_TYPE_INT64,
	B_FLOAT_TYPE = ATTR_TYPE_FLOAT,
	B_DOUBLE_TYPE = ATTR_TYPE_DOUBLE,
	B_BAD_TYPE = ATTR_TYPE_COUNT
};

enum
{
	B_QUERY_NON_INDEXED = 1,
	B_LIVE_QUERY = 2
};

enum
{
	B_LOW_PRIORITY = LOW_PRIORITY
};

enum
{
	B_MOUNT_READ_ONLY = MNTF_READONLY
};

typedef int type_code;
typedef ino_t vnode_id;
typedef kdev_t mount_id;
typedef void** thread_func;


static void* malloc( size_t nSize )
{
	return( kmalloc( nSize, MEMF_KERNEL | MEMF_CLEAR ) );
}
#define free kfree

#define	min_c(a,b)	(((a)<(b)) ? (a) : (b) )

static const char* strerror( int error )
{
	return( "" );
}

// Simple array, used for the duplicate handling in the B+Tree,
// and for the log entries.

struct sorted_array {
	public:
		off_t	count;
		
		#if __MWERKS__
			off_t	values[1];
		#else
			off_t	values[0];
		#endif

		inline int32 Find(off_t value) const;
		void Insert(off_t value);
		bool Remove(off_t value);

	private:
		bool FindInternal(off_t value,int32 &index) const;
};


inline int32
sorted_array::Find(off_t value) const
{
	int32 i;
	return FindInternal(value,i) ? i : -1;
}


// The BlockArray reserves a multiple of "blockSize" and
// maintain array size for new entries.
// This is used for the in-memory log entries before they
// are written to disk.

class BlockArray {
	public:
		BlockArray(int32 blockSize);
		~BlockArray();

		int32 Find(off_t value);
		status_t Insert(off_t value);
		status_t Remove(off_t value);

		void MakeEmpty();

		int32 CountItems() const { return fArray != NULL ? fArray->count : 0; }
		int32 BlocksUsed() const { return fArray != NULL ? ((fArray->count + 1) * sizeof(off_t) + fBlockSize - 1) / fBlockSize : 0; }
		sorted_array *Array() const { return fArray; }
		int32 Size() const { return fSize; }

	private:
		sorted_array *fArray;
		int32	fBlockSize;
		int32	fSize;
		int32	fMaxBlocks;
};


// Doubly linked list

template<class Node> struct node {
	Node *next,*prev;

	void
	Remove()
	{
		prev->next = next;
		next->prev = prev;
	}

	Node *
	Next()
	{
		if (next && next->next != NULL)
			return next;

		return NULL;
	}
};

template<class Node> struct list {
	Node *head,*tail,*last;

	list()
	{
		head = (Node *)&tail;
		tail = NULL;
		last = (Node *)&head;
	}

	void
	Add(Node *entry)
	{
		entry->next = (Node *)&tail;
		entry->prev = last;
		last->next = entry;
		last = entry;
	}
};


// Some atomic operations that are somehow missing in BeOS:
//
//	_atomic_test_and_set(value, newValue, testAgainst)
//		sets "value" to "newValue", if "value" is equal to "testAgainst"
//	_atomic_set(value, newValue)
//		sets "value" to "newValue"

#if _NO_INLINE_ASM
	// Note that these atomic versions *don't* work as expected!
	// They are only used for single processor user space tests
	// (and don't even work correctly there)
	inline int32
	_atomic_test_and_set(volatile int32 *value, int32 newValue, int32 testAgainst)
	{
		int32 oldValue = *value;
		if (oldValue == testAgainst)
			*value = newValue;

		return oldValue;
	}

	inline void
	_atomic_set(volatile int32 *value, int32 newValue)
	{
		*value = newValue;
	}
#elif __INTEL__
	inline int32
	_atomic_test_and_set(volatile int32 *value, int32 newValue, int32 testAgainst)
	{
		int32 oldValue;
		asm volatile("lock; cmpxchg %%ecx, (%%edx)"
			: "=a" (oldValue) : "a" (testAgainst), "c" (newValue), "d" (value));
		return oldValue;
	}

	inline void
	_atomic_set(volatile int32 *value, int32 newValue)
	{
		asm volatile("lock; xchg %%eax, (%%edx)"
			: : "a" (newValue), "d" (value));
	}
#elif __POWERPC__ && __MWERKS__ /* GCC has different assembler syntax */
inline asm int32
	_atomic_set(volatile int32 *value, int32)
	{
		loop:
			dcbf	r0, r3;
			lwarx	r0, 0, r3;
			stwcx.	r4, 0, r3;
			bc        5, 2, loop
		mr r3,r5;
		isync;
		blr;	
	}
	
inline asm int32
	_atomic_test_and_set(volatile int32 *value, int32 newValue, int32 testAgainst)
	{
		loop:
			dcbf	r0, r3;
			lwarx	r0, 0, r3;
			cmpw	r5, r0;
			bne		no_dice;
			stwcx.	r4, 0, r3;
			bc      5, 2, loop
			
		mr 		r3,r0;
		isync;
		blr;
		
		no_dice:
			stwcx.	r0, 0, r3;
			mr 		r3,r0;
			isync;
			blr;
	}
			
#else
#	error The macros _atomic_set(), and _atomic_test_and_set() are not defined for the target processor
#endif


extern "C" size_t strlcpy(char *dest, char const *source, size_t length);


#endif	/* UTILITY_H */
