CFLAGS   += -c -fexceptions -ffast-math
CXXFLAGS += -c -fexceptions -ffast-math
AOPTS   = -c
APPBIN = /system/extensions/dock
VPATH = ./
OBJDIR	:= objs
OBJS = Camera.o

all :	$(OBJDIR) $(OBJS) $(OBJDIR)/Camera.so

$(OBJDIR):
	mkdir $(OBJDIR)

$(OBJDIR)/Camera.so: $(OBJS)
	$(CXX) -shared -Xlinker "-soname=Camera.so" $(OBJS) -lsyllable -o $(OBJDIR)/Camera.so
	rescopy -r $(OBJDIR)/Camera.so images/*.png

install:  $(OBJDIR)/Camera.so
	mkdir -p $(APPBIN)
	install -s $(OBJDIR)/Camera.so $(APPBIN)/Camera.so

dist:  $(OBJDIR)/Camera.so
	mkdir -p $(DIST_DIR)/atheos/sys/extensions/dock
	install -s $(OBJDIR)/Camera.so $(DIST_DIR)/atheos/sys/extensions/dock/Camera.so

deps: $(OBJDIR)

clean:
	rm -f $(OBJS) $(OBJDIR)/Camera.so

-include $(OBJDIR)/*.d


