COPTS   = -I../../../server $(shell freetype-config --cflags) -c

DEVBIN = /system/drivers/appserver/decorators
OBJS =	PhotonDecorator.o

OBJDIR	:= objs
OBJS	:= $(addprefix $(OBJDIR)/,$(OBJS))


#
#	rules
#

$(OBJDIR)/%.o : %.c
	$(CC) $(COPTS) $< -o $@

$(OBJDIR)/%.o : %.cpp
	$(CXX) $(COPTS) $< -o $@


all :	objs $(OBJDIR)/Photon

deps:

objs:
	mkdir objs

$(OBJDIR)/Photon: $(OBJS)
	$(CXX) $(OBJS) -o $(OBJDIR)/Photon ../../../server/objs/appserver -lsyllable -lz
	$(CXX) -plugin $(OBJS) -o $(OBJDIR)/Photon -lsyllable

install: $(OBJDIR)/Photon
	cp -f $(OBJDIR)/Photon $(DEVBIN)/Photon

dist: $(OBJDIR)/Photon
	cp -f $(OBJDIR)/Photon $(DIST_DIR)/atheos/sys/drivers/appserver/decorators/Photon

clean:
	-rm $(OBJS) $(OBJDIR)/Photon

