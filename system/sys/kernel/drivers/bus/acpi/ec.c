/*
 *  acpi_ec.c - ACPI Embedded Controller Driver ($Revision$)
 *
 *  Copyright (C) 2004 Luming Yu <luming.yu@intel.com>
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
#include <atheos/spinlock.h>
#include <atheos/udelay.h>
#include <atheos/acpi.h>
#include <atheos/time.h>
#include <posix/errno.h>
#include <macros.h>


#define _COMPONENT		ACPI_EC_COMPONENT
ACPI_MODULE_NAME("ec");
#define ACPI_EC_COMPONENT		0x00100000
#define ACPI_EC_CLASS			"embedded_controller"
#define ACPI_EC_HID			"PNP0C09"
#define ACPI_EC_DEVICE_NAME		"Embedded Controller"
#define ACPI_EC_FILE_INFO		"info"
#undef PREFIX
#define PREFIX				"ACPI: EC: "
/* EC status register */
#define ACPI_EC_FLAG_OBF	0x01	/* Output buffer full */
#define ACPI_EC_FLAG_IBF	0x02	/* Input buffer full */
#define ACPI_EC_FLAG_BURST	0x10	/* burst mode */
#define ACPI_EC_FLAG_SCI	0x20	/* EC-SCI occurred */
/* EC commands */
enum ec_command {
	ACPI_EC_COMMAND_READ = 0x80,
	ACPI_EC_COMMAND_WRITE = 0x81,
	ACPI_EC_BURST_ENABLE = 0x82,
	ACPI_EC_BURST_DISABLE = 0x83,
	ACPI_EC_COMMAND_QUERY = 0x84,
};
/* EC events */
enum ec_event {
	ACPI_EC_EVENT_OBF_1 = 1,	/* Output buffer full */
	ACPI_EC_EVENT_IBF_0,	/* Input buffer empty */
};

#define ACPI_EC_DELAY		500	/* Wait 500ms max. during EC ops */
#define ACPI_EC_UDELAY_GLK	1000	/* Wait 1ms max. to get global lock */

static enum ec_mode {
	EC_INTR = 1,		/* Output buffer full */
	EC_POLL,		/* Input buffer empty */
} acpi_ec_mode = EC_POLL;

static int acpi_ec_remove(struct acpi_device *device, int type);
static int acpi_ec_start(struct acpi_device *device);
static int acpi_ec_stop(struct acpi_device *device, int type);
static int acpi_ec_add(struct acpi_device *device);

static struct acpi_driver acpi_ec_driver = {
	.name = "ACPI Embedded Controller Driver",
	.class = ACPI_EC_CLASS,
	.ids = ACPI_EC_HID,
	.ops = {
		.add = acpi_ec_add,
		.remove = acpi_ec_remove,
		.start = acpi_ec_start,
		.stop = acpi_ec_stop,
		},
};

/* If we find an EC via the ECDT, we need to keep a ptr to its context */
static struct acpi_ec {
	acpi_handle handle;
	unsigned long uid;
	unsigned long gpe;
	unsigned long command_addr;
	unsigned long data_addr;
	unsigned long global_lock;
	SpinLock_s lock;
	atomic_t query_pending;
	atomic_t event_count;
	atomic_t leaving_burst;	/* 0 : No, 1 : Yes, 2: abort */
	sem_id wait;
} *ec_ecdt;

/* External interfaces use first EC only, so remember */
static struct acpi_device *first_ec;

/* --------------------------------------------------------------------------
                             Transaction Management
   -------------------------------------------------------------------------- */

static inline u8 acpi_ec_read_status(struct acpi_ec *ec)
{
	return inb(ec->command_addr);
}

static inline u8 acpi_ec_read_data(struct acpi_ec *ec)
{
	return inb(ec->data_addr);
}

static inline void acpi_ec_write_cmd(struct acpi_ec *ec, u8 command)
{
	outb(command, ec->command_addr);
}

static inline void acpi_ec_write_data(struct acpi_ec *ec, u8 data)
{
	outb(data, ec->data_addr);
}

