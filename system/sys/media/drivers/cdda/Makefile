CFLAGS   = -Wall -c -O4 -pipe -fexceptions -march=i586 -mcpu=i586 -ffast-math -fomit-frame-pointer
CPPFLAGS = $(CFLAGS)
AOPTS   = -c

DRIVERBIN = /system/drivers/media

VPATH = ./

CC	= gcc
LL	= ld

OBJS	= cdda_input.o

OBJDIR	:= objs

include $(ATHEOS_SRC)/makefile.rules

all :	$(OBJDIR) $(OBJS) $(OBJDIR)/cdda.so

$(OBJDIR):
	mkdir $(OBJDIR)

$(OBJDIR)/cdda.so: $(OBJS)
	gcc -shared -Xlinker "-soname=cdda.so" $(OBJS) -lstdc++ -lsyllable -lmedia -lm -o $(OBJDIR)/cdda.so


deps:

install:  $(OBJDIR)/cdda.so
	
	install -s $(OBJDIR)/cdda.so $(DRIVERBIN)/cdda.so

dist:  $(OBJDIR)/cdda.so
	install -s $(OBJDIR)/cdda.so $(DIST_DIR)/atheos/sys/drivers/media/cdda.so

deps: $(OBJDIR) $(DEPS)

clean:
	rm -f $(OBJS) $(OBJDIR)/*

-include $(OBJDIR)/*.d









































