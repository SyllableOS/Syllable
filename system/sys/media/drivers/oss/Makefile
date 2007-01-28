CFLAGS   += -c -fexceptions -ffast-math
CXXFLAGS += -c -fexceptions -ffast-math
AOPTS    = -c

DRIVERBIN = /system/extensions/media

VPATH = ./

OBJS	= mixerchannel.o mixerview.o oss_output.o

OBJDIR	:= objs

include $(ATHEOS_SRC)/makefile.rules

all :	$(OBJDIR) $(OBJS) $(OBJDIR)/oss.so

$(OBJDIR):
	mkdir $(OBJDIR)

$(OBJDIR)/oss.so: $(OBJS)
	$(CXX) -shared -Xlinker "-soname=oss.so" $(OBJS) -lsyllable -lmedia -o $(OBJDIR)/oss.so


deps:

install: $(OBJDIR)/oss.so
	install -s $(OBJDIR)/oss.so $(IMAGE)/$(DRIVERBIN)/oss.so
	addattrib $(IMAGE)/$(DRIVERBIN)/oss.so os::Category AudioDriverCompat

dist: install

deps: $(OBJDIR) $(DEPS)

clean:
	rm -f $(OBJS) $(OBJDIR)/*

-include $(OBJDIR)/*.d

