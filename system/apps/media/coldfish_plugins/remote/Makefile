CFLAGS   = -Wall -c -O2 -pipe -fexceptions
CPPFLAGS = $(CFLAGS)
AOPTS   = -c

DRIVERBIN = /Applications/Media/Plugins

VPATH = ./

CC	= gcc
LL	= ld

OBJS	= Remote.o

OBJDIR	:= objs

include $(ATHEOS_SRC)/makefile.rules

all :	$(OBJDIR) $(OBJS) $(OBJDIR)/Remote

$(OBJDIR):
	mkdir $(OBJDIR)

$(OBJDIR)/Remote: $(OBJS)
	gcc -shared -Xlinker "-soname=Remote" $(OBJS) -lstdc++ -lsyllable -L/usr/lib/ -o $(OBJDIR)/Remote

deps:

install:  $(OBJDIR)/Remote
	mkdir -p $(DRIVERBIN)
	install -s $(OBJDIR)/Remote $(DRIVERBIN)/Remote

dist:  $(OBJDIR)/Blur
	mkdir -p $(DIST_DIR)/atheos/Applications/Media/Plugins/
	install -s $(OBJDIR)/Remote $(DIST_DIR)/atheos/Applications/Media/Plugins/

deps: $(OBJDIR) $(DEPS)

clean:
	rm -f $(OBJS) $(OBJDIR)/*

-include $(OBJDIR)/*.d



























