COPTS   = -I$(TOP)/src $(shell freetype-config --cflags) -c

DEVBIN = /system/drivers/appserver/video
OBJS =	i855.o i855_video.o

OBJDIR	:= objs
OBJS	:= $(addprefix $(OBJDIR)/,$(OBJS))


#
#	rules
#

$(OBJDIR)/%.o : %.c
	$(CC) $(COPTS) $< -o $@

$(OBJDIR)/%.o : %.cpp
	$(CXX) $(COPTS) $< -o $@


all :	objs $(OBJDIR)/i855

objs:
	mkdir objs

$(OBJDIR)/i855: $(OBJS)
	$(CXX) -plugin -Xlinker -Bsymbolic -nodefaultlibs $(OBJS) -o $(OBJDIR)/i855

deps:

install: $(OBJDIR)/i855
	cp -f $(OBJDIR)/i855 $(IMAGE)/$(DEVBIN)/i855

dist: install

clean:
	-rm $(OBJS) $(OBJDIR)/i855

