
/*
 *  The AtheOS kernel
 *  Copyright (C) 1999 - 2001 Kurt Skauen
 *  Copyright (C) 2002 Kristian Van Der Vliet
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

#include <posix/ioctl.h>

#include <atheos/kernel.h>
#include <atheos/threads.h>
#include <atheos/image.h>
#include <atheos/semaphore.h>
#include <atheos/pci.h>
#include <atheos/bcache.h>
#include <atheos/socket.h>
#include <net/net.h>
#include <atheos/time.h>
#include <atheos/irq.h>
#include <atheos/device.h>
#include <atheos/udelay.h>
#include <atheos/ctype.h>
#include <atheos/dma.h>
#include <atheos/spinlock.h>
#include <atheos/timer.h>
#include <atheos/msgport.h>
#include <atheos/bootmodules.h>
#include <atheos/tld.h>
#include <atheos/random.h>
#include <atheos/config.h>
#include <atheos/nls.h>
#include <atheos/resource.h>

#include <posix/fcntl.h>
#include <posix/unistd.h>
#include <posix/signal.h>

#include "inc/ksyms.h"
#include "inc/mman.h"
#include "inc/scheduler.h"
#include "inc/array.h"
#include "inc/swap.h"

typedef struct
{
	const char *pzName;
	void *pValue;
} KernelSymbol_s;

static KernelSymbol_s g_asKernelSymbols[];

#define KSYMBOL( name ) { #name, &name }


int ksym_get_symbol_count( void )
{
	int i;

	for ( i = 0; g_asKernelSymbols[i].pzName != NULL; ++i )

      /*** EMPTY ***/ ;
	return ( i );
}

const char *ksym_get_symbol( int nIndex, void **ppValue )
{
	*ppValue = g_asKernelSymbols[nIndex].pValue;
	return ( g_asKernelSymbols[nIndex].pzName );
}

