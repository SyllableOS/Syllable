COPTS   = -I../../../server -c

DEVBIN = /system/drivers/appserver/input
OBJS =	ps2mouse.o

OBJDIR	:= objs
OBJS	:= $(addprefix $(OBJDIR)/,$(OBJS))


#
#	rules
#

$(OBJDIR)/%.o : %.c
	$(CC) $(COPTS) $< -o $@

$(OBJDIR)/%.o : %.cpp
	$(CXX) $(COPTS) $< -o $@


all :	objs $(OBJDIR)/ps2mouse

objs:
	mkdir objs

$(OBJDIR)/ps2mouse: $(OBJS)
	$(CXX) -plugin -Xlinker -Bsymbolic -nodefaultlibs $(OBJS) -o $(OBJDIR)/ps2mouse -lsyllable

deps:

install: $(OBJDIR)/ps2mouse
	cp -f $(OBJDIR)/ps2mouse $(IMAGE)/$(DEVBIN)/ps2mouse

dist: install

clean:
	-rm $(OBJS) $(OBJDIR)/ps2mouse

