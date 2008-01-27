CFLAGS   = -Wall -c -O2 -std=gnu99 -fno-optimize-sibling-calls -Wstrict-prototypes -I./ -D__KERNEL__ -D__BUILD_KERNEL__ -ffreestanding -no-fPIC -mpreferred-stack-boundary=2 -fno-strict-aliasing
AFLAGS   = -c

OLD_INCLUDE_PATH:=$(C_INCLUDE_PATH)
C_INCLUDE_PATH=$(ATHEOS_INCLUDE_PATH)

VPATH =	./ lib vfs net nls

CP	= cp -f

LIBOBJS := ct_flags.o ct_upper.o ct_lower.o vsprintf.o string.o strstr.o qsort.o gcc34_memcpy.o strlcpy.o strlcat.o
VFSOBJS := vfs.o inode.o nodemonitor.o attributes.o flock.o root.o dev.o fifo.o pty.o
NETOBJS := core.o interface.o route.o ip.o udp.o tcp.o tcp_input.o tcp_output.o socket.o packets.o raw.o

NLSOBJS := nls_conv.o nls_iso8859-1.o nls_iso8859-2.o nls_iso8859-3.o nls_ascii.o nls_utf16.o \
	nls_cp1250.o nls_cp1251.o nls_cp1255.o nls_koi8-u.o nls_koi8-r.o nls_cp437.o nls_cp850.o \
	nls_cp852.o

OBJS := intel.o cpuid.o descriptors.o kernel.o dbconsole.o ksyms.o init.o fork.o syscall.o 	  		\
	debug.o timer.o process.o threads.o sema.o msgport.o memory.o strace.o	\
	smp.o apic.o udelay.o fault.o exec.o pci.o pgroup.o kmalloc.o swap.o dma.o	\
	elf.o timecalc.o virt86.o irq.o array.o schedule.o psignals.o mman.o	\
	areas.o vmalloc.o pagefault.o bcache.o bcalloc.o vesa_gfx.o random.o	\
	devices.o config.o ptrace.o save_i387.o resource.o \
	$(LIBOBJS) $(VFSOBJS) $(NETOBJS) $(NLSOBJS)

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
	ld -m elf_i386_syllable -Tkernel.lnk $(OBJS) $(OBJDIR)/version.o -o \
	   $(OBJDIR)/kernel.so `$(CC) -kernel -print-libgcc-file-name`
	$(CC) -nostdlib -w $(OBJDIR)/kernel.exports.c -o $(OBJDIR)/libkernel.so


$(OBJDIR)/eth_if: $(IF_OBJS) $(OBJDIR)/kernel.so
	$(CC) -kernel $(IF_OBJS) -L$(OBJDIR) -o $(OBJDIR)/eth_if

$(OBJDIR)/bintohex: bintohex.c
	export C_INCLUDE_PATH=$(OLD_INCLUDE_PATH) ; $(CC) bintohex.c -o $(OBJDIR)/bintohex

$(OBJDIR)/kernel.exports.c: ksyms.c
	grep '^[[:space:]]\+KSYMBOL' ksyms.c | \
	sed -e 's/^[[:space:]]\+KSYMBOL( *\([^ )]*\) *).*$$/void \1() {}/' \
	> $(OBJDIR)/kernel.exports.c

smp.c:	$(OBJDIR) $(OBJDIR)/smp_entry.hex

$(OBJDIR)/smp_entry.hex: $(OBJDIR)/smp_entry.bin $(OBJDIR)/bintohex
	$(OBJDIR)/bintohex $(OBJDIR)/smp_entry.bin $(OBJDIR)/smp_entry.hex

$(OBJDIR)/smp_entry.bin: smp_entry.asm
	nasm smp_entry.asm -o $(OBJDIR)/smp_entry.bin

install: all
	-mv /system/kernel.so /system/kernel.so.original
	-mv /system/drivers/net/if/eth_if /system/drivers/net/if/eth_if.original
	-mv /system/libraries/libkernel.so /system/libraries/libkernel.so.original
	cp -f $(OBJDIR)/kernel.so	/system/kernel.so
	cp -f $(OBJDIR)/eth_if		/system/drivers/net/if/eth_if
	$(CP) $(OBJDIR)/libkernel.so	/system/libraries/libkernel.so
	strip --strip-all /system/kernel.so

dist: all
	cp -f $(OBJDIR)/kernel.so	$(IMAGE)/system/kernel.so
	cp -f $(OBJDIR)/eth_if		$(IMAGE)/system/drivers/net/if/eth_if
	$(CP) $(OBJDIR)/libkernel.so	$(IMAGE)/system/libraries/libkernel.so
	strip --strip-all $(IMAGE)/system/kernel.so

#
# dependencies for kernel
#

$(OBJDIR)/version.o: version.c
version.c:	$(OBJS) $(OBJDIR)/kernel.exports.c Makefile
	touch version.c


#syscall.o:	syscall.s
#intel.o:	intel.s

doc:
	-rm -R docs/DriverAPI/* docs/Syscalls/* docs/Private/*
	doxygen driverapi.dox
	doxygen syscalls.dox
	doxygen private.dox

install-doc:
	-rm -R /Documentation/kernel/*
	-mkdir -p /Documentation/kernel
	cp -R docs/* /Documentation/kernel/

deps: $(OBJDIR) $(DEPS)

clean:
	rm -f $(OBJS) $(DEPS) $(IF_OBJS) $(OBJDIR)/version.o $(OBJDIR)/kernel.so $(OBJDIR)/libkernel.so $(OBJDIR)/eth_if $(OBJDIR)/kernel.exports.c

-include $(OBJDIR)/*.d

