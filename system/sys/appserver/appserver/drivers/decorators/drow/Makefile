include ../freetype.makefile

COPTS   = -I../../../server -I$(FREETYPEINCLUDE) -c -O3 -Wall

DEVBIN = /system/drivers/appserver/decorators
OBJS =	drowdecorator.o

OBJDIR	:= objs
OBJS	:= $(addprefix $(OBJDIR)/,$(OBJS))


#
#	rules
#

$(OBJDIR)/%.o : %.c
	$(CC) $(COPTS) $< -o $@

$(OBJDIR)/%.o : %.cpp
	$(CC) $(COPTS) $< -o $@


all :	objs $(OBJDIR)/drow

deps:

objs:
	mkdir objs

$(OBJDIR)/drow: $(OBJS)
	gcc $(OBJS) -o $(OBJDIR)/drow /system/appserver -lsyllable -lstdc++ -lz
	gcc -plugin $(OBJS) -o $(OBJDIR)/drow -lsyllable -lstdc++

install: $(OBJDIR)/drow
	cp -f $(OBJDIR)/drow $(DEVBIN)/drow

dist: $(OBJDIR)/drow
	cp -f $(OBJDIR)/drow $(DIST_DIR)/atheos/sys/drivers/appserver/decorators/Drow

clean:
	-rm $(OBJS) $(OBJDIR)/drow

