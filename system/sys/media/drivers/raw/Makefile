CFLAGS   = -Wall -c -O4 -pipe -fexceptions -march=i586 -mcpu=i586 -ffast-math -fomit-frame-pointer
CPPFLAGS = $(CFLAGS)
AOPTS   = -c

DRIVERBIN = /system/drivers/media

VPATH = ./

CC	= gcc
LL	= ld

OBJS	= raw_codec.o

OBJDIR	:= objs

include $(ATHEOS_SRC)/makefile.rules

all :	$(OBJDIR) $(OBJS) $(OBJDIR)/raw.so

$(OBJDIR):
	mkdir $(OBJDIR)

$(OBJDIR)/raw.so: $(OBJS)
	gcc -shared -Xlinker "-soname=raw.so" $(OBJS) -lstdc++ -latheos -lmedia -o $(OBJDIR)/raw.so


deps:

install:  $(OBJDIR)/raw.so
	
	install -s $(OBJDIR)/raw.so $(DRIVERBIN)/raw.so

dist:  $(OBJDIR)/raw.so
	install -s $(OBJDIR)/raw.so $(DIST_DIR)/atheos/sys/drivers/media/raw.so

deps: $(OBJDIR) $(DEPS)

clean:
	rm -f $(OBJS) $(OBJDIR)/*

-include $(OBJDIR)/*.d



