
CFLAGS   = -kernel -fno-PIC -c -O3 -Wall
AFLAGS   = -c


OBJS := usbmouse.o


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




all :	objs $(OBJDIR)/usb_mouse

objs:
	mkdir objs

$(OBJDIR)/usb_mouse: $(OBJS) Makefile
	gcc -kernel $(OBJS) -o $(OBJDIR)/usb_mouse

install: all
	mkdir -p /system/drivers/dev/input
	cp -f $(OBJDIR)/usb_mouse /system/drivers/dev/input/usb_mouse

dist: all
	mkdir -p $(DIST_DIR)/atheos/sys/drivers/dev/input
	cp -f $(OBJDIR)/usb_mouse $(DIST_DIR)/atheos/sys/drivers/dev/input/usb_mouse

clean:
	-rm -f $(OBJS) $(OBJDIR)/usb_mouse






