COPTS	= -c

OBJS	= main.o
EXE	= "Memory Monitor"

CATALOGS = da de es fr hu nl pt-br ru sv

$(EXE) : catalogs $(OBJS)
	$(CXX) $(OBJS) -lsyllable -o $@
	rescopy -r $(EXE) resources/MemoryMonitor.catalog
	for CATALOG in $(CATALOGS);do \
		echo $$CATALOG; \
		rescopy -a $(EXE) $$CATALOG/MemoryMonitor.catalog=resources/$$CATALOG/MemoryMonitor.catalog; \
	done;
	addattrib $(EXE) os::Category "System Tools"

catalogs:
	cd resources; \
	catcomp -c MemoryMonitor.cd; \
	for CATALOG in $(CATALOGS);do \
		echo $$CATALOG; \
		cd $$CATALOG; \
		catcomp -t MemoryMonitor.cd; \
		cd ..; \
	done;

deps:

install: $(EXE)
	cp -f $(EXE) "$(IMAGE)/Applications/System Tools/$(EXE)"
	strip --strip-all "$(IMAGE)/Applications/System Tools/$(EXE)"

dist: install

clean: catalog-clean
	-rm $(OBJS)
	-rm $(EXE)

catalog-clean:
	-rm resources/MemoryMonitor.catalog resources/MemoryMonitor.h; \
	for CATALOG in $(CATALOGS);do \
		rm resources/$$CATALOG/MemoryMonitor.catalog; \
	done;

.cpp.o:
	$(CXX) $(COPTS) $< -o $@

main.o : main.cpp
	$(CXX) $(COPTS) $< -o $@
