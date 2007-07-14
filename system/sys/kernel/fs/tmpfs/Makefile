CFLAGS   = -kernel -fno-PIC -c
AFLAGS   = -c

OBJS := tmpfs.o

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



all :	objs $(OBJDIR)/tmpfs

objs:
	mkdir objs

$(OBJDIR)/tmpfs: $(OBJS) Makefile
	$(CC) -kernel $(OBJS) -o $(OBJDIR)/tmpfs

install: all
	cp -f $(OBJDIR)/tmpfs $(IMAGE)/system/drivers/fs/tmpfs

dist: install

clean:
	-rm -f $(OBJS) $(OBJDIR)/tmpfs
