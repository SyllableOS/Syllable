COPTS   = -I$(TOP)/src $(shell freetype-config --cflags) -c

DEVBIN = /system/drivers/appserver/video
OBJS =	tdfx.o gtf.o

OBJDIR	:= objs
OBJS	:= $(addprefix $(OBJDIR)/,$(OBJS))


#
#	rules
#

$(OBJDIR)/%.o : %.c
	$(CC) $(COPTS) $< -o $@

$(OBJDIR)/%.o : %.cpp
	$(CXX) $(COPTS) $< -o $@


all :	objs $(OBJDIR)/tdfx

objs:
	mkdir objs

$(OBJDIR)/tdfx: $(OBJS)
	$(CXX) -plugin $(OBJS) -o $(OBJDIR)/tdfx -lsyllable

deps:

install: $(OBJDIR)/tdfx
	cp -f $(OBJDIR)/tdfx $(DEVBIN)/tdfx

dist: $(OBJDIR)/tdfx
	cp -f $(OBJDIR)/tdfx $(DIST_DIR)/atheos/sys/drivers/appserver/video/tdfx

clean:
	-rm $(OBJS) $(OBJDIR)/tdfx

