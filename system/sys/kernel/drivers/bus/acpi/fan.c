/*
 *  acpi_fan.c - ACPI Fan Driver ($Revision$)
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
#include <atheos/list.h>
#include <atheos/msgport.h>
#include <atheos/acpi.h>
#include <atheos/resource.h>
#include <posix/errno.h>
#include <macros.h>
#include <acpi/processor.h>

#define ACPI_FAN_COMPONENT		0x00200000
#define ACPI_FAN_CLASS			"fan"
#define ACPI_FAN_DRIVER_NAME		"ACPI Fan Driver"
#define ACPI_FAN_FILE_STATE		"state"

#define _COMPONENT		ACPI_FAN_COMPONENT
ACPI_MODULE_NAME("acpi_fan")


static int acpi_fan_add(struct acpi_device *device);
static int acpi_fan_remove(struct acpi_device *device, int type);
static int acpi_fan_suspend(struct acpi_device *device, int state);
static int acpi_fan_resume(struct acpi_device *device, int state);

static struct acpi_driver acpi_fan_driver = {
	.name = ACPI_FAN_DRIVER_NAME,
	.class = ACPI_FAN_CLASS,
	.ids = "PNP0C0B",
	.ops = {
		.add = acpi_fan_add,
		.remove = acpi_fan_remove,
		.suspend = acpi_fan_suspend,
		.resume = acpi_fan_resume,
		},
};

struct acpi_fan {
	struct acpi_device * device;
};


/* --------------------------------------------------------------------------
                                 Driver Interface
   -------------------------------------------------------------------------- */

static int acpi_fan_add(struct acpi_device *device)
{
	int result = 0;
	struct acpi_fan *fan = NULL;
	int state = 0;


	if (!device)
		return -EINVAL;

	fan = kmalloc(sizeof(struct acpi_fan), MEMF_KERNEL);
	if (!fan)
		return -ENOMEM;
	memset(fan, 0, sizeof(struct acpi_fan));

	fan->device = device;
	strcpy(acpi_device_name(device), "Fan");
	strcpy(acpi_device_class(device), ACPI_FAN_CLASS);
	acpi_driver_data(device) = fan;

	result = acpi_bus_get_power(device->handle, &state);
	if (result) {
		printk("ACPI: Reading power state\n");
		goto end;
	}

	device->flags.force_power_state = 1;
	acpi_bus_set_power(device->handle, state);
	device->flags.force_power_state = 0;

	printk("ACPI: %s [%s] (%s)\n",
	       acpi_device_name(device), acpi_device_bid(device),
	       !device->power.state ? "on" : "off");

      end:
	if (result)
		kfree(fan);

	return result;
}

static int acpi_fan_remove(struct acpi_device *device, int type)
{
	struct acpi_fan *fan = NULL;


	if (!device || !acpi_driver_data(device))
		return -EINVAL;

	fan = (struct acpi_fan *)acpi_driver_data(device);

	
	kfree(fan);

	return 0;
}

static int acpi_fan_suspend(struct acpi_device *device, int state)
{
	if (!device)
		return -EINVAL;

	acpi_bus_set_power(device->handle, ACPI_STATE_D0);

	return AE_OK;
}

static int acpi_fan_resume(struct acpi_device *device, int state)
{
	int result = 0;
	int power_state = 0;

	if (!device)
		return -EINVAL;

	result = acpi_bus_get_power(device->handle, &power_state);
	if (result) {
		ACPI_DEBUG_PRINT((ACPI_DB_ERROR,
				  "Error reading fan power state\n"));
		return result;
	}

	device->flags.force_power_state = 1;
	acpi_bus_set_power(device->handle, power_state);
	device->flags.force_power_state = 0;

	return result;
}

int acpi_fan_init(void)
{
	int result = 0;


	result = acpi_bus_register_driver(&acpi_fan_driver);
	if (result < 0) {
		return -ENODEV;
	}

	return 0;
}

static void acpi_fan_exit(void)
{

	acpi_bus_unregister_driver(&acpi_fan_driver);

	return;
}

