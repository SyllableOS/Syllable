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
	$(CXX) $(OBJS) -o $(OBJDIR)/Mulvane ../../../server/objs/appserver -lsyllable -lz
	$(CXX) -plugin $(OBJS) -o $(OBJDIR)/Mulvane -lsyllable

install: $(OBJDIR)/Mulvane
	cp -f $(OBJDIR)/Mulvane $(DEVBIN)/Mulvane

dist: $(OBJDIR)/Mulvane
	cp -f $(OBJDIR)/Mulvane $(DIST_DIR)/atheos/sys/drivers/appserver/decorators/Mulvane

clean:
	-rm $(OBJS) $(OBJDIR)/Mulvane

