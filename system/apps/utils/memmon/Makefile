COPTS	= -Wall -c

OBJS	= main.o
EXE	= memmon

CC	= g++
LL	= g++

$(EXE) : $(OBJS)
	$(LL) $(OBJS) -latheos -o $@

deps:

install: $(EXE)
	cp -f $(EXE) /bin/$(EXE)
	strip --strip-all /bin/$(EXE)

dist: $(EXE)
	cp -f $(EXE) $(DIST_DIR)/atheos/sys/bin/$(EXE)
	strip --strip-all $(DIST_DIR)/atheos/sys/bin/$(EXE)

clean:
	-rm $(OBJS)

.cpp.o:
	$(CC) $(COPTS) $< -o $@

main.o : main.cpp
	$(CC) $(COPTS) $< -o $@
