CFLAGS   = -D__KERNEL__ -fno-PIC -c -O3 -Wall

OBJS = nfs.o


include $(ATHEOS_SRC)/makefile.rules

all: nfs

nfs: nfs.o Makefile
	gcc --fno-PIC -nostdlib nfs.o -L/system/ -lkernel -o nfs

install: all
	cp -f nfs /system/drivers/fs/nfs

dist: all
	cp -f nfs $(DIST_DIR)/system/drivers/fs/nfs

nfs.o: nfs.c


