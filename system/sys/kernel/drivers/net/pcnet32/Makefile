
CFLAGS   = -kernel -fno-PIC -c -O3

C_INCLUDE_PATH=/ainc

OBJS := pcnet32.o
DRIVER := pcnet32

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


all :	objs $(OBJDIR)/pcnet32

deps:

objs:
	mkdir objs

$(OBJDIR)/pcnet32: $(OBJS) Makefile
	gcc -kernel $(OBJS) -o $(OBJDIR)/pcnet32

install: all
	cp -f $(OBJDIR)/pcnet32 /system/drivers/dev/net/eth/pcnet32

dist: all
	cp -f $(OBJDIR)/$(DRIVER) /system/drivers/dev/net/eth/$(DRIVER)

clean:
	-rm $(OBJS)
	-rm $(OBJDIR)/$(DRIVER)

