COPTS	= -Wall -c -fPIC

CC	= gcc
LL	= gcc

all: route ifconfig

ifconfig: ifconfig.o
	$(LL) ifconfig.o -o $@

route: route.o
	$(LL) route.o -o $@

deps:

install: $(EXE)
	cp -f ifconfig /bin/ifconfig
	cp -f route /bin/route

dist: $(EXE)
	cp -f ifconfig $(DIST_DIR)/atheos/sys/bin/ifconfig
	cp -f route $(DIST_DIR)/atheos/sys/bin/route

clean:
	-rm ifconfig.o route.o

.cpp.o:
	$(CC) $(COPTS) $< -o $@

main.o : main.cpp
	$(CC) $(COPTS) $< -o $@
