CFLAGS   = -Wall -c -O3 -pipe -fexceptions -ffast-math -fomit-frame-pointer
CXXFLAGS = $(CFLAGS)
AOPTS   = -c

DRIVERBIN = /system/drivers/media

VPATH = ./

OBJS	= vorbis_input.o vorbis_codec.o

OBJDIR	:= objs

include $(ATHEOS_SRC)/makefile.rules

all :	$(OBJDIR) $(OBJS) $(OBJDIR)/vorbis.so

$(OBJDIR):
	mkdir $(OBJDIR)

$(OBJDIR)/vorbis.so: $(OBJS)
	$(CXX) -shared -Xlinker "-soname=vorbis.so" $(OBJS) -lsyllable -lmedia -lvorbisfile -lvorbis -logg -lm -o $(OBJDIR)/vorbis.so


deps:

install:  $(OBJDIR)/vorbis.so
	
	install -s $(OBJDIR)/vorbis.so $(DRIVERBIN)/vorbis.so

dist:  $(OBJDIR)/vorbis.so
	install -s $(OBJDIR)/vorbis.so $(DIST_DIR)/atheos/sys/drivers/media/vorbis.so

deps: $(OBJDIR) $(DEPS)

clean:
	rm -f $(OBJS) $(OBJDIR)/*

-include $(OBJDIR)/*.d

