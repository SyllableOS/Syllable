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
	-strip --strip-all $(EXE) $(LIB) bin/AddLauncherIcon bin/LBrowser
	-mkdir $(DISTDIR)$(INSTDIR)
	-mkdir $(DISTDIR)$(INSTDIR)/bin
	-mkdir $(DISTDIR)$(INSTDIR)/lib
	-mkdir $(DISTDIR)$(INSTDIR)/include
	-mkdir $(DISTDIR)$(INSTDIR)/plugins
	-mkdir $(DISTDIR)$(INSTDIR)/icons
	-mkdir $(DISTDIR)$(INSTDIR)/mime
	cp README $(DISTDIR)$(INSTDIR)
	cp Developers $(DISTDIR)$(INSTDIR)
	cp COPYING $(DISTDIR)$(INSTDIR)
	cp include/COPYING $(DISTDIR)$(INSTDIR)/include
	cp $(EXE) $(DISTDIR)$(INSTDIR)/bin
	cp lib/*.so $(DISTDIR)$(INSTDIR)/lib
	cp include/launcher_plugin.h $(DISTDIR)$(INSTDIR)/include
	cp bin/create_default_config $(DISTDIR)$(INSTDIR)/bin
	cp bin/AddLauncherIcon $(DISTDIR)$(INSTDIR)/bin
	cp bin/LBrowser $(DISTDIR)$(INSTDIR)/bin
	cp default.cfg $(DIST_DIR)$(INSTDIR)/default.cfg
	cp icons/* $(DISTDIR)$(INSTDIR)/icons/
	cp -r mime/* $(DISTDIR)$(INSTDIR)/mime/
	(cd plugins && $(MAKE) install )


$(LIB): $(LIBOBJS)
	$(LL) $(LIBOBJS) $(LIBINC) -shared -Wl,-soname,liblauncher.so -o $(LIB)

lib/obj/launcher_plugin.o: lib/src/launcher_plugin.cpp include/launcher_plugin.h
	$(CC) $(COPTS) lib/src/launcher_plugin.cpp -o $@

lib/obj/launcher_bar.o: include/launcher_bar.h lib/src/launcher_bar.cpp
	$(CC) lib/src/launcher_bar.cpp $(COPTS) -o lib/obj/launcher_bar.o

lib/obj/launcher_mime.o: include/launcher_mime.h lib/src/launcher_mime.cpp
	$(CC) lib/src/launcher_mime.cpp $(COPTS) -o lib/obj/launcher_mime.o

obj/main.o: src/main.cpp src/main.h
	$(CC) $(COPTS) src/main.cpp  -o $@

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



