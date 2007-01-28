CFLAGS   += -c -fexceptions -ffast-math
CXXFLAGS += -c -fexceptions -ffast-math
AOPTS    = -c

DRIVERBIN = /system/extensions/media

VPATH = ./

OBJS	= raw_codec.o

OBJDIR	:= objs

include $(ATHEOS_SRC)/makefile.rules

all :	$(OBJDIR) $(OBJS) $(OBJDIR)/raw.so

$(OBJDIR):
	mkdir $(OBJDIR)

$(OBJDIR)/raw.so: $(OBJS)
	$(CXX) -shared -Xlinker "-soname=raw.so" $(OBJS) -lsyllable -lmedia -o $(OBJDIR)/raw.so


deps:

install: $(OBJDIR)/raw.so
	install -s $(OBJDIR)/raw.so $(IMAGE)/$(DRIVERBIN)/raw.so

dist: install

deps: $(OBJDIR) $(DEPS)

clean:
	rm -f $(OBJS) $(OBJDIR)/*

-include $(OBJDIR)/*.d

