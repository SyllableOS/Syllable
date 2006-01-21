/*
 *  acpi_osl.c - OS-dependent functions ($Revision$)
 *
 *  Copyright (C) 2000       Andrew Henroid
 *  Copyright (C) 2001, 2002 Andy Grover <andrew.grover@intel.com>
 *  Copyright (C) 2001, 2002 Paul Diefenbaugh <paul.s.diefenbaugh@intel.com>
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 */

#include <atheos/kernel.h>
#include <atheos/semaphore.h>
#include <atheos/kdebug.h>
#include <atheos/irq.h>
#include <atheos/isa_io.h>
#include <atheos/device.h>
#include <atheos/pci.h>
#include <atheos/threads.h>
#include <atheos/udelay.h>
#include <atheos/spinlock.h>
#include <atheos/list.h>
#include <atheos/ctype.h>
#include <atheos/acpi.h>
#include <posix/signal.h>


#define _COMPONENT		ACPI_OS_SERVICES
ACPI_MODULE_NAME	("osl")

#define PREFIX		"ACPI: "

struct acpi_os_dpc
{
    acpi_osd_exec_callback  function;
    void		    *context;
};
static int acpi_irq_irq = 0;
static acpi_osd_handler acpi_irq_handler = NULL;
static void *acpi_irq_context = NULL;

acpi_status
acpi_os_initialize(void)
{
	return AE_OK;
}

acpi_status
acpi_os_initialize1(void)
{
	return AE_OK;
}

acpi_status
acpi_os_terminate(void)
{
	if (acpi_irq_handler) {
		acpi_os_remove_interrupt_handler(acpi_irq_irq,
						 acpi_irq_handler);
	}

	return AE_OK;
}

void
acpi_os_printf(const char *fmt,...)
{
	va_list args;
	va_start(args, fmt);
	acpi_os_vprintf(fmt, args);
	va_end(args);
}
void
acpi_os_vprintf(const char *fmt, va_list args)
{
	static char buffer[512];
	
	vsprintf(buffer, fmt, args);

	printk("%s", buffer);
}

void *
acpi_os_allocate(acpi_size size)
{
	return kmalloc(size, MEMF_KERNEL);
}

void
acpi_os_free(void *ptr)
{
	kfree(ptr);
}


acpi_status
acpi_os_get_root_pointer(u32 flags, struct acpi_pointer *addr)
{
	if (ACPI_FAILURE(acpi_find_root_pointer(flags, addr))) {
		printk("System description tables not found\n");
		return AE_NOT_FOUND;
	}

	return AE_OK;
}

acpi_status
acpi_os_map_memory(acpi_physical_address phys, acpi_size size, void __iomem **virt)
{
	area_id area;
	unsigned long base = (unsigned long)phys;
	void *addr = NULL;
	unsigned long length = size;

	area = create_area( "acpi_memory", &addr, PAGE_ALIGN( length ) + PAGE_SIZE, PAGE_ALIGN( length ) + PAGE_SIZE,
												AREA_ANY_ADDRESS|AREA_KERNEL, AREA_FULL_LOCK );
	if( area < 0 )
	{
		printk( "acpi_os_map_memory() could not create area with size 0x%x\n", (uint)size );
		return AE_ERROR;
	}
	
	if( remap_area( area, (void*)( base & PAGE_MASK ) ) < 0 )
	{
		printk( "acpi_os_map_memory() could not remap area to 0x%x\n", (uint)phys );
		return AE_ERROR;
	}
	
	//printk( "acpi_os_map_memory() remapped 0x%x to 0x%x size 0x%x\n", (uint)base, (uint)addr, (uint)length );

	base = (unsigned long)addr + ( base - ( base & PAGE_MASK ) );

	*virt = (void*)(base);

	return AE_OK;
}

void
acpi_os_unmap_memory(void __iomem *virt, acpi_size size)
{
	//printk( "acpi_os_unmap_memory()\n" );
	//iounmap(virt);
}

#ifdef ACPI_FUTURE_USAGE
acpi_status
acpi_os_get_physical_address(void *virt, acpi_physical_address *phys)
{
	if(!phys || !virt)
		return AE_BAD_PARAMETER;

	*phys = virt_to_phys(virt);

	return AE_OK;
}
#endif

#define ACPI_MAX_OVERRIDE_LEN 100

