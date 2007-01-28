CFLAGS   += -c -fexceptions -ffast-math
CXXFLAGS += -c -fexceptions -ffast-math
AOPTS    = -c

DRIVERBIN = /system/extensions/media

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

install: $(OBJDIR)/vorbis.so
	install -s $(OBJDIR)/vorbis.so $(IMAGE)/$(DRIVERBIN)/vorbis.so

dist: install

deps: $(OBJDIR) $(DEPS)

clean:
	rm -f $(OBJS) $(OBJDIR)/*

-include $(OBJDIR)/*.d

