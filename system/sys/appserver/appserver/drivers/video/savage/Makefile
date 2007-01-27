COPTS   = -I$(TOP)/src -I. $(shell freetype-config --cflags) -c

DEVBIN = /system/drivers/appserver/video
OBJS =	savage_driver.o savage_accel.o savage_video.o

OBJDIR	:= objs
OBJS	:= $(addprefix $(OBJDIR)/,$(OBJS))


#
#	rules
#

$(OBJDIR)/%.o : %.c
	$(CC) $(COPTS) $< -o $@

$(OBJDIR)/%.o : %.cpp
	$(CXX) $(COPTS) $< -o $@


all :	objs $(OBJDIR)/savage

objs:
	mkdir objs

$(OBJDIR)/savage: $(OBJS)
	$(CXX) -plugin -Xlinker -Bsymbolic -nodefaultlibs $(OBJS) -o $(OBJDIR)/savage

deps:

install: $(OBJDIR)/savage
	cp -f $(OBJDIR)/savage $(IMAGE)/$(DEVBIN)/savage

dist: install

clean:
	-rm $(OBJS) $(OBJDIR)/savage

