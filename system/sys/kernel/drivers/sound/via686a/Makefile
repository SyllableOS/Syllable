CFLAGS   = -kernel -fno-PIC -O3 -c -Wall
AFLAGS   = -c

C_INCLUDE_PATH=$(ATHEOS_INCLUDE_PATH)

OBJS := ac97_codec.o via686a_io.o via686a.o
DRIVER := via686a

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




all :	objs $(OBJDIR)/via686a

deps:

objs:
	mkdir objs

$(OBJDIR)/via686a: $(OBJS) Makefile
	$(CC) -kernel $(OBJS) -o $(OBJDIR)/via686a

install: all
	cp -f $(OBJDIR)/$(DRIVER) /system/drivers/dev/sound/$(DRIVER)

dist: all
	cp -f $(OBJDIR)/$(DRIVER) $(DIST_DIR)/atheos/sys/drivers/dev/sound/$(DRIVER)

clean:
	-rm -f $(OBJS)
	-rm -f $(OBJDIR)/$(DRIVER)

