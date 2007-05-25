/*
 *  acpi_battery.c - ACPI Battery Driver ($Revision$)
 *
 *  Copyright (C) 2001, 2002 Andy Grover <andrew.grover@intel.com>
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

#include <atheos/kernel.h>
#include <atheos/irq.h>
#include <atheos/types.h>
#include <atheos/device.h>
#include <atheos/list.h>
#include <atheos/acpi.h>
#include <posix/errno.h>
#include <macros.h>


#define ACPI_BATTERY_VALUE_UNKNOWN 0xFFFFFFFF

#define ACPI_BATTERY_FORMAT_BIF	"NNNNNNNNNSSSS"
#define ACPI_BATTERY_FORMAT_BST	"NNNN"

#define ACPI_BATTERY_COMPONENT		0x00040000
#define ACPI_BATTERY_CLASS		"battery"
#define ACPI_BATTERY_HID		"PNP0C0A"
#define ACPI_BATTERY_DRIVER_NAME	"ACPI Battery Driver"
#define ACPI_BATTERY_DEVICE_NAME	"Battery"
#define ACPI_BATTERY_FILE_INFO		"info"
#define ACPI_BATTERY_FILE_STATUS	"state"
#define ACPI_BATTERY_FILE_ALARM		"alarm"
#define ACPI_BATTERY_NOTIFY_STATUS	0x80
#define ACPI_BATTERY_NOTIFY_INFO	0x81
#define ACPI_BATTERY_UNITS_WATTS	"mW"
#define ACPI_BATTERY_UNITS_AMPS		"mA"


#define _COMPONENT		ACPI_BATTERY_COMPONENT
ACPI_MODULE_NAME		("acpi_battery")

static int acpi_battery_add (struct acpi_device *device);
static int acpi_battery_remove (struct acpi_device *device, int type);

static struct acpi_driver acpi_battery_driver = {
	.name =		ACPI_BATTERY_DRIVER_NAME,
	.class =	ACPI_BATTERY_CLASS,
	.ids =		ACPI_BATTERY_HID,
	.ops =		{
				.add =		acpi_battery_add,
				.remove =	acpi_battery_remove,
			},
};

struct acpi_battery_status {
	acpi_integer		state;
	acpi_integer		present_rate;
	acpi_integer		remaining_capacity;
	acpi_integer		present_voltage;
};

struct acpi_battery_info {
	acpi_integer		power_unit;
	acpi_integer		design_capacity;
	acpi_integer		last_full_capacity;
	acpi_integer		battery_technology;
	acpi_integer		design_voltage;
	acpi_integer		design_capacity_warning;
	acpi_integer		design_capacity_low;
	acpi_integer		battery_capacity_granularity_1;
	acpi_integer		battery_capacity_granularity_2;
	acpi_string		model_number;
	acpi_string		serial_number;
	acpi_string		battery_type;
	acpi_string		oem_info;
};

struct acpi_battery_flags {
	u8			present:1;	/* Bay occupied? */
	u8			power_unit:1;	/* 0=watts, 1=apms */
	u8			alarm:1;	/* _BTP present? */
	u8			reserved:5;
};

struct acpi_battery_trips {
	unsigned long		warning;
	unsigned long		low;
};

struct acpi_battery {
	acpi_handle		handle;
	struct acpi_battery_flags flags;
	struct acpi_battery_trips trips;
	unsigned long		alarm;
	//struct acpi_battery_info *info;
	int	node;
};

static ACPI_bus_s* g_psBus = NULL;
static int g_nDeviceID = -1;

static DeviceOperations_s g_sOperations;

/* --------------------------------------------------------------------------
                               Battery Management
   -------------------------------------------------------------------------- */

static int
acpi_battery_get_info (
	struct acpi_battery	*battery,
	struct acpi_battery_info **bif)
{
	int			result = 0;
	acpi_status 		status = 0;
	struct acpi_buffer	buffer = {ACPI_ALLOCATE_BUFFER, NULL};
	struct acpi_buffer	format = {sizeof(ACPI_BATTERY_FORMAT_BIF),
						ACPI_BATTERY_FORMAT_BIF};
	struct acpi_buffer	data = {0, NULL};
	union acpi_object	*package = NULL;

