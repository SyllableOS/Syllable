
CFLAGS   = -kernel -D__BDD_EXTERNAL -fno-PIC -c -O3 -Wall
AFLAGS   = -c


OBJS := bdd.o


OBJDIR	:= objs
OBJS	:= $(addprefix $(OBJDIR)/,$(OBJS))


#
#	rules
#

$(OBJDIR)/%.o : %.c
	$(CC) $(CFLAGS) $< -o $@

$(OBJDIR)/%.o : %.cpp
	$(CC) $(CFLAGS) $< -o $@

$(OBJDIR)/%.o : %.s
	$(CC) $(AFLAGS) -x assembler-with-cpp $< -o $@

$(OBJDIR)/%.o : %.S
	$(CC) $(CFLAGS) -x assembler-with-cpp $< -o $@




all :	objs $(OBJDIR)/bios

objs:
	mkdir objs

$(OBJDIR)/bios: $(OBJS) Makefile
	gcc -kernel $(OBJS) -o $(OBJDIR)/bios

install: all
	cp -f $(OBJDIR)/bios /system/drivers/dev/disk/bios

dist: all
	cp -f $(OBJDIR)/bios $(DIST_DIR)/atheos/sys/drivers/dev/disk/bios

clean:
	-rm -f $(OBJS) $(OBJDIR)/bios



