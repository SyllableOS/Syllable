# -*- makefile -*-
#
# Main Makefile for building the Qt library, examples and tutorial.
# Read PORTING for instructions how to port Qt to a new platform.

SHELL=/bin/sh

init: FORCE
	@$(MAKE) QTDIR=`pwd` all

all: symlinks  src-moc src-mt sub-src sub-tools sub-tutorial sub-examples
	@echo
	@echo "The Qt library is now built in ./lib"
	@echo "The Qt examples are built in the directories in ./examples"
	@echo "The Qt tutorials are built in the directories in ./tutorial"
	@echo
	@echo 'Note: be sure to set $$QTDIR to point to here or to wherever'
	@echo '      you move these directories.'
	@echo
	@echo "Enjoy!   - the Trolltech team"
	@echo

src-moc: .buildopts symlinks  FORCE
	cd src/moc; $(MAKE)
	-rm -f bin/moc
	cp src/moc/moc bin/moc

sub-tools: sub-src FORCE
	cd tools; $(MAKE)

symlinks: .buildopts
	@cd include; rm -f q*.h; ln -s $(QTDIR)/src/*/q*.h .; ln -s $(QTDIR)/extensions/*/src/q*.h .; rm -f q*_p.h

sub-src: src-moc src-mt .buildopts FORCE
	cd src; $(MAKE)

src-mt: src-moc .buildopts FORCE
	$(MAKE) -f src-mt.mk

sub-tutorial: sub-src FORCE
	cd tutorial; $(MAKE)

sub-examples: sub-src FORCE
	cd examples; $(MAKE)

clean:
	-rm .buildopts
	cd src/moc; $(MAKE) clean
	cd src; $(MAKE) clean
	-rm src/tmp/*.o src/tmp/*.a src/allmoc.cpp
	-find src/3rdparty -name '*.o' | xargs rm
	cd tutorial; $(MAKE) clean
	cd examples; $(MAKE) clean
	cd tools; $(MAKE) clean

depend:
	cd src; $(MAKE) depend
	cd tutorial; $(MAKE) depend
	cd examples; $(MAKE) depend

.buildopts: Makefile
	@echo
	@echo '  Qt must first be configured using the "configure" script.'
	@echo
	@echo '  The make process will now run this...'
	@echo
	@./configure -frommake

dep: depend

FORCE:


