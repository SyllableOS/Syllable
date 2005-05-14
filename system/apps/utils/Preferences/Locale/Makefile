COPTS = -c
OBJS = main.o mainwindow.o 
EXE = Locale

$(EXE): $(OBJS)
	$(CXX) $(OBJS) -lsyllable -o $(EXE)
	strip --strip-all $(EXE)
	rescopy -r $(EXE) resources/*.png *.png
	rescopy -a $(EXE) resources/languages.db

dist: $(EXE)
	mkdir -p $(DIST_DIR)/atheos/Applications/Preferences
	install -s $(EXE) $(DIST_DIR)/atheos/Applications/Preferences/$(EXE)

clean:
	-rm $(OBJS)
	-rm $(EXE)

.cpp.o:
	$(CXX) $(COPTS) $< -o $@

