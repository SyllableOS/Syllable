CC = g++
LL = g++
COPTS = -Wall -c -O3
OBJS = main.o mainwindow.o
EXE = Prefs-Appearance

$(EXE): $(OBJS)
	$(LL) $(OBJS) -latheos -o $(EXE)
	strip --strip-all $(EXE)

clean:
	-rm $(OBJS)
	-rm $(EXE)

.cpp.o:
	$(CC) $(COPTS) $< -o $@
