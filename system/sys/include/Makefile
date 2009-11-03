DIST_TARGET_PATH := $(IMAGE)/system/development/headers

all:

deps:
	ln -sf atheos syllable

install:
	@cp -rp * $(DIST_TARGET_PATH)/
	@ln -sf atheos $(DIST_TARGET_PATH)/syllable
	rm -rf $(DIST_TARGET_PATH)/CVS
	rm $(DIST_TARGET_PATH)/Makefile

doc:

install-doc:

dist: install

clean:
