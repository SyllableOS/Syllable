CFLAGS   = -kernel -fno-PIC -c -O3 -Wall
AFLAGS   = -c


OBJS := driver.o


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




all :	objs $(OBJDIR)/w8378x

objs:
	mkdir objs

$(OBJDIR)/w8378x: $(OBJS) Makefile
	$(CC) -kernel $(OBJS) -o $(OBJDIR)/w8378x

install: all
	cp -f $(OBJDIR)/w8378x /system/drivers/dev/misc/w8378x

dist: all
	cp -f $(OBJDIR)/w8378x $(DIST_DIR)/atheos/sys/drivers/dev/misc/w8378x

clean:
	-rm -f $(OBJS) $(OBJDIR)/w8378x

