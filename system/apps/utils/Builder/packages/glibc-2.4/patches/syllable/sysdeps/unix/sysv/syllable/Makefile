# Don't compile the ctype glue code, since there is no old non-GNU C library.
inhibit-glue = yes

# Additional Syllable specific routines
ifeq ($(subdir),misc)

# Debugging functions
sysdep_routines += dbprintf syslog
sysdep_headers += sys/debug.h

# System information
sysdep_routines += get_system_info xget_system_info get_fs_info
sysdep_routines += get_system_time get_real_time get_idle_time
sysdep_headers += sys/systime.h

# Semaphore & mutexes
sysdep_routines += unlock_semaphore get_semaphore_count get_semaphore_holder lock_semaphore_x

# IPC
sysdep_routines += get_msg_x send_msg_x

# Processes & threads
sysdep_routines += tld get_thread_id get_process_id spawn_thread

# ELF images & libraries
sysdep_routines += image

# Filesystem extensions
sysdep_routines += close_extended open_attrdir open_indexdir fopendir probe_fs xprobe_fs
sysdep_routines += read_attr write_attr read_attrdir read_indexdir rewind_attrdir
sysdep_routines += rewind_indexdir
sysdep_headers += sys/mount.h

# PCI and general device support
sysdep_routines += pci

# Misc system calls for no specific subsystem
sysdep_routines += reboot libc_lock create_area set_real_time

# Some stubs
sysdep_routines += fsync ftruncate

# Additional headers we need to install
install-others += $(inst_includedir)/bits/syscall.h

# Generate the list of SYS_* macros for the system calls (__NR_* macros).
$(objpfx)syscall-%.h $(objpfx)syscall-%.d: ../syllable/sysdeps/unix/sysv/syllable/sys/syscall.h
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

generated += syscall-list.h syscall-list.d

# endif $(subdir),misc
endif

# Ensure we're thread-safe
ifeq ($(subdir),csu)
sysdep_routines += errno-loc
# endif $(subdir),csu
endif

# Stuff we need for POSIX.1
ifeq ($(subdir),posix)
sysdep_routines += uname
# endif $(subdir),posix
endif

# Directory entries
ifeq ($(subdir),dirent)
sysdep_routines += getdents getdents64 getdirentries getdirentries64
# endif $(subdir),dirent
endif

# Sockets & networks
ifeq ($(subdir),socket)
sysdep_routines += sa_len
sysdep_headers += net/ethernet.h net/if_arp.h
# endif $(subdir),socket
endif

# Dynamic link support
ifeq ($(subdir),dlfcn)
# This is enough to ensure that libdl.so is not built
extra-libs =
extra-libs-others =
endif

# ELF support on Syllable is complicated by the fact that we don't use GNU ld.so
# We have to override the targets and variables of the standard Makefile for the
# elf subdir, and ensures that none of the GNU ld.so stuff we don't need and don't
# want isn't built
ifeq ($(subdir),elf)
# Additional internal ELF function stubs
sysdep_routines += dl dl-stubs rtld-stubs
# endif $(subdir),elf
endif

# Glibc does not require the dynamic linker when running under Syllable
link-libc = $(common-objpfx)libc.so$(libc.so-version) \
	    $(common-objpfx)$(patsubst %,$(libtype.oS),c) $(gnulib)
# This is how to find at build-time things that will be installed there.
rpath-dirs = math elf dlfcn nss nis rt resolv crypt
