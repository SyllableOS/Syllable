CFLAGS   += -c -fexceptions
CXXFLAGS += -c -fexceptions
AOPTS   = -c

DRIVERBIN = /Applications/Media/Plugins

VPATH = ./

OBJS	= bitmap.o blur.o color.o config.o img.o loop.o paste.o render.o text.o blursk.o

OBJDIR	:= objs

include $(ATHEOS_SRC)/makefile.rules

all :	$(OBJDIR) $(OBJS) $(OBJDIR)/Blur

$(OBJDIR):
	mkdir $(OBJDIR)

$(OBJDIR)/Blur: $(OBJS)
	$(CXX) -shared -Xlinker "-soname=Blur" $(OBJS) -lsyllable -o $(OBJDIR)/Blur

deps:

install:  $(OBJDIR)/Blur
	mkdir -p $(IMAGE)/$(DRIVERBIN)
	install -s $(OBJDIR)/Blur $(IMAGE)/$(DRIVERBIN)/Blur

dist: install

deps: $(OBJDIR) $(DEPS)

clean:
	rm -f $(OBJS) $(OBJDIR)/*

-include $(OBJDIR)/*.d

