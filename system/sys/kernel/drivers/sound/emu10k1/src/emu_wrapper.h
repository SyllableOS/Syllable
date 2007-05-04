#ifndef __EMU_WRAPPER_H
#define __EMU_WRAPPER_H

#include <atheos/kernel.h>
#include <atheos/pci.h>
#include <atheos/types.h>
#include <atheos/soundcard.h>
#include <posix/errno.h>

typedef uint8 u8;
typedef uint16 u16;
typedef uint32 u32;
typedef int8 s8;
typedef int16 s16;
typedef int32 s32;

#define spinlock_t SpinLock_s

#define GFP_KERNEL	MEMF_KERNEL
#define copy_from_user	memcpy_from_user
#define copy_to_user	memcpy_to_user

#include "list.h"

#define UP_INODE_SEM(a)
#define DOWN_INODE_SEM(a)

#define GET_INODE_STRUCT()

#define vma_get_pgoff(v)	((v)->vm_pgoff)

#define min_t(type,x,y) \
	({ type __x = (x); type __y = (y); __x < __y ? __x: __y; })

typedef uint32 dma_addr_t;

int pci_set_dma_mask(PCI_Info_s *dev, dma_addr_t mask);

static inline unsigned long virt_to_bus(volatile void *addr)
{
	return (unsigned long) addr;
}

extern uint32 get_free_pages( int nPageCount, int nFlags ); // This should probably be in kernel.h
extern void free_pages( uint32 nPages, int nCount ); // This should probably be in kernel.h

static inline void* alloc_dma_mem(PCI_Info_s *pdev, size_t size, dma_addr_t *bus_addr)
{

	uint32 mem_addr = get_free_pages( PAGE_ALIGN(size) >> PAGE_SHIFT, MEMF_CLEAR );
	*bus_addr=virt_to_bus((void*)mem_addr);

	return (void*)mem_addr;
}

#define free_dma_mem(cookie, size, ptr, dma_ptr)	free_pages( (uint32)ptr, PAGE_ALIGN(size) >> PAGE_SHIFT );

static inline int pci_read_config_byte(PCI_Info_s *pdev, int where, uint8 *ptr)
{
	PCI_bus_s* psBus = get_busmanager( PCI_BUS_NAME, PCI_BUS_VERSION );
	*ptr=psBus->read_pci_config(pdev->nBus, pdev->nDevice, pdev->nFunction, where, sizeof(uint8));
	return EOK;
}

static inline int pci_read_config_word(PCI_Info_s *pdev, int where, uint16 *ptr)
{
	PCI_bus_s* psBus = get_busmanager( PCI_BUS_NAME, PCI_BUS_VERSION );
	*ptr=psBus->read_pci_config(pdev->nBus, pdev->nDevice, pdev->nFunction, where, sizeof(uint16));
	return EOK;
}

static inline int pci_read_config_dword(PCI_Info_s *pdev, int where, uint32 *ptr)
{
	PCI_bus_s* psBus = get_busmanager( PCI_BUS_NAME, PCI_BUS_VERSION );
	*ptr=psBus->read_pci_config(pdev->nBus, pdev->nDevice, pdev->nFunction, where, sizeof(uint32));
	return EOK;
}

/* Used to debug audio_ioctl */
static inline void debug_ioctl(uint32 cmd)
{
	printk("emu10k1_audio_ioctl() : Unknown command ");

	switch(cmd)
	{
		case OSS_GETVERSION:
			printk("OSS_GETVERSION\n");
			break;

		case SNDCTL_DSP_RESET:
			printk("SNDCTL_DSP_RESET\n");
			break;

		case SNDCTL_DSP_SYNC:
			printk("SNDCTL_DSP_SYNC\n");
			break;

		case SNDCTL_DSP_GETCAPS:
			printk("SNDCTL_DSP_GETCAPS\n");
			break;

		case SNDCTL_DSP_SPEED:
			printk("SNDCTL_DSP_SPEED\n");
			break;

		case SNDCTL_DSP_STEREO:
			printk("SNDCTL_DSP_STEREO\n");
			break;

		case SNDCTL_DSP_CHANNELS:
			printk("SNDCTL_DSP_CHANNELS\n");
			break;

		case SNDCTL_DSP_GETFMTS:
			printk("SNDCTL_DSP_GETFMTS\n");
			break;

		case SNDCTL_DSP_SETFMT:
			printk("SNDCTL_DSP_SETFMT\n");
			break;

		case SOUND_PCM_READ_BITS:
			printk("SOUND_PCM_READ_BITS\n");
			break;

		case SOUND_PCM_READ_RATE:
			printk("SOUND_PCM_READ_RATE\n");
			break;

		case SOUND_PCM_READ_CHANNELS:
			printk("SOUND_PCM_READ_CHANNELS\n");
			break;

		case SNDCTL_DSP_GETTRIGGER:
			printk("SNDCTL_DSP_GETTRIGGER\n");
			break;

		case SNDCTL_DSP_GETOSPACE:
			printk("SNDCTL_DSP_GETOSPACE\n");
			break;

		case SNDCTL_DSP_GETISPACE:
			printk("SNDCTL_DSP_GETISPACE\n");
			break;

		case SNDCTL_DSP_GETODELAY:
			printk("SNDCTL_DSP_GETODELAY\n");
			break;

		case SNDCTL_DSP_GETIPTR:
			printk("SNDCTL_DSP_GETIPTR\n");
			break;

		case SNDCTL_DSP_GETOPTR:
			printk("SNDCTL_DSP_GETOPTR\n");
			break;

		case SNDCTL_DSP_GETBLKSIZE:
			printk("SNDCTL_DSP_GETBLKSIZ\n");
			break;

		case SNDCTL_DSP_POST:
			printk("SNDCTL_DSP_POST\n");
			break;

		case SNDCTL_DSP_SETFRAGMENT:
			printk("SNDCTL_DSP_SETFRAGMENT\n");
			break;

		case SNDCTL_COPR_LOAD:
			printk("SNDCTL_COPR_LOAD\n");
			break;

		default:
			printk("0x%08X\n",(int)cmd);
	}

	return;
}

#endif /* __EMU_WRAPPER_H */