static inline int acpi_ec_check_status(struct acpi_ec *ec, enum ec_event event,
				       unsigned old_count)
{
	u8 status = acpi_ec_read_status(ec);
	if (old_count == atomic_read(&ec->event_count))
		return 0;
	if (event == ACPI_EC_EVENT_OBF_1) {
		if (status & ACPI_EC_FLAG_OBF)
			return 1;
	} else if (event == ACPI_EC_EVENT_IBF_0) {
		if (!(status & ACPI_EC_FLAG_IBF))
			return 1;
	}

	return 0;
}

static int acpi_ec_wait(struct acpi_ec *ec, enum ec_event event, unsigned count)
{
	if (acpi_ec_mode == EC_POLL) {
		bigtime_t delay = get_system_time() + ACPI_EC_DELAY * 1000;
		while ( delay > get_system_time()) {
			if (acpi_ec_check_status(ec, event, 0))
				return 0;
		}
	} else {
		bool bFinished = false;
		for( ;; )
		{
			if( sleep_on_sem( ec->wait, ACPI_EC_DELAY * 1000 ) == -ETIME )
				break;			
			if( acpi_ec_check_status(ec, event, count) )
			{
				bFinished = true;
				break;
			}

		}
		if (bFinished ||
		    acpi_ec_check_status(ec, event, 0)) {
			return 0;
		} else {
			printk("acpi_ec_wait timeout,"
			       " status = %d, expect_event = %d\n",
			       acpi_ec_read_status(ec), event);
		}
	}

	return -ETIME;
}

#ifdef ACPI_FUTURE_USAGE
/*
 * Note: samsung nv5000 doesn't work with ec burst mode.
 * http://bugzilla.kernel.org/show_bug.cgi?id=4980
 */
int acpi_ec_enter_burst_mode(struct acpi_ec *ec)
{
	u8 tmp = 0;
	u8 status = 0;

	status = acpi_ec_read_status(ec);
	if (status != -EINVAL && !(status & ACPI_EC_FLAG_BURST)) {
		status = acpi_ec_wait(ec, ACPI_EC_EVENT_IBF_0);
		if (status)
			goto end;
		acpi_ec_write_cmd(ec, ACPI_EC_BURST_ENABLE);
		status = acpi_ec_wait(ec, ACPI_EC_EVENT_OBF_1);
		tmp = acpi_ec_read_data(ec);
		if (tmp != 0x90) {	/* Burst ACK byte */
			return -EINVAL;
		}
	}

	atomic_set(&ec->leaving_burst, 0);
	return 0;
      end:
	ACPI_EXCEPTION((AE_INFO, status, "EC wait, burst mode"));
	return -1;
}

int acpi_ec_leave_burst_mode(struct acpi_ec *ec)
{
	u8 status = 0;

	status = acpi_ec_read_status(ec);
	if (status != -EINVAL && (status & ACPI_EC_FLAG_BURST)) {
		status = acpi_ec_wait(ec, ACPI_EC_EVENT_IBF_0);
		if (status)
			goto end;
		acpi_ec_write_cmd(ec, ACPI_EC_BURST_DISABLE);
		acpi_ec_wait(ec, ACPI_EC_EVENT_IBF_0);
	}
	atomic_set(&ec->leaving_burst, 1);
	return 0;
      end:
	ACPI_EXCEPTION((AE_INFO, status, "EC leave burst mode"));
	return -1;
}
#endif				/* ACPI_FUTURE_USAGE */

