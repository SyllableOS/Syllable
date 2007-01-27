CFLAGS   += -kernel -fno-PIC -c
AFLAGS   = -c

C_INCLUDE_PATH=$(ATHEOS_INCLUDE_PATH)

# Enable the following to enable debug output
CFLAGS += -D__ENABLE_DEBUG__

OBJS := ata.o ata_port.o ata_probe.o ata_io.o ata_command.o ata_drive.o atapi_drive.o atapi.o

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




all :	objs $(OBJDIR)/ata 

objs:
	mkdir objs

$(OBJDIR)/ata: $(OBJS) Makefile
	$(CC) -kernel $(OBJS) -o $(OBJDIR)/ata

install: all
	cp -f $(OBJDIR)/ata $(IMAGE)/system/drivers/dev/bus/ata

dist: install

clean:
	-rm -f $(OBJS)
	-rm -f $(OBJDIR)/ata

copy: all
	cp -f $(OBJDIR)/ata /home/root/Build/

