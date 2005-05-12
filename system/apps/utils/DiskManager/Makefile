CFLAGS   += -c
CXXFLAGS += -c

EXE = DiskManager

OBJS =	main.o diskview.o partitionview.o

OBJDIR	:= objs

include $(ATHEOS_SRC)/makefile.rules

all :	$(OBJDIR) $(OBJDIR)/$(EXE)

$(OBJDIR):
	mkdir $(OBJDIR)

$(OBJDIR)/$(EXE): $(OBJS)
	$(CXX) $(OBJS) -o $(OBJDIR)/$(EXE) -lsyllable

install: $(OBJDIR)/$(EXE)
	install -s $(OBJDIR)/$(EXE) /bin/$(EXE)

dist: $(OBJDIR)/$(EXE)
	install -s $(OBJDIR)/$(EXE) $(DIST_DIR)/atheos/sys/bin/$(EXE)

deps: $(OBJDIR) $(DEPS)

clean:
	rm -f $(OBJS) $(OBJDIR)/*.d $(OBJDIR)/$(EXE)

-include $(OBJDIR)/*.d

