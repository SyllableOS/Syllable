CFLAGS   += -kernel -fno-PIC -c -D__ENABLE_DEBUG__
AFLAGS   = -c

C_INCLUDE_PATH=$(ATHEOS_INCLUDE_PATH)

OBJS := usb_hub.o usb.o


OBJDIR	:= objs
OBJS	:= $(addprefix $(OBJDIR)/,$(OBJS))


#
#	rules
#

$(OBJDIR)/%.o : %.c
	$(CC) $(CFLAGS) $< -o $@

$(OBJDIR)/%.o : %.cpp
	$(CXX) $(CFLAGS) $< -o $@

$(OBJDIR)/%.o : %.s
	$(CC) $(AFLAGS) -x assembler-with-cpp $< -o $@

$(OBJDIR)/%.o : %.S
	$(CC) $(CFLAGS) -x assembler-with-cpp $< -o $@




all :	objs $(OBJDIR)/usb

objs:
	mkdir objs

$(OBJDIR)/usb: $(OBJS) Makefile
	$(CC) -kernel $(OBJS) -o $(OBJDIR)/usb

install: all
	mkdir -p /system/drivers/dev/bus
	cp -f $(OBJDIR)/usb /system/drivers/dev/bus/usb

dist: all
	mkdir -p $(DIST_DIR)/atheos/sys/drivers/dev/bus
	cp -f $(OBJDIR)/usb $(DIST_DIR)/atheos/sys/drivers/dev/bus/usb

clean:
	-rm -f $(OBJS) $(OBJDIR)/usb

