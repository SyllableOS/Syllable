# Default Makefile for XSpringies

STARTUP	= stdinit.o

OBJS = xdisp.o misc.o keypress.o widget.o obj.o file.o phys.o athgui.o widgets.o main.o
SRCS = xdisp.c misc.c keypress.c widget.c obj.c file.c phys.c athgui.cpp widgets.cpp main.cpp

DDIR = /usr/games/

XSPDIR = $(DDIR)lib/xspringies
BINDIR = $(DDIR)bin
MANDIR = $(DDIR)man/manx

LIBS = -lm -latheos -lstdc++
CFLAGS = -O3 -DDEF_PATH=\"$(XSPDIR)/\"

xspringies: $(OBJS)
	$(CC) $(CFLAGS) -o xspringies $(OBJS) $(LIBS)

.cpp.o :
	$(CC) $(COPTS) $< -o $@




$(OBJS): defs.h
file.o xdisp.o obj.o phys.o: obj.h
xdisp.o: bitmap.h title.h bfbm.h

main.o: main.cpp
	$(CC) -c -O2 $< -o $@
athgui.o: athgui.cpp
	$(CC) -c -O2 $< -o $@

widgets.o: widgets.cpp
	$(CC) -c -O2 $< -o $@

install: xspringies
	cp xspringies /bin/xspringies

dist: xspringies
	cp xspringies $(DIST_DIR)/atheos/sys/bin/xspringies

clean:
	rm -f $(OBJS) xspringies
