COPTS   = -I../../../server  -c -O2 -Wall

DEVBIN = /system/drivers/appserver/input
OBJS =	ps2mouse.o

OBJDIR	:= objs
OBJS	:= $(addprefix $(OBJDIR)/,$(OBJS))


#
#	rules
#

$(OBJDIR)/%.o : %.c
	$(CC) $(COPTS) $< -o $@

$(OBJDIR)/%.o : %.cpp
	$(CC) $(COPTS) $< -o $@


all :	objs $(OBJDIR)/ps2mouse

objs:
	mkdir objs

$(OBJDIR)/ps2mouse: $(OBJS)
	gcc -plugin $(OBJS) -o $(OBJDIR)/ps2mouse -lsyllable -lstdc++

deps:

install: $(OBJDIR)/ps2mouse
	cp -f $(OBJDIR)/ps2mouse $(DEVBIN)/ps2mouse

dist: $(OBJDIR)/ps2mouse
	cp -f $(OBJDIR)/ps2mouse $(DIST_DIR)/atheos/sys/drivers/appserver/input/ps2mouse

clean:
	-rm $(OBJS) $(OBJDIR)/ps2mouse
