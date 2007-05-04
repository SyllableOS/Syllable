/*
 * Resource Manager for Syllable
 * Copyright (C) 2006 Jarek Pelczar
 *
 * Based on linux code by Linus Torvalds & Martin Mares
 *
 */

#include <atheos/types.h>
#include <atheos/kernel.h>
#include <atheos/spinlock.h>
#include <atheos/kernel.h>
#include <posix/errno.h>
#include <atheos/resource.h>
#include <atheos/semaphore.h>
#include <macros.h>
#include "inc/sysbase.h"

/** \struct Resource_s
 *  \ingroup DriverAPI
 *  \brief Descriptor of the resource
 *  \par Description:
 *    Resource_s represents resource region of any type. It is used
 *    by kernel to manage physical memory and I/O regions. Resources
 *    are stored in trees. Each resource may have parent, children
 *    and sibling resources. Also field that contains resource flags
 *    is provided (e.g. to mark that given region is in the read-only
 *    or cacheable region.
 */

Resource_s g_sIOPortResource = {
    .rc_pzName		= "IO Port",
    .rc_nStart		= 0,
    .rc_nEnd		= 0xffff,
    .rc_nFlags		= IORESOURCE_IO
};

Resource_s g_sIOMemResource = {
    .rc_pzName		= "Memory",
    .rc_nStart		= 0,
    .rc_nEnd		= ~0,
    .rc_nFlags		= IORESOURCE_MEM
};

static sem_id g_hResourceLock;

#define MAX_BOOT_RESERVE	64

static Resource_s * __request_resource(Resource_s * psRoot, Resource_s * psNew);

static Resource_s	s_sBootReserve[MAX_BOOT_RESERVE];
static int		s_sBootReserveCounter = 0;

//****************************************************************************/
/** Reserves I/O port region at boot time.
 * \ingroup DriverAPI
 * \param nStart start of the area
 * \param nEnd end of the area
 * \param pzName name of the area
 * \sa boot_reserve_mem_region()
 *****************************************************************************/
void boot_reserve_io_region(uint nStart, uint nEnd, const char * pzName)
{
    Resource_s * psRes = s_sBootReserve + s_sBootReserveCounter;
    psRes->rc_pzName = pzName;
    psRes->rc_nStart = nStart;
    psRes->rc_nEnd = nEnd;
    psRes->rc_nFlags = IORESOURCE_IO | IORESOURCE_BUSY;
    if(!__request_resource(&g_sIOPortResource, psRes))
	s_sBootReserveCounter++;
}

//****************************************************************************/
/** Reserves physical memory region at boot time.
 * \ingroup DriverAPI
 * \param nStart start of the area
 * \param nEnd end of the area
 * \param pzName name of the area
 * \sa boot_reserve_io_region()
 *****************************************************************************/
void boot_reserve_mem_region(uint nStart, uint nEnd, const char * pzName)
{
    Resource_s * psRes = s_sBootReserve + s_sBootReserveCounter;
    psRes->rc_pzName = pzName;
    psRes->rc_nStart = nStart;
    psRes->rc_nEnd = nEnd;
    psRes->rc_nFlags = IORESOURCE_MEM | IORESOURCE_BUSY;
    if(!__request_resource(&g_sIOMemResource, psRes))
	s_sBootReserveCounter++;
}

static inline void print_rsrc_flags(uint nFlags, char * buf)
{
    buf[0] = 0;
    if(nFlags & IORESOURCE_IO) strcat(buf," IO");
    if(nFlags & IORESOURCE_MEM) strcat(buf," MEM");
    if(nFlags & IORESOURCE_IRQ) strcat(buf," IRQ");
    if(nFlags & IORESOURCE_DMA) strcat(buf," DMA");
    if(nFlags & IORESOURCE_PREFETCH) strcat(buf," PREFETCH");
    if(nFlags & IORESOURCE_READONLY) strcat(buf," READONLY");
    if(nFlags & IORESOURCE_CACHEABLE) strcat(buf," CACHEABLE");
    if(nFlags & IORESOURCE_SHADOWABLE) strcat(buf," SHADOWABLE");
    if(nFlags & IORESOURCE_BUS_HAS_VGA) strcat(buf," BUS_HAS_VGA");
    if(nFlags & IORESOURCE_DISABLED) strcat(buf," DISABLED");
    if(nFlags & IORESOURCE_UNSET) strcat(buf," UNSET");
    if(nFlags & IORESOURCE_AUTO) strcat(buf," AUTO");
    if(nFlags & IORESOURCE_BUSY) strcat(buf," BUSY");
    if(nFlags & IORESOURCE_HEAP) strcat(buf," HEAP");
}

