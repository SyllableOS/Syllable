SCREENSHOT = y

CFLAGS   = -Wall -c -O3 -pipe -fexceptions -ffast-math -fomit-frame-pointer $(shell freetype-config --cflags)
CXXFLAGS= $(CFLAGS)
AOPTS   = -c
LIBS    = -lsyllable

OBJS =  ddriver.o ddline.o vesadrv.o server.o input.o			\
	config.o keyboard.o keymap.o defaultkeymap.o			\
	sapplication.o swindow.o layer.o render.o bitmap.o crdtrans.o	\
	sfont.o fontnode.o wndborder.o desktop.o winselect.o sprite.o	\
	clipboard.o windowdecorator.o defaultdecorator.o

ifeq ($(SCREENSHOT),y)
	OBJS += screenshot.o
	LIBS += -lpng -lz
endif

ifeq ($(SCREENSHOT),n)
	CFLAGS += -D__NO_SCREENSHOT_SUPPORT__=1
endif

OBJDIR	:= objs

include $(ATHEOS_SRC)/makefile.rules

all :	$(OBJDIR) $(OBJDIR)/appserver

$(OBJDIR):
	mkdir $(OBJDIR)

$(OBJDIR)/appserver: $(OBJS)
	$(CXX) $(OBJS) -o $(OBJDIR)/appserver $(shell freetype-config --libs) $(LIBS) -lm

$(OBJDIR)/mini_appserver: $(OBJS)
	$(CXX) $(OBJS) -o $(OBJDIR)/appserver $(shell freetype-config --libs) $(LIBS) -lm

install: $(OBJDIR)/appserver
	cp -f $(OBJDIR)/appserver /system/appserver

dist: $(OBJDIR)/appserver
	cp -f $(OBJDIR)/appserver $(DIST_DIR)/atheos/sys/appserver

deps:

clean:
	rm -f $(OBJS) $(OBJDIR)/*.d $(OBJDIR)/appserver

-include $(OBJDIR)/*.d

