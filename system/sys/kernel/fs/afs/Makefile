CFLAGS   += -kernel -D__LOADABLE__ -fno-PIC -c -fno-strict-aliasing
AFLAGS   = -c

C_INCLUDE_PATH=$(ATHEOS_INCLUDE_PATH)

OBJS := afs.o balloc.o afs_inode.o attribs.o index.o journal.o stream.o btree.o bt_delete.o


OBJDIR	:= objs
OBJS	:= $(addprefix $(OBJDIR)/,$(OBJS))
FS = afs

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




all : objs $(OBJDIR)/$(FS)

objs:
	mkdir objs

$(OBJDIR)/$(FS): $(OBJS) Makefile
	$(CC) -kernel $(OBJS) -o $(OBJDIR)/$(FS)

deps:

install: all
	cp -f $(OBJDIR)/$(FS) $(IMAGE)/system/drivers/fs/$(FS)

dist: install

clean:
	rm -f $(OBJS)

