CFLAGS   = -kernel -fno-PIC -c -O3 -Wall

SOURCE_DIR=.

C_INCLUDE_PATH=/system/include

OBJS := 3c59x.o
DRIVER := 3c59x

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


all :	objs $(OBJDIR)/3c59x

deps:

objs:
	mkdir objs

$(OBJDIR)/3c59x: $(OBJS) Makefile
	gcc -kernel $(OBJS) -o $(OBJDIR)/3c59x

install: all
	cp -f $(OBJDIR)/3c59x /system/drivers/dev/net/eth/3c59x

dist: all
	cp -f $(OBJDIR)/$(DRIVER) $(DIST_DIR)/atheos/sys/drivers/dev/net/eth/$(DRIVER)

clean:
	-rm $(OBJS)
	-rm $(OBJDIR)/$(DRIVER)

