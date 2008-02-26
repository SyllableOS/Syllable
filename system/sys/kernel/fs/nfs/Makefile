CFLAGS   += -kernel -fno-PIC -c -Wno-multichar
AFLAGS   = -c

C_INCLUDE_PATH=$(ATHEOS_INCLUDE_PATH)

FS		:= network-fs
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



all :	objs $(OBJDIR)/$(FS)

objs:
	mkdir $(OBJDIR)

$(OBJDIR)/$(FS): $(OBJS) Makefile
	$(CC) -kernel $(OBJS) -o $(OBJDIR)/$(FS)

install: all
	cp -f $(OBJDIR)/$(FS) $(IMAGE)/system/drivers/fs/

dist: install

clean:
	rm -f $(OBJS) $(OBJDIR)/$(FS)
