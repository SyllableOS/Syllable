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
	$(CXX) $(OBJS) -o $(OBJDIR)/WarpSpeed ../../../server/objs/appserver -lsyllable -lz
	$(CXX) -plugin $(OBJS) -o $(OBJDIR)/WarpSpeed -lsyllable

deps:

install: $(OBJDIR)/WarpSpeed
	cp -f $(OBJDIR)/WarpSpeed $(DEVBIN)/WarpSpeed

dist: $(OBJDIR)/WarpSpeed
	cp -f $(OBJDIR)/WarpSpeed $(DIST_DIR)/atheos/sys/drivers/appserver/decorators/WarpSpeed

clean:
	-rm $(OBJS) $(OBJDIR)/WarpSpeed