static inline void __dump_resource(Resource_s * psRes, int nLevel, char * tmp, char * buf)
{
    int i;
    Resource_s * psTmp;
    nLevel &= 31;
    for(i = 0 ; i < nLevel ; i++)
    {
	tmp[i] = ' ';
    }
    tmp[i++] = '|';
    tmp[i++] = '-';
    tmp[i++] = 0;
    print_rsrc_flags(psRes->rc_nFlags, buf);
    printk("%s %x-%x (flags %s, low = %02x) : %s\n",tmp,psRes->rc_nStart,psRes->rc_nEnd,
	    buf, psRes->rc_nFlags & IORESOURCE_BITS,
	    psRes->rc_pzName ? psRes->rc_pzName : "<bad>");
    
    for(psTmp = psRes->rc_psChild;psTmp;psTmp=psTmp->rc_psSibling)
	__dump_resource(psTmp,nLevel+1,tmp,buf);
}

void dump_resource(Resource_s * psRes)
{
    char tmp[40];
    char buf[128];
    __dump_resource(psRes,0,tmp,buf);
}

void init_resource_manager(void)
{
    g_sIOMemResource.rc_nEnd = g_sSysBase.sb_nLastUserAddress;        
    g_hResourceLock = CREATE_RWLOCK("resource_lock");

   	boot_reserve_io_region(0x0000, 0x001f, "DMA1");
	boot_reserve_io_region(0x0020, 0x002f, "PIC1");
	boot_reserve_io_region(0x0040, 0x0043, "TIMER0");
	boot_reserve_io_region(0x0050, 0x0053, "TIMER1");
	boot_reserve_io_region(0x0060, 0x006f, "KEYBOARD");
	boot_reserve_io_region(0x0070, 0x0077, "RTC");
	boot_reserve_io_region(0x0080, 0x008f, "DMA_PAGE_REG");
	boot_reserve_io_region(0x00a0, 0x00a1, "PIC2");
	boot_reserve_io_region(0x00c0, 0x00df, "DMA2");
	boot_reserve_io_region(0x00f0, 0x00ff, "FPU");
	boot_reserve_io_region(0x03c0, 0x03df, "VGA");
    
}

static Resource_s * __request_resource(Resource_s * psRoot, Resource_s * psNew)
{
    uint nStart = psNew->rc_nStart;
    uint nEnd = psNew->rc_nEnd;
    Resource_s * psTmp, ** ppsNode;
    if(nEnd < nStart) return psRoot;
    if(nStart < psRoot->rc_nStart) return psRoot;
    if(nEnd > psRoot->rc_nEnd) return psRoot;
    
    ppsNode = &psRoot->rc_psChild;
    for(;;) {
	psTmp = *ppsNode;
	if(!psTmp || psTmp->rc_nStart > nEnd) {
	    psNew->rc_psSibling = psTmp;
	    *ppsNode = psNew;
	    psNew->rc_psParent = psRoot;
	    return NULL;
	}
	ppsNode = &psTmp->rc_psSibling;
	if(psTmp->rc_nEnd < nStart) continue;
	return psTmp;
    }
}

static int __release_resource(Resource_s * psOld)
{
    Resource_s * psTmp, ** ppsNode;
    ppsNode = &psOld->rc_psParent->rc_psChild;
    for(;;)
    {
	psTmp = *ppsNode;
	if(!psTmp) break;
	if(psTmp == psOld) {
	    *ppsNode = psTmp->rc_psSibling;
	    psOld->rc_psParent = NULL;
	    return 0;
	}
	ppsNode = &psTmp->rc_psSibling;
    }
    return -EINVAL;
}


//****************************************************************************/
/** Return pointer to the toplevel resource tree for I/O ports. All drivers
 *  which register I/O port, register them directly in this tree.
 *
 * \warning <b>Never</b> modify this structure.
 * \ingroup DriverAPI
 * \return Pointer to the resource descriptor.
 *****************************************************************************/
Resource_s * ioport_root_resource(void)
{
    return &g_sIOPortResource;
}

//****************************************************************************/
/** Return pointer to the toplevel resource tree for physical memory areas. All drivers
 *  which register memory regions use this tree (e.g. the PCI driver).
 *
 * \warning <b>Never</b> modify this structure.
 * \ingroup DriverAPI
 * \return Pointer to the resource descriptor.
 *****************************************************************************/
