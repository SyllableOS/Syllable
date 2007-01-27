CFLAGS   += -kernel -fno-PIC -c
AFLAGS   = -c

C_INCLUDE_PATH=$(ATHEOS_INCLUDE_PATH)

OBJS := devices.o


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




all :	objs $(OBJDIR)/apm

objs:
	mkdir objs

$(OBJDIR)/apm: $(OBJS) Makefile
	$(CC) -kernel $(OBJS) -o $(OBJDIR)/apm

install: all
	cp -f $(OBJDIR)/apm $(IMAGE)/system/drivers/dev/apm

dist: install

clean:
	-rm -f $(OBJS) $(OBJDIR)/apm

