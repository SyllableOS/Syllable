COPTS	= -Wall -c -I../../kernel/drivers/misc/w8378x/

OBJS	= main.o graphview.o barview.o
EXE	= cpu

CC	= g++
LL	= g++

$(EXE) : $(OBJS)
	$(LL) $(OBJS) -lsyllable -o $@

install: $(EXE)
	strip --strip-all $(EXE)
	cp -f $(EXE) /bin/$(EXE)

dist: $(EXE)
	strip --strip-all $(EXE)
	cp -f $(EXE) $(DIST_DIR)/atheos/sys/bin/$(EXE)

clean:
	-rm $(OBJS)
	-rm $(EXE)

.cpp.o:
	$(CC) $(COPTS) $< -o $@

main.o : main.cpp
	$(CC) $(COPTS) $< -o $@
barview.o : barview.cpp barview.h
	$(CC) $(COPTS) $< -o $@
graphview.o : graphview.cpp graphview.h
	$(CC) $(COPTS) $< -o $@
