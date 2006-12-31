CFLAGS   += -c
CXXFLAGS += -c

EXE = DiskManager

CATALOGS = da de es hu nl sv

OBJS =	main.o diskview.o partitionview.o

OBJDIR	:= objs

include $(ATHEOS_SRC)/makefile.rules

all :	$(OBJDIR) $(OBJDIR)/$(EXE)

$(OBJDIR):
	mkdir $(OBJDIR)

$(OBJDIR)/$(EXE): catalogs $(OBJS)
	$(CXX) $(OBJS) -o $(OBJDIR)/$(EXE) -lsyllable
	strip --strip-all $(OBJDIR)/$(EXE)
	rescopy -r $(OBJDIR)/$(EXE) icons/*.png
	rescopy -a $(OBJDIR)/$(EXE) resources/$(EXE).catalog
	for CATALOG in $(CATALOGS);do \
		echo $$CATALOG; \
		rescopy -a $(OBJDIR)/$(EXE) $$CATALOG/$(EXE).catalog=resources/$$CATALOG/$(EXE).catalog; \
	done;

catalogs:
	cd resources; \
	catcomp -c $(EXE).cd; \
	for CATALOG in $(CATALOGS);do \
		echo $$CATALOG; \
		cd $$CATALOG; \
		catcomp -t $(EXE).cd; \
		cd ..; \
	done;

install: $(OBJDIR)/$(EXE)
	-mkdir -p "/Applications/System Tools"
	cp -f $(OBJDIR)/$(EXE) "/Applications/System Tools/Disk Manager"
	addattrib "/Applications/System Tools/Disk Manager" "os::Category" "System Tools"

dist: $(OBJDIR)/$(EXE)
	-mkdir -p "$(DIST_DIR)/atheos/Applications/System Tools"
	cp -f $(OBJDIR)/$(EXE) "$(DIST_DIR)/atheos/Applications/System Tools/Disk Manager"
	addattrib "$(DIST_DIR)/atheos/Applications/System Tools/Disk Manager" "os::Category" "System Tools"

deps: $(OBJDIR) $(DEPS)

clean: catalog-clean
	rm -f $(OBJS) $(OBJDIR)/*.d $(OBJDIR)/$(EXE)

catalog-clean:
	-rm resources/$(EXE).catalog resources/$(EXE).h; \
	for CATALOG in $(CATALOGS);do \
		rm resources/$$CATALOG/$(EXE).catalog; \
	done;

-include $(OBJDIR)/*.d

