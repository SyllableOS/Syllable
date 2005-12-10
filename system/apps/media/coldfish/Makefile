CFLAGS   += -c -fexceptions -ffast-math
CXXFLAGS += -c -fexceptions -ffast-math
AOPTS   = -c

APPBIN = /Applications/Media

VPATH = ./

OBJS = lcd.o SelectWin.o ColdFish.o cimagebutton.o

OBJDIR	:= objs

include $(ATHEOS_SRC)/makefile.rules

all :	$(OBJDIR) $(OBJS) $(OBJDIR)/ColdFish

$(OBJDIR):
	mkdir $(OBJDIR)

$(OBJDIR)/ColdFish: $(OBJS)
	$(CXX) $(OBJS) -lsyllable -lmedia -o ColdFish
	rescopy -r ColdFish *.png
	rescopy -a ColdFish resources/coldfish.catalog
	rescopy -a ColdFish da/coldfish.catalog=resources/da/coldfish.catalog
	rescopy -a ColdFish de/coldfish.catalog=resources/de/coldfish.catalog
	rescopy -a ColdFish nl/coldfish.catalog=resources/nl/coldfish.catalog
	rescopy -a ColdFish pl/coldfish.catalog=resources/pl/coldfish.catalog
	rescopy -a ColdFish sv/coldfish.catalog=resources/sv/coldfish.catalog

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

