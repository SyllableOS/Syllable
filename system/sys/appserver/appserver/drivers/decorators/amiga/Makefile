COPTS   = -I../../../server $(shell freetype-config --cflags) -c -O3 -Wall

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
	$(CC) $(COPTS) $< -o $@


all :	objs $(OBJDIR)/Amiga

objs:
	mkdir objs

$(OBJDIR)/Amiga: $(OBJS)
	$(CXX) $(OBJS) -o $(OBJDIR)/Amiga ../../../server/objs/appserver -lsyllable -lz
	$(CXX) -plugin $(OBJS) -o $(OBJDIR)/Amiga -lsyllable

deps:

install: $(OBJDIR)/Amiga
	cp -f $(OBJDIR)/Amiga $(DEVBIN)/Amiga

dist: $(OBJDIR)/Amiga
	cp -f $(OBJDIR)/Amiga $(DIST_DIR)/atheos/sys/drivers/appserver/decorators/Amiga

clean:
	-rm $(OBJS) $(OBJDIR)/Amiga

