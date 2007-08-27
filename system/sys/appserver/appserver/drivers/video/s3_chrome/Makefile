COPTS   = -I$(TOP)/src $(shell freetype-config --cflags) -c

DEVBIN = /system/drivers/appserver/video
OBJS =	s3_chrome.o

OBJDIR	:= objs
OBJS	:= $(addprefix $(OBJDIR)/,$(OBJS))


#
#	rules
#

$(OBJDIR)/%.o : %.c
	$(CC) $(COPTS) $< -o $@

$(OBJDIR)/%.o : %.cpp
	$(CXX) $(COPTS) $< -o $@


all :	objs $(OBJDIR)/s3_chrome

objs:
	mkdir objs

$(OBJDIR)/s3_chrome: $(OBJS)
	$(CXX) -plugin -Xlinker -Bsymbolic -nodefaultlibs $(OBJS) -o $(OBJDIR)/s3_chrome

deps:

install: $(OBJDIR)/s3_chrome
	cp -f $(OBJDIR)/s3_chrome $(IMAGE)/$(DEVBIN)/s3_chrome

dist: install

clean:
	-rm $(OBJS) $(OBJDIR)/s3_chrome



