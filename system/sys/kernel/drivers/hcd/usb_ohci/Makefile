CFLAGS   = -kernel -fno-PIC -c -O3 -Wall -D__ENABLE_DEBUG__
AFLAGS   = -c


OBJS := usb-ohci.o


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




all :	objs $(OBJDIR)/usb_ohci

objs:
	mkdir objs

$(OBJDIR)/usb_ohci: $(OBJS) Makefile
	$(CC) -kernel $(OBJS) -o $(OBJDIR)/usb_ohci

install: all
	mkdir -p /system/drivers/dev/hcd
	cp -f $(OBJDIR)/usb_ohci /system/drivers/dev/hcd/usb_ohci

dist: all
	mkdir -p $(DIST_DIR)/atheos/sys/drivers/dev/hcd
	cp -f $(OBJDIR)/usb_ohci $(DIST_DIR)/atheos/sys/drivers/dev/hcd/usb_ohci

clean:
	rm -f $(OBJS) $(OBJDIR)/usb_ohci

