CFLAGS   += -Wstrict-prototypes -kernel -fno-PIC -c -fno-strict-aliasing -Wno-trigraphs -fno-common -D__ENABLE_DEBUG__ -I../acpi/include
AFLAGS   = -c

C_INCLUDE_PATH=$(ATHEOS_INCLUDE_PATH)

OBJS := pci.o pci_acpi_link.o pci_acpi_router.o pci_registers.o


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




all :	objs $(OBJDIR)/pci

objs:
	mkdir objs

$(OBJDIR)/pci: $(OBJS) Makefile
	$(CC) -kernel $(OBJS) -o $(OBJDIR)/pci

install: all
	cp -f $(OBJDIR)/pci $(IMAGE)/system/drivers/dev/bus/pci

dist: install

clean:
	-rm -f $(OBJS) $(OBJDIR)/pci
