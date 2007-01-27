CFLAGS   += -kernel -fno-PIC -c
AFLAGS   = -c

C_INCLUDE_PATH=$(ATHEOS_INCLUDE_PATH)

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
	$(CXX) $(CFLAGS) $< -o $@

$(OBJDIR)/%.o : %.s
	$(CC) $(AFLAGS) -x assembler-with-cpp $< -o $@

$(OBJDIR)/%.o : %.S
	$(CC) $(CFLAGS) -x assembler-with-cpp $< -o $@




all :	objs $(OBJDIR)/i810_audio

deps:

objs:
	mkdir objs

$(OBJDIR)/i810_audio: $(OBJS) Makefile
	$(CC) -kernel $(OBJS) -o $(OBJDIR)/i810_audio

install: all
	cp -f $(OBJDIR)/$(DRIVER) $(IMAGE)/system/drivers/dev/sound/$(DRIVER)

dist: install

clean:
	-rm -f $(OBJS)
	-rm -f $(OBJDIR)/$(DRIVER)

