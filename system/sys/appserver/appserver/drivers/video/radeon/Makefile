COPTS   = -I$(TOP)/src $(shell freetype-config --cflags) -c -O2 -Wall

DEVBIN = /system/drivers/appserver/video
CFGDIR = /system/config/drivers/appserver/video
OBJS = radeon.o radeon_accel.o radeon_cursor.o radeon_misc.o radeon_monitor.o radeon_pll.o radeon_rom.o radeon_video.o

OBJDIR	:= objs
OBJS	:= $(addprefix $(OBJDIR)/,$(OBJS))


#
#	rules
#

$(OBJDIR)/%.o : %.c
	$(CC) $(COPTS) $< -o $@

$(OBJDIR)/%.o : %.cpp
	$(CC) $(COPTS) $< -o $@


all :	objs $(OBJDIR)/radeon

deps:

objs:
	mkdir objs

$(OBJDIR)/radeon: $(OBJS)
	$(CXX) -plugin $(OBJS) -o $(OBJDIR)/radeon -lsyllable

install: $(OBJDIR)/radeon radeon.cfg
	cp -f $(OBJDIR)/radeon $(DEVBIN)/radeon
	cp -f radeon.cfg $(CFGDIR)/radeon.cfg

dist: $(OBJDIR)/radeon
	cp -f $(OBJDIR)/radeon $(DIST_DIR)/atheos/sys/drivers/appserver/video/radeon

clean:
	-rm $(OBJDIR)/*

