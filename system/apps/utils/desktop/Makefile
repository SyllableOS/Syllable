
#COPTS   = -c -o3 -Wall
OPTS   = -c -o3 -Wall

CC	= gcc
EXE	= desktop

OBJDIR	:= objs


$(OBJDIR)/%.o : %.c
	$(CC) $(OPTS) $< -o $@

$(OBJDIR)/%.o : %.cpp
	$(CC) $(OPTS) $< -o $@


OBJS := main.o iconview.o iconmenu.o iconmenu_prop.o login.o loadbitmap.o imageitem.o properties.o colorbutton.o coloredit.o bitmapscale.o disabled_textview.o
OBJS := $(addprefix $(OBJDIR)/,$(OBJS))


$(EXE): objs $(OBJS)
	gcc $(OBJS) -L/usr/local/lib -lstdc++ -latheos -lcrypt -lm -o $(EXE)
	rescopy -r $(EXE) ./images/*.jpg ./images/*.png

deps:
	mkdir objs
	
dist:
	cp  -f $(EXE) $(DIST_DIR)/atheos/sys/bin

install:
	cp -f desktop /bin/
	strip --strip-all /bin/desktop
clean:
	-rm $(OBJS)

