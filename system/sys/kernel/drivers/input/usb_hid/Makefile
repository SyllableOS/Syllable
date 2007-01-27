CFLAGS   += -kernel -fno-PIC -c
AFLAGS   = -c

C_INCLUDE_PATH=$(ATHEOS_INCLUDE_PATH)

OBJS := hid.o


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




all :	objs $(OBJDIR)/usb_hid

objs:
	mkdir objs

$(OBJDIR)/usb_hid: $(OBJS) Makefile
	$(CC) -kernel $(OBJS) -o $(OBJDIR)/usb_hid

install: all
	cp -f $(OBJDIR)/usb_hid $(IMAGE)/system/drivers/dev/input/usb_hid

dist: install

clean:
	-rm -f $(OBJS) $(OBJDIR)/usb_hid

