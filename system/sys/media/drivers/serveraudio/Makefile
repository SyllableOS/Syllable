CFLAGS   = -Wall -c -O2 -pipe -fexceptions
CPPFLAGS = $(CFLAGS)
AOPTS   = -c

DRIVERBIN = /system/drivers/media

VPATH = ./

CC	= gcc
LL	= ld

OBJS	= server_output.o

OBJDIR	:= objs

include $(ATHEOS_SRC)/makefile.rules

all :	$(OBJDIR) $(OBJS) $(OBJDIR)/serveraudio.so

$(OBJDIR):
	mkdir $(OBJDIR)

$(OBJDIR)/serveraudio.so: $(OBJS)
	gcc -shared -Xlinker "-soname=serveraudio.so" $(OBJS) -lstdc++ -latheos -lmedia -L/usr/lib/ -o $(OBJDIR)/serveraudio.so


deps:

install:  $(OBJDIR)/serveraudio.so
	
	install -s $(OBJDIR)/serveraudio.so $(DRIVERBIN)/serveraudio.so

dist:  $(OBJDIR)/oss.so
	install -s $(OBJDIR)/serveraudio.so $(DIST_DIR)/atheos/sys/drivers/media/serveraudio.so

deps: $(OBJDIR) $(DEPS)

clean:
	rm -f $(OBJS) $(OBJDIR)/*

-include $(OBJDIR)/*.d























