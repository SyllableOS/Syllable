## Process this file with automake to produce Makefile.in -*-Makefile-*-
AUTOMAKE_OPTIONS = ansi2knr

EXTRAdir = $(bindir)
EXTRA_PROGRAMS = chroot hostid nice stty uname uptime su
EXTRA_SCRIPTS = nohup

bin_SCRIPTS = groups @OPTIONAL_BIN_ZCRIPTS@

bin_PROGRAMS = basename date dirname echo env expr factor false \
  hostname id logname pathchk pinky printenv printf pwd seq \
  sleep tee test true tty users who whoami yes @OPTIONAL_BIN_PROGS@

datadir = $(prefix)/@DATADIRNAME@
localedir = $(datadir)/locale

noinst_HEADERS = sys2.h system.h
EXTRA_DIST = ansi2knr.1 ansi2knr.c \
  groups.sh nohup.sh
CLEANFILES = $(SCRIPTS) su

INCLUDES = -I.. -I$(srcdir) -I$(top_srcdir)/lib -I../intl
DEFS = -DLOCALEDIR=\"$(localedir)\" @DEFS@
LDADD = @INTLLIBS@ ../lib/libsu.a

# For sqrt
factor_LDADD = $(LDADD) @SQRT_LIBM@
factor_DEPENDENCIES = $(LDADD)

# If necessary, add -lm to resolve use of pow in lib/strtod.c.
printf_LDADD = $(LDADD) @POW_LIBM@
printf_DEPENDENCIES = $(LDADD)

# For floor, rint, modf
seq_LDADD = $(LDADD) @SEQ_LIBM@
seq_DEPENDENCIES = $(LDADD)

uptime_LDADD = $(LDADD) @GETLOADAVG_LIBS@
uptime_DEPENDENCIES = $(LDADD)

su_LDADD = $(LDADD) @LIB_CRYPT@
su_DEPENDENCIES = $(LDADD)

$(PROGRAMS): ../lib/libsu.a

$(SCRIPTS): Makefile

SUFFIXES = .sh

.sh:
	rm -f $@ $@-t
	sed \
	  -e 's!@''bindir''@!@bindir@!' \
	  -e 's/@''GNU_PACKAGE''@/@GNU_PACKAGE@/' \
	  -e 's/@''VERSION''@/@VERSION@/' $< > $@-t
	chmod +x $@-t
	mv $@-t $@

all-local: su

installed_su = $(bindir)/`echo su|sed '$(transform)'`

setuid_root_mode = a=rx,u+s

INSTALL_SU = \
  p=su; \
  echo "  $(INSTALL_PROGRAM) $$p $(installed_su)"; \
  $(INSTALL_PROGRAM) $$p $(installed_su); \
  echo "  chown root $(installed_su)"; \
  chown root $(installed_su); \
  echo "  chmod $(setuid_root_mode) $(installed_su)"; \
  chmod $(setuid_root_mode) $(installed_su)

install-root: su
	@$(INSTALL_SU)

install-exec-local: su
	@TMPFILE=$(bindir)/.su-$$$$; \
	rm -f $$TMPFILE; \
	echo > $$TMPFILE; \
## See if we can create a setuid root executable in $(bindir).
## If not, then don't even try to install su.
	can_create_suid_root_executable=no; \
	chown root $$TMPFILE > /dev/null 2>&1 \
	  && chmod $(setuid_root_mode) $$TMPFILE > /dev/null 2>&1 \
	  && can_create_suid_root_executable=yes; \
	rm -f $$TMPFILE; \
	if test $$can_create_suid_root_executable = yes; then \
	  $(INSTALL_SU); \
	else \
	  echo "WARNING: insufficient access; not installing su"; \
	  echo "NOTE: to install su, run 'make install-root' as root"; \
	fi

uninstall-local:
# Remove su only if it's one we installed.
	@if grep 'GNU sh-utils' $(installed_su) > /dev/null 2>&1; then \
	  echo "  rm -f $(installed_su)"; \
	  rm -f $(installed_su); \
	else :; fi

BUILT_SOURCES = false.c

false.c: true.c
	sed \
	  -e s/true/false/g \
	  -e s/success/failure/g \
	  -e s/EXIT_SUCCESS/EXIT_FAILURE/g \
	  $(srcdir)/true.c > $@-t
	mv $@-t $@
