COPTS   = -I../../../server $(shell freetype-config --cflags) -c

DEVBIN = /system/drivers/appserver/decorators
OBJS =	WinterDecorator.o

OBJDIR	:= objs
OBJS	:= $(addprefix $(OBJDIR)/,$(OBJS))


#
#	rules
#

$(OBJDIR)/%.o : %.c
	$(CC) $(COPTS) $< -o $@

$(OBJDIR)/%.o : %.cpp
	$(CXX) $(COPTS) $< -o $@


all :	objs $(OBJDIR)/Winter

deps:

objs:
	mkdir objs

$(OBJDIR)/Winter: $(OBJS)
	$(CXX) -plugin -Xlinker -Bsymbolic $(OBJS) -nodefaultlibs -o $(OBJDIR)/Winter

install: $(OBJDIR)/Winter
	cp -f $(OBJDIR)/Winter $(IMAGE)/$(DEVBIN)/Winter

dist: install

clean:
	-rm $(OBJS) $(OBJDIR)/Winter
