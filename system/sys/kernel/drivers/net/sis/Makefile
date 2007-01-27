CFLAGS   += -kernel -fno-PIC -c

C_INCLUDE_PATH=$(ATHEOS_INCLUDE_PATH)

SOURCE_DIR=.

OBJS := sis900.o
DRIVER := sis900

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


all :	objs $(OBJDIR)/sis900

deps:

objs:
	mkdir objs

$(OBJDIR)/sis900: $(OBJS) Makefile
	$(CC) -kernel $(OBJS) -o $(OBJDIR)/sis900

install: all
	cp -f $(OBJDIR)/sis900 $(IMAGE)/system/drivers/dev/net/eth/sis900

dist: install

clean:
	-rm -f $(OBJS)
	-rm -f $(OBJDIR)/$(DRIVER)

