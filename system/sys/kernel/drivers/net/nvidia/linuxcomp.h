#ifndef _LINUXCOMP_H_
#define _LINUXCOMP_H_


typedef uint8 					u8;
typedef uint16					u16;
typedef uint32					u32;
typedef uint64					u64;

typedef int8					s8;
typedef int16					s16;
typedef int32					s32;
typedef int64					s64;

typedef uint32					dma_addr_t;

#define EISA_bus 0

#define net_device device

#define jiffies     (get_system_time()/1000LL)
#define HZ          1000
#define JIFF2U(a)   ((a)*1000LL)    /* Convert time in JIFFIES to micro sec. */

#define virt_to_bus 			virt_to_phys
#define bus_to_virt 			phys_to_virt

#define cpu_to_le32(x) 			((uint32)(x))
#define le32_to_cpu(x) 			((uint32)(x))
#define cpu_to_le16(x) 			((uint16)(x))
#define le16_to_cpu(x) 			((uint16)(x))

#define virt_to_le32desc(addr)  cpu_to_le32(virt_to_bus(addr))
#define le32desc_to_virt(addr)  bus_to_virt(le32_to_cpu(addr))

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
#define KERN_ALERT

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

#define up(x)					UNLOCK(x)
#define down(x)					LOCK(x)

#define IFF_RUNNING 0x40
#define netif_wake_queue(dev)   do { clear_bit(0, (void*)&dev->tbusy); } while(0)
#define netif_start_queue(dev)  clear_bit(0, (void*)&dev->tbusy)
#define netif_start_tx_queue(dev)  do { (dev)->tbusy = 0; dev->start = 1; } while(0)
#define netif_stop_tx_queue(dev)  do { (dev)->tbusy = 1; dev->start = 0; } while(0)
#define netif_stop_queue(dev)   set_bit(0, (void*)&dev->tbusy)
#define netif_queue_paused(dev)	((dev)->tbusy != 0)
#define netif_pause_tx_queue(dev)	(test_and_set_bit(0, (void*)&dev->tbusy))
#define netif_unpause_tx_queue(dev)	do { clear_bit(0, (void*)&dev->tbusy); } while(0)
#define netif_resume_tx_queue(dev)	do { clear_bit(0, (void*)&dev->tbusy); } while(0)
#define netif_running(dev)	((dev)->start != 0)
#define netif_mark_up(dev)	do { (dev)->start = 1; } while (0)
#define netif_mark_down(dev)	do { (dev)->start = 0; } while (0)
#define netif_queue_stopped(dev)	((dev)->tbusy)
#define netif_link_down(dev)	(dev)->flags &= ~ IFF_RUNNING
#define netif_link_up(dev)	(dev)->flags |= IFF_RUNNING
#define netif_carrier_on(dev) netif_link_up(dev)
#define netif_carrier_off(dev) netif_link_down(dev)
#define netif_carrier_ok(dev) ((dev)->flags & IFF_RUNNING)

#define disable_irq disable_irq_nosync

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
	PCI_bus_s *psBus = get_busmanager( PCI_BUS_NAME, PCI_BUS_VERSION );
	if( psBus == NULL )
		return NULL;

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
	val = g_psBus->read_pci_config(dev->nBus, dev->nDevice, dev->nFunction,\
	address, sizeof(val))
#define pci_read_config_word(dev, address, val) \
	val = g_psBus->read_pci_config(dev->nBus, dev->nDevice, dev->nFunction,\
	address, sizeof(val))
#define pci_read_config_dword(dev, address, val) \
	val = g_psBus->read_pci_config(dev->nBus, dev->nDevice, dev->nFunction,\
	address, sizeof(val))
#define pci_write_config_byte(dev, address, val) \
	g_psBus->write_pci_config(dev->nBus, dev->nDevice, dev->nFunction,\
	address, sizeof(val), (uint32)val)
#define pci_write_config_word(dev, address, val) \
	g_psBus->write_pci_config(dev->nBus, dev->nDevice, dev->nFunction,\
	address, sizeof(val), (uint32)val)
#define pci_write_config_dword(dev, address, val) \
	g_psBus->write_pci_config(dev->nBus, dev->nDevice, dev->nFunction,\
	address, sizeof(val), (uint32)val)

#define pci_free_consistent(cookie, size, ptr, dma_ptr)	kfree(ptr)
#define pci_map_single(cookie, address, size, dir)	virt_to_bus(address)
#define pci_unmap_single(cookie, address, size, dir)

inline uint32 pci_size(uint32 base, uint32 mask)
{
	uint32 size = base & mask;
	return size & ~(size-1);
}

inline uint32 get_pci_memory_size(PCI_Info_s *pcPCIInfo, int nResource)
{
	int nBus = pcPCIInfo->nBus;
	int nDev = pcPCIInfo->nDevice;
	int nFnc = pcPCIInfo->nFunction;
	int nOffset = PCI_BASE_REGISTERS + nResource * 4;
	uint32 nBase = g_psBus->read_pci_config(nBus, nDev, nFnc, nOffset, 4);
	
	g_psBus->write_pci_config(nBus, nDev, nFnc, nOffset, 4, ~0);
	uint32 nSize = g_psBus->read_pci_config(nBus, nDev, nFnc, nOffset, 4);
	g_psBus->write_pci_config(nBus, nDev, nFnc, nOffset, 4, nBase);
	if (!nSize || nSize == 0xffffffff) return 0;
	if (nBase == 0xffffffff) nBase = 0;
	if (!(nSize & PCI_ADDRESS_SPACE)) {
		return pci_size(nSize, PCI_ADDRESS_MEMORY_32_MASK);
	} else {
		return pci_size(nSize, PCI_ADDRESS_IO_MASK & 0xffff);
	}
}

#define ALIGN( p, m)  (  (typeof(p)) (((uint32)(p)+m)&~m)  )
#define ALIGN8( p )   ALIGN(p,7)
#define ALIGN16( p )  ALIGN(p,15)
#define ALIGN32( p )  ALIGN(p,31)

/* To align packets. */
PacketBuf_s *alloc_pkt_buffer_aligned(int nSize) {
    PacketBuf_s *skb;
        
    skb = alloc_pkt_buffer( nSize+15 );
    if (skb)
        skb->pb_pData = ALIGN16( skb->pb_pData );
    
    return skb;
}

/* Needed because Athe does not have skb, guess atheos never will get it either */
void* skb_put( PacketBuf_s* psBuffer, int nSize )
{
    void* pOldEnd = psBuffer->pb_pHead + psBuffer->pb_nSize;
    
    psBuffer->pb_nSize += nSize;
    return( pOldEnd );
}

#define writel(b,addr) ((*(volatile unsigned int *) (addr)) = (b))
#define readl(addr) ((*(volatile unsigned int *) (addr)))

/* FIXME: I'm not sure if this is correct */
static inline void *ioremap(unsigned long phys_addr, unsigned long size) {
    return (void *)phys_addr;
}

#define PCI_ANY_ID (~0)

struct pci_device_id {
    unsigned int vendor_id, device_id;      /* Vendor and device ID or PCI_ANY_ID */
    unsigned int subvendor, subdevice;  /* Subsystem ID's or PCI_ANY_ID */
    unsigned int class, class_mask;     /* (class,subclass,prog-if) triplet */
    unsigned long driver_data;      /* Data private to the driver */
};

static inline void *request_region(unsigned long start, unsigned long len, char *name) {
    return (void *)1;    // not-NULL value
}

static inline void release_region(unsigned long start, unsigned long len) {
}

#define PCIBIOS_MAX_LATENCY 255

#endif
