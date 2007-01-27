CFLAGS   += -kernel -fno-PIC -c

C_INCLUDE_PATH=$(ATHEOS_INCLUDE_PATH)

OBJS := pcnet32.o
DRIVER := pcnet32

OBJDIR	:= objs
OBJS	:= $(addprefix $(OBJDIR)/,$(OBJS))

$(OBJDIR)/%.o : %.c
	$(CC) $(CFLAGS) $< -o $@

$(OBJDIR)/%.o : %.cpp
	$(CXX) $(CFLAGS) $< -o $@


$(OBJDIR)/%.o : %.s
	$(CC) $(CFLAGS) -x assembler-with-cpp $< -o $@

$(OBJDIR)/%.o : %.S
	$(CC) $(CFLAGS) -x assembler-with-cpp $< -o $@


all :	objs $(OBJDIR)/pcnet32

deps:

objs:
	mkdir objs

$(OBJDIR)/pcnet32: $(OBJS) Makefile
	$(CC) -kernel $(OBJS) -o $(OBJDIR)/pcnet32

install: all
	cp -f $(OBJDIR)/pcnet32 $(IMAGE)/system/drivers/dev/net/eth/pcnet32

dist: install

clean:
	-rm -f $(OBJS)
	-rm -f $(OBJDIR)/$(DRIVER)

