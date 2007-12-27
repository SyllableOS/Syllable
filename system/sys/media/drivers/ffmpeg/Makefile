CFLAGS   += -c -fexceptions -ffast-math -Iffmpeg/libavcodec -Iffmpeg/libavformat -Iffmpeg/libavutil
CXXFLAGS += -c -fexceptions -ffast-math -Iffmpeg/libavcodec -Iffmpeg/libavformat -Iffmpeg/libavutil -D__STDC_CONSTANT_MACROS
AOPTS    = -c

DRIVERBIN = /system/extensions/media

VPATH = ./

OBJS	= ffmpeg_demux.o ffmpeg_output.o ffmpeg_codec.o ffmpeg_addon.o
OBJDIR	:= objs

include $(ATHEOS_SRC)/makefile.rules

all :	$(OBJDIR) $(OBJS) $(OBJDIR)/ffmpeg.so

$(OBJDIR):
	mkdir $(OBJDIR)

$(OBJDIR)/ffmpeg.so: $(OBJS)
	$(CXX) -shared -Xlinker "-soname=ffmpeg.so" $(OBJS) -lm -lsyllable -lmedia -lavformat -Lffmpeg/libavcodec -Lffmpeg/libavformat -Lffmpeg/libavutil -lavcodec -lavutil -lz -lpthread -o $(OBJDIR)/ffmpeg.so


deps: $(LIBS)

install: $(OBJDIR)/ffmpeg.so
	install -s $(OBJDIR)/ffmpeg.so $(IMAGE)/$(DRIVERBIN)/ffmpeg.so

dist: install

deps: $(OBJDIR) $(DEPS)

clean:
	rm -f $(OBJS) $(OBJDIR)/*

-include $(OBJDIR)/*.d

