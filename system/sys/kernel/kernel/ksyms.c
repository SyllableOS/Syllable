
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


int ksym_get_symbol_count()
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

int get_processor_id( void )
{
	return ( GET_APIC_ID( apic_read( APIC_ID ) ) );
}

static KernelSymbol_s g_asKernelSymbols[] = {
	KSYMBOL( get_processor_id ),
	KSYMBOL( is_signals_pending ),
	KSYMBOL( MArray_Destroy ),
	KSYMBOL( MArray_GetNextIndex ),
	KSYMBOL( MArray_GetObj ),
	KSYMBOL( MArray_GetPrevIndex ),
	KSYMBOL( MArray_Init ),
	KSYMBOL( MArray_Insert ),
	KSYMBOL( MArray_Remove ),
	KSYMBOL( get_pci_info ),
	KSYMBOL( read_pci_config ),
	KSYMBOL( write_pci_config ),
	KSYMBOL( Schedule ),
	KSYMBOL( set_thread_priority ),
	KSYMBOL( __kfree ),
	KSYMBOL( __sched_lock ),
	KSYMBOL( add_route ),
	KSYMBOL( add_to_sleeplist ),
	KSYMBOL( add_to_waitlist ),
	KSYMBOL( alloc_real ),
	KSYMBOL( atol ),
	KSYMBOL( atomic_add ),
	KSYMBOL( atomic_swap ),
	KSYMBOL( bcopy ),
	KSYMBOL( cli ),
	KSYMBOL( dup_swap_page ),
	KSYMBOL( find_module_by_address ),
	KSYMBOL( flush_tlb ),
	KSYMBOL( flush_tlb_global ),
	KSYMBOL( flush_tlb_page ),
	KSYMBOL( format_ipaddress ),
	KSYMBOL( free_pages ),
	KSYMBOL( free_real ),
	KSYMBOL( free_swap_page ),
	KSYMBOL( get_app_server_port ),
	KSYMBOL( get_cpu_flags ),
	KSYMBOL( get_free_page ),
	KSYMBOL( get_free_pages ),
	KSYMBOL( get_idle_time ),
	KSYMBOL( get_msg_x ),
	KSYMBOL( get_next_area ),
	KSYMBOL( get_page_desc ),
	KSYMBOL( get_port_from_handle ),
	KSYMBOL( get_real_time ),
	KSYMBOL( get_signal_mode ),
	KSYMBOL( get_swap_info ),
	KSYMBOL( get_symbol_by_address ),
	KSYMBOL( get_system_path ),
	KSYMBOL( get_system_time ),
	KSYMBOL( isa_readb ),
	KSYMBOL( isa_readl ),
	KSYMBOL( isa_readw ),
	KSYMBOL( isa_writeb ),
	KSYMBOL( isa_writel ),
	KSYMBOL( isa_writew ),
	KSYMBOL( kmalloc ),
	KSYMBOL( load_fpu_state ),
	KSYMBOL( memcmp ),
	KSYMBOL( panic ),
	KSYMBOL( parse_ipaddress ),
	KSYMBOL( print_registers ),
	KSYMBOL( printk ),
	KSYMBOL( trace_stack ),
	KSYMBOL( protect_dos_mem ),
	KSYMBOL( put_cpu_flags ),
	KSYMBOL( qsort ),
	KSYMBOL( realint ),
	KSYMBOL( register_debug_cmd ),
	KSYMBOL( remove_from_sleeplist ),
	KSYMBOL( remove_from_waitlist ),
	KSYMBOL( request_irq ),
	KSYMBOL( release_irq ),
	KSYMBOL( enable_irq ),
	KSYMBOL( disable_irq_nosync ),
	KSYMBOL( save_fpu_state ),
	KSYMBOL( sched_unlock ),
	KSYMBOL( send_alarm_signals ),
	KSYMBOL( send_msg_x ),
	KSYMBOL( set_page_directory_base_reg ),
	KSYMBOL( sleep_on_queue ),
	KSYMBOL( spawn_kernel_thread ),
	KSYMBOL( sprintf ),
	KSYMBOL( start_thread ),
	KSYMBOL( sti ),
	KSYMBOL( stop_thread ),
	KSYMBOL( stricmp ),
	KSYMBOL( strnicmp ),
	KSYMBOL( strpbrk ),
	KSYMBOL( strspn ),
	KSYMBOL( strstr ),
	KSYMBOL( strtok ),
	KSYMBOL( strtol ),
	KSYMBOL( suspend ),
	KSYMBOL( snooze ),
//  KSYMBOL(get_next_thread),
	KSYMBOL( sys_get_process_id ),
	KSYMBOL( sys_get_thread_id ),
	KSYMBOL( sys_get_thread_info ),
	KSYMBOL( sys_get_thread_proc ),
	KSYMBOL( sys_GetTime ),
	KSYMBOL( sys_SetTime ),
	KSYMBOL( sys_debug_read ),
	KSYMBOL( sys_debug_write ),
	KSYMBOL( sys_delete_port ),
//  KSYMBOL(sys_dup),
//  KSYMBOL(sys_dup2),
	KSYMBOL( sys_exit ),
	KSYMBOL( sys_get_app_server_port ),
	KSYMBOL( get_image_info ),
	KSYMBOL( sys_get_msg ),
//  KSYMBOL(sys_get_next_proc),
//  KSYMBOL(sys_get_prev_proc),
//  KSYMBOL(sys_get_raw_idle_time),
//  KSYMBOL(sys_get_raw_real_time),
//  KSYMBOL(sys_get_raw_system_time),
	KSYMBOL( load_kernel_driver ),
	KSYMBOL( unload_kernel_driver ),
	KSYMBOL( get_symbol_address ),
	KSYMBOL( open_image_file ),
	KSYMBOL( sys_get_system_path ),
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
//  KSYMBOL(sys_pipe),
//  KSYMBOL(sys_raw_get_msg_x),
//  KSYMBOL(sys_raw_send_msg_x),
	KSYMBOL( sys_rename_thread ),
	KSYMBOL( sys_resume_thread ),
	KSYMBOL( sys_send_msg ),
	KSYMBOL( setgid ),
	KSYMBOL( setpgid ),
	KSYMBOL( setsid ),
	KSYMBOL( setuid ),
	KSYMBOL( sys_sigaction ),
	KSYMBOL( sys_suspend_thread ),
	KSYMBOL( symlink ),
	KSYMBOL( based_symlink ),
	KSYMBOL( sys_unmount ),
	KSYMBOL( sys_wait4 ),
	KSYMBOL( sys_wait_for_thread ),
	KSYMBOL( sys_waitpid ),
	KSYMBOL( unprotect_dos_mem ),
	KSYMBOL( vsprintf ),
	KSYMBOL( wake_up_queue ),
	KSYMBOL( wake_up_sleepers ),
	KSYMBOL( wakeup_thread ),

	KSYMBOL( create_device_node ),
	KSYMBOL( delete_device_node ),
	KSYMBOL( rename_device_node ),

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
	KSYMBOL( get_semaphore_info ),
	KSYMBOL( get_semaphore_count ),
	KSYMBOL( get_semaphore_owner ),
	KSYMBOL( is_semaphore_locked ),
	KSYMBOL( apm_poweroff ),

	// TLD functions
	KSYMBOL( alloc_tld ),
	KSYMBOL( free_tld ),

	// Memory-area functions:

	KSYMBOL( alloc_area_list ),
	KSYMBOL( create_area ),
	KSYMBOL( delete_area ),
	KSYMBOL( resize_area ),
	KSYMBOL( remap_area ),
//  KSYMBOL(map_area_to_file),
	KSYMBOL( get_area_info ),
	KSYMBOL( memcpy_to_user ),
	KSYMBOL( memcpy_from_user ),
	KSYMBOL( strncpy_from_user ),
	KSYMBOL( strcpy_to_user ),
	KSYMBOL( lock_mem_area ),
	KSYMBOL( unlock_mem_area ),

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

	KSYMBOL( iterate_directory ),
	KSYMBOL( get_directory_path ),

	KSYMBOL( notify_node_monitors ),

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
	KSYMBOL( flush_cache_block ),
	KSYMBOL( setup_device_cache ),
	KSYMBOL( shutdown_device_cache ),

	// Partition table functions:
	KSYMBOL( decode_disk_partitions ),

	// Network functions:
	KSYMBOL( alloc_pkt_buffer ),
	KSYMBOL( reserve_pkt_header ),
	KSYMBOL( free_pkt_buffer ),
	KSYMBOL( init_net_queue ),
	KSYMBOL( delete_net_queue ),
	KSYMBOL( enqueue_packet ),
	KSYMBOL( remove_head_packet ),
	KSYMBOL( setsockopt ),
	KSYMBOL( getsockopt ),

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
	KSYMBOL( dispatch_net_packet ),
	KSYMBOL( add_net_interface ),
	KSYMBOL( get_net_interface ),
	KSYMBOL( ip_find_route ),
	KSYMBOL( ip_release_route ),
	KSYMBOL( ip_send ),

	// Bootmodule functions:

	KSYMBOL( get_boot_module_count ),
	KSYMBOL( get_boot_module ),
	KSYMBOL( put_boot_module ),
	KSYMBOL( get_kernel_arguments ),
	KSYMBOL( get_str_arg ),
	KSYMBOL( get_num_arg ),

	KSYMBOL( __const_udelay ),
	KSYMBOL( __udelay ),
	KSYMBOL( __ctype_flags ),
	KSYMBOL( __ctype_tolower ),
	KSYMBOL( __ctype_toupper ),

	KSYMBOL( request_dma ),
	KSYMBOL( free_dma ),
	KSYMBOL( enable_dma_channel ),
	KSYMBOL( disable_dma_channel ),
	KSYMBOL( clear_dma_ff ),
	KSYMBOL( set_dma_mode ),
	KSYMBOL( set_dma_page ),
	KSYMBOL( set_dma_address ),
	KSYMBOL( set_dma_count ),

	// Public message port functions:

	KSYMBOL( make_port_public ),
	KSYMBOL( make_port_private ),
	KSYMBOL( find_port ),

	// Randomness

	KSYMBOL( seed ),
	KSYMBOL( rand ),

	{NULL, NULL}
};
