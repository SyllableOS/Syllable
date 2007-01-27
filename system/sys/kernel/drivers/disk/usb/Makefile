CFLAGS   += -kernel -fno-PIC -c
AFLAGS   = -c

C_INCLUDE_PATH=$(ATHEOS_INCLUDE_PATH)

OBJS := scsi_convert.o transport.o protocol.o jumpshot.o usb_disk.o


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
	cp -f $(OBJDIR)/usb $(IMAGE)/system/drivers/dev/disk/usb

dist: install

clean:
	-rm -f $(OBJS) $(OBJDIR)/usb

