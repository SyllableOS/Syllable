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

install:  $(OBJDIR)/oss.so
	install -s $(OBJDIR)/oss.so $(DRIVERBIN)/oss.so
	addattrib $(DRIVERBIN)/oss.so os::Category AudioDriverCompat

dist:  $(OBJDIR)/oss.so
	install -s $(OBJDIR)/oss.so $(DIST_DIR)/atheos/sys/extensions/media/oss.so
	addattrib $(DIST_DIR)/atheos/sys/extensions/media/oss.so os::Category AudioDriverCompat

deps: $(OBJDIR) $(DEPS)

clean:
	rm -f $(OBJS) $(OBJDIR)/*

-include $(OBJDIR)/*.d

