COPTS	= -c -O3

OBJS	= main.o tview.o
EXE	= aterm.new

CC	= gcc
LL	= gcc

$(EXE): $(OBJS)
	$(CC) $(OBJS) -latheos -lstdc++ -o $(EXE)

install: $(EXE)
	cp -f $(EXE) /bin/aterm
	strip --strip-all /bin/aterm

dist: $(EXE)
	cp -f $(EXE) $(DIST_DIR)/atheos/sys/bin/aterm
	strip --strip-all $(DIST_DIR)/atheos/sys/bin/aterm

clean:
	-rm $(OBJS)
	-rm $(EXE)

deps:

%.o : %.c
	$(CC) $(COPTS) $< -o $@

%.o : %.cpp
	$(CC) $(COPTS) $< -o $@

main.o:		main.cpp tview.h
tview.o:	tview.cpp tview.h