Resource_s * memory_root_resource(void)
{
    return &g_sIOMemResource;
}


//****************************************************************************/
/** Register resource in the specified resource tree. This function checks
 *  for resource collisions.
 *
 * \param psRoot pointer to the root of the resource tree
 * \param psNew pointer to the resource descriptor to be inserted
 * \return 0 if resource was inserted successfuly
 * \return -EBUSY if there was resource conflict
 * \ingroup DriverAPI
 * \return Pointer to the resource descriptor.
 * \sa ____request_resource()
 *****************************************************************************/
int request_resource(Resource_s * psRoot, Resource_s * psNew)
{
    Resource_s * psConflict;
    RWL_LOCK_RW(g_hResourceLock);
    psConflict = __request_resource(psRoot, psNew);
    RWL_UNLOCK_RW(g_hResourceLock);
    return psConflict ? -EBUSY : 0;
}


//****************************************************************************/
/** This function works similar to request_resource(), but it returns pointer
 *  to the conflicting resource.
 *
 * \param psRoot pointer to the root of the resource tree
 * \param psNew pointer to the resource descriptor to be inserted
 * \ingroup DriverAPI
 * \return NULL if resource was inserted
 * \return Pointer to the conflicting resource descriptor.
 *****************************************************************************/
Resource_s * ____request_resource(Resource_s * psRoot, Resource_s * psNew)
{
    Resource_s * psConflict;
    RWL_LOCK_RW(g_hResourceLock);
    psConflict = __request_resource(psRoot, psNew);
    RWL_UNLOCK_RW(g_hResourceLock);
    return psConflict;
}

//****************************************************************************/
/** Removes source from the resource tree. No memory is freed.
 * \ingroup DriverAPI
 * \param psOld pointer to the resource descriptor
 * \return 0 if descriptor was removed
 * \return -EINVAL if there was memory corruption
 * \sa request_resource()
 *****************************************************************************/

int release_resource(Resource_s * psOld)
{
    int retval;
    RWL_LOCK_RW(g_hResourceLock);
    retval = __release_resource(psOld);
    RWL_UNLOCK_RW(g_hResourceLock);
    return retval;
}

static int find_resource(Resource_s * psRoot, Resource_s * psNew,
		         uint nSize, uint nMin, uint nMax, uint nAlign,
			 void (* pfnAlign)(void * pData, Resource_s * psRes, uint nSize, uint nAlign),
			 void * pAlignData)
{
    Resource_s * psThis = psRoot->rc_psChild;
    
    if(psThis && psThis->rc_nStart == 0)
    {
	psNew->rc_nStart = psThis->rc_nEnd + 1;
	psThis = psThis->rc_psSibling;
    }
    
    for(;;)
    {
	if(psThis)
	    psNew->rc_nEnd = psThis->rc_nStart - 1;
	else
	    psNew->rc_nEnd = psRoot->rc_nEnd;
#define _ALIGN(x,a) (((x)+(a)-1)&~((a)-1))
	if(psNew->rc_nStart < nMin)
	    psNew->rc_nStart = nMin;
	if(psNew->rc_nEnd > nMax)
	    psNew->rc_nEnd = nMax;
	psNew->rc_nStart = _ALIGN(psNew->rc_nStart, nAlign);
	if(pfnAlign)
	    pfnAlign(pAlignData, psNew, nSize, nAlign);
	if(psNew->rc_nStart < psNew->rc_nEnd && psNew->rc_nEnd - psNew->rc_nStart >= nSize - 1)
	{
	    psNew->rc_nEnd = psNew->rc_nStart + nSize - 1;
	    return 0;
	}
	if(!psThis) break;
	psNew->rc_nStart = psThis->rc_nEnd + 1;
	psThis = psThis->rc_psSibling;
    }
    return -EBUSY;
}

//****************************************************************************/
/** This function allocates resource in the resource tree. It searches for the
 *  free area of the specified size and alignment. User has ability to modify
 *  resource alignment by the pfnAlign function. After the resource allocation,
 *  specified region is claimed in the resource tree.
 *
 * \ingroup DriverAPI
 * \param psRoot root resource node which we want to allocate resource in
 * \param psNew pointer to the new resource descriptor
 * \param nSize requested size
 * \param nMin minimal address
 * \param nMax maximal resource address
 * \param nAlign starting address alignment
 * \param pfnAlign user alignment function (optional)
 * \param pAlignData data passed to pfnAlign
 * \return 0 if resource was allocated
 * \return -EBUSY if there is no free resource with the specified constraints
 * \sa request_resource()
 *****************************************************************************/

