CFLAGS   = -kernel -fno-PIC -c -O3

C_INCLUDE_PATH=/ainc

OBJS := tulip.o
DRIVER := tulip

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


all :	objs $(OBJDIR)/tulip

deps:

objs:
	mkdir objs

$(OBJDIR)/tulip: $(OBJS) Makefile
	gcc -kernel $(OBJS) -o $(OBJDIR)/$(DRIVER)

install: all
	cp -f $(OBJDIR)/tulip /system/drivers/dev/net/eth/tulip

dist: all
	cp -f $(OBJDIR)/$(DRIVER) $(DIST_DIR)/atheos/sys/drivers/dev/net/eth/$(DRIVER)

clean:
	-rm $(OBJS)
	-rm $(OBJDIR)/$(DRIVER)

