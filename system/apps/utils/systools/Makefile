CFLAGS	+= -c
CXXFLAGS += -c
OBJDIR = objs

EXES = $(OBJDIR)/init $(OBJDIR)/ps $(OBJDIR)/ls_ports $(OBJDIR)/ls_sema	\
	$(OBJDIR)/top $(OBJDIR)/nice $(OBJDIR)/semctl $(OBJDIR)/shutdown \
	$(OBJDIR)/strace $(OBJDIR)/sysinfo $(OBJDIR)/unbuffer mkhome kill_all

init_OBJS     = init.o
ps_OBJS       = ps.o
ls_ports_OBJS = ls_ports.o
ls_sema_OBJS  = ls_sema.o
top_OBJS      = top.o
nice_OBJS     = nice.o
semctl_OBJS   = semctl.o
shutdown_OBJS = shutdown.o
strace_OBJS   = strace.o
sysinfo_OBJS  = sysinfo.o
unbuffer_OBJS = unbuffer.o

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
unbuffer_OBJS	:= $(addprefix $(OBJDIR)/,$(unbuffer_OBJS))

include $(ATHEOS_SRC)/makefile.rules

all:  $(OBJDIR) $(EXES)

$(OBJDIR):
	-mkdir $(OBJDIR)


$(OBJDIR)/init: $(init_OBJS)
	$(CC) $(init_OBJS) -o $@
$(OBJDIR)/ps: $(ps_OBJS)
	$(CC) $(ps_OBJS) -o $@
$(OBJDIR)/ls_ports: $(ls_ports_OBJS)
	$(CXX) $(ls_ports_OBJS) -lsyllable -o $@
$(OBJDIR)/ls_sema: $(ls_sema_OBJS)
	$(CXX) $(ls_sema_OBJS) -lsyllable -o $@
$(OBJDIR)/top: $(top_OBJS)
	$(CC) $(top_OBJS) -o $@
$(OBJDIR)/nice: $(nice_OBJS)
	$(CC) $(nice_OBJS) -o $@
$(OBJDIR)/semctl: $(semctl_OBJS)
	$(CC) $(semctl_OBJS) -o $@
$(OBJDIR)/shutdown: $(shutdown_OBJS)
	$(CXX) $(shutdown_OBJS) -lsyllable -o $@
$(OBJDIR)sysinfo: $(sysinfo_OBJS)
	$(CC) $(sysinfo_OBJS) -o $@
$(OBJDIR)/strace: $(strace_OBJS)
	$(CC) $(strace_OBJS) -o $@
$(OBJDIR)/unbuffer: $(unbuffer_OBJS)
	$(CC) $(unbuffer_OBJS) -lutil -o $@

deps:

install:
	cp -f $(EXES) /system/programs/
	ln -s /system/programs/shutdown /system/programs/reboot
	chmod +x /system/programs/$(EXES)

dist:
	cp -f $(EXES) $(IMAGE)/system/programs/
	ln -s /system/programs/shutdown $(IMAGE)/system/programs/reboot
	chmod +x $(IMAGE)/system/programs/mkhome

clean:
	-rm -Rv $(OBJDIR)
