CFLAGS   += -c
CXXFLAGS += -c

EXE = pkgmanager

OBJS = pkgmanager.o

OBJDIR	:= objs

include $(ATHEOS_SRC)/makefile.rules

all :	$(OBJDIR) $(OBJDIR)/$(EXE)

$(OBJDIR):
	mkdir $(OBJDIR)

$(OBJDIR)/$(EXE): $(OBJS) Makefile
	$(CXX) $(OBJS) -o $(OBJDIR)/$(EXE) -lsyllable

install: $(OBJDIR)/$(EXE)
	cp -af $(OBJDIR)/$(EXE) /system/bin/$(EXE)

dist: $(OBJDIR)/$(EXE)
	cp -af $(OBJDIR)/$(EXE) $(IMAGE)/system/bin/$(EXE)

deps: $(OBJDIR) $(DEPS)

clean:
	rm -f $(OBJS) $(OBJDIR)/*.d $(OBJDIR)/$(EXE)

-include $(OBJDIR)/*.d

