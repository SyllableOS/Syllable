CFLAGS   = -Wall -c -O4 -pipe -fexceptions -march=i586 -mcpu=i586 -ffast-math -fomit-frame-pointer
CPPFLAGS = $(CFLAGS)
AOPTS   = -c

APPBIN = /Applications/Media

VPATH = ./

CC	= gcc
LL	= ld

OBJS = MediaConverter.o

OBJDIR	:= objs

include $(ATHEOS_SRC)/makefile.rules

all :	$(OBJDIR) $(OBJS) $(OBJDIR)/MediaConverter

$(OBJDIR):
	mkdir $(OBJDIR)

$(OBJDIR)/MediaConverter: $(OBJS)
	gcc  $(OBJS) -lstdc++ -lsyllable -lmedia -o MediaConverter

deps:

install:  $(OBJDIR)/MediaConverter
	mkdir -p $(APPBIN)
	install -s MediaConverter $(APPBIN)/MediaConverter

dist:  $(OBJDIR)/MediaConverter
	mkdir -p $(DIST_DIR)/atheos/Applications/Media
	install -s MediaConverter $(DIST_DIR)/atheos/Applications/Media/MediaConverter

deps: $(OBJDIR) $(DEPS)

clean:
	rm -f $(OBJS) $(OBJDIR)/*.d MediaConverter

-include $(OBJDIR)/*.d






























