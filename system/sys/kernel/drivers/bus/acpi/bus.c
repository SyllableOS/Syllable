/*
 *  acpi_bus.c - ACPI Bus Driver ($Revision$)
 *
 *  Copyright (C) 2001, 2002 Paul Diefenbaugh <paul.s.diefenbaugh@intel.com>
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or (at
 *  your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */

#include <atheos/list.h>
#include <atheos/kernel.h>
#include <atheos/spinlock.h>
#include <atheos/types.h>
#include <atheos/semaphore.h>
#include <atheos/acpi.h>
#include <posix/errno.h>


#define _COMPONENT		ACPI_BUS_COMPONENT
ACPI_MODULE_NAME		("acpi_bus")

extern void acpi_pic_sci_set_trigger(unsigned int irq, u16 trigger);

FADT_DESCRIPTOR			acpi_fadt;
EXPORT_SYMBOL(acpi_fadt);

struct acpi_device		*acpi_root;

#define STRUCT_TO_INT(s)	(*((int*)&s))

/* --------------------------------------------------------------------------
                                Device Management
   -------------------------------------------------------------------------- */

int
acpi_bus_get_device (
	acpi_handle		handle,
	struct acpi_device	**device)
{
	acpi_status		status = AE_OK;

	ACPI_FUNCTION_TRACE("acpi_bus_get_device");

	if (!device)
		return_VALUE(-EINVAL);

	/* TBD: Support fixed-feature devices */

	status = acpi_get_data(handle, acpi_bus_data_handler, (void**) device);
	if (ACPI_FAILURE(status) || !*device) {
		ACPI_DEBUG_PRINT((ACPI_DB_WARN, "No context for object [%p]\n",
			handle));
		return_VALUE(-ENODEV);
	}

	return_VALUE(0);
}


int
acpi_bus_get_status (
	struct acpi_device	*device)
{
	acpi_status		status = AE_OK;
	unsigned long		sta = 0;
	
	ACPI_FUNCTION_TRACE("acpi_bus_get_status");

	if (!device)
		return_VALUE(-EINVAL);

	/*
	 * Evaluate _STA if present.
	 */
	if (device->flags.dynamic_status) {
		status = acpi_evaluate_integer(device->handle, "_STA", NULL, &sta);
		if (ACPI_FAILURE(status))
			return_VALUE(-ENODEV);
		STRUCT_TO_INT(device->status) = (int) sta;
	}

	/*
	 * Otherwise we assume the status of our parent (unless we don't
	 * have one, in which case status is implied).
	 */
	else if (device->parent)
		device->status = device->parent->status;
	else
		STRUCT_TO_INT(device->status) = 0x0F;

	if (device->status.functional && !device->status.present) {
		printk("ACPI: Device [%s] status [%08x]: "
			"functional but not present; setting present\n",
			device->pnp.bus_id,
			(u32) STRUCT_TO_INT(device->status));
		device->status.present = 1;
	}

	ACPI_DEBUG_PRINT((ACPI_DB_INFO, "Device [%s] status [%08x]\n", 
		device->pnp.bus_id, (u32) STRUCT_TO_INT(device->status)));

	return_VALUE(0);
}



/* --------------------------------------------------------------------------
                                 Power Management
   -------------------------------------------------------------------------- */
int
acpi_bus_get_power (
	acpi_handle		handle,
	int			*state)
{
	int			result = 0;
	acpi_status             status = 0;
	struct acpi_device	*device = NULL;
	unsigned long		psc = 0;

	ACPI_FUNCTION_TRACE("acpi_bus_get_power");

	result = acpi_bus_get_device(handle, &device);
	if (result)
		return_VALUE(result);

	*state = ACPI_STATE_UNKNOWN;

	if (!device->flags.power_manageable) {
		/* TBD: Non-recursive algorithm for walking up hierarchy */
		if (device->parent)
			*state = device->parent->power.state;
		else
			*state = ACPI_STATE_D0;
	}
	else {
		/*
		 * Get the device's power state either directly (via _PSC) or 
		 * indirectly (via power resources).
		 */
		if (device->power.flags.explicit_get) {
			status = acpi_evaluate_integer(device->handle, "_PSC", 
				NULL, &psc);
			if (ACPI_FAILURE(status))
				return_VALUE(-ENODEV);
			device->power.state = (int) psc;
		}
		else if (device->power.flags.power_resources) {
			result = acpi_power_get_inferred_state(device);
			if (result)
				return_VALUE(result);
		}

		*state = device->power.state;
	}

	ACPI_DEBUG_PRINT((ACPI_DB_INFO, "Device [%s] power state is D%d\n",
		device->pnp.bus_id, device->power.state));

	return_VALUE(0);
}
EXPORT_SYMBOL(acpi_bus_get_power);


