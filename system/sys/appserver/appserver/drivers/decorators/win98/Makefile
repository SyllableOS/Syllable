COPTS   = -I../../../server $(shell freetype-config --cflags) -c

DEVBIN = /system/drivers/appserver/decorators
OBJS =	Win98Decorator.o

OBJDIR	:= objs
OBJS	:= $(addprefix $(OBJDIR)/,$(OBJS))


#
#	rules
#

$(OBJDIR)/%.o : %.c
	$(CC) $(COPTS) $< -o $@

$(OBJDIR)/%.o : %.cpp
	$(CXX) $(COPTS) $< -o $@


all :	objs $(OBJDIR)/Win98

deps:

objs:
	mkdir objs

$(OBJDIR)/Win98: $(OBJS)
	$(CXX) -plugin -Xlinker -Bsymbolic $(OBJS) -nodefaultlibs -o $(OBJDIR)/Win98

install: $(OBJDIR)/Win98
	cp -f $(OBJDIR)/Win98 $(DEVBIN)/Win98

dist: $(OBJDIR)/Win98
	cp -f $(OBJDIR)/Win98 $(DIST_DIR)/atheos/sys/drivers/appserver/decorators/Win98

clean:
	-rm $(OBJS) $(OBJDIR)/Win98

