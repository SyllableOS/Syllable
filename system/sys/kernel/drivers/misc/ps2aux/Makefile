CFLAGS   += -kernel -fno-PIC -c
AFLAGS   = -c

C_INCLUDE_PATH=$(ATHEOS_INCLUDE_PATH)

OBJS := ps2aux.o


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




all :	objs $(OBJDIR)/ps2aux

objs:
	mkdir objs

$(OBJDIR)/ps2aux: $(OBJS) Makefile
	$(CC) -kernel $(OBJS) -o $(OBJDIR)/ps2aux

install: all
	cp -f $(OBJDIR)/ps2aux $(IMAGE)/system/drivers/dev/misc/ps2aux

dist: install

clean:
	-rm -f $(OBJS) $(OBJDIR)/ps2aux

