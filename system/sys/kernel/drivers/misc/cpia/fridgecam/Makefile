COPTS	= -c -I${ATHEOS_SRC}/kernel/drivers/misc/cpia/lib -Wno-multichar

OBJS	= main.o fcapp.o fcwin.o bitmapview.o
EXE	= fridgecam

$(EXE) : $(OBJS)
	$(CXX) $(OBJS) -lsyllable -ljpeg -lcpia -o $@

msgportmodule.so: msgportmodule.c
	$(CC) -shared msgportmodule.c -lpython1.5 -o msgportmodule.so

install: msgportmodule.so
	cp -f msgportmodule.so $(IMAGE)/usr/python/lib/python1.5/lib-dynload/msgportmodule.so

dist: install

clean:
	-rm $(OBJS)

.cpp.o:
	$(CC) $(COPTS) $< -o $@

