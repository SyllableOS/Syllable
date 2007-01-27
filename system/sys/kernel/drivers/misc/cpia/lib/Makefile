CFLAGS   = -c
CXXFLAGS = $(CFLAGS)
AOPTS   = -c

LIBBIN = /usr/lib

LIBNAME = libcpia.a

OBJS	= cpiacam.o iosystem.o iocapture.o iocpia.o iovp.o colorspace.o

OBJDIR	:= objs

include $(ATHEOS_SRC)/makefile.rules

all :	$(OBJDIR) $(OBJDIR)/$(LIBNAME)

$(OBJDIR):
	mkdir $(OBJDIR)

$(OBJDIR)/$(LIBNAME): $(OBJS)
	ar -cru $(OBJDIR)/$(LIBNAME) $(OBJS) 

install: $(OBJDIR)/$(LIBNAME)
	cp -f $(OBJDIR)/$(LIBNAME) $(IMAGE)/$(LIBBIN)/$(LIBNAME)

dist: install

deps: $(OBJDIR) $(DEPS)

clean:
	rm -f $(OBJS) $(OBJDIR)/*.d

-include $(OBJDIR)/*.d

