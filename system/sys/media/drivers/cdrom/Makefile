CFLAGS   = -Wall -c -O4 -pipe -fexceptions -march=i586 -mcpu=i586 -ffast-math -fomit-frame-pointer
CPPFLAGS = $(CFLAGS)
AOPTS   = -c

DRIVERBIN = /system/drivers/media

VPATH = ./

CC	= gcc
LL	= ld

OBJS	= cdrom.o

OBJDIR	:= objs

include $(ATHEOS_SRC)/makefile.rules

all :	$(OBJDIR) $(OBJS) $(OBJDIR)/cdrom.so

$(OBJDIR):
	mkdir $(OBJDIR)

$(OBJDIR)/cdrom.so: $(OBJS)
	gcc -shared -Xlinker "-soname=cdrom.so" $(OBJS) -lstdc++ -latheos -lmedia -L/usr/lib/ -o $(OBJDIR)/cdrom.so


deps:

install:  $(OBJDIR)/cdrom.so
	
	install -s $(OBJDIR)/cdrom.so $(DRIVERBIN)/cdrom.so

dist:  $(OBJDIR)/cdrom.so
	install -s $(OBJDIR)/cdrom.so $(DIST_DIR)/atheos/sys/drivers/media/cdrom.so

deps: $(OBJDIR) $(DEPS)

clean:
	rm -f $(OBJS) $(OBJDIR)/*

-include $(OBJDIR)/*.d






















