CFLAGS	= -Wall -c -I/usr/libungif/include

OBJS	= main.o
EXE	= bmview

CC	= g++
LL	= g++

$(EXE) : $(OBJS)
	$(LL) $(OBJS) -latheos -o $@ 

install: $(EXE)
	cp -f $(EXE) /bin/$(EXE)
	strip --strip-all /bin/$(EXE)

dist: $(EXE)
	cp -f $(EXE) $(DIST_DIR)/atheos/sys/bin/$(EXE)
	strip --strip-all $(DIST_DIR)/atheos/sys/bin/$(EXE)

clean:
	-rm $(OBJS)

.cpp.o:
	$(CC) $(CFLAGS) $< -o $@

main.o : main.cpp
	$(CC) $(CFLAGS) $< -o $@
