/*
 *  acpi_thermal.c - ACPI Thermal Zone Driver ($Revision$)
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
 *
 *  This driver fully implements the ACPI thermal policy as described in the
 *  ACPI 2.0 Specification.
 *
 *  TBD: 1. Implement passive cooling hysteresis.
 *       2. Enhance passive cooling (CPU) states/limit interface to support
 *          concepts of 'multiple limiters', upper/lower limits, etc.
 *
 */

#include <atheos/kernel.h>
#include <atheos/irq.h>
#include <atheos/types.h>
#include <atheos/list.h>
#include <atheos/timer.h>
#include <atheos/acpi.h>
#include <posix/errno.h>
#include <posix/unistd.h>
#include <acpi/processor.h>
#include <macros.h>

#define ACPI_THERMAL_COMPONENT		0x04000000
#define ACPI_THERMAL_CLASS		"thermal_zone"
#define ACPI_THERMAL_DRIVER_NAME	"ACPI Thermal Zone Driver"
#define ACPI_THERMAL_DEVICE_NAME	"Thermal Zone"
#define ACPI_THERMAL_FILE_STATE		"state"
#define ACPI_THERMAL_FILE_TEMPERATURE	"temperature"
#define ACPI_THERMAL_FILE_TRIP_POINTS	"trip_points"
#define ACPI_THERMAL_FILE_COOLING_MODE	"cooling_mode"
#define ACPI_THERMAL_FILE_POLLING_FREQ	"polling_frequency"
#define ACPI_THERMAL_NOTIFY_TEMPERATURE	0x80
#define ACPI_THERMAL_NOTIFY_THRESHOLDS	0x81
#define ACPI_THERMAL_NOTIFY_DEVICES	0x82
#define ACPI_THERMAL_NOTIFY_CRITICAL	0xF0
#define ACPI_THERMAL_NOTIFY_HOT		0xF1
#define ACPI_THERMAL_MODE_ACTIVE	0x00
#define ACPI_THERMAL_MODE_PASSIVE	0x01
#define ACPI_THERMAL_MODE_CRITICAL   	0xff
#define ACPI_THERMAL_PATH_POWEROFF	"/bin/reboot"

#define ACPI_THERMAL_MAX_ACTIVE	10
#define ACPI_THERMAL_MAX_LIMIT_STR_LEN 65

#define KELVIN_TO_CELSIUS(t)    (long)(((long)t-2732>=0) ? ((long)t-2732+5)/10 : ((long)t-2732-5)/10)
#define CELSIUS_TO_KELVIN(t)	((t+273)*10)

#define _COMPONENT		ACPI_THERMAL_COMPONENT
ACPI_MODULE_NAME		("acpi_thermal")


static int acpi_thermal_add (struct acpi_device *device);
static int acpi_thermal_remove (struct acpi_device *device, int type);

static struct acpi_driver acpi_thermal_driver = {
	.name =		ACPI_THERMAL_DRIVER_NAME,
	.class =	ACPI_THERMAL_CLASS,
	.ids =		ACPI_THERMAL_HID,
	.ops =		{
				.add =		acpi_thermal_add,
				.remove =	acpi_thermal_remove,
			},
};

struct acpi_thermal_state {
	u8			critical:1;
	u8			hot:1;
	u8			passive:1;
	u8			active:1;
	u8			reserved:4;
	int			active_index;
};

struct acpi_thermal_state_flags {
	u8			valid:1;
	u8			enabled:1;
	u8			reserved:6;
};

struct acpi_thermal_critical {
	struct acpi_thermal_state_flags flags;
	unsigned long		temperature;
};

struct acpi_thermal_hot {
	struct acpi_thermal_state_flags flags;
	unsigned long		temperature;
};

struct acpi_thermal_passive {
	struct acpi_thermal_state_flags flags;
	unsigned long		temperature;
	unsigned long		tc1;
	unsigned long		tc2;
	unsigned long		tsp;
	struct acpi_handle_list	devices;
};

struct acpi_thermal_active {
	struct acpi_thermal_state_flags flags;
	unsigned long		temperature;
	struct acpi_handle_list	devices;
};

struct acpi_thermal_trips {
	struct acpi_thermal_critical critical;
	struct acpi_thermal_hot	hot;
	struct acpi_thermal_passive passive;
	struct acpi_thermal_active active[ACPI_THERMAL_MAX_ACTIVE];
};

