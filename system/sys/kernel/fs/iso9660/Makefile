CFLAGS   = -kernel -fno-PIC -c -O3 -Wall -Wno-multichar -I../
AFLAGS   = -c

OLD_INCLUDE_PATH:=$(C_INCLUDE_PATH)
C_INCLUDE_PATH=/ainc
CPLUS_INCLUDE_PATH=/ainc


OBJS := iso.o kernel_interface.o

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



all :	objs $(OBJDIR)/iso9660

objs:
	mkdir objs

$(OBJDIR)/iso9660: $(OBJS) Makefile
	gcc -kernel $(OBJS) -o $(OBJDIR)/iso9660

install: all
	cp -f $(OBJDIR)/iso9660 /system/drivers/fs/iso9660

clean:
	rm $(OBJS) $(OBJDIR)/iso9660







