CFLAGS   += -c
CXXFLAGS += -c

EXE = DiskManager

CATALOGS = da de es fr hu nl pl sv

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
	mkdir -p "$(IMAGE)/Applications/System Tools"
	cp -f $(OBJDIR)/$(EXE) "$(IMAGE)/Applications/System Tools/Disk Manager"
	addattrib "$(IMAGE)/Applications/System Tools/Disk Manager" "os::Category" "System Tools"

dist: install

deps: $(OBJDIR) $(DEPS)

clean: catalog-clean
	rm -f $(OBJS) $(OBJDIR)/*.d $(OBJDIR)/$(EXE)

catalog-clean:
	-rm resources/$(EXE).catalog resources/$(EXE).h; \
	for CATALOG in $(CATALOGS);do \
		rm resources/$$CATALOG/$(EXE).catalog; \
	done;

-include $(OBJDIR)/*.d

