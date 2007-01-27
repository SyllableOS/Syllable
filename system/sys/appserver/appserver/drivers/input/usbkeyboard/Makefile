COPTS   = -I../../../server -c

DEVBIN = /system/drivers/appserver/input
OBJS =	keyboard.o

OBJDIR	:= objs
OBJS	:= $(addprefix $(OBJDIR)/,$(OBJS))


#
#	rules
#

$(OBJDIR)/%.o : %.c
	$(CC) $(COPTS) $< -o $@

$(OBJDIR)/%.o : %.cpp
	$(CXX) $(COPTS) $< -o $@


all :	objs $(OBJDIR)/usbkeyboard

objs:
	mkdir objs

$(OBJDIR)/usbkeyboard: $(OBJS)
	$(CXX) -plugin -Xlinker -Bsymbolic -nodefaultlibs $(OBJS) -o $(OBJDIR)/usbkeyboard -lsyllable

deps:

install: $(OBJDIR)/usbkeyboard
	cp -f $(OBJDIR)/usbkeyboard $(IMAGE)/$(DEVBIN)/usbkeyboard

dist: install

clean:
	-rm $(OBJS) $(OBJDIR)/usbkeyboard

