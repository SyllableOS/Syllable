CFLAGS   = -kernel -fno-PIC -c -O3 -Wall -D__ENABLE_DEBUG__ -ffreestanding
AFLAGS   = -c


OBJS := hcd.o ehci.o gcc34_memcpy.o


OBJDIR	:= objs
OBJS	:= $(addprefix $(OBJDIR)/,$(OBJS))


#
#	rules
#

$(OBJDIR)/%.o : %.c
	$(CC) $(CFLAGS) $< -o $@

$(OBJDIR)/%.o : %.cpp
	$(CC) $(CFLAGS) $< -o $@

$(OBJDIR)/%.o : %.s
	$(CC) $(AFLAGS) -x assembler-with-cpp $< -o $@

$(OBJDIR)/%.o : %.S
	$(CC) $(CFLAGS) -x assembler-with-cpp $< -o $@




all :	objs $(OBJDIR)/usb_ehci

objs:
	mkdir objs

$(OBJDIR)/usb_ehci: $(OBJS) Makefile
	$(CC) -kernel $(OBJS) -o $(OBJDIR)/usb_ehci

install: all
	mkdir -p /system/drivers/dev/hcd
	cp -f $(OBJDIR)/usb_ehci /system/drivers/dev/hcd/usb_ehci

dist: all
	mkdir -p $(DIST_DIR)/atheos/sys/drivers/dev/hcd
	cp -f $(OBJDIR)/usb_ehci $(DIST_DIR)/atheos/sys/drivers/dev/hcd/usb_ehci

clean:
	rm -f $(OBJS) $(OBJDIR)/usb_ehci

