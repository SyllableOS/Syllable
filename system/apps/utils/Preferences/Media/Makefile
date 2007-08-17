COPTS = -c
OBJS = main.o mainwindow.o
EXE = Media

CATALOGS = da de el es fr hu nl pt-br ru sv

$(EXE): catalogs $(OBJS)
	$(CXX) $(OBJS) -lsyllable -lmedia -o $(EXE)
	strip --strip-all $(EXE)
	rescopy $(EXE) -r *.png
	rescopy -a Media resources/Media.catalog
	for CATALOG in $(CATALOGS);do \
		echo $$CATALOG; \
		rescopy -a Media $$CATALOG/Media.catalog=resources/$$CATALOG/Media.catalog; \
	done;

catalogs:
	cd resources; \
	catcomp -c Media.cd; \
	for CATALOG in $(CATALOGS);do \
		echo $$CATALOG; \
		cd $$CATALOG; \
		catcomp -t Media.cd; \
		cd ..; \
	done;

dist: $(EXE)
	install -s $(EXE) $(IMAGE)/Applications/Preferences/$(EXE)

clean:
	-rm $(OBJS)
	-rm $(EXE)
	-rm resources/Media.catalog resources/Media.h; \
	for CATALOG in $(CATALOGS);do \
		rm resources/$$CATALOG/Media.catalog; \
	done;

.cpp.o:
	$(CXX) $(COPTS) $< -o $@

