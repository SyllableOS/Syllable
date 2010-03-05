COPTS	= -c -fPIC

all: route ifconfig ping

ifconfig: ifconfig.o
	$(CC) ifconfig.o -o $@

route: route.o
	$(CC) route.o -o $@

ping: ping.o
	$(CC) ping.o -o $@

deps:

install: $(EXE)
	cp -f ifconfig /system/programs/ifconfig
	cp -f route /system/programs/route
	cp -f ping /system/programs/ping

dist: $(EXE)
	cp -f ifconfig $(IMAGE)/system/programs/ifconfig
	cp -f route $(IMAGE)/system/programs/route
	cp -f ping $(IMAGE)/system/programs/ping

clean:
	-rm ifconfig.o route.o ping.o

.cpp.o:
	$(CXX) $(COPTS) $< -o $@

main.o : main.cpp
	$(CXX) $(COPTS) $< -o $@
