COPTS	= -Wall -c -g

OBJS	= server.o
EXE	= nfs

CC	= gcc
LL	= gcc

$(EXE) : $(OBJS)
	$(LL) $(OBJS) -o $@

install: $(EXE)
	cp -f $(EXE) /bin/$(EXE)

dist: $(EXE)
	cp -f $(EXE) $(DIST_DIR)/atheos/sys/bin/$(EXE)

clean:
	-rm $(OBJS)

.cpp.o:
	$(CC) $(COPTS) $< -o $@

server.o: server.c
	$(CC) $(COPTS) $< -o $@
