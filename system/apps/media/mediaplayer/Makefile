CFLAGS   = -Wall -c -O3 -pipe -fexceptions -ffast-math -fomit-frame-pointer
CXXFLAGS = $(CFLAGS)
AOPTS   = -c

APPBIN = /Applications/Media

VPATH = ./

OBJS = seekslider.o cimagebutton.o MediaPlayer.o

OBJDIR	:= objs

include $(ATHEOS_SRC)/makefile.rules

all :	$(OBJDIR) $(OBJS) $(OBJDIR)/MediaPlayer

$(OBJDIR):
	mkdir $(OBJDIR)

$(OBJDIR)/MediaPlayer: $(OBJS)
	$(CXX) $(OBJS) -lsyllable -lmedia -o MediaPlayer
	rescopy -r MediaPlayer *.png
	rescopy -a MediaPlayer resources/mediaplayer.catalog
	rescopy -a MediaPlayer de/mediaplayer.catalog=resources/de/mediaplayer.catalog

deps:

install:  $(OBJDIR)/MediaPlayer
	mkdir -p $(APPBIN)
	install -s MediaPlayer $(APPBIN)/MediaPlayer

dist:  $(OBJDIR)/MediaPlayer
	mkdir -p $(DIST_DIR)/atheos/Applications/Media
	install -s MediaPlayer $(DIST_DIR)/atheos/Applications/Media/MediaPlayer

deps: $(OBJDIR) $(DEPS)

clean:
	rm -f $(OBJS) $(OBJDIR)/*.d MediaPlayer

-include $(OBJDIR)/*.d

