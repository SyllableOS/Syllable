COPTS   = -I../../../server -c -O3 -Wall

DEVBIN = /system/drivers/appserver/decorators
OBJS =	RedDecorator.o

OBJDIR	:= objs
OBJS	:= $(addprefix $(OBJDIR)/,$(OBJS))


#
#	rules
#

$(OBJDIR)/%.o : %.c
	$(CC) $(COPTS) $< -o $@

$(OBJDIR)/%.o : %.cpp
	$(CC) $(COPTS) $< -o $@


all :	objs $(OBJDIR)/Red

deps:

objs:
	mkdir objs

$(OBJDIR)/Red: $(OBJS)
	gcc $(OBJS) -o $(OBJDIR)/Red /system/appserver -latheos -lstdc++
	gcc -plugin $(OBJS) -o $(OBJDIR)/Red -latheos -lstdc++

install: $(OBJDIR)/Red
	cp -f $(OBJDIR)/Red $(DEVBIN)/Red

dist: $(OBJDIR)/Red
	cp -f $(OBJDIR)/Red $(DIST_DIR)/atheos/sys/drivers/appserver/decorators/Red

clean:
	-rm $(OBJS) $(OBJDIR)/Red
