COPTS   = -I$(TOP)/src $(shell freetype-config --cflags) -c -O2 -Wall

DEVBIN = /system/drivers/appserver/video
OBJS = init301.o init.o sis_video.o sis.o sis3xx.o

OBJDIR	:= objs
OBJS	:= $(addprefix $(OBJDIR)/,$(OBJS))


#
#	rules
#

$(OBJDIR)/%.o : %.c
	$(CC) $(COPTS) $< -o $@

$(OBJDIR)/%.o : %.cpp
	$(CC) $(COPTS) $< -o $@


all :	objs $(OBJDIR)/sis3xx

objs:
	mkdir objs

$(OBJDIR)/sis3xx: $(OBJS)
	$(CXX) -plugin $(OBJS) -o $(OBJDIR)/sis3xx -lsyllable

install: $(OBJDIR)/sis3xx
	cp -f $(OBJDIR)/sis3xx $(DEVBIN)/sis3xx


dist: $(OBJDIR)/sis3xx
	cp -f $(OBJDIR)/sis3xx $(DIST_DIR)/atheos/sys/drivers/appserver/video/sis3xx

clean:
	-rm $(OBJS) $(OBJDIR)/sis3xx

