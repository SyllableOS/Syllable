CFLAGS   = -kernel -fno-PIC -c -O3 -Wall
AFLAGS   = -c

OBJS := ramdisk.o

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



all :	objs $(OBJDIR)/ramdisk

objs:
	mkdir objs

$(OBJDIR)/ramdisk: $(OBJS) Makefile
	gcc -kernel $(OBJS) -o $(OBJDIR)/ramdisk

install: all
	cp -f $(OBJDIR)/ramdisk /system/drivers/fs/ramdisk

dist: all
	cp -f $(OBJDIR)/ramdisk $(DIST_DIR)/system/drivers/fs/ramdisk

clean:
	-rm -f $(OBJS) $(OBJDIR)/ramdisk