static char acpi_os_name[ACPI_MAX_OVERRIDE_LEN];

acpi_status
acpi_os_predefined_override (const struct acpi_predefined_names *init_val,
		             acpi_string *new_val)
{
	if (!init_val || !new_val)
		return AE_BAD_PARAMETER;

	*new_val = NULL;
	if (!memcmp (init_val->name, "_OS_", 4) && strlen(acpi_os_name)) {
		printk("Overriding _OS definition to '%s'\n",
			acpi_os_name);
		*new_val = acpi_os_name;
	}

	return AE_OK;
}

acpi_status
acpi_os_table_override (struct acpi_table_header *existing_table,
			struct acpi_table_header **new_table)
{
	if (!existing_table || !new_table)
		return AE_BAD_PARAMETER;
		
	*new_table = NULL;
	return AE_OK;
}

static int
acpi_irq(int irq, void *dev_id, SysCallRegs_s *regs)
{
	return (*acpi_irq_handler)(acpi_irq_context) ? 0 : 0;
}

acpi_status
acpi_os_install_interrupt_handler(u32 gsi, acpi_osd_handler handler, void *context)
{
	unsigned int irq;

	irq = acpi_fadt.sci_int;
	acpi_irq_handler = handler;
	acpi_irq_context = context;
	if (( acpi_irq_irq = request_irq(irq, acpi_irq, NULL, SA_SHIRQ, "acpi", acpi_irq)) < 0) {
		printk("SCI (IRQ%d) allocation failed\n", irq);
		return AE_NOT_ACQUIRED;
	}
	
	return AE_OK;
}

acpi_status
acpi_os_remove_interrupt_handler(u32 irq, acpi_osd_handler handler)
{
	if (irq) {
		release_irq(irq, acpi_irq_irq);
		acpi_irq_handler = NULL;
		acpi_irq_irq = 0;
	}
	
	return AE_OK;
}

/*
 * Running in interpreter thread context, safe to sleep
 */

void
acpi_os_sleep(acpi_integer ms)
{
	//current->state = TASK_INTERRUPTIBLE;
	//schedule_timeout(((signed long) ms * HZ) / 1000);
	snooze( ( ms ) * 1000 );
}

void
acpi_os_stall(u32 us)
{
	while (us) {
		u32 delay = 1000;

		if (delay > us)
			delay = us;
		udelay(delay);
		us -= delay;
	}
}

/*
 * Support ACPI 3.0 AML Timer operand
 * Returns 64-bit free-running, monotonically increasing timer
 * with 100ns granularity
 */
u64
acpi_os_get_timer (void)
{
	static u64 t;

	if (!t)
		printk("acpi_os_get_timer() TBD\n");

	return ++t;
}

acpi_status
acpi_os_read_port(
	acpi_io_address	port,
	u32		*value,
	u32		width)
{
	u32 dummy;

	if (!value)
		value = &dummy;

	switch (width)
	{
	case 8:
		*(u8*)  value = inb(port);
		break;
	case 16:
		*(u16*) value = inw(port);
		break;
	case 32:
		*(u32*) value = inl(port);
		break;
	default:
		break;
	}

	return AE_OK;
}

acpi_status
acpi_os_write_port(
	acpi_io_address	port,
	u32		value,
	u32		width)
{
	switch (width)
	{
	case 8:
		outb(value, port);
		break;
	case 16:
		outw(value, port);
		break;
	case 32:
		outl(value, port);
		break;
	default:
		break;
	}

	return AE_OK;
}
EXPORT_SYMBOL(acpi_os_write_port);

acpi_status
acpi_os_read_memory(
	acpi_physical_address	phys_addr,
	u32			*value,
	u32			width)
{
	u32			dummy;
	void __iomem		*virt_addr;
	
	virt_addr = (void*)((unsigned long)phys_addr);
	if (!value)
		value = &dummy;

	switch (width) {
	case 8:
		*(u8*) value = readb(virt_addr);
		break;
	case 16:
		*(u16*) value = readw(virt_addr);
		break;
	case 32:
		*(u32*) value = readl(virt_addr);
		break;
	default:
		break;
	}

	
	return AE_OK;
}

