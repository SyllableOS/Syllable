COPTS	= -c -O3 -Wall

OBJS	= main.o tview.o settings.o swindow.o
EXE	= aterm.new

CC	= gcc
LL	= gcc

$(EXE): $(OBJS)
	$(CC) $(OBJS) -lsyllable -lstdc++ -o $(EXE)

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

settings.o:	settings.cpp settings.h
main.o:		main.cpp tview.h messages.h
tview.o:	tview.cpp tview.h swindow.h messages.h
swindow.o:	swindow.cpp swindow.h messages.h


