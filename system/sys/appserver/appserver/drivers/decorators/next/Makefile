COPTS   = -I../../../server $(shell freetype-config --cflags) -c -O3 -Wall

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
	$(CC) $(COPTS) $< -o $@


all :	objs $(OBJDIR)/Next

objs:
	mkdir objs

$(OBJDIR)/Next: $(OBJS)
	$(CXX) $(OBJS) -o $(OBJDIR)/Next /system/appserver -lsyllable -lz
	$(CXX) -plugin $(OBJS) -o $(OBJDIR)/Next -lsyllable

deps:

install: $(OBJDIR)/Next
	cp -f $(OBJDIR)/Next $(DEVBIN)/Next

dist: $(OBJDIR)/Next
	cp -f $(OBJDIR)/Next $(DIST_DIR)/atheos/sys/drivers/appserver/decorators/Next

clean:
	-rm $(OBJS) $(OBJDIR)/Next

