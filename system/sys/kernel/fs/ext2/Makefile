CFLAGS   = -kernel -fno-PIC -c -O3 -Wall -Wno-multichar -I../
AFLAGS   = -c

FS		:= ext2
OBJS	:= ext2.o

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



all :	objs $(OBJDIR)/ext2

objs:
	mkdir objs

$(OBJDIR)/ext2: $(OBJS) Makefile
	$(CC) -kernel $(OBJS) -o $(OBJDIR)/ext2

install: all
	cp -f $(OBJDIR)/ext2 /system/drivers/fs/ext2

dist: all
	cp -f $(OBJDIR)/$(FS) $(DIST_DIR)/atheos/sys/drivers/fs/$(FS)

clean:
	rm -f $(OBJS) $(OBJDIR)/ext2

