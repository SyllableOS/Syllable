CFLAGS   += -kernel -fno-PIC -c

C_INCLUDE_PATH=$(ATHEOS_INCLUDE_PATH)

OBJS := rtl8139.o


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


all :	objs $(OBJDIR)/rtl8139

objs:
	mkdir objs

$(OBJDIR)/rtl8139: $(OBJS) Makefile
	$(CC) -kernel $(OBJS) -o $(OBJDIR)/rtl8139

install: all
	cp -f $(OBJDIR)/rtl8139 $(IMAGE)/system/drivers/dev/net/eth/rtl8139

dist: install

clean:
	-rm -f $(OBJS) $(OBJDIR)/rtl8139

