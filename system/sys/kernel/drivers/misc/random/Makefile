CFLAGS   = -kernel -fno-PIC -c -O3 -Wall
AFLAGS   = -c


OBJS := random.o


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




all :	objs $(OBJDIR)/random

objs:
	mkdir objs

$(OBJDIR)/random: $(OBJS) Makefile
	$(CC) -kernel $(OBJS) -o $(OBJDIR)/random

install: all
	cp -f $(OBJDIR)/random /system/drivers/dev/random

dist: all
	cp -f $(OBJDIR)/random $(DIST_DIR)/atheos/sys/drivers/dev/random

clean:
	-rm -f $(OBJS) $(OBJDIR)/random

