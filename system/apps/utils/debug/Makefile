COPTS	= -c

OBJS	= term.o
EXE	= dbterm

all: dbterm

$(EXE) : $(OBJS)
	$(CC) $(OBJS) -o $(EXE)

ftp:	ftp.o
	$(CC) ftp.o -o ftp

.cpp.o :
	$(CXX) $(COPTS) $< -o $@
.c.o :
	$(CC) $(COPTS) $< -o $@

install:
	cp -f $(EXE) /system/bin/$(EXE)

dist:
	cp -f $(EXE) $(IMAGE)/system/bin/$(EXE)

deps:

term.o : term.c

clean:
	-rm $(OBJS) dbterm rfs

