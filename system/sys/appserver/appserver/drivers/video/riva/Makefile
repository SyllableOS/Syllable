COPTS   = -I$(TOP)/src $(shell freetype-config --cflags) -c -O2 -Wall

DEVBIN = /system/drivers/appserver/video
OBJS =	riva.o riva_hw.o

OBJDIR	:= objs
OBJS	:= $(addprefix $(OBJDIR)/,$(OBJS))


#
#	rules
#

$(OBJDIR)/%.o : %.c
	$(CC) $(COPTS) $< -o $@

$(OBJDIR)/%.o : %.cpp
	$(CC) $(COPTS) $< -o $@


all :	objs $(OBJDIR)/riva

objs:
	mkdir objs

$(OBJDIR)/riva: $(OBJS)
	gcc -plugin $(OBJS) -o $(OBJDIR)/riva -lsyllable -lstdc++

deps:

install: $(OBJDIR)/riva
	cp -f $(OBJDIR)/riva $(DEVBIN)/riva

dist: $(OBJDIR)/riva
	cp -f $(OBJDIR)/riva $(DIST_DIR)/atheos/sys/drivers/appserver/video/riva

clean:
	-rm $(OBJS) $(OBJDIR)/riva
