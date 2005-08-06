CFLAGS   = -Wall -c -O4 -pipe -fexceptions -march=i586 -ffast-math -fomit-frame-pointer
CPPFLAGS = $(CFLAGS)
AOPTS   = -c
APPBIN = /system/drivers/dock
VPATH = ./
CC	= gcc
LL	= ld
OBJDIR	:= objs
OBJS = Switcher.o

all :	$(OBJDIR) $(OBJS) $(OBJDIR)/Switcher.so

$(OBJDIR):
	mkdir $(OBJDIR)

$(OBJDIR)/Switcher.so: $(OBJS)
	gcc -shared -Xlinker "-soname=Switcher.so" $(OBJS) -lstdc++ -lsyllable -o $(OBJDIR)/Switcher.so
	rescopy -r $(OBJDIR)/Switcher.so images/*.png

install:  $(OBJDIR)/Switcher.so
	mkdir -p $(APPBIN)
	install -s $(OBJDIR)/Switcher.so $(APPBIN)/Switcher.so

dist:  $(OBJDIR)/Switcher.so
	mkdir -p $(DIST_DIR)/system/drivers/dock
	install -s $(OBJDIR)/Switcher.so $(DIST_DIR)/atheos/sys/drivers/dock/Switcher.so

deps: $(OBJDIR)

clean:
	rm -f $(OBJS) $(OBJDIR)/*.d $(OBJDIR)/Switcher.so

-include $(OBJDIR)/*.d


