CFLAGS   += -kernel -fno-strict-aliasing
AFLAGS   = -c

#CFLAGS += -I.. -I../../../include

OBJS := iso.o kernel_interface.o


FS = iso9660

$(FS): $(OBJS)
	$(CC) -nostdlib -shared -L../../kernel/objs $(OBJS) -o $(FS) -lkernel `gcc -print-libgcc-file-name`

clean:
	rm -f $(FS) $(OBJS)
