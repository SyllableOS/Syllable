COPTS   = -I$(TOP)/src $(shell freetype-config --cflags) -c

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
	$(CXX) $(COPTS) $< -o $@


all :	objs $(OBJDIR)/riva

objs:
	mkdir objs

$(OBJDIR)/riva: $(OBJS)
	$(CXX) -plugin -Xlinker -Bsymbolic -nodefaultlibs $(OBJS) -o $(OBJDIR)/riva

deps:

install: $(OBJDIR)/riva
	cp -f $(OBJDIR)/riva $(DEVBIN)/riva

dist: $(OBJDIR)/riva
	cp -f $(OBJDIR)/riva $(DIST_DIR)/atheos/sys/drivers/appserver/video/riva

clean:
	-rm $(OBJS) $(OBJDIR)/riva

