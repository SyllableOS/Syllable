COPTS   = -I../../../server $(shell freetype-config --cflags) -c -O3 -Wall

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
	$(CC) $(COPTS) $< -o $@


all :	objs $(OBJDIR)/warpspeed

objs:
	mkdir objs

$(OBJDIR)/warpspeed: $(OBJS)
	gcc $(OBJS) -o $(OBJDIR)/warpspeed /system/appserver -lsyllable -lstdc++ -lz
	gcc -plugin $(OBJS) -o $(OBJDIR)/warpspeed -lsyllable -lstdc++

deps:

install: $(OBJDIR)/warpspeed
	cp -f $(OBJDIR)/warpspeed $(DEVBIN)/WarpSpeed

dist: $(OBJDIR)/warpspeed
	cp -f $(OBJDIR)/warpspeed $(DIST_DIR)/atheos/sys/drivers/appserver/decorators/amiga

clean:
	-rm $(OBJS) $(OBJDIR)/warpspeed

