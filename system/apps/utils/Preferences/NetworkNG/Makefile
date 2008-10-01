CXXFLAGS += -c
LIBS = -lsyllable

CATALOGS = ru

OBJS = main.o Hostname.o Nameserver.o Hosts.o App.o MainWin.o ListViewEditRow.o StringWriter.o StringReader.o Interface.o IFaceWin.o

EXE = Network

CATALOG = resources/$(EXE).cd

all: catalogs $(OBJS)
	$(CXX) $(LFLAGS) $(OBJS) -o $(EXE) $(LIBS)
	strip --strip-all $(EXE)
	rescopy $(EXE) -r resources/*.png
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

clean: catalog-clean
	rm $(OBJS) $(EXE)

catalog-clean:
	-rm resources/$(EXE).catalog resources/$(EXE).h; \
	for CATALOG in $(CATALOGS);do \
		rm resources/$$CATALOG/$(EXE).catalog; \
	done;

install: $(EXE)
	install -s $(EXE) $(IMAGE)/Applications/Preferences/$(EXE)

dist: install

%.o: %.cpp %.h
	$(CXX) -c $< $(CXXFLAGS) -o $@
