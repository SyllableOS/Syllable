CFLAGS   += -kernel -fno-PIC -c -D__ENABLE_DEBUG__ -I.

C_INCLUDE_PATH=$(ATHEOS_INCLUDE_PATH)

SOURCE_DIR=.

OBJS := tg3.o
DRIVER := tg3

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


all:	objs $(OBJDIR)/$(DRIVER)

deps:

objs:
	mkdir objs

$(OBJDIR)/$(DRIVER): $(OBJS) Makefile
	$(CC) -kernel $(OBJS) -o $@

install: all
	cp -f $(OBJDIR)/$(DRIVER) $(IMAGE)/system/drivers/dev/net/eth/$(DRIVER)

dist: install

clean:
	-rm -f $(OBJS)
	-rm -f $(OBJDIR)/$(DRIVER)