acpi_status
acpi_os_write_memory(
	acpi_physical_address	phys_addr,
	u32			value,
	u32			width)
{
	void __iomem		*virt_addr;
	

	virt_addr = (void*)((unsigned long)phys_addr);
	switch (width) {
	case 8:
		writeb(value, virt_addr);
		break;
	case 16:
		writew(value, virt_addr);
		break;
	case 32:
		writel(value, virt_addr);
		break;
	default:
		break;
	}

	
	return AE_OK;
}

uint32 simple_pci_read_config( int nBusNum, int nDevNum, int nFncNum, int nOffset, int nSize );
status_t simple_pci_write_config( int nBusNum, int nDevNum, int nFncNum, int nOffset, int nSize, uint32 nValue );

acpi_status
acpi_os_read_pci_configuration (struct acpi_pci_id *pci_id, u32 reg, void *value, u32 width)
{
	int size;
	uint32 result;

	if (!value)
		return AE_BAD_PARAMETER;

	switch (width) {
	case 8:
		size = 1;
		break;
	case 16:
		size = 2;
		break;
	case 32:
		size = 4;
		break;
	default:
		return AE_ERROR;
	}
	
	result = simple_pci_read_config( pci_id->bus, pci_id->device, pci_id->function, reg, size );

	memcpy( value, &result, size );
	
	return ( AE_OK);
}

acpi_status
acpi_os_write_pci_configuration (struct acpi_pci_id *pci_id, u32 reg, acpi_integer value, u32 width)
{
	int result, size;

	switch (width) {
	case 8:
		size = 1;
		break;
	case 16:
		size = 2;
		break;
	case 32:
		size = 4;
		break;
	default:
		return AE_ERROR;
	}
	
	result = simple_pci_write_config( pci_id->bus, pci_id->device, pci_id->function, reg, size, value );

	return (result ? AE_ERROR : AE_OK);
}

/* TODO: Change code to take advantage of driver model more */
void
acpi_os_derive_pci_id_2 (
	acpi_handle		rhandle,        /* upper bound  */
	acpi_handle		chandle,        /* current node */
	struct acpi_pci_id	**id,
	int			*is_bridge,
	u8			*bus_number)
{
	acpi_handle		handle;
	struct acpi_pci_id	*pci_id = *id;
	acpi_status		status;
	unsigned long		temp;
	acpi_object_type	type;
	u8			tu8;

	acpi_get_parent(chandle, &handle);
	if (handle != rhandle) {
		acpi_os_derive_pci_id_2(rhandle, handle, &pci_id, is_bridge, bus_number);

		status = acpi_get_type(handle, &type);
		if ( (ACPI_FAILURE(status)) || (type != ACPI_TYPE_DEVICE) )
			return;

		status = acpi_evaluate_integer(handle, METHOD_NAME__ADR, NULL, &temp);
		if (ACPI_SUCCESS(status)) {
			pci_id->device  = ACPI_HIWORD (ACPI_LODWORD (temp));
			pci_id->function = ACPI_LOWORD (ACPI_LODWORD (temp));

			if (*is_bridge)
				pci_id->bus = *bus_number;

			/* any nicer way to get bus number of bridge ? */
			status = acpi_os_read_pci_configuration(pci_id, 0x0e, &tu8, 8);
			if (ACPI_SUCCESS(status) &&
			    ((tu8 & 0x7f) == 1 || (tu8 & 0x7f) == 2)) {
				status = acpi_os_read_pci_configuration(pci_id, 0x18, &tu8, 8);
				if (!ACPI_SUCCESS(status)) {
					/* Certainly broken...  FIX ME */
					return;
				}
				*is_bridge = 1;
				pci_id->bus = tu8;
				status = acpi_os_read_pci_configuration(pci_id, 0x19, &tu8, 8);
				if (ACPI_SUCCESS(status)) {
					*bus_number = tu8;
				}
			} else
				*is_bridge = 0;
		}
	}
}

void
acpi_os_derive_pci_id (
	acpi_handle		rhandle,        /* upper bound  */
	acpi_handle		chandle,        /* current node */
	struct acpi_pci_id	**id)
{
	int is_bridge = 1;
	u8 bus_number = (*id)->bus;

	acpi_os_derive_pci_id_2(rhandle, chandle, id, &is_bridge, &bus_number);
}



