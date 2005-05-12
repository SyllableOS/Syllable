CFLAGS   += -c
CXXFLAGS += -c

TRANSDIR = /system/drivers/translators

VPATH = ./

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
	$(CXX) -shared $(JPGOBJS) -lsyllable -ljpeg -o $(OBJDIR)/jpegtrans.so

$(OBJDIR)/pngtrans.so: $(PNGOBJS)
	$(CXX) -shared $(PNGOBJS) -lsyllable -lpng -lz -o $(OBJDIR)/pngtrans.so

$(OBJDIR)/giftrans.so: $(GIFOBJS)
	$(CXX) -shared $(GIFOBJS) -lsyllable -lungif -lz -o $(OBJDIR)/giftrans.so

$(OBJDIR)/icontrans.so: $(ICONOBJS)
	$(CXX) -shared $(ICONOBJS) -lsyllable -o $(OBJDIR)/icontrans.so

$(OBJDIR)/winicontrans.so: $(WINICONOBJS)
	$(CXX) -shared $(WINICONOBJS) -lsyllable -o $(OBJDIR)/winicontrans.so

install: $(OBJDIR)/jpegtrans.so $(OBJDIR)/pngtrans.so $(OBJDIR)/giftrans.so $(OBJDIR)/icontrans.so $(OBJDIR)/winicontrans.so
	install -s $(OBJDIR)/jpegtrans.so $(OBJDIR)/pngtrans.so $(OBJDIR)/giftrans.so $(OBJDIR)/icontrans.so $(OBJDIR)/winicontrans.so $(TRANSDIR)/

dist: $(OBJDIR)/jpegtrans.so $(OBJDIR)/pngtrans.so $(OBJDIR)/giftrans.so $(OBJDIR)/icontrans.so $(OBJDIR)/winicontrans.so
	install -s $(OBJDIR)/jpegtrans.so $(OBJDIR)/pngtrans.so $(OBJDIR)/giftrans.so $(OBJDIR)/icontrans.so $(OBJDIR)/winicontrans.so $(DIST_DIR)/atheos/sys/translators/

deps: $(OBJDIR) $(DEPS)

clean:
	rm -f *.o $(OBJDIR)/*.d $(OBJDIR)/*.so

-include $(OBJDIR)/*.d