struct acpi_thermal_flags {
	u8			cooling_mode:1;		/* _SCP */
	u8			devices:1;		/* _TZD */
	u8			reserved:6;
};

struct acpi_thermal {
	acpi_handle		handle;
	acpi_bus_id		name;
	unsigned long		temperature;
	unsigned long		last_temperature;
	unsigned long		polling_frequency;
	u8			cooling_mode;
	volatile u8		zombie;
	struct acpi_thermal_flags flags;
	struct acpi_thermal_state state;
	struct acpi_thermal_trips trips;
	struct acpi_handle_list	devices;
	ktimer_t	timer;
};

/* --------------------------------------------------------------------------
                             Thermal Zone Management
   -------------------------------------------------------------------------- */

static int
acpi_thermal_get_temperature (
	struct acpi_thermal *tz)
{
	acpi_status		status = AE_OK;

	ACPI_FUNCTION_TRACE("acpi_thermal_get_temperature");

	if (!tz)
		return_VALUE(-EINVAL);

	tz->last_temperature = tz->temperature;

	status = acpi_evaluate_integer(tz->handle, "_TMP", NULL, &tz->temperature);
	if (ACPI_FAILURE(status))
		return_VALUE(-ENODEV);

	ACPI_DEBUG_PRINT((ACPI_DB_INFO, "Temperature is %lu dK\n", tz->temperature));

	return_VALUE(0);
}


static int
acpi_thermal_get_polling_frequency (
	struct acpi_thermal	*tz)
{
	acpi_status		status = AE_OK;

	ACPI_FUNCTION_TRACE("acpi_thermal_get_polling_frequency");

	if (!tz)
		return_VALUE(-EINVAL);

	status = acpi_evaluate_integer(tz->handle, "_TZP", NULL, &tz->polling_frequency);
	if (ACPI_FAILURE(status))
		return_VALUE(-ENODEV);

	ACPI_DEBUG_PRINT((ACPI_DB_INFO, "Polling frequency is %lu dS\n", tz->polling_frequency));

	return_VALUE(0);
}


static int
acpi_thermal_set_polling (
	struct acpi_thermal	*tz,
	int			seconds)
{
	ACPI_FUNCTION_TRACE("acpi_thermal_set_polling");

	if (!tz)
		return_VALUE(-EINVAL);

	tz->polling_frequency = seconds * 10;	/* Convert value to deci-seconds */

	ACPI_DEBUG_PRINT((ACPI_DB_INFO, "Polling frequency set to %lu seconds\n", tz->polling_frequency));

	return_VALUE(0);
}


static int
acpi_thermal_set_cooling_mode (
	struct acpi_thermal	*tz,
	int			mode)
{
	acpi_status		status = AE_OK;
	union acpi_object	arg0 = {ACPI_TYPE_INTEGER};
	struct acpi_object_list	arg_list = {1, &arg0};
	acpi_handle		handle = NULL;

	ACPI_FUNCTION_TRACE("acpi_thermal_set_cooling_mode");

	if (!tz)
		return_VALUE(-EINVAL);

	status = acpi_get_handle(tz->handle, "_SCP", &handle);
	if (ACPI_FAILURE(status)) {
		ACPI_DEBUG_PRINT((ACPI_DB_INFO, "_SCP not present\n"));
		return_VALUE(-ENODEV);
	}

	arg0.integer.value = mode;

	status = acpi_evaluate_object(handle, NULL, &arg_list, NULL);
	if (ACPI_FAILURE(status))
		return_VALUE(-ENODEV);

	tz->cooling_mode = mode;

	printk( "ACPI: Cooling mode [%s]\n", 
		mode?"passive":"active" );

	return_VALUE(0);
}


static int
acpi_thermal_get_trip_points (
	struct acpi_thermal *tz)
{
	acpi_status		status = AE_OK;
	int			i = 0;

	ACPI_FUNCTION_TRACE("acpi_thermal_get_trip_points");

	if (!tz)
		return_VALUE(-EINVAL);

	/* Critical Shutdown (required) */

