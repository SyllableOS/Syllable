CFLAGS   = -kernel -fno-PIC -O3 -c -Wall
AFLAGS   = -c

OBJS := ac97_codec.o vt82xx_io.o vt82xx.o


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




all :	objs $(OBJDIR)/vt82xx

objs:
	mkdir objs

$(OBJDIR)/vt82xx: $(OBJS) Makefile
	gcc -kernel $(OBJS) -o $(OBJDIR)/vt82xx

install: all
	mkdir -p /system/drivers/dev/sound/
	cp -f $(OBJDIR)/vt82xx /system/drivers/dev/sound/vt82xx


clean:
	rm $(OBJS)
	rm objs/vt82xx