static int acpi_ec_transaction_unlocked(struct acpi_ec *ec, u8 command,
					const u8 * wdata, unsigned wdata_len,
					u8 * rdata, unsigned rdata_len)
{
	int result = 0;
	unsigned count = atomic_read(&ec->event_count);
	acpi_ec_write_cmd(ec, command);

	for (; wdata_len > 0; --wdata_len) {
		result = acpi_ec_wait(ec, ACPI_EC_EVENT_IBF_0, count);
		if (result) {
			printk(
			       "write_cmd timeout, command = %d\n", command);
			goto end;
		}
		count = atomic_read(&ec->event_count);
		acpi_ec_write_data(ec, *(wdata++));
	}

	if (!rdata_len) {
		result = acpi_ec_wait(ec, ACPI_EC_EVENT_IBF_0, count);
		if (result) {
			printk(
			       "finish-write timeout, command = %d\n", command);
			goto end;
		}
	} else if (command == ACPI_EC_COMMAND_QUERY) {
		atomic_set(&ec->query_pending, 0);
	}

	for (; rdata_len > 0; --rdata_len) {
		result = acpi_ec_wait(ec, ACPI_EC_EVENT_OBF_1, count);
		if (result) {
			printk("read timeout, command = %d\n",
			       command);
			goto end;
		}
		count = atomic_read(&ec->event_count);
		*(rdata++) = acpi_ec_read_data(ec);
	}
      end:
	return result;
}

static int acpi_ec_transaction(struct acpi_ec *ec, u8 command,
			       const u8 * wdata, unsigned wdata_len,
			       u8 * rdata, unsigned rdata_len)
{
	int status;
	u32 glk;

	if (!ec || (wdata_len && !wdata) || (rdata_len && !rdata))
		return -EINVAL;

	if (rdata)
		memset(rdata, 0, rdata_len);

	spinlock(&ec->lock);
	if (ec->global_lock) {
		status = acpi_acquire_global_lock(ACPI_EC_UDELAY_GLK, &glk);
		if (ACPI_FAILURE(status)) {
			spinlock(&ec->lock);
			return -ENODEV;
		}
	}

	/* Make sure GPE is enabled before doing transaction */
	acpi_enable_gpe(NULL, ec->gpe, ACPI_NOT_ISR);

	status = acpi_ec_wait(ec, ACPI_EC_EVENT_IBF_0, 0);
	if (status) {
		printk(
		       "input buffer is not empty, aborting transaction\n");
		goto end;
	}

	status = acpi_ec_transaction_unlocked(ec, command,
					      wdata, wdata_len,
					      rdata, rdata_len);

      end:

	if (ec->global_lock)
		acpi_release_global_lock(glk);
	spinlock(&ec->lock);

	return status;
}

static int acpi_ec_read(struct acpi_ec *ec, u8 address, u8 * data)
{
	int result;
	u8 d;

	result = acpi_ec_transaction(ec, ACPI_EC_COMMAND_READ,
				     &address, 1, &d, 1);
	*data = d;
	return result;
}

static int acpi_ec_write(struct acpi_ec *ec, u8 address, u8 data)
{
	u8 wdata[2] = { address, data };
	return acpi_ec_transaction(ec, ACPI_EC_COMMAND_WRITE,
				   wdata, 2, NULL, 0);
}

/*
 * Externally callable EC access functions. For now, assume 1 EC only
 */
int ec_read(u8 addr, u8 * val)
{
	struct acpi_ec *ec;
	int err;
	u8 temp_data;

	if (!first_ec)
		return -ENODEV;

	ec = acpi_driver_data(first_ec);

	err = acpi_ec_read(ec, addr, &temp_data);

	if (!err) {
		*val = temp_data;
		return 0;
	} else
		return err;
}

EXPORT_SYMBOL(ec_read);

int ec_write(u8 addr, u8 val)
{
	struct acpi_ec *ec;
	int err;

	if (!first_ec)
		return -ENODEV;

	ec = acpi_driver_data(first_ec);

	err = acpi_ec_write(ec, addr, val);

	return err;
}

EXPORT_SYMBOL(ec_write);

int ec_transaction(u8 command,
		   const u8 * wdata, unsigned wdata_len,
		   u8 * rdata, unsigned rdata_len)
{
	struct acpi_ec *ec;

	if (!first_ec)
		return -ENODEV;

	ec = acpi_driver_data(first_ec);

	return acpi_ec_transaction(ec, command, wdata,
				   wdata_len, rdata, rdata_len);
}

EXPORT_SYMBOL(ec_transaction);

