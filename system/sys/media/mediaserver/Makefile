CFLAGS   = -Wall -c -O2 -pipe -fexceptions
CPPFLAGS = $(CFLAGS)
AOPTS   = -c

APPBIN = /system

VPATH = ./

CC	= gcc
LL	= ld

OBJS = mediaserver.o mediacontrols.o mixerchannel.o barview.o

OBJDIR	:= objs

include $(ATHEOS_SRC)/makefile.rules

all :	$(OBJDIR) $(OBJS) $(OBJDIR)/mediaserver

$(OBJDIR):
	mkdir $(OBJDIR)

$(OBJDIR)/mediaserver: $(OBJS)
	gcc  $(OBJS) -lstdc++ -lsyllable -lmedia -o mediaserver

deps:

install:  $(OBJDIR)/mediaserver
	
	install -s mediaserver $(APPBIN)/mediaserver

dist:  $(OBJDIR)/mediaserver
	install -s mediaserver $(DIST_DIR)/atheos/sys/mediaserver

deps: $(OBJDIR) $(DEPS)

clean:
	rm -f $(OBJS) $(OBJDIR)/*.d mediaserver

-include $(OBJDIR)/*.d































