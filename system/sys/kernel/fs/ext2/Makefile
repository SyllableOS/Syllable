CFLAGS   = -kernel -fno-PIC -c -O3 -Wall -Wno-multichar -I../
AFLAGS   = -c

OLD_INCLUDE_PATH:=$(C_INCLUDE_PATH)
C_INCLUDE_PATH=/ainc
CPLUS_INCLUDE_PATH=/ainc


OBJS := ext2.o

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



all :	objs $(OBJDIR)/ext2

objs:
	mkdir objs

$(OBJDIR)/ext2: $(OBJS) Makefile
	gcc -kernel $(OBJS) -o $(OBJDIR)/ext2

install: all
	cp -f $(OBJDIR)/ext2 /system/drivers/fs/ext2

clean:
	rm $(OBJS) $(OBJDIR)/ext2







