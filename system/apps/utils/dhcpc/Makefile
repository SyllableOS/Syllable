CC := gcc
LL := gcc

COPTS := -Wall -O3 -c
LOPTS :=

OBJDIR := objs

OBJS := client.o dhcp.o packet.o interface.o state.o inbound.o
OBJS := $(addprefix $(OBJDIR)/,$(OBJS))

EXE := dhcpc

# The following debug levels are currently defined:
#
# 0 (INFO)
# 1 (ERROR)
# 2 (WARNING)
# 3 (PANIC)
#
# You can also have dhcpc dump raw (hex) packet data by defining the PKT_TRACE constant.
# Until I am sure that dhcpc is working O.K, the debug level is set to 0, without packet dumps.

COPTS += -DENABLE_DEBUG -DDEBUG_LEVEL=0
#COPTS += -DPKT_TRACE

all: $(OBJDIR) $(OBJS)
	$(LL) $(LOPTS) $(OBJS) -o $(EXE)

install: all
	cp -f $(EXE) /system/bin/

dist: all
	install -s $(EXE) $(DIST_DIR)/atheos/sys/bin/$(EXE)

clean:
	-rm $(EXE)
	-rm $(OBJS)

$(OBJDIR):
	-mkdir $(OBJDIR)

$(OBJDIR)/client.o: client.c dhcp.h
	$(CC) client.c $(COPTS) -o $@

$(OBJDIR)/dhcp.o: dhcp.c dhcp.h debug.h
	$(CC) dhcp.c $(COPTS) -o $@

$(OBJDIR)/packet.o: packet.c packet.h
	$(CC) packet.c $(COPTS) -o $@

$(OBJDIR)/interface.o: interface.c interface.h
	$(CC) interface.c $(COPTS) -o $@

$(OBJDIR)/state.o: state.c state.h
	$(CC) state.c $(COPTS) -o $@

$(OBJDIR)/inbound.o: inbound.c inbound.h
	$(CC) inbound.c $(COPTS) -o $@

