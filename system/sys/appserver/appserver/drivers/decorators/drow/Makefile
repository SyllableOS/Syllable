COPTS   = -I../../../server $(shell freetype-config --cflags) -c -O3 -Wall

DEVBIN = /system/drivers/appserver/decorators
OBJS =	drowdecorator.o

OBJDIR	:= objs
OBJS	:= $(addprefix $(OBJDIR)/,$(OBJS))


#
#	rules
#

$(OBJDIR)/%.o : %.c
	$(CC) $(COPTS) $< -o $@

$(OBJDIR)/%.o : %.cpp
	$(CC) $(COPTS) $< -o $@


all :	objs $(OBJDIR)/Drow

deps:

objs:
	mkdir objs

$(OBJDIR)/Drow: $(OBJS)
	$(CXX) $(OBJS) -o $(OBJDIR)/Drow ../../../server/objs/appserver -lsyllable -lz
	$(CXX) -plugin $(OBJS) -o $(OBJDIR)/Drow -lsyllable

install: $(OBJDIR)/Drow
	cp -f $(OBJDIR)/Drow $(DEVBIN)/Drow

dist: $(OBJDIR)/Drow
	cp -f $(OBJDIR)/Drow $(DIST_DIR)/atheos/sys/drivers/appserver/decorators/Drow

clean:
	-rm $(OBJS) $(OBJDIR)/Drow

