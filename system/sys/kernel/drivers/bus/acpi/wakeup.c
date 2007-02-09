/*
 * wakeup.c - support wakeup devices
 * Copyright (C) 2004 Li Shaohua <shaohua.li@intel.com>
 */

#include <atheos/kernel.h>
#include <atheos/irq.h>
#include <atheos/types.h>
#include <atheos/list.h>
#include <atheos/msgport.h>
#include <atheos/spinlock.h>
#include <atheos/acpi.h>
#include <posix/errno.h>
#include <macros.h>

#define _COMPONENT		ACPI_SYSTEM_COMPONENT
ACPI_MODULE_NAME		("wakeup_devices")

extern struct list_head	acpi_wakeup_device_list;
extern SpinLock_s acpi_device_lock;


/**
 * acpi_enable_wakeup_device_prep - prepare wakeup devices
 *	@sleep_state:	ACPI state
 * Enable all wakup devices power if the devices' wakeup level
 * is higher than requested sleep level
 */

void
acpi_enable_wakeup_device_prep(
	u8		sleep_state)
{
	struct list_head * node, * next;

	ACPI_FUNCTION_TRACE("acpi_enable_wakeup_device_prep");

	spinlock(&acpi_device_lock);
	list_for_each_safe(node, next, &acpi_wakeup_device_list) {
		struct acpi_device * dev = container_of(node, 
			struct acpi_device, wakeup_list);
		
		if (!dev->wakeup.flags.valid || 
			!dev->wakeup.state.enabled ||
			(sleep_state > (u32) dev->wakeup.sleep_state))
			continue;

		spinunlock(&acpi_device_lock);
		acpi_enable_wakeup_device_power(dev);
		spinlock(&acpi_device_lock);
	}
	spinunlock(&acpi_device_lock);
}

/**
 * acpi_enable_wakeup_device - enable wakeup devices
 *	@sleep_state:	ACPI state
 * Enable all wakup devices's GPE
 */
void
acpi_enable_wakeup_device(
	u8		sleep_state)
{
	struct list_head * node, * next;

	/* 
	 * Caution: this routine must be invoked when interrupt is disabled 
	 * Refer ACPI2.0: P212
	 */
	ACPI_FUNCTION_TRACE("acpi_enable_wakeup_device");
	spinlock(&acpi_device_lock);
	list_for_each_safe(node, next, &acpi_wakeup_device_list) {
		struct acpi_device * dev = container_of(node, 
			struct acpi_device, wakeup_list);

		/* If users want to disable run-wake GPE,
		 * we only disable it for wake and leave it for runtime
		 */
		if (dev->wakeup.flags.run_wake && !dev->wakeup.state.enabled) {
			spinunlock(&acpi_device_lock);
			acpi_set_gpe_type(dev->wakeup.gpe_device, 
				dev->wakeup.gpe_number, ACPI_GPE_TYPE_RUNTIME);
			/* Re-enable it, since set_gpe_type will disable it */
			acpi_enable_gpe(dev->wakeup.gpe_device, 
				dev->wakeup.gpe_number, ACPI_ISR);
			spinlock(&acpi_device_lock);
			continue;
		}

		if (!dev->wakeup.flags.valid ||
			!dev->wakeup.state.enabled ||
			(sleep_state > (u32) dev->wakeup.sleep_state))
			continue;
			
		spinunlock(&acpi_device_lock);
		/* run-wake GPE has been enabled */
		if (!dev->wakeup.flags.run_wake)
			acpi_enable_gpe(dev->wakeup.gpe_device, 
				dev->wakeup.gpe_number, ACPI_ISR);
		dev->wakeup.state.active = 1;
		spinlock(&acpi_device_lock);
	}
	spinunlock(&acpi_device_lock);
}

/**
 * acpi_disable_wakeup_device - disable devices' wakeup capability
 *	@sleep_state:	ACPI state
 * Disable all wakup devices's GPE and wakeup capability
 */
