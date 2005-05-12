COPTS	= -c

OBJS	= main.o sysinfopanel.o drivespanel.o processpanel.o memview.o cpuview.o performancepanel.o devicespanel.o
EXE	= SlbMgr

$(EXE) : $(OBJS)
	$(CXX) $(OBJS) -lsyllable -o $@

dist: $(EXE)
	cp -f $(EXE) $(DIST_DIR)/atheos/sys/bin/$(EXE)

clean:  
	-rm -f $(OBJS)
	-rm -f $(EXE)

.cpp.o:
	$(CXX) $(COPTS) $< -o $@

main.o : main.cpp
	$(CXX) $(COPTS) $< -o $@
	
sysinfopanel.o : sysinfopanel.cpp
	$(CXX) $(COPTS) $< -o $@

drivespanel.o : drivespanel.cpp
	$(CXX) $(COPTS) $< -o $@

processpanel.o : processpanel.cpp 
	$(CXX) $(COPTS) $< -o $@

memview.o : memview.cpp
	$(CXX) $(COPTS) $< -o $@
	
cpuview.o : cpuview.cpp
	$(CXX) $(COPTS) $< -o $@

devicespanel.o : devicespanel.cpp
	$(CXX) $(COPTS) $< -o $@

