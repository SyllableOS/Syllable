COPTS   = -I../../../server $(shell freetype-config --cflags) -c

DEVBIN = /system/drivers/appserver/decorators
OBJS =	warpspeeddecorator.o

OBJDIR	:= objs
OBJS	:= $(addprefix $(OBJDIR)/,$(OBJS))


#
#	rules
#

$(OBJDIR)/%.o : %.c
	$(CC) $(COPTS) $< -o $@

$(OBJDIR)/%.o : %.cpp
	$(CXX) $(COPTS) $< -o $@


all :	objs $(OBJDIR)/WarpSpeed

objs:
	mkdir objs

$(OBJDIR)/WarpSpeed: $(OBJS)
	$(CXX) -plugin -Xlinker -Bsymbolic $(OBJS) -nodefaultlibs -o $(OBJDIR)/WarpSpeed

deps:

install: $(OBJDIR)/WarpSpeed
	cp -f $(OBJDIR)/WarpSpeed $(IMAGE)/$(DEVBIN)/WarpSpeed

dist: install

clean:
	-rm $(OBJS) $(OBJDIR)/WarpSpeed

