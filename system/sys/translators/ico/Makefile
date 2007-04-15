ICONOBJS = icontrans.o
WINICONOBJS = winicontrans.o
OBJDIR = objs

TRANSDIR = /system/extensions/translators

#include $(ATHEOS_SRC)/makefile.rules

all :	$(OBJDIR) $(OBJDIR)/icontrans.so $(OBJDIR)/winicontrans.so

$(OBJDIR):
	mkdir $(OBJDIR)

$(OBJDIR)/icontrans.so: $(ICONOBJS)
	$(CXX) $(CXXFLAGS) -shared $(ICONOBJS) -lsyllable -o $(OBJDIR)/icontrans.so

$(OBJDIR)/winicontrans.so: $(WINICONOBJS)
	$(CXX) $(CXXFLAGS) -shared $(WINICONOBJS) -lsyllable -o $(OBJDIR)/winicontrans.so

clean:
	rm -f *.o $(OBJDIR)/*.d $(OBJDIR)/*.so

install:
	install $(OBJDIR)/icontrans.so $(OBJDIR)/winicontrans.so $(IMAGE)/$(TRANSDIR)/

dist: install

-include $(OBJDIR)/*.d 

