CFLAGS   = -kernel -fno-PIC -c -O3 -Wall
AFLAGS   = -c

C_INCLUDE_PATH=$(ATHEOS_INCLUDE_PATH)

OBJS := kbd.o


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




all :	objs $(OBJDIR)/keybd

objs:
	mkdir objs

$(OBJDIR)/keybd: $(OBJS) Makefile
	$(CC) -kernel $(OBJS) -o $(OBJDIR)/keybd

install: all
	cp -f $(OBJDIR)/keybd /system/drivers/dev/keybd

dist: all
	cp -f $(OBJDIR)/keybd $(DIST_DIR)/atheos/sys/drivers/dev/keybd

clean:
	-rm -f $(OBJS) $(OBJDIR)/keybd

