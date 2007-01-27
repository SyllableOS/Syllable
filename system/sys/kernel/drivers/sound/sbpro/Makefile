CFLAGS   += -kernel -fno-PIC -c
AFLAGS   = -c

C_INCLUDE_PATH=$(ATHEOS_INCLUDE_PATH)

OBJS := sbpro.o
DRIVER := sbpro

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




all :	objs $(OBJDIR)/sbpro

deps:

objs:
	mkdir objs

$(OBJDIR)/sbpro: $(OBJS) Makefile
	$(CC) -kernel $(OBJS) -o $(OBJDIR)/sbpro

install: all
	cp -f $(OBJDIR)/sbpro $(IMAGE)/system/drivers/dev/audio/sbpro

dist: install

clean:
	-rm -f $(OBJS)
	-rm -f $(OBJDIR)/$(DRIVER)

