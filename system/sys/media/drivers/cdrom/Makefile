CFLAGS   += -c -fexceptions -ffast-math
CXXFLAGS += -c -fexceptions -ffast-math
AOPTS    = -c

DRIVERBIN = /system/extensions/media

VPATH = ./

OBJS	= cdrom.o

OBJDIR	:= objs

include $(ATHEOS_SRC)/makefile.rules

all :	$(OBJDIR) $(OBJS) $(OBJDIR)/cdrom.so

$(OBJDIR):
	mkdir $(OBJDIR)

$(OBJDIR)/cdrom.so: $(OBJS)
	$(CXX) -shared -Xlinker "-soname=cdrom.so" $(OBJS) -lsyllable -lmedia -o $(OBJDIR)/cdrom.so


deps:

install: $(OBJDIR)/cdrom.so
	install -s $(OBJDIR)/cdrom.so $(IMAGE)/$(DRIVERBIN)/cdrom.so

dist: install

deps: $(OBJDIR) $(DEPS)

clean:
	rm -f $(OBJS) $(OBJDIR)/*

-include $(OBJDIR)/*.d

