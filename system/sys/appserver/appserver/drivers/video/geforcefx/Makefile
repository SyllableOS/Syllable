COPTS   = -I$(TOP)/src $(shell freetype-config --cflags) -c -O2 -Wall

DEVBIN = /system/drivers/appserver/video
OBJS =	fx_hw.o fx.o

OBJDIR	:= objs
OBJS	:= $(addprefix $(OBJDIR)/,$(OBJS))


#
#	rules
#

$(OBJDIR)/%.o : %.c
	$(CC) $(COPTS) $< -o $@

$(OBJDIR)/%.o : %.cpp
	$(CC) $(COPTS) $< -o $@


all :	objs $(OBJDIR)/geforcefx

objs:
	mkdir objs

$(OBJDIR)/geforcefx: $(OBJS)
	gcc -plugin $(OBJS) -o $(OBJDIR)/geforcefx -lsyllable -lstdc++

deps:

install: $(OBJDIR)/geforcefx
	cp -f $(OBJDIR)/geforcefx $(DEVBIN)/geforcefx

dist: $(OBJDIR)/geforcefx
	cp -f $(OBJDIR)/geforcefx $(DIST_DIR)/atheos/sys/drivers/appserver/video/geforcefx

clean:
	-rm $(OBJS) $(OBJDIR)/geforcefx