	status = acpi_evaluate_integer(tz->handle, "_CRT", NULL, 
		&tz->trips.critical.temperature);
	if (ACPI_FAILURE(status)) {
		tz->trips.critical.flags.valid = 0;
		ACPI_DEBUG_PRINT((ACPI_DB_ERROR, "No critical threshold\n"));
		return_VALUE(-ENODEV);
	}
	else {
		tz->trips.critical.flags.valid = 1;
		printk( "ACPI: Found critical threshold [%lu]\n", tz->trips.critical.temperature);
	}

	/* Critical Sleep (optional) */

	status = acpi_evaluate_integer(tz->handle, "_HOT", NULL, &tz->trips.hot.temperature);
	if (ACPI_FAILURE(status)) {
		tz->trips.hot.flags.valid = 0;
		ACPI_DEBUG_PRINT((ACPI_DB_INFO, "No hot threshold\n"));
	}
	else {
		tz->trips.hot.flags.valid = 1;
		printk( "ACPI: Found hot threshold [%lu]\n", tz->trips.hot.temperature);
	}

	/* Passive: Processors (optional) */

	status = acpi_evaluate_integer(tz->handle, "_PSV", NULL, &tz->trips.passive.temperature);
	if (ACPI_FAILURE(status)) {
		tz->trips.passive.flags.valid = 0;
		ACPI_DEBUG_PRINT((ACPI_DB_INFO, "No passive threshold\n"));
	}
	else {
		tz->trips.passive.flags.valid = 1;

		status = acpi_evaluate_integer(tz->handle, "_TC1", NULL, &tz->trips.passive.tc1);
		if (ACPI_FAILURE(status))
			tz->trips.passive.flags.valid = 0;

		status = acpi_evaluate_integer(tz->handle, "_TC2", NULL, &tz->trips.passive.tc2);
		if (ACPI_FAILURE(status))
			tz->trips.passive.flags.valid = 0;

		status = acpi_evaluate_integer(tz->handle, "_TSP", NULL, &tz->trips.passive.tsp);
		if (ACPI_FAILURE(status))
			tz->trips.passive.flags.valid = 0;

		status = acpi_evaluate_reference(tz->handle, "_PSL", NULL, &tz->trips.passive.devices);
		if (ACPI_FAILURE(status))
			tz->trips.passive.flags.valid = 0;

		if (!tz->trips.passive.flags.valid)
			ACPI_DEBUG_PRINT((ACPI_DB_WARN, "Invalid passive threshold\n"));
		else
			printk( "ACPI: Found passive threshold [%lu]\n", tz->trips.passive.temperature);
	}

	/* Active: Fans, etc. (optional) */

	for (i=0; i<ACPI_THERMAL_MAX_ACTIVE; i++) {

		char name[5] = {'_','A','C',('0'+i),'\0'};

		status = acpi_evaluate_integer(tz->handle, name, NULL, &tz->trips.active[i].temperature);
		if (ACPI_FAILURE(status))
			break;

		name[2] = 'L';
		status = acpi_evaluate_reference(tz->handle, name, NULL, &tz->trips.active[i].devices);
		if (ACPI_SUCCESS(status)) {
			tz->trips.active[i].flags.valid = 1;
			printk( "ACPI: Found active threshold [%d]:[%lu]\n", i, tz->trips.active[i].temperature);
		}
		else
			ACPI_DEBUG_PRINT((ACPI_DB_ERROR, "Invalid active threshold [%d]\n", i));
	}

	return_VALUE(0);
}


static int
acpi_thermal_get_devices (
	struct acpi_thermal	*tz)
{
	acpi_status		status = AE_OK;

	ACPI_FUNCTION_TRACE("acpi_thermal_get_devices");

	if (!tz)
		return_VALUE(-EINVAL);

	status = acpi_evaluate_reference(tz->handle, "_TZD", NULL, &tz->devices);
	if (ACPI_FAILURE(status))
		return_VALUE(-ENODEV);

	return_VALUE(0);
}


static int
acpi_thermal_critical (
	struct acpi_thermal	*tz)
{
	int			result = 0;
	struct acpi_device	*device = NULL;

	ACPI_FUNCTION_TRACE("acpi_thermal_critical");

	if (!tz || !tz->trips.critical.flags.valid)
		return_VALUE(-EINVAL);

	if (tz->temperature >= tz->trips.critical.temperature) {
		ACPI_DEBUG_PRINT((ACPI_DB_WARN, "Critical trip point\n"));
		tz->trips.critical.flags.enabled = 1;
	}
	else if (tz->trips.critical.flags.enabled)
		tz->trips.critical.flags.enabled = 0;

	
	printk("ACPI: Critical temperature reached (%ld C), shutting down.\n", KELVIN_TO_CELSIUS(tz->temperature));

