COPTS	= -c -Wall

OBJS	= term.o
EXE	= dbterm

all: dbterm

$(EXE) : $(OBJS)
	$(CC) $(OBJS) -o $(EXE)

ftp:	ftp.o
	$(CC) ftp.o -o ftp

.cpp.o :
	$(CC) $(COPTS) $< -o $@
.c.o :
	$(CC) $(COPTS) $< -o $@

install:
	cp -f $(EXE) /bin/$(EXE)

dist:
	cp -f $(EXE) $(DIST_DIR)/atheos/sys/bin/$(EXE)

deps:

term.o : term.c

clean:
	-rm $(OBJS) dbterm rfs

