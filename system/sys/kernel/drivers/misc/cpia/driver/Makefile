
CFLAGS   = -D__KERNEL__ -fno-PIC -c -O2 -Wall
AFLAGS   = -c


OBJS := driver.o cpialink.o pport.o isa_dma.o


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




all :	objs $(OBJDIR)/cpia

objs:
	mkdir objs

$(OBJDIR)/cpia: $(OBJS) Makefile
	gcc -nostdlib $(OBJS) -lgcc -lkernel -o $(OBJDIR)/cpia

install: all
	cp -f $(OBJDIR)/cpia /system/drivers/dev/video/cpia

dist: all
	cp -f $(OBJDIR)/cpia $(DIST_DIR)/atheos/sys/drivers/dev/video/cpia

clean:
	-rm $(OBJS)

