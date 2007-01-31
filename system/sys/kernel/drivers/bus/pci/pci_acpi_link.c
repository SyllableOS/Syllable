
/*
 *  The Syllable kernel
 *  PCI busmanager ( ACPI PCI Interrupt Link Device Driver )
 *  Copyright (C) 2001, 2002 Andy Grover <andrew.grover@intel.com>
 *  Copyright (C) 2001, 2002 Paul Diefenbaugh <paul.s.diefenbaugh@intel.com>
 *  Copyright (C) 2002       Dominik Brodowski <devel@brodo.de>
 *  Copyright (C) 2003 Arno Klenke
 *  Copyright (C) 1999 - 2001 Kurt Skauen
 *  
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


#include <atheos/kernel.h>
#include <atheos/irq.h>
#include <atheos/types.h>
#include <atheos/list.h>
#include <atheos/msgport.h>
#include <atheos/device.h>
#include <atheos/acpi.h>
#include <posix/errno.h>
#include <macros.h>

#include "pci_internal.h"

#define ACPI_PCI_LINK_CLASS		"pci_irq_routing"
#define ACPI_PCI_LINK_HID		"PNP0C0F"
#define ACPI_PCI_LINK_DRIVER_NAME	"ACPI PCI Interrupt Link Driver"
#define ACPI_PCI_LINK_DEVICE_NAME	"PCI Interrupt Link"
#define ACPI_PCI_LINK_FILE_INFO		"info"
#define ACPI_PCI_LINK_FILE_STATUS	"state"

#define ACPI_PCI_LINK_MAX_POSSIBLE 16


static int acpi_pci_link_add (struct acpi_device *device);
static int acpi_pci_link_remove (struct acpi_device *device, int type);

static struct acpi_driver g_sACPILinkDriver = {
	.name =		ACPI_PCI_LINK_DRIVER_NAME,
	.class =	ACPI_PCI_LINK_CLASS,
	.ids =		ACPI_PCI_LINK_HID,
	.ops =		{
				.add =    acpi_pci_link_add,
				.remove = acpi_pci_link_remove,
			},
};

struct acpi_pci_link_irq {
	uint8			nActive;			/* Current IRQ */
	uint8			nEdgeLevel;		/* All IRQs */
	uint8			nActiveHighLow;	/* All IRQs */
	uint8			bInitialized;
	uint8			nResourceType;
	uint8			nPossibleCount;
	uint8			nPossible[ACPI_PCI_LINK_MAX_POSSIBLE];
};

struct acpi_pci_link {
	struct acpi_device	*psDevice;
	acpi_handle			nHandle;
	struct acpi_pci_link_irq sIrq;
};


static int g_nIRQPenalty[16] = {
	1000000, 1000000, 1000000, 1000, 1000, 0, 1000, 1000,
	0, 0, 0, 0, 1000, 100000, 100000, 100000
};


static ACPI_bus_s* g_psBus = NULL;

#undef DEBUG_LIMIT
#define DEBUG_LIMIT KERN_DEBUG


/* --------------------------------------------------------------------------
                            PCI Link Device Management
   -------------------------------------------------------------------------- */

/*
 * set context (link) possible list from resource list
 */
