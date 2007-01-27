CFLAGS   += -kernel -fno-PIC -c
AFLAGS   = -c

C_INCLUDE_PATH=$(ATHEOS_INCLUDE_PATH)

OBJS := driver.o


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




all :	objs $(OBJDIR)/vmware

objs:
	mkdir objs

$(OBJDIR)/vmware: $(OBJS) Makefile
	$(CC) -kernel $(OBJS) -o $(OBJDIR)/vmware

install: all
	cp -f $(OBJDIR)/vmware $(IMAGE)/system/drivers/dev/graphics/

dist: install

clean:
	-rm -f $(OBJS) $(OBJDIR)/vmware

