ifeq ($(subdir),csu)
CFLAGS-init-first.c = -fkeep-inline-functions

sysdep_routines += errno-loc
endif

ifeq ($(subdir),db2)
CPPFLAGS += -DHAVE_LLSEEK=1
endif

ifeq ($(subdir),misc)
sysdep_routines += getresuid getresgid stricmp strnicmp read_pos readv_pos write_pos writev_pos
#elided-routines += syslog
sysdep_routines += reboot get_system_path xget_system_info get_system_info get_system_time get_real_time get_idle_time set_real_time
static-only-routines = get_system_info
# PCI support

sysdep_routines += get_pci_info pci

# Some kernel support for calling the Vesa2.0 BIOS
sysdep_routines += realint get_vesa_mode_info get_vesa_info

# AtheOS thread API
sysdep_routines += spawn_thread suspend_thread resume_thread wait_for_thread	\
		   set_thread_priority get_thread_id get_process_id exit_thread	\
		   get_thread_info get_next_thread_info get_thread_proc		\
		   rename_thread send_data receive_data thread_data_size	\
		   tld kill_proc

# The semaphore API
sysdep_routines += create_semaphore delete_semaphore lock_semaphore		\
		   lock_semaphore_x unlock_semaphore unlock_semaphore_x		\
		   get_semaphore_info get_next_semaphore_info			\
		   get_semaphore_count get_semaphore_holder

# The message port API
sysdep_routines += create_port delete_port get_msg get_msg_x send_msg send_msg_x	\
		   get_port_info get_next_port_info set_app_server_port get_app_server_port

# AtheOS extention to the file system API
sysdep_routines += based_open fsync sync fopendir freadlink get_directory_path	\
		   get_mount_point_count get_mount_point get_fs_info		\
		   xget_fs_info probe_fs xprobe_fs mount unmount initialize_fs	\
		   open_attrdir close_attrdir rewind_attrdir read_attrdir	\
		   remove_attr stat_attr write_attr read_attr open_indexdir	\
		   close_indexdir read_indexdir rewind_indexdir create_index	\
		   remove_index watch_node delete_node_monitor
static-only-routines = get_fs_info probe_fs

# AtheOS memory managment API

sysdep_routines += create_area clone_area delete_area get_area_info remap_area


# The image (dll) API
sysdep_routines += image get_image_info get_symbol_address libc_lock
sysdep_headers += sys/image.h

# Debugging API
sysdep_routines += dbprintf strace strace_exclude strace_include debug_read debug_write
sysdep_headers += sys/debug.h



sysdep_headers += sys/mount.h sys/acct.h sys/quota.h sys/fsuid.h \
		  sys/sendfile.h

install-others += $(inst_includedir)/bits/syscall.h

# Generate the list of SYS_* macros for the system calls (__NR_* macros).
$(objpfx)syscall-%.h $(objpfx)syscall-%.d: ../sysdeps/unix/sysv/atheos/sys/syscall.h
	rm -f $(@:.h=.d)-t
	{ \
	 echo '/* Generated at libc build time from kernel syscall list.  */';\
	 echo ''; \
	 echo '#ifndef _SYSCALL_H'; \
	 echo '# error "Never use <bits/syscall.h> directly; include <sys/syscall.h> instead."'; \
	 echo '#endif'; \
	 echo ''; \
	 SUNPRO_DEPENDENCIES='$(@:.h=.d)-t $@' \
	 $(CC) -E -x c $(sysincludes) $< -D_LIBC -dM | \
	 sed -n 's@^#define __NR_\([^ ]*\) .*$$@#define SYS_\1 __NR_\1@p'; \
	} > $(@:.d=.h).new
	mv -f $(@:.d=.h).new $(@:.d=.h)
	sed < $(@:.h=.d)-t > $(@:.h=.d)-t2 \
	    -e 's,$(subst .,\.,$@),$(patsubst $(objpfx)%,$$(objpfx)%,\
					      $(@:.d=.h) $(@:.h=.d)),'
	rm -f $(@:.h=.d)-t
	mv -f $(@:.h=.d)-t2 $(@:.h=.d)

$(inst_includedir)/bits/syscall.h: $(objpfx)syscall-list.h $(+force)
	$(make-target-directory)
	if test -r $@ && cmp -s $< $@; \
        then echo 'bits/syscall.h unchanged'; \
        else $(INSTALL_DATA) $< $@; fi

ifndef no_deps
# Get the generated list of dependencies (probably /usr/include/asm/unistd.h).
-include $(objpfx)syscall-list.d
endif
generated += syscall-list.h syscall-list.d
endif

ifeq ($(subdir),util)
sysdep_routines += openpty
endif


ifeq ($(subdir),time)
sysdep_headers += sys/timex.h

sysdep_routines += ntp_gettime stime
endif

ifeq ($(subdir),socket)
sysdep_headers += net/if.h net/if_ppp.h net/ppp-comp.h \
		  net/ppp_defs.h net/if_arp.h net/route.h net/ethernet.h \
		  net/if_slip.h net/if_packet.h net/if_shaper.h
sysdep_routines += cmsg_nxthdr sa_len send recv sendto recvfrom

endif

#ifeq ($(subdir),sunrpc)
#sysdep_headers += nfs/nfs.h
#endif

ifeq ($(subdir),termios)
sysdep_headers += termio.h
endif

ifeq ($(subdir),stdio-common)
# Just disable the auto generation in sysdeps/generic
inhibit-siglist := yes
endif

ifeq ($(subdir),io)
sysdep_routines += mknod pipe lseek
endif

ifeq ($(subdir),posix)
sysdep_headers += bits/pthreadtypes.h
sysdep_routines += getpid uname
endif

ifeq ($(subdir),inet)
sysdep_headers += netinet/if_fddi.h netinet/if_tr.h netinet/igmp.h \
		  netipx/ipx.h netash/ash.h netax25/ax25.h netatalk/at.h \
		  netrom/netrom.h netpacket/packet.h netrose/rose.h \
		  neteconet/ec.h
endif

# Don't compile the ctype glue code, since there is no old non-GNU C library.
inhibit-glue = yes

ifeq ($(subdir),dirent)
sysdep_routines += getdents64 getdirentries getdirentries64
endif

ifeq ($(subdir),nis)
CFLAGS-ypclnt.c = -DUSE_BINDINGDIR=1
endif

ifeq ($(subdir),elf)
endif


# We don't need the dynamic linker when running under AtheOS (KHS)
link-libc = $(common-objpfx)libc.so$(libc.so-version) \
	    $(common-objpfx)$(patsubst %,$(libtype.oS),c) \
	     -Xlinker --whole-archive $(gnulib) -Xlinker --no-whole-archive

define built-program-cmd
	$(built-program-file)
endef

atheos=yes
config-LDFLAGS =
libc.so-version=.1
libm.so-version=.1
libnss_files.so=.2
libnss_dns.so=.2
libnss_db.so=.2
