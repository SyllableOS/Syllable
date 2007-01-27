COPTS   = -I../../../server -c

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
	$(CXX) $(COPTS) $< -o $@


all :	objs $(OBJDIR)/usbmouse

objs:
	mkdir objs

$(OBJDIR)/usbmouse: $(OBJS)
	$(CXX) -plugin -Xlinker -Bsymbolic -nodefaultlibs $(OBJS) -o $(OBJDIR)/usbmouse -lsyllable

deps:

install: $(OBJDIR)/usbmouse
	cp -f $(OBJDIR)/usbmouse $(IMAGE)/$(DEVBIN)/usbmouse

dist: install

clean:
	-rm $(OBJS) $(OBJDIR)/usbmouse

