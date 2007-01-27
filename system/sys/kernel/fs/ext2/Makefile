CFLAGS   += -kernel -fno-PIC -c -Wno-multichar -I../
AFLAGS   = -c

C_INCLUDE_PATH=$(ATHEOS_INCLUDE_PATH)

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
	$(CXX) $(CFLAGS) $< -o $@

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
	cp -f $(OBJDIR)/ext2 $(IMAGE)/system/drivers/fs/ext2

dist: install

clean:
	rm -f $(OBJS) $(OBJDIR)/ext2

