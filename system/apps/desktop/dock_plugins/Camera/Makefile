CFLAGS   = -Wall -c -O4 -pipe -fexceptions -march=i586 -mcpu=i586 -ffast-math -fomit-frame-pointer
CPPFLAGS = $(CFLAGS)
AOPTS   = -c
APPBIN = /system/drivers/dock
VPATH = ./
CC	= gcc
LL	= ld
OBJDIR	:= objs
OBJS = Camera.o

all :	$(OBJDIR) $(OBJS) $(OBJDIR)/Camera.so

$(OBJDIR):
	mkdir $(OBJDIR)

$(OBJDIR)/Camera.so: $(OBJS)
	gcc -shared -Xlinker "-soname=Camera.so" $(OBJS) -lstdc++ -lsyllable -o $(OBJDIR)/Camera.so
	rescopy -r $(OBJDIR)/Camera.so images/*.png

install:  $(OBJDIR)/Camera.so
	mkdir -p $(APPBIN)
	install -s $(OBJDIR)/Camera.so $(APPBIN)/Camera.so

dist:  $(OBJDIR)/Camera.so
	mkdir -p $(DIST_DIR)/system/drivers/dock
	install -s $(OBJDIR)/Camera.so $(DIST_DIR)/atheos/sys/drivers/dock/Camera.so

deps: $(OBJDIR)

clean:
	rm -f $(OBJS) $(OBJDIR)/*.d $(OBJDIR)/Camera.so

-include $(OBJDIR)/*.d


