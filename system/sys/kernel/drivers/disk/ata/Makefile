CFLAGS   = -kernel -fno-PIC -c -O3 -Wall
AFLAGS   = -c

# Enable the following to enable possible CSH addressing
#CFLAGS += -D__ENABLE_CSH_

# Enable the following to enable debug output
CFLAGS += -D__ENABLE_DEBUG__

OBJS := ata.o ata-drive.o atapi-drive.o ata-io.o ata-probe.o ata-dma.o


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




all :	objs $(OBJDIR)/ata

objs:
	mkdir objs

$(OBJDIR)/ata: $(OBJS) Makefile
	gcc -kernel $(OBJS) -o $(OBJDIR)/ata

install: all
	cp -f $(OBJDIR)/ata /system/drivers/dev/disk/ata

dist: all
	cp -f $(OBJDIR)/ata $(DIST_DIR)/atheos/sys/drivers/dev/disk/ata

clean:
	-rm -f $(OBJS)
	-rm -f $(OBJDIR)/ata


