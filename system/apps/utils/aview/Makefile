COPTS	= -Wall -c

OBJS	= main.o loadbitmap.o
EXE	= aview

CC	= g++
LL	= g++

$(EXE) : $(OBJS)
	$(LL) $(OBJS) -latheos -lstdc++ -o $@

install: $(EXE)
	cp -f $(EXE) /system/bin/$(EXE)
	strip --strip-all /system/bin/$(EXE)
upgrade: $(EXE)
	rfs $(EXE) /system/bin/$(EXE)
clean:
	-rm $(OBJS)
	-rm $(EXE)
deps:

dist:
	mkdir $(DIST_DIR)/Applications/AView
	mkdir $(DIST_DIR)/Applications/AView/bin
	cp -f  $(EXE) $(DIST_DIR)/Applications/AView/bin/$(EXE)

#.c.o :
#	$(CC) $(COPTS) $< -o $@
.cpp.o:
	$(CC) $(COPTS) $< -o $@

main.o:		main.cpp mainapp.h window.h
loadbitmap.o:     loadbitmap.h loadbitmap.cpp


