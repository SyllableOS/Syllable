COPTS   = -I../../../server $(shell freetype-config --cflags) -c

DEVBIN = /system/drivers/appserver/decorators
OBJS =	drowdecorator.o

OBJDIR	:= objs
OBJS	:= $(addprefix $(OBJDIR)/,$(OBJS))


#
#	rules
#

$(OBJDIR)/%.o : %.c
	$(CC) $(COPTS) $< -o $@

$(OBJDIR)/%.o : %.cpp
	$(CXX) $(COPTS) $< -o $@


all :	objs $(OBJDIR)/Drow

deps:

objs:
	mkdir objs

$(OBJDIR)/Drow: $(OBJS)
	$(CXX) -plugin -Xlinker -Bsymbolic $(OBJS) -nodefaultlibs -o $(OBJDIR)/Drow

install: $(OBJDIR)/Drow
	cp -f $(OBJDIR)/Drow $(IMAGE)/$(DEVBIN)/Drow

dist: install

clean:
	-rm $(OBJS) $(OBJDIR)/Drow

