COPTS   = -I../../../server $(shell freetype-config --cflags) -c -O3 -Wall

DEVBIN = /system/drivers/appserver/decorators
OBJS =	RedDecorator.o

OBJDIR	:= objs
OBJS	:= $(addprefix $(OBJDIR)/,$(OBJS))


#
#	rules
#

$(OBJDIR)/%.o : %.c
	$(CC) $(COPTS) $< -o $@

$(OBJDIR)/%.o : %.cpp
	$(CC) $(COPTS) $< -o $@


all :	objs $(OBJDIR)/Red

deps:

objs:
	mkdir objs

$(OBJDIR)/Red: $(OBJS)
	$(CXX) $(OBJS) -o $(OBJDIR)/Red /system/appserver -lsyllable -lz
	$(CXX) -plugin $(OBJS) -o $(OBJDIR)/Red -lsyllable

install: $(OBJDIR)/Red
	cp -f $(OBJDIR)/Red $(DEVBIN)/Red

dist: $(OBJDIR)/Red
	cp -f $(OBJDIR)/Red $(DIST_DIR)/atheos/sys/drivers/appserver/decorators/Red

clean:
	-rm $(OBJS) $(OBJDIR)/Red