static int32
acpi_os_execute_deferred (
	void *context)
{
	struct acpi_os_dpc	*dpc = NULL;

	ACPI_FUNCTION_TRACE ("os_execute_deferred");

	dpc = (struct acpi_os_dpc *) context;
	if (!dpc) {
		ACPI_DEBUG_PRINT ((ACPI_DB_ERROR, "Invalid (NULL) context.\n"));
		return 0;
	}

	dpc->function(dpc->context);

	kfree(dpc);

	return 0;
}

acpi_status
acpi_os_queue_for_execution(
	u32			priority,
	acpi_osd_exec_callback	function,
	void			*context)
{
	acpi_status 		status = AE_OK;
	struct acpi_os_dpc	*dpc;
	thread_id id;

	ACPI_FUNCTION_TRACE ("os_queue_for_execution");

	ACPI_DEBUG_PRINT ((ACPI_DB_EXEC, "Scheduling function [%p(%p)] for deferred execution.\n", function, context));

	if (!function)
		return_ACPI_STATUS (AE_BAD_PARAMETER);

	/*
	 * Allocate/initialize DPC structure.  Note that this memory will be
	 * freed by the callee.  The kernel handles the tq_struct list  in a
	 * way that allows us to also free its memory inside the callee.
	 * Because we may want to schedule several tasks with different
	 * parameters we can't use the approach some kernel code uses of
	 * having a static tq_struct.
	 * We can save time and code by allocating the DPC and tq_structs
	 * from the same memory.
	 */

	dpc = kmalloc(sizeof(struct acpi_os_dpc), MEMF_KERNEL | MEMF_CLEAR );
	if (!dpc)
		return_ACPI_STATUS (AE_NO_MEMORY);

	dpc->function = function;
	dpc->context = context;
	
	id = spawn_kernel_thread( "acpi_task", acpi_os_execute_deferred,
				     0, 4096, dpc );
	wakeup_thread( id, false );

	return_ACPI_STATUS ( status );
}

void
acpi_os_wait_events_complete(
	void *context)
{
	printk( "acpi_os_wait_events_complete()\n" );
}

/*
 * Allocate the memory for a spinlock and initialize it.
 */
acpi_status
acpi_os_create_lock (
	acpi_handle	*out_handle)
{
	SpinLock_s *lock_ptr;

	ACPI_FUNCTION_TRACE ("os_create_lock");

	lock_ptr = acpi_os_allocate(sizeof(SpinLock_s));

	spinlock_init(lock_ptr, "acpi_lock");

	ACPI_DEBUG_PRINT ((ACPI_DB_MUTEX, "Creating spinlock[%p].\n", lock_ptr));

	*out_handle = lock_ptr;

	return_ACPI_STATUS (AE_OK);
}


/*
 * Deallocate the memory for a spinlock.
 */
void
acpi_os_delete_lock (
	acpi_handle	handle)
{
	ACPI_FUNCTION_TRACE ("os_create_lock");

	ACPI_DEBUG_PRINT ((ACPI_DB_MUTEX, "Deleting spinlock[%p].\n", handle));

	acpi_os_free(handle);

	return_VOID;
}

/*
 * Acquire a spinlock.
 *
 * handle is a pointer to the spinlock_t.
 * flags is *not* the result of save_flags - it is an ACPI-specific flag variable
 *   that indicates whether we are at interrupt level.
 */
unsigned long
acpi_os_acquire_lock( acpi_handle handle )
{
	unsigned long flags;
	flags = spinlock( (SpinLock_s *)handle );
	return( flags );
}


/*
 * Release a spinlock. See above.
 */
void
acpi_os_release_lock( acpi_handle handle, unsigned long flags )
{
	spinunlock_restore((SpinLock_s *)handle, flags);
}


acpi_status
acpi_os_create_semaphore(
	u32		max_units,
	u32		initial_units,
	acpi_handle	*handle)
{
	sem_id	*sem = NULL;

	ACPI_FUNCTION_TRACE ("os_create_semaphore");

	sem = acpi_os_allocate(sizeof(sem_id));
	if (!sem)
		return_ACPI_STATUS (AE_NO_MEMORY);
	memset(sem, 0, sizeof(sem_id));

	*sem = create_semaphore( "acpi_semaphore", initial_units, 0 );

	*handle = (acpi_handle*)sem;

	ACPI_DEBUG_PRINT ((ACPI_DB_MUTEX, "Creating semaphore[%p|%d].\n", *handle, initial_units));

	return_ACPI_STATUS (AE_OK);
}


