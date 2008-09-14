CFLAGS   += -kernel -fno-PIC -c -I. -O2 -Wall
AFLAGS   = -c

CFLAGS += -D__ENABLE_DEBUG__

C_INCLUDE_PATH=$(ATHEOS_INCLUDE_PATH)

OBJS := agp.o


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




all :	objs $(OBJDIR)/agp

objs:
	mkdir objs

$(OBJDIR)/agp: $(OBJS) Makefile
	$(CC) -kernel $(OBJS) -o $(OBJDIR)/agp

install: all
	cp -f $(OBJDIR)/agp $(IMAGE)/system/drivers/dev/bus/agp

dist: install

clean:
	-rm -f $(OBJS) $(OBJDIR)/agp

