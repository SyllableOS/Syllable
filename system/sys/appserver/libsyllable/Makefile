CFLAGS   += -c -fexceptions
CXXFLAGS += -c -fexceptions -I.
AOPTS    = -c

CATALOGS = da de es hu nl pl pt-br ru sv

LIBBIN = /system/libs

LIBNAME = libsyllable.so.7

VPATH = ./ gui storage util translation gui/colorselector gui/fontrequester

OBJS	= rect.o region.o desktop.o bitmap.o view.o tabview.o			\
	  window.o slider.o scrollbar.o spinner.o frameview.o			\
	  checkbox.o textview.o button.o font.o stdbitmaps.o			\
	  control.o stringview.o menu.o tableview.o sprite.o progressbar.o	\
	  listview.o requesters.o layoutview.o			\
	  radiobutton.o filerequester.o dropdownmenu.o checkmenu.o splitter.o \
	  image.o imageview.o imagebutton.o treeview.o \
      popupmenu.o iconview.o icondirview.o statusbar.o toolbar.o separator.o inputbox.o

OBJS += colorselector.o colorview.o colorwheel.o gradientview.o
OBJS += fontrequester.o fontrequesterview.o

OBJS += appserverconfig.o looper.o handler.o application.o message.o messenger.o messagequeue.o	\
	invoker.o clipboard.o string.o regexp.o variant.o circularbuffer.o	\
	resources.o optionparser.o settings.o thread.o messagefilter.o event.o
OBJS += filereference.o filereference_info.o filereference_rename.o operation_copy.o operation_move.o operation_delete.o fsnode.o streamableio.o \
	seekableio.o file.o tempfile.o directory.o directory_create.o symlink.o nodemonitor.o path.o memfile.o registrar.o volumes.o \
	datetime.o shortcutkey.o catalog.o \
	locale.o
OBJS += translator.o
OBJS += vstubs.o

OBJDIR	:= objs

include $(ATHEOS_SRC)/makefile.rules

all :	$(OBJDIR) catalogs $(OBJDIR)/$(LIBNAME)

$(OBJDIR):
	mkdir $(OBJDIR)

$(OBJDIR)/$(LIBNAME): $(OBJS)
	$(CXX) -shared -Xlinker "-soname=$(LIBNAME)" $(OBJS) -lm -o $(OBJDIR)/$(LIBNAME)

catalogs: catalogs/libsyllable.h

catalogs/libsyllable.h: catalogs/libsyllable.cd
	cd catalogs; \
	catcomp -c libsyllable.cd; \
	for CATALOG in $(CATALOGS); do \
		echo $$CATALOG; \
		cd $$CATALOG; \
		catcomp -t libsyllable.cd; \
		cd ..; \
	done

deps: $(OBJDIR) catalogs $(DEPS)

install: $(OBJDIR)/$(LIBNAME)
	strip --strip-all $(OBJDIR)/$(LIBNAME)
	install -s $(OBJDIR)/$(LIBNAME) $(LIBBIN)/$(LIBNAME)
	ln -sf $(LIBNAME) $(LIBBIN)/libsyllable.so
	cp catalogs/libsyllable.catalog /system/resources/catalogs/
	for CATALOG in $(CATALOGS); do \
		echo $$CATALOG; \
		cp catalogs/$$CATALOG/libsyllable.catalog /system/resources/catalogs/$$CATALOG/; \
	done

dist: $(OBJDIR)/$(LIBNAME)
	install -s $(OBJDIR)/$(LIBNAME) $(DIST_DIR)/atheos/sys/libs/$(LIBNAME)
	ln -sf $(LIBNAME) $(DIST_DIR)/atheos/sys/libs/libsyllable.so
	cp catalogs/libsyllable.catalog $(DIST_DIR)/atheos/sys/resources/catalogs/
	for CATALOG in $(CATALOGS); do \
		echo $$CATALOG; \
		cp catalogs/$$CATALOG/libsyllable.catalog $(DIST_DIR)/atheos/sys/resources/catalogs/$$CATALOG/; \
	done

doc:
	-rm -R docs/*
	doxygen libsyllable.dox

install-doc:
	-rm -R /atheos/Documentation/gui/*
	cp -R docs/html/* /atheos/Documentation/gui

clean:
	-rm -f $(OBJS) $(OBJDIR)/*
	-rm catalogs/libsyllable.catalog catalogs/libsyllable.h
	for CATALOG in $(CATALOGS); do \
		rm catalogs/$$CATALOG/libsyllable.catalog; \
	done

-include $(OBJDIR)/*.d

