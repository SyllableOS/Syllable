CFLAGS   += -c -fexceptions -ffast-math
CXXFLAGS += -c -fexceptions -ffast-math
AOPTS   = -c

CATALOGS = da de es fr hu nl pl pt-br ru sv

APPBIN = /Applications/Media

VPATH = ./

OBJS = seekslider.o cimagebutton.o MediaPlayer.o

OBJDIR	:= objs

include $(ATHEOS_SRC)/makefile.rules

all :	$(OBJDIR) catalogs $(OBJS) $(OBJDIR)/MediaPlayer

$(OBJDIR):
	mkdir $(OBJDIR)

$(OBJDIR)/MediaPlayer: $(OBJS)
	$(CXX) $(OBJS) -lsyllable -lmedia -o MediaPlayer
	rescopy -r MediaPlayer images/*.png
	rescopy -a MediaPlayer resources/mediaplayer.catalog
	for CATALOG in $(CATALOGS);do \
		echo $$CATALOG; \
		rescopy -a MediaPlayer $$CATALOG/mediaplayer.catalog=resources/$$CATALOG/mediaplayer.catalog; \
	done;
	addattrib MediaPlayer os::Category Media

catalogs:
	cd resources; \
	catcomp -c mediaplayer.cd; \
	for CATALOG in $(CATALOGS);do \
		echo $$CATALOG; \
		cd $$CATALOG; \
		catcomp -t mediaplayer.cd; \
		cd ..; \
	done;

deps:

install:  $(OBJDIR)/MediaPlayer
	mkdir -p $(IMAGE)/$(APPBIN)
	cp MediaPlayer $(IMAGE)/$(APPBIN)

dist: install

deps: $(OBJDIR) $(DEPS)

clean:
	rm -f $(OBJS) $(OBJDIR)/*.d MediaPlayer
	rm resources/mediaplayer.catalog resources/mediaplayer.h; \
	for CATALOG in $(CATALOGS);do \
		rm resources/$$CATALOG/mediaplayer.catalog; \
	done;

-include $(OBJDIR)/*.d

