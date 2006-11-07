CFLAGS   += -kernel -fno-PIC -c
AFLAGS   = -c

C_INCLUDE_PATH=$(ATHEOS_INCLUDE_PATH)

OBJS := lp.o


OBJDIR	:= objs
OBJS	:= $(addprefix $(OBJDIR)/,$(OBJS))


#
#	rules
#

$(OBJDIR)/%.o : %.c
	$(CC) $(CFLAGS) $< -o $@

$(OBJDIR)/%.o : %.cpp
	$(CXX) $(CFLAGS) $< -o $@

$(OBJDIR)/%.o : %.s
	$(CC) $(AFLAGS) -x assembler-with-cpp $< -o $@

$(OBJDIR)/%.o : %.S
	$(CC) $(CFLAGS) -x assembler-with-cpp $< -o $@




all :	objs $(OBJDIR)/lp      

objs:
	mkdir objs

$(OBJDIR)/lp: $(OBJS) Makefile
	$(CC) -kernel $(OBJS) -o $(OBJDIR)/lp      

install: all
	-mkdir -p /system/drivers/dev/printer
	cp -f $(OBJDIR)/lp /system/drivers/dev/printer/lp

dist: all
	-mkdir -p $(DIST_DIR)/atheos/sys/drivers/dev/printer
	cp -f $(OBJDIR)/lp $(DIST_DIR)/atheos/sys/drivers/dev/printer/lp

clean:
	-rm -f $(OBJS) $(OBJDIR)/lp      


