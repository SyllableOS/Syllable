CXXFLAGS += -c
LIBS = -lsyllable

OBJS = main.o Hostname.o Nameserver.o Hosts.o App.o MainWin.o ListViewEditRow.o StringWriter.o StringReader.o Interface.o IFaceWin.o

EXE = Network

CATALOG = resources/$(EXE).cd

all: strings $(OBJS)
	$(CXX) $(LFLAGS) $(OBJS) -o $(EXE) $(LIBS)
	strip --strip-all $(EXE)
	rescopy $(EXE) -r resources/*.png
	rescopy -a $(EXE) resources/$(EXE).catalog

clean:
	rm $(OBJS) $(EXE)

install: $(EXE)
	install -s $(EXE) $(IMAGE)/Applications/Preferences/$(EXE)

dist: install

strings: $(CATALOG)
	catcomp -t $(CATALOG)

%.o: %.cpp %.h
	$(CXX) -c $< $(CXXFLAGS) -o $@
