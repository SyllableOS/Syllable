CFLAGS   = -Wall -c -O4 -pipe -fexceptions -march=i586 -mcpu=i586 -ffast-math -fomit-frame-pointer
CPPFLAGS = $(CFLAGS)
AOPTS   = -c

DRIVERBIN = /system/drivers/media

VPATH = ./

CC	= gcc
LL	= ld

OBJS	= videobitmap.o rgb2rgb.o yuv2rgb.o

OBJDIR	:= objs

include $(ATHEOS_SRC)/makefile.rules

all :	$(OBJDIR) $(OBJS) $(OBJDIR)/videobitmap.so

$(OBJDIR):
	mkdir $(OBJDIR)

$(OBJDIR)/videobitmap.so: $(OBJS)
	gcc -shared -Xlinker "-soname=videobitmap.so" $(OBJS) -lstdc++ -latheos -lmedia -L/usr/lib/ -o $(OBJDIR)/videobitmap.so


deps:

install:  $(OBJDIR)/videobitmap.so
	
	install -s $(OBJDIR)/videobitmap.so $(DRIVERBIN)/videobitmap.so

dist:  $(OBJDIR)/videobitmap.so
	install -s $(OBJDIR)/videobitmap.so $(DIST_DIR)/atheos/sys/drivers/media/videobitmap.so

deps: $(OBJDIR) $(DEPS)

clean:
	rm -f $(OBJS) $(OBJDIR)/*

-include $(OBJDIR)/*.d






























