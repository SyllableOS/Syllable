CFLAGS   = -Wall -c -O4 -pipe -fexceptions -march=i586 -mcpu=i586 -ffast-math -fomit-frame-pointer
CPPFLAGS = $(CFLAGS)
AOPTS   = -c

DRIVERBIN = /system/drivers/media

VPATH = ./

CC	= gcc
LL	= ld

OBJS	= vorbis_input.o vorbis_codec.o

OBJDIR	:= objs

include $(ATHEOS_SRC)/makefile.rules

all :	$(OBJDIR) $(OBJS) $(OBJDIR)/vorbis.so

$(OBJDIR):
	mkdir $(OBJDIR)

$(OBJDIR)/vorbis.so: $(OBJS)
	gcc -shared -Xlinker "-soname=vorbis.so" $(OBJS) -lstdc++ -lsyllable -lmedia -lvorbisfile -lvorbis -logg -lm -o $(OBJDIR)/vorbis.so


deps:

install:  $(OBJDIR)/vorbis.so
	
	install -s $(OBJDIR)/vorbis.so $(DRIVERBIN)/vorbis.so

dist:  $(OBJDIR)/vorbis.so
	install -s $(OBJDIR)/vorbis.so $(DIST_DIR)/atheos/sys/drivers/media/vorbis.so

deps: $(OBJDIR) $(DEPS)

clean:
	rm -f $(OBJS) $(OBJDIR)/*

-include $(OBJDIR)/*.d









































