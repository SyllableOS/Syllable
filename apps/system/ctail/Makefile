
CFLAGS = -O3 -c

OBJS = main.o


ctail: $(OBJS)
	g++ $(OBJS) -o ctail

.cpp.o:
	g++ $(CFLAGS) $< -o $@

main.o: main.cpp

install: ctail
	cp -f ctail /bin/ctail

dist: ctail
	cp -f ctail $(DIST_DIR)/atheos/sys/bin/ctail

clean:
	-rm ctail ctail.o
