include ../freetype.makefile

COPTS   = -I../../../server -I$(FREETYPEINCLUDE) -c -O3 -Wall

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
	$(CC) $(COPTS) $< -o $@


all :	objs $(OBJDIR)/Photon

deps:

objs:
	mkdir objs

$(OBJDIR)/Photon: $(OBJS)
	gcc $(OBJS) -o $(OBJDIR)/Photon /system/appserver -lsyllable -lstdc++ -lz
	gcc -plugin $(OBJS) -o $(OBJDIR)/Photon -lsyllable -lstdc++

install: $(OBJDIR)/Photon
	cp -f $(OBJDIR)/Photon $(DEVBIN)/Photon

dist: $(OBJDIR)/Photon
	cp -f $(OBJDIR)/Photon $(DIST_DIR)/atheos/sys/drivers/appserver/decorators/Photon

clean:
	-rm $(OBJS) $(OBJDIR)/Photon


