CFLAGS   = -kernel -fno-PIC -c -O3
# -Wall

OBJS := via.o


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


all :	objs $(OBJDIR)/via

objs:
	mkdir objs

$(OBJDIR)/via: $(OBJS) Makefile
	$(CC) -kernel $(OBJS) -o $(OBJDIR)/via

install: all
	cp -f $(OBJDIR)/via /system/drivers/dev/net/eth/via

dist: all
	cp -f $(OBJDIR)/via $(DIST_DIR)/atheos/sys/drivers/dev/net/eth/via

clean:
	-rm -f $(OBJS) $(OBJDIR)/via

