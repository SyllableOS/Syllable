COPTS   = -c -O3 -Wall -g
#OPTS   = -c -O -Wall

CC	= gcc
EXE	= desktop

OBJDIR	:= objs


$(OBJDIR)/%.o : %.c
	$(CC) $(COPTS) $< -o $@

$(OBJDIR)/%.o : %.cpp
	$(CC) $(COPTS) $< -o $@


OBJS := main.o iconview.o jpeg.o login.o
OBJS := $(addprefix $(OBJDIR)/,$(OBJS))


$(EXE): objs $(OBJS)
	gcc $(OBJS) -L/usr/local/lib -lstdc++ -latheos -ljpeg -lcrypt -o $(EXE)

objs:
	mkdir objs

install:
	cp -f desktop /bin/
	strip --strip-all /bin/desktop

dist:
	cp -f desktop $(DIST_DIR)/atheos/sys/bin/
	strip --strip-all $(DIST_DIR)/atheos/sys/bin/desktop

deps:

clean:
	-rm $(OBJS)

main.o:		 main.cpp
jpeg.0:		 jpeg.cpp
