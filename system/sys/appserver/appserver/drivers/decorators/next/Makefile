include ../freetype.makefile

COPTS   = -I../../../server -I$(FREETYPEINCLUDE) -c -O3 -Wall

DEVBIN = /system/drivers/appserver/decorators
OBJS =	nextdecorator.o

OBJDIR	:= objs
OBJS	:= $(addprefix $(OBJDIR)/,$(OBJS))


#
#	rules
#

$(OBJDIR)/%.o : %.c
	$(CC) $(COPTS) $< -o $@

$(OBJDIR)/%.o : %.cpp
	$(CC) $(COPTS) $< -o $@


all :	objs $(OBJDIR)/next

objs:
	mkdir objs

$(OBJDIR)/next: $(OBJS)
	gcc $(OBJS) -o $(OBJDIR)/next /system/appserver -latheos -lstdc++
	gcc -plugin $(OBJS) -o $(OBJDIR)/next -latheos -lstdc++

deps:

install: $(OBJDIR)/next
	cp -f $(OBJDIR)/next $(DEVBIN)/next

dist: $(OBJDIR)/next
	cp -f $(OBJDIR)/next $(DIST_DIR)/atheos/sys/drivers/appserver/decorators/next

clean:
	-rm $(OBJS) $(OBJDIR)/next
