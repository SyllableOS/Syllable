CFLAGS   = -kernel -fno-PIC -c -I.
AFLAGS   = -c


OBJS := savage.o


OBJDIR	:= objs
OBJS	:= $(addprefix $(OBJDIR)/,$(OBJS))


#
#	rules
#

$(OBJDIR)/%.o : %.c
	$(CC) $(CFLAGS) $< -o $@

$(OBJDIR)/%.o : %.cpp
	$(CXX) $(CXXFLAGS) $< -o $@

$(OBJDIR)/%.o : %.s
	$(CC) $(AFLAGS) -x assembler-with-cpp $< -o $@

$(OBJDIR)/%.o : %.S
	$(CC) $(CFLAGS) -x assembler-with-cpp $< -o $@




all :	objs $(OBJDIR)/savage

objs:
	mkdir objs

$(OBJDIR)/savage: $(OBJS) Makefile
	$(CC) -kernel $(OBJS) -o $(OBJDIR)/savage

install: all
	mkdir -p /system/drivers/dev/graphics
	cp -f $(OBJDIR)/savage /system/drivers/dev/graphics

dist: all
	mkdir -p $(DIST_DIR)/atheos/sys/drivers/dev/graphics
	cp -f $(OBJDIR)/savage $(DIST_DIR)/atheos/sys/drivers/dev/graphics

clean:
	-rm -f $(OBJS) $(OBJDIR)/savage
