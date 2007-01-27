CFLAGS   += -c -fexceptions -ffast-math
CXXFLAGS += -c -fexceptions -ffast-math
AOPTS   = -c
APPBIN = /system/extensions/dock
VPATH = ./
OBJDIR	:= objs
OBJS = WallpaperChanger.o WallpaperChangerLooper.o WallpaperChangerSettings.o bitmapscale.o

all :	$(OBJDIR) $(OBJS) $(OBJDIR)/WallpaperChanger.so

$(OBJDIR):
	mkdir $(OBJDIR)

$(OBJDIR)/WallpaperChanger.so: $(OBJS)
	$(CXX) -shared -Xlinker "-soname=WallpaperChanger.so" $(OBJS) -lsyllable -o $(OBJDIR)/WallpaperChanger.so
	rescopy -r $(OBJDIR)/WallpaperChanger.so images/*.png

install:  $(OBJDIR)/WallpaperChanger.so
	install -s $(OBJDIR)/WallpaperChanger.so $(APPBIN)/WallpaperChanger.so

dist:  $(OBJDIR)/WallpaperChanger.so
	install -s $(OBJDIR)/WallpaperChanger.so $(IMAGE)/$(APPBIN)/WallpaperChanger.so

deps: $(OBJDIR)

clean:
	rm -f $(OBJS) $(OBJDIR)/*.d $(OBJDIR)/WallpaperChanger.so

-include $(OBJDIR)/*.d

