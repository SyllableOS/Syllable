CFLAGS   = -kernel -fno-PIC -c -O3 -Wall -I../../bus/ata
AFLAGS   = -c

# Enable the following to enable debug output
CFLAGS += -D__ENABLE_DEBUG__

OBJS := ata_pci.o ata_via.o ata_intel.o ata_sis.o ata_amd.o

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




all :	objs $(OBJDIR)/ata_pci

objs:
	mkdir objs

$(OBJDIR)/ata_pci: $(OBJS) Makefile
	gcc -kernel $(OBJS) -o $(OBJDIR)/ata_pci

install: all
	cp -f $(OBJDIR)/ata_pci /system/drivers/dev/hcd/ata_pci

dist: all
	cp -f $(OBJDIR)/ata_pci $(DIST_DIR)/atheos/sys/drivers/dev/hcd/ata_pci

clean:
	-rm -f $(OBJS)
	-rm -f $(OBJDIR)/ata_pci






























