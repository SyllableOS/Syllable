DRIVER := emu10k1

all:
	@cd src/ && $(MAKE)

deps:

clean:
	@cd src/ && $(MAKE) clean

install:
	cp -f $(DRIVER) /atheos/system/drivers/dev/sound/$(DRIVER)

dist:
	cp -f $(DRIVER) $(DIST_DIR)/atheos/sys/drivers/dev/sound/$(DRIVER)