static acpi_status
acpi_pci_link_check_possible (
	struct acpi_resource	*resource,
	void			*context)
{
	struct acpi_pci_link	*link = (struct acpi_pci_link *) context;
	u32			i = 0;

	ACPI_FUNCTION_TRACE("acpi_pci_link_check_possible");

	switch (resource->type) {
	case ACPI_RESOURCE_TYPE_START_DEPENDENT:
		return_ACPI_STATUS(AE_OK);
	case ACPI_RESOURCE_TYPE_IRQ:
	{
		struct acpi_resource_irq *p = &resource->data.irq;
		if (!p || !p->interrupt_count) {
			ACPI_DEBUG_PRINT((ACPI_DB_WARN, "Blank IRQ resource\n"));
			return_ACPI_STATUS(AE_OK);
		}
		for (i = 0; (i<p->interrupt_count && i<ACPI_PCI_LINK_MAX_POSSIBLE); i++) {
			if (!p->interrupts[i]) {
				ACPI_DEBUG_PRINT((ACPI_DB_WARN, "Invalid IRQ %d\n", p->interrupts[i]));
				continue;
			}
			link->sIrq.nPossible[i] = p->interrupts[i];
			link->sIrq.nPossibleCount++;
		}
		link->sIrq.nEdgeLevel = p->triggering;
		link->sIrq.nActiveHighLow = p->polarity;
		link->sIrq.nResourceType = ACPI_RESOURCE_TYPE_IRQ;
		break;
	}
	case ACPI_RESOURCE_TYPE_EXTENDED_IRQ:
	{
		struct acpi_resource_extended_irq *p = &resource->data.extended_irq;
		if (!p || !p->interrupt_count) {
			ACPI_DEBUG_PRINT((ACPI_DB_WARN, 
				"Blank EXT IRQ resource\n"));
			return_ACPI_STATUS(AE_OK);
		}
		for (i = 0; (i<p->interrupt_count && i<ACPI_PCI_LINK_MAX_POSSIBLE); i++) {
			if (!p->interrupts[i]) {
				ACPI_DEBUG_PRINT((ACPI_DB_WARN, "Invalid IRQ %d\n", p->interrupts[i]));
				continue;
			}
			link->sIrq.nPossible[i] = p->interrupts[i];
			link->sIrq.nPossibleCount++;
		}
		link->sIrq.nEdgeLevel = p->triggering;
		link->sIrq.nActiveHighLow = p->polarity;
		link->sIrq.nResourceType = ACPI_RESOURCE_TYPE_EXTENDED_IRQ;
		break;
	}
	default:
		ACPI_DEBUG_PRINT((ACPI_DB_ERROR, 
			"Resource is not an IRQ entry\n"));
		return_ACPI_STATUS(AE_OK);
	}

	return_ACPI_STATUS(AE_CTRL_TERMINATE);
}


static int
acpi_pci_link_get_possible (
	struct acpi_pci_link	*link)
{
	acpi_status		status;

	ACPI_FUNCTION_TRACE("acpi_pci_link_get_possible");

	if (!link)
		return_VALUE(-EINVAL);

	status = g_psBus->walk_resources(link->nHandle, METHOD_NAME__PRS,
			acpi_pci_link_check_possible, link);
	if (ACPI_FAILURE(status)) {
		ACPI_DEBUG_PRINT((ACPI_DB_ERROR, "Error evaluating _PRS\n"));
		return_VALUE(-ENODEV);
	}

	ACPI_DEBUG_PRINT((ACPI_DB_INFO, 
		"Found %d possible IRQs\n", link->irq.possible_count));

	return_VALUE(0);
}


static acpi_status
acpi_pci_link_check_current (
	struct acpi_resource	*resource,
	void			*context)
{
	int			*irq = (int *) context;

	ACPI_FUNCTION_TRACE("acpi_pci_link_check_current");

	switch (resource->type) {
	case ACPI_RESOURCE_TYPE_IRQ:
	{
		struct acpi_resource_irq *p = &resource->data.irq;
		if (!p || !p->interrupt_count) {
			/*
			 * IRQ descriptors may have no IRQ# bits set,
			 * particularly those those w/ _STA disabled
			 */
			ACPI_DEBUG_PRINT((ACPI_DB_INFO,
				"Blank IRQ resource\n")); 
			return_ACPI_STATUS(AE_OK);
		}
		*irq = p->interrupts[0];
		break;
	}
	case ACPI_RESOURCE_TYPE_EXTENDED_IRQ:
	{
		struct acpi_resource_extended_irq *p = &resource->data.extended_irq;
		if (!p || !p->interrupt_count) {
			/*
			 * extended IRQ descriptors must
			 * return at least 1 IRQ
			 */
			ACPI_DEBUG_PRINT((ACPI_DB_WARN,
				"Blank EXT IRQ resource\n"));
			return_ACPI_STATUS(AE_OK);
		}
		*irq = p->interrupts[0];
		break;
	}
	default:
		ACPI_DEBUG_PRINT((ACPI_DB_ERROR,
			"Resource isn't an IRQ\n"));
		return_ACPI_STATUS(AE_OK);
	}
	return_ACPI_STATUS(AE_CTRL_TERMINATE);
}

