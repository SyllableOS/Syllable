/*
 *  The AtheOS kernel
 *  Copyright (C) 1999 - 2001 Kurt Skauen
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

#ifndef	_F_SYSCALL_H_
#define	_F_SYSCALL_H_


/*#define	__NR_idle			0*/
#define	__NR_open			1
#define	__NR_close			2
#define	__NR_read			3
#define	__NR_write			4
#define	__NR_Fork			5
#define	__NR_exit			6
#define	__NR_rename			7
#define	__NR_getdents			8
#define	__NR_alarm			9
#define	__NR_wait4			10
#define	__NR_fstat			11
#define	__NR_FileLength			12
#define	__NR_old_lseek			13
#define	__NR_mkdir			14
#define	__NR_rmdir			15
#define	__NR_dup			16
#define	__NR_dup2			17
#define	__NR_fchdir			18
#define	__NR_chdir			19
#define	__NR_unlink			20
#define	__NR_rewinddir			21
#define	__NR_get_thread_info		22
#define	__NR_get_thread_proc		23
#define	__NR_get_next_thread_info	24
#define	__NR_get_thread_id		25
#define	__NR_send_data			26
#define	__NR_receive_data		27
#define	__NR_thread_data_size		28
#define	__NR_has_data			29
#define	__NR_SetThreadExitCode		30
#define	__NR_old_spawn_thread		31
#define	__NR_spawn_thread		32
#define	__NR_get_raw_system_time	33
#define	__NR_get_raw_real_time		34
#define	__NR_get_raw_idle_time		35
#define	__NR_umask			36
#define	__NR_mknod			37
#define	__NR_reboot			38
#define	__NR_GetToken			39
#define	__NR_lseek			40
#define	__NR_utime			41
#define	__NR_select			42
#define	__NR_exit_thread		43
#define	__NR_initialize_fs		44
#define	__NR_DebugPrint			45
#define	__NR_realint			46
#define	__NR_get_system_path		47
#define	__NR_get_app_server_port	48
/*#define	__NR_MemClear			49*/
#define	__NR_mount			50
#define	__NR_unmount			51
#define	__NR_create_area		52
#define	__NR_remap_area			53
#define	__NR_get_area_info		54
#define	__NR_sbrk			55
#define	__NR_get_vesa_info		56
#define	__NR_get_vesa_mode_info		57
#define	__NR_set_real_time		58
#define	__NR_create_port		59
#define	__NR_delete_port		60
#define	__NR_send_msg			61
#define	__NR_get_msg			62
#define	__NR_raw_send_msg_x		63
#define	__NR_raw_get_msg_x		64
#define	__NR_debug_write		65
#define	__NR_debug_read			66
#define	__NR_get_system_info		67
#define	__NR_create_semaphore		68
#define	__NR_delete_semaphore		69
#define	__NR_lock_semaphore		70
#define	__NR_raw_lock_semaphore_x	71
#define	__NR_unlock_semaphore_x		72
#define	__NR_get_semaphore_info		73
#define	__NR_clone_area			74
#define	__NR_execve			75
#define	__NR_delete_area		76
#define	__NR_chmod			77
#define	__NR_fchmod			78
#define	__NR_GetTime			79
#define	__NR_SetTime			80
#define	__NR_old_load_library		81
#define	__NR_unload_library		82
#define	__NR_load_library		83
#define	__NR_get_symbol_address		84
#define	__NR_chown			85
#define	__NR_fchown			86
#define	__NR_raw_read_pci_config	87
#define	__NR_raw_write_pci_config	88
#define	__NR_get_pci_info		89
/*#define	__NR_signal			90*/
#define	__NR_sig_return			91
#define	__NR_kill			92
#define	__NR_sigaction			93
/*
#define	__NR_sigaddset			94
#define	__NR_sigdelset			95
#define	__NR_sigemptyset		96
#define	__NR_sigfillset			97
#define	__NR_sigismember		98
*/
#define	__NR_sigpending			99
#define	__NR_sigprocmask		100
#define	__NR_sigsuspend			101
/*#define	__NR_Delay			102*/
#define	__NR_set_thread_priority	103
#define	__NR_suspend_thread		104
#define	__NR_resume_thread		105
#define	__NR_wait_for_thread		106
#define	__NR_snooze			107
/*#define	__NR_Permit			108*/
#define	__NR_killpg			109
#define	__NR_kill_proc			110
#define	__NR_get_process_id		111
#define	__NR_sync			112
#define	__NR_fsync			113
#define	__NR_isatty			114
#define	__NR_fcntl			115
#define __NR_ioctl			116
#define __NR_pipe			117
#define __NR_access			118
/*#define __NR_set_strace_level			119*/
#define	__NR_symlink			120
#define	__NR_readlink			121
#define	__NR_call_v86			122
#define	__NR_stat			123
#define	__NR_lstat			124
#define	__NR_setpgid			125
#define	__NR_getpgid			126
#define	__NR_getpgrp			127
#define __NR_getppid			128
#define	__NR_getsid			129
#define	__NR_setsid			130
#define	__NR_getegid			131
#define	__NR_geteuid			132
#define	__NR_getgid			133
#define	__NR_getgroups			134
#define	__NR_getuid			135
#define	__NR_setgid			136
#define	__NR_setuid			137
#define	__NR_set_app_server_port	138
#define	__NR_rename_thread		139

