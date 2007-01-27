CFLAGS   = -c -fexceptions -ffast-math
CXXFLAGS = $(CFLAGS)
AOPTS   = -c
APPBIN = /system/extensions/dock
VPATH = ./
OBJDIR	:= objs
OBJS = Clipper.o

all :	$(OBJDIR) $(OBJS) $(OBJDIR)/Clipper.so

$(OBJDIR):
	mkdir $(OBJDIR)

$(OBJDIR)/Clipper.so: $(OBJS)
	$(CXX) -shared -Xlinker "-soname=Clipper.so" $(OBJS) -lsyllable -o $(OBJDIR)/Clipper.so
	rescopy -r $(OBJDIR)/Clipper.so images/*.png

install:  $(OBJDIR)/Clipper.so
	install -s $(OBJDIR)/Clipper.so $(APPBIN)/Clipper.so

dist:  $(OBJDIR)/Clipper.so
	install -s $(OBJDIR)/Clipper.so $(IMAGE)/$(APPBIN)/Clipper.so

deps: $(OBJDIR)

clean:
	rm -f $(OBJS) $(OBJDIR)/*.d $(OBJDIR)/Clipper.so

-include $(OBJDIR)/*.d