/*
 * Run _CRS and set link->irq.active
 *
 * return value:
 * 0 - success
 * !0 - failure
 */
static int
acpi_pci_link_get_current (
	struct acpi_pci_link	*link)
{
	int			result = 0;
	acpi_status		status = AE_OK;
	int			irq = 0;

	ACPI_FUNCTION_TRACE("acpi_pci_link_get_current");

	if (!link || !link->nHandle)
		return_VALUE(-EINVAL);

	link->sIrq.nActive = 0;

	/* 
	 * Query and parse _CRS to get the current IRQ assignment. 
	 */

	status = g_psBus->walk_resources(link->nHandle, METHOD_NAME__CRS,
			acpi_pci_link_check_current, &irq);
	if (ACPI_FAILURE(status)) {
		ACPI_DEBUG_PRINT((ACPI_DB_ERROR, "Error evaluating _CRS\n"));
		result = -ENODEV;
		goto end;
	}

	link->sIrq.nActive = irq;

	ACPI_DEBUG_PRINT((ACPI_DB_INFO, "Link at IRQ %d \n", link->irq.active));

end:
	return_VALUE(result);
}

static int
acpi_pci_link_set (
	struct acpi_pci_link	*link,
	int			irq)
{
	int			result = 0;
	acpi_status		status = AE_OK;
	struct {
		struct acpi_resource	res;
		struct acpi_resource	end;
	}    *resource;
	struct acpi_buffer	buffer = {0, NULL};

	ACPI_FUNCTION_TRACE("acpi_pci_link_set");

	if (!link || !irq)
		return_VALUE(-EINVAL);

	resource = kmalloc( sizeof(*resource)+1, MEMF_KERNEL);
	if(!resource)
		return_VALUE(-ENOMEM);

	memset(resource, 0, sizeof(*resource)+1);
	buffer.length = sizeof(*resource) +1;
	buffer.pointer = resource;

	switch(link->sIrq.nResourceType) {
	case ACPI_RESOURCE_TYPE_IRQ:
		resource->res.type = ACPI_RESOURCE_TYPE_IRQ;
		resource->res.length = sizeof(struct acpi_resource);
		resource->res.data.irq.triggering = link->sIrq.nEdgeLevel;
		resource->res.data.irq.polarity = link->sIrq.nActiveHighLow;
		if (link->sIrq.nEdgeLevel == ACPI_EDGE_SENSITIVE)
			resource->res.data.irq.sharable = ACPI_EXCLUSIVE;
		else
			resource->res.data.irq.sharable = ACPI_SHARED;
		resource->res.data.irq.interrupt_count = 1;
		resource->res.data.irq.interrupts[0] = irq;
		break;
	   
	case ACPI_RESOURCE_TYPE_EXTENDED_IRQ:
		resource->res.type = ACPI_RESOURCE_TYPE_EXTENDED_IRQ;
		resource->res.length = sizeof(struct acpi_resource);
		resource->res.data.extended_irq.producer_consumer = ACPI_CONSUMER;
		resource->res.data.extended_irq.triggering = link->sIrq.nEdgeLevel;
		resource->res.data.extended_irq.polarity = link->sIrq.nActiveHighLow;
		if (link->sIrq.nEdgeLevel == ACPI_EDGE_SENSITIVE)
			resource->res.data.irq.sharable = ACPI_EXCLUSIVE;
		else
			resource->res.data.irq.sharable = ACPI_SHARED;
		resource->res.data.extended_irq.interrupt_count = 1;
		resource->res.data.extended_irq.interrupts[0] = irq;
		/* ignore resource_source, it's optional */
		break;
	default:
		kerndbg( KERN_FATAL, "ACPI BUG: resource_type %d\n", link->sIrq.nResourceType);
		result = -EINVAL;
		goto end;

	}
	resource->end.type = ACPI_RESOURCE_TYPE_END_TAG;

	/* Attempt to set the resource */
	status = g_psBus->set_current_resources(link->nHandle, &buffer);

	/* check for total failure */
	if (ACPI_FAILURE(status)) {
		ACPI_DEBUG_PRINT((ACPI_DB_ERROR, "Error evaluating _SRS\n"));
		result = -ENODEV;
		goto end;
	}

	/* Query _STA, set device->status */
	result = g_psBus->get_status(link->psDevice);
	if (result) {
		ACPI_DEBUG_PRINT((ACPI_DB_ERROR, "Unable to read status\n"));
		goto end;
	}
	if (!link->psDevice->status.enabled) {
		kerndbg( KERN_WARNING, "PCI: %s [%s] disabled and referenced, BIOS bug.\n",
			acpi_device_name(link->psDevice),
			acpi_device_bid(link->psDevice));
	}

	/* Query _CRS, set link->irq.active */
	result = acpi_pci_link_get_current(link);
	if (result) {
		goto end;
	}

	/*
	 * Is current setting not what we set?
	 * set link->irq.active
	 */
	if (link->sIrq.nActive != irq) {
		/*
		 * policy: when _CRS doesn't return what we just _SRS
		 * assume _SRS worked and override _CRS value.
		 */
		kerndbg( KERN_WARNING, "PCI: %s [%s] BIOS reported IRQ %d, using IRQ %d\n",
			acpi_device_name(link->psDevice),
			acpi_device_bid(link->psDevice),
			link->sIrq.nActive, irq);
		link->sIrq.nActive = irq;
	}

	ACPI_DEBUG_PRINT((ACPI_DB_INFO, "Set IRQ %d\n", link->irq.active));
	
end:
	kfree(resource);
	return_VALUE(result);
}


