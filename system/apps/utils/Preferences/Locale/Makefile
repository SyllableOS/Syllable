CC = g++
LL = g++
COPTS = -Wall -c -O3
OBJS = main.o mainwindow.o 
EXE = Prefs-Locale

$(EXE): $(OBJS)
	$(LL) $(OBJS) -lsyllable -o $(EXE)
	strip --strip-all $(EXE)
	rescopy -r $(EXE) resources/*.png

dist: $(EXE)
	install -s $(EXE) $(DIST_DIR)/atheos/Applications/Preferences/$(EXE)

clean:
	-rm $(OBJS)
	-rm $(EXE)

.cpp.o:
	$(CC) $(COPTS) $< -o $@
