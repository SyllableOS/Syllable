CFLAGS   = -kernel -fno-PIC -c -O3 -Wall -D__ENABLE_DEBUG__

SOURCE_DIR=.

OBJS := dp83815.o
DRIVER := dp83815

OBJDIR	:= objs
OBJS	:= $(addprefix $(OBJDIR)/,$(OBJS))

$(OBJDIR)/%.o : $(SOURCE_DIR)/%.c
	$(CC) $(CFLAGS) $< -o $@

$(OBJDIR)/%.o : $(SOURCE_DIR)/%.cpp
	$(CC) $(CFLAGS) $< -o $@


$(OBJDIR)/%.o : $(SOURCE_DIR)/%.s
	$(CC) $(CFLAGS) -x assembler-with-cpp $< -o $@

$(OBJDIR)/%.o : $(SOURCE_DIR)/%.S
	$(CC) $(CFLAGS) -x assembler-with-cpp $< -o $@


all :	objs $(OBJDIR)/dp83815

deps:

objs:
	mkdir objs

$(OBJDIR)/dp83815: $(OBJS) Makefile
	$(CC) -kernel $(OBJS) -o $(OBJDIR)/dp83815

install: all
	cp -f $(OBJDIR)/$(DRIVER) /system/drivers/dev/net/eth/$(DRIVER)

dist: all
	cp -f $(OBJDIR)/$(DRIVER) $(DIST_DIR)/atheos/sys/drivers/dev/net/eth/$(DRIVER)

clean:
	-rm -f $(OBJS)
	-rm -f $(OBJDIR)/$(DRIVER)

