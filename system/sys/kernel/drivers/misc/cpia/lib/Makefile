CFLAGS   = -Wall -c -g -O2
CPPFLAGS = $(CFLAGS)
AOPTS   = -c

LIBBIN = /usr/lib

LIBNAME = libcpia.a

CC	= gcc
LL	= ld

OBJS	= cpiacam.o iosystem.o iocapture.o iocpia.o iovp.o colorspace.o

OBJDIR	:= objs

include $(ATHEOS_SRC)/makefile.rules

all :	$(OBJDIR) $(OBJDIR)/$(LIBNAME)

$(OBJDIR):
	mkdir $(OBJDIR)

$(OBJDIR)/$(LIBNAME): $(OBJS)
	ar -cru $(OBJDIR)/$(LIBNAME) $(OBJS) 

install: $(OBJDIR)/$(LIBNAME)
	cp -f $(OBJDIR)/$(LIBNAME) $(LIBBIN)/$(LIBNAME)

dist: $(OBJDIR)/$(LIBNAME)
	cp -f $(OBJDIR)/$(LIBNAME) $(DIST_DIR)/atheos/sys/$(LIBBIN)/$(LIBNAME)

deps: $(OBJDIR) $(DEPS)

clean:
	rm -f $(OBJS) $(OBJDIR)/*.d

-include $(OBJDIR)/*.d
