#ifndef _LINUXCOMP_H_
#define _LINUXCOMP_H_


typedef uint8 					u8;
typedef uint16 					u16;
typedef uint32					u32;

typedef u32						dma_addr_t;

#define spinlock_t			SpinLock_s

#define virt_to_bus 			virt_to_phys
#define bus_to_virt 			phys_to_virt

#define cpu_to_le32(x) 			((u32)(x))
#define le32_to_cpu(x) 			((u32)(x))
#define cpu_to_le16(x) 			((u16)(x))
#define le16_to_cpu(x) 			((u16)(x))

#define copy_to_user(x, y, z) 	memcpy_to_user(x, y, z)
#define copy_from_user(x, y, z) memcpy_from_user(x, y, z)

#define put_user(x, y) 			do { *y = x; } while(0)
#define get_user(x, y) 			do { x = *y; } while(0)

#define KERN_INFO				
#define KERN_ERR 			
#define KERN_WARNING	
#define KERN_DEBUG	

#define _IOC_NRBITS	8
#define _IOC_TYPEBITS	8
#define _IOC_SIZEBITS	14
#define _IOC_DIRBITS	2

#define _IOC_NRMASK	((1 << _IOC_NRBITS)-1)
#define _IOC_TYPEMASK	((1 << _IOC_TYPEBITS)-1)
#define _IOC_SIZEMASK	((1 << _IOC_SIZEBITS)-1)
#define _IOC_DIRMASK	((1 << _IOC_DIRBITS)-1)

#define _IOC_NRSHIFT	0
#define _IOC_TYPESHIFT	(_IOC_NRSHIFT+_IOC_NRBITS)
#define _IOC_SIZESHIFT	(_IOC_TYPESHIFT+_IOC_TYPEBITS)
#define _IOC_DIRSHIFT	(_IOC_SIZESHIFT+_IOC_SIZEBITS)

#define _IOC_DIR(nr)		(((nr) >> _IOC_DIRSHIFT) & _IOC_DIRMASK)
#define _IOC_TYPE(nr)		(((nr) >> _IOC_TYPESHIFT) & _IOC_TYPEMASK)
#define _IOC_NR(nr)		(((nr) >> _IOC_NRSHIFT) & _IOC_NRMASK)
#define _IOC_SIZE(nr)		(((nr) >> _IOC_SIZESHIFT) & _IOC_SIZEMASK)

#define FMODE_READ				1	
#define FMODE_WRITE				2

#define atomic_read(x)			atomic_add(x, 0)
#define atomic_set(x, y)		atomic_swap(x, y)
#define atomic_inc(x)			atomic_add(x, 1)
#define atomic_dec(x)			atomic_dec_and_test(x)

#define spin_lock(x)			spinlock(x)
#define spin_unlock(x)			spinunlock(x)

#define spin_lock_irq(x)		spinlock(x)
#define spin_unlock_irq(x)		spinunlock(x)

#define spin_lock_irqsave(x, y)	spinlock_cli(x, y)
#define spin_unlock_irqrestore(x, y) spinunlock_restore(x, y)

extern inline unsigned long virt_to_phys(volatile void * address)
{
	return (unsigned long) address;
}

extern inline void * phys_to_virt(unsigned long address)
{
	return (void *) address;
}

static inline void *pci_alloc_consistent(PCI_Info_s *hwdev, size_t size,
					 dma_addr_t *dma_handle)
{
	void *virt_ptr;

	virt_ptr = kmalloc(size, MEMF_KERNEL);
	*dma_handle = virt_to_bus(virt_ptr);
	return virt_ptr;
}

#define pci_free_consistent(cookie, size, ptr, dma_ptr)	kfree(ptr)
#define pci_map_single(cookie, address, size, dir)	virt_to_bus(address)
#define pci_unmap_single(cookie, address, size, dir)

#endif