void
acpi_disable_wakeup_device (
	u8		sleep_state)
{
	struct list_head * node, * next;

	ACPI_FUNCTION_TRACE("acpi_disable_wakeup_device");

	spinlock(&acpi_device_lock);
	list_for_each_safe(node, next, &acpi_wakeup_device_list) {
		struct acpi_device * dev = container_of(node, 
			struct acpi_device, wakeup_list);

		if (dev->wakeup.flags.run_wake && !dev->wakeup.state.enabled) {
			spinunlock(&acpi_device_lock);
			acpi_set_gpe_type(dev->wakeup.gpe_device, 
				dev->wakeup.gpe_number, ACPI_GPE_TYPE_WAKE_RUN);
			/* Re-enable it, since set_gpe_type will disable it */
			acpi_enable_gpe(dev->wakeup.gpe_device, 
				dev->wakeup.gpe_number, ACPI_NOT_ISR);
			spinlock(&acpi_device_lock);
			continue;
		}

		if (!dev->wakeup.flags.valid || 
			!dev->wakeup.state.active ||
			(sleep_state > (u32) dev->wakeup.sleep_state))
			continue;

		spinunlock(&acpi_device_lock);
		acpi_disable_wakeup_device_power(dev);
		/* Never disable run-wake GPE */
		if (!dev->wakeup.flags.run_wake) {
			acpi_disable_gpe(dev->wakeup.gpe_device, 
				dev->wakeup.gpe_number, ACPI_NOT_ISR);
			acpi_clear_gpe(dev->wakeup.gpe_device, 
				dev->wakeup.gpe_number, ACPI_NOT_ISR);
		}
		dev->wakeup.state.active = 0;
		spinlock(&acpi_device_lock);
	}
	spinunlock(&acpi_device_lock);
}

int acpi_wakeup_device_init(void)
{
	struct list_head * node, * next;

	char zBuffer[255];
	memset( zBuffer, 0, 255 );

	spinlock(&acpi_device_lock);
	list_for_each_safe(node, next, &acpi_wakeup_device_list) {
		struct acpi_device * dev = container_of(node, 
			struct acpi_device, wakeup_list);
		
		/* In case user doesn't load button driver */
		if (dev->wakeup.flags.run_wake && !dev->wakeup.state.enabled) {
			spinunlock(&acpi_device_lock);
			acpi_set_gpe_type(dev->wakeup.gpe_device, 
				dev->wakeup.gpe_number, ACPI_GPE_TYPE_WAKE_RUN);
			acpi_enable_gpe(dev->wakeup.gpe_device, 
				dev->wakeup.gpe_number, ACPI_NOT_ISR);
			dev->wakeup.state.enabled = 1;
			spinlock(&acpi_device_lock);
		}
		
		char zTemp[20];
		sprintf( zTemp, "%s ", dev->pnp.bus_id);
		strcat( zBuffer, zTemp );
	}
	spinunlock(&acpi_device_lock);
	
	kerndbg( KERN_INFO, "ACPI: Wakeup devices %s\n", zBuffer );

	return 0;
}


/*
 * Disable all wakeup GPEs before power off.
 * 
 * Since acpi_enter_sleep_state() will disable all
 * RUNTIME GPEs, we simply mark all GPES that
 * are not enabled for wakeup from S5 as RUNTIME.
 */
void acpi_gpe_sleep_prepare(u32 sleep_state)
{
	struct list_head *node, *next;

	list_for_each_safe(node, next, &acpi_wakeup_device_list) {
		struct acpi_device *dev = container_of(node,
						       struct acpi_device,
						       wakeup_list);

		/* The GPE can wakeup system from this state, don't touch it */
		if ((u32) dev->wakeup.sleep_state >= sleep_state)
			continue;
		/* acpi_set_gpe_type will automatically disable GPE */
		acpi_set_gpe_type(dev->wakeup.gpe_device,
				  dev->wakeup.gpe_number,
				  ACPI_GPE_TYPE_RUNTIME);
	}
}











