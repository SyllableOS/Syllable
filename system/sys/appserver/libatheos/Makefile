CFLAGS   = -Wall -c -O2 -pipe -fexceptions
CPPFLAGS = $(CFLAGS)
AOPTS   = -c

LIBBIN = /system/libs

LIBNAME = libatheos.so.2

VPATH = ./ gui storage util translation

CC	= gcc
LL	= ld

OBJS	= rect.o region.o desktop.o bitmap.o view.o tabview.o			\
	  window.o slider.o scrollbar.o spinner.o frameview.o			\
	  checkbox.o textview.o button.o font.o stdbitmaps.o			\
	  control.o stringview.o menu.o tableview.o sprite.o progressbar.o	\
	  listview.o directoryview.o requesters.o layoutview.o			\
	  radiobutton.o filerequester.o dropdownmenu.o appserverconfig.o	\
	  checkmenu.o splitter.o

OBJS += looper.o handler.o application.o message.o messenger.o messagequeue.o	\
	invoker.o clipboard.o string.o regexp.o variant.o circularbuffer.o	\
	resources.o optionparser.o
OBJS += filereference.o fsnode.o streamableio.o seekableio.o file.o tempfile.o directory.o symlink.o nodemonitor.o path.o
OBJS += translator.o
OBJS += vstubs.o

OBJDIR	:= objs

include $(ATHEOS_SRC)/makefile.rules

all :	$(OBJDIR) $(OBJDIR)/$(LIBNAME)

$(OBJDIR):
	mkdir $(OBJDIR)

$(OBJDIR)/$(LIBNAME): $(OBJS)
	gcc -shared -Xlinker "-soname=$(LIBNAME)" $(OBJS) -lstdc++ -lm -L/usr/lib/ -o $(OBJDIR)/$(LIBNAME)

deps:

install: $(OBJDIR)/$(LIBNAME)
	install -s $(OBJDIR)/$(LIBNAME) $(LIBBIN)/$(LIBNAME)
	ln -sf $(LIBNAME) $(LIBBIN)/libatheos.so

dist: $(OBJDIR)/$(LIBNAME)
	install -s $(OBJDIR)/$(LIBNAME) $(DIST_DIR)/atheos/sys/libs/$(LIBNAME)
	ln -sf $(LIBNAME) $(DIST_DIR)/atheos/sys/libs/libatheos.so

doc:
	-rm -R docs/*
	doxygen libatheos.dox

install-doc:
	-rm -R /atheos/Documentation/gui/*
	cp -R docs/html/* /atheos/Documentation/gui

deps: $(OBJDIR) $(DEPS)

clean:
	rm -f $(OBJS) $(OBJDIR)/*.d

-include $(OBJDIR)/*.d
