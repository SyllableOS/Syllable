COPTS   = -I../../../server -c

DEVBIN = /system/drivers/appserver/input
OBJS =	sermouse.o

OBJDIR	:= objs
OBJS	:= $(addprefix $(OBJDIR)/,$(OBJS))


#
#	rules
#

$(OBJDIR)/%.o : %.c
	$(CC) $(COPTS) $< -o $@

$(OBJDIR)/%.o : %.cpp
	$(CXX) $(COPTS) $< -o $@


all :	objs $(OBJDIR)/sermouse

objs:
	mkdir objs

$(OBJDIR)/sermouse: $(OBJS)
	$(CXX) -plugin $(OBJS) -o $(OBJDIR)/sermouse -lsyllable

deps:

install: $(OBJDIR)/sermouse
	cp -f $(OBJDIR)/sermouse $(DEVBIN)/sermouse

dist: $(OBJDIR)/sermouse
	cp -f $(OBJDIR)/sermouse $(DIST_DIR)/atheos/sys/drivers/appserver/input/sermouse

clean:
	-rm $(OBJS) $(OBJDIR)/sermouse

