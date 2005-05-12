CFLAGS += -c -D__KERNEL__

OBJS = start32.o stage2.o loadelf.o lib.o
all: init.com stage2

stage2:	$(OBJS)
	ld -m elf_i386_syllable -Tstage2.lnk $(OBJS) -o stage2
# `$(CC) -print-libgcc-file-name`
#	$(CC) -nostartfiles -nostdlib start32.o stage2.o lib.o -o stage2

stage2.o: stage2.c
	$(CC) $(CFLAGS) stage2.c
loadelf.o: loadelf.c
	$(CC) $(CFLAGS) loadelf.c
lib.o: lib.c
	$(CC) $(CFLAGS) lib.c
start32.o: start32.s
	$(CC) $(CFLAGS) start32.s

aload.com: init32.o

init.com: init.asm
	nasm init.asm -o init.com

#install:
#	cp init.com ../../init.com
#	cp stage2   ../../stage2

