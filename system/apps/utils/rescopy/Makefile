CFLAGS   = -pipe -O2 -c -Wall
CPPFLAGS= $(CFLAGS)

EXE = rescopy

CC	= g++
LL	= g++

OBJS = rescopy.o

OBJDIR	:= objs

include $(ATHEOS_SRC)/makefile.rules

all :	$(OBJDIR)/$(EXE) $(OBJDIR)

$(OBJDIR):
	mkdir $(OBJDIR)

$(OBJDIR)/$(EXE): $(OBJS) Makefile
	g++ $(OBJS) -o $(OBJDIR)/$(EXE) -lsyllable

install: $(OBJDIR)/$(EXE)
	cp -af $(OBJDIR)/$(EXE) /bin/$(EXE)

dist: $(OBJDIR)/$(EXE)
	cp -af $(OBJDIR)/$(EXE) $(DIST_DIR)/atheos/sys/bin/$(EXE)

deps: $(OBJDIR) $(DEPS)

clean:
	rm -f $(OBJS) $(OBJDIR)/*.d $(OBJDIR)/$(EXE)

-include $(OBJDIR)/*.d

