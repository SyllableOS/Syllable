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
	addattrib MediaConverter os::Category Media

deps:

install:  $(OBJDIR)/MediaConverter
	mkdir -p $(APPBIN)
	cp MediaConverter $(APPBIN)/MediaConverter

dist:  $(OBJDIR)/MediaConverter
	mkdir -p $(DIST_DIR)/atheos/Applications/Media
	cp MediaConverter $(DIST_DIR)/atheos/Applications/Media/MediaConverter

deps: $(OBJDIR) $(DEPS)

clean:
	rm -f $(OBJS) $(OBJDIR)/*.d MediaConverter

-include $(OBJDIR)/*.d