/* --------------------------------------------------------------------------
                            PCI Link IRQ Management
   -------------------------------------------------------------------------- */

static int acpi_pci_link_allocate(struct acpi_pci_link* link) {
	int irq;
	int i;

	ACPI_FUNCTION_TRACE("acpi_pci_link_allocate");

	if (link->sIrq.bInitialized)
		return_VALUE(0);

	/*
	 * search for active IRQ in list of possible IRQs.
	 */
	for (i = 0; i < link->sIrq.nPossibleCount; ++i) {
		if (link->sIrq.nActive == link->sIrq.nPossible[i])
			break;
	}
	/*
	 * forget active IRQ that is not in possible list
	 */
	if (i == link->sIrq.nPossibleCount) {
		link->sIrq.nActive = 0;
	}

	/*
	 * if active found, use it; else pick entry from end of possible list.
	 */
	if (link->sIrq.nActive) {
		irq = link->sIrq.nActive;
	} else {
		irq = link->sIrq.nPossible[link->sIrq.nPossibleCount - 1];
	}

	if (!link->sIrq.nActive) {
		/*
		 * Select the best IRQ.  This is done in reverse to promote
		 * the use of IRQs 9, 10, 11, and >15.
		 */
		 
		for (i = (link->sIrq.nPossibleCount - 1); i >= 0; i--) {
			if( g_nIRQPenalty[link->sIrq.nPossible[i]] < g_nIRQPenalty[irq] )
				irq = link->sIrq.nPossible[i];
		}
	}

	/* Attempt to enable the link device at this IRQ. */
	if (acpi_pci_link_set(link, irq)) {
		kerndbg( KERN_DEBUG, "PCI: Unable to set IRQ for %s [%s] (likely buggy ACPI BIOS).\n"
				"Try disable_pci_irq_routing=true or disable_acpi=true\n",
			acpi_device_name(link->psDevice),
			acpi_device_bid(link->psDevice));
		return_VALUE(-ENODEV);
	} else {
		g_nIRQPenalty[link->sIrq.nActive]++;
		kerndbg( KERN_DEBUG, "PCI: %s [%s] enabled at IRQ %d\n", 
			acpi_device_name(link->psDevice),
			acpi_device_bid(link->psDevice), link->sIrq.nActive);
	}

	link->sIrq.bInitialized = 1;

	return_VALUE(0);
}

