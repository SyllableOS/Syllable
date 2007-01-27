CFLAGS   = -D__KERNEL__ -fno-PIC -c
AFLAGS   = -c

C_INCLUDE_PATH=$(ATHEOS_INCLUDE_PATH)

OBJS := driver.o cpialink.o pport.o isa_dma.o


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




all :	objs $(OBJDIR)/cpia

objs:
	mkdir objs

$(OBJDIR)/cpia: $(OBJS) Makefile
	$(CC) -nostdlib $(OBJS) -lgcc -lkernel -o $(OBJDIR)/cpia

install: all
	cp -f $(OBJDIR)/cpia $(IMAGE)/system/drivers/dev/video/cpia

dist: install

clean:
	-rm $(OBJS)

