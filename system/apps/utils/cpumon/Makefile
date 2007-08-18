COPTS	= -c -I../../kernel/drivers/misc/w8378x/

OBJS	= main.o graphview.o barview.o
EXE	= "CPU Monitor"

CATALOGS = da de el es fr hu nl pt-br ru sv

$(EXE) : catalogs $(OBJS)
	$(CXX) $(OBJS) -lsyllable -o "CPU Monitor"
	strip --strip-all $(EXE)
	rescopy -r $(EXE) *.png
	rescopy -a $(EXE) resources/CPU-Monitor.catalog
	for CATALOG in $(CATALOGS);do \
		echo $$CATALOG; \
		rescopy -a $(EXE) $$CATALOG/CPU-Monitor.catalog=resources/$$CATALOG/CPU-Monitor.catalog; \
	done;
	addattrib $(EXE) os::Category "System Tools"

catalogs:
	cd resources; \
	catcomp -c CPU-Monitor.cd; \
	for CATALOG in $(CATALOGS);do \
		echo $$CATALOG; \
		cd $$CATALOG; \
		catcomp -t CPU-Monitor.cd; \
		cd ..; \
	done;

install: $(EXE)
	mkdir -p "$(IMAGE)/Applications/System Tools/System Information"
	cp -f $(EXE) "$(IMAGE)/Applications/System Tools//System Information/"

dist: install

clean: catalog-clean
	-rm $(OBJS)
	-rm $(EXE)

catalog-clean:
	-rm resources/CPU-Monitor.catalog resources/CPU-Monitor.h; \
	for CATALOG in $(CATALOGS);do \
		rm resources/$$CATALOG/CPU-Monitor.catalog; \
	done;

.cpp.o:
	$(CXX) $(COPTS) $< -o $@

main.o : main.cpp
	$(CXX) $(COPTS) $< -o $@
barview.o : barview.cpp barview.h
	$(CXX) $(COPTS) $< -o $@
graphview.o : graphview.cpp graphview.h
	$(CXX) $(COPTS) $< -o $@

