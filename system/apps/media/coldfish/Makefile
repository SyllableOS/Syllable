CFLAGS   = -Wall -c -O4 -pipe -fexceptions -march=i586 -mcpu=i586 -ffast-math -fomit-frame-pointer
CPPFLAGS = $(CFLAGS)
AOPTS   = -c

APPBIN = /Applications/Media

VPATH = ./

CC	= gcc
LL	= ld

OBJS = lcd.o seekslider.o SelectWin.o ColdFish.o

OBJDIR	:= objs

include $(ATHEOS_SRC)/makefile.rules

all :	$(OBJDIR) $(OBJS) $(OBJDIR)/ColdFish

$(OBJDIR):
	mkdir $(OBJDIR)

$(OBJDIR)/ColdFish: $(OBJS)
	gcc  $(OBJS) -lstdc++ -latheos -lmedia -o ColdFish
	rescopy -r ColdFish *.png


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

































