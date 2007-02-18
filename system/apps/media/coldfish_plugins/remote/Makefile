CFLAGS   += -c -fexceptions
CXXFLAGS += -c -fexceptions
AOPTS   = -c

DRIVERBIN = /Applications/Media/Plugins/

VPATH = ./

OBJS	= Remote.o

OBJDIR	:= objs

include $(ATHEOS_SRC)/makefile.rules

all :	$(OBJDIR) $(OBJS) $(OBJDIR)/Remote

$(OBJDIR):
	mkdir $(OBJDIR)

$(OBJDIR)/Remote: $(OBJS)
	$(CXX) -shared -Xlinker "-soname=Remote" $(OBJS) -lsyllable -o $(OBJDIR)/Remote

deps:

install:  $(OBJDIR)/Remote
	mkdir -p $(IMAGE)/$(DRIVERBIN)
	install -s $(OBJDIR)/Remote $(IMAGE)/$(DRIVERBIN)

dist: install

deps: $(OBJDIR) $(DEPS)

clean:
	rm -f $(OBJS) $(OBJDIR)/*

-include $(OBJDIR)/*.d

