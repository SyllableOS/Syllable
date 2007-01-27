CFLAGS   += -D__ENABLE_DEBUG__ -kernel -fno-PIC -c --freestanding
AFLAGS   = -c

C_INCLUDE_PATH=$(ATHEOS_INCLUDE_PATH)

OBJS := usbprinter.o


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




all :	objs $(OBJDIR)/usb_printer

objs:
	mkdir objs

$(OBJDIR)/usb_printer: $(OBJS) Makefile
	$(CC) -kernel $(OBJS) -o $(OBJDIR)/usb_printer

install: all
	cp -f $(OBJDIR)/usb_printer $(IMAGE)/system/drivers/dev/printer/usb_printer

dist: install

clean:
	-rm -f $(OBJS) $(OBJDIR)/usb_printer

