CFLAGS   += -c -kernel
AFLAGS   = -c

C_INCLUDE_PATH=$(ATHEOS_INCLUDE_PATH)

OBJS := ramfs.o


OBJDIR	:= objs
OBJS	:= $(addprefix $(OBJDIR)/,$(OBJS))
FS = ramfs

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
	$(CC) $(OBJS) -kernel -o $(OBJDIR)/$(FS)

$(OBJDIR)/ramfs.o: ramfs.c
	$(CC) $(CFLAGS) -fno-PIC $< -o $@

deps:

install: all
	install -s $(OBJDIR)/$(FS) $(IMAGE)/system/drivers/fs/$(FS)

dist: install

clean:
	rm -f $(OBJS)

