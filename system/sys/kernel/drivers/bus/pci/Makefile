CFLAGS   = -kernel -fno-PIC -c -O3 -Wall -fno-strict-aliasing -Wno-trigraphs -fno-common -D__ENABLE_DEBUG__ -Wstrict-prototypes -fomit-frame-pointer
AFLAGS   = -c

C_INCLUDE_PATH=$(ATHEOS_INCLUDE_PATH)

OBJS := pci.o pci_irq_router.o


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




all :	objs $(OBJDIR)/pci

objs:
	mkdir objs

$(OBJDIR)/pci: $(OBJS) Makefile
	$(CC) -kernel $(OBJS) -o $(OBJDIR)/pci

install: all
	mkdir -p /system/drivers/bus
	cp -f $(OBJDIR)/pci /system/drivers/bus/pci

dist: all
	mkdir -p $(DIST_DIR)/atheos/sys/drivers/bus
	cp -f $(OBJDIR)/pci $(DIST_DIR)/atheos/sys/drivers/bus/pci

clean:
	-rm -f $(OBJS) $(OBJDIR)/pci

