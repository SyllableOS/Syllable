CFLAGS	= -Wall -c -O2

CC	= gcc
LL	= g++

OBJDIR=objs
BINDIR=../../bin

deps: $(OBJDIR) $(OBJDIR)/makeramdisk $(OBJDIR)/splitramdisk $(OBJDIR)/rdloader
	@cp -f $(OBJDIR)/makeramdisk $(BINDIR)/makeramdisk
	@cp -f $(OBJDIR)/splitramdisk $(BINDIR)/splitramdisk

dist:
	@echo Nothing to be done for $(PWD)

install: $(OBJDIR)/makeramdisk $(OBJDIR)/splitramdisk
	install -s $(OBJDIR)/makeramdisk /bin/makeramdisk
	install -s $(OBJDIR)/splitramdisk /bin/splitramdisk

clean:
	@if [ -e $(OBJDIR) ];then rm -rf $(OBJDIR);fi;
	@if [ -e $(BINDIR)/makeramdisk ];then rm $(BINDIR)/makeramdisk;fi;
	@if [ -e $(BINDIR)/splitramdisk ];then rm $(BINDIR)/splitramdisk;fi;

$(OBJDIR):
	mkdir $(OBJDIR)

$(OBJDIR)/rdloader : $(OBJDIR)/rdloader.o
	$(LL) $(OBJDIR)/rdloader.o -latheos -lz -o $@

$(OBJDIR)/makeramdisk: $(OBJDIR)/makeramdisk.o
	$(LL) $(OBJDIR)/makeramdisk.o -o $@

$(OBJDIR)/splitramdisk: $(OBJDIR)/splitramdisk.o
	$(LL) $(OBJDIR)/splitramdisk.o -o $@

$(OBJDIR)/%.o : %.c
	@$(CC) $(CFLAGS) $< -o $@
$(OBJDIR)/%.o : %.cpp
	@$(CC) $(CFLAGS) $< -o $@

$(OBJDIR)/rdloader.o:		rdloader.cpp
$(OBJDIR)/makeramdisk.o:	makeramdisk.c
$(OBJDIR)/splitramdisk.o:	splitramdisk.c
