COPTS   = -I$(TOP)/src $(shell freetype-config --cflags) -c

DEVBIN = /system/drivers/appserver/video
CFGDIR = /system/config/drivers/appserver/video
OBJS = radeon.o radeon_accel.o radeon_misc.o radeon_monitor.o radeon_pll.o radeon_rom.o radeon_video.o

OBJDIR	:= objs
OBJS	:= $(addprefix $(OBJDIR)/,$(OBJS))


#
#	rules
#

$(OBJDIR)/%.o : %.c
	$(CC) $(COPTS) $< -o $@

$(OBJDIR)/%.o : %.cpp
	$(CXX) $(COPTS) $< -o $@


all :	objs $(OBJDIR)/radeon

deps:

objs:
	mkdir objs

$(OBJDIR)/radeon: $(OBJS)
	$(CXX) -plugin -Xlinker -Bsymbolic -nodefaultlibs $(OBJS) -o $(OBJDIR)/radeon

install: $(OBJDIR)/radeon radeon.cfg
	cp -f $(OBJDIR)/radeon $(IMAGE)/$(DEVBIN)/radeon
	cp -f radeon.cfg $(IMAGE)/$(CFGDIR)/radeon.cfg

dist: install

clean:
	-rm $(OBJDIR)/*