	reboot();

	return_VALUE(0);
}


static int
acpi_thermal_hot (
	struct acpi_thermal	*tz)
{
	int			result = 0;
	struct acpi_device	*device = NULL;

	ACPI_FUNCTION_TRACE("acpi_thermal_hot");

	if (!tz || !tz->trips.hot.flags.valid)
		return_VALUE(-EINVAL);

	if (tz->temperature >= tz->trips.hot.temperature) {
		ACPI_DEBUG_PRINT((ACPI_DB_WARN, "Hot trip point\n"));
		tz->trips.hot.flags.enabled = 1;
	}
	else if (tz->trips.hot.flags.enabled)
		tz->trips.hot.flags.enabled = 0;


	/* TBD: Call user-mode "sleep(S4)" function */

	return_VALUE(0);
}


static void
acpi_thermal_passive (
	struct acpi_thermal	*tz)
{
	int			result = 0;
	struct acpi_thermal_passive *passive = NULL;
	int			trend = 0;
	int			i = 0;

	ACPI_FUNCTION_TRACE("acpi_thermal_passive");

	if (!tz || !tz->trips.passive.flags.valid)
		return;

	passive = &(tz->trips.passive);

	/*
	 * Above Trip?
	 * -----------
	 * Calculate the thermal trend (using the passive cooling equation)
	 * and modify the performance limit for all passive cooling devices
	 * accordingly.  Note that we assume symmetry.
	 */
	if (tz->temperature >= passive->temperature) {
		trend = (passive->tc1 * (tz->temperature - tz->last_temperature)) + (passive->tc2 * (tz->temperature - passive->temperature));
		ACPI_DEBUG_PRINT((ACPI_DB_INFO, 
			"trend[%d]=(tc1[%lu]*(tmp[%lu]-last[%lu]))+(tc2[%lu]*(tmp[%lu]-psv[%lu]))\n", 
			trend, passive->tc1, tz->temperature, 
			tz->last_temperature, passive->tc2, 
			tz->temperature, passive->temperature));
		tz->trips.passive.flags.enabled = 1;
		/* Heating up? */
		if (trend > 0)
			for (i=0; i<passive->devices.count; i++) {
				printk( "Error: acpi_processor_set_thermal_limit() not implemented!\n" );
				#if 0
				acpi_processor_set_thermal_limit(
					passive->devices.handles[i], 
					ACPI_PROCESSOR_LIMIT_INCREMENT);
				#endif
			}
		/* Cooling off? */
		else if (trend < 0) {
			for (i=0; i<passive->devices.count; i++) {
				printk( "Error: acpi_processor_set_thermal_limit() not implemented!\n" );
				#if 0
				acpi_processor_set_thermal_limit(
					passive->devices.handles[i], 
					ACPI_PROCESSOR_LIMIT_DECREMENT);
				#endif
			}
			/*
			 * Leave cooling mode, even if the temp might
			 * higher than trip point This is because some
			 * machines might have long thermal polling
			 * frequencies (tsp) defined. We will fall back
			 * into passive mode in next cycle (probably quicker)
			 */
			if (result) {
				passive->flags.enabled = 0;
				ACPI_DEBUG_PRINT((ACPI_DB_INFO,
						  "Disabling passive cooling, still above threshold,"
						  " but we are cooling down\n"));
			}
		}
		return;
	}

	/*
	 * Below Trip?
	 * -----------
	 * Implement passive cooling hysteresis to slowly increase performance
	 * and avoid thrashing around the passive trip point.  Note that we
	 * assume symmetry.
	 */
	if (!passive->flags.enabled)
		return;
	for (i=0; i<passive->devices.count; i++) {
		printk( "Error: acpi_processor_set_thermal_limit() not implemented!\n" );
	#if 0
		result = acpi_processor_set_thermal_limit(
			passive->devices.handles[i], 
			ACPI_PROCESSOR_LIMIT_DECREMENT);
	#endif
	}
	if (result) {
		passive->flags.enabled = 0;
		ACPI_DEBUG_PRINT((ACPI_DB_INFO,
				  "Disabling passive cooling (zone is cool)\n"));
	}
}


