COPTS = -c
OBJS = main.o mainwindow.o datepicker.o timepicker.o
EXE = DateTime

CATALOGS = da de el es fr hu lt nl pt-br ru sv

$(EXE): catalogs $(OBJS)
	$(CXX) $(OBJS) -lsyllable -o $(EXE)
	rescopy -r $(EXE) timezone.png timezone.info *.png
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
	install -s $(EXE) $(IMAGE)/Applications/Preferences/$(EXE)

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

