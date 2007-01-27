CFLAGS   += -kernel -fno-PIC -c -I../
AFLAGS   = -c

C_INCLUDE_PATH=$(ATHEOS_INCLUDE_PATH)

OBJS := audio.o ac97audio.o i8xx.o


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




all :	objs $(OBJDIR)/i8xx

objs:
	mkdir objs

$(OBJDIR)/i8xx: $(OBJS) Makefile
	$(CC) -kernel $(OBJS) -o $(OBJDIR)/i8xx

install: all
	cp -f $(OBJDIR)/i8xx $(IMAGE)/system/drivers/dev/audio/

dist: install

clean:
	-rm -f $(OBJS) $(OBJDIR)/i8xx