#define	__NR_get_mount_point_count	140
#define	__NR_get_mount_point		141
#define	__NR_get_fs_info		142

#define	__NR_open_attrdir		143
#define	__NR_close_attrdir		144
#define	__NR_rewind_attrdir		145
#define	__NR_read_attrdir		146
#define	__NR_remove_attr		147
/*#define __NR_rename_attr			148*/
#define	__NR_stat_attr			149
#define	__NR_write_attr			150
#define	__NR_read_attr			151
#define __NR_get_image_info		152
#define __NR_freadlink			153
#define __NR_get_directory_path		154
#define __NR_socket			155
#define __NR_closesocket		156
#define __NR_bind			157
#define __NR_sendmsg			158
#define __NR_recvmsg			159
#define __NR_connect			160
#define __NR_listen			161
#define __NR_accept			162
#define __NR_setsockopt			163
#define __NR_getsockopt			164
#define __NR_getpeername		165
#define __NR_shutdown			166
#define __NR_based_open			167
#define __NR_getsockname		168
#define __NR_setregid			169
#define __NR_setgroups			170
#define __NR_setreuid			171
#define __NR_setresuid			172
#define __NR_getresuid			173
#define __NR_setresgid			174
#define __NR_getresgid			175
#define __NR_setfsuid			176
#define __NR_setfsgid			177
#define __NR_chroot			178

#define __NR_open_indexdir		179
#define __NR_rewind_indexdir		180
#define __NR_read_indexdir		181
#define __NR_create_index		182
#define __NR_remove_index		183
#define __NR_stat_index			184
#define __NR_probe_fs			185
#define __NR_get_next_semaphore_info	186
#define __NR_get_port_info		187
#define	__NR_get_next_port_info		188

#define	__NR_read_pos			189
#define	__NR_readv			190
#define	__NR_readv_pos			191
#define	__NR_write_pos			192
#define	__NR_writev			193
#define	__NR_writev_pos			194

#define	__NR_alloc_tld			195
#define	__NR_free_tld			196

#define	__NR_watch_node			197
#define	__NR_delete_node_monitor	198

#define __NR_open_image_file		199
#define __NR_find_module_by_address	200
#define __NR_based_mkdir		201
#define __NR_based_rmdir		202
#define __NR_based_unlink		203
#define __NR_based_symlink		204

#define __NR_apm_poweroff		205

#define __NR_make_port_public		206
#define __NR_make_port_private		207
#define __NR_find_port			208

#define __NR_strace			209
#define __NR_strace_exclude		210
#define __NR_strace_include		211
#define __NR_ptrace			212
#define __NR_ftruncate			213
#define __NR_truncate			214
#define __NR_sigaltstack		215

#define __NR_get_tld_addr		216

#define __NR_get_msg_size		217
#define __NR_do_schedule		218
#define __NR_event				219

#define __NR_setitimer			220
#define __NR_getitimer			221

#define	__NR_SysCallCount		222
#define	__NR_DeprecatedSysCallCount	11	/* Obsolete syscalls */

/* The TRUE number of ACTIVE syscalls E.g. __NR_SysCallCount minus the number of
   deprecated syscalls */
#define	__NR_TrueSysCallCount ( __NR_SysCallCount - __NR_DeprecatedSysCallCount )

#endif /* _F_SYSCALL_H_ */
