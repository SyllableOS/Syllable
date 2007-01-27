COPTS   = -I$(TOP)/src $(shell freetype-config --cflags) -c

DEVBIN = /system/drivers/appserver/video
OBJS =	nvidia.o riva_hw.o

OBJDIR	:= objs
OBJS	:= $(addprefix $(OBJDIR)/,$(OBJS))


#
#	rules
#

$(OBJDIR)/%.o : %.c
	$(CC) $(COPTS) $< -o $@

$(OBJDIR)/%.o : %.cpp
	$(CXX) $(COPTS) $< -o $@


all :	objs $(OBJDIR)/nvidia

objs:
	mkdir objs

$(OBJDIR)/nvidia: $(OBJS)
	$(CXX) -plugin $(OBJS) -o $(OBJDIR)/nvidia -lsyllable

deps:

install: $(OBJDIR)/nvidia
	cp -f $(OBJDIR)/nvidia $(IMAGE)/$(DEVBIN)/nvidia

dist: install

clean:
	-rm $(OBJS) $(OBJDIR)/nvidia

