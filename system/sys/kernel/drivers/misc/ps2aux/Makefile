
CFLAGS   = -kernel -fno-PIC -c -O3 -Wall
AFLAGS   = -c


OBJS := ps2aux.o


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




all :	objs $(OBJDIR)/ps2aux

objs:
	mkdir objs

$(OBJDIR)/ps2aux: $(OBJS) Makefile
	gcc -kernel $(OBJS) -o $(OBJDIR)/ps2aux

install: all
	cp -f $(OBJDIR)/ps2aux /system/drivers/dev/misc/ps2aux

dist: all
	cp -f $(OBJDIR)/ps2aux $(DIST_DIR)/atheos/sys/drivers/dev/misc/ps2aux

clean:
	-rm $(OBJS)

