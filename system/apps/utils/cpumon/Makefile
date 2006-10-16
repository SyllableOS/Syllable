COPTS	= -c -I../../kernel/drivers/misc/w8378x/

OBJS	= main.o graphview.o barview.o
EXE	= cpumon

CATALOGS = da es hu pt-br

$(EXE) : catalogs $(OBJS)
	$(CXX) $(OBJS) -lsyllable -o $@
	rescopy -r $(EXE) resources/$(EXE).catalog
	for CATALOG in $(CATALOGS);do \
		echo $$CATALOG; \
		rescopy -a $(EXE) $$CATALOG/$(EXE).catalog=resources/$$CATALOG/$(EXE).catalog; \
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

install: $(EXE)
	strip --strip-all $(EXE)
	cp -f $(EXE) /bin/$(EXE)

dist: $(EXE)
	strip --strip-all $(EXE)
	cp -f $(EXE) $(DIST_DIR)/atheos/sys/bin/$(EXE)

clean: catalog-clean
	-rm $(OBJS)
	-rm $(EXE)

catalog-clean:
	-rm resources/$(EXE).catalog resources/$(EXE).h; \
	for CATALOG in $(CATALOGS);do \
		rm resources/$$CATALOG/$(EXE).catalog; \
	done;

.cpp.o:
	$(CXX) $(COPTS) $< -o $@

main.o : main.cpp
	$(CXX) $(COPTS) $< -o $@
barview.o : barview.cpp barview.h
	$(CXX) $(COPTS) $< -o $@
graphview.o : graphview.cpp graphview.h
	$(CXX) $(COPTS) $< -o $@

