COPTS	= -O3 -Wall -c

OBJS	= main.o fontpanel.o screenpanel.o windowpanel.o keyboardpanel.o coloredit.o
EXE	= guiprefs

CC	= gcc
LL	= g++
LIBS	= -lm -latheos

$(EXE) : $(OBJS)
	$(LL) $(OBJS) $(LIBS) -o $@

deps:

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
