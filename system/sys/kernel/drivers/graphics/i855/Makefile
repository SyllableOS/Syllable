CFLAGS   = -kernel -fno-PIC -c -O3 -Wall
AFLAGS   = -c

C_INCLUDE_PATH=$(ATHEOS_INCLUDE_PATH)

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




all :	objs $(OBJDIR)/i855

objs:
	mkdir objs

$(OBJDIR)/i855: $(OBJS) Makefile
	$(CC) -kernel $(OBJS) -o $(OBJDIR)/i855

install: all
	mkdir -p /system/drivers/dev/graphics
	cp -f $(OBJDIR)/i855 /system/drivers/dev/graphics

dist: all
	mkdir -p $(DIST_DIR)/atheos/sys/drivers/dev/graphics
	cp -f $(OBJDIR)/i855 $(DIST_DIR)/atheos/sys/drivers/dev/graphics

clean:
	-rm -f -R $(OBJS) $(OBJDIR)

