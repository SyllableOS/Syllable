CC = g++
LL = g++
COPTS = -Wall -c -O3
OBJS = main.o mainwindow.o modifyconnection.o configuration.o detectmsgwindow.o validator.o
EXE = Prefs-Network

$(EXE): $(OBJS)
	$(LL) $(OBJS) -latheos -o $(EXE)
	strip --strip-all $(EXE)

clean:
	-rm $(OBJS)
	-rm $(EXE)

.cpp.o:
	$(CC) $(COPTS) $< -o $@
