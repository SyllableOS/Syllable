COPTS   = -I../../../server $(shell freetype-config --cflags) -c

DEVBIN = /system/drivers/appserver/decorators
OBJS =	MulvaneDecorator.o

OBJDIR	:= objs
OBJS	:= $(addprefix $(OBJDIR)/,$(OBJS))


#
#	rules
#

$(OBJDIR)/%.o : %.c
	$(CC) $(COPTS) $< -o $@

$(OBJDIR)/%.o : %.cpp
	$(CXX) $(COPTS) $< -o $@


all :	objs $(OBJDIR)/Mulvane

deps:

objs:
	mkdir objs

$(OBJDIR)/Mulvane: $(OBJS)
	$(CXX) -plugin -Xlinker -Bsymbolic $(OBJS) -nodefaultlibs -o $(OBJDIR)/Mulvane

#	$(CXX) $(OBJS) -o $(OBJDIR)/Mulvane ../../../server/objs/appserver -lsyllable -lz
#	$(CXX) -plugin $(OBJS) -o $(OBJDIR)/Mulvane -lsyllable

install: $(OBJDIR)/Mulvane
	cp -f $(OBJDIR)/Mulvane $(IMAGE)/$(DEVBIN)/Mulvane

dist: install

clean:
	-rm $(OBJS) $(OBJDIR)/Mulvane

