CFLAGS   = -Wall -c -O3 -pipe -fexceptions -ffast-math -fomit-frame-pointer
CXXFLAGS = $(CFLAGS)
AOPTS   = -c

DRIVERBIN = /system/drivers/media

VPATH = ./

OBJS	= videobitmap.o rgb2rgb.o yuv2rgb.o

OBJDIR	:= objs

include $(ATHEOS_SRC)/makefile.rules

all :	$(OBJDIR) $(OBJS) $(OBJDIR)/videobitmap.so

$(OBJDIR):
	mkdir $(OBJDIR)

$(OBJDIR)/videobitmap.so: $(OBJS)
	$(CXX) -shared -Xlinker "-soname=videobitmap.so" $(OBJS) -lsyllable -lmedia -o $(OBJDIR)/videobitmap.so


deps:

install:  $(OBJDIR)/videobitmap.so
	
	install -s $(OBJDIR)/videobitmap.so $(DRIVERBIN)/videobitmap.so

dist:  $(OBJDIR)/videobitmap.so
	install -s $(OBJDIR)/videobitmap.so $(DIST_DIR)/atheos/sys/drivers/media/videobitmap.so

deps: $(OBJDIR) $(DEPS)

clean:
	rm -f $(OBJS) $(OBJDIR)/*

-include $(OBJDIR)/*.d

