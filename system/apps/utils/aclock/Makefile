COPTS	= -Wall -c

OBJS	= main.o gui/clockview.o
EXE	= aclock

CC	= gcc
LL	= gcc

$(EXE) : $(OBJS)
	$(LL) $(OBJS) -latheos -lstdc++ -lm -o $@

install: $(EXE)
	cp -f $(EXE) /system/bin/$(EXE)
	cp lcd2n.ttf /system/fonts/
	strip --strip-all /system/bin/$(EXE)
upgrade: $(EXE)
	rfs $(EXE) /system/bin/$(EXE)
deps:
clean:
	-rm $(OBJS)
#.c.o :
#	$(CC) $(COPTS) $< -o $@
.cpp.o:
	$(CC) $(COPTS) $< -o $@

main.o:	main.cpp gui/clockview.h mywindow.h
gui/clockview.o: gui/clockview.cpp gui/clockview.h