int
acpi_bus_set_power (
	acpi_handle		handle,
	int			state)
{
	int			result = 0;
	acpi_status		status = AE_OK;
	struct acpi_device	*device = NULL;
	char			object_name[5] = {'_','P','S','0'+state,'\0'};

	ACPI_FUNCTION_TRACE("acpi_bus_set_power");

	result = acpi_bus_get_device(handle, &device);
	if (result)
		return_VALUE(result);

	if ((state < ACPI_STATE_D0) || (state > ACPI_STATE_D3))
		return_VALUE(-EINVAL);

	/* Make sure this is a valid target state */

	if (!device->flags.power_manageable) {
		ACPI_DEBUG_PRINT((ACPI_DB_WARN, "Device is not power manageable\n"));
		return_VALUE(-ENODEV);
	}
	if (state == device->power.state) {
		ACPI_DEBUG_PRINT((ACPI_DB_INFO, "Device is already at D%d\n", state));
		return_VALUE(0);
	}
	if (!device->power.states[state].flags.valid) {
		ACPI_DEBUG_PRINT((ACPI_DB_WARN, "Device does not support D%d\n", state));
		return_VALUE(-ENODEV);
	}
	if (device->parent && (state < device->parent->power.state)) {
		ACPI_DEBUG_PRINT((ACPI_DB_WARN, "Cannot set device to a higher-powered state than parent\n"));
		return_VALUE(-ENODEV);
	}

	/*
	 * Transition Power
	 * ----------------
	 * On transitions to a high-powered state we first apply power (via
	 * power resources) then evalute _PSx.  Conversly for transitions to
	 * a lower-powered state.
	 */ 
	if (state < device->power.state) {
		if (device->power.flags.power_resources) {
			result = acpi_power_transition(device, state);
			if (result)
				goto end;
		}
		if (device->power.states[state].flags.explicit_set) {
			status = acpi_evaluate_object(device->handle, 
				object_name, NULL, NULL);
			if (ACPI_FAILURE(status)) {
				result = -ENODEV;
				goto end;
			}
		}
	}
	else {
		if (device->power.states[state].flags.explicit_set) {
			status = acpi_evaluate_object(device->handle, 
				object_name, NULL, NULL);
			if (ACPI_FAILURE(status)) {
				result = -ENODEV;
				goto end;
			}
		}
		if (device->power.flags.power_resources) {
			result = acpi_power_transition(device, state);
			if (result)
				goto end;
		}
	}

end:
	if (result)
		ACPI_DEBUG_PRINT((ACPI_DB_WARN, "Error transitioning device [%s] to D%d\n",
			device->pnp.bus_id, state));
	else
		ACPI_DEBUG_PRINT((ACPI_DB_INFO, "Device [%s] transitioned to D%d\n",
			device->pnp.bus_id, state));

	return_VALUE(result);
}



/* --------------------------------------------------------------------------
                             Notification Handling
   -------------------------------------------------------------------------- */

static int
acpi_bus_check_device (
	struct acpi_device	*device,
	int			*status_changed)
{
	acpi_status             status = 0;
	struct acpi_device_status old_status;

	ACPI_FUNCTION_TRACE("acpi_bus_check_device");

	if (!device)
		return_VALUE(-EINVAL);

	if (status_changed)
		*status_changed = 0;

	old_status = device->status;

	/*
	 * Make sure this device's parent is present before we go about
	 * messing with the device.
	 */
	if (device->parent && !device->parent->status.present) {
		device->status = device->parent->status;
		if (STRUCT_TO_INT(old_status) != STRUCT_TO_INT(device->status)) {
			if (status_changed)
				*status_changed = 1;
		}
		return_VALUE(0);
	}

	status = acpi_bus_get_status(device);
	if (ACPI_FAILURE(status))
		return_VALUE(-ENODEV);

	if (STRUCT_TO_INT(old_status) == STRUCT_TO_INT(device->status))
		return_VALUE(0);

	if (status_changed)
		*status_changed = 1;
	
	/*
	 * Device Insertion/Removal
	 */
	if ((device->status.present) && !(old_status.present)) {
		ACPI_DEBUG_PRINT((ACPI_DB_INFO, "Device insertion detected\n"));
		/* TBD: Handle device insertion */
	}
	else if (!(device->status.present) && (old_status.present)) {
		ACPI_DEBUG_PRINT((ACPI_DB_INFO, "Device removal detected\n"));
		/* TBD: Handle device removal */
	}

	return_VALUE(0);
}