	ACPI_FUNCTION_TRACE("acpi_battery_get_info");

	if (!battery || !bif)
		return_VALUE(-EINVAL);

	/* Evalute _BIF */

	status = g_psBus->evaluate_object(battery->handle, "_BIF", NULL, &buffer);
	if (ACPI_FAILURE(status)) {
		ACPI_DEBUG_PRINT((ACPI_DB_ERROR, "Error evaluating _BIF\n"));
		return_VALUE(-ENODEV);
	}

	package = (union acpi_object *) buffer.pointer;

	/* Extract Package Data */

	status = g_psBus->extract_package(package, &format, &data);
	if (status != AE_BUFFER_OVERFLOW) {
		ACPI_DEBUG_PRINT((ACPI_DB_ERROR, "Error extracting _BIF\n"));
		result = -ENODEV;
		goto end;
	}

	data.pointer = kmalloc(data.length, MEMF_KERNEL);
	if (!data.pointer) {
		result = -ENOMEM;
		goto end;
	}
	memset(data.pointer, 0, data.length);

	status = g_psBus->extract_package(package, &format, &data);
	if (ACPI_FAILURE(status)) {
		ACPI_DEBUG_PRINT((ACPI_DB_ERROR, "Error extracting _BIF\n"));
		kfree(data.pointer);
		result = -ENODEV;
		goto end;
	}

end:
	kfree(buffer.pointer);

	if (!result)
		(*bif) = (struct acpi_battery_info *) data.pointer;

	return_VALUE(result);
}

static int
acpi_battery_get_status (
	struct acpi_battery	*battery,
	struct acpi_battery_status **bst)
{
	int			result = 0;
	acpi_status 		status = 0;
	struct acpi_buffer	buffer = {ACPI_ALLOCATE_BUFFER, NULL};
	struct acpi_buffer	format = {sizeof(ACPI_BATTERY_FORMAT_BST),
						ACPI_BATTERY_FORMAT_BST};
	struct acpi_buffer	data = {0, NULL};
	union acpi_object	*package = NULL;

	ACPI_FUNCTION_TRACE("acpi_battery_get_status");

	if (!battery || !bst)
		return_VALUE(-EINVAL);

	/* Evalute _BST */

	status = g_psBus->evaluate_object(battery->handle, "_BST", NULL, &buffer);
	if (ACPI_FAILURE(status)) {
		ACPI_DEBUG_PRINT((ACPI_DB_ERROR, "Error evaluating _BST\n"));
		return_VALUE(-ENODEV);
	}

	package = (union acpi_object *) buffer.pointer;

	/* Extract Package Data */

	status = g_psBus->extract_package(package, &format, &data);
	if (status != AE_BUFFER_OVERFLOW) {
		ACPI_DEBUG_PRINT((ACPI_DB_ERROR, "Error extracting _BST\n"));
		result = -ENODEV;
		goto end;
	}

	data.pointer = kmalloc(data.length, MEMF_KERNEL);
	if (!data.pointer) {
		result = -ENOMEM;
		goto end;
	}
	memset(data.pointer, 0, data.length);

	status = g_psBus->extract_package(package, &format, &data);
	if (ACPI_FAILURE(status)) {
		ACPI_DEBUG_PRINT((ACPI_DB_ERROR, "Error extracting _BST\n"));
		kfree(data.pointer);
		result = -ENODEV;
		goto end;
	}

end:
	kfree(buffer.pointer);

	if (!result)
		(*bst) = (struct acpi_battery_status *) data.pointer;

	return_VALUE(result);
}


