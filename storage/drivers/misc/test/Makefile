
CFLAGS   = -kernel -fno-PIC -c -O3 -Wall
AFLAGS   = -c


OBJS := main.o


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




all :	objs $(OBJDIR)/test

objs:
	mkdir objs

$(OBJDIR)/test: $(OBJS) Makefile
	gcc -kernel $(OBJS) -o $(OBJDIR)/test

install: all
	cp -f $(OBJDIR)/test /system/drivers/dev/misc/test

dist: all
	cp -f $(OBJDIR)/test $(DIST_DIR)/atheos/sys/drivers/dev/misc/test

clean:
	-rm $(OBJS)

