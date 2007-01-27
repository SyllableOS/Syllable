COPTS   = -I$(TOP)/src $(shell freetype-config --cflags) -c

DEVBIN = /system/drivers/appserver/video
OBJS = mach64_vesa.o mach64_accel.o mach64_crtc.o mach64_io.o mach64_misc.o mach64_video.o mach64_ct.o mach64.o

OBJDIR	:= objs
OBJS	:= $(addprefix $(OBJDIR)/,$(OBJS))


#
#	rules
#

$(OBJDIR)/%.o : %.c
	$(CC) $(COPTS) $< -o $@

$(OBJDIR)/%.o : %.cpp
	$(CXX) $(COPTS) $< -o $@


all :	objs $(OBJDIR)/mach64

deps:

objs:
	mkdir objs

$(OBJDIR)/mach64: $(OBJS)
	$(CXX) -plugin -Xlinker -Bsymbolic -nodefaultlibs $(OBJS) -o $(OBJDIR)/mach64

install: $(OBJDIR)/mach64
	cp -f $(OBJDIR)/mach64 $(IMAGE)/$(DEVBIN)/mach64

dist: install

clean:
	-rm $(OBJDIR)/*

