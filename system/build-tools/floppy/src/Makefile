CFLAGS	= -Wall -c -O2

CC	= gcc
LL	= g++

OBJDIR=objs
BINDIR=../bin

all: $(OBJDIR) $(BINDIR) $(OBJDIR)/makeramdisk $(OBJDIR)/splitramdisk
	@cp -f $(OBJDIR)/makeramdisk $(BINDIR)/makeramdisk
	@cp -f $(OBJDIR)/splitramdisk $(BINDIR)/splitramdisk

clean:
	@if [ -e $(OBJDIR) ];then rm -rf $(OBJDIR);fi;
	@if [ -e $(BINDIR)/makeramdisk ];then rm $(BINDIR)/makeramdisk;fi;
	@if [ -e $(BINDIR)/splitramdisk ];then rm $(BINDIR)/splitramdisk;fi;

$(OBJDIR):
	mkdir $(OBJDIR)

$(BINDIR):
	mkdir $(BINDIR)

$(OBJDIR)/makeramdisk: $(OBJDIR)/makeramdisk.o
	$(LL) $(OBJDIR)/makeramdisk.o -o $@

$(OBJDIR)/splitramdisk: $(OBJDIR)/splitramdisk.o
	$(LL) $(OBJDIR)/splitramdisk.o -o $@

$(OBJDIR)/%.o : %.c
	@$(CC) $(CFLAGS) $< -o $@
$(OBJDIR)/%.o : %.cpp
	@$(CC) $(CFLAGS) $< -o $@

$(OBJDIR)/makeramdisk.o:	makeramdisk.c
$(OBJDIR)/splitramdisk.o:	splitramdisk.c
