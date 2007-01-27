COPTS   = -I../../../server $(shell freetype-config --cflags) -c

DEVBIN = /system/drivers/appserver/decorators
OBJS =	amigadecorator.o

OBJDIR	:= objs
OBJS	:= $(addprefix $(OBJDIR)/,$(OBJS))


#
#	rules
#

$(OBJDIR)/%.o : %.c
	$(CC) $(COPTS) $< -o $@

$(OBJDIR)/%.o : %.cpp
	$(CXX) $(COPTS) $< -o $@


all :	objs $(OBJDIR)/Amiga

objs:
	mkdir objs

$(OBJDIR)/Amiga: $(OBJS)
	$(CXX) -plugin -Xlinker -Bsymbolic $(OBJS) -nodefaultlibs -o $(OBJDIR)/Amiga

deps:

install: $(OBJDIR)/Amiga
	cp -f $(OBJDIR)/Amiga $(IMAGE)/$(DEVBIN)/Amiga

dist: install

clean:
	-rm $(OBJS) $(OBJDIR)/Amiga

