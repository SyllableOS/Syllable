CFLAGS   = -Wall -c -O4 -pipe -fexceptions -march=i586 -mcpu=i586 -ffast-math -fomit-frame-pointer
CPPFLAGS = $(CFLAGS)
AOPTS   = -c

APPBIN = /Applications/Media

VPATH = ./

CC	= gcc
LL	= ld

OBJS = lcd.o SelectWin.o ColdFish.o cimagebutton.o

OBJDIR	:= objs

include $(ATHEOS_SRC)/makefile.rules

all :	$(OBJDIR) $(OBJS) $(OBJDIR)/ColdFish

$(OBJDIR):
	mkdir $(OBJDIR)

$(OBJDIR)/ColdFish: $(OBJS)
	gcc  $(OBJS) -lstdc++ -lsyllable -lmedia -o ColdFish
	rescopy -r ColdFish *.png
	rescopy -a ColdFish resources/coldfish.catalog
	rescopy -a ColdFish sv/coldfish.catalog=resources/sv/coldfish.catalog
	rescopy -a ColdFish de/coldfish.catalog=resources/de/coldfish.catalog

deps:

install:  $(OBJDIR)/ColdFish
	mkdir -p $(APPBIN)
	install -s ColdFish $(APPBIN)/ColdFish

dist:  $(OBJDIR)/ColdFish
	mkdir -p $(DIST_DIR)/atheos/Applications/Media
	install -s ColdFish $(DIST_DIR)/atheos/Applications/Media/ColdFish

deps: $(OBJDIR) $(DEPS)

clean:
	rm -f $(OBJS) $(OBJDIR)/*.d ColdFish

-include $(OBJDIR)/*.d




































