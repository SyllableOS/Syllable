CFLAGS	= -O3 -c
CPPFLAGS= -O3 -c
OBJDIR = objs

EXES	= $(OBJDIR)/init $(OBJDIR)/ps $(OBJDIR)/ls_ports $(OBJDIR)/ls_sema	\
	  $(OBJDIR)/top $(OBJDIR)/reboot $(OBJDIR)/nice $(OBJDIR)/semctl	\
	  $(OBJDIR)/shutdown $(OBJDIR)/strace $(OBJDIR)/sysinfo

init_OBJS     = init.o
ps_OBJS       = ps.o
ls_ports_OBJS = ls_ports.o
ls_sema_OBJS  = ls_sema.o
top_OBJS      = top.o
reboot_OBJS   = reboot.o
nice_OBJS     = nice.o
semctl_OBJS   = semctl.o
shutdown_OBJS = shutdown.o
strace_OBJS   = strace.o
sysinfo_OBJS  = sysinfo.o

init_OBJS	:= $(addprefix $(OBJDIR)/,$(init_OBJS))
ps_OBJS		:= $(addprefix $(OBJDIR)/,$(ps_OBJS))
ls_ports_OBJS	:= $(addprefix $(OBJDIR)/,$(ls_ports_OBJS))
ls_sema_OBJS	:= $(addprefix $(OBJDIR)/,$(ls_sema_OBJS))
top_OBJS	:= $(addprefix $(OBJDIR)/,$(top_OBJS))
reboot_OBJS	:= $(addprefix $(OBJDIR)/,$(reboot_OBJS))
nice_OBJS	:= $(addprefix $(OBJDIR)/,$(nice_OBJS))
semctl_OBJS	:= $(addprefix $(OBJDIR)/,$(semctl_OBJS))
shutdown_OBJS	:= $(addprefix $(OBJDIR)/,$(shutdown_OBJS))
strace_OBJS	:= $(addprefix $(OBJDIR)/,$(strace_OBJS))
sysinfo_OBJS	:= $(addprefix $(OBJDIR)/,$(sysinfo_OBJS))

CC	= gcc
LL	= gcc

include $(ATHEOS_SRC)/makefile.rules

all:  $(OBJDIR) $(EXES)

$(OBJDIR):
	-mkdir $(OBJDIR)


$(OBJDIR)/init: $(init_OBJS)
	$(CC) $(init_OBJS) -o $@
$(OBJDIR)/ps: $(ps_OBJS)
	$(CC) $(ps_OBJS) -o $@
$(OBJDIR)/ls_ports: $(ls_ports_OBJS)
	$(CC) $(ls_ports_OBJS) -o $@
$(OBJDIR)/ls_sema: $(ls_sema_OBJS)
	g++ $(ls_sema_OBJS) -latheos -o $@
$(OBJDIR)/top: $(top_OBJS)
	$(CC) $(top_OBJS) -o $@
$(OBJDIR)/reboot: $(reboot_OBJS)
	$(CC) $(reboot_OBJS) -o $@
$(OBJDIR)/nice: $(nice_OBJS)
	$(CC) $(nice_OBJS) -o $@
$(OBJDIR)/semctl: $(semctl_OBJS)
	$(CC) $(semctl_OBJS) -o $@
$(OBJDIR)/shutdown: $(shutdown_OBJS)
	$(CC) $(shutdown_OBJS) -o $@
$(OBJDIR)sysinfo: $(sysinfo_OBJS)
	$(CC) $(sysinfo_OBJS) -o $@
$(OBJDIR)/strace: $(strace_OBJS)
	$(CC) $(strace_OBJS) -o $@

deps:

install:
	cp -f $(EXES) /bin/

dist:
	cp -f $(EXES) $(DIST_DIR)/atheos/sys/bin/

clean:
	-rm -Rv $(OBJDIR)

