
CFLAGS   = -kernel -fno-PIC -c -O3 -Wall
AFLAGS   = -c


OBJS := usb_uhci.o


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




all :	objs $(OBJDIR)/usb_uhci

objs:
	mkdir objs

$(OBJDIR)/usb_uhci: $(OBJS) Makefile
	gcc -kernel $(OBJS) -o $(OBJDIR)/usb_uhci

install: all
	mkdir -p /system/drivers/dev/hcd
	cp -f $(OBJDIR)/usb_uhci /system/drivers/dev/hcd/usb_uhci

dist: all
	mkdir -p $(DIST_DIR)/atheos/sys/drivers/dev/hcd
	cp -f $(OBJDIR)/usb_uhci $(DIST_DIR)/atheos/sys/drivers/dev/hcd/usb_uhci

clean:
	rm -f $(OBJS) $(OBJDIR)/usb_uhci




