CFLAGS   = -Wall -c -O3 -pipe -fexceptions -ffast-math -fomit-frame-pointer
CXXFLAGS = $(CFLAGS)
AOPTS   = -c

DRIVERBIN = /system/drivers/media

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

install:  $(OBJDIR)/cdda.so
	
	install -s $(OBJDIR)/cdda.so $(DRIVERBIN)/cdda.so

dist:  $(OBJDIR)/cdda.so
	install -s $(OBJDIR)/cdda.so $(DIST_DIR)/atheos/sys/drivers/media/cdda.so

deps: $(OBJDIR) $(DEPS)

clean:
	rm -f $(OBJS) $(OBJDIR)/*

-include $(OBJDIR)/*.d

