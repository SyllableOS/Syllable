CFLAGS   = -kernel -fno-PIC -O3 -c -Wall
AFLAGS   = -c

OBJS := ac97_codec.o i810_audio.o
DRIVER := i810_audio

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




all :	objs $(OBJDIR)/i810_audio

deps:

objs:
	mkdir objs

$(OBJDIR)/i810_audio: $(OBJS) Makefile
	gcc -kernel $(OBJS) -o $(OBJDIR)/i810_audio

install: all
	cp -f $(OBJDIR)/$(DRIVER) /system/drivers/dev/sound/$(DRIVER)

dist: all
	cp -f $(OBJDIR)/$(DRIVER) $(DIST_DIR)/atheos/sys/drivers/dev/sound/$(DRIVER)

clean:
	-rm -f $(OBJS)
	-rm -f $(OBJDIR)/$(DRIVER)

