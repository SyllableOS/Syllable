DRIVERNAME = ide
OBJS := ide_device.o ide.o


CFLAGS   = -D__KERNEL__ -D__IDE_EXTERNAL -fno-PIC -c -O3 -Wall
AFLAGS   = -c


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




all :	objs $(OBJDIR)/$(DRIVERNAME)

objs:
	mkdir objs

$(OBJDIR)/$(DRIVERNAME): $(OBJS) makefile
	gcc -nostdlib $(OBJS) -lgcc -lkernel -o $(OBJDIR)/$(DRIVERNAME)

install: all
	cp -f $(OBJDIR)/$(DRIVERNAME) /altos/sys/drivers/dev/disk/$(DRIVERNAME)


clean:
	rm $(OBJS)

