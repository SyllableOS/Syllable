CFLAGS   = -kernel -fno-PIC -O3 -c -Wall
AFLAGS   = -c

C_INCLUDE_PATH=$(ATHEOS_INCLUDE_PATH)

OBJS := ac97_codec.o vt82xx_io.o vt82xx.o
DRIVER := vt82xx

OBJDIR	:= objs
OBJS	:= $(addprefix $(OBJDIR)/,$(OBJS))


#
#	rules
#

$(OBJDIR)/%.o : %.c
	$(CC) $(CFLAGS) $< -o $@

$(OBJDIR)/%.o : %.cpp
	$(CC) $(CFLAGS) $< -o $@

$(OBJDIR)/%.o : %.s
	$(CC) $(AFLAGS) -x assembler-with-cpp $< -o $@

$(OBJDIR)/%.o : %.S
	$(CC) $(CFLAGS) -x assembler-with-cpp $< -o $@




all :	objs $(OBJDIR)/vt82xx

objs:
	mkdir objs

$(OBJDIR)/vt82xx: $(OBJS) Makefile
	$(CC) -kernel $(OBJS) -o $(OBJDIR)/vt82xx

install: all
	mkdir -p /system/drivers/dev/sound/
	cp -f $(OBJDIR)/vt82xx /system/drivers/dev/sound/vt82xx


dist: all
	cp -f $(OBJDIR)/$(DRIVER) $(DIST_DIR)/atheos/sys/drivers/dev/sound/$(DRIVER)

clean:
	rm -f $(OBJS)
	rm -f objs/vt82xx

