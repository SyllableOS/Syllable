include ../freetype.makefile

COPTS   = -I../../../server -I$(FREETYPEINCLUDE) -c -O3 -Wall

DEVBIN = /system/drivers/appserver/decorators
OBJS =	amigadecorator.o

OBJDIR	:= objs
OBJS	:= $(addprefix $(OBJDIR)/,$(OBJS))


#
#	rules
#

$(OBJDIR)/%.o : %.c
	$(CC) $(COPTS) $< -o $@

$(OBJDIR)/%.o : %.cpp
	$(CC) $(COPTS) $< -o $@


all :	objs $(OBJDIR)/amiga

objs:
	mkdir objs

$(OBJDIR)/amiga: $(OBJS)
	gcc $(OBJS) -o $(OBJDIR)/amiga /system/appserver -lsyllable -lstdc++ -lz
	gcc -plugin $(OBJS) -o $(OBJDIR)/amiga -lsyllable -lstdc++

deps:

install: $(OBJDIR)/amiga
	cp -f $(OBJDIR)/amiga $(DEVBIN)/amiga

dist: $(OBJDIR)/amiga
	cp -f $(OBJDIR)/amiga $(DIST_DIR)/atheos/sys/drivers/appserver/decorators/amiga

clean:
	-rm $(OBJS) $(OBJDIR)/amiga