static int
acpi_battery_set_alarm (
	struct acpi_battery	*battery,
	unsigned long		alarm)
{
	acpi_status		status = 0;
	union acpi_object	arg0 = {ACPI_TYPE_INTEGER};
	struct acpi_object_list	arg_list = {1, &arg0};

	ACPI_FUNCTION_TRACE("acpi_battery_set_alarm");

	if (!battery)
		return_VALUE(-EINVAL);

	if (!battery->flags.alarm)
		return_VALUE(-ENODEV);

	arg0.integer.value = alarm;

	status = g_psBus->evaluate_object(battery->handle, "_BTP", &arg_list, NULL);
	if (ACPI_FAILURE(status))
		return_VALUE(-ENODEV);

	ACPI_DEBUG_PRINT((ACPI_DB_INFO, "Alarm set to %d\n", (u32) alarm));

	battery->alarm = alarm;

	return_VALUE(0);
}


static int
acpi_battery_check (
	struct acpi_battery	*battery)
{
	int			result = 0;
	acpi_status		status = AE_OK;
	acpi_handle		handle = NULL;
	struct acpi_device	*device = NULL;
	struct acpi_battery_info *bif = NULL;

	ACPI_FUNCTION_TRACE("acpi_battery_check");
	
	if (!battery)
		return_VALUE(-EINVAL);

	result = g_psBus->get_device(battery->handle, &device);
	if (result)
		return_VALUE(result);

	result = g_psBus->get_status(device);
	if (result)
		return_VALUE(result);

	/* Insertion? */

	if (!battery->flags.present && device->status.battery_present) {

		ACPI_DEBUG_PRINT((ACPI_DB_INFO, "Battery inserted\n"));

		/* Evalute _BIF to get certain static information */

		result = acpi_battery_get_info(battery, &bif);
		if (result)
			return_VALUE(result);

		battery->flags.power_unit = bif->power_unit;
		battery->trips.warning = bif->design_capacity_warning;
		battery->trips.low = bif->design_capacity_low;
		kfree(bif);

		/* See if alarms are supported, and if so, set default */

		status = g_psBus->get_handle(battery->handle, "_BTP", &handle);
		if (ACPI_SUCCESS(status)) {
			battery->flags.alarm = 1;
			acpi_battery_set_alarm(battery, battery->trips.warning);
		}
	}

	/* Removal? */

	else if (battery->flags.present && !device->status.battery_present) {
		ACPI_DEBUG_PRINT((ACPI_DB_INFO, "Battery removed\n"));
	}

	battery->flags.present = device->status.battery_present;

	return_VALUE(result);
}



/* --------------------------------------------------------------------------
                                 Driver Interface
   -------------------------------------------------------------------------- */

static void
acpi_battery_notify (
	acpi_handle		handle,
	u32			event,
	void			*data)
{
	struct acpi_battery	*battery = (struct acpi_battery *) data;
	struct acpi_device	*device = NULL;

	ACPI_FUNCTION_TRACE("acpi_battery_notify");

	if (!battery)
		return_VOID;

	if (g_psBus->get_device(handle, &device))
		return_VOID;

	switch (event) {
	case ACPI_BATTERY_NOTIFY_STATUS:
	case ACPI_BATTERY_NOTIFY_INFO:
		acpi_battery_check(battery);
		break;
	default:
		ACPI_DEBUG_PRINT((ACPI_DB_INFO,
			"Unsupported event [0x%x]\n", event));
		break;
	}

	return_VOID;
}


static int
acpi_battery_add (
	struct acpi_device	*device)
{
	int			result = 0;
	acpi_status		status = 0;
	struct acpi_battery	*battery = NULL;

	ACPI_FUNCTION_TRACE("acpi_battery_add");
	
	if (!device)
		return_VALUE(-EINVAL);

	battery = kmalloc(sizeof(struct acpi_battery), MEMF_KERNEL);
	if (!battery)
		return_VALUE(-ENOMEM);
	memset(battery, 0, sizeof(struct acpi_battery));

	battery->handle = device->handle;
	strcpy(acpi_device_name(device), ACPI_BATTERY_DEVICE_NAME);
	strcpy(acpi_device_class(device), ACPI_BATTERY_CLASS);
	acpi_driver_data(device) = battery;

	result = acpi_battery_check(battery);
	if (result)
		goto end;

