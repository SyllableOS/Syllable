CFLAGS   = -D__KERNEL__ -D__BUILD_KERNEL__ -no-fPIC -c -g -O3 -fno-strict-aliasing -Wall -march=i586 -mcpu=i586 -I./
CPPFLAGS = $(CFLAGS)
AFLAGS   = -c

OLD_INCLUDE_PATH:=$(C_INCLUDE_PATH)
C_INCLUDE_PATH=$(ATHEOS_INCLUDE_PATH)
CPLUS_INCLUDE_PATH=$(ATHEOS_INCLUDE_PATH)

VPATH =	./ lib vfs net ../devices ../drivers/disk/bios ../fs/lfndos ../fs/afs

CC	= gcc
CP	= cp -f

VFSOBJS := vfs.o inode.o nodemonitor.o attributes.o flock.o root.o dev.o fifo.o pty.o
NETOBJS := core.o interface.o route.o ip.o udp.o tcp.o tcp_input.o tcp_output.o socket.o packets.o raw.o
LIBOBJS := ct_flags.o ct_upper.o ct_lower.o vsprintf.o string.o qsort.o

OBJS :=intel.o kernel.o dbconsole.o ksyms.o init.o fork.o syscall.o 	  		\
	debug.o timer.o process.o threads.o sema.o msgport.o memory.o strace.o	\
	smp.o udelay.o fault.o exec.o pci.o pgroup.o kmalloc.o swap.o dma.o	\
	elf.o timecalc.o virt86.o irq.o array.o schedule.o psignals.o mman.o	\
	areas.o vmalloc.o pagefault.o bcache.o bcalloc.o vesa_gfx.o random.o	\
	devices.o config.o ptrace.o \
	$(VFSOBJS) $(NETOBJS) $(LIBOBJS)

IF_OBJS := eth_interface.o arp.o

OBJDIR	:= objs

IF_OBJS	:= $(addprefix $(OBJDIR)/,$(IF_OBJS))

include $(ATHEOS_SRC)/makefile.rules

#
#	Final targets
#

all:	$(OBJDIR) $(OBJDIR)/kernel.so $(OBJDIR)/eth_if

$(OBJDIR):
	mkdir $(OBJDIR)


$(OBJDIR)/kernel.so: $(OBJDIR)/version.o
	ld -m elf_i386_atheos -Tkernel.lnk $(OBJS) $(OBJDIR)/version.o -o \
	   $(OBJDIR)/kernel.so `gcc -kernel -print-libgcc-file-name`
	gcc -nostdlib -w $(OBJDIR)/kernel.exports.c -o $(OBJDIR)/libkernel.so


$(OBJDIR)/eth_if: $(IF_OBJS) $(OBJDIR)/kernel.so
	gcc -kernel $(IF_OBJS) -L$(OBJDIR) -o $(OBJDIR)/eth_if

$(OBJDIR)/bintohex: bintohex.c
	export C_INCLUDE_PATH=$(OLD_INCLUDE_PATH) ; gcc bintohex.c -o $(OBJDIR)/bintohex

$(OBJDIR)/kernel.exports.c: ksyms.c
	grep '^[[:space:]]\+KSYMBOL' ksyms.c | \
	sed -e 's/^[[:space:]]\+KSYMBOL( *\([^ )]*\) *).*$$/void \1() {}/' \
	> $(OBJDIR)/kernel.exports.c

smp.c:	$(OBJDIR)/smp_entry.hex

$(OBJDIR)/smp_entry.hex: $(OBJDIR)/smp_entry.bin $(OBJDIR)/bintohex
	$(OBJDIR)/bintohex $(OBJDIR)/smp_entry.bin $(OBJDIR)/smp_entry.hex

$(OBJDIR)/smp_entry.bin: smp_entry.asm
	nasm smp_entry.asm -o $(OBJDIR)/smp_entry.bin

install: all
	cp -f $(OBJDIR)/kernel.so	/system/kernel.so
	cp -f $(OBJDIR)/eth_if		/system/drivers/net/if/eth_if
	$(CP) $(OBJDIR)/libkernel.so	/system/libs/libkernel.so

dist: all
	cp -f $(OBJDIR)/kernel.so	$(DIST_DIR)/atheos/sys/kernel.so
	cp -f $(OBJDIR)/eth_if		$(DIST_DIR)/atheos/sys/drivers/net/if/eth_if
	$(CP) $(OBJDIR)/libkernel.so	$(DIST_DIR)/atheos/sys/libs/libkernel.so

#
# dependencies for kernel
#

$(OBJDIR)/version.o: version.c
version.c:	$(OBJS) $(OBJDIR)/kernel.exports.c Makefile
	touch version.c


#syscall.o:	syscall.s
#intel.o:	intel.s

doc:
	-rm -R docs/DriverAPI/* docs/Syscalls/*
	doxygen driverapi.dox
	doxygen syscalls.dox

install-doc:
	-rm -R /atheos/Documentation/kernel/*
	cp -R docs/* /atheos/Documentation/kernel/

deps: $(OBJDIR) $(DEPS)

clean:
	rm -f $(OBJS) $(DEPS) $(IF_OBJS) $(OBJDIR)/version.o $(OBJDIR)/kernel.so $(OBJDIR)/libkernel.so $(OBJDIR)/eth_if $(OBJDIR)/kernel.exports.c

-include $(OBJDIR)/*.d
