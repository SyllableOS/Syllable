include ../freetype.makefile

COPTS   = -I$(TOP)/src -I$(FREETYPEINCLUDE) -c -O2 -Wall -fexceptions

DEVBIN = /system/drivers/appserver/video
OBJS =	mga.o mga_crtc.o mga_gx00.o mga_tvp3026.o

OBJDIR	:= objs
OBJS	:= $(addprefix $(OBJDIR)/,$(OBJS))

DRIVER := $(OBJDIR)/mga

#
#	rules
#

$(OBJDIR)/%.o : %.c
	$(CC) $(COPTS) $< -o $@

$(OBJDIR)/%.o : %.cpp
	$(CC) $(COPTS) $< -o $@


all :	objs $(DRIVER)

objs:
	mkdir objs

$(DRIVER): $(OBJS)
	g++ -plugin -latheos $(OBJS) -o $(DRIVER)

deps:

install: $(DRIVER)
	cp -f $(DRIVER) $(DEVBIN)/mga

dist: $(DRIVER)
	cp -f $(DRIVER) $(DIST_DIR)/atheos/sys/drivers/appserver/video/

clean:
	-rm $(OBJS) $(DRIVER)

