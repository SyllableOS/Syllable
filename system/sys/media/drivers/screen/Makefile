CFLAGS   += -c -fexceptions -ffast-math -fno-PIC
CXXFLAGS += -c -fexceptions -ffast-math -fno-PIC
AOPTS    = -c

DRIVERBIN = /system/extensions/media

VPATH = ./

OBJS	= yuv2rgb.o rgb2rgb.o screen.o

OBJDIR	:= objs

include $(ATHEOS_SRC)/makefile.rules

all :	$(OBJDIR) $(OBJS) $(OBJDIR)/screen.so

$(OBJDIR):
	mkdir $(OBJDIR)

$(OBJDIR)/screen.so: $(OBJS)
	$(CXX) -shared -Xlinker "-soname=screen.so" $(OBJS) -lsyllable -lmedia -o $(OBJDIR)/screen.so


deps:

install: $(OBJDIR)/screen.so
	install -s $(OBJDIR)/screen.so $(IMAGE)/$(DRIVERBIN)/screen.so

dist: install

deps: $(OBJDIR) $(DEPS)

clean:
	rm -f $(OBJS) $(OBJDIR)/*

-include $(OBJDIR)/*.d

