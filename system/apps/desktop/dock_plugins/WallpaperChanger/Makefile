CFLAGS   = -Wall -c -O4 -pipe -fexceptions -march=i586 -mcpu=i586 -ffast-math -fomit-frame-pointer
CPPFLAGS = $(CFLAGS)
AOPTS   = -c
APPBIN = /system/drivers/dock
VPATH = ./
CC	= gcc
LL	= ld
OBJDIR	:= objs
OBJS = WallpaperChanger.o WallpaperChangerLooper.o WallpaperChangerSettings.o bitmapscale.o

all :	$(OBJDIR) $(OBJS) $(OBJDIR)/WallpaperChanger.so

$(OBJDIR):
	mkdir $(OBJDIR)

$(OBJDIR)/WallpaperChanger.so: $(OBJS)
	gcc -shared -Xlinker "-soname=WallpaperChanger.so" $(OBJS) -lstdc++ -lsyllable -o $(OBJDIR)/WallpaperChanger.so
	rescopy -r $(OBJDIR)/WallpaperChanger.so images/*.png

install:  $(OBJDIR)/WallpaperChanger.so
	mkdir -p $(APPBIN)
	install -s $(OBJDIR)/WallpaperChanger.so $(APPBIN)/WallpaperChanger.so

dist:  $(OBJDIR)/WallpaperChanger.so
	mkdir -p $(DIST_DIR)/system/drivers/dock
	install -s $(OBJDIR)/WallpaperChanger.so $(DIST_DIR)/atheos/sys/drivers/dock/WallpaperChanger.so

deps: $(OBJDIR)

clean:
	rm -f $(OBJS) $(OBJDIR)/*.d $(OBJDIR)/WallpaperChanger.so

-include $(OBJDIR)/*.d


