CFLAGS   = -kernel -fno-PIC -O3 -c -Wall
AFLAGS   = -c

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
	$(CC) $(CFLAGS) $< -o $@

$(OBJDIR)/%.o : %.s
	$(CC) $(AFLAGS) -x assembler-with-cpp $< -o $@

$(OBJDIR)/%.o : %.S
	$(CC) $(CFLAGS) -x assembler-with-cpp $< -o $@




all :	objs $(OBJDIR)/sbpro

deps:

objs:
	mkdir objs

$(OBJDIR)/sbpro: $(OBJS) Makefile
	gcc -kernel $(OBJS) -o $(OBJDIR)/sbpro

install: all
	cp -f $(OBJDIR)/sbpro /system/drivers/dev/sound/sbpro

dist: all
	cp -f $(OBJDIR)/$(DRIVER) $(DIST_DIR)/atheos/sys/drivers/dev/sound/$(DRIVER)

clean:
	-rm $(OBJS)
	-rm $(OBJDIR)/$(DRIVER)

