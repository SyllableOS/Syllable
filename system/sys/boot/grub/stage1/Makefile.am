pkgdatadir = $(datadir)/$(PACKAGE)/$(host_cpu)-$(host_vendor)
nodist_pkgdata_DATA = stage1

CLEANFILES = $(nodist_pkgdata_DATA)

# We can't use builtins or standard includes.
AM_CFLAGS = $(STAGE1_CFLAGS) -fno-builtin -nostdinc
LDFLAGS = -nostdlib -Wl,-N,-Ttext,7C00

noinst_PROGRAMS = stage1.exec
stage1_exec_SOURCES = stage1.S stage1.h

%: %.exec
	$(OBJCOPY) -O binary -R .hash -R .dynsym -R .dynstr -R .rel.text -R .rel.data -R .rel.got -R .got -R .dynamic $< $@
