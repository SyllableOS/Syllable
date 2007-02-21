CFLAGS	= -c

OBJDIR = objs
EXES	= $(OBJDIR)/fsprobe $(OBJDIR)/mount $(OBJDIR)/unmount $(OBJDIR)/format $(OBJDIR)/addattrib $(OBJDIR)/lsattribs $(OBJDIR)/mkindex $(OBJDIR)/devstat $(OBJDIR)/rmattrib $(OBJDIR)/dumpattrib $(OBJDIR)/df

fsprobe_OBJS    = fsprobe.o
mount_OBJS      = mount.o
unmount_OBJS    = unmount.o
format_OBJS     = format.o
addattrib_OBJS  = addattrib.o
lsattribs_OBJS  = lsattribs.o
rmattrib_OBJS   = rmattrib.o
mkindex_OBJS    = mkindex.o
devstat_OBJS    = devstat.o
dumpattrib_OBJS = dumpattrib.o
df_OBJS			= df.o

fsprobe_OBJS	:= $(addprefix $(OBJDIR)/,$(fsprobe_OBJS))
mount_OBJS	:= $(addprefix $(OBJDIR)/,$(mount_OBJS))
unmount_OBJS	:= $(addprefix $(OBJDIR)/,$(unmount_OBJS))
format_OBJS	:= $(addprefix $(OBJDIR)/,$(format_OBJS))
addattrib_OBJS	:= $(addprefix $(OBJDIR)/,$(addattrib_OBJS))
lsattribs_OBJS	:= $(addprefix $(OBJDIR)/,$(lsattribs_OBJS))
rmattrib_OBJS	:= $(addprefix $(OBJDIR)/,$(rmattrib_OBJS))
mkindex_OBJS	:= $(addprefix $(OBJDIR)/,$(mkindex_OBJS))
devstat_OBJS	:= $(addprefix $(OBJDIR)/,$(devstat_OBJS))
dumpattrib_OBJS	:= $(addprefix $(OBJDIR)/,$(dumpattrib_OBJS))
df_OBJS			:= $(addprefix $(OBJDIR)/,$(df_OBJS))

include $(ATHEOS_SRC)/makefile.rules

all:  $(OBJDIR) $(EXES)

$(OBJDIR):
	-mkdir $(OBJDIR)

$(OBJDIR)/fsprobe : $(fsprobe_OBJS)
	$(CC) $(fsprobe_OBJS) -o $@
$(OBJDIR)/mount : $(mount_OBJS)
	$(CC) $(mount_OBJS) -o $@
$(OBJDIR)/unmount : $(unmount_OBJS)
	$(CC) $(unmount_OBJS) -o $@
$(OBJDIR)/format: $(format_OBJS)
	$(CC) $(format_OBJS) -o $@
$(OBJDIR)/addattrib: $(addattrib_OBJS)
	$(CC) $(addattrib_OBJS) -o $@
$(OBJDIR)/rmattrib: $(rmattrib_OBJS)
	$(CC) $(rmattrib_OBJS) -o $@
$(OBJDIR)/lsattribs: $(lsattribs_OBJS)
	$(CC) $(lsattribs_OBJS) -o $@
$(OBJDIR)/mkindex: $(mkindex_OBJS)
	$(CC) $(mkindex_OBJS) -o $@
$(OBJDIR)/devstat: $(devstat_OBJS)
	$(CC) $(devstat_OBJS) -o $@
$(OBJDIR)/dumpattrib: $(dumpattrib_OBJS)
	$(CC) $(dumpattrib_OBJS) -o $@
$(OBJDIR)/df: $(df_OBJS)
	$(CC) $(df_OBJS) -o $@

deps:

install: all
	cp -f $(EXES) /system/bin/

dist: all
	cp -f $(EXES) $(IMAGE)/system/bin/

clean:
	-rm -Rv $(OBJDIR)

