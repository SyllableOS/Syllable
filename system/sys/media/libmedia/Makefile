CFLAGS   += -c -fexceptions -ffast-math
CXXFLAGS += -c -fexceptions -ffast-math
AOPTS    = -c

LIBBIN = /system/libraries

LIBNAME = libmedia.so.2

VPATH = ./

OBJS	= codec.o input.o output.o manager.o inputselector.o soundplayer.o stage.o

OBJDIR	:= objs

include $(ATHEOS_SRC)/makefile.rules

all :	$(OBJDIR) $(OBJDIR)/$(LIBNAME)

$(OBJDIR):
	mkdir $(OBJDIR)

$(OBJDIR)/$(LIBNAME): $(OBJS)
	$(CXX) -shared -Xlinker "-soname=$(LIBNAME)" $(OBJS) -lm -lsyllable -o $(OBJDIR)/$(LIBNAME)

deps:

install: $(OBJDIR)/$(LIBNAME)
	install -s $(OBJDIR)/$(LIBNAME) $(IMAGE)/$(LIBBIN)/$(LIBNAME)
	ln -sf $(LIBNAME) $(IMAGE)/$(LIBBIN)/libmedia.so

dist: install

doc:
	-rm -R docs/*
	doxygen libmedia.dox

install-doc:
	-rm -R /atheos/Documentation/media/*
	-mkdir -p /atheos/Documentation/media
	cp -R docs/html/* /atheos/Documentation/media

deps: $(OBJDIR) $(DEPS)

clean:
	rm -f $(OBJS) $(OBJDIR)/*.d $(OBJDIR)/$(LIBNAME)

-include $(OBJDIR)/*.d

