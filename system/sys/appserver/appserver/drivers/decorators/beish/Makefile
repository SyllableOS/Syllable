COPTS   = -I../../../server $(shell freetype-config --cflags) -c -O3 -Wall

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


all :	objs $(OBJDIR)/Beish

deps:

objs:
	mkdir objs

$(OBJDIR)/Beish: $(OBJS)
	gcc $(OBJS) -o $(OBJDIR)/Beish /system/appserver -lsyllable -lstdc++ -lz
	gcc -plugin $(OBJS) -o $(OBJDIR)/Beish -lsyllable -lstdc++

install: $(OBJDIR)/Beish
	cp -f $(OBJDIR)/Beish $(DEVBIN)/Beish

dist: $(OBJDIR)/Beish
	cp -f $(OBJDIR)/Beish $(DIST_DIR)/atheos/sys/drivers/appserver/decorators/Beish

clean:
	-rm $(OBJS) $(OBJDIR)/Beish

