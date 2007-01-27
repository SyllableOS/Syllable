COPTS   = -I$(TOP)/src $(shell freetype-config --cflags) -c

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
	$(CXX) $(COPTS) $< -o $@


all :	objs $(OBJDIR)/geforcefx

objs:
	mkdir objs

$(OBJDIR)/geforcefx: $(OBJS)
	$(CXX) -plugin -Xlinker -Bsymbolic -nodefaultlibs $(OBJS) -o $(OBJDIR)/geforcefx 

deps:

install: $(OBJDIR)/geforcefx
	cp -f $(OBJDIR)/geforcefx $(IMAGE)/$(DEVBIN)/geforcefx

dist: install

clean:
	-rm $(OBJS) $(OBJDIR)/geforcefx