static int acpi_ec_query(struct acpi_ec *ec, u8 * data)
{
	int result;
	u8 d;

	if (!ec || !data)
		return -EINVAL;

	/*
	 * Query the EC to find out which _Qxx method we need to evaluate.
	 * Note that successful completion of the query causes the ACPI_EC_SCI
	 * bit to be cleared (and thus clearing the interrupt source).
	 */

	result = acpi_ec_transaction(ec, ACPI_EC_COMMAND_QUERY, NULL, 0, &d, 1);
	if (result)
		return result;

	if (!d)
		return -ENODATA;

	*data = d;
	return 0;
}

/* --------------------------------------------------------------------------
                                Event Management
   -------------------------------------------------------------------------- */

static void acpi_ec_gpe_query(void *ec_cxt)
{
	struct acpi_ec *ec = (struct acpi_ec *)ec_cxt;
	u8 value = 0;
	char object_name[8];

	if (!ec || acpi_ec_query(ec, &value))
		return;

	sprintf(object_name, "_Q%2.2X", value);

	ACPI_DEBUG_PRINT((ACPI_DB_INFO, "Evaluating %s", object_name));

	acpi_evaluate_object(ec->handle, object_name, NULL, NULL);
}

static u32 acpi_ec_gpe_handler(void *data)
{
	acpi_status status = AE_OK;
	u8 value;
	struct acpi_ec *ec = (struct acpi_ec *)data;
	atomic_inc(&ec->event_count);
	if (acpi_ec_mode == EC_INTR) {
		wakeup_sem(ec->wait,true);
	}

	value = acpi_ec_read_status(ec);
	if ((value & ACPI_EC_FLAG_SCI) && !atomic_read(&ec->query_pending)) {
		atomic_set(&ec->query_pending, 1);
		status =
		    acpi_os_execute(OSL_EC_BURST_HANDLER, acpi_ec_gpe_query,
				    ec);
	}

	return status == AE_OK ?
	    ACPI_INTERRUPT_HANDLED : ACPI_INTERRUPT_NOT_HANDLED;
}

/* --------------------------------------------------------------------------
                             Address Space Management
   -------------------------------------------------------------------------- */

static acpi_status
acpi_ec_space_setup(acpi_handle region_handle,
		    u32 function, void *handler_context, void **return_context)
{
	/*
	 * The EC object is in the handler context and is needed
	 * when calling the acpi_ec_space_handler.
	 */
	*return_context = (function != ACPI_REGION_DEACTIVATE) ?
	    handler_context : NULL;

	return AE_OK;
}

static acpi_status
acpi_ec_space_handler(u32 function,
		      acpi_physical_address address,
		      u32 bit_width,
		      acpi_integer * value,
		      void *handler_context, void *region_context)
{
	int result = 0;
	struct acpi_ec *ec = NULL;
	u64 temp = *value;
	acpi_integer f_v = 0;
	int i = 0;

	if ((address > 0xFF) || !value || !handler_context)
		return AE_BAD_PARAMETER;

	if (bit_width != 8 && acpi_strict) {
		return AE_BAD_PARAMETER;
	}

	ec = (struct acpi_ec *)handler_context;

      next_byte:
	switch (function) {
	case ACPI_READ:
		temp = 0;
		result = acpi_ec_read(ec, (u8) address, (u8 *) & temp);
		break;
	case ACPI_WRITE:
		result = acpi_ec_write(ec, (u8) address, (u8) temp);
		break;
	default:
		result = -EINVAL;
		goto out;
		break;
	}

	bit_width -= 8;
	if (bit_width) {
		if (function == ACPI_READ)
			f_v |= temp << 8 * i;
		if (function == ACPI_WRITE)
			temp >>= 8;
		i++;
		address++;
		goto next_byte;
	}

	if (function == ACPI_READ) {
		f_v |= temp << 8 * i;
		*value = f_v;
	}

      out:
	switch (result) {
	case -EINVAL:
		return AE_BAD_PARAMETER;
		break;
	case -ENODEV:
		return AE_NOT_FOUND;
		break;
	case -ETIME:
		return AE_TIME;
		break;
	default:
		return AE_OK;
	}
}



