COPTS	= -Wall -c

OBJS	= main.o gui/zoomview.o
EXE	= BZoom

CC	= gcc
LL	= gcc

$(EXE) : $(OBJS)
	$(LL) $(OBJS) -latheos -lstdc++ -lm -o $@

dist: $(EXE)
	mkdir $(DIST_DIR)/Applications/BZoom
	cp -f $(EXE) $(DIST_DIR)//Applications/BZoom/$(EXE)
	strip --strip-all $(DIST_DIR)//Applications/BZoom/$(EXE)

upgrade: $(EXE)
	rfs $(EXE) /Applications/BZoom/$(EXE)
clean:
	-rm $(OBJS)
#.c.o :
#	$(CC) $(COPTS) $< -o $@
.cpp.o:
	$(CC) $(COPTS) $< -o $@

main.o:		main.cpp mywindow.h gui/zoomview.h



