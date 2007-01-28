CFLAGS   += -c -fexceptions
CXXFLAGS += -c -fexceptions
AOPTS    = -c

APPBIN = /system

VPATH = ./

OBJS = mediaserver.o mediacontrols.o barview.o

OBJDIR	:= objs

include $(ATHEOS_SRC)/makefile.rules

all :	$(OBJDIR) $(OBJS) $(OBJDIR)/mediaserver

$(OBJDIR):
	mkdir $(OBJDIR)

$(OBJDIR)/mediaserver: $(OBJS)
	$(CXX) $(OBJS) -lsyllable -lmedia -o mediaserver
	rescopy mediaserver -r *.png

deps:

install: $(OBJDIR)/mediaserver
	install -s mediaserver $(IMAGE)/$(APPBIN)/mediaserver

dist: install

deps: $(OBJDIR) $(DEPS)

clean:
	rm -f $(OBJS) $(OBJDIR)/*.d mediaserver

-include $(OBJDIR)/*.d

