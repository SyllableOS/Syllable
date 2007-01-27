COPTS   = -I../../../server $(shell freetype-config --cflags) -c

DEVBIN = /system/drivers/appserver/decorators
OBJS =	nextdecorator.o

OBJDIR	:= objs
OBJS	:= $(addprefix $(OBJDIR)/,$(OBJS))


#
#	rules
#

$(OBJDIR)/%.o : %.c
	$(CC) $(COPTS) $< -o $@

$(OBJDIR)/%.o : %.cpp
	$(CXX) $(COPTS) $< -o $@


all :	objs $(OBJDIR)/Next

objs:
	mkdir objs

$(OBJDIR)/Next: $(OBJS)
	$(CXX) -plugin -Xlinker -Bsymbolic $(OBJS) -nodefaultlibs -o $(OBJDIR)/Next

deps:

install: $(OBJDIR)/Next
	cp -f $(OBJDIR)/Next $(IMAGE)/$(DEVBIN)/Next

dist: install

clean:
	-rm $(OBJS) $(OBJDIR)/Next

