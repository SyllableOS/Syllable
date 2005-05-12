COPTS	= -c

OBJS	= main.o gui/clockview.o
EXE	= aclock

$(EXE) : $(OBJS)
	$(CXX) $(OBJS) -lsyllable -lm -o $@

install: $(EXE)
	cp -f $(EXE) /system/bin/$(EXE)
	cp lcd2n.ttf /system/fonts/
	strip --strip-all /system/bin/$(EXE)

upgrade: $(EXE)
	rfs $(EXE) /system/bin/$(EXE)
deps:

dist: $(EXE)
	cp -f $(EXE) $(DIST_DIR)/atheos/sys/bin/$(EXE)
	cp lcd2n.ttf $(DIST_DIR)/atheos/sys/fonts/
	strip --strip-all $(DIST_DIR)/atheos/sys/bin/$(EXE)

clean:
	-rm $(OBJS)
#.c.o :
#	$(CC) $(COPTS) $< -o $@
.cpp.o:
	$(CXX) $(COPTS) $< -o $@

main.o:	main.cpp gui/clockview.h mywindow.h
gui/clockview.o: gui/clockview.cpp gui/clockview.h

