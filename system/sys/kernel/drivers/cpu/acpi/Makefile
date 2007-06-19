CFLAGS   += -kernel -fno-PIC -c -I../../bus/acpi/include
AFLAGS   = -c

C_INCLUDE_PATH=$(ATHEOS_INCLUDE_PATH)

OBJS := acpi_processor.o acpi_cstate.o acpi_pstate.o


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




all :	objs $(OBJDIR)/acpi_cpu

objs:
	mkdir objs

$(OBJDIR)/acpi_cpu: $(OBJS) Makefile
	$(CC) -kernel $(OBJS) -o $(OBJDIR)/acpi_cpu

install: all
	cp -f $(OBJDIR)/acpi_cpu $(IMAGE)/system/drivers/dev/

dist: install

clean:
	-rm -f $(OBJS) $(OBJDIR)/acpi_cpu













