CFLAGS   = -Wall -c -O3 -pipe -fexceptions -ffast-math -fomit-frame-pointer
CXXFLAGS = $(CFLAGS)
AOPTS   = -c

DRIVERBIN = /system/drivers/media

VPATH = ./

OBJS	= yuv2rgb.o rgb2rgb.o screen.o

OBJDIR	:= objs

include $(ATHEOS_SRC)/makefile.rules

all :	$(OBJDIR) $(OBJS) $(OBJDIR)/screen.so

$(OBJDIR):
	mkdir $(OBJDIR)

$(OBJDIR)/screen.so: $(OBJS)
	$(CXX) -shared -Xlinker "-soname=screen.so" $(OBJS) -lsyllable -lmedia -o $(OBJDIR)/screen.so


deps:

install:  $(OBJDIR)/screen.so
	
	install -s $(OBJDIR)/screen.so $(DRIVERBIN)/screen.so

dist:  $(OBJDIR)/screen.so
	install -s $(OBJDIR)/screen.so $(DIST_DIR)/atheos/sys/drivers/media/screen.so

deps: $(OBJDIR) $(DEPS)

clean:
	rm -f $(OBJS) $(OBJDIR)/*

-include $(OBJDIR)/*.d

