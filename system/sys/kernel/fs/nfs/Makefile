CFLAGS   += -kernel -fno-PIC -c -Wno-multichar
AFLAGS   = -c

C_INCLUDE_PATH=$(ATHEOS_INCLUDE_PATH)

FS		:= nfs
OBJS	:= nfs.o gcc34_memcpy.o

OBJDIR	:= objs
OBJS	:= $(addprefix $(OBJDIR)/,$(OBJS))


#
#	rules
#

$(OBJDIR)/%.o : %.c
	$(CC) $(CFLAGS) $< -o $@

$(OBJDIR)/%.o : %.cpp
	$(CXX) $(CXXFLAGS) $< -o $@

$(OBJDIR)/%.o : %.s
	$(CC) $(AFLAGS) -x assembler-with-cpp $< -o $@

$(OBJDIR)/%.o : %.S
	$(CC) $(CFLAGS) -x assembler-with-cpp $< -o $@



all :	objs $(OBJDIR)/nfs

objs:
	mkdir objs

$(OBJDIR)/nfs: $(OBJS) Makefile
	$(CC) -kernel $(OBJS) -o $(OBJDIR)/nfs

install: all
	cp -f $(OBJDIR)/nfs $(IMAGE)/system/drivers/fs/nfs

dist: install

clean:
	rm -f $(OBJS) $(OBJDIR)/nfs
