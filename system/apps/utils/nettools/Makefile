COPTS	= -Wall -c -fPIC

all: route ifconfig ping

ifconfig: ifconfig.o
	$(CC) ifconfig.o -o $@

route: route.o
	$(CC) route.o -o $@

ping: ping.o
	$(CC) ping.o -o $@

deps:

install: $(EXE)
	cp -f ifconfig /bin/ifconfig
	cp -f route /bin/route
	cp -f ping /bin/ping

dist: $(EXE)
	cp -f ifconfig $(DIST_DIR)/atheos/sys/bin/ifconfig
	cp -f route $(DIST_DIR)/atheos/sys/bin/route
	cp -f ping $(DIST_DIR)/atheos/sys/bin/ping

clean:
	-rm ifconfig.o route.o ping.o

.cpp.o:
	$(CC) $(COPTS) $< -o $@

main.o : main.cpp
	$(CC) $(COPTS) $< -o $@

