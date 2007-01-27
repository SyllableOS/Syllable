CFLAGS   += -kernel -fno-PIC -c
AFLAGS   = -c

C_INCLUDE_PATH=$(ATHEOS_INCLUDE_PATH)

OBJS := ac97_codec.o vt82xx_io.o vt82xx.o
DRIVER := vt82xx

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




all :	objs $(OBJDIR)/vt82xx

objs:
	mkdir objs

$(OBJDIR)/vt82xx: $(OBJS) Makefile
	$(CC) -kernel $(OBJS) -o $(OBJDIR)/vt82xx

install: all
	cp -f $(OBJDIR)/vt82xx $(IMAGE)/system/drivers/dev/sound/vt82xx

dist: install

clean:
	rm -f $(OBJS)
	rm -f objs/vt82xx

