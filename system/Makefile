# Root Makefile for Syllable

include Makeconf

# Normal rules

SUBDIRS := build-tools libs sys apps other
include recurse.rules

# High-level targets

# The distribution target sets up the distribution tree,
# copies in the non-buildable files I.E. Fonts etc. &
# then runs "make dist" on the directories that we have
# built

distribution:
	@echo Running pre-build cleanup
	@$(BUILD_TOOLS)/prebuild.sh

	@echo Building distribution in $(DIST_DIR);\echo
	@$(BUILD_TOOLS)/mkdisttree.sh $(DIST_DIR)

	@echo;\echo Making Deps;\echo
	@$(MAKE) deps

	@echo;\echo Making all;\echo
	@$(MAKE)

	@echo;\echo Building distribution;\echo
	@$(MAKE) dist

	@echo;echo Finalising and cleaning up;\echo
	@$(BUILD_TOOLS)/finalise.sh

	@echo;\echo ################;\echo Stick a fork in it!;\echo ################;\echo

assemble-dist:
	@echo;\echo Building distribution;\echo
	@$(MAKE) dist

	@echo;echo Finalising and cleaning up;\echo
	@$(BUILD_TOOLS)/finalise.sh

	@echo;\echo ################;\echo Stick a fork in it!;\echo ################;\echo