/*
 * acpi_pci_link_get_irq
 * success: return IRQ >= 0
 * failure: return -1
 */

int
acpi_pci_link_get_irq (
	acpi_handle		handle,
	int			index,
	int*			edge_level,
	int*			active_high_low)
{
	int                     result = 0;
	struct acpi_device	*device = NULL;
	struct acpi_pci_link	*link = NULL;

	ACPI_FUNCTION_TRACE("acpi_pci_link_get_irq");

	result = g_psBus->get_device(handle, &device);
	if (result) {
		ACPI_DEBUG_PRINT((ACPI_DB_ERROR, "Invalid link device\n"));
		return_VALUE(-1);
	}

	link = (struct acpi_pci_link *) acpi_driver_data(device);
	if (!link) {
		ACPI_DEBUG_PRINT((ACPI_DB_ERROR, "Invalid link context\n"));
		return_VALUE(-1);
	}
	
	if (acpi_pci_link_allocate(link))
		return_VALUE(-1);
	
	if (edge_level) *edge_level = link->sIrq.nEdgeLevel;
	if (active_high_low) *active_high_low = link->sIrq.nActiveHighLow;
	
	return_VALUE(link->sIrq.nActive);
}


/* --------------------------------------------------------------------------
                                 Driver Interface
   -------------------------------------------------------------------------- */

static int
acpi_pci_link_add (
	struct acpi_device *device)
{
	int			result = 0;
	struct acpi_pci_link	*link = NULL;
	int			i = 0;
	int			found = 0;
	
	ACPI_FUNCTION_TRACE("acpi_pci_link_add");

	if (!device)
		return_VALUE(-EINVAL);

	link = kmalloc(sizeof(struct acpi_pci_link), MEMF_KERNEL);
	if (!link)
		return_VALUE(-ENOMEM);
	memset(link, 0, sizeof(struct acpi_pci_link));
	
	link->psDevice = device;
	link->nHandle = device->handle;
	strcpy(acpi_device_name(device), ACPI_PCI_LINK_DEVICE_NAME);
	strcpy(acpi_device_class(device), ACPI_PCI_LINK_CLASS);
	acpi_driver_data(device) = link;

	result = acpi_pci_link_get_possible(link);
	if (result)
		goto end;
		
	/* query and set link->irq.active */
	acpi_pci_link_get_current(link);

	char zBuffer[255];
	char zTemp[255];
	sprintf( zBuffer, "%s [%s] (IRQs", acpi_device_name(device),
		acpi_device_bid(device));
		
	for (i = 0; i < link->sIrq.nPossibleCount; i++) {
		if (link->sIrq.nActive == link->sIrq.nPossible[i]) {
			sprintf( zTemp," *%d", link->sIrq.nPossible[i]);
			strcat( zBuffer, zTemp );
			found = 1;
		}
		else {
			sprintf( zTemp," %d", link->sIrq.nPossible[i]);
			strcat( zBuffer, zTemp );
		}
	}

	strcat( zBuffer, ")" );

	if (!found)
	{
		sprintf( zTemp," *%d", link->sIrq.nActive);
		strcat( zBuffer, zTemp );
	}

	if(!link->psDevice->status.enabled)
		strcat( zBuffer,", disabled.");
		
	kerndbg( KERN_DEBUG, "PCI: %s\n", zBuffer );

	
end:
	if (result)
		kfree(link);

	return_VALUE(result);
}


static int
acpi_pci_link_remove (
	struct acpi_device	*device,
	int			type)
{
	struct acpi_pci_link *link = NULL;

	ACPI_FUNCTION_TRACE("acpi_pci_link_remove");

	if (!device || !acpi_driver_data(device))
		return_VALUE(-EINVAL);

	link = (struct acpi_pci_link *) acpi_driver_data(device);

	kfree(link);

	return_VALUE(0);
}

void init_acpi_pci_links(void)
{
	g_psBus = get_busmanager( ACPI_BUS_NAME, ACPI_BUS_VERSION );
    if( g_psBus == NULL )
    	return;

	g_psBus->register_driver( &g_sACPILinkDriver );
}

























