COPTS	= -Wall -c -O2 -I${ATHEOS_SRC}/kernel/drivers/misc/cpia/lib -Wno-multichar

OBJS	= main.o fcapp.o fcwin.o bitmapview.o
EXE	= fridgecam

CC	= g++
LL	= g++

$(EXE) : $(OBJS)
	$(LL) $(OBJS) -latheos -ljpeg -L/usr/lib -lcpia -o $@

msgportmodule.so: msgportmodule.c
	gcc -Wall -O2 -shared msgportmodule.c -lpython1.5 -o msgportmodule.so

install: msgportmodule.so
	cp -f msgportmodule.so /usr/python/lib/python1.5/lib-dynload/msgportmodule.so

dist: msgportmodule.so
	cp -f msgportmodule.so $(DIST_DIR)/atheos/sys/usr/python/lib/python1.5/lib-dynload/msgportmodule.so

clean:
	-rm $(OBJS)

.cpp.o:
	$(CC) $(COPTS) $< -o $@

