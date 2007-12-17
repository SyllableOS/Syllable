COPTS   = -I../../../server $(shell freetype-config --cflags) -c

DEVBIN = /system/drivers/appserver/decorators
OBJS =	BlueDecorator.o

OBJDIR	:= objs
OBJS	:= $(addprefix $(OBJDIR)/,$(OBJS))


#
#	rules
#

$(OBJDIR)/%.o : %.c
	$(CC) $(COPTS) $< -o $@

$(OBJDIR)/%.o : %.cpp
	$(CXX) $(COPTS) $< -o $@


all :	objs $(OBJDIR)/Blue

deps:

objs:
	mkdir objs

$(OBJDIR)/Blue: $(OBJS)
	$(CXX) -plugin -Xlinker -Bsymbolic $(OBJS) -nodefaultlibs -o $(OBJDIR)/Blue


install: $(OBJDIR)/Blue
	cp -f $(OBJDIR)/Blue $(IMAGE)/$(DEVBIN)/Blue

dist: install

clean:
	-rm $(OBJS) $(OBJDIR)/Blue
