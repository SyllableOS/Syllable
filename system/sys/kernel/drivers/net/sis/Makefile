CFLAGS   = -kernel -fno-PIC -c -O3 -Wall

SOURCE_DIR=.

C_INCLUDE_PATH=/system/include

OBJS := sis900.o
DRIVER := sis900

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


all :	objs $(OBJDIR)/sis900

deps:

objs:
	mkdir objs

$(OBJDIR)/sis900: $(OBJS) Makefile
	gcc -kernel $(OBJS) -o $(OBJDIR)/sis900

install: all
	cp -f $(OBJDIR)/sis900 /system/drivers/dev/net/eth/sis900

dist: all
	cp -f $(OBJDIR)/$(DRIVER) $(DIST_DIR)/atheos/sys/drivers/dev/net/eth/$(DRIVER)

clean:
	-rm -f $(OBJS)
	-rm -f $(OBJDIR)/$(DRIVER)

