include makefile.freetype

SCREENSHOT = n

CFLAGS   = -pipe -O2 -I./$(FREETYPE)/include -c -Wall
CPPFLAGS= $(CFLAGS)
AOPTS   = -c
LIBS    = -latheos

CC	= gcc
LL	= ld

OBJS =	ddriver.o ddline.o vesadrv.o server.o input.o	\
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
	gcc $(OBJS) -o $(OBJDIR)/appserver $(FREETYPE)/objs/libfreetype.a $(LIBS) -lstdc++ -lm

$(OBJDIR)/mini_appserver: $(OBJS)
	gcc $(OBJS) -o $(OBJDIR)/appserver $(FREETYPE)/objs/libfreetype.a $(LIBS) -lstdc++ -lm

install: $(OBJDIR)/appserver
	cp -f $(OBJDIR)/appserver /system/appserver

dist: $(OBJDIR)/appserver
	cp -f $(OBJDIR)/appserver $(DIST_DIR)/atheos/sys/appserver

deps: $(OBJDIR) $(DEPS)
	$(MAKE) -C $(FREETYPE)/

clean:
	rm -f $(OBJS) $(OBJDIR)/*.d $(OBJDIR)/appserver

-include $(OBJDIR)/*.d



