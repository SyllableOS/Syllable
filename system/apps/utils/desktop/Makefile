
COPTS   = -c -O3 -Wall -g
#OPTS   = -c -O -Wall

CC	= gcc
EXE	= desktop

OBJDIR	:= objs


$(OBJDIR)/%.o : %.c
	$(CC) $(COPTS) $< -o $@

$(OBJDIR)/%.o : %.cpp
	$(CC) $(COPTS) $< -o $@


OBJS := main.o iconview.o login.o loadbitmap.o imageitem.o properties.o scaledbitmap.o colorbutton.o coloredit.o
OBJS := $(addprefix $(OBJDIR)/,$(OBJS))


$(EXE): objs $(OBJS)
	gcc $(OBJS) -L/usr/local/lib -lstdc++ -latheos -lcrypt -o $(EXE)
	rescopy -r $(EXE) ./images/*.jpg ./images/*.png
objs:
	mkdir objs

install:
	cp -f desktop /bin/
	strip --strip-all /bin/desktop
clean:
	-rm $(OBJS)
