CFLAGS   += -c -fexceptions -I../audio_shared
CXXFLAGS += -c -fexceptions -I../audio_shared
AOPTS    = -c

DRIVERBIN = /system/extensions/media

VPATH = ./ ../audio_shared

OBJS	= ac97.o ac97channel.o

OBJDIR	:= objs

include $(ATHEOS_SRC)/makefile.rules

all :	$(OBJDIR) $(OBJS) $(OBJDIR)/ac97_audio.so

$(OBJDIR):
	mkdir $(OBJDIR)

$(OBJDIR)/ac97_audio.so: $(OBJS)
	$(CXX) -shared -Xlinker "-soname=ac97_audio.so" $(OBJS) -lsyllable -lmedia -o $(OBJDIR)/ac97_audio.so
	
deps:

install: $(OBJDIR)/ac97_audio.so
	install -s $(OBJDIR)/ac97_audio.so $(IMAGE)/$(DRIVERBIN)/ac97_audio.so
	addattrib $(IMAGE)/$(DRIVERBIN)/ac97_audio.so os::Category AudioDriver

dist: install

deps: $(OBJDIR) $(DEPS)

clean:
	rm -f $(OBJS) $(OBJDIR)/*

-include $(OBJDIR)/*.d