/*
 * TODO: A better way to delete semaphores?  Linux doesn't have a
 * 'delete_semaphore()' function -- may result in an invalid
 * pointer dereference for non-synchronized consumers.	Should
 * we at least check for blocked threads and signal/cancel them?
 */

acpi_status
acpi_os_delete_semaphore(
	acpi_handle	handle)
{
	sem_id *sem = (sem_id*) handle;

	ACPI_FUNCTION_TRACE ("os_delete_semaphore");

	if (!sem)
		return_ACPI_STATUS (AE_BAD_PARAMETER);
		
	delete_semaphore( *sem );

	ACPI_DEBUG_PRINT ((ACPI_DB_MUTEX, "Deleting semaphore[%p].\n", handle));

	acpi_os_free(sem); sem =  NULL;

	return_ACPI_STATUS (AE_OK);
}

/*
 * TODO: The kernel doesn't have a 'down_timeout' function -- had to
 * improvise.  The process is to sleep for one scheduler quantum
 * until the semaphore becomes available.  Downside is that this
 * may result in starvation for timeout-based waits when there's
 * lots of semaphore activity.
 *
 * TODO: Support for units > 1?
 */
acpi_status
acpi_os_wait_semaphore(
	acpi_handle		handle,
	u32			units,
	u16			timeout)
{
	acpi_status		status = AE_OK;
	sem_id	*sem = (sem_id*)handle;
	int			ret = 0;

	ACPI_FUNCTION_TRACE ("os_wait_semaphore");

	if (!sem || (units < 1))
		return_ACPI_STATUS (AE_BAD_PARAMETER);

	if (units > 1)
		return_ACPI_STATUS (AE_SUPPORT);

	ACPI_DEBUG_PRINT ((ACPI_DB_MUTEX, "Waiting for semaphore[%p|%d|%d]\n", handle, units, timeout));

/*	if (in_atomic())
		timeout = 0;
*/
	switch (timeout)
	{
		/*
		 * No Wait:
		 * --------
		 * A zero timeout value indicates that we shouldn't wait - just
		 * acquire the semaphore if available otherwise return AE_TIME
		 * (a.k.a. 'would block').
		 */
		case 0:
		if(TRY_LOCK(*sem))
			status = AE_TIME;
		break;

		/*
		 * Wait Indefinitely:
		 * ------------------
		 */
		case ACPI_WAIT_FOREVER:
		LOCK(*sem);
		break;

		/*
		 * Wait w/ Timeout:
		 * ----------------
		 */
		default:
		// TODO: A better timeout algorithm?
		{
			status = lock_semaphore( *sem, SEM_NOSIG, timeout * 1000 );
	
			if (ret != 0)
				status = AE_TIME;
		}
		break;
	}

	if (ACPI_FAILURE(status)) {
		ACPI_DEBUG_PRINT ((ACPI_DB_ERROR, "Failed to acquire semaphore[%p|%d|%d], %s\n", 
			handle, units, timeout, acpi_format_exception(status)));
	}
	else {
		ACPI_DEBUG_PRINT ((ACPI_DB_MUTEX, "Acquired semaphore[%p|%d|%d]\n", handle, units, timeout));
	}

	return_ACPI_STATUS (status);
}

/*
 * TODO: Support for units > 1?
 */
acpi_status
acpi_os_signal_semaphore(
    acpi_handle 	    handle,
    u32 		    units)
{
	sem_id *sem = (sem_id *) handle;

	ACPI_FUNCTION_TRACE ("os_signal_semaphore");

	if (!sem || (units < 1))
		return_ACPI_STATUS (AE_BAD_PARAMETER);

	if (units > 1)
		return_ACPI_STATUS (AE_SUPPORT);

	ACPI_DEBUG_PRINT ((ACPI_DB_MUTEX, "Signaling semaphore[%p|%d]\n", handle, units));

	UNLOCK( *sem );

	return_ACPI_STATUS (AE_OK);
}
EXPORT_SYMBOL(acpi_os_signal_semaphore);

