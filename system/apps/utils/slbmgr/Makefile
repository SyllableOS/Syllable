COPTS	= -c

CATALOGS = da de es hu nl sv

OBJS	= main.o sysinfopanel.o drivespanel.o processpanel.o memview.o cpuview.o performancepanel.o devicespanel.o
EXE	= SlbMgr

$(EXE) : catalogs $(OBJS)
	$(CXX) $(OBJS) -lsyllable -o $@
	strip --strip-all $(EXE)
	rescopy -r $(EXE) icons/*.png
	rescopy -a $(EXE) resources/SlbMgr.catalog
	for CATALOG in $(CATALOGS);do \
		echo $$CATALOG; \
		rescopy -a $(EXE) $$CATALOG/SlbMgr.catalog=resources/$$CATALOG/SlbMgr.catalog; \
	done;

catalogs:
	cd resources; \
	catcomp -c SlbMgr.cd; \
	for CATALOG in $(CATALOGS);do \
		echo $$CATALOG; \
		cd $$CATALOG; \
		catcomp -t SlbMgr.cd; \
		cd ..; \
	done;

dist: $(EXE)
	-mkdir -p "$(DIST_DIR)/atheos/Applications/System Tools"
	cp -f $(EXE) "$(DIST_DIR)/atheos/Applications/System Tools/System Information"
	addattrib "$(DIST_DIR)/atheos/Applications/System Tools/System Information" "os::Category" "System Tools"
	ln -sf "/Applications/System Tools/System Information" $(DIST_DIR)/atheos/sys/bin/$(EXE)

clean:  
	-rm -f $(OBJS)
	-rm -f $(EXE)
	-rm resources/SlbMgr.catalog resources/SlbMgr.h; \
	for CATALOG in $(CATALOGS);do \
		rm resources/$$CATALOG/SlbMgr.catalog; \
	done;

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

