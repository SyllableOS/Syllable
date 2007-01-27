CFLAGS   += -kernel -fno-PIC -c

C_INCLUDE_PATH=$(ATHEOS_INCLUDE_PATH)

SOURCE_DIR=.

OBJS := eepro100.o
DRIVER := eepro100

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


all :	objs $(OBJDIR)/eepro100

deps:

objs:
	mkdir objs

$(OBJDIR)/eepro100: $(OBJS) Makefile
	$(CC) -kernel $(OBJS) -o $(OBJDIR)/eepro100

install: all
	cp -f $(OBJDIR)/eepro100 $(IMAGE)/system/drivers/dev/net/eth/eepro100

dist: install

clean:
	-rm -f $(OBJS)
	-rm -f $(OBJDIR)/$(DRIVER)