	status = g_psBus->install_notify_handler(battery->handle,
		ACPI_DEVICE_NOTIFY, acpi_battery_notify, battery);
	if (ACPI_FAILURE(status)) {
		ACPI_DEBUG_PRINT((ACPI_DB_ERROR,
			"Error installing notify handler\n"));
		result = -ENODEV;
		goto end;
	}

	printk("ACPI: %s Slot [%s] (battery %s)\n",
		ACPI_BATTERY_DEVICE_NAME, acpi_device_bid(device),
		device->status.battery_present?"present":"absent");
		
	battery->node = create_device_node( g_nDeviceID, -1, "misc/battery", &g_sOperations, battery );
    if ( battery->node < 0 ) {
		return( battery->node );
    }
	
end:
	if (result) {
		kfree(battery);
	}

	return_VALUE(result);
}


static int
acpi_battery_remove (
	struct acpi_device	*device,
	int			type)
{
	acpi_status		status = 0;
	struct acpi_battery	*battery = NULL;

	ACPI_FUNCTION_TRACE("acpi_battery_remove");

	if (!device || !acpi_driver_data(device))
		return_VALUE(-EINVAL);

	battery = (struct acpi_battery *) acpi_driver_data(device);
	
	delete_device_node( battery->node );
	
	status = g_psBus->remove_notify_handler(battery->handle,
		ACPI_DEVICE_NOTIFY, acpi_battery_notify);
	if (ACPI_FAILURE(status))
		ACPI_DEBUG_PRINT((ACPI_DB_ERROR,
			"Error removing notify handler\n"));

	kfree(battery);

	return_VALUE(0);
}


status_t acpi_get_battery_status( struct acpi_battery* psBattery, BatteryStatus_s* psBatStatus )
{
	if( !psBattery )
		return( -ENODEV );
	
	struct acpi_battery_info* psInfo;
	struct acpi_battery_status* psStatus;
	if( acpi_battery_get_info( psBattery, &psInfo ) )
		return( -EIO );
	if( acpi_battery_get_status( psBattery, &psStatus ) )
		return( -EIO );
	
	psBatStatus->bs_nPercentage = (int)psStatus->remaining_capacity * 100 / (int)psInfo->last_full_capacity;
	psBatStatus->bs_bChargingOrFull = !( psStatus->state & 0x01 );
		
	kfree( psInfo );
	kfree( psStatus );
	
	return( 0 );
}

status_t battery_ioctl( void* pNode, void* pCookie, uint32 nCommand, void* pArgs, bool bFromKernel )
{
	struct acpi_battery* psBattery = pNode;
	BatteryStatus_s sStatus;
	
	BatteryStatus_s* psTarget = ( BatteryStatus_s* )pArgs;
	switch( nCommand )
	{
		case 0:
			if( psTarget == NULL )
				break;
			int nError = acpi_get_battery_status( psBattery, &sStatus );
			if( nError != 0 )
				return( nError );
			if( bFromKernel )
				memcpy( pArgs, &sStatus, psTarget->bs_nDataSize );
			else
				memcpy_to_user( pArgs, &sStatus, psTarget->bs_nDataSize );
			return( 0 );
		break;
		default:
			return( -EINVAL );
	}
    return( -EINVAL );
}


static DeviceOperations_s g_sOperations = {
	NULL,
	NULL,
    battery_ioctl,
	NULL,
	NULL
};

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

status_t device_init( int nDeviceID )
{
    int nError;
    
    g_psBus = get_busmanager( ACPI_BUS_NAME, ACPI_BUS_VERSION );
    if( g_psBus == NULL )
    	return( -1 );
    
    g_nDeviceID = nDeviceID;
    
	nError = g_psBus->register_driver(&acpi_battery_driver);
	if( nError <= 0 ) {
		disable_device_on_bus( nDeviceID, ACPI_BUS_NAME );
		return( -1 );
	}
    
    return( 0 );
}

/*****************************************************************************
 * NAME:
 * DESC:
 * NOTE:
 * SEE ALSO:
 ****************************************************************************/

status_t device_uninit( int nDeviceID )
{
	g_psBus->unregister_driver(&acpi_battery_driver);
    return( 0 );
}

















