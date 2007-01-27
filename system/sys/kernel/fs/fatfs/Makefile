CFLAGS   += -kernel -fno-PIC -c -Wno-multichar -fno-strict-aliasing -Itextencoding -I../
AFLAGS   = -c

C_INCLUDE_PATH=$(ATHEOS_INCLUDE_PATH)

#OBJS := afs.o balloc.o afs_inode.o attribs.o journal.o stream.o btree.o bt_delete.o

VPATH =	./ textencoding


OBJS := dosfs.o dir.o dlist.o fat.o file.o iter.o util.o vcache.o encodings.o EncodingConversions.o UnicodeMappings.o version.o attr.o
#       dosfs.o dir.o dlist.o fat.o file.o iter.o util.o vcache.o encodings.o EncodingConversions.o UnicodeMappings.o version.o
#  attr.o

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




all :	objs $(OBJDIR)/fatfs

objs:
	mkdir objs

$(OBJDIR)/fatfs: $(OBJS) Makefile
	$(CC) -kernel $(OBJS) -o $(OBJDIR)/fatfs

deps:

install: all
	cp -f $(OBJDIR)/fatfs $(IMAGE)/system/drivers/fs/fatfs

dist: install

clean:
	rm -f $(OBJS) $(OBJDIR)/fatfs

