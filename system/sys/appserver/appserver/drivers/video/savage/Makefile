# include ../freetype.makefile

FREETYPEINCLUDE = $(shell freetype-config --cflags)

COPTS   = -I$(TOP)/src $(FREETYPEINCLUDE) -c 

DEVBIN = /system/drivers/appserver/video
OBJS =	savagedrv.o

OBJDIR	:= objs
OBJS	:= $(addprefix $(OBJDIR)/,$(OBJS))


#
#	rules
#

$(OBJDIR)/%.o : %.c
	$(CC) $(COPTS) $< -o $@

$(OBJDIR)/%.o : %.cpp
	$(CC) $(COPTS) $< -o $@


all :	objs $(OBJDIR)/s3_savage

objs:
	mkdir objs

$(OBJDIR)/s3_savage: $(OBJS)
	gcc -plugin $(OBJS) -o $(OBJDIR)/s3_savage -lsyllable -lstdc++

deps:

install: $(OBJDIR)/s3_savage
	cp -f $(OBJDIR)/s3_savage $(DEVBIN)/s3_savage

dist: $(OBJDIR)/s3_savage
	cp -f $(OBJDIR)/s3_savage $(DIST_DIR)/atheos/sys/drivers/appserver/video/s3_savage

clean:
	-rm $(OBJS) $(OBJDIR)/s3_savage
