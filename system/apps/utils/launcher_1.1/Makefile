COPTS	= -Wall -c -O2
LIBINC  = -Llib -lsyllable -lm -lstdc++

OBJS	= obj/main.o obj/launcher_window.o obj/launcher_prefs.o
LIBOBJS = lib/obj/launcher_plugin.o lib/obj/launcher_bar.o lib/obj/launcher_mime.o
EXE	= bin/launcher
LIB = lib/liblauncher.so

CC	= g++
LL	= g++

INSTDIR = /Applications/Launcher

$(EXE) : $(LIB) $(LIB2) $(OBJS) bin/AddLauncherIcon bin/LBrowser
	( cd plugins && $(MAKE) )
	$(LL) $(OBJS) $(LIBINC) -llauncher -o $@

clean:
	(cd plugins && $(MAKE) clean)
	-rm $(EXE) $(LIB) $(OBJS) $(LIBOBJS) bin/LBrowser bin/AddLauncherIcon obj/LBrowser.o obj/AddLauncherIcon.o

install: $(EXE)
	strip --strip-all $(EXE) $(LIB) bin/AddLauncherIcon bin/LBrowser
	mkdir $(INSTDIR)
	mkdir $(INSTDIR)/bin
	mkdir $(INSTDIR)/lib
	mkdir $(INSTDIR)/include
	mkdir $(INSTDIR)/plugins
	mkdir $(INSTDIR)/icons
	mkdir $(INSTDIR)/mime
	cp README $(INSTDIR)
	cp Developers $(INSTDIR)
	cp COPYING (INSTDIR)
	cp include/COPYING $(INSTDIR)/include
	cp $(EXE) $(INSTDIR)/bin
	cp lib/*.so $(INSTDIR)/lib
	cp include/launcher_plugin.h $(INSTDIR)/include
	cp bin/create_default_config $(INSTDIR)/bin
	cp bin/AddLauncherIcon $(INSTDIR)/bin
	cp bin/LBrowser $(INSTDIR)/bin
	cp default.cfg $(INSTDIR)/default.cfg
	cp icons/* $(INSTDIR)/icons/
	cp -r mime/* $(INSTDIR)/mime/
	(cd plugins && $(MAKE) install)

dist: $(EXE)
	strip --strip-all $(EXE) $(LIB) bin/AddLauncherIcon bin/LBrowser
	mkdir $(DIST_DIR)/atheos$(INSTDIR)
	mkdir $(DIST_DIR)/atheos$(INSTDIR)/bin
	mkdir $(DIST_DIR)/atheos$(INSTDIR)/lib
	mkdir $(DIST_DIR)/atheos$(INSTDIR)/include
	mkdir $(DIST_DIR)/atheos$(INSTDIR)/plugins
	mkdir $(DIST_DIR)/atheos$(INSTDIR)/icons
	mkdir $(DIST_DIR)/atheos$(INSTDIR)/mime
	cp README $(DIST_DIR)/atheos$(INSTDIR)
	cp Developers $(DIST_DIR)/atheos$(INSTDIR)
	cp COPYING $(DIST_DIR)/atheos$(INSTDIR)
	cp include/COPYING $(DIST_DIR)/atheos$(INSTDIR)/include
	cp $(EXE) $(DIST_DIR)/atheos$(INSTDIR)/bin
	cp lib/*.so $(DIST_DIR)/atheos$(INSTDIR)/lib
	cp include/launcher_plugin.h $(DIST_DIR)/atheos$(INSTDIR)/include
	cp bin/create_default_config $(DIST_DIR)/atheos$(INSTDIR)/bin
	cp bin/AddLauncherIcon $(DIST_DIR)/atheos$(INSTDIR)/bin
	cp bin/LBrowser $(DIST_DIR)/atheos$(INSTDIR)/bin
	cp default.cfg $(DIST_DIR)/atheos$(INSTDIR)/default.cfg
	cp icons/* $(DIST_DIR)/atheos$(INSTDIR)/icons/
	cp -r mime/* $(DIST_DIR)/atheos$(INSTDIR)/mime/
	(cd plugins && $(MAKE) dist)


$(LIB): lib/obj $(LIBOBJS)
	$(LL) $(LIBOBJS) $(LIBINC) -shared -Wl,-soname,liblauncher.so -o $(LIB)

lib/obj:
	mkdir lib/obj

lib/obj/launcher_plugin.o: lib/src/launcher_plugin.cpp include/launcher_plugin.h
	$(CC) $(COPTS) lib/src/launcher_plugin.cpp -o $@

lib/obj/launcher_bar.o: include/launcher_bar.h lib/src/launcher_bar.cpp
	$(CC) lib/src/launcher_bar.cpp $(COPTS) -o lib/obj/launcher_bar.o

lib/obj/launcher_mime.o: include/launcher_mime.h lib/src/launcher_mime.cpp
	$(CC) lib/src/launcher_mime.cpp $(COPTS) -o lib/obj/launcher_mime.o

obj/main.o: obj src/main.cpp src/main.h
	$(CC) $(COPTS) src/main.cpp  -o $@

obj:
	mkdir obj

obj/launcher_window.o: src/launcher_window.h src/launcher_window.cpp
	$(CC) $(COPTS) src/launcher_window.cpp -o $@

obj/launcher_prefs.o: src/launcher_prefs.cpp src/launcher_window.h
	$(CC) $(COPTS) src/launcher_prefs.cpp -o $@

obj/AddLauncherIcon.o: src/AddLauncherIcon.cpp
	$(CC) $(COPTS) src/AddLauncherIcon.cpp -o $@

bin/AddLauncherIcon: obj/AddLauncherIcon.o
	$(LL) obj/AddLauncherIcon.o $(LIBINC) -o $@

obj/LBrowser.o: src/LBrowser.cpp src/LBrowser.h
	$(CC) $(COPTS) src/LBrowser.cpp -o $@

bin/LBrowser: obj/LBrowser.o
	$(LL) obj/LBrowser.o $(LIBINC) -llauncher -o $@



