CFLAGS   = -kernel -fno-PIC -c
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
	$(CXX) $(CFLAGS) $< -o $@

$(OBJDIR)/%.o : %.s
	$(CC) $(AFLAGS) -x assembler-with-cpp $< -o $@

$(OBJDIR)/%.o : %.S
	$(CC) $(CFLAGS) -x assembler-with-cpp $< -o $@



all :	objs $(OBJDIR)/ramdisk

objs:
	mkdir objs

$(OBJDIR)/ramdisk: $(OBJS) Makefile
	$(CC) -kernel $(OBJS) -o $(OBJDIR)/ramdisk

install: all
	cp -f $(OBJDIR)/ramdisk $(IMAGE)/system/drivers/fs/ramdisk

dist: install

clean:
	-rm -f $(OBJS) $(OBJDIR)/ramdisk
