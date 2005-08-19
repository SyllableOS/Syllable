COPTS   = -I../../../server $(shell freetype-config --cflags) -c

DEVBIN = /system/drivers/appserver/decorators
OBJS =	RedDecorator.o

OBJDIR	:= objs
OBJS	:= $(addprefix $(OBJDIR)/,$(OBJS))


#
#	rules
#

$(OBJDIR)/%.o : %.c
	$(CC) $(COPTS) $< -o $@

$(OBJDIR)/%.o : %.cpp
	$(CXX) $(COPTS) $< -o $@


all :	objs $(OBJDIR)/Red

deps:

objs:
	mkdir objs

$(OBJDIR)/Red: $(OBJS)
	$(CXX) -plugin -Xlinker -Bsymbolic $(OBJS) -nodefaultlibs -o $(OBJDIR)/Red

install: $(OBJDIR)/Red
	cp -f $(OBJDIR)/Red $(DEVBIN)/Red

dist: $(OBJDIR)/Red
	cp -f $(OBJDIR)/Red $(DIST_DIR)/atheos/sys/drivers/appserver/decorators/Red

clean:
	-rm $(OBJS) $(OBJDIR)/Red

