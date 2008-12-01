CFLAGS   += -c
CXXFLAGS += -c

EXE = rescopy

OBJS = rescopy.o

OBJDIR	:= objs

include $(ATHEOS_SRC)/makefile.rules

all :	$(OBJDIR)/$(EXE) $(OBJDIR)

$(OBJDIR):
	mkdir $(OBJDIR)

$(OBJDIR)/$(EXE): $(OBJS) Makefile
	$(CXX) $(OBJS) -o $(OBJDIR)/$(EXE) -lsyllable

install: $(OBJDIR)/$(EXE)
	cp -af $(OBJDIR)/$(EXE) $(IMAGE)/system/bin/$(EXE)

dist: install

deps: $(OBJDIR) $(DEPS)

clean:
	rm -f $(OBJS) $(OBJDIR)/*.d $(OBJDIR)/$(EXE)

-include $(OBJDIR)/*.d

