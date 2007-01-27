COPTS   = -I../../../server $(shell freetype-config --cflags) -c

DEVBIN = /system/drivers/appserver/decorators
OBJS =	beishdecorator.o gradient.o

OBJDIR	:= objs
OBJS	:= $(addprefix $(OBJDIR)/,$(OBJS))


#
#	rules
#

$(OBJDIR)/%.o : %.c
	$(CC) $(COPTS) $< -o $@

$(OBJDIR)/%.o : %.cpp
	$(CXX) $(COPTS) $< -o $@


all :	objs $(OBJDIR)/Beish

deps:

objs:
	mkdir objs

$(OBJDIR)/Beish: $(OBJS)
	$(CXX) -plugin -Xlinker -Bsymbolic $(OBJS) -nodefaultlibs -o $(OBJDIR)/Beish

install: $(OBJDIR)/Beish
	cp -f $(OBJDIR)/Beish $(IMAGE)/$(DEVBIN)/Beish

dist: install

clean:
	-rm $(OBJS) $(OBJDIR)/Beish

