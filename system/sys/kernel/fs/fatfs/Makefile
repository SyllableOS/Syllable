#SRCS= dosfs.c attr.c dir.c dlist.c fat.c file.c iter.c util.c vcache.c \
#	textencoding/encodings.cpp \
#	textencoding/EncodingConversions.cpp \
#	textencoding/UnicodeMappings.cpp \
#	version.c


CFLAGS   = -kernel -fno-PIC -c -O3 -Wall -Wno-multichar -fno-strict-aliasing -Itextencoding -I../
AFLAGS   = -c

#OBJS := afs.o balloc.o afs_inode.o attribs.o journal.o stream.o btree.o bt_delete.o

VPATH =	./ textencoding


OBJS := dosfs.o dir.o dlist.o fat.o file.o iter.o util.o vcache.o encodings.o EncodingConversions.o UnicodeMappings.o version.o
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
	$(CC) $(CFLAGS) $< -o $@

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
	cp -f $(OBJDIR)/fatfs /system/drivers/fs/fatfs

dist: all
	cp -f $(OBJDIR)/fatfs $(DIST_DIR)/atheos/sys/drivers/fs/fatfs

clean:
	rm $(OBJS)

