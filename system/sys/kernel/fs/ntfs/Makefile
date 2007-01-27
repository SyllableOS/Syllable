CFLAGS   += -I. -c -fno-strict-aliasing -kernel -D__ENABLE_DEBUG__

C_INCLUDE_PATH=$(ATHEOS_INCLUDE_PATH)

FS		:= ntfs
OBJS	:= ntfs.o util.o inode.o attr.o unistr.o support.o super.o dir.o

OBJDIR	:= objs
OBJS	:= $(addprefix $(OBJDIR)/,$(OBJS))


#
#	rules
#

$(OBJDIR)/%.o : %.c
	$(CC) $(CFLAGS) $< -o $@


all :	objs $(OBJDIR)/$(FS)

objs:
	mkdir objs

$(OBJDIR)/$(FS): $(OBJS) Makefile
	$(CC) -kernel $(OBJS) -o $(OBJDIR)/$(FS)

install: all
	cp -f $(OBJDIR)/$(FS) $(IMAGE)/system/drivers/fs/$(FS)

dist: install

clean:
	rm -f $(OBJS) $(OBJDIR)/$(FS)

