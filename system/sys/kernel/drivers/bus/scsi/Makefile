CFLAGS   = -kernel -fno-PIC -c -O3 -Wall
AFLAGS   = -c


OBJS := scsi.o


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




all :	objs $(OBJDIR)/scsi

objs:
	mkdir objs

$(OBJDIR)/scsi: $(OBJS) Makefile
	$(CC) -kernel $(OBJS) -o $(OBJDIR)/scsi

install: all
	mkdir -p /system/drivers/bus
	cp -f $(OBJDIR)/scsi /system/drivers/bus/scsi

dist: all
	mkdir -p $(DIST_DIR)/atheos/sys/drivers/bus
	cp -f $(OBJDIR)/scsi $(DIST_DIR)/atheos/sys/drivers/bus/scsi

clean:
	-rm -f $(OBJS) $(OBJDIR)/scsi

