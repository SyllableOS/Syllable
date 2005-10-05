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


all :	objs $(OBJDIR)/keyboard

objs:
	mkdir objs

$(OBJDIR)/keyboard: $(OBJS)
	$(CXX) -plugin -Xlinker -Bsymbolic -nodefaultlibs $(OBJS) -o $(OBJDIR)/keyboard -lsyllable

deps:

install: $(OBJDIR)/keyboard
	cp -f $(OBJDIR)/keyboard $(DEVBIN)/keyboard

dist: $(OBJDIR)/keyboard
	cp -f $(OBJDIR)/keyboard $(DIST_DIR)/atheos/sys/drivers/appserver/input/keyboard

clean:
	-rm $(OBJS) $(OBJDIR)/keyboard


