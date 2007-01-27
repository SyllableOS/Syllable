CFLAGS   += -kernel -fno-PIC -c -I../../bus/acpi/include
AFLAGS   = -c

C_INCLUDE_PATH=$(ATHEOS_INCLUDE_PATH)

OBJS := battery.o


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




all :	objs $(OBJDIR)/battery

objs:
	mkdir objs

$(OBJDIR)/battery: $(OBJS) Makefile
	$(CC) -kernel $(OBJS) -o $(OBJDIR)/battery

install: all
	cp -f $(OBJDIR)/battery $(IMAGE)/system/drivers/dev/battery

dist: install

clean:
	-rm -f $(OBJS) $(OBJDIR)/battery