/* --------------------------------------------------------------------------
                               Driver Interface
   -------------------------------------------------------------------------- */

static int acpi_ec_add(struct acpi_device *device)
{
	int result = 0;
	acpi_status status = AE_OK;
	struct acpi_ec *ec = NULL;

	if (!device)
		return -EINVAL;

	ec = kmalloc(sizeof(struct acpi_ec), MEMF_KERNEL);
	if (!ec)
		return -ENOMEM;

	ec->handle = device->handle;
	ec->uid = -1;
	spinlock_init(&ec->lock, "acpi_ec_lock");
	atomic_set(&ec->query_pending, 0);
	atomic_set(&ec->event_count, 1);
	if (acpi_ec_mode == EC_INTR) {
		atomic_set(&ec->leaving_burst, 1);
		ec->wait = create_semaphore( "ec_wait", 0, 0 );
	}
	strcpy(acpi_device_name(device), ACPI_EC_DEVICE_NAME);
	strcpy(acpi_device_class(device), ACPI_EC_CLASS);
	acpi_driver_data(device) = ec;

	/* Use the global lock for all EC transactions? */
	acpi_evaluate_integer(ec->handle, "_GLK", NULL, &ec->global_lock);

	/* XXX we don't test uids, because on some boxes ecdt uid = 0, see:
	   http://bugzilla.kernel.org/show_bug.cgi?id=6111 */
	if (ec_ecdt) {
		acpi_remove_address_space_handler(ACPI_ROOT_OBJECT,
						  ACPI_ADR_SPACE_EC,
						  &acpi_ec_space_handler);

		acpi_remove_gpe_handler(NULL, ec_ecdt->gpe,
					&acpi_ec_gpe_handler);

		kfree(ec_ecdt);
	}

	/* Get GPE bit assignment (EC events). */
	/* TODO: Add support for _GPE returning a package */
	status = acpi_evaluate_integer(ec->handle, "_GPE", NULL, &ec->gpe);
	if (ACPI_FAILURE(status)) {
		ACPI_EXCEPTION((AE_INFO, status,
				"Obtaining GPE bit assignment"));
		result = -ENODEV;
		goto end;
	}


	printk( "ACPI: %s [%s] (gpe %d)\n",
			  acpi_device_name(device), acpi_device_bid(device),
			  (u32) ec->gpe);

	if (!first_ec)
		first_ec = device;

      end:
	if (result)
		kfree(ec);

	return result;
}

static int acpi_ec_remove(struct acpi_device *device, int type)
{
	struct acpi_ec *ec = NULL;

	if (!device)
		return -EINVAL;

	ec = acpi_driver_data(device);


	kfree(ec);

	return 0;
}

static acpi_status
acpi_ec_io_ports(struct acpi_resource *resource, void *context)
{
	struct acpi_ec *ec = (struct acpi_ec *)context;

	if (resource->type != ACPI_RESOURCE_TYPE_IO) {
		return AE_OK;
	}

	/*
	 * The first address region returned is the data port, and
	 * the second address region returned is the status/command
	 * port.
	 */
	if (ec->data_addr == 0) {
		ec->data_addr = resource->data.io.minimum;
	} else if (ec->command_addr == 0) {
		ec->command_addr = resource->data.io.minimum;
	} else {
		return AE_CTRL_TERMINATE;
	}

	return AE_OK;
}

