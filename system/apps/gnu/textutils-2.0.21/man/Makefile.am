## Process this file with automake to produce Makefile.in -*-Makefile-*-
dist_man_MANS = cat.1 cksum.1 comm.1 csplit.1 cut.1 expand.1 fmt.1 \
  fold.1 head.1 join.1 md5sum.1 nl.1 od.1 paste.1 pr.1 ptx.1 \
  sha1sum.1 sort.1 split.1 sum.1 \
  tac.1 tail.1 tr.1 tsort.1 unexpand.1 uniq.1 wc.1
man_aux = $(dist_man_MANS:.1=.x)

EXTRA_DIST = $(man_aux)
MAINTAINERCLEANFILES = $(man_MANS)

# Depend on configure.ac to get version number changes.
common_dep = $(top_srcdir)/configure.ac

cat.1:		$(common_dep)	$(srcdir)/cat.x		../src/cat.c
cksum.1:	$(common_dep)	$(srcdir)/cksum.x	../src/cksum.c
comm.1:		$(common_dep)	$(srcdir)/comm.x	../src/comm.c
csplit.1:	$(common_dep)	$(srcdir)/csplit.x	../src/csplit.c
cut.1:		$(common_dep)	$(srcdir)/cut.x		../src/cut.c
expand.1:	$(common_dep)	$(srcdir)/expand.x	../src/expand.c
fmt.1:		$(common_dep)	$(srcdir)/fmt.x		../src/fmt.c
fold.1:		$(common_dep)	$(srcdir)/fold.x	../src/fold.c
head.1:		$(common_dep)	$(srcdir)/head.x	../src/head.c
join.1:		$(common_dep)	$(srcdir)/join.x	../src/join.c
md5sum.1:	$(common_dep)	$(srcdir)/md5sum.x	../src/md5sum.c
nl.1:		$(common_dep)	$(srcdir)/nl.x		../src/nl.c
od.1:		$(common_dep)	$(srcdir)/od.x		../src/od.c
paste.1:	$(common_dep)	$(srcdir)/paste.x	../src/paste.c
pr.1:		$(common_dep)	$(srcdir)/pr.x		../src/pr.c
ptx.1:		$(common_dep)	$(srcdir)/ptx.x		../src/ptx.c
sha1sum.1:	$(common_dep)	$(srcdir)/sha1sum.x	../src/md5sum.c
sort.1:		$(common_dep)	$(srcdir)/sort.x	../src/sort.c
split.1:	$(common_dep)	$(srcdir)/split.x	../src/split.c
sum.1:		$(common_dep)	$(srcdir)/sum.x		../src/sum.c
tac.1:		$(common_dep)	$(srcdir)/tac.x		../src/tac.c
tail.1:		$(common_dep)	$(srcdir)/tail.x	../src/tail.c
tr.1:		$(common_dep)	$(srcdir)/tr.x		../src/tr.c
tsort.1:	$(common_dep)	$(srcdir)/tsort.x	../src/tsort.c
unexpand.1:	$(common_dep)	$(srcdir)/unexpand.x	../src/unexpand.c
uniq.1:		$(common_dep)	$(srcdir)/uniq.x	../src/uniq.c
wc.1:		$(common_dep)	$(srcdir)/wc.x		../src/wc.c

SUFFIXES = .x .1

.x.1:
	@echo "Updating man page $@"; \
	PATH=../src:$$PATH; export PATH; \
	$(HELP2MAN) \
	    --include=$(srcdir)/$*.x \
	    --output=$@ $*
