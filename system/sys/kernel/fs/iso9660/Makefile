CFLAGS   += -kernel -fno-PIC -c -Wno-multichar -fno-strict-aliasing -I../
AFLAGS   = -c

C_INCLUDE_PATH=$(ATHEOS_INCLUDE_PATH)

FS		:= iso9660
OBJS	:= iso.o kernel_interface.o

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



all :	objs $(OBJDIR)/iso9660

objs:
	mkdir objs

$(OBJDIR)/iso9660: $(OBJS) Makefile
	$(CC) -kernel $(OBJS) -o $(OBJDIR)/iso9660

install: all
	cp -f $(OBJDIR)/iso9660 /system/drivers/fs/iso9660

dist: all
	cp -f $(OBJDIR)/$(FS) $(DIST_DIR)/atheos/sys/drivers/fs/$(FS)

clean:
	rm -f $(OBJS) $(OBJDIR)/iso9660

