COPTS = -Wall -c

OBJS = main.o loadbitmap.o colorbutton.o
EXE	= dlogin

CC	= g++
LL	= g++

$(EXE) : $(OBJS)
	$(LL) $(OBJS) -latheos -lcrypt -o $@
	rescopy -r $(EXE) syllable.gif 


clean:
	-rm -f $(OBJS)
	-rm -f $(EXE)


dist:
	cp  -f $(EXE) $(DIST_DIR)/atheos/sys/bin

main.o:		main.cpp
loadbitmap.o:	loadbitmap.cpp
colorbutton.o:	colorbutton.cpp