static int acpi_ec_start(struct acpi_device *device)
{
	acpi_status status = AE_OK;
	struct acpi_ec *ec = NULL;

	if (!device)
		return -EINVAL;

	ec = acpi_driver_data(device);

	if (!ec)
		return -EINVAL;

	/*
	 * Get I/O port addresses. Convert to GAS format.
	 */
	status = acpi_walk_resources(ec->handle, METHOD_NAME__CRS,
				     acpi_ec_io_ports, ec);
	if (ACPI_FAILURE(status) || ec->command_addr == 0) {
		ACPI_EXCEPTION((AE_INFO, status,
				"Error getting I/O port addresses"));
		return -ENODEV;
	}

	ACPI_DEBUG_PRINT((ACPI_DB_INFO, "gpe=0x%02lx, ports=0x%2lx,0x%2lx",
			  ec->gpe, ec->command_addr, ec->data_addr));

	/*
	 * Install GPE handler
	 */
	status = acpi_install_gpe_handler(NULL, ec->gpe,
					  ACPI_GPE_EDGE_TRIGGERED,
					  &acpi_ec_gpe_handler, ec);
	if (ACPI_FAILURE(status)) {
		return -ENODEV;
	}
	acpi_set_gpe_type(NULL, ec->gpe, ACPI_GPE_TYPE_RUNTIME);
	acpi_enable_gpe(NULL, ec->gpe, ACPI_NOT_ISR);

	status = acpi_install_address_space_handler(ec->handle,
						    ACPI_ADR_SPACE_EC,
						    &acpi_ec_space_handler,
						    &acpi_ec_space_setup, ec);
	if (ACPI_FAILURE(status)) {
		acpi_remove_gpe_handler(NULL, ec->gpe, &acpi_ec_gpe_handler);
		return -ENODEV;
	}

	return AE_OK;
}

static int acpi_ec_stop(struct acpi_device *device, int type)
{
	acpi_status status = AE_OK;
	struct acpi_ec *ec = NULL;

	if (!device)
		return -EINVAL;

	ec = acpi_driver_data(device);

	status = acpi_remove_address_space_handler(ec->handle,
						   ACPI_ADR_SPACE_EC,
						   &acpi_ec_space_handler);
	if (ACPI_FAILURE(status))
		return -ENODEV;

	status = acpi_remove_gpe_handler(NULL, ec->gpe, &acpi_ec_gpe_handler);
	if (ACPI_FAILURE(status))
		return -ENODEV;

	return 0;
}

#if 0
static acpi_status __init
acpi_fake_ecdt_callback(acpi_handle handle,
			u32 Level, void *context, void **retval)
{
	acpi_status status;

	mutex_init(&ec_ecdt->lock);
	atomic_set(&ec_ecdt->event_count, 1);
	if (acpi_ec_mode == EC_INTR) {
		init_waitqueue_head(&ec_ecdt->wait);
	}
	status = acpi_walk_resources(handle, METHOD_NAME__CRS,
				     acpi_ec_io_ports, ec_ecdt);
	if (ACPI_FAILURE(status))
		return status;

	ec_ecdt->uid = -1;
	acpi_evaluate_integer(handle, "_UID", NULL, &ec_ecdt->uid);

	status = acpi_evaluate_integer(handle, "_GPE", NULL, &ec_ecdt->gpe);
	if (ACPI_FAILURE(status))
		return status;
	ec_ecdt->global_lock = TRUE;
	ec_ecdt->handle = handle;

	ACPI_DEBUG_PRINT((ACPI_DB_INFO, "GPE=0x%02lx, ports=0x%2lx, 0x%2lx",
			  ec_ecdt->gpe, ec_ecdt->command_addr,
			  ec_ecdt->data_addr));

	return AE_CTRL_TERMINATE;
}

/*
 * Some BIOS (such as some from Gateway laptops) access EC region very early
 * such as in BAT0._INI or EC._INI before an EC device is found and
 * do not provide an ECDT. According to ACPI spec, ECDT isn't mandatorily
 * required, but if EC regison is accessed early, it is required.
 * The routine tries to workaround the BIOS bug by pre-scan EC device
 * It assumes that _CRS, _HID, _GPE, _UID methods of EC don't touch any
 * op region (since _REG isn't invoked yet). The assumption is true for
 * all systems found.
 */