static void
acpi_thermal_active (
	struct acpi_thermal	*tz)
{
	int			result = 0;
	struct acpi_thermal_active *active = NULL;
	int                     i = 0;
	int			j = 0;
	unsigned long		maxtemp = 0;

	ACPI_FUNCTION_TRACE("acpi_thermal_active");

	if (!tz)
		return;

	for (i=0; i<ACPI_THERMAL_MAX_ACTIVE; i++) {

		active = &(tz->trips.active[i]);
		if (!active || !active->flags.valid)
			break;

		/*
		 * Above Threshold?
		 * ----------------
		 * If not already enabled, turn ON all cooling devices
		 * associated with this active threshold.
		 */
		if (tz->temperature >= active->temperature) {
			if (active->temperature > maxtemp)
				tz->state.active_index = i;
			maxtemp = active->temperature;
			if (!active->flags.enabled) {
				for (j = 0; j < active->devices.count; j++) {
					result = acpi_bus_set_power(active->devices.handles[j], ACPI_STATE_D0);
					if (result) {
						printk( "ACPI: Unable to turn cooling device [%p] 'on'\n", active->devices.handles[j]);
						continue;
					}
					active->flags.enabled = 1;
					ACPI_DEBUG_PRINT((ACPI_DB_INFO, "Cooling device [%p] now 'on'\n", active->devices.handles[j]));
				}
			}
		}
		/*
		 * Below Threshold?
		 * ----------------
		 * Turn OFF all cooling devices associated with this
		 * threshold.
		 */
		else if (active->flags.enabled) {
			for (j = 0; j < active->devices.count; j++) {
				result = acpi_bus_set_power(active->devices.handles[j], ACPI_STATE_D3);
				if (result) {
					printk( "ACPI: Unable to turn cooling device [%p] 'off'\n", active->devices.handles[j]);
					continue;
				}
				active->flags.enabled = 0;
				ACPI_DEBUG_PRINT((ACPI_DB_INFO, "Cooling device [%p] now 'off'\n", active->devices.handles[j]));
			}
		}
	}

}


static void acpi_thermal_check (void *context);

static void
acpi_thermal_run (
	void*		data)
{
	struct acpi_thermal *tz = (struct acpi_thermal *)data;
	if (!tz->zombie)
		acpi_os_queue_for_execution(OSD_PRIORITY_GPE,  
			acpi_thermal_check, (void *) data);
}


static void
acpi_thermal_check (
	void                    *data)
{
	int			result = 0;
	struct acpi_thermal	*tz = (struct acpi_thermal *) data;
	unsigned long		sleep_time = 0;
	int			i = 0;
	struct acpi_thermal_state state;

	ACPI_FUNCTION_TRACE("acpi_thermal_check");

	if (!tz) {
		ACPI_DEBUG_PRINT((ACPI_DB_ERROR, "Invalid (NULL) context.\n"));
		return_VOID;
	}

	state = tz->state;

	result = acpi_thermal_get_temperature(tz);
	if (result)
		return_VOID;
	
	memset(&tz->state, 0, sizeof(tz->state));
	
	/*
	 * Check Trip Points
	 * -----------------
	 * Compare the current temperature to the trip point values to see
	 * if we've entered one of the thermal policy states.  Note that
	 * this function determines when a state is entered, but the 
	 * individual policy decides when it is exited (e.g. hysteresis).
	 */
	if (tz->trips.critical.flags.valid)
		state.critical |= (tz->temperature >= tz->trips.critical.temperature);
	if (tz->trips.hot.flags.valid)
		state.hot |= (tz->temperature >= tz->trips.hot.temperature);
	if (tz->trips.passive.flags.valid)
		state.passive |= (tz->temperature >= tz->trips.passive.temperature);
	for (i=0; i<ACPI_THERMAL_MAX_ACTIVE; i++)
		if (tz->trips.active[i].flags.valid)
			state.active |= (tz->temperature >= tz->trips.active[i].temperature);

	/*
	 * Invoke Policy
	 * -------------
	 * Separated from the above check to allow individual policy to 
	 * determine when to exit a given state.
	 */
	if (state.critical)
		acpi_thermal_critical(tz);
	if (state.hot)
		acpi_thermal_hot(tz);
	if (state.passive)
		acpi_thermal_passive(tz);
	if (state.active)
		acpi_thermal_active(tz);

	/*
	 * Calculate State
	 * ---------------
	 * Again, separated from the above two to allow independent policy
	 * decisions.
	 */
	if (tz->trips.critical.flags.enabled)
		tz->state.critical = 1;
	if (tz->trips.hot.flags.enabled)
		tz->state.hot = 1;
	if (tz->trips.passive.flags.enabled)
		tz->state.passive = 1;
	for (i=0; i<ACPI_THERMAL_MAX_ACTIVE; i++)
		tz->state.active |= tz->trips.active[i].flags.enabled;

	/*
	 * Calculate Sleep Time
	 * --------------------
	 * If we're in the passive state, use _TSP's value.  Otherwise
	 * use the default polling frequency (e.g. _TZP).  If no polling
	 * frequency is specified then we'll wait forever (at least until
	 * a thermal event occurs).  Note that _TSP and _TZD values are
	 * given in 1/10th seconds (we must covert to milliseconds).
	 */
	if (tz->state.passive)
		sleep_time = tz->trips.passive.tsp * 100;
	else if (tz->polling_frequency > 0)
		sleep_time = tz->polling_frequency * 100;

	ACPI_DEBUG_PRINT((ACPI_DB_INFO, "%s: temperature[%lu] sleep[%lu]\n", 
		tz->name, tz->temperature, sleep_time));

	/*
	 * Schedule Next Poll
	 * ------------------
	 */
	if (!sleep_time) {
	}
	else {
		start_timer( tz->timer, acpi_thermal_run, (void*)tz, sleep_time * 1000, true );
	}

	return_VOID;
}


