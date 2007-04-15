OBJS = xbmtrans.o
OBJDIR = objs

TRANSDIR = /system/extensions/translators

#include $(ATHEOS_SRC)/makefile.rules

all :	$(OBJDIR) $(OBJDIR)/xbmtrans.so

$(OBJDIR):
	mkdir $(OBJDIR)

$(OBJDIR)/xbmtrans.so: $(OBJS)
	$(CXX) $(CXXFLAGS) -shared $(OBJS) -lsyllable -o $(OBJDIR)/xbmtrans.so

clean:
	rm -f *.o $(OBJDIR)/*.d $(OBJDIR)/*.so

install:
	install $(OBJDIR)/xbmtrans.so $(IMAGE)/$(TRANSDIR)/

dist: install

-include $(OBJDIR)/*.d 

