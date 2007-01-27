CFLAGS   += -kernel -fno-PIC -c

C_INCLUDE_PATH=$(ATHEOS_INCLUDE_PATH)

SOURCE_DIR=.

OBJS := 3c59x.o
DRIVER := 3c59x

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


all :	objs $(OBJDIR)/3c59x

deps:

objs:
	mkdir objs

$(OBJDIR)/3c59x: $(OBJS) Makefile
	$(CC) -kernel $(OBJS) -o $(OBJDIR)/3c59x

install: all
	cp -f $(OBJDIR)/3c59x $(IMAGE)/system/drivers/dev/net/eth/3c59x

dist: install

clean:
	-rm -f $(OBJS)
	-rm -f $(OBJDIR)/$(DRIVER)

