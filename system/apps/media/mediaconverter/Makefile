CFLAGS   += -c -fexceptions -ffast-math
CXXFLAGS += -c -fexceptions -ffast-math
AOPTS   = -c

APPBIN = /Applications/Media

VPATH = ./

OBJS = MediaConverter.o

OBJDIR	:= objs

include $(ATHEOS_SRC)/makefile.rules

all :	$(OBJDIR) $(OBJS) $(OBJDIR)/MediaConverter

$(OBJDIR):
	mkdir $(OBJDIR)

$(OBJDIR)/MediaConverter: $(OBJS)
	$(CXX) $(OBJS) -lsyllable -lmedia -o MediaConverter

deps:

install:  $(OBJDIR)/MediaConverter
	mkdir -p $(APPBIN)
	install -s MediaConverter $(APPBIN)/MediaConverter

dist:  $(OBJDIR)/MediaConverter
	mkdir -p $(DIST_DIR)/atheos/Applications/Media
	install -s MediaConverter $(DIST_DIR)/atheos/Applications/Media/MediaConverter

deps: $(OBJDIR) $(DEPS)

clean:
	rm -f $(OBJS) $(OBJDIR)/*.d MediaConverter

-include $(OBJDIR)/*.d

