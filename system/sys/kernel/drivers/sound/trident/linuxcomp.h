#ifndef _LINUXCOMP_H_
#define _LINUXCOMP_H_


typedef uint8 					u8;
typedef uint16 					u16;
typedef uint32					u32;
typedef uint64					u64;

typedef u32					dma_addr_t;

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
#define KERN_CRIT
#define KERN_NOTICE

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

#define spin_lock(x)			spinlock(x)
#define spin_unlock(x)			spinunlock(x)

#define spin_lock_irq(x)		spinlock(x)
#define spin_unlock_irq(x)		spinunlock(x)

#define spin_lock_irqsave(x, y)	spinlock_cli(x, y)
#define spin_unlock_irqrestore(x, y) spinunlock_restore(x, y)

#define up(x)					UNLOCK(x)
#define down(x)					LOCK(x)

#define assert(expr) \
        if(!(expr)) {					\
        printk( "Assertion failed! %s,%s,%s,line=%d\n",	\
        #expr,__FILE__,__FUNCTION__,__LINE__);		\
        }

#define PCI_CLASS_REVISION	0x08

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

static inline PCI_Info_s * pci_find_device(int vendor_id, int device_id, PCI_Info_s * pci_dev)
{
	int i;
	static PCI_Info_s  sInfo;
	PCI_bus_s* psBus = get_busmanager( PCI_BUS_NAME, PCI_BUS_VERSION );
	if( psBus == NULL )
		return( NULL );

	for ( i = 0 ; psBus->get_pci_info( &sInfo, i ) == 0 ; ++i )
	{
		if ( sInfo.nVendorID == vendor_id && sInfo.nDeviceID == device_id)
		{
			return &sInfo;
		}
	}
	return NULL;
}

#define pci_read_config_byte(dev, address, val) \
	val = psBus->read_pci_config(dev->nBus, dev->nDevice, dev->nFunction,\
	address, sizeof(val))
#define pci_read_config_dword(dev, address, val) \
	val = psBus->read_pci_config(dev->nBus, dev->nDevice, dev->nFunction,\
	address, sizeof(val))
#define pci_write_config_byte(dev, address, val) \
	psBus->write_pci_config(dev->nBus, dev->nDevice, dev->nFunction,\
	address, sizeof(val), (uint32)val)
#define pci_write_config_dword(dev, address, val) \
	psBus->write_pci_config(dev->nBus, dev->nDevice, dev->nFunction,\
	address, sizeof(val), (uint32)val)

#define pci_free_consistent(cookie, size, ptr, dma_ptr)	kfree(ptr)
#define pci_map_single(cookie, address, size, dir)	virt_to_bus(address)
#define pci_unmap_single(cookie, address, size, dir)

#endif
