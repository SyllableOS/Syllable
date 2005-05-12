COPTS = -c
OBJS = main.o mainwindow.o datepicker.o timepicker.o
EXE = DateTime

$(EXE): $(OBJS)
	$(CXX) $(OBJS) -lsyllable -o $(EXE)
	rescopy -r $(EXE) timezone.png timezone.info *.png

dist: $(EXE)
	install -s $(EXE) $(DIST_DIR)/atheos/Applications/Preferences/$(EXE)

clean:
	-rm $(OBJS)
	-rm $(EXE)

.cpp.o:
	$(CXX) $(COPTS) $< -o $@

