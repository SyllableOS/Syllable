CFLAGS   += -c -fexceptions -ffast-math
CXXFLAGS += -c -fexceptions -ffast-math
AOPTS   = -c
APPBIN = /system/extensions/dock
VPATH = ./
OBJDIR	:= objs
OBJS = Switcher.o

all :	$(OBJDIR) $(OBJS) $(OBJDIR)/Switcher.so

$(OBJDIR):
	mkdir $(OBJDIR)

$(OBJDIR)/Switcher.so: $(OBJS)
	$(CXX) -shared -Xlinker "-soname=Switcher.so" $(OBJS) -lsyllable -o $(OBJDIR)/Switcher.so
	rescopy -r $(OBJDIR)/Switcher.so images/*.png

install:  $(OBJDIR)/Switcher.so
	install -s $(OBJDIR)/Switcher.so $(APPBIN)/Switcher.so

dist:  $(OBJDIR)/Switcher.so
	install -s $(OBJDIR)/Switcher.so $(IMAGE)/$(APPBIN)/Switcher.so

deps: $(OBJDIR)

clean:
	rm -f $(OBJS) $(OBJDIR)/*.d $(OBJDIR)/Switcher.so

-include $(OBJDIR)/*.d

