CFLAGS   = -Wall -c -O3 -pipe -fexceptions -ffast-math -fomit-frame-pointer
CXXFLAGS = $(CFLAGS)
AOPTS   = -c

DRIVERBIN = /system/drivers/media

VPATH = ./

OBJS	= yuv2rgb.o rgb2rgb.o videooverlay.o

OBJDIR	:= objs

include $(ATHEOS_SRC)/makefile.rules

all :	$(OBJDIR) $(OBJS) $(OBJDIR)/videooverlay.so

$(OBJDIR):
	mkdir $(OBJDIR)

$(OBJDIR)/videooverlay.so: $(OBJS)
	$(CXX) -shared -Xlinker "-soname=videooverlay.so" $(OBJS) -lsyllable -lmedia -o $(OBJDIR)/videooverlay.so


deps:

install:  $(OBJDIR)/videooverlay.so
	
	install -s $(OBJDIR)/videooverlay.so $(DRIVERBIN)/videooverlay.so

dist:  $(OBJDIR)/videooverlay.so
	install -s $(OBJDIR)/videooverlay.so $(DIST_DIR)/atheos/sys/drivers/media/videooverlay.so

deps: $(OBJDIR) $(DEPS)

clean:
	rm -f $(OBJS) $(OBJDIR)/*

-include $(OBJDIR)/*.d