static int __init acpi_ec_fake_ecdt(void)
{
	acpi_status status;
	int ret = 0;

	ACPI_DEBUG_PRINT((ACPI_DB_INFO, "Try to make an fake ECDT"));

	ec_ecdt = kzalloc(sizeof(struct acpi_ec), GFP_KERNEL);
	if (!ec_ecdt) {
		ret = -ENOMEM;
		goto error;
	}

	status = acpi_get_devices(ACPI_EC_HID,
				  acpi_fake_ecdt_callback, NULL, NULL);
	if (ACPI_FAILURE(status)) {
		kfree(ec_ecdt);
		ec_ecdt = NULL;
		ret = -ENODEV;
		ACPI_EXCEPTION((AE_INFO, status, "Can't make an fake ECDT"));
		goto error;
	}
	return 0;
      error:
	return ret;
}
#endif
static int acpi_ec_get_real_ecdt(void)
{
	acpi_status status;
	struct acpi_table_ecdt *ecdt_ptr;

	status = acpi_get_table(ACPI_SIG_ECDT, 1,
				(struct acpi_table_header **)&ecdt_ptr);
	if (ACPI_FAILURE(status))
		return -ENODEV;

	ACPI_DEBUG_PRINT((ACPI_DB_INFO, "Found ECDT"));

	/*
	 * Generate a temporary ec context to use until the namespace is scanned
	 */
	ec_ecdt = kmalloc(sizeof(struct acpi_ec), MEMF_KERNEL);
	if (!ec_ecdt)
		return -ENOMEM;

	spinlock_init(&ec_ecdt->lock, "acpi_ec_lock");
	atomic_set(&ec_ecdt->event_count, 1);
	if (acpi_ec_mode == EC_INTR) {
		ec_ecdt->wait = create_semaphore( "ec_wait", 0, 0 );
	}
	ec_ecdt->command_addr = ecdt_ptr->control.address;
	ec_ecdt->data_addr = ecdt_ptr->data.address;
	ec_ecdt->gpe = ecdt_ptr->gpe;
	/* use the GL just to be safe */
	ec_ecdt->global_lock = TRUE;
	ec_ecdt->uid = ecdt_ptr->uid;

	status = acpi_get_handle(NULL, ecdt_ptr->id, &ec_ecdt->handle);
	if (ACPI_FAILURE(status)) {
		goto error;
	}

	return 0;
      error:
	ACPI_EXCEPTION((AE_INFO, status, "Could not use ECDT"));
	kfree(ec_ecdt);
	ec_ecdt = NULL;

	return -ENODEV;
}


int acpi_ec_ecdt_probe(void)
{
	acpi_status status;
	int ret;

	ret = acpi_ec_get_real_ecdt();
	#if 0
	/* Try to make a fake ECDT */
	if (ret && acpi_fake_ecdt_enabled) {
		ret = acpi_ec_fake_ecdt();
	}
	#endif

	if (ret)
		return 0;

	/*
	 * Install GPE handler
	 */
	status = acpi_install_gpe_handler(NULL, ec_ecdt->gpe,
					  ACPI_GPE_EDGE_TRIGGERED,
					  &acpi_ec_gpe_handler, ec_ecdt);
	if (ACPI_FAILURE(status)) {
		goto error;
	}
	acpi_set_gpe_type(NULL, ec_ecdt->gpe, ACPI_GPE_TYPE_RUNTIME);
	acpi_enable_gpe(NULL, ec_ecdt->gpe, ACPI_NOT_ISR);

	status = acpi_install_address_space_handler(ACPI_ROOT_OBJECT,
						    ACPI_ADR_SPACE_EC,
						    &acpi_ec_space_handler,
						    &acpi_ec_space_setup,
						    ec_ecdt);
	if (ACPI_FAILURE(status)) {
		acpi_remove_gpe_handler(NULL, ec_ecdt->gpe,
					&acpi_ec_gpe_handler);
		goto error;
	}

	return 0;

      error:
	ACPI_EXCEPTION((AE_INFO, status, "Could not use ECDT"));
	kfree(ec_ecdt);
	ec_ecdt = NULL;

	return -ENODEV;
}

int acpi_ec_init(void)
{
	int result = 0;

	/* Now register the driver for the EC */
	result = acpi_bus_register_driver(&acpi_ec_driver);
	if (result < 0) {
		return -ENODEV;
	}

	return result;
}










