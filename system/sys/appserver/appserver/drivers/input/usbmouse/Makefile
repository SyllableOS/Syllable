COPTS   = -I../../../server  -c -O2 -Wall

DEVBIN = /system/drivers/appserver/input
OBJS =	usbmouse.o

OBJDIR	:= objs
OBJS	:= $(addprefix $(OBJDIR)/,$(OBJS))


#
#	rules
#

$(OBJDIR)/%.o : %.c
	$(CC) $(COPTS) $< -o $@

$(OBJDIR)/%.o : %.cpp
	$(CC) $(COPTS) $< -o $@


all :	objs $(OBJDIR)/usbmouse

objs:
	mkdir objs

$(OBJDIR)/usbmouse: $(OBJS)
	gcc -plugin $(OBJS) -o $(OBJDIR)/usbmouse -lsyllable -lstdc++

deps:

install: $(OBJDIR)/usbmouse
	cp -f $(OBJDIR)/usbmouse $(DEVBIN)/usbmouse

dist: $(OBJDIR)/usbmouse
	cp -f $(OBJDIR)/usbmouse $(DIST_DIR)/atheos/sys/drivers/appserver/input/usbmouse

clean:
	-rm $(OBJS) $(OBJDIR)/usbmouse