#ifdef ACPI_FUTURE_USAGE
u32
acpi_os_get_line(char *buffer)
{

#ifdef ENABLE_DEBUGGER
	if (acpi_in_debugger) {
		u32 chars;

		kdb_read(buffer, sizeof(line_buf));

		/* remove the CR kdb includes */
		chars = strlen(buffer) - 1;
		buffer[chars] = '\0';
	}
#endif

	return 0;
}
#endif  /*  ACPI_FUTURE_USAGE  */

/* Assumes no unreadable holes inbetween */
u8
acpi_os_readable(void *ptr, acpi_size len)
{
	return 1;
}

#ifdef ACPI_FUTURE_USAGE
u8
acpi_os_writable(void *ptr, acpi_size len)
{
	/* could do dummy write (racy) or a kernel page table lookup.
	   The later may be difficult at early boot when kmap doesn't work yet. */
	return 1;
}
#endif

u32
acpi_os_get_thread_id (void)
{
	return( sys_get_thread_id( NULL ) );

	return 0;
}

acpi_status
acpi_os_signal (
    u32		function,
    void	*info)
{
	switch (function)
	{
	case ACPI_SIGNAL_FATAL:
		printk("Fatal opcode executed\n");
		break;
	case ACPI_SIGNAL_BREAKPOINT:
		/*
		 * AML Breakpoint
		 * ACPI spec. says to treat it as a NOP unless
		 * you are debugging.  So if/when we integrate
		 * AML debugger into the kernel debugger its
		 * hook will go here.  But until then it is
		 * not useful to print anything on breakpoints.
		 */
		break;
	default:
		break;
	}

	return AE_OK;
}

int
acpi_os_name_setup(char *str)
{
	char *p = acpi_os_name;
	int count = ACPI_MAX_OVERRIDE_LEN-1;

	if (!str || !*str)
		return 0;

	for (; count-- && str && *str; str++) {
		if (isalnum(*str) || *str == ' ' || *str == ':')
			*p++ = *str;
		else if (*str == '\'' || *str == '"')
			continue;
		else
			break;
	}
	*p = 0;

	return 1;
		
}



/*******************************************************************************
 *
 * FUNCTION:    acpi_os_create_cache
 *
 * PARAMETERS:  CacheName       - Ascii name for the cache
 *              ObjectSize      - Size of each cached object
 *              MaxDepth        - Maximum depth of the cache (in objects)
 *              ReturnCache     - Where the new cache object is returned
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Create a cache object
 *
 ******************************************************************************/

acpi_status
acpi_os_create_cache(char *name, u16 size, u16 depth, acpi_cache_t ** cache)
{
	*cache = (acpi_cache_t*)size;
	return AE_OK;
}

/*******************************************************************************
 *
 * FUNCTION:    acpi_os_purge_cache
 *
 * PARAMETERS:  Cache           - Handle to cache object
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Free all objects within the requested cache.
 *
 ******************************************************************************/

acpi_status acpi_os_purge_cache(acpi_cache_t * cache)
{
	return (AE_OK);
}

/*******************************************************************************
 *
 * FUNCTION:    acpi_os_delete_cache
 *
 * PARAMETERS:  Cache           - Handle to cache object
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Free all objects within the requested cache and delete the
 *              cache object.
 *
 ******************************************************************************/

acpi_status acpi_os_delete_cache(acpi_cache_t * cache)
{
	return (AE_OK);
}

/*******************************************************************************
 *
 * FUNCTION:    acpi_os_release_object
 *
 * PARAMETERS:  Cache       - Handle to cache object
 *              Object      - The object to be released
 *
 * RETURN:      None
 *
 * DESCRIPTION: Release an object to the specified cache.  If cache is full,
 *              the object is deleted.
 *
 ******************************************************************************/

acpi_status acpi_os_release_object(acpi_cache_t * cache, void *object)
{
	kfree( object );
	return (AE_OK);
}

/*******************************************************************************
 *
 * FUNCTION:    acpi_os_acquire_object
 *
 * PARAMETERS:  Cache           - Handle to cache object
 *              ReturnObject    - Where the object is returned
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Get an object from the specified cache.  If cache is empty,
 *              the object is allocated.
 *
 ******************************************************************************/

void *acpi_os_acquire_object(acpi_cache_t * cache)
{
	u16 size = (u16)cache;
	void* obj = kmalloc( size, MEMF_KERNEL );
	return( obj );	
}

















