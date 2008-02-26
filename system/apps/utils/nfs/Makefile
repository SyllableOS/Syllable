COPTS	= -c

OBJS	= server.o
EXE	= network-fs-server

$(EXE) : $(OBJS)
	$(CC) $(OBJS) -o $@

clean:
	-rm $(OBJS)

install: $(EXE)
	cp -f $(EXE) $(IMAGE)/system/bin/

dist: install

.cpp.o:
	$(CC) $(COPTS) $< -o $@

server.o: server.c
	$(CC) $(COPTS) $< -o $@
