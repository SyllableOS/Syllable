CFLAGS   += -c -fexceptions -ffast-math
CXXFLAGS += -c -fexceptions -ffast-math
AOPTS    = -c

LIBBIN = /system/libs

LIBNAME = libmedia.so.1

VPATH = ./

OBJS	= codec.o input.o output.o manager.o inputselector.o soundplayer.o

OBJDIR	:= objs

include $(ATHEOS_SRC)/makefile.rules

all :	$(OBJDIR) $(OBJDIR)/$(LIBNAME)

$(OBJDIR):
	mkdir $(OBJDIR)

$(OBJDIR)/$(LIBNAME): $(OBJS)
	$(CXX) -shared -Xlinker "-soname=$(LIBNAME)" $(OBJS) -lm -lsyllable -o $(OBJDIR)/$(LIBNAME)

deps:

install: $(OBJDIR)/$(LIBNAME)
	mkdir -p /system/drivers/media
	mkdir -p /system/sounds
	install -s $(OBJDIR)/$(LIBNAME) $(LIBBIN)/$(LIBNAME)
	ln -sf $(LIBNAME) $(LIBBIN)/libmedia.so

dist: $(OBJDIR)/$(LIBNAME)
	mkdir -p $(DIST_DIR)/atheos/sys/drivers/media
	mkdir -p $(DIST_DIR)/atheos/sys/sounds
	install -s $(OBJDIR)/$(LIBNAME) $(DIST_DIR)/atheos/sys/libs/$(LIBNAME)
	ln -sf $(LIBNAME) $(DIST_DIR)/atheos/sys/libs/libmedia.so

doc:
	-rm -R docs/*
	doxygen libmedia.dox

install-doc:
	-rm -R /atheos/Documentation/media/*
	cp -R docs/html/* /atheos/Documentation/media

deps: $(OBJDIR) $(DEPS)

clean:
	rm -f $(OBJS) $(OBJDIR)/*.d $(OBJDIR)/$(LIBNAME)

-include $(OBJDIR)/*.d

