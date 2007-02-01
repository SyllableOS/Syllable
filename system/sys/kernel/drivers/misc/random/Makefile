CFLAGS   += -kernel -fno-PIC -c
AFLAGS   = -c

C_INCLUDE_PATH=$(ATHEOS_INCLUDE_PATH)

OBJS := random.o


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




all :	objs $(OBJDIR)/random

objs:
	mkdir objs

$(OBJDIR)/random: $(OBJS) Makefile
	$(CC) -kernel $(OBJS) -o $(OBJDIR)/random

install: all
	cp -f $(OBJDIR)/random $(IMAGE)/system/drivers/dev/random

dist: install

clean:
	-rm -f $(OBJS) $(OBJDIR)/random
