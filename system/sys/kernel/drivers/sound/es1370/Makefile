CFLAGS   = -kernel -fno-PIC -O3 -c -Wall -g
AFLAGS   = -c

OBJS := devinit.o es1370.o
DRIVER := es1370

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




all :	objs $(OBJDIR)/es1370

deps:

objs:
	mkdir objs

$(OBJDIR)/es1370: $(OBJS) Makefile
	$(CC) -kernel $(OBJS) -o $(OBJDIR)/es1370

install: all
	cp -f $(OBJDIR)/es1370 /system/drivers/dev/sound/es1370

dist: all
	cp -f $(OBJDIR)/$(DRIVER) $(DIST_DIR)/atheos/sys/drivers/dev/sound/$(DRIVER)

clean:
	-rm -f $(OBJS)
	-rm -f $(OBJDIR)/$(DRIVER)

