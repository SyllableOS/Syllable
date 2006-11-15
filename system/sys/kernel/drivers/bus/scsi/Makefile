CFLAGS   += -kernel -fno-PIC -c -I.
AFLAGS   = -c

CFLAGS += -D__ENABLE_DEBUG__

C_INCLUDE_PATH=$(ATHEOS_INCLUDE_PATH)

OBJS := scsi.o scsi_common.o scsi_generic.o scsi_disk.o scsi_cdrom.o


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




all :	objs $(OBJDIR)/scsi

objs:
	mkdir objs

$(OBJDIR)/scsi: $(OBJS) Makefile
	$(CC) -kernel $(OBJS) -o $(OBJDIR)/scsi

install: all
	mkdir -p /system/drivers/dev/bus
	cp -f $(OBJDIR)/scsi /system/drivers/dev/bus/scsi

dist: all
	mkdir -p $(DIST_DIR)/atheos/sys/drivers/dev/bus
	cp -f $(OBJDIR)/scsi $(DIST_DIR)/atheos/sys/drivers/dev/bus/scsi

clean:
	-rm -f $(OBJS) $(OBJDIR)/scsi

