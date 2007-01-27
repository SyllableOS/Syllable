CFLAGS   += -kernel -fno-PIC -c -I../
AFLAGS   = -c

C_INCLUDE_PATH=$(ATHEOS_INCLUDE_PATH)

OBJS := audio.o hda_codec.o hda.o


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




all :	objs $(OBJDIR)/hda

objs:
	mkdir objs

$(OBJDIR)/hda: $(OBJS) Makefile
	$(CC) -kernel $(OBJS) -o $(OBJDIR)/hda

install: all
	cp -f $(OBJDIR)/hda $(IMAGE)/system/drivers/dev/audio/

dist: install

clean:
	-rm -f $(OBJS) $(OBJDIR)/hda