static int
acpi_bus_check_scope (
	struct acpi_device	*device)
{
	int			result = 0;
	int			status_changed = 0;

	ACPI_FUNCTION_TRACE("acpi_bus_check_scope");

	if (!device)
		return_VALUE(-EINVAL);

	/* Status Change? */
	result = acpi_bus_check_device(device, &status_changed);
	if (result)
		return_VALUE(result);

	if (!status_changed)
		return_VALUE(0);

	/*
	 * TBD: Enumerate child devices within this device's scope and
	 *       run acpi_bus_check_device()'s on them.
	 */

	return_VALUE(0);
}


/**
 * acpi_bus_notify
 * ---------------
 * Callback for all 'system-level' device notifications (values 0x00-0x7F).
 */
static void 
acpi_bus_notify (
	acpi_handle             handle,
	u32                     type,
	void                    *data)
{
	int			result = 0;
	struct acpi_device	*device = NULL;

	ACPI_FUNCTION_TRACE("acpi_bus_notify");

	if (acpi_bus_get_device(handle, &device))
		return_VOID;

	switch (type) {

	case ACPI_NOTIFY_BUS_CHECK:
		ACPI_DEBUG_PRINT((ACPI_DB_INFO, "Received BUS CHECK notification for device [%s]\n", 
			device->pnp.bus_id));
		result = acpi_bus_check_scope(device);
		/* 
		 * TBD: We'll need to outsource certain events to non-ACPI
		 *	drivers via the device manager (device.c).
		 */
		break;

	case ACPI_NOTIFY_DEVICE_CHECK:
		ACPI_DEBUG_PRINT((ACPI_DB_INFO, "Received DEVICE CHECK notification for device [%s]\n", 
			device->pnp.bus_id));
		result = acpi_bus_check_device(device, NULL);
		/* 
		 * TBD: We'll need to outsource certain events to non-ACPI
		 *	drivers via the device manager (device.c).
		 */
		break;

	case ACPI_NOTIFY_DEVICE_WAKE:
		ACPI_DEBUG_PRINT((ACPI_DB_INFO, "Received DEVICE WAKE notification for device [%s]\n", 
			device->pnp.bus_id));
		/* TBD */
		break;

	case ACPI_NOTIFY_EJECT_REQUEST:
		ACPI_DEBUG_PRINT((ACPI_DB_INFO, "Received EJECT REQUEST notification for device [%s]\n", 
			device->pnp.bus_id));
		/* TBD */
		break;

	case ACPI_NOTIFY_DEVICE_CHECK_LIGHT:
		ACPI_DEBUG_PRINT((ACPI_DB_INFO, "Received DEVICE CHECK LIGHT notification for device [%s]\n", 
			device->pnp.bus_id));
		/* TBD: Exactly what does 'light' mean? */
		break;

	case ACPI_NOTIFY_FREQUENCY_MISMATCH:
		ACPI_DEBUG_PRINT((ACPI_DB_INFO, "Received FREQUENCY MISMATCH notification for device [%s]\n", 
			device->pnp.bus_id));
		/* TBD */
		break;

	case ACPI_NOTIFY_BUS_MODE_MISMATCH:
		ACPI_DEBUG_PRINT((ACPI_DB_INFO, "Received BUS MODE MISMATCH notification for device [%s]\n", 
			device->pnp.bus_id));
		/* TBD */
		break;

	case ACPI_NOTIFY_POWER_FAULT:
		ACPI_DEBUG_PRINT((ACPI_DB_INFO, "Received POWER FAULT notification for device [%s]\n", 
			device->pnp.bus_id));
		/* TBD */
		break;

	default:
		ACPI_DEBUG_PRINT((ACPI_DB_INFO, "Received unknown/unsupported notification [%08x]\n", 
			type));
		break;
	}

	return_VOID;
}

/* --------------------------------------------------------------------------
                             Initialization/Cleanup
   -------------------------------------------------------------------------- */

