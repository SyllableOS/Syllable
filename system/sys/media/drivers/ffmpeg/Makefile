CFLAGS   = -Wall -c -O4 -pipe -fexceptions -march=i586 -mcpu=i586 -ffast-math -fomit-frame-pointer -Iffmpeg/libavcodec -Iffmpeg/libavformat
CPPFLAGS = $(CFLAGS)
AOPTS   = -c

DRIVERBIN = /system/drivers/media

VPATH = ./

CC	= gcc
LL	= ld

OBJS	= ffmpeg_demux.o ffmpeg_output.o ffmpeg_codec.o ffmpeg_addon.o
LIBS	:= ffmpeg/libavcodec/libavcodec.a ffmpeg/libavformat/libavformat.a

OBJDIR	:= objs

include $(ATHEOS_SRC)/makefile.rules

all :	$(OBJDIR) $(OBJS) $(OBJDIR)/ffmpeg.so

$(OBJDIR):
	mkdir $(OBJDIR)

$(OBJDIR)/ffmpeg.so: $(OBJS) $(LIBS)
	gcc -shared -Xlinker "-soname=ffmpeg.so" $(OBJS) -lm -lz -lstdc++ -lsyllable -lmedia -lavformat -Lffmpeg/libavcodec -Lffmpeg/libavformat -lavcodec -L/usr/lib/ -o $(OBJDIR)/ffmpeg.so

ffmpeg/libavcodec/libavcodec.a:
	make -C ffmpeg

ffmpeg/libavformat/libavformat.a:
	make -C ffmpeg


deps:

install:  $(OBJDIR)/ffmpeg.so
	
	install -s $(OBJDIR)/ffmpeg.so $(DRIVERBIN)/ffmpeg.so

dist:  $(OBJDIR)/ffmpeg.so
	install -s $(OBJDIR)/ffmpeg.so $(DIST_DIR)/atheos/sys/drivers/media/ffmpeg.so

deps: $(OBJDIR) $(DEPS)

clean:
	rm -f $(OBJS) $(OBJDIR)/*

-include $(OBJDIR)/*.d
































