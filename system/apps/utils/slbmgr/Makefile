COPTS	= -Wall -c

OBJS	= main.o sysinfopanel.o processpanel.o memview.o cpuview.o performancepanel.o
EXE	= SlbMgr

CC	= g++
LL	= g++

$(EXE) : $(OBJS)
	$(LL) $(OBJS) -latheos -o $@

dist: $(EXE)
	cp -f $(EXE) %(DIST_DIR)/sys/bin/$(EXE)

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


