CFLAGS   += -c -fexceptions -I../audio_shared
CXXFLAGS += -c -fexceptions -I../audio_shared
AOPTS    = -c

DRIVERBIN = /system/extensions/media

VPATH = ./ ../audio_shared

OBJS	= hda_channel.o hda.o

OBJDIR	:= objs

include $(ATHEOS_SRC)/makefile.rules

all :	$(OBJDIR) $(OBJS) $(OBJDIR)/hd_audio.so

$(OBJDIR):
	mkdir $(OBJDIR)

$(OBJDIR)/hd_audio.so: $(OBJS)
	$(CXX) -shared -Xlinker "-soname=hd_audio.so" $(OBJS) -lsyllable -lmedia -o $(OBJDIR)/hd_audio.so
	
deps:

install:  $(OBJDIR)/hd_audio.so
	install -s $(OBJDIR)/hd_audio.so $(DRIVERBIN)/hd_audio.so
	addattrib $(DRIVERBIN)/hd_audio.so os::Category AudioDriver

dist:  $(OBJDIR)/hd_audio.so
	install -s $(OBJDIR)/hd_audio.so $(DIST_DIR)/atheos/sys/extensions/media/hd_audio.so
	addattrib $(DIST_DIR)/atheos/sys/extensions/media/hd_audio.so os::Category AudioDriver

deps: $(OBJDIR) $(DEPS)

clean:
	rm -f $(OBJS) $(OBJDIR)/*

-include $(OBJDIR)/*.d




















