CFLAGS   += -c -fexceptions
CXXFLAGS += -c -fexceptions
AOPTS    = -c

DRIVERBIN = /system/extensions/media

VPATH = ./

OBJS	= server_output.o

OBJDIR	:= objs

include $(ATHEOS_SRC)/makefile.rules

all :	$(OBJDIR) $(OBJS) $(OBJDIR)/serveraudio.so

$(OBJDIR):
	mkdir $(OBJDIR)

$(OBJDIR)/serveraudio.so: $(OBJS)
	$(CXX) -shared -Xlinker "-soname=serveraudio.so" $(OBJS) -lsyllable -lmedia -o $(OBJDIR)/serveraudio.so


deps:

install: $(OBJDIR)/serveraudio.so
	install -s $(OBJDIR)/serveraudio.so $(IMAGE)/$(DRIVERBIN)/serveraudio.so

dist: install

deps: $(OBJDIR) $(DEPS)

clean:
	rm -f $(OBJS) $(OBJDIR)/*

-include $(OBJDIR)/*.d