static int
acpi_bus_init_irq (void)
{
	acpi_status		status = AE_OK;
	union acpi_object	arg = {ACPI_TYPE_INTEGER};
	struct acpi_object_list	arg_list = {1, &arg};
	char			*message = NULL;

	ACPI_FUNCTION_TRACE("acpi_bus_init_irq");

	/* 
	 * Let the system know what interrupt model we are using by
	 * evaluating the \_PIC object, if exists.
	 */

	switch (acpi_irq_model) {
	case ACPI_IRQ_MODEL_PIC:
		message = "PIC";
		break;
	case ACPI_IRQ_MODEL_IOAPIC:
		message = "IOAPIC";
		break;
	case ACPI_IRQ_MODEL_IOSAPIC:
		message = "IOSAPIC";
		break;
	default:
		printk("ACPI: Unknown interrupt routing model\n");
		return_VALUE(-ENODEV);
	}

	kerndbg( KERN_INFO, "ACPI: Using %s for interrupt routing\n", message);

	arg.integer.value = acpi_irq_model;

	status = acpi_evaluate_object(NULL, "\\_PIC", &arg_list, NULL);
	if (ACPI_FAILURE(status) && (status != AE_NOT_FOUND)) {
		ACPI_DEBUG_PRINT((ACPI_DB_ERROR, "Error evaluating _PIC\n"));
		return_VALUE(-ENODEV);
	}

	return_VALUE(0);
}


static int
acpi_bus_init (void)
{
	int			result = 0;
	acpi_status		status = AE_OK;
	struct acpi_buffer	buffer = {sizeof(acpi_fadt), &acpi_fadt};
	extern acpi_status	acpi_os_initialize1(void);
	
	/* enable workarounds, unless strict ACPI spec. compliance */
	if (!acpi_strict)
		acpi_gbl_enable_interpreter_slack = TRUE;

	status = acpi_initialize_subsystem();
	if (ACPI_FAILURE(status)) {
		printk("ACPI: Unable to initialize the ACPI Interpreter\n");
		goto error0;
	}

	status = acpi_load_tables();
	if (ACPI_FAILURE(status)) {
		printk("ACPI: Unable to load the System Description Tables\n");
		goto error0;
	}

	/*
	 * Get a separate copy of the FADT for use by other drivers.
	 */
	status = acpi_get_table(ACPI_TABLE_FADT, 1, &buffer);
	if (ACPI_FAILURE(status)) {
		printk("ACPI: Unable to get the FADT\n");
		goto error0;
	}

	if (1/*!acpi_ioapic*/) {
		extern acpi_interrupt_flags acpi_sci_flags;

		/* compatible (0) means level (3) */
		if (acpi_sci_flags.trigger == 0)
			acpi_sci_flags.trigger = 3;

		/* Set PIC-mode SCI trigger type */
		acpi_pic_sci_set_trigger(acpi_fadt.sci_int, acpi_sci_flags.trigger);
	}
	
#if 0
	 else {
		extern int acpi_sci_override_gsi;
		/*
		 * now that acpi_fadt is initialized,
		 * update it with result from INT_SRC_OVR parsing
		 */
		acpi_fadt.sci_int = acpi_sci_override_gsi;
	}
#endif

	status = acpi_os_initialize1();

	status = acpi_enable_subsystem(ACPI_FULL_INITIALIZATION);
	if (ACPI_FAILURE(status)) {
		printk("ACPI: Unable to enable ACPI\n");
		goto error0;
	}

	status = acpi_ec_ecdt_probe();

	status = acpi_initialize_objects(ACPI_FULL_INITIALIZATION);
	if (ACPI_FAILURE(status)) {
		printk("ACPI: Unable to initialize ACPI objects\n");
		goto error1;
	}

	/*
	 * Get the system interrupt model and evaluate \_PIC.
	 */
	result = acpi_bus_init_irq();
	if (result)
		goto error1;

	/*
	 * Register the for all standard device notifications.
	 */
	status = acpi_install_notify_handler(ACPI_ROOT_OBJECT, ACPI_SYSTEM_NOTIFY, &acpi_bus_notify, NULL);
	if (ACPI_FAILURE(status)) {
		printk("ACPI: Unable to register for device notifications\n");
		goto error1;
	}

	return_VALUE(0);

	/* Mimic structured exception handling */
error1:
	acpi_terminate();
error0:
	return_VALUE(-ENODEV);
}

int acpi_init (void)
{
	int			result = 0;

	ACPI_FUNCTION_TRACE("acpi_init");
	
	kerndbg( KERN_DEBUG, "ACPI: Subsystem revision %08x\n",
		ACPI_CA_VERSION);

	result = acpi_bus_init();
	
	return_VALUE(result);
}
























