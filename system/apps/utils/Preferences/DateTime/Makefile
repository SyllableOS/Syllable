CC = g++
LL = g++
COPTS = -Wall -c -O3
OBJS = main.o mainwindow.o datepicker.o timepicker.o
EXE = Prefs-DateTime

$(EXE): $(OBJS)
	$(LL) $(OBJS) -latheos -o $(EXE)

clean:
	-rm $(OBJS)
	-rm $(EXE)

.cpp.o:
	$(CC) $(COPTS) $< -o $@
