CFLAGS   = -Wall -c -O4 -pipe -fexceptions -march=i586 -mcpu=i586 -ffast-math -fomit-frame-pointer
CPPFLAGS = $(CFLAGS)
AOPTS   = -c

APPBIN = /Applications/Media

VPATH = ./

CC	= gcc
LL	= ld

OBJS = seekslider.o MediaPlayer.o

OBJDIR	:= objs

include $(ATHEOS_SRC)/makefile.rules

all :	$(OBJDIR) $(OBJS) $(OBJDIR)/MediaPlayer

$(OBJDIR):
	mkdir $(OBJDIR)

$(OBJDIR)/MediaPlayer: $(OBJS)
	gcc  $(OBJS) -lstdc++ -latheos -lmedia -o MediaPlayer
	rescopy -r MediaPlayer *.png


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






























