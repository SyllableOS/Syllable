CFLAGS   = -Wall -c -O4 -pipe -fexceptions -march=i586 -ffast-math -fomit-frame-pointer
CPPFLAGS = $(CFLAGS)
AOPTS   = -c
APPBIN = /system/drivers/dock
VPATH = ./
CC	= gcc
LL	= ld
OBJDIR	:= objs
OBJS = ColdFishRemote.o

all :	$(OBJDIR) $(OBJS) $(OBJDIR)/ColdFishRemote.so

$(OBJDIR):
	mkdir $(OBJDIR)

$(OBJDIR)/ColdFishRemote.so: $(OBJS)
	gcc -shared -Xlinker "-soname=ColdFishRemote.so" $(OBJS) -lstdc++ -lsyllable -o $(OBJDIR)/ColdFishRemote.so
	rescopy -r $(OBJDIR)/ColdFishRemote.so images/*.png

install:  $(OBJDIR)/ColdFishRemote.so
	mkdir -p $(APPBIN)
	install -s $(OBJDIR)/ColdFishRemote.so $(APPBIN)/ColdFishRemote.so

dist:  $(OBJDIR)/ColdFishRemote.so
	mkdir -p $(DIST_DIR)/system/drivers/dock
	install -s $(OBJDIR)/ColdFishRemote.so $(DIST_DIR)/atheos/sys/drivers/dock/ColdFishRemote.so

deps: $(OBJDIR)

clean:
	rm -f $(OBJS) $(OBJDIR)/*.d $(OBJDIR)/ColdFishRemote.so

-include $(OBJDIR)/*.d


