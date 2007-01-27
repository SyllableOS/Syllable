CFLAGS   += -kernel -fno-PIC -c -I. -D__ENABLE_DEBUG__
#CFLAGS   += -DDEBUG_LIMIT=KERN_INFO
AFLAGS   = -c

C_INCLUDE_PATH=$(ATHEOS_INCLUDE_PATH)

OBJS := maestro.o
DRIVER := maestro

OBJDIR	:= objs
OBJS	:= $(addprefix $(OBJDIR)/,$(OBJS))

#
#	rules
#

$(OBJDIR)/%.o : %.c
	$(CC) $(CFLAGS) $< -o $@

$(OBJDIR)/%.o : %.cpp
	$(CXX) $(CFLAGS) $< -o $@

$(OBJDIR)/%.o : %.s
	$(CC) $(AFLAGS) -x assembler-with-cpp $< -o $@

$(OBJDIR)/%.o : %.S
	$(CC) $(CFLAGS) -x assembler-with-cpp $< -o $@

#
# targets
#

all :	objs $(OBJDIR)/$(DRIVER)

deps:

objs:
	mkdir $(OBJDIR)

$(OBJDIR)/$(DRIVER): $(OBJS) Makefile
	$(CC) -kernel $(OBJS) -o $@

install: all
	cp -f $(OBJDIR)/$(DRIVER) $(IMAGE)/system/drivers/dev/audio/$(DRIVER)

dist: install

clean:
	-rm -f $(OBJS)
	-rm -f $(OBJDIR)/$(DRIVER)

