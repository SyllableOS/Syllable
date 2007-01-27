CFLAGS   += -kernel -fno-PIC -c

C_INCLUDE_PATH=$(ATHEOS_INCLUDE_PATH)

SOURCE_DIR=.

OBJS := forcedeth.o
DRIVER := forcedeth

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


all :	objs $(OBJDIR)/forcedeth

deps:

objs:
	mkdir objs

$(OBJDIR)/forcedeth: $(OBJS) Makefile
	$(CC) -kernel $(OBJS) -o $(OBJDIR)/forcedeth

install: all
	cp -f $(OBJDIR)/forcedeth $(IMAGE)/system/drivers/dev/net/eth/forcedeth

dist: install

clean:
	-rm -f $(OBJS)
	-rm -f $(OBJDIR)/$(DRIVER)

