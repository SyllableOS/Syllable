CFLAGS   = -kernel -fno-PIC -c -O3 -Wall
AFLAGS   = -c


OBJS := devices.o


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




all :	objs $(OBJDIR)/apm

objs:
	mkdir objs

$(OBJDIR)/apm: $(OBJS) Makefile
	$(CC) -kernel $(OBJS) -o $(OBJDIR)/apm

install: all
	cp -f $(OBJDIR)/apm /system/drivers/dev/apm

dist: all
	cp -f $(OBJDIR)/apm $(DIST_DIR)/atheos/sys/drivers/dev/apm

clean:
	-rm -f $(OBJS) $(OBJDIR)/apm

