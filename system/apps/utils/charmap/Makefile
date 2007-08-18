OPTS   = -c

EXE	= CharMap

CATALOGS = bg da de el es fr hu nl pt-br ru sv

OBJDIR	:= objs
INSTALLDIR := /Applications/CharMap

$(OBJDIR)/%.o : %.c
	$(CC) $(OPTS) $< -o $@

$(OBJDIR)/%.o : %.cpp
	$(CXX) $(OPTS) $< -o $@

OBJS := ./objs/main.o ./objs/mainwin.o ./objs/charmap.o

$(EXE): objs catalogs $(OBJS)
	$(CXX) $(OBJS) -lsyllable -o $(EXE)
	-rescopy -r $(EXE) ./images/*.png
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

deps:
	mkdir objs

clean: catalog-clean
	-rm $(OBJS)
	-rm $(EXE)

catalog-clean:
	-rm resources/$(EXE).catalog resources/$(EXE).h; \
	for CATALOG in $(CATALOGS);do \
		rm resources/$$CATALOG/$(EXE).catalog; \
	done;

install: $(EXE)
	mkdir -p $(IMAGE)/$(INSTALLDIR)
	cp $(EXE) $(IMAGE)/$(INSTALLDIR)

dist: install

