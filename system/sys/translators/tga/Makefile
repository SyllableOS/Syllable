OBJS = tgatrans.o
OBJDIR = objs

TRANSDIR = /system/extensions/translators

#include $(ATHEOS_SRC)/makefile.rules

all :	$(OBJDIR) $(OBJDIR)/tgatrans.so

$(OBJDIR):
	mkdir $(OBJDIR)

$(OBJDIR)/tgatrans.so: $(OBJS)
	$(CXX) $(CXXFLAGS) -shared $(OBJS) -lsyllable -o $(OBJDIR)/tgatrans.so

clean:
	rm -f *.o $(OBJDIR)/*.d $(OBJDIR)/*.so

install:
	install $(OBJDIR)/tgatrans.so $(IMAGE)/$(TRANSDIR)/

dist: install

-include $(OBJDIR)/*.d 

