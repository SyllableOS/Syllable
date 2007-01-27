CFLAGS   += -kernel -fno-strict-aliasing
AFLAGS   = -c

#CFLAGS += -I.. -I../../../include

OBJS := iso.o kernel_interface.o


FS = iso9660

all: $(OBJS)
	$(CC) -nostdlib -shared -L../../kernel/objs $(OBJS) -o $(FS) -lkernel `gcc -print-libgcc-file-name`

deps:

install: all
	cp -f $(FS) $(IMAGE)/system/drivers/fs/$(FS)

dist: install

clean:
	rm -f $(FS) $(OBJS)

