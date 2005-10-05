COPTS   = -I$(TOP)/src $(shell freetype-config --cflags) -c

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
	$(CXX) $(COPTS) $< -o $@


all :	objs $(OBJDIR)/s3_virge

objs:
	mkdir objs

$(OBJDIR)/s3_virge: $(OBJS)
	$(CXX) -plugin -Xlinker -Bsymbolic -nodefaultlibs $(OBJS) -o $(OBJDIR)/s3_virge

deps:

install: $(OBJDIR)/s3_virge
	cp -f $(OBJDIR)/s3_virge $(DEVBIN)/s3_virge

dist: $(OBJDIR)/s3_virge
	cp -f $(OBJDIR)/s3_virge $(DIST_DIR)/atheos/sys/drivers/appserver/video/s3_virge

clean:
	-rm $(OBJS) $(OBJDIR)/s3_virge