/* --------------------------------------------------------------------------
                                 Driver Interface
   -------------------------------------------------------------------------- */

static void
acpi_thermal_notify (
	acpi_handle 		handle,
	u32 			event,
	void 			*data)
{
	struct acpi_thermal	*tz = (struct acpi_thermal *) data;
	struct acpi_device	*device = NULL;

	ACPI_FUNCTION_TRACE("acpi_thermal_notify");

	if (!tz)
		return_VOID;

	if (acpi_bus_get_device(tz->handle, &device))
		return_VOID;

	switch (event) {
	case ACPI_THERMAL_NOTIFY_TEMPERATURE:
		acpi_thermal_check(tz);
		break;
	case ACPI_THERMAL_NOTIFY_THRESHOLDS:
		acpi_thermal_get_trip_points(tz);
		acpi_thermal_check(tz);
		break;
	case ACPI_THERMAL_NOTIFY_DEVICES:
		if (tz->flags.devices)
			acpi_thermal_get_devices(tz);
		break;
	default:
		ACPI_DEBUG_PRINT((ACPI_DB_INFO,
			"Unsupported event [0x%x]\n", event));
		break;
	}

	return_VOID;
}


static int
acpi_thermal_get_info (
	struct acpi_thermal	*tz)
{
	int			result = 0;

	ACPI_FUNCTION_TRACE("acpi_thermal_get_info");

	if (!tz)
		return_VALUE(-EINVAL);

	/* Get temperature [_TMP] (required) */
	result = acpi_thermal_get_temperature(tz);
	if (result)
		return_VALUE(result);

	/* Get trip points [_CRT, _PSV, etc.] (required) */
	result = acpi_thermal_get_trip_points(tz);
	if (result)
		return_VALUE(result);

	/* Set the cooling mode [_SCP] to active cooling (default) */
	result = acpi_thermal_set_cooling_mode(tz, ACPI_THERMAL_MODE_ACTIVE);
	if (!result) 
		tz->flags.cooling_mode = 1;
	else { 
		/* Oh,we have not _SCP method.
		   Generally show cooling_mode by _ACx, _PSV,spec 12.2*/
		tz->flags.cooling_mode = 0;
		if ( tz->trips.active[0].flags.valid && tz->trips.passive.flags.valid ) {
			if ( tz->trips.passive.temperature > tz->trips.active[0].temperature )
				tz->cooling_mode = ACPI_THERMAL_MODE_ACTIVE;
			else 
				tz->cooling_mode = ACPI_THERMAL_MODE_PASSIVE;
		} else if ( !tz->trips.active[0].flags.valid && tz->trips.passive.flags.valid ) {
			tz->cooling_mode = ACPI_THERMAL_MODE_PASSIVE;
		} else if ( tz->trips.active[0].flags.valid && !tz->trips.passive.flags.valid ) {
			tz->cooling_mode = ACPI_THERMAL_MODE_ACTIVE;
		} else {
			/* _ACx and _PSV are optional, but _CRT is required */
			tz->cooling_mode = ACPI_THERMAL_MODE_CRITICAL;
		}
	}

