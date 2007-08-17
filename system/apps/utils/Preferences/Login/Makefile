CXXFLAGS += -c -I.
OBJS = main.o login_window.o
EXE = Login
TARGET_EXE = Login\ Details

CATALOGS = da de el es fr hu nl pt-br ru sv

#Define TEST if you're testing the Preferences; doesn't overwrite system files
#CXXFLAGS += -DTEST

$(EXE): catalogs $(OBJS)
	$(CXX) $(OBJS) -lsyllable -lcrypt -o $(EXE)
	strip --strip-all $(EXE)
	rescopy $(EXE) -r *.png
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

install: $(EXE)
	install -s $(EXE) $(IMAGE)/Applications/Preferences/$(TARGET_EXE)
	chmod +s $(IMAGE)/Applications/Preferences/$(TARGET_EXE)

dist: install

clean: catalog-clean
	-rm $(OBJS)
	-rm $(EXE)

catalog-clean:
	-rm resources/$(EXE).catalog resources/$(EXE).h; \
	for CATALOG in $(CATALOGS);do \
		rm resources/$$CATALOG/$(EXE).catalog; \
	done;

.cpp.o:
	$(CXX) $(CXXFLAGS) $< -o $@

