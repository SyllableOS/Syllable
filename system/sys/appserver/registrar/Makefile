CFLAGS   = -Wall -c -O2 -pipe -fexceptions
CPPFLAGS = $(CFLAGS)
AOPTS   = -c

APPBIN = /system

VPATH = ./

CC	= gcc
LL	= ld

OBJS = registrar.o

OBJDIR	:= objs

include $(ATHEOS_SRC)/makefile.rules

all :	$(OBJDIR) $(OBJS) $(OBJDIR)/registrar

$(OBJDIR):
	mkdir $(OBJDIR)

$(OBJDIR)/registrar: $(OBJS)
	gcc  $(OBJS) -lstdc++ -lsyllable -o registrar
#	rescopy mediaserver -r *.png

deps:

install:  $(OBJDIR)/registrar
	
	install -s registrar $(APPBIN)/registrar

dist:  $(OBJDIR)/registrar
	install -s registrar $(DIST_DIR)/atheos/sys/registrar

deps: $(OBJDIR) $(DEPS)

clean:
	rm -f $(OBJS) $(OBJDIR)/*.d registrar

-include $(OBJDIR)/*.d


