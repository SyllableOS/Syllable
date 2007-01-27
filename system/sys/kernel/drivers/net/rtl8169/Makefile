CFLAGS   += -kernel -fno-PIC -c -D__ENABLE_DEBUG__

C_INCLUDE_PATH=$(ATHEOS_INCLUDE_PATH)

SOURCE_DIR=.

OBJS := r8169.o
DRIVER := r8169

OBJDIR	:= objs
OBJS	:= $(addprefix $(OBJDIR)/,$(OBJS))

$(OBJDIR)/%.o : $(SOURCE_DIR)/%.c
	$(CC) $(CFLAGS) $< -o $@

$(OBJDIR)/%.o : $(SOURCE_DIR)/%.cpp
	$(CXX) $(CFLAGS) $< -o $@


$(OBJDIR)/%.o : $(SOURCE_DIR)/%.s
	$(CC) $(CFLAGS) -x assembler-with-cpp $< -o $@

$(OBJDIR)/%.o : $(SOURCE_DIR)/%.S
	$(CC) $(CFLAGS) -x assembler-with-cpp $< -o $@


all:	objs $(OBJDIR)/r8169

deps:

objs:
	mkdir objs

$(OBJDIR)/r8169: $(OBJS) Makefile
	$(CC) -kernel $(OBJS) -o $(OBJDIR)/r8169

install: all
	cp -f $(OBJDIR)/$(DRIVER) $(IMAGE)/system/drivers/dev/net/eth/$(DRIVER)

dist: install

clean:
	-rm -f $(OBJS)
	-rm -f $(OBJDIR)/$(DRIVER)

