COPTS =  -Wall -fPIC -c -O2
LLIBS = -L../lib -latheos -lstdc++ -lm -llauncher

CC = g++
LL = g++

PLUGINS = obj/gbar.o Clock.so LauncherBar.so CPU.so Mem.so LauncherIcon.so CPUII.so MEMII.so Battery.so
OBJS = obj/gbar.o obj/Clock.o obj/LauncherBar.o obj/CPU.o obj/Mem.o obj/LauncherIcon.o obj/CPUII.o obj/MEMII.o obj/Battery.o

INSTDIR = /Applications/Launcher

all : $(PLUGINS) $(OBJS)

clean:
	-rm $(OBJS) $(PLUGINS)

install: all
	-strip --strip-all *.so
	-cp *.so $(DIST_DIR)$(INSTDIR)/plugins

obj/gbar.o: src/gbar.cpp src/gbar.h src/blendablecolor.h
	$(CC) src/gbar.cpp $(COPTS) -o obj/gbar.o

Clock.so: obj/Clock.o
	$(LL) obj/Clock.o $(LLIBS) -shared -Wl,-soname,Clock.so -o Clock.so
                                                                                                                                                                                                        
obj/Clock.o: src/Clock.cpp
	$(CC) src/Clock.cpp $(COPTS) -o obj/Clock.o

MEMII.so: obj/MEMII.o
	$(LL) obj/MEMII.o obj/gbar.o $(LLIBS) -shared -Wl,-soname,MEMII.so -o MEMII.so
                                                                                                                                                                                                        
obj/MEMII.o: src/MEMII.cpp src/gbar.h
	$(CC) src/MEMII.cpp $(COPTS) -o obj/MEMII.o

CPUII.so: obj/CPUII.o
	$(LL) obj/CPUII.o obj/gbar.o $(LLIBS) -shared -Wl,-soname,CPUII.so -o CPUII.so
                                                                                                                                                                                                        
obj/CPUII.o: src/CPUII.cpp src/gbar.h
	$(CC) src/CPUII.cpp $(COPTS) -o obj/CPUII.o

Battery.so: obj/Battery.o
	$(LL) obj/Battery.o obj/gbar.o $(LLIBS) -shared -Wl,-soname,Battery.so -o Battery.so
                                                                                                                                                                                                        
obj/Battery.o: src/Battery.cpp src/gbar.h
	$(CC) src/Battery.cpp $(COPTS) -o obj/Battery.o

LauncherBar.so: obj/LauncherBar.o 
	$(LL) obj/LauncherBar.o $(LLIBS) -shared -Wl,-soname,LauncherBar.so -o LauncherBar.so

obj/LauncherBar.o: src/LauncherBar.cpp 
	$(CC) src/LauncherBar.cpp $(COPTS) -o obj/LauncherBar.o

CPU.so: obj/CPU.o
	$(LL) obj/CPU.o $(LLIBS) -shared -Wl,-soname,CPU.so -o CPU.so

obj/CPU.o: src/CPU.cpp
	$(CC) src/CPU.cpp $(COPTS) -o obj/CPU.o

Mem.so: obj/Mem.o
	$(LL) obj/Mem.o $(LLIBS) -shared -Wl,-soname,Mem.so -o Mem.so

obj/Mem.o: src/Mem.cpp
	$(CC) src/Mem.cpp $(COPTS) -o obj/Mem.o

LauncherIcon.so: obj/LauncherIcon.o
	$(LL) obj/LauncherIcon.o $(LLIBS) -shared -Wl,-soname,LauncherIcon.so -o LauncherIcon.so

obj/LauncherIcon.o: src/LauncherIcon.cpp
	$(CC) src/LauncherIcon.cpp $(COPTS) -o obj/LauncherIcon.o

