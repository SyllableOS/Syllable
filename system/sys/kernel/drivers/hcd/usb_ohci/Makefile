CFLAGS   += -kernel -fno-PIC -c -D__ENABLE_DEBUG__
AFLAGS   = -c

C_INCLUDE_PATH=$(ATHEOS_INCLUDE_PATH)

OBJS := usb-ohci.o


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




all :	objs $(OBJDIR)/usb_ohci

objs:
	mkdir objs

$(OBJDIR)/usb_ohci: $(OBJS) Makefile
	$(CC) -kernel $(OBJS) -o $(OBJDIR)/usb_ohci

install: all
	cp -f $(OBJDIR)/usb_ohci $(IMAGE)/system/drivers/dev/hcd/usb_ohci

dist: install

clean:
	rm -f $(OBJS) $(OBJDIR)/usb_ohci

