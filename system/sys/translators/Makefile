CFLAGS   = -Wall -c -g -O3 -pipe
CPPFLAGS = $(CFLAGS)

TRANSDIR = /system/drivers/translators

VPATH = ./

CC	= gcc
LL	= ld

JPGOBJS = jpegtrans.o
PNGOBJS = pngtrans.o
GIFOBJS = giftrans.o
ICONOBJS = icontrans.o
WINICONOBJS = winicontrans.o

OBJS = $(JPGOBJS) $(PNGOBJS) $(GIFOBJS) $(ICONOBJS) $(WINICONOBJS)

OBJDIR	:= objs

include $(ATHEOS_SRC)/makefile.rules

all :	$(OBJDIR) $(OBJDIR)/jpegtrans.so $(OBJDIR)/pngtrans.so $(OBJDIR)/giftrans.so $(OBJDIR)/icontrans.so $(OBJDIR)/winicontrans.so

$(OBJDIR):
	mkdir $(OBJDIR)

$(OBJDIR)/jpegtrans.so: $(JPGOBJS)
	g++ -shared $(JPGOBJS) -lsyllable -ljpeg -L/usr/lib/ -o $(OBJDIR)/jpegtrans.so

$(OBJDIR)/pngtrans.so: $(PNGOBJS)
	g++ -shared $(PNGOBJS) -lsyllable -lpng -lz -L/usr/lib/ -o $(OBJDIR)/pngtrans.so

$(OBJDIR)/giftrans.so: $(GIFOBJS)
	g++ -shared $(GIFOBJS) -lsyllable -lungif -lz -L/usr/lib/ -o $(OBJDIR)/giftrans.so

$(OBJDIR)/icontrans.so: $(ICONOBJS)
	g++ -shared $(ICONOBJS) -lsyllable -L/usr/lib/ -o $(OBJDIR)/icontrans.so

$(OBJDIR)/winicontrans.so: $(WINICONOBJS)
	g++ -shared $(WINICONOBJS) -lsyllable -L/usr/lib/ -o $(OBJDIR)/winicontrans.so

install: $(OBJDIR)/jpegtrans.so $(OBJDIR)/pngtrans.so $(OBJDIR)/giftrans.so $(OBJDIR)/icontrans.so $(OBJDIR)/winicontrans.so
	install -s $(OBJDIR)/jpegtrans.so $(OBJDIR)/pngtrans.so $(OBJDIR)/giftrans.so $(OBJDIR)/icontrans.so $(OBJDIR)/winicontrans.so $(TRANSDIR)/

dist: $(OBJDIR)/jpegtrans.so $(OBJDIR)/pngtrans.so $(OBJDIR)/giftrans.so $(OBJDIR)/icontrans.so $(OBJDIR)/winicontrans.so
	install -s $(OBJDIR)/jpegtrans.so $(OBJDIR)/pngtrans.so $(OBJDIR)/giftrans.so $(OBJDIR)/icontrans.so $(OBJDIR)/winicontrans.so $(DIST_DIR)/atheos/sys/translators/

deps: $(OBJDIR) $(DEPS)

clean:
	rm -f *.o $(OBJDIR)/*.d $(OBJDIR)/*.so

-include $(OBJDIR)/*.d