static KernelSymbol_s g_asKernelSymbols[] = {
	// General functions:
	KSYMBOL( get_processor_id ),
	KSYMBOL( get_processor_acpi_id ),
	KSYMBOL( get_active_cpu_count ),
	KSYMBOL( get_cpu_extended_info ),
	KSYMBOL( update_cpu_speed ),
	KSYMBOL( set_cpu_time_handler ),
	KSYMBOL( reboot ),
	KSYMBOL( apm_poweroff ),
	KSYMBOL( get_app_server_port ),
	KSYMBOL( sys_get_app_server_port ),
	KSYMBOL( get_system_path ),
	KSYMBOL( sys_get_system_path ),
	KSYMBOL( get_system_time ),
	KSYMBOL( get_real_time ),
	KSYMBOL( get_idle_time ),
	KSYMBOL( sys_GetTime ),
	KSYMBOL( sys_SetTime ),
	KSYMBOL( set_idle_loop_handler ),
	KSYMBOL( get_system_info ),
	
	// Process functions:
	KSYMBOL( sys_get_process_id ),
	KSYMBOL( sys_wait4 ),
	KSYMBOL( sys_wait_for_thread ),
	KSYMBOL( sys_waitpid ),
	KSYMBOL( setgid ),
	KSYMBOL( setpgid ),
	KSYMBOL( setsid ),
	KSYMBOL( setuid ),
	KSYMBOL( getegid ),
	KSYMBOL( geteuid ),
	KSYMBOL( getgid ),
	KSYMBOL( getuid ),
	KSYMBOL( getfsgid ),
	KSYMBOL( getfsuid ),
	KSYMBOL( getgroups ),
	KSYMBOL( getpgid ),
	KSYMBOL( getpgrp ),
	KSYMBOL( getppid ),
	KSYMBOL( getsid ),
	KSYMBOL( sys_kill ),
	KSYMBOL( sys_kill_proc ),
	KSYMBOL( sys_killpg ),
	KSYMBOL( sys_exit ),
	
	// Thread functions:
	KSYMBOL( spawn_kernel_thread ),
	KSYMBOL( start_thread ),
	KSYMBOL( stop_thread ),
	KSYMBOL( sys_suspend_thread ),
	KSYMBOL( sys_rename_thread ),
	KSYMBOL( sys_resume_thread ),
	KSYMBOL( sys_get_thread_id ),
	KSYMBOL( sys_get_thread_info ),
	KSYMBOL( sys_get_thread_proc ),
	KSYMBOL( suspend ),
	KSYMBOL( snooze ),
	KSYMBOL( wakeup_thread ),
	KSYMBOL( set_thread_priority ),
	KSYMBOL( set_thread_target_cpu ),
			
	// Scheduler functions:
	KSYMBOL( Schedule ),
//	KSYMBOL( __sched_lock ),
//	KSYMBOL( sched_unlock ),
	KSYMBOL( sleep_on_queue ),
	KSYMBOL( wake_up_sleepers ),

	// Signal functions:
	KSYMBOL( is_signal_pending ),
	KSYMBOL( get_signal_mode ),
	KSYMBOL( sys_sigaction ),
	
	// Image functions:
	KSYMBOL( load_kernel_driver ),
	KSYMBOL( unload_kernel_driver ),
	KSYMBOL( open_image_file ),
	KSYMBOL( get_symbol_by_address ),
	KSYMBOL( get_image_info ),	
	KSYMBOL( get_symbol_address ),
	
	// Debug functions:
	KSYMBOL( print_registers ),	
	KSYMBOL( trace_stack ),	
	KSYMBOL( register_debug_cmd ),
	KSYMBOL( sys_debug_read ),
	KSYMBOL( sys_debug_write ),
	KSYMBOL( panic ),
	KSYMBOL( printk ),
	KSYMBOL( dbprintf ),
	
	// I/O functions:
	KSYMBOL( isa_readb ),
	KSYMBOL( isa_readl ),
	KSYMBOL( isa_readw ),
	KSYMBOL( isa_writeb ),
	KSYMBOL( isa_writel ),
	KSYMBOL( isa_writew ),

	// IRQ functions:
	KSYMBOL( cli ),
	KSYMBOL( sti ),
	KSYMBOL( get_cpu_flags ),
	KSYMBOL( put_cpu_flags ),
	KSYMBOL( realint ),
	KSYMBOL( request_irq ),
	KSYMBOL( release_irq ),
	KSYMBOL( enable_irq ),
	KSYMBOL( disable_irq_nosync ),

	// DMA functions:
	KSYMBOL( request_dma ),
	KSYMBOL( free_dma ),
	KSYMBOL( enable_dma_channel ),
	KSYMBOL( disable_dma_channel ),
	KSYMBOL( clear_dma_ff ),
	KSYMBOL( set_dma_mode ),
	KSYMBOL( set_dma_page ),
	KSYMBOL( set_dma_address ),
	KSYMBOL( set_dma_count ),

	// Timer functions:
	KSYMBOL( create_timer ),
	KSYMBOL( start_timer ),
	KSYMBOL( delete_timer ),

	// Semaphore functions:

	KSYMBOL( create_semaphore ),
	KSYMBOL( delete_semaphore ),
	KSYMBOL( lock_semaphore_ex ),
	KSYMBOL( lock_mutex ),
	KSYMBOL( lock_semaphore ),
	KSYMBOL( unlock_mutex ),
	KSYMBOL( unlock_semaphore_ex ),
	KSYMBOL( unlock_and_suspend ),
	KSYMBOL( unlock_semaphore ),
	KSYMBOL( spinunlock_and_suspend ),
	KSYMBOL( sleep_on_sem ),
	KSYMBOL( wakeup_sem ),
	KSYMBOL( reset_semaphore ),
	KSYMBOL( get_semaphore_info ),
	KSYMBOL( get_semaphore_count ),
	KSYMBOL( get_semaphore_owner ),
	KSYMBOL( is_semaphore_locked ),
	KSYMBOL( rwl_lock_read ),
	KSYMBOL( rwl_lock_read_ex ),
	KSYMBOL( rwl_lock_write ),
	KSYMBOL( rwl_lock_write_ex ),
	KSYMBOL( rwl_unlock_read ),
	KSYMBOL( rwl_unlock_write ),
	KSYMBOL( rwl_count_readers ),
	KSYMBOL( rwl_count_all_readers ),
	KSYMBOL( rwl_count_writers ),
	KSYMBOL( rwl_count_all_writers ),
	
	// TLD functions
	KSYMBOL( alloc_tld ),
	KSYMBOL( free_tld ),
	KSYMBOL( get_tld_addr ),
	
	// Memory functions:
	KSYMBOL( kmalloc ),
	KSYMBOL( __kfree ),
	KSYMBOL( alloc_real ),
	KSYMBOL( free_real ),
	KSYMBOL( get_free_page ),
	KSYMBOL( get_free_pages ),
	KSYMBOL( free_pages ),
	KSYMBOL( flush_tlb ),
	KSYMBOL( flush_tlb_global ),
	KSYMBOL( flush_tlb_page ),
	KSYMBOL( get_page_desc ),
	KSYMBOL( get_swap_info ),
	KSYMBOL( protect_dos_mem ),
	KSYMBOL( unprotect_dos_mem ),
	KSYMBOL( dup_swap_page ),
	KSYMBOL( free_swap_page ),
	KSYMBOL( set_page_directory_base_reg ),

	// Memory-area functions:
	KSYMBOL( alloc_area_list ),
	KSYMBOL( create_area ),
	KSYMBOL( delete_area ),
	KSYMBOL( resize_area ),
	KSYMBOL( remap_area ),
	KSYMBOL( get_area_info ),
	KSYMBOL( get_area_physical_address ),
	KSYMBOL( memcpy_to_user ),
	KSYMBOL( memcpy_from_user ),
	KSYMBOL( strncpy_from_user ),
	KSYMBOL( strcpy_to_user ),
	KSYMBOL( get_next_area ),

	// Inode functions:
	KSYMBOL( get_vnode ),
	KSYMBOL( put_vnode ),
	KSYMBOL( flush_inode ),
	KSYMBOL( set_inode_deleted_flag ),
	KSYMBOL( get_inode_deleted_flag ),

	// VFS function

	KSYMBOL( open ),
	KSYMBOL( close ),
	KSYMBOL( based_open ),

	KSYMBOL( write ),
	KSYMBOL( write_pos ),
	KSYMBOL( writev ),
	KSYMBOL( writev_pos ),
	KSYMBOL( read ),
	KSYMBOL( read_pos ),
	KSYMBOL( readv ),
	KSYMBOL( readv_pos ),

	KSYMBOL( readlink ),
	KSYMBOL( freadlink ),

	KSYMBOL( stat ),
	KSYMBOL( fstat ),
	KSYMBOL( lseek ),
	KSYMBOL( chdir ),
	KSYMBOL( fchdir ),
	KSYMBOL( ioctl ),
	KSYMBOL( mkdir ),
	KSYMBOL( based_mkdir ),
	KSYMBOL( rmdir ),
	KSYMBOL( based_rmdir ),
	KSYMBOL( unlink ),
	KSYMBOL( based_unlink ),
	KSYMBOL( sys_mount ),
	KSYMBOL( sys_initialize_fs ),
	KSYMBOL( symlink ),
	KSYMBOL( based_symlink ),
	KSYMBOL( sys_unmount ),

	KSYMBOL( iterate_directory ),
	KSYMBOL( get_directory_path ),

	KSYMBOL( notify_node_monitors ),
	
	// Device node functions:
	KSYMBOL( create_device_node ),
	KSYMBOL( delete_device_node ),
	KSYMBOL( rename_device_node ),
	
	// Block cache functions:
	KSYMBOL( get_empty_block ),
	KSYMBOL( get_cache_block ),
	KSYMBOL( mark_blocks_dirty ),
	KSYMBOL( set_blocks_info ),
	KSYMBOL( write_logged_blocks ),
	KSYMBOL( release_cache_block ),
	KSYMBOL( read_phys_blocks ),
	KSYMBOL( write_phys_blocks ),
	KSYMBOL( cached_read ),
	KSYMBOL( cached_write ),
	KSYMBOL( flush_device_cache ),
	KSYMBOL( flush_locked_device_cache ),
	KSYMBOL( flush_cache_block ),
	KSYMBOL( setup_device_cache ),
	KSYMBOL( shutdown_device_cache ),

	// Partition table functions:
	KSYMBOL( decode_disk_partitions ),


	/**
	 * Network functions:
	 */
	// Packet buffers
	KSYMBOL( alloc_pkt_buffer ),
	KSYMBOL( reserve_pkt_header ),
	KSYMBOL( free_pkt_buffer ),


	// Packet queues
	KSYMBOL( init_net_queue ),
	KSYMBOL( delete_net_queue ),
	KSYMBOL( enqueue_packet ),
	KSYMBOL( remove_head_packet ),
	KSYMBOL( setsockopt ),
	KSYMBOL( getsockopt ),


	// Sockets
	KSYMBOL( socket ),
	KSYMBOL( closesocket ),
	KSYMBOL( shutdown ),
	KSYMBOL( getpeername ),
	KSYMBOL( getsockname ),
	KSYMBOL( connect ),
	KSYMBOL( bind ),
	KSYMBOL( listen ),
	KSYMBOL( accept ),
	KSYMBOL( recvmsg ),
	KSYMBOL( sendmsg ),
	KSYMBOL( recvfrom ),
	KSYMBOL( sendto ),


	// Network entry points
	KSYMBOL( send_packet ),
	KSYMBOL( dispatch_net_packet ),
	KSYMBOL( ip_send ),
	KSYMBOL( ip_send_via ),
	KSYMBOL( ip_in ),


	// Network interface management
	KSYMBOL( add_net_interface ),
	KSYMBOL( del_net_interface ),
	KSYMBOL( acquire_net_interface ),
	KSYMBOL( get_net_interface ),
	KSYMBOL( find_interface ),
	KSYMBOL( find_interface_by_addr ),
	KSYMBOL( release_net_interface ),

	KSYMBOL( get_interface_address ),
	KSYMBOL( get_interface_broadcast ),
	KSYMBOL( get_interface_destination ),
	KSYMBOL( get_interface_netmask ),
	KSYMBOL( get_interface_mtu ),
	KSYMBOL( get_interface_flags ),

	KSYMBOL( set_interface_address ),
	KSYMBOL( set_interface_broadcast ),
	KSYMBOL( set_interface_destination ),
	KSYMBOL( set_interface_netmask ),
	KSYMBOL( set_interface_mtu ),
	KSYMBOL( set_interface_flags ),


	// Routing table functions
	KSYMBOL( add_route ),
	KSYMBOL( del_route ),
	KSYMBOL( ip_acquire_route ),
	KSYMBOL( ip_find_route ),
	KSYMBOL( ip_find_device_route ),
	KSYMBOL( ip_find_static_route ),
	KSYMBOL( ip_release_route ),
	
	// Misc network functions:
	KSYMBOL( format_ipaddress ),
	KSYMBOL( parse_ipaddress ),

	// Bootmodule functions:
	KSYMBOL( find_module_by_address ),
	KSYMBOL( get_boot_module_count ),
	KSYMBOL( get_boot_module ),
	KSYMBOL( put_boot_module ),
	KSYMBOL( get_kernel_arguments ),
	KSYMBOL( get_str_arg ),
	KSYMBOL( get_num_arg ),
	KSYMBOL( get_bool_arg ),

	// LIBC functions:
	KSYMBOL( __const_udelay ),
	KSYMBOL( __udelay ),
	KSYMBOL( __ctype_flags ),
	KSYMBOL( __ctype_tolower ),
	KSYMBOL( __ctype_toupper ),
	KSYMBOL( atol ),
	KSYMBOL( bcopy ),
	KSYMBOL( memcmp ),
	KSYMBOL( qsort ),
	KSYMBOL( sprintf ),
	KSYMBOL( stricmp ),
	KSYMBOL( strlcat ),
	KSYMBOL( strlcpy ),
	KSYMBOL( strnicmp ),
	KSYMBOL( strpbrk ),
	KSYMBOL( strspn ),
	KSYMBOL( strstr ),
	KSYMBOL( strtok ),
	KSYMBOL( strtol ),
	KSYMBOL( strtoul ),
	KSYMBOL( vsprintf ),
	
	// Array functions:
	KSYMBOL( MArray_Destroy ),
	KSYMBOL( MArray_GetNextIndex ),
	KSYMBOL( MArray_GetObj ),
	KSYMBOL( MArray_GetPrevIndex ),
	KSYMBOL( MArray_Init ),
	KSYMBOL( MArray_Insert ),
	KSYMBOL( MArray_Remove ),
	
	// Public message port functions:
	KSYMBOL( make_port_public ),
	KSYMBOL( make_port_private ),
	KSYMBOL( find_port ),
	KSYMBOL( get_msg_x ),
	KSYMBOL( send_msg_x ),
	KSYMBOL( get_port_from_handle ),
	KSYMBOL( sys_get_msg ),
	KSYMBOL( sys_delete_port ),
	KSYMBOL( sys_send_msg ),

	// Randomness
	KSYMBOL( seed ),
	KSYMBOL( rand ),

	// Configuration file access 
	KSYMBOL( write_kernel_config_entry_header ),
	KSYMBOL( write_kernel_config_entry_data ),
	KSYMBOL( read_kernel_config_entry ),

	// Devices & Busmanagers

	KSYMBOL( register_device ),
	KSYMBOL( unregister_device ),
	KSYMBOL( claim_device ),
	KSYMBOL( release_device ),
	KSYMBOL( set_device_data ),
	KSYMBOL( get_device_data ),
	KSYMBOL( get_device_info ),
	KSYMBOL( register_busmanager ),
	KSYMBOL( get_busmanager ),
	KSYMBOL( disable_device ),
	KSYMBOL( disable_device_on_bus ),
	KSYMBOL( enable_devices_on_bus ),
	KSYMBOL( enable_all_devices ),
	KSYMBOL( suspend_devices ),
	KSYMBOL( resume_devices ),
	
	// Resource manager
	KSYMBOL( ioport_root_resource ),
	KSYMBOL( memory_root_resource ),
	KSYMBOL( request_resource ),
	KSYMBOL( ____request_resource ),
	KSYMBOL( release_resource ),
	KSYMBOL( allocate_resource ),
	KSYMBOL( insert_resource ),
	KSYMBOL( adjust_resource ),
	KSYMBOL( __request_region ),
	KSYMBOL( __release_region ),

	// NLS
	KSYMBOL( nls_conv_cp_to_utf8 ),
	KSYMBOL( nls_conv_utf8_to_cp ),

	{NULL, NULL}
};
