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
	$(CXX) -plugin -Xlinker -Bsymbolic $(OBJS) -nodefaultlibs -o $(OBJDIR)/Photon

install: $(OBJDIR)/Photon
	cp -f $(OBJDIR)/Photon $(IMAGE)/$(DEVBIN)/Photon

dist: install

clean:
	-rm $(OBJS) $(OBJDIR)/Photon

