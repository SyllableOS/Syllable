COPTS   = -I$(TOP)/src $(shell freetype-config --cflags) -c

DEVBIN = /system/drivers/appserver/video
OBJS = init301.o init.o sis_video.o sis.o sis3xx.o

OBJDIR	:= objs
OBJS	:= $(addprefix $(OBJDIR)/,$(OBJS))


#
#	rules
#

$(OBJDIR)/%.o : %.c
	$(CC) $(COPTS) $< -o $@

$(OBJDIR)/%.o : %.cpp
	$(CXX) $(COPTS) $< -o $@


all :	objs $(OBJDIR)/sis3xx

objs:
	mkdir objs

$(OBJDIR)/sis3xx: $(OBJS)
	$(CXX) -plugin -Xlinker -Bsymbolic -nodefaultlibs $(OBJS) -o $(OBJDIR)/sis3xx

install: $(OBJDIR)/sis3xx
	cp -f $(OBJDIR)/sis3xx $(IMAGE)/$(DEVBIN)/sis3xx


dist: install

clean:
	-rm $(OBJS) $(OBJDIR)/sis3xx

