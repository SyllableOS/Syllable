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




all :	objs $(OBJDIR)/sis3xx

objs:
	mkdir objs

$(OBJDIR)/sis3xx: $(OBJS) Makefile
	$(CC) -kernel $(OBJS) -o $(OBJDIR)/sis3xx

install: all
	mkdir -p /system/drivers/dev/graphics
	cp -f $(OBJDIR)/sis3xx /system/drivers/dev/graphics

dist: all
	mkdir -p $(DIST_DIR)/atheos/sys/drivers/dev/graphics
	cp -f $(OBJDIR)/sis3xx $(DIST_DIR)/atheos/sys/drivers/dev/graphics

clean:
	-rm -f $(OBJS) $(OBJDIR)/sis3xx

