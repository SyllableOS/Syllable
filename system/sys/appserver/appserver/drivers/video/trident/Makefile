COPTS   = -I$(TOP)/src $(shell freetype-config --cflags) -c -O2 -Wall

DEVBIN = /system/drivers/appserver/video
OBJS =	trident.o

OBJDIR	:= objs
OBJS	:= $(addprefix $(OBJDIR)/,$(OBJS))


#
#	rules
#

$(OBJDIR)/%.o : %.c
	$(CC) $(COPTS) $< -o $@

$(OBJDIR)/%.o : %.cpp
	$(CC) $(COPTS) $< -o $@


all :	objs $(OBJDIR)/trident

deps:

objs:
	mkdir objs

$(OBJDIR)/trident: $(OBJS)
	$(CXX) -plugin $(OBJS) -o $(OBJDIR)/trident -lsyllable

install: $(OBJDIR)/trident
	cp -f $(OBJDIR)/trident $(DEVBIN)/trident

dist: $(OBJDIR)/trident
	cp -f $(OBJDIR)/trident $(DIST_DIR)/atheos/sys/drivers/appserver/video/trident

clean:
	-rm $(OBJS) $(OBJDIR)/trident

