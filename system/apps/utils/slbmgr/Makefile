COPTS	= -Wall -c

OBJS	= main.o sysinfopanel.o processpanel.o memview.o cpuview.o performancepanel.o devicespanel.o
EXE	= SlbMgr

CC	= g++
LL	= g++

$(EXE) : $(OBJS)
	$(LL) $(OBJS) -lsyllable -o $@

dist: $(EXE)
	cp -f $(EXE) %(DIST_DIR)/atheos/sys/bin/$(EXE)

clean:  
	-rm -f $(OBJS)
	-rm -f $(EXE)

.cpp.o:
	$(CC) $(COPTS) $< -o $@

main.o : main.cpp
	$(CC) $(COPTS) $< -o $@
	
sysinfopanel.o : sysinfopanel.cpp
	$(CC) $(COPTS) $< -o $@

processpanel.o : processpanel.cpp 
	$(CC) $(COPTS) $< -o $@

memview.o : memview.cpp
	$(CC) $(COPTS) $< -o $@
	
cpuview.o : cpuview.cpp
	$(CC) $(COPTS) $< -o $@

devicespanel.o : devicespanel.cpp
	$(CC) $(COPTS) $< -o $@




