
CFLAGS= -c -O3 -D__KERNEL__

OBJS = start32.o stage2.o loadelf.o lib.o
all: init.com stage2

stage2:	$(OBJS)
	ld -m elf_i386_atheos -Tstage2.lnk $(OBJS) -o stage2
# `gcc -print-libgcc-file-name`
#	gcc -nostartfiles -nostdlib start32.o stage2.o lib.o -o stage2

stage2.o: stage2.c
	gcc $(CFLAGS) stage2.c
loadelf.o: loadelf.c
	gcc $(CFLAGS) loadelf.c
lib.o: lib.c
	gcc $(CFLAGS) lib.c
start32.o: start32.s
	gcc $(CFLAGS) start32.s

aload.com: init32.o

init.com: init.asm
	nasm init.asm -o init.com

#install:
#	cp init.com ../../init.com
#	cp stage2   ../../stage2
