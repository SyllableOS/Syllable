CFLAGS   += -c -fexceptions -ffast-math
CXXFLAGS += -c -fexceptions -ffast-math -DUSE_FFMPEG
AOPTS    = -c

DRIVERBIN = /system/extensions/media

VPATH = ./

OBJS	= aac_codec.o

OBJDIR	:= objs

include $(ATHEOS_SRC)/makefile.rules

all :	$(OBJDIR) $(OBJS) $(OBJDIR)/aac.so

$(OBJDIR):
	mkdir $(OBJDIR)

$(OBJDIR)/aac.so: $(OBJS)
	$(CXX) -shared -Xlinker "-soname=aac.so" $(OBJS) -lsyllable -lmedia -lfaad -o $(OBJDIR)/aac.so


deps:

install: $(OBJDIR)/aac.so
	install -s $(OBJDIR)/aac.so $(IMAGE)/$(DRIVERBIN)/aac.so

dist: install

deps: $(OBJDIR) $(DEPS)

clean:
	rm -f $(OBJS) $(OBJDIR)/*

-include $(OBJDIR)/*.d
