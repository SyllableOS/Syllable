CC = g++
LL = g++
COPTS = -Wall -c -O3
OBJS = main.o mainwindow.o modifyconnection.o configuration.o detectmsgwindow.o errorwindow.o validator.o
EXE = prefsnet

$(EXE): $(OBJS)
	$(LL) $(OBJS) -latheos -o $(EXE)
	strip --strip-all $(EXE)

clean:
	-rm $(OBJS)
	-rm $(EXE)

.cpp.o:
	$(CC) $(COPTS) $< -o $@
