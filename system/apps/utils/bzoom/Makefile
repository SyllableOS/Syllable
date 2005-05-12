COPTS	= -c

OBJS	= main.o gui/zoomview.o
EXE	= BZoom

$(EXE) : $(OBJS)
	$(CXX) $(OBJS) -lsyllable -lm -o $@

dist: $(EXE)
	mkdir $(DIST_DIR)/atheos/Applications/BZoom
	cp -f $(EXE) $(DIST_DIR)/atheos/Applications/BZoom/$(EXE)
	strip --strip-all $(DIST_DIR)/atheos/Applications/BZoom/$(EXE)

upgrade: $(EXE)
	rfs $(EXE) /Applications/BZoom/$(EXE)
clean:
	-rm $(OBJS)
#.c.o :
#	$(CC) $(COPTS) $< -o $@
.cpp.o:
	$(CXX) $(COPTS) $< -o $@

main.o:		main.cpp mywindow.h gui/zoomview.h

