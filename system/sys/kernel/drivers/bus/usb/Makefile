
CFLAGS   = -kernel -fno-PIC -c -O3 -Wall
AFLAGS   = -c


OBJS := usb_hub.o usb.o


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




all :	objs $(OBJDIR)/usb

objs:
	mkdir objs

$(OBJDIR)/usb: $(OBJS) Makefile
	gcc -kernel $(OBJS) -o $(OBJDIR)/usb

install: all
	mkdir -p /system/drivers/bus
	cp -f $(OBJDIR)/usb /system/drivers/bus/usb

dist: all
	mkdir -p $(DIST_DIR)/atheos/sys/drivers/bus
	cp -f $(OBJDIR)/usb $(DIST_DIR)/atheos/sys/drivers/bus/usb

clean:
	-rm -f $(OBJS) $(OBJDIR)/usb







