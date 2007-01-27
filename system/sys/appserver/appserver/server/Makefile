SCREENSHOT = n

CFLAGS   += -c -fexceptions -ffast-math $(shell freetype-config --cflags)
CXXFLAGS += -c -fexceptions -ffast-math $(shell freetype-config --cflags)
AOPTS    = -c
LIBS     = -lsyllable

OBJS =  ddriver.o ddline.o ddmemory.o event.o vesadrv.o server.o input.o			\
	config.o keyboard.o keymap.o defaultkeymap.o			\
	sapplication.o swindow.o layer.o toplayer.o render.o bitmap.o crdtrans.o	\
	sfont.o fontnode.o wndborder.o desktop.o winselect.o sprite.o	\
	clipboard.o windowdecorator.o defaultdecorator.o

ifeq ($(SCREENSHOT),y)
	OBJS += screenshot.o
	LIBS += -lpng -lz
endif

ifeq ($(SCREENSHOT),n)
	CXXFLAGS += -D__NO_SCREENSHOT_SUPPORT__=1
endif

OBJDIR	:= objs

include $(ATHEOS_SRC)/makefile.rules

all :	$(OBJDIR) $(OBJDIR)/appserver

$(OBJDIR):
	mkdir $(OBJDIR)

$(OBJDIR)/appserver: $(OBJS)
	$(CXX) $(OBJS) -o $(OBJDIR)/appserver $(shell freetype-config --libs) $(LIBS) -lm
	strip --strip-all $(OBJDIR)/appserver

$(OBJDIR)/mini_appserver: $(OBJS)
	$(CXX) $(OBJS) -o $(OBJDIR)/appserver $(shell freetype-config --libs) $(LIBS) -lm

install: $(OBJDIR)/appserver
	cp -f $(OBJDIR)/appserver $(IMAGE)/system/appserver

dist: install

deps:

clean:
	rm -f $(OBJS) $(OBJDIR)/*.d $(OBJDIR)/appserver

-include $(OBJDIR)/*.d

