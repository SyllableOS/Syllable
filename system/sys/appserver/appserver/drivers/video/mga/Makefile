COPTS   = -I$(TOP)/src  -c -O2 -Wall -fexceptions

DEVBIN = /system/drivers/appserver/video
OBJS =	mga.o gx00_crtc.o gx00_crtc1.o

OBJDIR	:= objs
OBJS	:= $(addprefix $(OBJDIR)/,$(OBJS))


#
#	rules
#

$(OBJDIR)/%.o : %.c
	$(CC) $(COPTS) $< -o $@

$(OBJDIR)/%.o : %.cpp
	$(CC) $(COPTS) $< -o $@


all :	objs $(OBJDIR)/mga_generic

objs:
	mkdir objs

$(OBJDIR)/mga_generic: $(OBJS)
	g++ -plugin $(OBJS) -o $(OBJDIR)/mga_generic -latheos

deps:

install: $(OBJDIR)/mga_generic
	cp -f $(OBJDIR)/mga_generic $(DEVBIN)/mga_generic

dist: $(OBJDIR)/mga_generic
	cp -f $(OBJDIR)/mga_generic $(DIST_DIR)/atheos/sys/drivers/appserver/video/mga_generic

clean:
	-rm $(OBJS) $(OBJDIR)/mga_generic
