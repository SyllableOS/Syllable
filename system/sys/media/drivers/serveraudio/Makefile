CFLAGS   = -Wall -c -O2 -pipe -fexceptions
CXXFLAGS = $(CFLAGS)
AOPTS   = -c

DRIVERBIN = /system/drivers/media

VPATH = ./

OBJS	= prefs.o server_output.o

OBJDIR	:= objs

include $(ATHEOS_SRC)/makefile.rules

all :	$(OBJDIR) $(OBJS) $(OBJDIR)/serveraudio.so

$(OBJDIR):
	mkdir $(OBJDIR)

$(OBJDIR)/serveraudio.so: $(OBJS)
	$(CXX) -shared -Xlinker "-soname=serveraudio.so" $(OBJS) -lsyllable -lmedia -o $(OBJDIR)/serveraudio.so


deps:

install:  $(OBJDIR)/serveraudio.so
	
	install -s $(OBJDIR)/serveraudio.so $(DRIVERBIN)/serveraudio.so

dist:  $(OBJDIR)/serveraudio.so
	install -s $(OBJDIR)/serveraudio.so $(DIST_DIR)/atheos/sys/drivers/media/serveraudio.so

deps: $(OBJDIR) $(DEPS)

clean:
	rm -f $(OBJS) $(OBJDIR)/*

-include $(OBJDIR)/*.d

