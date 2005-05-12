CFLAGS   += -kernel -fno-PIC -c
AFLAGS   = -c

C_INCLUDE_PATH=$(ATHEOS_INCLUDE_PATH)

OBJS := driver.o


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




all :	objs $(OBJDIR)/tdfx

objs:
	mkdir objs

$(OBJDIR)/tdfx: $(OBJS) Makefile
	$(CC) -kernel $(OBJS) -o $(OBJDIR)/tdfx

install: all
	mkdir -p /system/drivers/dev/graphics
	cp -f $(OBJDIR)/tdfx /system/drivers/dev/graphics

dist: all
	mkdir -p $(DIST_DIR)/atheos/sys/drivers/dev/graphics
	cp -f $(OBJDIR)/tdfx $(DIST_DIR)/atheos/sys/drivers/dev/graphics

clean:
	-rm -f $(OBJS) $(OBJDIR)/tdfx

