COPTS = -Wall -c
OBJS = main.o mainwindow.o
EXE = Media

$(EXE): $(OBJS)
	$(CXX) $(OBJS) -lsyllable -lmedia -o $(EXE)
	strip --strip-all $(EXE)
	rescopy $(EXE) -r *.png

dist: $(EXE)
	install -s $(EXE) $(DIST_DIR)/atheos/Applications/Preferences/$(EXE)

clean:
	-rm $(OBJS)
	-rm $(EXE)

.cpp.o:
	$(CC) $(COPTS) $< -o $@

