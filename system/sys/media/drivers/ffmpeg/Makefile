CFLAGS   += -c -fexceptions -ffast-math -Iffmpeg/libavcodec -Iffmpeg/libavformat
CXXFLAGS += -c -fexceptions -ffast-math -Iffmpeg/libavcodec -Iffmpeg/libavformat
AOPTS    = -c

DRIVERBIN = /system/drivers/media

VPATH = ./

OBJS	= ffmpeg_demux.o ffmpeg_output.o ffmpeg_codec.o ffmpeg_addon.o
OBJDIR	:= objs

include $(ATHEOS_SRC)/makefile.rules

all :	$(OBJDIR) $(OBJS) $(OBJDIR)/ffmpeg.so

$(OBJDIR):
	mkdir $(OBJDIR)

$(OBJDIR)/ffmpeg.so: $(OBJS)
	$(CXX) -shared -Xlinker "-soname=ffmpeg.so" $(OBJS) -lm -lz -lsyllable -lmedia -lavformat -Lffmpeg/libavcodec -Lffmpeg/libavformat -lavcodec -o $(OBJDIR)/ffmpeg.so


deps: $(LIBS)

install:  $(OBJDIR)/ffmpeg.so
	install -s $(OBJDIR)/ffmpeg.so $(DRIVERBIN)/ffmpeg.so

dist:  $(OBJDIR)/ffmpeg.so
	install -s $(OBJDIR)/ffmpeg.so $(DIST_DIR)/atheos/sys/drivers/media/ffmpeg.so

deps: $(OBJDIR) $(DEPS)

clean:
	rm -f $(OBJS) $(OBJDIR)/*

-include $(OBJDIR)/*.d

