CFLAGS   = -Wall -c -O3 -pipe -fexceptions -ffast-math -fomit-frame-pointer
CXXFLAGS = $(CFLAGS)
AOPTS   = -c

DRIVERBIN = /system/drivers/media

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

install:  $(OBJDIR)/cdrom.so
	
	install -s $(OBJDIR)/cdrom.so $(DRIVERBIN)/cdrom.so

dist:  $(OBJDIR)/cdrom.so
	install -s $(OBJDIR)/cdrom.so $(DIST_DIR)/atheos/sys/drivers/media/cdrom.so

deps: $(OBJDIR) $(DEPS)

clean:
	rm -f $(OBJS) $(OBJDIR)/*

-include $(OBJDIR)/*.d

