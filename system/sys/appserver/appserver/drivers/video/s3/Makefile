COPTS   = -I$(TOP)/src $(shell freetype-config --cflags) -c

DEVBIN = /system/drivers/appserver/video
OBJS =	virgedrv.o

OBJDIR	:= objs
OBJS	:= $(addprefix $(OBJDIR)/,$(OBJS))


#
#	rules
#

$(OBJDIR)/%.o : %.c
	$(CC) $(COPTS) $< -o $@

$(OBJDIR)/%.o : %.cpp
	$(CXX) $(COPTS) $< -o $@


all :	objs $(OBJDIR)/s3_virge

objs:
	mkdir objs

$(OBJDIR)/s3_virge: $(OBJS)
	$(CXX) -plugin -Xlinker -Bsymbolic -nodefaultlibs $(OBJS) -o $(OBJDIR)/s3_virge

deps:

install: $(OBJDIR)/s3_virge
	cp -f $(OBJDIR)/s3_virge $(IMAGE)/$(DEVBIN)/s3_virge

dist: install

clean:
	-rm $(OBJS) $(OBJDIR)/s3_virge