int allocate_resource(Resource_s * psRoot, Resource_s * psNew,
		      uint nSize, uint nMin, uint nMax, uint nAlign, 
		      void (* pfnAlign)(void * pData, Resource_s * psRes, uint nSize, uint nAlign),
		      void * pAlignData)
{
    int nErr;
    RWL_LOCK_RW(g_hResourceLock);
    nErr = find_resource(psRoot, psNew, nSize, nMin, nMax, nAlign, pfnAlign, pAlignData);
    if(nErr>=0 && __request_resource(psRoot, psNew))
	nErr = -EBUSY;
    RWL_UNLOCK_RW(g_hResourceLock);
    return nErr;
}

//****************************************************************************/
/** Insert resource into the resource tree. This function works similar to
 *  request_resource(), when no resource conflict happens. If a conflict happens,
 *  and the conflicting resources fit entirely in the psNew resource range,
 *  resource psNew becomes new parent of the conflicting resources. Otherwise
 *  it becomes child of the conflicting resource.
 *
 * \ingroup DriverAPI
 * \param psParent resource tree root
 * \param psNew pointer to the new resource to be inserted
 * \return 0 if everything went OK
 * \return -EBUSY when function is unable to insert resource
 * \sa request_resource(), ____request_resource()
 *****************************************************************************/

int insert_resource(Resource_s * psParent, Resource_s * psNew)
{
    int nResult;
    Resource_s * psFirst, * psNext;
    RWL_LOCK_RW(g_hResourceLock);
begin:
    nResult = 0;
    psFirst = __request_resource(psParent, psNew);
    if(!psFirst) goto out;
    nResult = -EBUSY;
    if(psFirst == psParent) goto out;
    if(psFirst->rc_nStart <= psNew->rc_nStart && psFirst->rc_nEnd >= psNew->rc_nEnd)
    {
	psParent = psFirst;
	goto begin;
    }
    for(psNext=psFirst;;psNext=psNext->rc_psSibling)
    {
	if(psNext->rc_nStart < psNew->rc_nStart || psNext->rc_nEnd > psNew->rc_nEnd) goto out;
	if(!psNext->rc_psSibling) break;
	if(psNext->rc_psSibling->rc_nStart > psNew->rc_nEnd) break;
    }
    
    nResult = 0;
    
    psNew->rc_psParent = psParent;
    psNew->rc_psSibling = psNext->rc_psSibling;
    psNew->rc_psChild = psFirst;
    psNext->rc_psSibling = NULL;
    for(psNext = psFirst ; psNext ; psNext = psNext->rc_psSibling)
	psNext->rc_psParent = psNew;
    if(psParent->rc_psChild == psFirst)
    {
	psParent->rc_psChild = psNew;
    } else {
	psNext = psParent->rc_psChild;
	while(psNext->rc_psSibling != psFirst)
	    psNext = psNext->rc_psSibling;
	psNext->rc_psSibling = psNext;
    }
out:
    RWL_UNLOCK_RW(g_hResourceLock);
    return nResult;
}

//****************************************************************************/
/** Adjust new resource area for the given base and size. Children areas
 *  aren't touched. If children wouldn't fit in the new rage, error is
 *  returned.
 * \ingroup DriverAPI
 * \param psRes resource to be modified
 * \param nStart start of the resource
 * \param nEnd end of the resource
 * \return 0 if everything went OK
 * \return -EBUSY if resource won't fit
 *****************************************************************************/

int adjust_resource(Resource_s * psRes, uint nStart, uint nSize)
{
    Resource_s * psTmp,* psParent = psRes->rc_psParent;
    uint nEnd = nStart + nSize - 1;
    int nResult = -EBUSY;

    RWL_LOCK_RW(g_hResourceLock);
    
    if((nStart < psParent->rc_nStart) || (nEnd > psParent->rc_nEnd))
	goto out;

    for(psTmp = psRes->rc_psChild ; psTmp ; psTmp = psTmp->rc_psSibling)
    {
	if((psTmp->rc_nStart < nStart) || (psTmp->rc_nEnd > nEnd))
	    goto out;
    }
    
    if(psRes->rc_psSibling && (psRes->rc_psSibling->rc_nStart <= nEnd))
	goto out;

    psTmp = psParent->rc_psChild;
    if(psTmp != psRes)
    {
	while(psTmp->rc_psSibling != psRes)
	    psTmp = psTmp->rc_psSibling;
	if(nStart <= psTmp->rc_nEnd) goto out;
    }
    
    psRes->rc_nStart = nStart;
    psRes->rc_nEnd = nEnd;
    nResult = 0;
out:
    RWL_UNLOCK_RW(g_hResourceLock);
    return nResult;
}

