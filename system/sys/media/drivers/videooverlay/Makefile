CFLAGS   = -Wall -c -O4 -pipe -fexceptions -march=i586 -mcpu=i586 -ffast-math -fomit-frame-pointer
CPPFLAGS = $(CFLAGS)
AOPTS   = -c

DRIVERBIN = /system/drivers/media

VPATH = ./

CC	= gcc
LL	= ld

OBJS	= yuv2rgb.o rgb2rgb.o videooverlay.o

OBJDIR	:= objs

include $(ATHEOS_SRC)/makefile.rules

all :	$(OBJDIR) $(OBJS) $(OBJDIR)/videooverlay.so

$(OBJDIR):
	mkdir $(OBJDIR)

$(OBJDIR)/videooverlay.so: $(OBJS)
	gcc -shared -Xlinker "-soname=videooverlay.so" $(OBJS) -lstdc++ -lsyllable -lmedia -L/usr/lib/ -o $(OBJDIR)/videooverlay.so


deps:

install:  $(OBJDIR)/videooverlay.so
	
	install -s $(OBJDIR)/videooverlay.so $(DRIVERBIN)/videooverlay.so

dist:  $(OBJDIR)/videooverlay.so
	install -s $(OBJDIR)/videooverlay.so $(DIST_DIR)/atheos/sys/drivers/media/videooverlay.so

deps: $(OBJDIR) $(DEPS)

clean:
	rm -f $(OBJS) $(OBJDIR)/*

-include $(OBJDIR)/*.d


























