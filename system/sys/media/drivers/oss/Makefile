CFLAGS   = -Wall -c -O4 -pipe -fexceptions -march=i586 -mcpu=i586 -ffast-math -fomit-frame-pointer
CPPFLAGS = $(CFLAGS)
AOPTS   = -c

DRIVERBIN = /system/drivers/media

VPATH = ./

CC	= gcc
LL	= ld

OBJS	= mixerchannel.o mixerview.o oss_output.o

OBJDIR	:= objs

include $(ATHEOS_SRC)/makefile.rules

all :	$(OBJDIR) $(OBJS) $(OBJDIR)/oss.so

$(OBJDIR):
	mkdir $(OBJDIR)

$(OBJDIR)/oss.so: $(OBJS)
	gcc -shared -Xlinker "-soname=oss.so" $(OBJS) -lstdc++ -lsyllable -lmedia -L/usr/lib/ -o $(OBJDIR)/oss.so


deps:

install:  $(OBJDIR)/oss.so
	
	install -s $(OBJDIR)/oss.so $(DRIVERBIN)/oss.so

dist:  $(OBJDIR)/oss.so
	install -s $(OBJDIR)/oss.so $(DIST_DIR)/atheos/sys/drivers/media/oss.so

deps: $(OBJDIR) $(DEPS)

clean:
	rm -f $(OBJS) $(OBJDIR)/*

-include $(OBJDIR)/*.d


























