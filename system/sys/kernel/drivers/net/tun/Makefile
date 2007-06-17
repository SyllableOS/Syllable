CFLAGS   += -kernel -fno-PIC -c -D__ENABLE_DEBUG__ -I.

C_INCLUDE_PATH=$(ATHEOS_INCLUDE_PATH)

SOURCE_DIR=.

OBJS := tun_if.o tun_dev.o
DRIVER := tun

OBJDIR	:= objs
OBJS	:= $(addprefix $(OBJDIR)/,$(OBJS))

$(OBJDIR)/%.o : $(SOURCE_DIR)/%.c
	$(CC) $(CFLAGS) $< -o $@

all:	objs $(OBJDIR)/$(DRIVER)

deps:

objs:
	mkdir objs

$(OBJDIR)/$(DRIVER): $(OBJS) Makefile
	$(CC) -kernel $(OBJS) -o $@

install: all
	cp -f $(OBJDIR)/$(DRIVER) $(IMAGE)/system/drivers/dev/net/$(DRIVER)

dist: install

clean:
	-rm -f $(OBJS)
	-rm -f $(OBJDIR)/$(DRIVER)