	/* Get default polling frequency [_TZP] (optional) */
	acpi_thermal_get_polling_frequency(tz);

	/* Get devices in this thermal zone [_TZD] (optional) */
	result = acpi_thermal_get_devices(tz);
	if (!result)
		tz->flags.devices = 1;

	return_VALUE(0);
}


static int
acpi_thermal_add (
	struct acpi_device 		*device)
{
	int			result = 0;
	acpi_status		status = AE_OK;
	struct acpi_thermal	*tz = NULL;

	ACPI_FUNCTION_TRACE("acpi_thermal_add");

	if (!device)
		return_VALUE(-EINVAL);

	tz = kmalloc(sizeof(struct acpi_thermal), MEMF_KERNEL);
	if (!tz)
		return_VALUE(-ENOMEM);
	memset(tz, 0, sizeof(struct acpi_thermal));

	tz->handle = device->handle;
	strcpy(tz->name, device->pnp.bus_id);
	strcpy(acpi_device_name(device), ACPI_THERMAL_DEVICE_NAME);
	strcpy(acpi_device_class(device), ACPI_THERMAL_CLASS);
	acpi_driver_data(device) = tz;

	result = acpi_thermal_get_info(tz);
	if (result)
		goto end;

	tz->timer = create_timer();

	acpi_thermal_check(tz);

	status = acpi_install_notify_handler(tz->handle,
		ACPI_DEVICE_NOTIFY, acpi_thermal_notify, tz);
	if (ACPI_FAILURE(status)) {
		ACPI_DEBUG_PRINT((ACPI_DB_ERROR,
			"Error installing notify handler\n"));
		result = -ENODEV;
		goto end;
	}

	printk("ACPI: %s [%s] (%ld C)\n",
		acpi_device_name(device), acpi_device_bid(device),
		KELVIN_TO_CELSIUS(tz->temperature));

end:
	if (result) {
		kfree(tz);
	}

	return_VALUE(result);
}


static int
acpi_thermal_remove (
	struct acpi_device	*device,
	int			type)
{
	acpi_status		status = AE_OK;
	struct acpi_thermal	*tz = NULL;

	ACPI_FUNCTION_TRACE("acpi_thermal_remove");

	if (!device || !acpi_driver_data(device))
		return_VALUE(-EINVAL);

	tz = (struct acpi_thermal *) acpi_driver_data(device);

	/* avoid timer adding new defer task */
	tz->zombie = 1;
	/* wait for running timer (on other CPUs) finish */
	delete_timer( tz->timer );
	/* synchronize deferred task */
	acpi_os_wait_events_complete(NULL);

	status = acpi_remove_notify_handler(tz->handle,
		ACPI_DEVICE_NOTIFY, acpi_thermal_notify);
	if (ACPI_FAILURE(status))
		ACPI_DEBUG_PRINT((ACPI_DB_ERROR,
			"Error removing notify handler\n"));

	/* Terminate policy */
	if (tz->trips.passive.flags.valid
		&& tz->trips.passive.flags.enabled) {
		tz->trips.passive.flags.enabled = 0;
		acpi_thermal_passive(tz);
	}
	if (tz->trips.active[0].flags.valid
		&& tz->trips.active[0].flags.enabled) {
		tz->trips.active[0].flags.enabled = 0;
		acpi_thermal_active(tz);
	}


	kfree(tz);
	return_VALUE(0);
}


int
acpi_thermal_init (void)
{
	int			result = 0;

	ACPI_FUNCTION_TRACE("acpi_thermal_init");

	result = acpi_bus_register_driver(&acpi_thermal_driver);
	if (result < 0) {
		return_VALUE(-ENODEV);
	}

	return_VALUE(0);
}


void
acpi_thermal_exit (void)
{
	ACPI_FUNCTION_TRACE("acpi_thermal_exit");

	acpi_bus_unregister_driver(&acpi_thermal_driver);

	
	return_VOID;
}







