include ../freetype.makefile

COPTS   = -I../../../server -I$(FREETYPEINCLUDE) -c -O3 -Wall

DEVBIN = /system/drivers/appserver/decorators
OBJS =	beishdecorator.o gradient.o

OBJDIR	:= objs
OBJS	:= $(addprefix $(OBJDIR)/,$(OBJS))


#
#	rules
#

$(OBJDIR)/%.o : %.c
	$(CC) $(COPTS) $< -o $@

$(OBJDIR)/%.o : %.cpp
	$(CC) $(COPTS) $< -o $@


all :	objs $(OBJDIR)/beish

deps:

objs:
	mkdir objs

$(OBJDIR)/beish: $(OBJS)
	gcc $(OBJS) -o $(OBJDIR)/beish /system/appserver -lsyllable -lstdc++ -lz
	gcc -plugin $(OBJS) -o $(OBJDIR)/beish -lsyllable -lstdc++

install: $(OBJDIR)/beish
	cp -f $(OBJDIR)/beish $(DEVBIN)/beish

dist: $(OBJDIR)/beish
	cp -f $(OBJDIR)/beish $(DIST_DIR)/atheos/sys/drivers/appserver/decorators/Beish

clean:
	-rm $(OBJS) $(OBJDIR)/beish

