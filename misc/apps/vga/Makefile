COPTS	= -Wall -c

OBJS	= main.o pdfview.o png.o
EXE	= vga

CC	= gcc
LL	= gcc

$(EXE) : $(OBJS)
	$(LL) $(OBJS) -L/usr/lib  -lstdc++ -lpng -lz -latheos -o $@ 

install: $(EXE)
	cp -f $(EXE) /bin/$(EXE)

dist: $(EXE)
	cp -f $(EXE) $(DIST_DIR)/atheos/sys/bin/$(EXE)

clean:
	-rm $(OBJS)

.cpp.o:
	$(CC) $(COPTS) $< -o $@

main.o : main.cpp
	$(CC) $(COPTS) $< -o $@

pdfview.o : pdfview.cpp
	$(CC) $(COPTS) $< -o $@

png.o : png.cpp
	$(CC) $(COPTS) $< -o $@
