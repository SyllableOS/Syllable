OBJS = tifftrans.o
OBJDIR = objs

TRANSDIR = /system/extensions/translators

#include $(ATHEOS_SRC)/makefile.rules

all :	$(OBJDIR) $(OBJDIR)/tifftrans.so

$(OBJDIR):
	mkdir $(OBJDIR)

$(OBJDIR)/tifftrans.so: $(OBJS)
	$(CXX) $(CXXFLAGS) -shared $(OBJS) -lsyllable -ltiff -ljpeg -lz -o $(OBJDIR)/tifftrans.so

clean:
	rm -f *.o $(OBJDIR)/*.d $(OBJDIR)/*.so

install:
	install $(OBJDIR)/tifftrans.so $(IMAGE)/$(TRANSDIR)/

dist: install

-include $(OBJDIR)/*.d 

