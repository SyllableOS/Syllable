CFLAGS   = -pipe -O2 -c -Wall
CPPFLAGS= $(CFLAGS)

EXE = pkgmanager

CC	= g++
LL	= g++

OBJS = pkgmanager.o

OBJDIR	:= objs

include $(ATHEOS_SRC)/makefile.rules

all :	$(OBJDIR) $(OBJDIR)/$(EXE)

$(OBJDIR):
	mkdir $(OBJDIR)

$(OBJDIR)/$(EXE): $(OBJS) Makefile
	g++ $(OBJS) -o $(OBJDIR)/$(EXE) -latheos

install: $(OBJDIR)/$(EXE)
	cp -af $(OBJDIR)/$(EXE) /bin/$(EXE)

dist: $(OBJDIR)/$(EXE)
	cp -af $(OBJDIR)/$(EXE) $(DIST_DIR)/atheos/sys/bin/$(EXE)

deps: $(OBJDIR) $(DEPS)

clean:
	rm -f $(OBJS) $(OBJDIR)/*.d $(OBJDIR)/$(EXE)

-include $(OBJDIR)/*.d
