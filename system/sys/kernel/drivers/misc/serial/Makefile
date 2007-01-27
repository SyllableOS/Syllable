CFLAGS   += -kernel -fno-PIC -c
AFLAGS   = -c

C_INCLUDE_PATH=$(ATHEOS_INCLUDE_PATH)

OBJS := serial.o


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




all :	objs $(OBJDIR)/serial

objs:
	mkdir objs

$(OBJDIR)/serial: $(OBJS) Makefile
	$(CC) -kernel $(OBJS) -o $(OBJDIR)/serial

install: all
	cp -f $(OBJDIR)/serial $(IMAGE)/system/drivers/dev/misc/serial

dist: install

clean:
	-rm -f $(OBJS) $(OBJDIR)/serial

