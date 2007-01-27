CFLAGS   += -kernel -D__BDD_EXTERNAL -fno-PIC -c
AFLAGS   = -c

C_INCLUDE_PATH=$(ATHEOS_INCLUDE_PATH)

OBJS := bdd.o


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




all :	objs $(OBJDIR)/bios

objs:
	mkdir objs

$(OBJDIR)/bios: $(OBJS) Makefile
	$(CC) -kernel $(OBJS) -o $(OBJDIR)/bios

install: all
	cp -f $(OBJDIR)/bios $(IMAGE)/system/drivers/dev/disk/bios

dist: install

clean:
	-rm -f $(OBJS) $(OBJDIR)/bios

