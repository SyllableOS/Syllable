COPTS	= -Wall -c -O2 -pipe -fexceptions -march=i586 -ffast-math -fomit-frame-pointer

OBJS	= main.o tview.o settings.o swindow.o
EXE	= aterm.new

$(EXE): $(OBJS)
	$(CXX) $(OBJS) -lsyllable -o $(EXE)
	strip --strip-all $(EXE)
	rescopy -r $(EXE) *.png

install: $(EXE)
	cp -f $(EXE) /bin/aterm

dist: $(EXE)
	cp -f $(EXE) $(DIST_DIR)/atheos/sys/bin/aterm

clean:
	-rm $(OBJS)
	-rm $(EXE)

deps:

%.o : %.c
	$(CC) $(COPTS) $< -o $@

%.o : %.cpp
	$(CXX) $(COPTS) $< -o $@

settings.o:	settings.cpp settings.h
main.o:		main.cpp tview.h messages.h
tview.o:	tview.cpp tview.h swindow.h messages.h
swindow.o:	swindow.cpp swindow.h messages.h

