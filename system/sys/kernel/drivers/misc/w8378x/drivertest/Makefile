COPTS	= -Wall -c

OBJS	= Main.o
EXE	= drivertest

CC	= gcc
LL	= gcc

$(EXE) : $(OBJS)
	$(LL) $(OBJS) -o $@

install: $(EXE)
	cp -f $(EXE) /bin/$(EXE)
clean:
	-rm $(OBJS)
#.c.o :
#	$(CC) $(COPTS) $< -o $@
.cpp.o:
	$(CC) $(COPTS) $< -o $@

Main.o : Main.cpp
	$(CC) $(COPTS) $< -o $@
