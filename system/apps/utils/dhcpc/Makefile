CFLAGS += -c -I.

OBJDIR = objs

OBJS = client dhcp packet interface state inbound ntp
OBJS := $(addprefix $(OBJDIR)/,$(addsuffix .o,$(OBJS)))

EXE = dhcpc

# The following debug levels are currently defined:
#
# 0 (INFO)
# 1 (ERROR)
# 2 (WARNING)
# 3 (PANIC)
#
# You can also have dhcpc dump raw (hex) packet data by defining the PKT_TRACE constant.
# Until I am sure that dhcpc is working O.K, the debug level is set to 0, without packet dumps.

CFLAGS += -DENABLE_DEBUG -DDEBUG_LEVEL=0

all: $(OBJDIR) $(OBJS)
	$(CC) $(OBJS) -o $(EXE)

install: all
	install -s $(EXE) $(IMAGE)/system/bin/$(EXE)

dist: install

clean:
	-rm $(EXE)
	-rm $(OBJS)

$(OBJDIR):
	-mkdir $(OBJDIR)

$(OBJDIR)/%.o : %.c
	@echo Compiling : $<
	@$(CC) $(CFLAGS) $< -o $@

