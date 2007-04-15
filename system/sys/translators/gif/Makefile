OBJS = giftrans.o
OBJDIR = objs

TRANSDIR = /system/extensions/translators

#include $(ATHEOS_SRC)/makefile.rules

all :	$(OBJDIR) $(OBJDIR)/giftrans.so

$(OBJDIR):
	mkdir $(OBJDIR)

$(OBJDIR)/giftrans.so: $(OBJS)
	$(CXX) $(CXXFLAGS) -shared $(OBJS) -lsyllable -lungif -o $(OBJDIR)/giftrans.so

clean:
	rm -f *.o $(OBJDIR)/*.d $(OBJDIR)/*.so

install:
	install $(OBJDIR)/giftrans.so $(IMAGE)/$(TRANSDIR)/

dist: install

-include $(OBJDIR)/*.d 

