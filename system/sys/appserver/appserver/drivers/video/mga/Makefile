COPTS   = -I$(TOP)/src $(shell freetype-config --cflags) -c -fexceptions

DEVBIN = /system/drivers/appserver/video
OBJS =	mga.o mga_crtc.o mga_gx00.o mga_tvp3026.o

OBJDIR	:= objs
OBJS	:= $(addprefix $(OBJDIR)/,$(OBJS))

DRIVER := $(OBJDIR)/mga

#
#	rules
#

$(OBJDIR)/%.o : %.c
	$(CC) $(COPTS) $< -o $@

$(OBJDIR)/%.o : %.cpp
	$(CXX) $(COPTS) $< -o $@


all :	objs $(DRIVER)

objs:
	mkdir objs

$(DRIVER): $(OBJS)
	$(CXX) -plugin -Xlinker -Bsymbolic -nodefaultlibs $(OBJS) -o $(DRIVER)

deps:

install: $(DRIVER)
	cp -f $(DRIVER) $(IMAGE)/$(DEVBIN)/mga

dist: install

clean:
	-rm $(OBJS) $(DRIVER)

