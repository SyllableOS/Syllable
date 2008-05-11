
/*
 *  The Syllable kernel
 *  ACPI busmanager
 *  Copyright (C) 2005 Arno Klenke
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

#include <posix/errno.h>
#include <atheos/device.h>
#include <atheos/kernel.h>
#include <atheos/isa_io.h>
#include <atheos/types.h>
#include <atheos/pci.h>
#include <atheos/kernel.h>
#include <atheos/spinlock.h>
#include <atheos/pci.h>
#include <atheos/bootmodules.h>
#include <atheos/config.h>
#include <atheos/list.h>
#include <atheos/acpi.h>

#include <macros.h>

int acpi_disabled = 0;
int acpi_strict = 0;

acpi_native_uint acpi_gbl_permanent_mmap = 1;

enum acpi_irq_model_id		acpi_irq_model;
acpi_interrupt_flags acpi_sci_flags;

extern void simple_pci_init();
extern int acpi_init();
extern int acpi_scan_init(void);
extern int acpi_wakeup_device_init();
extern int acpi_ec_init(void);
extern int acpi_thermal_init(void);
extern int acpi_fan_init(void);
extern int acpi_power_init(void);
extern int acpi_button_init(void);
extern int acpi_video_init(void);

#define ACPI_MAX_TABLES		128
static struct acpi_table_desc initial_tables[ACPI_MAX_TABLES];


/*
 * acpi_pic_sci_set_trigger()
 * 
 * use ELCR to set PIC-mode trigger type for SCI
 *
 * If a PIC-mode SCI is not recognized or gives spurious IRQ7's
 * it may require Edge Trigger -- use "acpi_sci=edge"
 *
 * Port 0x4d0-4d1 are ECLR1 and ECLR2, the Edge/Level Control Registers
 * for the 8259 PIC.  bit[n] = 1 means irq[n] is Level, otherwise Edge.
 * ECLR1 is IRQ's 0-7 (IRQ 0, 1, 2 must be 0)
 * ECLR2 is IRQ's 8-15 (IRQ 8, 13 must be 0)
 */

void
acpi_pic_sci_set_trigger(unsigned int irq, u16 trigger)
{
	unsigned int mask = 1 << irq;
	unsigned int old, new;

	/* Real old ELCR mask */
	old = inb(0x4d0) | (inb(0x4d1) << 8);

	/*
	 * If we use ACPI to set PCI irq's, then we should clear ELCR
	 * since we will set it correctly as we enable the PCI irq
	 * routing.
	 */
	new = old;

	/*
	 * Update SCI information in the ELCR, it isn't in the PCI
	 * routing tables..
	 */
	switch (trigger) {
	case 1:	/* Edge - clear */
		new &= ~mask;
		break;
	case 3: /* Level - set */
		new |= mask;
		break;
	}

	if (old == new)
		return;

	kerndbg( KERN_DEBUG, "ACPI: Setting ELCR to %04x (from %04x)\n", new, old);
	outb(new, 0x4d0);
	outb(new >> 8, 0x4d1);
}

int 
acpi_boot_init (void)
{
	/*
	 * The default interrupt routing model is PIC (8259).  This gets
	 * overriden if IOAPICs are enumerated (below).
	 */
	acpi_irq_model = ACPI_IRQ_MODEL_PIC;
	
	
	acpi_sci_flags.polarity = 0;
	acpi_sci_flags.trigger = 0;

	
	return 0;
}

void acpi_print_power_states()
{
	int i;
	char zBuffer[255];
	memset( zBuffer, 0, 255 );
	
	for( i = 0; i < ACPI_S_STATE_COUNT; i++ ) {
		u8 type_a, type_b;
		acpi_status status;
		status = acpi_get_sleep_type_data(i, &type_a, &type_b);
		
		if (ACPI_SUCCESS(status)) {
			char zTemp[20];
			sprintf( zTemp, "S%d ", i );
			strcat( zBuffer, zTemp );
		}
	}
	kerndbg( KERN_INFO, "ACPI: %ssupported\n", zBuffer );
}

extern void acpi_gpe_sleep_prepare(int state );

void acpi_poweroff()
{
	ACPI_FLUSH_CPU_CACHE();
	acpi_enable_wakeup_device_prep(ACPI_STATE_S5);
	acpi_gpe_sleep_prepare(ACPI_STATE_S5);
	acpi_enter_sleep_state_prep(ACPI_STATE_S5);
	ACPI_DISABLE_IRQS();
	acpi_enter_sleep_state(ACPI_STATE_S5);
}

struct acpi_table_fadt* acpi_get_fadt()
{
	return( &acpi_gbl_FADT );
}

ACPI_bus_s sBus = {
	acpi_poweroff,
	acpi_evaluate_object,
	acpi_evaluate_integer,
	acpi_extract_package,
	acpi_get_name,
	acpi_get_handle,
	acpi_bus_get_device,
	acpi_bus_get_status,
	acpi_walk_resources,
	acpi_set_current_resources,
	acpi_get_irq_routing_table,
	acpi_bus_register_driver,
	acpi_bus_unregister_driver,
	acpi_install_notify_handler,
	acpi_remove_notify_handler,
	acpi_get_fadt,
	acpi_set_register,
	acpi_get_register
};

void* acpi_bus_get_hooks( int nVersion )
{
	if ( nVersion != ACPI_BUS_VERSION )
		return ( NULL );
	return ( ( void * )&sBus );
}



#include <atheos/udelay.h>

/** 
 * \par Description: Initialize the pci busmanager.
 * \par Note:
 * \par Warning:
 * \param
 * \return
 * \sa
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

status_t device_init( int nDeviceID )
{
	/* Check if the use of the bus is disabled */
	int i;
	int argc;
	const char *const *argv;
	bool bDisableACPI = false;

	get_kernel_arguments( &argc, &argv );

	for ( i = 0; i < argc; ++i )
	{
		if ( get_bool_arg( &bDisableACPI, "disable_acpi=", argv[i], strlen( argv[i] ) ) )
			if ( bDisableACPI )
			{
				printk( "ACPI disabled by user\n" );
				return ( -1 );
			}
	}
	
	if( !bDisableACPI )
	{
		/* Initialize ACPI system */
		enable_devices_on_bus( ACPI_BUS_NAME );
		simple_pci_init();
		acpi_initialize_tables(initial_tables, ACPI_MAX_TABLES, 0);
		if( acpi_boot_init() < 0 )
			return( -ENODEV );
		if( acpi_init() < 0 )
			return( -ENODEV );
		if( acpi_scan_init() < 0 )
			return( -ENODEV );
		acpi_print_power_states();
		acpi_thermal_init();
		acpi_fan_init();
		acpi_power_init();		
		acpi_ec_init();
		acpi_button_init();
		acpi_wakeup_device_init();
		//acpi_video_init();
	}
	
	register_busmanager( nDeviceID, "acpi", acpi_bus_get_hooks );
	
	printk( "ACPI: Busmanager initialized\n" );

	return ( 0 );
}


status_t device_uninit( int nDeviceID )
{
	return( 0 );
}







































