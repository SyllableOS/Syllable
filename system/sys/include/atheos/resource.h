/*
 * Resource Manager for Syllable
 * Copyright (C) 2006 Jarek Pelczar
 *
 * Based on linux code by Linus Torvalds & Martin Mares
 *
 */

#ifndef __SYLLABLE__RESOURCE_H__
#define __SYLLABLE__RESOURCE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <atheos/types.h>

#ifdef __KERNEL__

typedef struct __Resource_s	Resource_s;

struct __Resource_s
{
    const char *	rc_pzName;
    uint		rc_nStart;
    uint		rc_nEnd;
    uint		rc_nFlags;
    Resource_s *	rc_psParent;
    Resource_s *	rc_psSibling;
    Resource_s *	rc_psChild;
};

/*
 * IO resources have these defined flags.
 */
#define IORESOURCE_BITS		0x000000ff	/* Bus-specific bits */

#define IORESOURCE_IO		0x00000100	/* Resource type */
#define IORESOURCE_MEM		0x00000200
#define IORESOURCE_IRQ		0x00000400
#define IORESOURCE_DMA		0x00000800

#define IORESOURCE_PREFETCH	0x00001000	/* No side effects */
#define IORESOURCE_READONLY	0x00002000
#define IORESOURCE_CACHEABLE	0x00004000
#define IORESOURCE_RANGELENGTH	0x00008000
#define IORESOURCE_SHADOWABLE	0x00010000
#define IORESOURCE_BUS_HAS_VGA	0x00080000
#define IORESOURCE_HEAP		0x00040000

#define IORESOURCE_DISABLED	0x10000000
#define IORESOURCE_UNSET	0x20000000
#define IORESOURCE_AUTO		0x40000000
#define IORESOURCE_BUSY		0x80000000	/* Driver has marked this resource busy */

#ifdef __BUILD_KERNEL__
void init_resource_manager();
void boot_reserve_io_region(uint nStart, uint nEnd, const char * pzName);
void boot_reserve_mem_region(uint nStart, uint nEnd, const char * pzName);
extern Resource_s g_sIOPortResource;
extern Resource_s g_sIOMemResource;
void __free_resource(Resource_s * psRoot, uint nStart, uint nReqMask);
#define request_region(start,len,name) \
	__request_region(&g_sIOPortResource,(start),(len),(name))
#define request_mem_region(start,len,name) \
	__request_region(&g_sIOMemResource,(start),(len),(name))
#define release_region(start,len) \
	__release_region(&g_sIOPortResource,(start),(len))
#define release_mem_region(start,len) \
	__release_region(&g_sIOMemResource,(start),(len))
#else
#define request_region(start,len,name) \
	__request_region(ioport_root_resource(),(start),(len),(name))
#define request_mem_region(start,len,name) \
	__request_region(memory_root_resource(),(start),(len),(name))
#define release_region(start,len) \
	__release_region(ioport_root_resource(),(start),(len))
#define release_mem_region(start,len) \
	__release_region(memory_root_resource(),(start),(len))
#endif

#define rename_region(r,n) do { (r)->rc_pzName = (n); } while(0)

Resource_s * ioport_root_resource(void);
Resource_s * memory_root_resource(void);
int request_resource(Resource_s * psRoot, Resource_s * psNew);
Resource_s * ____request_resource(Resource_s * psRoot, Resource_s * psNew);
int release_resource(Resource_s * psOld);
int allocate_resource(Resource_s * psRoot, Resource_s * psNew,
		      uint nSize, uint nMin, uint nMax, uint nAlign, 
		      void (* pfnAlign)(void * pData, Resource_s * psRes, uint nSize, uint nAlign),
		      void * pAlignData);
int insert_resource(Resource_s * psParent, Resource_s * psNew);
int adjust_resource(Resource_s * psRes, uint nStart, uint nSize);
Resource_s * __request_region(Resource_s * psParent, uint nStart, uint nSize, const char * pzName);
void __release_region(Resource_s * psParent, uint nStart, uint nSize);

#endif /* __KERNEL__ */

#ifdef __cplusplus
}
#endif

#endif


