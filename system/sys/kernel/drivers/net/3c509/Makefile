
CFLAGS   = -D__KERNEL__ -fno-PIC -c -O3 -Wall

C_INCLUDE_PATH=/system/include

OBJS := 3c509.o

DRIVER := 3c509

OBJDIR	:= objs
OBJS	:= $(addprefix $(OBJDIR)/,$(OBJS))

$(OBJDIR)/%.o : %.c
	$(CC) $(CFLAGS) $< -o $@

$(OBJDIR)/%.o : %.cpp
	$(CC) $(CFLAGS) $< -o $@


$(OBJDIR)/%.o : %.s
	$(CC) $(CFLAGS) -x assembler-with-cpp $< -o $@

$(OBJDIR)/%.o : %.S
	$(CC) $(CFLAGS) -x assembler-with-cpp $< -o $@


all :	objs $(OBJDIR)/$(DRIVER)

deps:

objs:
	mkdir objs

$(OBJDIR)/$(DRIVER): $(OBJS) Makefile
	gcc -nostdlib $(OBJS) -lkernel -o $(OBJDIR)/$(DRIVER)

install: all
	cp -f $(OBJDIR)/$(DRIVER) /system/drivers/dev/net/eth/$(DRIVER)

dist: all
	cp -f $(OBJDIR)/$(DRIVER) $(DIST_DIR)/atheos/sys/drivers/dev/net/eth/$(DRIVER)

clean:
	-rm -f $(OBJS)
	-rm -f $(OBJDIR)/$(DRIVER)


