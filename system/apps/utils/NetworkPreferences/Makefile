CC = g++
LL = g++
COPTS = -Wall -c -O3
OBJS = main.o mainwindow.o modifyconnection.o configuration.o detectmsgwindow.o errorwindow.o validator.o
EXE = prefsnet

$(EXE): $(OBJS)
	$(LL) $(OBJS) -latheos -o $(EXE)
	strip --strip-all $(EXE)
dist:
	mkdir $(DIST_DIR)/Applications/NetworkPreferences
	mkdir $(DIST_DIR)/Applications/NetworkPreferences/bin
	cp -f prefsnet $(DIST_DIR)/Applications/NetworkPreferences/bin

clean:
	-rm $(OBJS)
	-rm $(EXE)

.cpp.o:
	$(CC) $(COPTS) $< -o $@
