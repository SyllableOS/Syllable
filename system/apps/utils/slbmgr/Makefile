COPTS	= -c

CATALOGS = da de es fr hu nl ru sv

OBJS = main.o sysinfopanel.o drivespanel.o processpanel.o memview.o cpuview.o performancepanel.o devicespanel.o
EXE	= "System Information"

$(EXE) : catalogs $(OBJS)
	$(CXX) $(OBJS) -lsyllable -o "System Information"
	strip --strip-all $(EXE)
	rescopy -r $(EXE) icons/*.png
	rescopy -a $(EXE) resources/SysInfo.catalog
	for CATALOG in $(CATALOGS);do \
		echo $$CATALOG; \
		rescopy -a $(EXE) $$CATALOG/SysInfo.catalog=resources/$$CATALOG/SysInfo.catalog; \
	done;
	addattrib $(EXE) os::Category "System Tools"

catalogs:
	cd resources; \
	catcomp -c SysInfo.cd; \
	for CATALOG in $(CATALOGS);do \
		echo $$CATALOG; \
		cd $$CATALOG; \
		catcomp -t SysInfo.cd; \
		cd ..; \
	done;

install: $(EXE)
	mkdir -p "$(IMAGE)/Applications/System Tools/System Information"
	cp -f $(EXE) "$(IMAGE)/Applications/System Tools/System Information/"
	ln -sf "/Applications/System Tools/System Information" $(IMAGE)/system/bin/SlbMgr

dist: install

clean:  
	-rm -f $(OBJS)
	-rm -f $(EXE)
	-rm resources/SysInfo.catalog resources/SysInfo.h; \
	for CATALOG in $(CATALOGS);do \
		rm resources/$$CATALOG/SysInfo.catalog; \
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

