CFLAGS   += -kernel -fno-PIC -c

C_INCLUDE_PATH=$(ATHEOS_INCLUDE_PATH)

OBJS := ne2000.o


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


all :	objs $(OBJDIR)/ne2000

objs:
	mkdir objs

$(OBJDIR)/ne2000: $(OBJS) Makefile
	$(CC) -kernel $(OBJS) -o $(OBJDIR)/ne2000

install: all
	cp -f $(OBJDIR)/ne2000 $(IMAGE)/system/drivers/dev/net/eth/8390

dist: install

clean:
	-rm -f $(OBJS) $(OBJDIR)/ne2000

