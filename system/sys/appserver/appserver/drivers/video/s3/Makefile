COPTS   = -I$(TOP)/src  -c -O2 -Wall

DEVBIN = /system/drivers/appserver/video
OBJS =	virgedrv.o

OBJDIR	:= objs
OBJS	:= $(addprefix $(OBJDIR)/,$(OBJS))


#
#	rules
#

$(OBJDIR)/%.o : %.c
	$(CC) $(COPTS) $< -o $@

$(OBJDIR)/%.o : %.cpp
	$(CC) $(COPTS) $< -o $@


all :	objs $(OBJDIR)/s3_virge

objs:
	mkdir objs

$(OBJDIR)/s3_virge: $(OBJS)
	gcc -plugin $(OBJS) -o $(OBJDIR)/s3_virge -latheos -lstdc++

deps:

install: $(OBJDIR)/s3_virge
	cp -f $(OBJDIR)/s3_virge $(DEVBIN)/s3_virge

dist: $(OBJDIR)/s3_virge
	cp -f $(OBJDIR)/s3_virge $(DIST_DIR)/atheos/sys/drivers/appserver/video/s3_virge

clean:
	-rm $(OBJS) $(OBJDIR)/s3_virge