//****************************************************************************/
/** Request region in the given resource tree. Caller provides region area and
 *  region name. Resource descriptor is automatically allocated. New resource is
 *  claimed.
 *
 * \ingroup DriverAPI
 * \param psParent parent resource
 * \param nStart beginning of the resource area
 * \param nSize size of the resource
 * \param pzName name of the resource
 * \return NULL if it was unable to claim resource
 * \return new resource descriptor
 * \sa ____request_resource(), request_resource()
 *****************************************************************************/

Resource_s * __request_region(Resource_s * psParent, uint nStart, uint nSize, const char * pzName)
{
    Resource_s * psRes = kmalloc(sizeof(Resource_s),MEMF_KERNEL|MEMF_OKTOFAIL|MEMF_CLEAR);
    if(psRes)
    {
	psRes->rc_pzName = pzName;
	psRes->rc_nStart = nStart;
	psRes->rc_nEnd = nStart + nSize - 1;
	psRes->rc_nFlags = IORESOURCE_BUSY;
	
	RWL_LOCK_RW(g_hResourceLock);
	
	for(;;)
	{
	    Resource_s * psConflict;
	    psConflict = __request_resource(psParent, psRes);
	    if(!psConflict) break;
	    if(psConflict != psParent) {
		psParent = psConflict;
		if(!(psConflict->rc_nFlags & IORESOURCE_BUSY))
		    continue;
	    }
	    kfree(psRes);
	    psRes = NULL;
	    break;
	}
	
	RWL_UNLOCK_RW(g_hResourceLock);
    }
    return psRes;
}

//****************************************************************************/
/** Release region in the resource tree. Specified region is searched for in
 *  the resource tree. If the region wasn't found, function reports warning by
 *  the kernel message. Corresponding resource descriptor is removed and freed.
 *
 * \ingroup DriverAPI
 * \param psParent root resource
 * \param nStart start of the resource
 * \param nEnd end of thre resource
 * \sa release_resource()
 *****************************************************************************/

void __release_region(Resource_s * psParent, uint nStart, uint nSize)
{
    Resource_s ** p = &psParent->rc_psChild;
    uint nEnd = nStart + nSize - 1;

    RWL_LOCK_RW(g_hResourceLock);
    for(;;)
    {
	Resource_s * psRes = *p;
	if(!psRes) break;
	if(psRes->rc_nStart <= nStart && psRes->rc_nEnd >= nEnd) {
	    if(!(psRes->rc_nFlags & IORESOURCE_BUSY))
	    {
		p = &psRes->rc_psChild;
		continue;
	    }
	    if(psRes->rc_nStart != nStart || psRes->rc_nEnd != nEnd) break;
	    *p = psRes->rc_psSibling;
	    RWL_UNLOCK_RW(g_hResourceLock);
	    kfree(psRes);
	    return;
	}
    }
    RWL_UNLOCK_RW(g_hResourceLock);
    printk("Trying to free nonexistent region %x-%x\n",nStart, nEnd);
}

//****************************************************************************/
/** Free resource which starts in the 1st level of the root children. Only
 *  resources that match given mask are freed. This is useful for free() like
 *  function when using resource manager to manage allocatable heaps.
 * \ingroup DriverAPI
 * \param psRoot parent resource
 * \param nStart address of the beginning of the resource to be freed
 * \param nReqMask required bits must be set in the resource flags
 * \sa __release_region(), release_resource()
 *****************************************************************************/

void __free_resource(Resource_s * psRoot, uint nStart, uint nReqMask)
{
    RWL_LOCK_RW(g_hResourceLock);
    Resource_s * psRes;
    for(psRes = psRoot->rc_psChild ; psRes ; psRes=psRes->rc_psSibling)
    {
	if(psRes->rc_nStart == nStart && (psRes->rc_nFlags & nReqMask)==nReqMask)
	{
	    __release_resource(psRes);
	    kfree(psRes);
	    break;
	}
    }
    RWL_UNLOCK_RW(g_hResourceLock);
}


