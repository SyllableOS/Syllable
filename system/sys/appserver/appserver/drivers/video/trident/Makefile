COPTS   = -I$(TOP)/src $(shell freetype-config --cflags) -c

DEVBIN = /system/drivers/appserver/video
OBJS =	trident.o

OBJDIR	:= objs
OBJS	:= $(addprefix $(OBJDIR)/,$(OBJS))


#
#	rules
#

$(OBJDIR)/%.o : %.c
	$(CC) $(COPTS) $< -o $@

$(OBJDIR)/%.o : %.cpp
	$(CXX) $(COPTS) $< -o $@


all :	objs $(OBJDIR)/trident

deps:

objs:
	mkdir objs

$(OBJDIR)/trident: $(OBJS)
	$(CXX) -plugin -Xlinker -Bsymbolic -nodefaultlibs $(OBJS) -o $(OBJDIR)/trident

install: $(OBJDIR)/trident
	cp -f $(OBJDIR)/trident $(IMAGE)/$(DEVBIN)/trident

dist: install

clean:
	-rm $(OBJS) $(OBJDIR)/trident

