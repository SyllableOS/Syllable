
CFLAGS   = -kernel -fno-PIC -c -O3 -Wall

C_INCLUDE_PATH=/ainc

OBJS := ne2000.o


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


all :	objs $(OBJDIR)/ne2000

objs:
	mkdir objs

$(OBJDIR)/ne2000: $(OBJS) Makefile
	gcc -kernel $(OBJS) -o $(OBJDIR)/ne2000

install: all
	cp -f $(OBJDIR)/ne2000 /system/drivers/dev/net/eth/8390

dist: all
	cp -f $(OBJDIR)/ne2000 $(DIST_DIR)/atheos/sys/drivers/dev/net/eth/8390

clean:
	-rm -f $(OBJS) $(OBJDIR)/ne2000


