COPTS	= -c

OBJS	= main.o gui/zoomview.o
EXE	= BZoom

CATALOGS = bg da de el es fr hu nl pt-br ru sv

$(EXE) : catalogs $(OBJS)
	$(CXX) $(OBJS) -lsyllable -lm -o $@
	rescopy -r $(EXE) *.png
	rescopy -a $(EXE) resources/$(EXE).catalog
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

dist: $(EXE)
	mkdir $(IMAGE)/applications/BZoom
	cp -f $(EXE) $(IMAGE)/applications/BZoom/
	strip --strip-all $(IMAGE)/applications/BZoom/$(EXE)

upgrade: $(EXE)
	rfs $(EXE) /applications/BZoom/$(EXE)

clean: catalog-clean
	-rm $(OBJS)
#.c.o :
#	$(CC) $(COPTS) $< -o $@
.cpp.o:
	$(CXX) $(COPTS) $< -o $@

catalog-clean:
	-rm resources/$(EXE).catalog resources/$(EXE).h; \
	for CATALOG in $(CATALOGS);do \
		rm resources/$$CATALOG/$(EXE).catalog; \
	done;

main.o:		main.cpp mywindow.h gui/zoomview.h
