CFLAGS   += -kernel -fno-PIC -c

C_INCLUDE_PATH=$(ATHEOS_INCLUDE_PATH)

OBJS := via.o


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


all :	objs $(OBJDIR)/via

objs:
	mkdir objs

$(OBJDIR)/via: $(OBJS) Makefile
	$(CC) -kernel $(OBJS) -o $(OBJDIR)/via

install: all
	cp -f $(OBJDIR)/via $(IMAGE)/system/drivers/dev/net/eth/via

dist: install

clean:
	-rm -f $(OBJS) $(OBJDIR)/via

