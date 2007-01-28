CFLAGS   += -c -fexceptions -ffast-math
CXXFLAGS += -c -fexceptions -ffast-math
AOPTS    = -c

DRIVERBIN = /system/extensions/media

VPATH = ./

OBJS	= cdda_input.o

OBJDIR	:= objs

include $(ATHEOS_SRC)/makefile.rules

all :	$(OBJDIR) $(OBJS) $(OBJDIR)/cdda.so

$(OBJDIR):
	mkdir $(OBJDIR)

$(OBJDIR)/cdda.so: $(OBJS)
	$(CXX) -shared -Xlinker "-soname=cdda.so" $(OBJS) -lsyllable -lmedia -lm -o $(OBJDIR)/cdda.so


deps:

install: $(OBJDIR)/cdda.so
	install -s $(OBJDIR)/cdda.so $(IMAGE)/$(DRIVERBIN)/cdda.so

dist: install

deps: $(OBJDIR) $(DEPS)

clean:
	rm -f $(OBJS) $(OBJDIR)/*

-include $(OBJDIR)/*.d

