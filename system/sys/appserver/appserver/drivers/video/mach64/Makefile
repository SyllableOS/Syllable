include ../freetype.makefile

COPTS   = -I$(TOP)/src -I$(FREETYPEINCLUDE) -c -O2 -Wall

DEVBIN = /system/drivers/appserver/video
OBJS = mach64_vesa.o mach64_accel.o mach64_crtc.o mach64_cursor.o mach64_io.o mach64_misc.o mach64_video.o mach64_ct.o mach64.o

OBJDIR	:= objs
OBJS	:= $(addprefix $(OBJDIR)/,$(OBJS))


#
#	rules
#

$(OBJDIR)/%.o : %.c
	$(CC) $(COPTS) $< -o $@

$(OBJDIR)/%.o : %.cpp
	$(CC) $(COPTS) $< -o $@


all :	objs $(OBJDIR)/mach64

deps:

objs:
	mkdir objs

$(OBJDIR)/mach64: $(OBJS)
	g++ -plugin $(OBJS) -o $(OBJDIR)/mach64 -latheos

install: $(OBJDIR)/mach64
	cp -f $(OBJDIR)/mach64 $(DEVBIN)/mach64

dist: $(OBJDIR)/mach64
	cp -f $(OBJDIR)/mach64 $(DIST_DIR)/atheos/sys/dribers/appserver/video/mach64

clean:
	-rm $(OBJDIR)/*



