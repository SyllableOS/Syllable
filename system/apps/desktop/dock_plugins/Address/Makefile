CFLAGS   = -Wall -c -O4 -pipe -fexceptions -march=i586 -mcpu=i586 -ffast-math -fomit-frame-pointer
CPPFLAGS = $(CFLAGS)
AOPTS   = -c
APPBIN = /system/drivers/dock
VPATH = ./
CC	= gcc
LL	= ld
OBJDIR	:= objs
OBJS = Address.o Settings.o

all :	$(OBJDIR) $(OBJS) $(OBJDIR)/Address.so

$(OBJDIR):
	mkdir $(OBJDIR)

$(OBJDIR)/Address.so: $(OBJS)
	gcc -shared -Xlinker "-soname=Address.so" $(OBJS) -lstdc++ -lsyllable -o $(OBJDIR)/Address.so
	rescopy -r $(OBJDIR)/Address.so resources/*

install:  $(OBJDIR)/Address.so
	mkdir -p $(APPBIN)
	install -s $(OBJDIR)/Address.so $(APPBIN)/Address.so

dist:  $(OBJDIR)/Address.so
	mkdir -p $(DIST_DIR)/system/drivers/dock
	install -s $(OBJDIR)/Address.so $(DIST_DIR)/atheos/sys/drivers/dock/Address.so

deps: $(OBJDIR)

clean:
	rm -f $(OBJS) $(OBJDIR)/*.d $(OBJDIR)/Address.so

-include $(OBJDIR)/*.d


